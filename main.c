
    #include <stdio.h>
    #include <stdlib.h>
    #include <stdint.h>
    #include <string.h>
    #include <math.h>
    #include <immintrin.h>	
    #include <windows.h>    
    #include "portaudio.h"
    
    #include "console.c"
    #define PRINT console_print

	#include "threads.c"
	#include "conf.c"
	
    // ------------------------------------------------------------------------------------------------------------ //
	// ------------------------------------------ malloc ---------------------------------------------------------- //

    void * getmem( size_t count ){
        void *p = malloc( count );
        if( !p ){ MessageBox( GetActiveWindow(), "Out of memory", "Out of memory", MB_OK ); exit(1); };
		memset( p, 0, count );
        return p; }

    void *aligned_malloc( size_t bytes, size_t alignment ) {
        void *p1, *p2; p1 = getmem( bytes + alignment + sizeof(size_t) );
        size_t addr = (size_t)p1 +alignment +sizeof(size_t);
        p2 = (void *)(addr -(addr%alignment));
        *((size_t *)p2-1) = (size_t)p1;
        return p2; }

    void aligned_free( void *p ){
        free((void *)(*((size_t *) p-1))); }

	// ------------------------------------------------------------------------------------------------------------ //
	// ------------------------------------------ clock ----------------------------------------------------------- //

	double clock_tick_in_samples;
	int64_t clock_count_first;  // ??

	void clock_init( int samplerate ){
		int64_t freq;
		QueryPerformanceFrequency( &freq );
		clock_tick_in_samples = (double)samplerate / (double)freq;
		QueryPerformanceCounter( &clock_count_first ); }

	int64_t clock_time(){
		int64_t count;
		QueryPerformanceCounter( &count );
		count -= clock_count_first;
		return (int64_t)floor( ((double)count)*clock_tick_in_samples ); }

	#define NOW clock_time()
	
	char* timestr( int seconds ){
        static char str[30];
        int h = seconds / 3600;
        int m = seconds / 60 - h*60;
        int s = seconds -h*3600 -m*60;
        sprintf( str, "%.2d:%.2d:%.2d", h, m, s );
        return str; }

    // ############################################################################################################ //
	// ########################################## GLOBALS ######################################################### //

	// keep 3 seconds of audio (msize). 1 second max filter. 1 second max delay. 1 second left for buffersizes and correction headroom.
	// repeat these 3 seconds 3 times in memory and use the middle one so memcopy does not care for boundaries.
	// constantly measure both buffersizes (L) and cursor-to-end-of-input-buffer (G). should be equal.
	
	struct stat {
		int64_t t;
		int64_t avail; };

    struct port {
		int type;  // 0/1 - in/out
		int stage;  // 0 -initial; 1 -200ms passed
		int channels_count;
        int64_t t0;
		int64_t len;
		struct stat *stats;  // now to end-of-data
		int64_t min, max;
		int stats_len;
		PaStream *stream;
		};

    struct out {
        int src;
        __m256 *k;
		int kn;
		char *kname;
		}; 
	
	int samplerate;
	
	int msize;  // 3 seconds
	int ssize;  // max stats; samplerate*2 (ssize) so if frameCount == 1 and we stat before and after copy - we have 0.75s of stats.
	
    float *canvas;  // all inputs data
	volatile struct out *map; // output channels to input channels
	volatile struct port *ports;  // 0/1 - in/out
	
	volatile int64_t cursor;  // input lane read address in "stream-time"
	
	volatile float L;  // buffersizes
	volatile struct stat *lstat;  
	volatile int lstat_len;

	volatile float G;  // cursor to end-of-data
	volatile struct stat *gstat;  
	volatile int gstat_len;
	
	volatile float dith_sig;  // cursor direction. samples per second. pos/neg. 
	volatile int64_t dith_t;  // last correction time
	volatile int64_t dith_p;  // correction period [samples]
	
	int jobs_per_channel; 

    // ############################################################################################################ //
	// ########################################## FILTERS ######################################################### //
	
	struct op_clear_arg { void *p; int len; };		
	void op_clear( void *task ){ struct op_clear_arg *t = (struct op_clear_arg *) task;
		memset( t->p, 0, t->len * sizeof(float) ); }

	struct op_copy_arg { void *dst, *src; int len; };	
	void op_copy( void *task ) { struct op_copy_arg *t = (struct op_copy_arg *) task;
		memcpy( t->dst, t->src, t->len * sizeof(float) ); }
		
	struct convolve_task {
        float *in;
        float *out;
        int len;
        __m256* k;
        int kn;
	};

    #define ALIGNMENT 32
    
    void set_filter( int out, float *k, int kn, char *kname ){
        if( map[out].k ) { aligned_free( map[out].k ); map[out].k = 0; }
		if( !k ) return;
        map[out].kname = kname;
		map[out].kn = ( kn / 32 ) * 32 + 32; // filter len multiple of 32 samples
		map[out].k = aligned_malloc( map[out].kn * sizeof(float) * 8, ALIGNMENT );
        memset( map[out].k, 0, map[out].kn * sizeof(float) * 8 );
        for(int i=0; i<kn; i++) map[out].k[i] = _mm256_broadcast_ss( k +kn -i -1 );
        }

    #define SSE_SIMD_LENGTH 4
    #define AVX_SIMD_LENGTH 8
    #define VECTOR_LENGTH 16
	
    void convolve( void *task ){
		float* in = ((struct convolve_task*)task)->in;
		float* out = ((struct convolve_task*)task)->out;
		int length = ((struct convolve_task*)task)->len;
		__m256* kernel = ((struct convolve_task*)task)->k;
		int kernel_length = ((struct convolve_task*)task)->kn;
		in = in -kernel_length +1;  // scipy convolve mode valid
		length = length +kernel_length -1;  // see in orig repo
        // original code: github/hgomersall/SSE-convolution/convolve.c
        // convolve_avx_unrolled_vector_unaligned_fma
        __m256 data_block __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc0 __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc1 __attribute__ ((aligned (ALIGNMENT)));
        for(int i=0; i<length-kernel_length; i+=VECTOR_LENGTH){
            acc0 = _mm256_setzero_ps();
            acc1 = _mm256_setzero_ps();
            for(int k=0; k<kernel_length; k+=VECTOR_LENGTH){
                int data_offset = i + k;
                for (int l = 0; l < SSE_SIMD_LENGTH; l++){
                    for (int m = 0; m < VECTOR_LENGTH; m+=SSE_SIMD_LENGTH) {
                        data_block = _mm256_loadu_ps(in + l + data_offset + m);
                        acc0 = _mm256_fmadd_ps(kernel[k+l+m], data_block, acc0);
                        data_block = _mm256_loadu_ps(in + l + data_offset + m + AVX_SIMD_LENGTH);
                        acc1 = _mm256_fmadd_ps(kernel[k+l+m], data_block, acc1); } } }
            _mm256_storeu_ps(out+i, acc0);
            _mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); }
        int i = length - kernel_length;
        out[i] = 0.0;
        for(int k=0; k<kernel_length; k++){
            out[i] += in[i+k] * (*((float*)(kernel +k ))); } }    


    // ############################################################################################################ //
	// ########################################## MAIN ############################################################ //


	void _makestat( volatile struct port * p ){

		int64_t now = NOW;
		
		if( p->stats_len == 0 )
			p->t0 = now;
		
		// add stat
		int i = p->stats_len % ssize;
		p->stats[i].t = now;
		p->stats[i].avail = p->t0 + p->len -now;  // now to end-of-data (available)
		p->stats_len ++;

		if( now - p->t0 > samplerate/5 )  // 200ms
			p->stage = 1;

		// update min/max
		int64_t v, min = INT64_MAX; int64_t max = INT64_MIN;
		for( int si = p->stats_len-1;
			si >= 0 
			&& p->stats[si%ssize].t > now-samplerate*3  // 3 sec
			&& p->stats[si%ssize].t > p->t0 +samplerate/10  // skip first 100ms
			;si-- ){  // upto 3sec
			v = p->stats[si%ssize].avail;
			if( v < min ) min = v;
			if( v > max ) max = v; }
		p->min = min;
		p->max = max;
		now = NOW;

		if( ports[0].stage < 1 || ports[1].stage < 1 )
			return;
		
		// calc L
		int x = lstat_len;
		lstat[x%ssize].t = now;
		lstat[x%ssize].avail = (ports[0].max-ports[0].min) + (ports[1].max-ports[1].min);
		lstat_len = ++x;
		int64_t sum = 0, len = 0;
		for( int si = x-1;
			si >= 0 
			&& lstat[si%ssize].t > now-samplerate*3 
			//&& lstat[si%ssize].t > now -samplerate*3   // todo: skip first 100ms; make lstat_t0
			; si-- ){  // upto 3sec
			sum += lstat[si%ssize].avail;
			len += 1; }
		L = (float)sum / (float)len;
		now = NOW;

		if( cursor < 0 ){
			
			// set cursor
			cursor = now -ports[0].t0 +ports[0].min -(int)(L*0.66); //-(ports[1].max -ports[1].min)*2;
			dith_t = now;
			if( cursor >= 0 ){
				PRINT("%s init %d \r\n", timestr((now -ports[0].t0)/samplerate), cursor ); }
		}
	
		else {

			// calc G
			int x = gstat_len;
			gstat[x%ssize].t = now;
			gstat[x%ssize].avail = ports[0].len -cursor;
			gstat_len = ++x;
			int64_t sum = 0, len = 0;
			for( int si = x-1; si >= 0 && gstat[si%ssize].t > now-samplerate*3; si-- ){  // upto 3sec; todo: gstat_t0 skip 100ms
				sum += gstat[si%ssize].avail;
				len += 1; }
			G = (float)sum / (float)len;
			now = NOW;

			// dith
			dith_sig = (L-G) / 10.0;  // G to L for 1 second (speed)
			//if( cursor > samplerate )
			//	dith_sig /= 10.0 ;
			dith_p = (int64_t)( (float)samplerate / ( dith_sig > 0.0 ? dith_sig : -dith_sig ) );
			if( dith_t + dith_p < now ){
				if( dith_sig < 0 ){
					cursor += 1;
					// todo: remove 1 from all stats so dith sig is lower next ime
					PRINT("%s skipped 1 sample %d \r\n", timestr((now -ports[0].t0)/samplerate), cursor ); 
					}
				else {
					cursor -= 1;
					// todo: add 1 to all stats so dith sig is bigger next ime
					PRINT("%s replayed 1 sample %d \r\n", timestr((now -ports[0].t0)/samplerate), cursor );
					}
				dith_t = now;
			}
		}
	}
		

    PaStreamCallbackResult _tick(
        float **input,
        float **output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo *timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userdata ){

		struct op_clear_arg clear_task;
		struct op_copy_arg  copy_task;
		struct convolve_task T;

        if( statusFlags ){
            if( paInputUnderflow & statusFlags ) PRINT( "Input Underflow \n" );
            if( paInputOverflow & statusFlags ) PRINT( "Input Overflow \n" );
            if( paOutputUnderflow & statusFlags ) PRINT( "Output Underflow \n" );
            if( paOutputOverflow & statusFlags ) PRINT( "Output Overflow \n" );
            if( paPrimingOutput & statusFlags ) PRINT( "Priming Output \n" );}

        if( input ){
			
			_makestat( ports );

            // write
            int ofs = ports[0].len % msize;
            for( int i=0; i<ports[0].channels_count; i++ )
                for( int j=0; j<3; j++ )
                    memcpy( canvas +i*msize*3 +j*msize +ofs, input[i], frameCount*sizeof(float) );
				
            ports[0].len += frameCount;			
			_makestat( ports ); }
                
        if( output ){		
		
			_makestat( ports+1 );

			if( cursor > 0 ){
				if( cursor + frameCount > ports[0].len ) PRINT( "GLITCH %d \r\n", cursor -ports[0].len );
				if( cursor < ports[0].len -msize ) PRINT( "GLITCH %d \r\n", ports[0].len -msize -cursor ); }

			// write
			int jlen = frameCount / jobs_per_channel;
			int rem = frameCount % jobs_per_channel;                        
			for( int j = 0; j<jobs_per_channel; j++ )
				for( int i=0; i<ports[1].channels_count; i++ ){
					if( cursor < 0 || map[i].src == -1 ){
						// clear
						clear_task.p = output[i] +j*jlen;
						clear_task.len = (j == jobs_per_channel-1) ? jlen+rem : jlen;
						threads_submit( &op_clear, &clear_task, sizeof(clear_task) );
					}
					else if( cursor >= 0 && map[i].src >= 0 && !map[i].k ){
						// copy
						copy_task.dst = output[i] +j*jlen;
						copy_task.src = canvas + map[i].src*msize*3 +msize + cursor%msize +j*jlen;
						copy_task.len = (j == jobs_per_channel-1) ? jlen+rem : jlen;
						threads_submit( &op_copy, &copy_task, sizeof(copy_task)  );
					}
					else {
						// convolve
						T.in = canvas + map[i].src*msize*3 +msize + cursor%msize +j*jlen;
						T.out = output[i] +j*jlen;
						T.len = (j == jobs_per_channel-1) ? jlen+rem : jlen;
						T.k = map[i].k;
						T.kn = map[i].kn;
						threads_submit( &convolve, &T, sizeof(T) );
					}
				}
			threads_wait();
			if( cursor >= 0 )
				cursor += frameCount;

            ports[1].len += frameCount;			
            _makestat( ports+1 );
			}

        return paContinue; }


	int init(){
		if( Pa_Initialize() ) PRINT( "ERROR: Could not initialize PortAudio. \n" );
        if( Pa_GetDeviceCount() <= 0 ) PRINT( "ERROR: No Devices Found. \n" );        
		samplerate = msize = ssize = canvas = ports = map = gstat = gstat_len = jobs_per_channel = 0;
		cursor = -1; }


    int start( int input_device_id, int output_device_id, int sr, int tc ){  // -> bool
		int in = Pa_GetDeviceInfo( input_device_id )->maxInputChannels;
		int on = Pa_GetDeviceInfo( output_device_id )->maxOutputChannels;
		cursor = -1;
		samplerate = sr;
		msize = sr * 3;
		ssize = sr * 2;
		canvas = getmem( sizeof(float) * msize * 3 * in );
		ports = getmem( sizeof(struct port) * 2 );
		map = getmem( sizeof(struct out) * on ); for( int i=0; i<on; i++ ) { map[i].src = -1; map[i].k = 0; }
		lstat = getmem( sizeof(struct stat) * ssize ); lstat_len = 0;		
		gstat = getmem( sizeof(struct stat) * ssize ); gstat_len = 0;		
		ports[0].channels_count = in;
		ports[0].stats = getmem( sizeof(struct stat) * ssize );
		ports[1].type = 1;
		ports[1].channels_count = on;
		ports[1].stats = getmem( sizeof(struct stat) * ssize );
		clock_init( sr );
		threads_init( tc ); jobs_per_channel = (int)ceil( ((float)tc)/((float)on) );		
        for( int i=0; i<2; i++ ){						
            PRINT( "starting %s ... ", ( i==0 ? "input" : "output" ) );
			 
            int device_id = ( i==0 ? input_device_id : output_device_id );
            const PaDeviceInfo *device_info = Pa_GetDeviceInfo( device_id );
            static PaStreamParameters params;
            PaStream **stream = &(ports[i].stream);
            
            params.device = device_id;
            params.sampleFormat = paFloat32|paNonInterleaved;
            params.hostApiSpecificStreamInfo = 0;            
            params.suggestedLatency = ( i==0 ? device_info->defaultLowInputLatency : device_info->defaultLowOutputLatency );
            params.channelCount = ( i==0 ? device_info->maxInputChannels : device_info->maxOutputChannels );
            
            PaError err = Pa_OpenStream(
                stream,
                i==0 ? &params : 0,
                i==0 ? 0 : &params,
                samplerate,
                paFramesPerBufferUnspecified,
                paClipOff | paDitherOff | paPrimeOutputBuffersUsingStreamCallback, // paNeverDropInput is for full-duplex only
                &_tick,
                0 );

            if( err != paNoError ){
                if( err != paUnanticipatedHostError ) {
                    PRINT( "ERROR 1: %s \n", Pa_GetErrorText( err ) );
                    return 0; }
                else {
                    const PaHostErrorInfo* herr = Pa_GetLastHostErrorInfo();
                    PRINT( "ERROR 2: %s \n", herr->errorText );
                    return 0; }}
			
            err = Pa_StartStream( *stream );
            if( err != paNoError ){
                PRINT( "ERROR 3: %s \n", Pa_GetErrorText( err ) );
                return 0; }

			PRINT( "ok \r\n" );

            }

        // done
        return 1; }

    // ############################################################################################################ //
    // ########################################## GUI ############################################################# //
    // ############################################################################################################ //

    HWND hwnd;
    MSG msg;

    #define R1 (1)
    #define R2 (2)
    #define R3 (3)
    #define R4 (4)
    #define LL (5)
    #define BTN01 (121)
    #define BTN02 (122)
    #define CMB1 (555)
    #define CMB2 (556)
    #define BTN1 (123)
    #define LB1 (110)
    #define CB1 (220)
    #define CB2 (330)
    #define EDT (400)

    HWND hr1, hr2, hr3, hr4, hLL, hbctl1, hbctl2, hCombo1, hCombo2, hBtn;
    HWND cbs[10];
    HWND cbs2[10];
    HWND hEdit;

    LOGFONT font1 = {0}, font2 = {0}, font3 = {0};
    HFONT hfont1, hfont2, hfont3;	

	// ------------------------------------------------------------------------------------------------------------ //

	void draw();
    void CALLBACK every_5ms( HWND hwnd, UINT uMsg, UINT timerId, DWORD dwTime ){
		if( console_changed ){
			console_changed = 0;
			SetWindowText( hEdit, console ); }
		draw();
        if( cursor > -1 ){
            char txt[50]; sprintf( txt, "Load: %d%% \n", (int)ceil((Pa_GetStreamCpuLoad(ports[0].stream)+Pa_GetStreamCpuLoad(ports[1].stream))*200.0) ); // 50% == 100%
            SetWindowText( hLL, txt ); } }

	// ------------------------------------------------------------------------------------------------------------ //

    char * names = 0;
    char * device_name( int device_id ){
        if( !names ){ 
            names = getmem( Pa_GetDeviceCount() * 200 );
            memset( names, 0, Pa_GetDeviceCount() * 200 );
            char drv_name[50] = "", dev_name[150] = "";
            for( int i=0; i<Pa_GetDeviceCount(); i++ ){
                strcpy( dev_name, Pa_GetDeviceInfo( i )->name );
                strcpy( drv_name, Pa_GetHostApiInfo( Pa_GetDeviceInfo( i )->hostApi )->name );
                sprintf( names +i*200, "  %s  /  %s  ", strstr( drv_name, "Windows" ) ? drv_name+8 : drv_name, dev_name ); } }
        return names +device_id*200; }

    int device_id( char * device_name ){        
        for( int i=0; i<Pa_GetDeviceCount(); i++ )
            if( strcmp( names +i*200, device_name ) == 0 )
                return i;
        return -1; }

	// ------------------------------------------------------------------------------------------------------------ //

    struct filter {
        char *name;
        float* k;
        int kn; 
    };
    
    struct filter ** filters = 0; // null terminated list of pointers
    
    struct filter * filter_p( char * name ){
        for( int i=0; filters[i]; i++ )
            if( strcmp( filters[i]->name, name ) == 0 )
                return filters[i];
        return 0; }

    int filter_i( char * name ){
        for( int i=0; filters[i]; i++ )
            if( strcmp( filters[i]->name, name ) == 0 )
                return i;
        return -1; }

    void load_filters( int max_len ){  // TODO: open one file only; rm FindFirstFile(); load_filter(i, filename, max_len)
		if( filters ){
			for( int i=0; filters[i]; i++ ){
				free( filters[i]->name );
				free( filters[i]->k ); }
			free( filters ); filters = 0; }
		filters = getmem( sizeof(struct filter *) * 100 );
		// add 'None' - unity kernel
        filters[0] = getmem( sizeof( struct filter ) );
		filters[0]->name = getmem( 5 ); strcpy( filters[0]->name, "None" );
        filters[0]->k = getmem( sizeof(float) * 1 );
        filters[0]->k[0] = 1.0;
        filters[0]->kn = 1;        
		// add files
        WIN32_FIND_DATA r;
        HANDLE h = FindFirstFile( "filters\\*", &r ); if( h == INVALID_HANDLE_VALUE ) return;
        int i = 1;
        do if( !(r.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && r.cFileName[0] != '.' ){
            char fname[100] = ""; sprintf( fname, "filters\\%s", r.cFileName );
            FILE *f = fopen( fname, "r" );
            if( !f ) continue;
            int count = 0;
            float num; while( fscanf( f, "%f", &num ) == 1 ) count ++;
            if( count == 0 ) continue;
            if( count > max_len ){
                PRINT( "NOT loaded %s exceeds %d taps \r\n", r.cFileName, max_len );
                continue; }
            fseek( f, 0, SEEK_SET );
            float *data = malloc( sizeof(float) * count );
            for( int j=0; j<count; j++ )
                fscanf( f, "%f", data +j );
            fclose(f);
            // push to list
            filters[i] = getmem( sizeof( struct filter ) );
            filters[i]->name = getmem( strlen( fname )+1 );
            strcpy( filters[i]->name, r.cFileName );
            filters[i]->k = data;
            filters[i]->kn = count;
            i++;
        } while( FindNextFile( h, &r ) );
        FindClose(h);
        PRINT( "loaded %d filters \r\n", i-1 );
    }

	// ------------------------------------------------------------------------------------------------------------ //

    int devices_as_in_conf(){ // whether the two devices selected are as in the conf
        char a[250], b[250];
        if( !conf_get( "device1" ) || !conf_get( "device2" ) ) return 0;
        GetDlgItemText( hwnd, CMB1, a, 250);
        GetDlgItemText( hwnd, CMB2, b, 250);
        return strcmp( a, conf_get( "device1" ) ) == 0 && strcmp( b, conf_get( "device2" ) ) == 0; }

    int samplerate_as_in_conf(){ // whether the selected samplerate is as in the conf
        if( !conf_get( "samplerate" ) ) return 0;
        int samplerate_from_conf = -1;
        return sscanf( conf_get( "samplerate" ), "%d", &samplerate_from_conf ) == 1 && samplerate_from_conf == samplerate; }

    void show_conf(){ // sets the bottom combos as in conf or clear them; routing if devices are same and filters if devs&SR are same

		// fill left combos
		char txt1[250], txt2[250];
		GetDlgItemText( hwnd, CMB1, txt1, 250 );
		GetDlgItemText( hwnd, CMB2, txt2, 250 );
		for( int i=0; i<Pa_GetDeviceInfo( device_id( txt2 ) )->maxOutputChannels; i++ ){
			SendMessage( cbs[i], CB_RESETCONTENT, (WPARAM)0, (LPARAM)0 );
			SendMessage( cbs[i], CB_ADDSTRING, (WPARAM)0, (LPARAM)"" );
			SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
			for( int j=0; j<Pa_GetDeviceInfo( device_id( txt1 ) )->maxInputChannels; j++ ){
				char n[3]; sprintf( n, "%d", j+1 );
				SendMessage( cbs[i], CB_ADDSTRING, (WPARAM)0, (LPARAM)n ); } }

		// fill right combos
		for( int i=0; i<10; i++ ){
			SendMessage( cbs2[i], CB_RESETCONTENT, (WPARAM)0, (LPARAM)0 );
			SendMessage( cbs2[i], CB_ADDSTRING, (WPARAM)0, (LPARAM)"" );
			for( int j=0; filters[j]; j++ ) SendMessage( cbs2[i], CB_ADDSTRING, 0, filters[j]->name );
			SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 ); }
			
        if( !devices_as_in_conf() ){ // clear selections
            for( int i=0; i<10; i++ ){
                SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
                SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 ); }
			return;
            }

        char key[20]; int val;
        for( int i=0; i<10; i++ ){
            sprintf( key, "out%d", i+1 );
            if( conf_get( key ) && sscanf( conf_get( key ), "%d", &val ) == 1 )
                SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)val, (LPARAM)0 );
            else SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
            sprintf( key, "filter%d", i+1 );
            if( samplerate_as_in_conf() && conf_get( key ) && filter_p( conf_get( key ) ) )
                SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)(filter_i( conf_get( key ) )+1), (LPARAM)0 );
            else SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 ); } }

    void save_conf(){
        char txt[250], key[20];
        sprintf( txt, "%d", samplerate );
        conf_set( "samplerate", txt );
        GetDlgItemText( hwnd, CMB1, txt, 250 );
        conf_set( "device1", txt );
        GetDlgItemText( hwnd, CMB2, txt, 250 );
        conf_set( "device2", txt );
        for( int i=0; i<10; i++ ){
            sprintf( key, "out%d", i+1 );
            GetDlgItemText( hwnd, CB1+i, txt, 250 );
            conf_set( key, txt );
            sprintf( key, "filter%d", i+1 );
            GetDlgItemText( hwnd, CB2+i, txt, 250 );
            conf_set( key, txt ); }
        conf_save(); }

    int get_cpu_cores(){
        typedef BOOL (WINAPI *LPFN_GLPI)( PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);        
        LPFN_GLPI glpi = GetProcAddress( GetModuleHandle("kernel32"), "GetLogicalProcessorInformation");        
        if( !glpi ) { return 1; } // winxp?
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer, ptr = NULL;
        DWORD returnLength = 0;
        glpi( buffer, &returnLength );
        buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION) getmem( returnLength );
        BOOL r = glpi( buffer, &returnLength );
        if( !r ) { return 1; } // ???
        ptr = buffer;
        DWORD byteOffset = 0;
        int processorCoreCount = 0;
        while( byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength ){
            if( ptr->Relationship == RelationProcessorCore ) processorCoreCount++;
            byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
            ptr ++; }
        return processorCoreCount; }

    LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ){
        if( msg == WM_COMMAND ){

            if( BN_CLICKED == HIWORD(wParam) && LOWORD(wParam) <= R4 ){  // samplerate clicked

				switch( LOWORD(wParam) ){
                    case R1: samplerate = 44100; break;
                    case R2: samplerate = 48000; break;
                    case R3: samplerate = 96000; break;
                    case R4: samplerate = 192000; }
					
				load_filters( samplerate ); // maxlen 1s
				show_conf();

            } else if( BN_CLICKED == HIWORD(wParam) && (LOWORD(wParam) == BTN01 || LOWORD(wParam) == BTN02) ){  //  config device clicked
                char txt[250];
                GetDlgItemText( hwnd, LOWORD(wParam) == BTN01 ? CMB1 : CMB2, txt, 250 );
                if( strstr( Pa_GetHostApiInfo( Pa_GetDeviceInfo( device_id( txt ) )->hostApi )->name, "ASIO" ) != 0 ){
                    PaAsio_ShowControlPanel( device_id( txt ), hwnd );
                } else {
                    STARTUPINFO si; memset( &si, 0, sizeof(STARTUPINFO) );
                    PROCESS_INFORMATION pi; memset( &pi, 0, sizeof(PROCESS_INFORMATION) );
                    si.cb = sizeof(STARTUPINFO);
                    char cmd[300] = "";
                    GetWindowsDirectory( cmd, 300 );
                    strcat( cmd, "\\System32\\control.exe mmsys.cpl,," );
                    strcat( cmd, (LOWORD(wParam) == BTN01) ? "1" : "0" );
                    CreateProcessA( 0, cmd, 0, 0, 0, 0, 0, 0, &si, &pi ); }

            } else if( CBN_SELCHANGE == HIWORD(wParam) && LOWORD(wParam) == CMB1 ){  // device 1 changed
				show_conf();
				
            } else if( CBN_SELCHANGE == HIWORD(wParam) && LOWORD(wParam) == CMB2 ){  // device 2 changed
				show_conf();
                
            } else if( LOWORD(wParam) == BTN1 ){  // play clicked
                int sd, dd;
                char txt[250];
                GetDlgItemText( hwnd, CMB1, txt, 250 );
                sd = device_id( txt );
                GetDlgItemText( hwnd, CMB2, txt, 250 );
                dd = device_id( txt );
                if( start( sd, dd, samplerate, get_cpu_cores() ) ){                                   // START
                    EnableWindow( hr1, 0 );
                    EnableWindow( hr2, 0 );
                    EnableWindow( hr3, 0 );
                    EnableWindow( hr4, 0 );
                    EnableWindow( hbctl1, 0 );
                    EnableWindow( hbctl2, 0 );
                    EnableWindow( hCombo1, 0 );
                    EnableWindow( hCombo2, 0 );
                    EnableWindow( hBtn, 0 );
					show_conf();
                    for( int i=0; i<ports[1].channels_count; i++ ){
                        if( i < 10 ){
                            EnableWindow( cbs[i], 1 );
                            for( int j=0; j<ports[0].channels_count; j++ ){
                                char key[6]; int val;
                                sprintf( key, "out%d", i+1 );
                                if( devices_as_in_conf() && conf_get( key ) && sscanf( conf_get( key ), "%d", &val ) == 1 )
                                    map[i].src = val-1;
                                sprintf( key, "filter%d", i+1 );
                                if( samplerate_as_in_conf() && devices_as_in_conf() && conf_get( key ) && filter_p( conf_get( key ) )){
									struct filter *fl = filter_p( conf_get( key ) );
                                    set_filter( i, fl->k, fl->kn, fl->name );
                                    SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)(filter_i( conf_get( key ) )+1), (LPARAM)0 ); } }
                            SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)map[i].src+1, (LPARAM)0 );
                            EnableWindow( cbs2[i], 1 ); } }
                    }

            } else if( (LOWORD(wParam) >= CB1) && LOWORD(wParam) <= CB1+10 && CBN_SELCHANGE == HIWORD(wParam) ){
                int out = LOWORD(wParam) - CB1;                
                char  txt[20];
                GetDlgItemText( hwnd, CB1+out, txt, 20 );      
				if( txt[0] == 0 ){
					map[out].src = -1;
                    PRINT( "out %d: removed source \r\n", out+1 );
				}else {
					int chan = txt[0]-48-1;
					map[out].src = chan;
					PRINT( "out %d: set source %d \r\n", out+1, chan+1 );
			    }

                
            } else if( (LOWORD(wParam) >= CB2) && LOWORD(wParam) <= CB2+10 && CBN_SELCHANGE == HIWORD(wParam) ){                
                int out = LOWORD(wParam) - CB2;                
                char txt[100];
				GetDlgItemText( hwnd, CB2+out, txt, 100 );
				if( txt[0] == 0 ){
					set_filter( out, 0, 0, 0 );
					PRINT( "out %d: removed filter \r\n", out );
				} else {
					struct filter *fl = filter_p( txt );
					set_filter( out, fl->k, fl->kn, fl->name );            
					PRINT( "out %d: set filter %s \r\n", out, txt );                
				}
            }
        }

        else if( msg == WM_CLOSE ){
            if( cursor > 0 ) save_conf();
            DestroyWindow( hwnd ); }

        else if( msg == WM_DESTROY ){
            PostQuitMessage( 0 ); }

        return DefWindowProc( hwnd, msg, wParam, lParam ); }

	// ---------------------------- draw ------------------------------------------------------
	HDC hdc, hdcMem;
	RECT rc;
	HBITMAP hbmp;
	void ** pixels;
    int VPw = 40000;        // ViewPort width
    int VPh = 2500;        // ViewPort height
    int VPx = 0;           // ViewPort pos x corner
    int VPy = 0;           // ViewPort pos y corner
	POINT points[192000*2];
	void draw_prepare(){
        BITMAPINFO bmi;
        memset( &bmi, 0, sizeof(bmi) );
        bmi.bmiHeader.biSize = sizeof(bmi);
        bmi.bmiHeader.biWidth = 200;
        bmi.bmiHeader.biHeight =  -200;         // Order pixels from top to bottom
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;             // last byte not used, 32 bit for alignment
        bmi.bmiHeader.biCompression = BI_RGB;
        hdc = GetDC( hwnd );
        hbmp = CreateDIBSection( hdc, &bmi, DIB_RGB_COLORS, &pixels, 0, 0 );
        hdcMem = CreateCompatibleDC( hdc );
        SelectObject( hdcMem, hbmp );
        SetBkMode( hdcMem, TRANSPARENT ); }
    void transform_point( POINT *p ){
		double Qw = ((double)200)/((double)VPw);
        double Qh = ((double)200)/((double)VPh);        
        p->x = (long)round( (p->x - VPx) * Qw );
        p->y = (long)round( (p->y - VPy) * Qh );
        p->y = 200 - p->y; }
    void place_marker( int val, COLORREF c ){
		POINT p1 = { VPx, val };
		POINT p2 = { VPx+VPw, val };
		transform_point( &p1 );
		transform_point( &p2 );
		MoveToEx( hdcMem, p1.x, p1.y, 0 );
		SetDCPenColor(hdcMem, c);
		LineTo( hdcMem, p2.x, p2.y ); }
	void draw(){
		memset( pixels, 192, 200*200*4 );
		SelectObject( hdcMem, GetStockObject(DC_PEN) );		
		if( ports && ports[0].stats_len && ports[1].stats_len ){

			int now = NOW;
			
			// move view
			///*
			if( now + 1000 > ports[0].t0 + VPw ){  // look 1000 samples in future
				VPx = now + 1000 - VPw; }
			else { VPx = now - VPw; }			
			//*/
			
			// draw 2 plylines
			for( int i=0; i<2; i++ ){
				
				int bias = (i==0 ? -ports[0].min +(ports[1].max-ports[1].min) : -ports[1].min );
				
				if( ports[i].stats_len ){
					SetDCPenColor(hdcMem, RGB( 199-i*199, i*66 ,0 ) );
					int pi=0;
					for( int si=ports[i].stats_len-1; si>=0; si-- ) {
						points[pi].x = ports[i].stats[si%ssize].t;
						points[pi].y = ports[i].stats[si%ssize].avail + bias;
						transform_point( points+pi );
						if( points[pi].x < 0 ) break;
						pi ++; }
					Polyline( hdcMem, points, pi );				
				}

				// and 2 lines
				//place_marker( ports[0].min+bias, RGB(255, 0,0) );
				//place_marker( ports[1].max+bias, RGB(0, 99, 0) );

			}
		}

		if( gstat_len ){
			SetDCPenColor(hdcMem, RGB(0,0,99));
			int pi=0;
			for( int i=gstat_len-1; i>0; i-- ) {
				points[pi].x = gstat[i%ssize].t;
				points[pi].y = gstat[i%ssize].avail;
				transform_point( points+pi );
				if( points[pi].x < 0 ) break;
				pi ++; }
			Polyline( hdcMem, points, pi );
			place_marker( G, RGB(255,255,00) );
		}
		if( lstat_len ){
			//place_marker( L, RGB(250,50,255) );
			SetDCPenColor( hdcMem, RGB(255,255,255));
			int pi=0;
			for( int i=lstat_len-1; i>0; i-- ) {
				points[pi].x = lstat[i%ssize].t;
				points[pi].y = lstat[i%ssize].avail;
				transform_point( points+pi );
				if( points[pi].x < 0 ) break;
				pi ++; }			
			Polyline( hdcMem, points, pi );
			
		}
		place_marker( 0, RGB(0,0,0) );
		MoveToEx( hdcMem, 0, 0, 0 );
		SetDCPenColor(hdcMem, RGB(0,0,0));
		LineTo( hdcMem, 199, 0 ); LineTo( hdcMem, 199, 199 );
		LineTo( hdcMem, 0, 199 ); LineTo( hdcMem, 0, 0 );
		BitBlt( hdc, 382, 460, 200, 200, hdcMem, 0, 0, SRCCOPY );
	}
	// ----------------------------------------------------------------------------------
	

    int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow ){

		// window class
        WNDCLASSEX wc;
        memset( &wc, 0, sizeof(wc) );
        wc.cbSize = sizeof(wc);
        wc.hInstance = hInstance;
        wc.lpfnWndProc = WndProc;
        wc.lpszClassName = "mainwindow";
        wc.hbrBackground = COLOR_WINDOW; //CreateSolidBrush( RGB(64, 64, 64) );
        wc.hCursor = LoadCursor( 0, IDC_ARROW );
        wc.hIcon = LoadIcon(hInstance, "main.ico");
        RegisterClassEx(&wc);
		
		// fonts
        strcpy(font1.lfFaceName, "Wingdings");
        font1.lfCharSet = DEFAULT_CHARSET;
        hfont1 = CreateFontIndirect(&font1);
        strcpy(font2.lfFaceName, "Tahoma");
        font2.lfCharSet = DEFAULT_CHARSET;
        font2.lfHeight = 16;            
        hfont2 = CreateFontIndirect(&font2);
        strcpy(font3.lfFaceName, "Tahoma");
        font3.lfCharSet = DEFAULT_CHARSET;
        font3.lfHeight = 18;
        hfont3 = CreateFontIndirect(&font3);        

		// create
		hwnd = CreateWindowEx( WS_EX_APPWINDOW, "mainwindow", "RTFIR", WS_MINIMIZEBOX | WS_SYSMENU | WS_POPUP | WS_CAPTION, 300, 200, 600, 700, 0, 0, hInstance, 0 );        
        hr1 = CreateWindowEx( 0, "Button", "44.1k", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_AUTORADIOBUTTON|WS_GROUP, 70, 10, 50, 23, hwnd, R1, 0, 0);
        hr2 = CreateWindowEx( 0, "Button", "48k", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_AUTORADIOBUTTON, 160, 10, 50, 23, hwnd, R2, 0, 0);
        hr3 = CreateWindowEx( 0, "Button", "96k", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_AUTORADIOBUTTON, 230, 10, 50, 23, hwnd, R3, 0, 0);
        hr4 = CreateWindowEx( 0, "Button", "192k", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_AUTORADIOBUTTON, 300, 10, 50, 23, hwnd, R4, 0, 0);
        hLL = CreateWindowEx( 0, "static", "", WS_VISIBLE|WS_CHILD, 410, 13, 80, 15, hwnd, LL, NULL, NULL);        
        hbctl1 = CreateWindowEx( 0, "Button", "\x052", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON, 10, 40, 30, 23, hwnd, BTN01, NULL, NULL);
        hbctl2 = CreateWindowEx( 0, "Button", "\x052", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON, 10, 70, 30, 23, hwnd, BTN02, NULL, NULL);
        hCombo1 = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST|CBS_SORT, 48, 40, 450, 8000, hwnd, CMB1, NULL, NULL);
        hCombo2 = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST|CBS_SORT, 48, 70, 450, 8000, hwnd, CMB2, NULL, NULL);
        hBtn = CreateWindowEx( 0, "Button", "Play >", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_DEFPUSHBUTTON, 507, 40, 77, 53, hwnd, BTN1, NULL, NULL);
		hEdit = CreateWindowEx( 0, "static", "", WS_CHILD | WS_VISIBLE, 10, 460, 370, 200, hwnd, EDT, NULL, NULL);

        SendMessageA( hr1, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
        SendMessageA( hr2, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
        SendMessageA( hr3, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
        SendMessageA( hr4, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
        SendMessageA( hLL, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
        SendMessageA( hbctl1, WM_SETFONT, (WPARAM)hfont1, (LPARAM)MAKELONG(TRUE, 0));
        SendMessageA( hbctl2, WM_SETFONT, (WPARAM)hfont1, (LPARAM)MAKELONG(TRUE, 0));
        SendMessageA( hCombo1, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
        SendMessageA( hCombo2, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
        SendMessageA( hBtn, WM_SETFONT, (WPARAM)hfont3, (LPARAM)MAKELONG(TRUE, 0));
        SendMessageA( hEdit, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));

        for( int i=0; i<10; i++ ){        
            char str[7]; HANDLE h;
            sprintf( str, "out %d", i+1 );
            h = CreateWindowEx( 0, "static", str, WS_VISIBLE|WS_CHILD, 50, 135+i*30, 50, 15, hwnd, LB1+i, NULL, NULL);
            SendMessageA( h, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
            cbs[i] = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST, 100, 130+i*30, 90, 800, hwnd, CB1+i, NULL, NULL);
            SendMessageA( cbs[i], WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
            EnableWindow( cbs[i], 0 );            
            h = CreateWindowEx( 0, "static", "filter", WS_VISIBLE|WS_CHILD, 220, 135+i*30, 50, 15, hwnd, LB1+10+i, NULL, NULL);
            SendMessageA( h, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
            cbs2[i] = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST, 260, 130+i*30, 130, 800, hwnd, CB2+i, NULL, NULL);
            SendMessageA( cbs2[i], WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
			EnableWindow( cbs2[i], 0 ); }

        ShowWindow( hwnd, SW_SHOW );
		
		// init
		console_init();
		PRINT( "built " ); PRINT( __DATE__ ); PRINT( " " ); PRINT( __TIME__ ); PRINT( "\r\n" );
		init();
		conf_load( "RTFIR.conf" );

        // populate device dropdowns
        for( int i=0, ci; i<Pa_GetDeviceCount(); i++ ){
            if( Pa_GetDeviceInfo( i )->maxInputChannels ){
                ci = SendMessage( hCombo1, CB_ADDSTRING, 0, device_name( i ) );
                if( conf_get( "device1" ) && strcmp( conf_get( "device1" ), device_name( i ) ) == 0 )
                    SendMessage( hCombo1, CB_SETCURSEL, (WPARAM)ci, (LPARAM)0 ); }
            if( Pa_GetDeviceInfo( i )->maxOutputChannels ){
                ci = SendMessage( hCombo2, CB_ADDSTRING, 0, device_name( i ) );
                if( conf_get( "device2" ) && strcmp( conf_get( "device2" ), device_name( i ) ) == 0 )
                    SendMessage( hCombo2, CB_SETCURSEL, (WPARAM)ci, (LPARAM)0 ); } }
        // if no selection choose the first in the list
        if( SendMessage( hCombo1, CB_GETCURSEL, (WPARAM)0, (LPARAM)0 ) == CB_ERR )
            SendMessage( hCombo1, CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
        if( SendMessage( hCombo2, CB_GETCURSEL, (WPARAM)0, (LPARAM)0 ) == CB_ERR )
            SendMessage( hCombo2, CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );

		// populate samplerate radios
        int samplerate_from_conf = -1;
        if( conf_get( "samplerate" ) && sscanf( conf_get( "samplerate" ), "%d", &samplerate_from_conf ) == 1 ){
            switch( samplerate_from_conf ){
                case 44100: SendMessage( hr1, BM_CLICK, 0, 0 ); break;
                case 48000: SendMessage( hr2, BM_CLICK, 0, 0 ); break;
                case 96000: SendMessage( hr3, BM_CLICK, 0, 0 ); break;
                case 192000: SendMessage( hr4, BM_CLICK, 0, 0 ); }
		} else SendMessage( hr1, BM_CLICK, 0, 0 );
		
		draw_prepare();		
		SetTimer( 0, 0, 5, (TIMERPROC) &every_5ms );

        // loop
        while( GetMessage( &msg, 0, 0, 0 ) > 0 ){
            TranslateMessage( &msg );
            DispatchMessage( &msg ); }

	return msg.wParam; }
