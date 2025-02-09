

	// y[n] = x[n]*h[0] + x[n-1]*h[1] + x[n-2]*h[2]
	// reverse(h); cursor -= len(h)-1
	// y[n] = x[n]*h[0] + x[n+1]*h[1]
	
	// TODO: fix that last sample. from original code
	// TODO: convolve_naive

	#if __INCLUDE_LEVEL__ == 0
		#include <stdio.h>
		#include <stdint.h>
		#include <immintrin.h>	
		#define PRINT printf
		#define ERROR(x) { PRINT(x); exit(1); }
		#include "mem.c"
		#define MEM mem_alloc
		#define MEMA mem_alloc_aligned
		#define FREE mem_free
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
		struct convolve_kernel *R = MEM( sizeof( struct convolve_kernel ) );
		R->name = MEM( strlen( name )+1 );
		strcpy( R->name, name );
		R->len = ( kn / 32 ) * 32 + 32; // filter len multiple of 32 samples
		R->data = MEMA( R->len * sizeof(float) * 8, ALIGNMENT );
		for( int i=0; i<kn; i++ )
			R->data[ R->len-kn +i] = _mm256_broadcast_ss( k +kn-1 -i );
		return R; }

	#define SSE_SIMD_LENGTH 4
	#define AVX_SIMD_LENGTH 8
	#define VECTOR_LENGTH 16

	void convolve( void *task ){
		float* in = ((struct convolve_task*)task)->in;
		float* out = ((struct convolve_task*)task)->out;
		int length = ((struct convolve_task*)task)->len;
		__m256* kernel = ((struct convolve_task*)task)->k->data;
		int kernel_length = ((struct convolve_task*)task)->k->len;
		in = in -kernel_length;  // scipy convolve mode valid
		length = length +kernel_length;  // see in orig repo
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
		/*
		int i = length - kernel_length;
		out[i] = 0.0;
		for(int k=0; k<kernel_length; k++)
			out[i] += in[i+k] * (*((float*)(kernel +k )));
		*/
		}  

	#undef ALIGNMENT
	#undef SSE_SIMD_LENGTH
	#undef AVX_SIMD_LENGTH
	#undef VECTOR_LENGTH



//  gcc -w convolve.c -fpermissive -o cvl -march=native -O3

#if __INCLUDE_LEVEL__ == 0
	
	int main( int, int ){
		}
	
#endif