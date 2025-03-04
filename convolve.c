


	// y[n] = x[n]*h[0] + x[n-1]*h[1] + x[n-2]*h[2] + ...


	#if __INCLUDE_LEVEL__ == 0
		#include <stdio.h>
		#include <stdint.h>
		#include <immintrin.h>	
		#define print printf		
		void error( char *msg ){
			print( "error: %s", msg );
			exit(1); }		
		#include "mem.c"
		#include "clock.c"
	#endif


	struct convolve_kernel {
		char *name;
		int len;
		__m256* data; };
	
	struct convolve_task {
		float *in;
		float *out;
		int len;
		struct convolve_kernel *k; };

	#define ALIGNMENT 32

	struct convolve_kernel* convolve_kernel_new( char *name, float *k, int kn ){
		struct convolve_kernel *R = mem( sizeof( struct convolve_kernel ) );
		R->name = mem( strlen( name )+1 );
		strcpy( R->name, name );
		R->len = ( kn / 16 ) * 16 + 16; // filter len multiple of 16 samples
		R->data = mem_aligned( R->len * sizeof(float) * 8, ALIGNMENT );
		for( int i=0; i<kn; i++ )
			R->data[ R->len-kn +i] = _mm256_broadcast_ss( k +kn-1 -i );
		return R; }
	
	void convolve_kernel_free( struct convolve_kernel *k ){
		mem_free( k->name );
		mem_free( k->data );
		mem_free( k ); }

	void convolve_naive( void *task ){
		float *x = ((struct convolve_task*)task)->in;
		float *y = ((struct convolve_task*)task)->out;
		int length = ((struct convolve_task*)task)->len;
		__m256* k = ((struct convolve_task*)task)->k->data;
		int kn = ((struct convolve_task*)task)->k->len;		
		for( int i=0; i<length; i++ ){
			y[i] = 0;			
			for( int j=0; j<kn; j++ )
				y[i] += x[i-kn+j+1] * ( *( (float*)(k+j) ) ); } }

	#define SSE_SIMD_LENGTH 4
	#define AVX_SIMD_LENGTH 8
	#define VECTOR_LENGTH 16

	void convolve( void *task ){
		float* in = ((struct convolve_task*)task)->in;
		float* out = ((struct convolve_task*)task)->out;
		int length = ((struct convolve_task*)task)->len;
		__m256* kernel = ((struct convolve_task*)task)->k->data;
		int kernel_length = ((struct convolve_task*)task)->k->len;
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
			_mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); } }

	#undef ALIGNMENT
	#undef SSE_SIMD_LENGTH
	#undef AVX_SIMD_LENGTH
	#undef VECTOR_LENGTH


	// -------------------------------------------------------------------------------------------------------------- //


	int convolve_test(){

		#define LENGTH 50000

		// sig = random192
		float *sig = mem( sizeof(float) * (LENGTH +LENGTH-1) ); sig += LENGTH-1; // allow convolve() to read kernel_length-1 samples backwards
		for( int i=0; i<LENGTH; i++ )
			sig[i] = rand() % 10;
		
		// h1 = {1.0}
		float *h1 = mem( sizeof(float) * 1 );
		h1[0] = 1.0;
		
		// h2 = random192
		float *h2 = mem( sizeof(float) * LENGTH );
		for( int i=0; i<LENGTH; i++ )
			h2[i] = rand() % 10;
		
		// y1, y2
		float *y1 = mem( sizeof(float) * LENGTH );
		float *y2 = mem( sizeof(float) * LENGTH );

		// load kernels
		struct convolve_kernel
			*k1 = convolve_kernel_new( "h1", h1, 1 ),
			*k2 = convolve_kernel_new( "h2", h2, LENGTH );

		// task T
		struct convolve_task T;

		// Test0
		//print( "Test 0: convolve_naive() with {1} is the same as memcopy: " );
		T.k = k1;
		T.in = sig;
		T.out = y1;
		T.len = LENGTH;
		convolve_naive( &T );
		for( int i=0; i<LENGTH; i++ )
			if( y1[i] != sig[i] ){
				print( " FAIL \r\n" );
				print( " convolve_naive with {1} does not produce the same as memcopy" );
				return 0; }
		//print( " PASS \r\n" );

		// Test1
		print( "Test 1: convolve() with {1} is the same as memcopy: " );
		T.k = k1;
		T.in = sig;
		T.out = y1;
		T.len = LENGTH;
		convolve( &T );
		for( int i=0; i<LENGTH; i++ )
			if( y1[i] != sig[i] ){
				print( "  FAIL \r\n" );
				return 0; }
		print( "  PASS \r\n" );

		// Test3
		print( "Test 2: convolve() produces the same as convolve_naive(): " );
		clock_init(1000); uint64_t t1, t2, t3;
		T.k = k2;
		T.in = sig;
		T.out = y1;
		T.len = LENGTH;
		t1 = clock_time();
		convolve_naive( &T );
		T.out = y2;
		t2 = clock_time();
		convolve( &T );
		t3 = clock_time();
		for( int i=0; i<LENGTH; i++ )
			if( y1[i] != y2[i] ){
				print( " FAIL \r\n" );
				return 0; }
		print( " PASS \r\n" );
		
		print( "SPEED: \r\n" );
		print( "  %d times faster than convolve_naive() \r\n", (t2-t1)/(t3-t2) );
		float macs = ((float)LENGTH * (float)LENGTH) / ((float)(t3-t2) / 1000.0);
		//print( "  %.1f multiply-add operations per second on 1 core \r\n", macs );
		float gflops = (2.0*(float)macs) / 1000000000.0;
		print( "  %.1f gigaflops per core \r\n", gflops );
		float gflops_mini = (50000.0 * 50000.0 * 2.0) / 1000000000.0;
		print( "  ~%.1f channels per core @ 192k with 1 second long filters \r\n", (gflops / gflops_mini) );
		print( "  ~%.1f channels per core @ 96k with 1 second long filters \r\n", (gflops / gflops_mini) *4 );
		print( "  ~%.1f channels per core @ 48k with 1 second long filters \r\n", (gflops / gflops_mini) *16 );
		
		#undef LENGTH
		
		// TODO: Test3: chunked convolve is the same as convolve
		
		return 1;
	}



// gcc -w convolve.c -fpermissive -o cvl -march=native -O3 && ./cvl

#if __INCLUDE_LEVEL__ == 0

	int main( int, int ){
		return convolve_test();
	}

#endif