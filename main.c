

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
	#define MEM(x) mem_alloc(x)
	#define MEMA(x,y) mem_alloc_aligned(x,y)
	#define FREE(x) mem_free(x)

	#include "console.c"
	#define PRINT console_print

	#include "clock.c"
	#define NOW clock_time()
	
	#include "threads.c"
	#include "convolve.c"

	#include "core.c"
	#include "frontend.c"
