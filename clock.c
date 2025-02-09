


	int clock_samplerate;
	double clock_tick_in_samples;
	int64_t clock_count_first;  // ??


	void clock_init( int samplerate ){
		int64_t freq;
		clock_samplerate = samplerate;
		QueryPerformanceFrequency( &freq );
		clock_tick_in_samples = (double)samplerate / (double)freq;
		QueryPerformanceCounter( &clock_count_first ); }


	int64_t clock_time(){
		int64_t count;
		QueryPerformanceCounter( &count );
		count -= clock_count_first;
		return (int64_t)floor( ((double)count)*clock_tick_in_samples ); }


	char* clock_timestr(){
		int seconds = clock_time() / clock_samplerate;
		static char str[30];
		int h = seconds / 3600;
		int m = seconds / 60 - h*60;
		int s = seconds -h*3600 -m*60;
		sprintf( str, "%.2d:%.2d:%.2d", h, m, s );
		return str; }
