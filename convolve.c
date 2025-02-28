


	// y[n] = x[n]*h[0] + x[n-1]*h[1] + x[n-2]*h[2]
	// reverse(h); cursor -= len(h)-1
	// y[n] = x[n]*h[0] + x[n+1]*h[1]

	#if __INCLUDE_LEVEL__ == 0
		#include <stdio.h>
		#include <stdint.h>
		#include <immintrin.h>	
		#define print printf		
		void error( char *msg ){
			print( "error: %s", msg );
			exit(1); }		
		#include "mem.c"
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



// gcc -w convolve.c -fpermissive -o cvl -march=native -O3 && ./cvl

#if __INCLUDE_LEVEL__ == 0

	/*
	// -----------------------------------------------------------------------------------------------
	// --- TEST vector read write ---
	
	int main( int, int ){
		float v = 0.5;
		__m256 m =  _mm256_broadcast_ss( &v );
		print( "%d \r\n", sizeof( m ) / sizeof( v ) );  // 8
		print( "%.2f \r\n", v );  // 0.5
		print( "%.2f \r\n", *( (float*)&m ) ); // 0.5
		print( "%.2f \r\n", *( (float*)&m +7 ) ); // 0.5
		}
	// -----------------------------------------------------------------------------------------------
	*/
	
	/*
	// -----------------------------------------------------------------------------------------------
	// --- TEST convolution visually; test if splitting results the same as one block ---
	
	void print_floats( float* k, int len ){
		for( int i=0; i<len; i++ ){
			if( k[i] == 0 ) print( "." );
			else print( "%1g", k[i] ); } }

	void print_kernel( struct convolve_kernel *k ){
		float v;
		for( int i=0; i<k->len; i++ ){
			v = *( (float*)(k->data+i) );
			if( v == 0 ) print( "." );
			else print( "%1g", v ); } }
	
	int main( int, int ){
		
		#define HLEN 32
		#define SLEN 96

		print( "\r\n   " );
		for( int i=0; i<96; i++ ) if( !(i%10) ) print("."); else print( "%d", i%10 );
		print( "\r\n" );		

		float *h = mem( sizeof(float)*HLEN );
		h[0] = 1;
		h[30] = 1;
		h[31] = 1;
		
		print( "h: " );
		print_floats( h, HLEN );
		print( "\r\n" );

		struct convolve_kernel			
			*k = convolve_kernel_new( "H", h, HLEN );

		print( "k: " );
		print_kernel( k );
		print( "\r\n" );

		float *x = mem( sizeof(float)*SLEN );
		x[32]=1; x[33]=2; x[34]=3; x[35]=2; x[36]=1;
		
		print( "x: " );
		print_floats( x, SLEN );
		print( "\r\n" );

		float *y = mem( sizeof(float)*SLEN );		
		struct convolve_task T;
		T.in = x + 32;
		T.out = y + 32;
		T.len = 64;
		T.k = k;
		convolve( &T );
		
		print( "y: " );
		print_floats( y, SLEN );
		print( "\r\n" );
		
		// --------------------------------------------------
		memset( y, 0, sizeof(float)*SLEN );
		struct convolve_task t1, t2; t1.k = t2.k = k;
		t1.in = x +32;
		t1.out = y +32;
		t1.len = 32;
		t2.in = x +64;
		t2.out = y +64;
		t2.len = 32;
		convolve( &t1 );
		convolve( &t2 );
		
		print( "y: " );
		print_floats( y, SLEN );
		print( "\r\n" );

	}
	*/

	// -----------------------------------------------------------------------------------------------
	// --- TEST if 4 blocks 'convolve' is the same as 1 block 'convolve_naive' ---
	
	void fill_floats( float *p, int n ){
		for( int i=0; i<n; i++ )
			*p = rand() % 10; }
	
	void compare_floats( float *a, float *b, int n ){
		for( int i=0; i<n; i++ )
			if( *a == *b ) print( "." );
			else print( "X" ); }
	
	int main( int, int ){
		
		#define HLEN 32
		#define SLEN 96
		
		float *h = mem( sizeof(float)*HLEN );
		float *x = mem( sizeof(float)*SLEN );
		float *y1 = mem( sizeof(float)*SLEN );
		float *y2 = mem( sizeof(float)*SLEN );

		fill_floats( h, SLEN );
		fill_floats( x, SLEN );
		
		struct convolve_kernel
			*k = convolve_kernel_new( "H", h, HLEN );

		struct convolve_task T;
		
		T.k = k;
		T.in = x + 32;
		T.out = y1 + 32;
		T.len = 64;
		convolve_naive( &T );
		T.out = y2 + 32;
		convolve( &T );
		compare_floats( y1, y2, SLEN );
		
		// ------------------------------------------
		memset( y2, 0, sizeof(float)*SLEN );
		for( int i=0; i<4; i++ ){
			T.in = x + 32 + i*16;
			T.out = y2 + 32 + i*16;
			T.len = 16;
			convolve( &T ); }
		compare_floats( y1, y2, SLEN );
		
	}
	// -----------------------------------------------------------------------------------------------

#endif