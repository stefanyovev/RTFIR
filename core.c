

	// keep 3 seconds of audio (msize). 1 second max filter. 1 second max delay. 1 second left for buffersizes and correction headroom.
	// repeat these 3 seconds 3 times in memory and use the middle one so memcopy does not care for boundaries.
	// constantly measure both buffersizes (L) and cursor-to-end-of-input-buffer (G). should be equal.

	
	struct stat {
		int64_t t;
		int64_t avail;
		};

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
		struct convolve_kernel *k;
		}; 


	// ------------------------------------------------------------------------------------------------------------ //


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

	int jobs_per_channel;  // = threads / outs


	// ------------------------------------------------------------------------------------------------------------ //


	void set_filter( int out, float *k, int kn, char *kname ){
		if( map[out].k ) { FREE( map[out].k ); map[out].k = 0; }
		if( !k ) return;
		map[out].k = convolve_kernel_new( kname, k, kn ); }


	void _makestat( volatile struct port * p ){

		int64_t now = NOW();
		
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
		now = NOW();

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
		now = NOW();

		if( cursor < 0 ){
			
			// set cursor
			cursor = now -ports[0].t0 +ports[0].min -(int)(L*0.66); //-(ports[1].max -ports[1].min)*2;
			dith_t = now;
			if( cursor >= 0 ){
				PRINT("%s init %d \r\n", clock_timestr(), cursor ); }
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
			now = NOW();

			// dith
			dith_sig = (L-G) / 10.0;  // G to L for 1 second (speed)
			//if( cursor > samplerate )
			//	dith_sig /= 10.0 ;
			dith_p = (int64_t)( (float)samplerate / ( dith_sig > 0.0 ? dith_sig : -dith_sig ) );
			if( dith_t + dith_p < now ){
				if( dith_sig < 0 ){
					cursor += 1;
					// todo: remove 1 from all stats so dith sig is lower next ime
					PRINT("%s skipped 1 sample %d \r\n", clock_timestr(), cursor ); 
					}
				else {
					cursor -= 1;
					// todo: add 1 to all stats so dith sig is bigger next ime
					PRINT("%s replayed 1 sample %d \r\n", clock_timestr(), cursor );
					}
				dith_t = now;
			}
		}
	}

	struct op_clear_arg { void *p; int len; };		
	void op_clear( void *task ){ struct op_clear_arg *t = (struct op_clear_arg *) task;
		memset( t->p, 0, t->len * sizeof(float) ); }

	struct op_copy_arg { void *dst, *src; int len; };	
	void op_copy( void *task ) { struct op_copy_arg *t = (struct op_copy_arg *) task;
		memcpy( t->dst, t->src, t->len * sizeof(float) ); }

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
					memcpy( canvas +i*msize*4 +j*msize +ofs, input[i], frameCount*sizeof(float) );
				
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
						copy_task.src = canvas + map[i].src*msize*4 +msize + cursor%msize +j*jlen;
						copy_task.len = (j == jobs_per_channel-1) ? jlen+rem : jlen;
						threads_submit( &op_copy, &copy_task, sizeof(copy_task)  );
					}
					else {
						// convolve
						T.in = canvas + map[i].src*msize*4 +msize + cursor%msize +j*jlen;
						T.out = output[i] +j*jlen;
						T.len = (j == jobs_per_channel-1) ? jlen+rem : jlen;
						T.k = map[i].k;
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


	int start( int sd, int dd, int sr, int tc ){  // -> bool
		int in = Pa_GetDeviceInfo( sd )->maxInputChannels;
		int on = Pa_GetDeviceInfo( dd )->maxOutputChannels;
		cursor = -1;
		samplerate = sr;
		msize = sr * 3;
		ssize = sr * 2;
		canvas = MEM( sizeof(float) * msize * 4 * in );  // 4th is just because the third memcopy may be out
		ports = MEM( sizeof(struct port) * 2 );
		map = MEM( sizeof(struct out) * on ); for( int i=0; i<on; i++ ) map[i].src = -1;
		lstat = MEM( sizeof(struct stat) * ssize ); lstat_len = 0;		
		gstat = MEM( sizeof(struct stat) * ssize ); gstat_len = 0;		
		ports[0].channels_count = in;
		ports[0].stats = MEM( sizeof(struct stat) * ssize );
		ports[1].type = 1;
		ports[1].channels_count = on;
		ports[1].stats = MEM( sizeof(struct stat) * ssize );
		clock_init( sr );
		threads_init( tc ); jobs_per_channel = (int)ceil( ((float)tc)/((float)on) );		
		for( int i=0; i<2; i++ ){						
			PRINT( "starting %s ... ", ( i==0 ? "input" : "output" ) );
 
			int device_id = ( i==0 ? sd : dd );
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
