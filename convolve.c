

    ////////////////////////////////////////////////////////////////////////////////////////
    //
    //  Original code by
    //  Henry Gomersall <heng@cantab.net>
    //  https://github.com/hgomersall/SSE-convolution/
    //
    ////////////////////////////////////////////////////////////////////////////////////////

    // /mingw64/bin/gcc -std=c99 -Wall -O3 -msse3 -mavx -march=native -fno-aggressive-loop-optimizations -fPIC -MD -o convolve.o -c convolve.c -D convolve_EXPORTS
    // /mingw64/bin/gcc -o convolve.dll convolve.o -s -shared -Wl,--subsystem,windows
    // cp -f convolve.dll lib
    // rm convolve.o convolve.d

    #include <immintrin.h>

    #define SSE_SIMD_LENGTH 4
    #define AVX_SIMD_LENGTH 8
    #define KERNEL_LENGTH 16
    #define ALIGNMENT 32
    #define VECTOR_LENGTH 16


    int convolve_0 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_naive
        for(int i=0; i<=length-kernel_length; i++){
            out[i] = 0.0;
            for(int k=0; k<kernel_length; k++){
                out[i] += in[i+k] * kernel[kernel_length - k - 1]; } }
        return 0; }


    int convolve_1 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_sse_partial_unroll
        __m128 kernel_reverse[kernel_length] __attribute__ ((aligned (16)));    
        __m128 data_block __attribute__ ((aligned (16)));
        __m128 prod __attribute__ ((aligned (16)));
        __m128 acc __attribute__ ((aligned (16)));
        for(int i=0; i<kernel_length; i++){
            kernel_reverse[i] = _mm_set1_ps(kernel[kernel_length - i - 1]); }
        for(int i=0; i<length-kernel_length; i+=4){
            acc = _mm_setzero_ps();
            for(int k=0; k<kernel_length; k+=4){
                int data_offset = i + k;
                for (int l = 0; l < 4; l++){
                    data_block = _mm_loadu_ps(in + data_offset + l);
                    prod = _mm_mul_ps(kernel_reverse[k+l], data_block);
                    acc = _mm_add_ps(acc, prod); } }
            _mm_storeu_ps(out+i, acc); }
        int i = length - kernel_length;
        out[i] = 0.0;
        for(int k=0; k<kernel_length; k++){
            out[i] += in[i+k] * kernel[kernel_length - k - 1]; }
        return 0; }


    int convolve_2_16 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_avx_unrolled_vector_unaligned_fma
        __m256 kernel_reverse[KERNEL_LENGTH] __attribute__ ((aligned (ALIGNMENT)));
        __m256 data_block __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc0 __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc1 __attribute__ ((aligned (ALIGNMENT)));
        for(int i=0; i<KERNEL_LENGTH; i++){
            kernel_reverse[i] = _mm256_broadcast_ss(&kernel[KERNEL_LENGTH - i - 1]); }
        for(int i=0; i<length-KERNEL_LENGTH; i+=VECTOR_LENGTH){
            acc0 = _mm256_setzero_ps();
            acc1 = _mm256_setzero_ps();
            for(int k=0; k<KERNEL_LENGTH; k+=VECTOR_LENGTH){
                int data_offset = i + k;
                for (int l = 0; l < SSE_SIMD_LENGTH; l++){
                    for (int m = 0; m < VECTOR_LENGTH; m+=SSE_SIMD_LENGTH) {
                        data_block = _mm256_loadu_ps(in + l + data_offset + m);
                        acc0 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc0);
                        data_block = _mm256_loadu_ps(in + l + data_offset + m + AVX_SIMD_LENGTH);
                        acc1 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc1); } } }
            _mm256_storeu_ps(out+i, acc0);
            _mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); }
        int i = length - KERNEL_LENGTH;
        out[i] = 0.0;
        for(int k=0; k<KERNEL_LENGTH; k++){
            out[i] += in[i+k] * kernel[KERNEL_LENGTH - k - 1]; }
        return 0; }


    ////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////

    
    #undef KERNEL_LENGTH
    #define KERNEL_LENGTH 32
    int convolve_2_32 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_avx_unrolled_vector_unaligned_fma
        __m256 kernel_reverse[KERNEL_LENGTH] __attribute__ ((aligned (ALIGNMENT)));
        __m256 data_block __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc0 __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc1 __attribute__ ((aligned (ALIGNMENT)));
        for(int i=0; i<KERNEL_LENGTH; i++){
            kernel_reverse[i] = _mm256_broadcast_ss(&kernel[KERNEL_LENGTH - i - 1]); }
        for(int i=0; i<length-KERNEL_LENGTH; i+=VECTOR_LENGTH){
            acc0 = _mm256_setzero_ps();
            acc1 = _mm256_setzero_ps();
            for(int k=0; k<KERNEL_LENGTH; k+=VECTOR_LENGTH){
                int data_offset = i + k;
                for (int l = 0; l < SSE_SIMD_LENGTH; l++){
                    for (int m = 0; m < VECTOR_LENGTH; m+=SSE_SIMD_LENGTH) {
                        data_block = _mm256_loadu_ps(in + l + data_offset + m);
                        acc0 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc0);
                        data_block = _mm256_loadu_ps(in + l + data_offset + m + AVX_SIMD_LENGTH);
                        acc1 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc1); } } }
            _mm256_storeu_ps(out+i, acc0);
            _mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); }
        int i = length - KERNEL_LENGTH;
        out[i] = 0.0;
        for(int k=0; k<KERNEL_LENGTH; k++){
            out[i] += in[i+k] * kernel[KERNEL_LENGTH - k - 1]; }
        return 0; }


    #undef KERNEL_LENGTH
    #define KERNEL_LENGTH 64
    int convolve_2_64 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_avx_unrolled_vector_unaligned_fma
        __m256 kernel_reverse[KERNEL_LENGTH] __attribute__ ((aligned (ALIGNMENT)));
        __m256 data_block __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc0 __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc1 __attribute__ ((aligned (ALIGNMENT)));
        for(int i=0; i<KERNEL_LENGTH; i++){
            kernel_reverse[i] = _mm256_broadcast_ss(&kernel[KERNEL_LENGTH - i - 1]); }
        for(int i=0; i<length-KERNEL_LENGTH; i+=VECTOR_LENGTH){
            acc0 = _mm256_setzero_ps();
            acc1 = _mm256_setzero_ps();
            for(int k=0; k<KERNEL_LENGTH; k+=VECTOR_LENGTH){
                int data_offset = i + k;
                for (int l = 0; l < SSE_SIMD_LENGTH; l++){
                    for (int m = 0; m < VECTOR_LENGTH; m+=SSE_SIMD_LENGTH) {
                        data_block = _mm256_loadu_ps(in + l + data_offset + m);
                        acc0 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc0);
                        data_block = _mm256_loadu_ps(in + l + data_offset + m + AVX_SIMD_LENGTH);
                        acc1 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc1); } } }
            _mm256_storeu_ps(out+i, acc0);
            _mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); }
        int i = length - KERNEL_LENGTH;
        out[i] = 0.0;
        for(int k=0; k<KERNEL_LENGTH; k++){
            out[i] += in[i+k] * kernel[KERNEL_LENGTH - k - 1]; }
        return 0; }


    #undef KERNEL_LENGTH
    #define KERNEL_LENGTH 128
    int convolve_2_128 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_avx_unrolled_vector_unaligned_fma
        __m256 kernel_reverse[KERNEL_LENGTH] __attribute__ ((aligned (ALIGNMENT)));
        __m256 data_block __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc0 __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc1 __attribute__ ((aligned (ALIGNMENT)));
        for(int i=0; i<KERNEL_LENGTH; i++){
            kernel_reverse[i] = _mm256_broadcast_ss(&kernel[KERNEL_LENGTH - i - 1]); }
        for(int i=0; i<length-KERNEL_LENGTH; i+=VECTOR_LENGTH){
            acc0 = _mm256_setzero_ps();
            acc1 = _mm256_setzero_ps();
            for(int k=0; k<KERNEL_LENGTH; k+=VECTOR_LENGTH){
                int data_offset = i + k;
                for (int l = 0; l < SSE_SIMD_LENGTH; l++){
                    for (int m = 0; m < VECTOR_LENGTH; m+=SSE_SIMD_LENGTH) {
                        data_block = _mm256_loadu_ps(in + l + data_offset + m);
                        acc0 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc0);
                        data_block = _mm256_loadu_ps(in + l + data_offset + m + AVX_SIMD_LENGTH);
                        acc1 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc1); } } }
            _mm256_storeu_ps(out+i, acc0);
            _mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); }
        int i = length - KERNEL_LENGTH;
        out[i] = 0.0;
        for(int k=0; k<KERNEL_LENGTH; k++){
            out[i] += in[i+k] * kernel[KERNEL_LENGTH - k - 1]; }
        return 0; }


    #undef KERNEL_LENGTH
    #define KERNEL_LENGTH 256
    int convolve_2_256 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_avx_unrolled_vector_unaligned_fma
        __m256 kernel_reverse[KERNEL_LENGTH] __attribute__ ((aligned (ALIGNMENT)));
        __m256 data_block __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc0 __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc1 __attribute__ ((aligned (ALIGNMENT)));
        for(int i=0; i<KERNEL_LENGTH; i++){
            kernel_reverse[i] = _mm256_broadcast_ss(&kernel[KERNEL_LENGTH - i - 1]); }
        for(int i=0; i<length-KERNEL_LENGTH; i+=VECTOR_LENGTH){
            acc0 = _mm256_setzero_ps();
            acc1 = _mm256_setzero_ps();
            for(int k=0; k<KERNEL_LENGTH; k+=VECTOR_LENGTH){
                int data_offset = i + k;
                for (int l = 0; l < SSE_SIMD_LENGTH; l++){
                    for (int m = 0; m < VECTOR_LENGTH; m+=SSE_SIMD_LENGTH) {
                        data_block = _mm256_loadu_ps(in + l + data_offset + m);
                        acc0 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc0);
                        data_block = _mm256_loadu_ps(in + l + data_offset + m + AVX_SIMD_LENGTH);
                        acc1 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc1); } } }
            _mm256_storeu_ps(out+i, acc0);
            _mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); }
        int i = length - KERNEL_LENGTH;
        out[i] = 0.0;
        for(int k=0; k<KERNEL_LENGTH; k++){
            out[i] += in[i+k] * kernel[KERNEL_LENGTH - k - 1]; }
        return 0; }


    #undef KERNEL_LENGTH
    #define KERNEL_LENGTH 512
    int convolve_2_512 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_avx_unrolled_vector_unaligned_fma
        __m256 kernel_reverse[KERNEL_LENGTH] __attribute__ ((aligned (ALIGNMENT)));
        __m256 data_block __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc0 __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc1 __attribute__ ((aligned (ALIGNMENT)));
        for(int i=0; i<KERNEL_LENGTH; i++){
            kernel_reverse[i] = _mm256_broadcast_ss(&kernel[KERNEL_LENGTH - i - 1]); }
        for(int i=0; i<length-KERNEL_LENGTH; i+=VECTOR_LENGTH){
            acc0 = _mm256_setzero_ps();
            acc1 = _mm256_setzero_ps();
            for(int k=0; k<KERNEL_LENGTH; k+=VECTOR_LENGTH){
                int data_offset = i + k;
                for (int l = 0; l < SSE_SIMD_LENGTH; l++){
                    for (int m = 0; m < VECTOR_LENGTH; m+=SSE_SIMD_LENGTH) {
                        data_block = _mm256_loadu_ps(in + l + data_offset + m);
                        acc0 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc0);
                        data_block = _mm256_loadu_ps(in + l + data_offset + m + AVX_SIMD_LENGTH);
                        acc1 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc1); } } }
            _mm256_storeu_ps(out+i, acc0);
            _mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); }
        int i = length - KERNEL_LENGTH;
        out[i] = 0.0;
        for(int k=0; k<KERNEL_LENGTH; k++){
            out[i] += in[i+k] * kernel[KERNEL_LENGTH - k - 1]; }
        return 0; }


    #undef KERNEL_LENGTH
    #define KERNEL_LENGTH 1024
    int convolve_2_1024 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_avx_unrolled_vector_unaligned_fma
        __m256 kernel_reverse[KERNEL_LENGTH] __attribute__ ((aligned (ALIGNMENT)));
        __m256 data_block __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc0 __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc1 __attribute__ ((aligned (ALIGNMENT)));
        for(int i=0; i<KERNEL_LENGTH; i++){
            kernel_reverse[i] = _mm256_broadcast_ss(&kernel[KERNEL_LENGTH - i - 1]); }
        for(int i=0; i<length-KERNEL_LENGTH; i+=VECTOR_LENGTH){
            acc0 = _mm256_setzero_ps();
            acc1 = _mm256_setzero_ps();
            for(int k=0; k<KERNEL_LENGTH; k+=VECTOR_LENGTH){
                int data_offset = i + k;
                for (int l = 0; l < SSE_SIMD_LENGTH; l++){
                    for (int m = 0; m < VECTOR_LENGTH; m+=SSE_SIMD_LENGTH) {
                        data_block = _mm256_loadu_ps(in + l + data_offset + m);
                        acc0 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc0);
                        data_block = _mm256_loadu_ps(in + l + data_offset + m + AVX_SIMD_LENGTH);
                        acc1 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc1); } } }
            _mm256_storeu_ps(out+i, acc0);
            _mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); }
        int i = length - KERNEL_LENGTH;
        out[i] = 0.0;
        for(int k=0; k<KERNEL_LENGTH; k++){
            out[i] += in[i+k] * kernel[KERNEL_LENGTH - k - 1]; }
        return 0; }


    #undef KERNEL_LENGTH
    #define KERNEL_LENGTH 2048
    int convolve_2_2048 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_avx_unrolled_vector_unaligned_fma
        __m256 kernel_reverse[KERNEL_LENGTH] __attribute__ ((aligned (ALIGNMENT)));
        __m256 data_block __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc0 __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc1 __attribute__ ((aligned (ALIGNMENT)));
        for(int i=0; i<KERNEL_LENGTH; i++){
            kernel_reverse[i] = _mm256_broadcast_ss(&kernel[KERNEL_LENGTH - i - 1]); }
        for(int i=0; i<length-KERNEL_LENGTH; i+=VECTOR_LENGTH){
            acc0 = _mm256_setzero_ps();
            acc1 = _mm256_setzero_ps();
            for(int k=0; k<KERNEL_LENGTH; k+=VECTOR_LENGTH){
                int data_offset = i + k;
                for (int l = 0; l < SSE_SIMD_LENGTH; l++){
                    for (int m = 0; m < VECTOR_LENGTH; m+=SSE_SIMD_LENGTH) {
                        data_block = _mm256_loadu_ps(in + l + data_offset + m);
                        acc0 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc0);
                        data_block = _mm256_loadu_ps(in + l + data_offset + m + AVX_SIMD_LENGTH);
                        acc1 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc1); } } }
            _mm256_storeu_ps(out+i, acc0);
            _mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); }
        int i = length - KERNEL_LENGTH;
        out[i] = 0.0;
        for(int k=0; k<KERNEL_LENGTH; k++){
            out[i] += in[i+k] * kernel[KERNEL_LENGTH - k - 1]; }
        return 0; }


    #undef KERNEL_LENGTH
    #define KERNEL_LENGTH 4096
    int convolve_2_4096 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_avx_unrolled_vector_unaligned_fma
        __m256 kernel_reverse[KERNEL_LENGTH] __attribute__ ((aligned (ALIGNMENT)));
        __m256 data_block __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc0 __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc1 __attribute__ ((aligned (ALIGNMENT)));
        for(int i=0; i<KERNEL_LENGTH; i++){
            kernel_reverse[i] = _mm256_broadcast_ss(&kernel[KERNEL_LENGTH - i - 1]); }
        for(int i=0; i<length-KERNEL_LENGTH; i+=VECTOR_LENGTH){
            acc0 = _mm256_setzero_ps();
            acc1 = _mm256_setzero_ps();
            for(int k=0; k<KERNEL_LENGTH; k+=VECTOR_LENGTH){
                int data_offset = i + k;
                for (int l = 0; l < SSE_SIMD_LENGTH; l++){
                    for (int m = 0; m < VECTOR_LENGTH; m+=SSE_SIMD_LENGTH) {
                        data_block = _mm256_loadu_ps(in + l + data_offset + m);
                        acc0 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc0);
                        data_block = _mm256_loadu_ps(in + l + data_offset + m + AVX_SIMD_LENGTH);
                        acc1 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc1); } } }
            _mm256_storeu_ps(out+i, acc0);
            _mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); }
        int i = length - KERNEL_LENGTH;
        out[i] = 0.0;
        for(int k=0; k<KERNEL_LENGTH; k++){
            out[i] += in[i+k] * kernel[KERNEL_LENGTH - k - 1]; }
        return 0; }


    #undef KERNEL_LENGTH
    #define KERNEL_LENGTH 8192
    int convolve_2_8192 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_avx_unrolled_vector_unaligned_fma
        __m256 kernel_reverse[KERNEL_LENGTH] __attribute__ ((aligned (ALIGNMENT)));
        __m256 data_block __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc0 __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc1 __attribute__ ((aligned (ALIGNMENT)));
        for(int i=0; i<KERNEL_LENGTH; i++){
            kernel_reverse[i] = _mm256_broadcast_ss(&kernel[KERNEL_LENGTH - i - 1]); }
        for(int i=0; i<length-KERNEL_LENGTH; i+=VECTOR_LENGTH){
            acc0 = _mm256_setzero_ps();
            acc1 = _mm256_setzero_ps();
            for(int k=0; k<KERNEL_LENGTH; k+=VECTOR_LENGTH){
                int data_offset = i + k;
                for (int l = 0; l < SSE_SIMD_LENGTH; l++){
                    for (int m = 0; m < VECTOR_LENGTH; m+=SSE_SIMD_LENGTH) {
                        data_block = _mm256_loadu_ps(in + l + data_offset + m);
                        acc0 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc0);
                        data_block = _mm256_loadu_ps(in + l + data_offset + m + AVX_SIMD_LENGTH);
                        acc1 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc1); } } }
            _mm256_storeu_ps(out+i, acc0);
            _mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); }
        int i = length - KERNEL_LENGTH;
        out[i] = 0.0;
        for(int k=0; k<KERNEL_LENGTH; k++){
            out[i] += in[i+k] * kernel[KERNEL_LENGTH - k - 1]; }
        return 0; }


    #undef KERNEL_LENGTH
    #define KERNEL_LENGTH 16384
    int convolve_2_16384 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_avx_unrolled_vector_unaligned_fma
        __m256 kernel_reverse[KERNEL_LENGTH] __attribute__ ((aligned (ALIGNMENT)));
        __m256 data_block __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc0 __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc1 __attribute__ ((aligned (ALIGNMENT)));
        for(int i=0; i<KERNEL_LENGTH; i++){
            kernel_reverse[i] = _mm256_broadcast_ss(&kernel[KERNEL_LENGTH - i - 1]); }
        for(int i=0; i<length-KERNEL_LENGTH; i+=VECTOR_LENGTH){
            acc0 = _mm256_setzero_ps();
            acc1 = _mm256_setzero_ps();
            for(int k=0; k<KERNEL_LENGTH; k+=VECTOR_LENGTH){
                int data_offset = i + k;
                for (int l = 0; l < SSE_SIMD_LENGTH; l++){
                    for (int m = 0; m < VECTOR_LENGTH; m+=SSE_SIMD_LENGTH) {
                        data_block = _mm256_loadu_ps(in + l + data_offset + m);
                        acc0 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc0);
                        data_block = _mm256_loadu_ps(in + l + data_offset + m + AVX_SIMD_LENGTH);
                        acc1 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc1); } } }
            _mm256_storeu_ps(out+i, acc0);
            _mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); }
        int i = length - KERNEL_LENGTH;
        out[i] = 0.0;
        for(int k=0; k<KERNEL_LENGTH; k++){
            out[i] += in[i+k] * kernel[KERNEL_LENGTH - k - 1]; }
        return 0; }


    #undef KERNEL_LENGTH
    #define KERNEL_LENGTH 32768
    int convolve_2_32768 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_avx_unrolled_vector_unaligned_fma
        __m256 kernel_reverse[KERNEL_LENGTH] __attribute__ ((aligned (ALIGNMENT)));
        __m256 data_block __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc0 __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc1 __attribute__ ((aligned (ALIGNMENT)));
        for(int i=0; i<KERNEL_LENGTH; i++){
            kernel_reverse[i] = _mm256_broadcast_ss(&kernel[KERNEL_LENGTH - i - 1]); }
        for(int i=0; i<length-KERNEL_LENGTH; i+=VECTOR_LENGTH){
            acc0 = _mm256_setzero_ps();
            acc1 = _mm256_setzero_ps();
            for(int k=0; k<KERNEL_LENGTH; k+=VECTOR_LENGTH){
                int data_offset = i + k;
                for (int l = 0; l < SSE_SIMD_LENGTH; l++){
                    for (int m = 0; m < VECTOR_LENGTH; m+=SSE_SIMD_LENGTH) {
                        data_block = _mm256_loadu_ps(in + l + data_offset + m);
                        acc0 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc0);
                        data_block = _mm256_loadu_ps(in + l + data_offset + m + AVX_SIMD_LENGTH);
                        acc1 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc1); } } }
            _mm256_storeu_ps(out+i, acc0);
            _mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); }
        int i = length - KERNEL_LENGTH;
        out[i] = 0.0;
        for(int k=0; k<KERNEL_LENGTH; k++){
            out[i] += in[i+k] * kernel[KERNEL_LENGTH - k - 1]; }
        return 0; }


    #undef KERNEL_LENGTH
    #define KERNEL_LENGTH 65536
    int convolve_2_65536 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_avx_unrolled_vector_unaligned_fma
        __m256 kernel_reverse[KERNEL_LENGTH] __attribute__ ((aligned (ALIGNMENT)));
        __m256 data_block __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc0 __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc1 __attribute__ ((aligned (ALIGNMENT)));
        for(int i=0; i<KERNEL_LENGTH; i++){
            kernel_reverse[i] = _mm256_broadcast_ss(&kernel[KERNEL_LENGTH - i - 1]); }
        for(int i=0; i<length-KERNEL_LENGTH; i+=VECTOR_LENGTH){
            acc0 = _mm256_setzero_ps();
            acc1 = _mm256_setzero_ps();
            for(int k=0; k<KERNEL_LENGTH; k+=VECTOR_LENGTH){
                int data_offset = i + k;
                for (int l = 0; l < SSE_SIMD_LENGTH; l++){
                    for (int m = 0; m < VECTOR_LENGTH; m+=SSE_SIMD_LENGTH) {
                        data_block = _mm256_loadu_ps(in + l + data_offset + m);
                        acc0 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc0);
                        data_block = _mm256_loadu_ps(in + l + data_offset + m + AVX_SIMD_LENGTH);
                        acc1 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc1); } } }
            _mm256_storeu_ps(out+i, acc0);
            _mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); }
        int i = length - KERNEL_LENGTH;
        out[i] = 0.0;
        for(int k=0; k<KERNEL_LENGTH; k++){
            out[i] += in[i+k] * kernel[KERNEL_LENGTH - k - 1]; }
        return 0; }


    #undef KERNEL_LENGTH
    #define KERNEL_LENGTH 131072
    int convolve_2_131072 (float* in, float* out, int length, float* kernel, int kernel_length){
        // original name: convolve_avx_unrolled_vector_unaligned_fma
        __m256 kernel_reverse[KERNEL_LENGTH] __attribute__ ((aligned (ALIGNMENT)));
        __m256 data_block __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc0 __attribute__ ((aligned (ALIGNMENT)));
        __m256 acc1 __attribute__ ((aligned (ALIGNMENT)));
        for(int i=0; i<KERNEL_LENGTH; i++){
            kernel_reverse[i] = _mm256_broadcast_ss(&kernel[KERNEL_LENGTH - i - 1]); }
        for(int i=0; i<length-KERNEL_LENGTH; i+=VECTOR_LENGTH){
            acc0 = _mm256_setzero_ps();
            acc1 = _mm256_setzero_ps();
            for(int k=0; k<KERNEL_LENGTH; k+=VECTOR_LENGTH){
                int data_offset = i + k;
                for (int l = 0; l < SSE_SIMD_LENGTH; l++){
                    for (int m = 0; m < VECTOR_LENGTH; m+=SSE_SIMD_LENGTH) {
                        data_block = _mm256_loadu_ps(in + l + data_offset + m);
                        acc0 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc0);
                        data_block = _mm256_loadu_ps(in + l + data_offset + m + AVX_SIMD_LENGTH);
                        acc1 = _mm256_fmadd_ps(kernel_reverse[k+l+m], data_block, acc1); } } }
            _mm256_storeu_ps(out+i, acc0);
            _mm256_storeu_ps(out+i+AVX_SIMD_LENGTH, acc1); }
        int i = length - KERNEL_LENGTH;
        out[i] = 0.0;
        for(int k=0; k<KERNEL_LENGTH; k++){
            out[i] += in[i+k] * kernel[KERNEL_LENGTH - k - 1]; }
        return 0; }

