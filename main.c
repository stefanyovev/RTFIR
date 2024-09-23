

    #define title "RTFIR"

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <math.h>
    #include <windows.h>
    #include <portaudio.h>
    #include "conf.c"

    void PRINT( char *format, ... );
    void MSGBOX( char *format, ... );

    void * getmem( long count ){
        void *p = malloc( count );
        if( !p ){ MSGBOX( "Out of memory" ); exit(1); }
        return p; }

    void PaUtil_InitializeClock( void );
    double PaUtil_GetTime( void );
    
    
    // ############################################################################################################ //

    int convolve_0 ( float* in, float* out, int length, float* kernel, int kernel_length ); // basic
    int convolve_1 ( float* in, float* out, int length, float* kernel, int kernel_length ); // sse
    int convolve_2_16 ( float* in, float* out, int length, float* kernel, int kernel_length ); // avx
    int convolve_2_32 ( float* in, float* out, int length, float* kernel, int kernel_length );
    int convolve_2_64 ( float* in, float* out, int length, float* kernel, int kernel_length );
    int convolve_2_128 ( float* in, float* out, int length, float* kernel, int kernel_length );
    int convolve_2_256 ( float* in, float* out, int length, float* kernel, int kernel_length );
    int convolve_2_512 ( float* in, float* out, int length, float* kernel, int kernel_length );
    int convolve_2_1024 ( float* in, float* out, int length, float* kernel, int kernel_length );
    int convolve_2_2048 ( float* in, float* out, int length, float* kernel, int kernel_length );
    int convolve_2_4096 ( float* in, float* out, int length, float* kernel, int kernel_length );
    int convolve_2_8192 ( float* in, float* out, int length, float* kernel, int kernel_length );
    int convolve_2_16384 ( float* in, float* out, int length, float* kernel, int kernel_length );
    int convolve_2_32768 ( float* in, float* out, int length, float* kernel, int kernel_length );
    int convolve_2_65536 ( float* in, float* out, int length, float* kernel, int kernel_length );

    void * f3[13] = {
        &convolve_2_16,
        &convolve_2_32,
        &convolve_2_64,
        &convolve_2_128,
        &convolve_2_256,
        &convolve_2_512,
        &convolve_2_1024,
        &convolve_2_2048,
        &convolve_2_4096,
        &convolve_2_8192,
        &convolve_2_16384,
        &convolve_2_32768,
        &convolve_2_65536 };
    
    int closest_larger_size( len ){ // or equal    
        int mask = 65536;
        while( mask/2 >= len ) mask /= 2;
        return mask < 16 ? 16 : mask; }

    // ############################################################################################################ //
    
    double T0;
    
    #define SAMPLERATE 48000 // [samples/second]
    #define SAMPLESIZE sizeof(float)

    #define NOW ((long)(ceil((PaUtil_GetTime()-T0)*SAMPLERATE))) // [samples]   
     
    #define POINTSMAX 1000  // should be even
    #define POINTSMIN 20
    #define DIFFSMAX 100
    
    #define MSIZE 250000 // keep 5 seconds

    struct graph {
        POINT points[POINTSMAX];
        int full;
        long cursor;
        int min_i, max_i; };

    struct port {
        int channels_count;
        PaStream *stream;
        long t0, len;        
        struct graph graph; }

    INPORT, OUTPORT;

    long cursor;
    
    struct diff {
        int mag;
        long count;
    }
    
    diffs[DIFFSMAX];
    
    int diffs_i;
    int diffs_full;


    float *canvas;
    
    struct out {
        int src_chan;
        float *k;
        int kn;
        int knr;
        int (* f)( float*, float*, int, float*, int );
    }
    
    *map;

    int jobs_per_channel = 1;

    // ####################### FILTERS ###############################################################################
    // ####################### FILTERS ###############################################################################
    // ####################### FILTERS ###############################################################################
    
    struct filter {
        char *name;
        float* k;
        int kn;
        int knr;
    }
    
    *filters[100]; // null terminated list of pointers
    
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

    void load_filters(){
        memset( filters, 0, sizeof(struct filter *) );
        filters[0] = getmem( sizeof( struct filter ) );
        filters[0]->name = "None";
        filters[0]->knr = closest_larger_size(1);
        filters[0]->k = getmem( SAMPLESIZE * (filters[0]->knr) );
        memset( filters[0]->k, 0, SAMPLESIZE * (filters[0]->knr) );
        filters[0]->k[0] = 1.0;
        filters[0]->kn = 1;
        WIN32_FIND_DATA r;
        HANDLE h = FindFirstFile( "filters\\*", &r );
        if( h == INVALID_HANDLE_VALUE )
            return;
        int i = 1;
        do if( !(r.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && r.cFileName[0] != '.' ){
            char fname[100] = "";
            sprintf( fname, "filters\\%s", r.cFileName );
            
            FILE *f;
            f = fopen( fname, "r" );
            if( !f )
                continue;
                
            int count = 0;
            float num;
            while( fscanf( f, "%f", &num ) == 1 )
                count ++;
                
            if( count == 0 )
                continue;

            fseek( f, 0, SEEK_SET );
            
            int real_len = closest_larger_size(count);
            float *data = getmem( SAMPLESIZE * real_len );
            memset( data, 0, SAMPLESIZE * real_len );
            int j=0 ;
            while( fscanf( f, "%f", data + (j++) ) == 1 );
            
            PRINT( "loaded %s %d taps \n", r.cFileName, count );
            
            fclose(f);
            
            // push to list
            filters[i] = getmem( sizeof( struct filter ) );
            filters[i]->name = getmem( strlen( fname )+1 );
            strcpy( filters[i]->name, r.cFileName );
            filters[i]->k = data;
            filters[i]->kn = count;
            filters[i]->knr = real_len;
            
            i++;
        } while( FindNextFile( h, &r ) );
        FindClose(h);
        PRINT( "loaded %d filters \n", i-1 );
    }

    // ########################## THREADS ################################################
    // ########################## THREADS ################################################
    // ########################## THREADS ################################################

    #define THREADSMAX 100

    int threads_cores = 0;
    int threads_count = 0;
    int threads_shutdown = 0;

    void get_cpu_cores(){        
        typedef BOOL (WINAPI *LPFN_GLPI)( PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);        
        LPFN_GLPI glpi = GetProcAddress( GetModuleHandle("kernel32"), "GetLogicalProcessorInformation");        
        if( !glpi ) { threads_cores = 1; return; }
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer, ptr = NULL;
        DWORD returnLength = 0;        
        glpi( buffer, &returnLength );
        buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION) getmem( returnLength );
        BOOL r = glpi( buffer, &returnLength );
        if( !r ) { threads_cores = 1; return; }
        ptr = buffer;
        DWORD byteOffset = 0;
        DWORD processorCoreCount = 0;
        while( byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength ){
            if( ptr->Relationship == RelationProcessorCore ) processorCoreCount++;
            byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
            ptr ++; }
        threads_cores = processorCoreCount; }

    void choose_threads_count(){
        threads_count = threads_cores;
        }

    void threads_init(){
        get_cpu_cores();
        choose_threads_count();
        threads_shutdown = 0; }

    struct thread {
        int status; // 0 done; 1 work; 2 emerging
        void (* f)( float*, float*, int, float*, int );
        float *in;
        float *out;
        int len;
        float *k;
        int klen;
    }
    
    threads[THREADSMAX];

    void threads_body( struct thread *self ){
        while( 1 ){
            while( self->status != 1 )
                if( threads_shutdown )
                    return;
            self->f(
                self->in -self->klen+1, // * matlab
                self->out,
                self->len +self->klen-1, // * format
                self->k,
                self->klen
                );
            self->status = 0; } }

    void threads_start(){
        if( !threads_count ) threads_init();
        memset( &threads, 0, sizeof( struct thread ) * threads_count );
        for( int i; i<threads_count; i++ )
            CreateThread( 0, 10000000, &threads_body, threads+i, 0, 0 ); }

    void threads_stop(){
        threads_shutdown = 1; }

    int threads_done(){
        for( int i=0; i<threads_count; i++ )
            if( threads[i].status != 0 )
                return 0;
        return 1; }

    void threads_submit( void *f, float *in, float *out, int len, float *k, int klen ){
        while( 1 )
            for( int i=0; i<threads_count; i++ )
                if( threads[i].status == 0 ){
                    threads[i].status = 2;
                    threads[i].f = f;
                    threads[i].in = in;
                    threads[i].out = out;
                    threads[i].len = len;
                    threads[i].k = k;
                    threads[i].klen = klen;
                    threads[i].status = 1;
                    return; } }
    
    // ####################### FILTERS ###############################################################################
    // ####################### FILTERS ###############################################################################
    // ####################### FILTERS ###############################################################################


    void aftermath( int sel, long t, int avail_after, int frameCount ){
        struct graph *g = ( sel == 0 ? &(INPORT.graph) : &(OUTPORT.graph) );
        
        // make a stat
        g->points[g->cursor].x = t;
        g->points[g->cursor].y = avail_after -frameCount; // avail before insert
        g->cursor++;
        
        g->points[g->cursor].x = t;
        g->points[g->cursor].y = avail_after;
        g->cursor++;
        
        // graph end ?
        if( g->cursor == POINTSMAX ){
            g->cursor = 0;
            if( g->full == 0 )
                g->full = 1;
        }

        // find min/max
        int pi = 0;
        int gi = ( g->full ? g->cursor : 0 );
        int count = ( g->full ? POINTSMAX : g->cursor );
        int min = 999999;
        int max = -999999;
        for( int i=0; i<count; i++ ){
            if( g->points[gi].y < min ){
                min = g->points[gi].y;
                g->min_i = gi;
            }
            if( g->points[gi].y > max ){
                max = g->points[gi].y;
                g->max_i = gi;
            }
            gi ++;
            gi %= POINTSMAX;
        }        

        // cursor ?
        if( (INPORT.graph.full || INPORT.graph.cursor > POINTSMIN) && (OUTPORT.graph.full || OUTPORT.graph.cursor > POINTSMIN) ){
        
            long new_cursor_candidate = 
                OUTPORT.t0 -INPORT.t0 // output.t0 as input lane address
                +OUTPORT.len // where is output end on the input
                +INPORT.graph.points[INPORT.graph.min_i].y -OUTPORT.graph.points[OUTPORT.graph.max_i].y; // our latency
                
            if( new_cursor_candidate >= 0 ){
            
                if( cursor == -1 ){
                    cursor = new_cursor_candidate;
                    PRINT( "curosr initialized (%d) \n", cursor );
                    
                } else if( cursor - new_cursor_candidate != 0 ){
                    // push a diff to think about
                    long diff = cursor - new_cursor_candidate;
                    if( diff == diffs[diffs_i].mag ){
                        diffs[diffs_i].count ++;
                    } else {
                        diffs_i ++;
                        if( diffs_i == DIFFSMAX ){
                            diffs_i = 0;
                            diffs_full = 1;
                        }
                        diffs[diffs_i].mag = diff;
                        diffs[diffs_i].count = 1;
                    }
                }
            }
        }
    }

    void correct_cursor_if_necessary(){
        if( diffs_full || diffs_i > 0 ){
            long diffs_sum = 0;
            long diffs_total = 0;
            int di = ( diffs_full ? diffs_i : 0 );
            int count = ( diffs_full ? DIFFSMAX : diffs_i );
            for( int i=0; i<count; i++ ){
                diffs_sum += diffs[di].mag * diffs[di].count;
                diffs_total += diffs[di].count;
                di ++; di %= DIFFSMAX;
            }
            if( diffs_sum != 0 ){ // may cancel each other
                double diffs_avg = ((double)diffs_sum)/((double)diffs_total);
                if( diffs_avg > 0 )
                    diffs_sum = (long)( floor( diffs_avg ) );
                else
                    diffs_sum = (long)( ceil( diffs_avg ) );
                if( diffs_sum != 0 ){ // may round to zero
                    cursor -= diffs_sum;
                    PRINT( "correction of %d samples \n", diffs_sum );
                    diffs_full = diffs_i = 0;
                }
            }
        }
    }

    PaStreamCallbackResult device_tick(
        float **input,
        float **output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo *timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userdata ){
        
        long now;

        if( statusFlags ){
            if( paInputUnderflow & statusFlags ) PRINT( "Input Underflow \n" );
            if( paInputOverflow & statusFlags ) PRINT( "Input Overflow \n" );
            if( paOutputUnderflow & statusFlags ) PRINT( "Output Underflow \n" );
            if( paOutputOverflow & statusFlags ) PRINT( "Output Overflow \n" );
            if( paPrimingOutput & statusFlags ) PRINT( "Priming Output \n" );}
        
        if( input && output )
            PRINT( "strange \n" );

        if( input ){
        
            // write
            int ofs = INPORT.len % MSIZE;
            for( int i=0; i<INPORT.channels_count; i++ )
                for( int j=0; j<3; j++ )
                    memcpy( canvas +i*MSIZE*4 +j*MSIZE +ofs, input[i], frameCount*SAMPLESIZE );
        
            // stamp
            now = NOW;
            
            if( INPORT.t0 == -1 )
                INPORT.t0 = now;
                
            // commit
            INPORT.len += frameCount;
            
            // log
            aftermath( 0, now, INPORT.t0 + INPORT.len -now, frameCount );
        }
        
        if( output ){

            if( cursor > -1 ){
                // copy 
                for( int i=0; i<OUTPORT.channels_count; i++ ){
                    memset( output[i], 0, frameCount*SAMPLESIZE );
                    if( map[i].src_chan == -1 )
                        continue;
                    int jlen = frameCount / jobs_per_channel;
                    int rem = frameCount % jobs_per_channel;                        
                    for( int j = 0; j<jobs_per_channel; j++ )
                        threads_submit(
                            map[i].f,
                            canvas + map[i].src_chan*MSIZE*4 +MSIZE + cursor%MSIZE +j*jlen,
                            output[i] +j*jlen,
                            (j == jobs_per_channel-1) ? jlen+rem : jlen,
                            map[i].k,
                            map[i].knr
                            );
                }

                while( !threads_done() );

                if( cursor + frameCount > INPORT.len )
                    PRINT( "glitch %d \n", cursor -INPORT.len );

                cursor += frameCount;
            }

            // stamp
            now = NOW;
            
            if( OUTPORT.t0 == -1 )
                OUTPORT.t0 = now;

            // commit
            OUTPORT.len += frameCount;

            // log
            aftermath( 1, now, OUTPORT.t0 + OUTPORT.len -now, frameCount );
        }

        return paContinue; }


    int start( int input_device_id, int output_device_id ){
        for( int i=1; i>-1; i-- ){
            PRINT( "starting %s %d ... \n", ( i ? "input" : "output" ), ( i ? input_device_id : output_device_id ) );
        
            int device_id = ( i ? input_device_id : output_device_id );
            const PaDeviceInfo *device_info = Pa_GetDeviceInfo( device_id );
            static PaStreamParameters params;
            PaStream **stream = ( i ? &(INPORT.stream) : &(OUTPORT.stream) );
            
            params.device = device_id;
            params.sampleFormat = paFloat32|paNonInterleaved;
            params.hostApiSpecificStreamInfo = 0;            
            params.suggestedLatency = ( i ? device_info->defaultLowInputLatency : device_info->defaultLowOutputLatency ); // "buffer size" [seconds]
            params.channelCount = ( i ? device_info->maxInputChannels : device_info->maxOutputChannels );
            
            PaError err = Pa_OpenStream(
                stream,
                i ? &params : 0,
                i ? 0 : &params,
                SAMPLERATE,
                paFramesPerBufferUnspecified,
                paClipOff | paDitherOff | paPrimeOutputBuffersUsingStreamCallback, // paNeverDropInput is for full-duplex only
                &device_tick,
                0 );

            if( err != paNoError ){
                if( err != paUnanticipatedHostError ) {
                    PRINT( "ERROR 1: %s \n", Pa_GetErrorText( err ) );
                    return 0; }
                else {
                    const PaHostErrorInfo* herr = Pa_GetLastHostErrorInfo();
                    PRINT( "ERROR 2: %s \n", herr->errorText );
                    return 0; }}
            
            if( i ){
                INPORT.channels_count = device_info->maxInputChannels;
                PRINT( "%d channels \n", INPORT.channels_count );
                canvas = getmem( INPORT.channels_count * MSIZE*4 * SAMPLESIZE );
                memset( canvas, 0, INPORT.channels_count * MSIZE*4 * SAMPLESIZE );
            } else {
                OUTPORT.channels_count = device_info->maxOutputChannels;
                PRINT( "%d channels \n", OUTPORT.channels_count );
                map = getmem( OUTPORT.channels_count * sizeof(struct out) );
                for( int i=0; i<OUTPORT.channels_count; i++ ){
                    map[i].src_chan = -1;
                    map[i].k = filters[0]->k;
                    map[i].kn = filters[0]->kn;
                    map[i].knr = filters[0]->knr; int index = 0, v = 16; while( v != map[i].knr ){ v *= 2; index ++; }
                    map[i].f = f3[index];
                }
                threads_start();
                jobs_per_channel = (int)ceil(((float)threads_count)/((float)(OUTPORT.channels_count)));
                PRINT( "JOBSPERCHANNEL %d \n", jobs_per_channel );
            }
            
            err = Pa_StartStream( *stream );
            if( err != paNoError ){
                PRINT( "ERROR 3: %s \n", Pa_GetErrorText( err ) );
                return 0; }

            PaStreamInfo *stream_info = Pa_GetStreamInfo( *stream );            
            PRINT( "%d / %d \n",
                (int)round(stream_info->sampleRate),
                (int)round( i ? (stream_info->inputLatency)*SAMPLERATE : (stream_info->outputLatency)*SAMPLERATE ) );
        }

        // done
        PRINT( "both started. \n" );
        return 1; }



    // ############################################################################################################ //
    // ############################################################################################################ //
    // ############################################################################################################ //

    const int width = 600;
    const int height = 700;
    const int WW = 574, HH = 200;

    HWND hwnd;
    HDC hdc;    
    RECT rc;
    MSG msg;

    #define CMB1 (555)
    #define CMB2 (556)
    #define BTN1 (123)
    
    #define LB1 (110)
    #define CB1 (220)
    #define CB2 (330)
    
    #define EDT (400)

    HWND hCombo1, hCombo2, hBtn;
    HWND cbs[10];
    HWND cbs2[10];
    HWND hEdit;

    void PRINT( char *format, ... ){
        char str[1000] = "";
        va_list( args );
        va_start( args, format );
        vsprintf( str, format, args );
        va_end( args );
        if( str[0] == 0 )
            return;
        int index = GetWindowTextLength( hEdit );
        SendMessageA( hEdit, EM_SETSEL, (WPARAM)index, (LPARAM)index ); // select from end to end
        SendMessageA( hEdit, EM_REPLACESEL, 0, (LPARAM)str ); }
    
    void MSGBOX( char *format, ... ){
        char str[1000] = "";
        va_list( args );
        va_start( args, format );
        vsprintf( str, format, args );
        va_end( args );
        MessageBox( 0, str, "Message", MB_OK ); }

    void CALLBACK every_second( HWND hwnd, UINT uMsg, UINT timerId, DWORD dwTime ){
        correct_cursor_if_necessary();
        if( cursor > 0 )
            PRINT( "load %d%% \n", (int)ceil((Pa_GetStreamCpuLoad(INPORT.stream)+Pa_GetStreamCpuLoad(OUTPORT.stream))*200.0) ); } // 50% == 100%

    char * device_name( int device_id ){
        static char *names = 0;
        if( !names ){ 
            names = getmem( Pa_GetDeviceCount() * 250 );
            memset( names, 0, Pa_GetDeviceCount() * 250 ); }
        char * name = names + device_id*250;
        if( !name[0] ){
            char dev_name[100] = "", drv_name[100] = "";
            strcpy( dev_name, Pa_GetDeviceInfo( device_id )->name );
            strcpy( drv_name, Pa_GetHostApiInfo( Pa_GetDeviceInfo( device_id )->hostApi )->name );
            sprintf( name, " %3d  /  %s  /  %s ", device_id, strstr( drv_name, "Windows" ) ? drv_name+8 : drv_name, dev_name ); }
        return name; }

    int device_id( char * device_name ){
        int id; sscanf( device_name, "  %3d", &id );
        return id; }

    void fill_left_combos( int count ){
        char n[3];
        for( int i=0; i<10; i++ ){
            SendMessage( cbs[i], CB_RESETCONTENT, (WPARAM)0, (LPARAM)0 );
            SendMessage( cbs[i], CB_ADDSTRING, (WPARAM)0, (LPARAM)"None" );
            SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
            for( int j=0; j<count; j++ ){
                sprintf( n, "%d", j+1 );
                SendMessage( cbs[i], CB_ADDSTRING, (WPARAM)0, (LPARAM)n ); } } }

    void set_filter( int out, struct filter * fl ){
        map[out].k = fl->k;
        map[out].kn = fl->kn;
        map[out].knr = fl->knr; int index = 0, v = 16; while( v != map[out].knr ){ v *= 2; index ++; }
        map[out].f = f3[index]; }

    int is_conf_used(){ // wether the two devices selected are as in the conf
        char a[250], b[250];
        if( !conf_get( "device1" ) || !conf_get( "device2" ) ) return 0;
        GetDlgItemText( hwnd, CMB1, a, 250);
        GetDlgItemText( hwnd, CMB2, b, 250);
        return strcmp( a, conf_get( "device1" ) ) == 0 && strcmp( b, conf_get( "device2" ) ) == 0; }

    void show_conf( int b ){ // sets the bottom combos as in conf or clear them
        if( !b ){
            for( int i=0; i<10; i++ ){
                SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
                SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 ); }
            return; }
        char key[20]; int val;
        for( int i=0; i<10; i++ ){
            sprintf( key, "out%d", i+1 );
            if( conf_get( key ) && sscanf( conf_get( key ), "%d", &val ) == 1 )
                SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)val, (LPARAM)0 );
            else SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
            sprintf( key, "filter%d", i+1 );
            if( conf_get( key ) && filter_p( conf_get( key ) ) )
                SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)(filter_i( conf_get( key ) )), (LPARAM)0 );
            else SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 ); } }

    void save_conf(){
        char txt[250], key[20];
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

    LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ){
        if( msg == WM_COMMAND ){
        
            if( LOWORD(wParam) == CMB1 && CBN_SELCHANGE == HIWORD(wParam) ){
                char txt[250];
                GetDlgItemText( hwnd, CMB1, txt, 250 );
                fill_left_combos( Pa_GetDeviceInfo( device_id( txt ) )->maxInputChannels );
                show_conf( is_conf_used() );
                
            } else if( LOWORD(wParam) == CMB2 && CBN_SELCHANGE == HIWORD(wParam) ){
                char txt[250];
                GetDlgItemText( hwnd, CMB2, txt, 250 );
                show_conf( is_conf_used() );
                
            } else if( LOWORD(wParam) == BTN1 ){
                int sd, dd;
                char txt[250];
                GetDlgItemText( hwnd, CMB1, txt, 250 );
                sd = device_id( txt );
                GetDlgItemText( hwnd, CMB2, txt, 250 );
                dd = device_id( txt );
                if( start( sd, dd ) ){                                   // START
                    EnableWindow( hCombo1, 0 );
                    EnableWindow( hCombo2, 0 );
                    EnableWindow( hBtn, 0 );
                    for( int i=0; i<OUTPORT.channels_count; i++ ){
                        if( i < 10 ){
                            EnableWindow( cbs[i], 1 );
                            for( int j=0; j<INPORT.channels_count; j++ ){
                                char key[6]; int val;
                                sprintf( key, "out%d", i+1 );
                                if( is_conf_used() && conf_get( key ) && sscanf( conf_get( key ), "%d", &val ) == 1 )
                                    map[i].src_chan = val-1;
                                sprintf( key, "filter%d", i+1 );
                                if( is_conf_used() && conf_get( key ) && strcmp( conf_get( key ), "None" ) != 0 && filter_p( conf_get( key ) )){
                                    set_filter( i, filter_p( conf_get( key ) ) );
                                    SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)(filter_i( conf_get( key ) )), (LPARAM)0 ); } }
                            SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)map[i].src_chan+1, (LPARAM)0 );
                            EnableWindow( cbs2[i], 1 ); } }
                    SetTimer( 0, 0, 1000, (TIMERPROC) &every_second ); }

            } else if( (LOWORD(wParam) >= CB1) && LOWORD(wParam) <= CB1+10 && CBN_SELCHANGE == HIWORD(wParam) ){
                int out = LOWORD(wParam) - CB1;                
                char  txt[20];
                GetDlgItemText( hwnd, CB1+out, txt, 20 );                
                int chan = txt[0]-48-1;
                if( chan < 10 ) {
                    map[out].src_chan = chan;
                    PRINT( "mapped out %d to in %d\n", out+1, chan+1 ); }
                else {
                    map[out].src_chan = -1;
                    PRINT( "muted out %d \n", out+1 ); }
                
            } else if( (LOWORD(wParam) >= CB2) && LOWORD(wParam) <= CB2+10 && CBN_SELCHANGE == HIWORD(wParam) ){                
                int out = LOWORD(wParam) - CB2;                
                char  txt[100];
                GetDlgItemText( hwnd, CB2+out, txt, 100 );
                struct filter *fl = filter_p( txt );
                set_filter( out, fl );                
                PRINT( "mapped out %d to filter %s \n", out+1, txt );                
            }
        }

        else if( msg == WM_CLOSE ){
            if( cursor > 0 ) save_conf();
            DestroyWindow( hwnd ); }

        else if( msg == WM_DESTROY ){
            threads_stop();
            PostQuitMessage( 0 ); }

        return DefWindowProc( hwnd, msg, wParam, lParam ); }


    int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow ){
        
        // SetProcessDPIAware();
        WNDCLASSEX wc;
        memset( &wc, 0, sizeof(wc) );
        wc.cbSize = sizeof(wc);
        wc.hInstance = hInstance;
        wc.lpfnWndProc = WndProc;
        wc.lpszClassName = "mainwindow";
        wc.hbrBackground = COLOR_WINDOW; //CreateSolidBrush( RGB(64, 64, 64) );
        wc.hCursor = LoadCursor( 0, IDC_ARROW );
        if( !RegisterClassEx(&wc) ){ MessageBox( 0, "Failed to register window class.", "Error", MB_OK ); return 1; }
        hwnd = CreateWindowEx( WS_EX_APPWINDOW, "mainwindow", title, WS_MINIMIZEBOX | WS_SYSMENU | WS_POPUP | WS_CAPTION, 300, 200, width, height, 0, 0, hInstance, 0 );
        hdc = GetDC( hwnd );
        hCombo1 = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST, 10, 10, 490, 8000, hwnd, CMB1, NULL, NULL);
        hCombo2 = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST, 10, 40, 490, 8000, hwnd, CMB2, NULL, NULL);
        hBtn = CreateWindowEx( 0, "Button", "Play >", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_DEFPUSHBUTTON, 507, 10, 77, 53, hwnd, BTN1, NULL, NULL);
        hEdit = CreateWindowEx( 0, "EDIT", 0, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 10, 460, WW, HH, hwnd, EDT, NULL, NULL);

        // init core
        if( Pa_Initialize() ) PRINT( "ERROR: Could not initialize PortAudio. \n" );
        if( Pa_GetDeviceCount() <= 0 ) PRINT( "ERROR: No Devices Found. \n" );
        PaUtil_InitializeClock();
        T0 = PaUtil_GetTime();
        conf_load( "RTFIR.conf" );
        threads_init();
        PRINT( "built " ); PRINT( __DATE__ ); PRINT( " " ); PRINT( __TIME__ ); PRINT( "\n" );
        PRINT( "SAMPLERATE %d \n", SAMPLERATE );
        PRINT( "Cores/Threads %d/%d\n", threads_cores, threads_count );

        // globals
        struct port* ports [2] = { &INPORT, &OUTPORT };
        for( int i=0; i<2; i++ ){
            struct port *p = ports[i];
            memset( p, 0, sizeof(struct port) );
            memset( p, 0, sizeof(struct port) );
            p->t0 = -1; // invalid
            p->len = 0;
            p->graph.cursor = 0;
            p->graph.full = 0;
            p->graph.min_i = -1; // invalid
            p->graph.max_i = -1; // invalid
            }
        cursor = -1; // invalid
        diffs_full = 0;
        diffs_i = 0;

        // load filters
        load_filters();

        // add map labels and dropdowns
        for( int i=0; i<10; i++ ){        
            char str[7];
            sprintf( str, "out %d", i+1 );
            CreateWindowEx( 0, "static", str, WS_VISIBLE|WS_CHILD, 50, 105+i*30, 50, 80, hwnd, LB1+i, NULL, NULL);        
            cbs[i] = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST, 100, 100+i*30, 90, 800, hwnd, CB1+i, NULL, NULL);
            EnableWindow( cbs[i], 0 );            
            CreateWindowEx( 0, "static", "filter", WS_VISIBLE|WS_CHILD, 250, 105+i*30, 50, 80, hwnd, LB1+10+i, NULL, NULL);        
            cbs2[i] = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST, 300, 100+i*30, 130, 800, hwnd, CB2+i, NULL, NULL);            
            int j=0;
            while( filters[j] )
                SendMessage( cbs2[i], CB_ADDSTRING, 0, filters[j++]->name );
            SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
            EnableWindow( cbs2[i], 0 ); }
        
        // SHOW
        ShowWindow( hwnd, SW_SHOW );

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
        if( SendMessage( hCombo1, CB_GETCURSEL, (WPARAM)0, (LPARAM)0 ) == CB_ERR )
            SendMessage( hCombo1, CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
        if( SendMessage( hCombo2, CB_GETCURSEL, (WPARAM)0, (LPARAM)0 ) == CB_ERR )
            SendMessage( hCombo2, CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );

        char txt[250];
        GetDlgItemText( hwnd, CMB1, txt, 250 );
        fill_left_combos( Pa_GetDeviceInfo( device_id( txt ) )->maxInputChannels );

        show_conf( is_conf_used() );

        // loop
        while( GetMessage( &msg, 0, 0, 0 ) > 0 ){
            TranslateMessage( &msg );
            DispatchMessage( &msg ); }

	return msg.wParam; }
