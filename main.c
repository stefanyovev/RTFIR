

	#include <stdio.h>
	#include <stdlib.h>
	#include <stdint.h>
	#include <string.h>
	#include <math.h>
	#include <immintrin.h>	
	#include "portaudio.h"
	#include <windows.h>

	#define ERROR(x) { MessageBox( GetActiveWindow(), x, "ERROR", MB_OK ); exit(1); }

	#include "mem.c"
	#define MEM mem_alloc
	#define MEMA mem_alloc_aligned
	#define FREE mem_free

	#include "console.c"
	#define PRINT console_print

	#include "clock.c"
	#define NOW clock_time
	
	#include "threads.c"
	#include "convolve.c"

	#include "core.c"
	#include "frontend.c"
