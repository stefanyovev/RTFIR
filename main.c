

	#include <stdio.h>
	#include <stdlib.h>
	#include <stdint.h>
	#include <string.h>
	#include <math.h>
	#include <immintrin.h>	
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
	#include "conf.c"
	#include "frontend.c"


	/*

		Todos:

			- test convolve btn
			- reload filters btn
			- save conf btn

			- private portaudio; device, devs, ndevs; names
			- output buffers, processed cursors
			- link object; port-port; port-effect; effect-effect; effect-port
			- threads submit on input
			
			- mixing
			
			- device-aggregation
				- threads instances; processing the input to the virtual canvas from more than 1
					device will require more than 1 thread pools to know the processed cursors
					of individual channels

			- frontend as object; create, run, events, callbacks; add_device; on_play_clicked
				- scene-graph of rects/widgets; nested, clipped; window/font/text/rect/bitmap

	*/