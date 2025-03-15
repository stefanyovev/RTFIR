

	#include <stdio.h>
	#include <stdlib.h>
	#include <stdint.h>
	#include <string.h>
	#include <math.h>
	#include <windows.h>
	#include "portaudio.h"

	void error( char *msg ){
		printf( "error: %s", msg );
		MessageBox( GetActiveWindow(), msg, "error", MB_OK );
		exit(1); }

	#include "mem.c"
	#include "print.c"
	#include "clock.c"
	#include "threads.c"
	#include "convolve.c"
	#include "core.c"
	#include "frontend.c"


	/*

		Todos:

			- private portaudio; device, devs, ndevs; names

			- frontend as object; create, run, events, callbacks; add_device; on_play_clicked
				- scene-graph of rects/widgets; nested, clipped; window/font/text/rect/bitmap

			- output buffers, threads instances, threads submit on input, processed cursors
			- graph, link, mix; delay as object
	*/