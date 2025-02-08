

	#include <stdio.h>
	#include <stdlib.h>
	#include <string.h>

	// fast print function

	// prints to memory / exposes an alwayss valid string for reading / may mix content but not crash or block
	
	// init() first
	// max print bytes = width*height else no-operation.
	// set label text *console
	
	// data len = width*height + place for endbyte + the maximum count of newlines we may put to wrap (height-1)

	// tests are for 3x3
	
	// TODO: when wrapped its more than CONSOLE_HEIGHT
	// TODO: mode without wrapping
	// TOOD: remove wrapping too complicated for no gain
	// TODO: rm init - if static initialilzed
    // TODO: when printing a single cahracter does nothing sometimes

	#define CONSOLE_WIDTH 80
	#define CONSOLE_HEIGHT 15
	#define CONSOLE_SIZE (CONSOLE_WIDTH*CONSOLE_HEIGHT)

	volatile char volatile *console;
	volatile char *console_data, *console_tmp1, *console_tmp2;
	volatile int console_len, console_lll;  // static
	volatile int console_changed;
	//HANDLE console_lock;

	void console_init(){
		
		console_data = malloc( CONSOLE_SIZE +CONSOLE_HEIGHT-1 +1 );  // Width*Height +maxnewlines +endbyte
		console_data [ CONSOLE_SIZE+CONSOLE_HEIGHT-1 ] = 0;              // last addr 
		
		memset( console_data, 65, CONSOLE_SIZE );  // unused
		memset( console_data +CONSOLE_SIZE, '\n', CONSOLE_HEIGHT-1 );  // "\n\n\0"
		
		//printf( "\n" ); for( int i=0; i<12; i++ ) printf( "%2d ", i ); printf( "\n" );  // addr
		//printf( "\n" ); for( int i=0; i<CONSOLE_SIZE+CONSOLE_HEIGHT; i++ ) printf( "%d ", (unsigned char)(console_data[i]) ); printf( "\n" );  // content
		//exit(1);
		
		console = console_data +CONSOLE_SIZE;    // start = last addr -newlines (first newline addr)
		
		//printf( "\n---console---\n%s\n---/console---", console );
		//exit(1);
				
		console_tmp1 = malloc( CONSOLE_SIZE +1 ); // same as data without maxnewlines; +endbyte (so you can printf it when its full / vsn may not put null)
		console_tmp1 [CONSOLE_SIZE] = 0;          // last addr
		memset( console_tmp1, 65, CONSOLE_SIZE );  // unused

		//printf( "\n---tmp1---\n%s\n---/tmp1---", console_tmp1 ); // should say AAAA x SIZE not crash
		//exit(1);
				
		console_tmp2 = malloc( CONSOLE_SIZE +CONSOLE_HEIGHT );  // same as data
		console_tmp2 [CONSOLE_SIZE+CONSOLE_HEIGHT-1] = 0;          // last addr
		memset( console_tmp2, 65, CONSOLE_SIZE+CONSOLE_HEIGHT-1 );  // unused

		//printf( "\n---tmp2---\n%s\n---/tmp2---", console_tmp2 ); // should say AAAA x (SIZE+(height-1)) not crash
		//exit(1);

		console_len = CONSOLE_HEIGHT-1;  // content-length; without the null; now full with newlines x(height-1)
		console_lll = 0;  // lastlinelen
		console_changed = 1;
		
		//console_lock = CreateMutex( 0, 0, 0 );
		}

	void console_print ( char *format, ... ){  // max width*height
		
		//WaitForSingleObject( console_lock, INFINITE );
		
		int inlen;
	
		// expand "%".
		va_list( args );
		va_start( args, format );
		inlen = vsnprintf( console_tmp1, CONSOLE_SIZE, format, args );  // max width*height expanded also
		// vsn may or may not put null at end but retval is always without
		va_end( args );
		
		if( inlen <= 0 )  // error during formatting (<0) or too much input (==0)
			return;  // give up
		
		// expand newlines
		int j=0;
		for( int i=0; i<inlen; i++ ){
			
			if( console_lll == CONSOLE_WIDTH && console_tmp1[i] != '\n' ){
				console_tmp2[j++] = '\n';
				console_lll = 0;
				console_len ++;
				}
			
			console_tmp2[j++] = console_tmp1[i];

			if( console_tmp1[i] == '\n' )
				console_lll = 0;
			else
				console_lll ++;
			
			console_len ++;
		}			
		//printf( "%d %d", inlen, j ); exit(1);
		inlen = j; 

		//console_tmp2[j] = 0; printf( "---inp---\n%s\n---/inp---\n", console_tmp2 ); exit(1);

		// copy
		if( inlen == CONSOLE_SIZE+CONSOLE_HEIGHT-1 ){  // totally expanded len == total storage (without the null)
			memcpy( console_data, console_tmp2, CONSOLE_SIZE +CONSOLE_HEIGHT-1 );
			console = console_data;
			//printf( "\n---console---\n%s\n---/console---", console );
			//exit(1);
			console_changed = 1;
			return;  // done
			}
		
		// scroll
		memmove( console_data, console_data +inlen, CONSOLE_SIZE +CONSOLE_HEIGHT-1 -inlen );
			
		// copy
		memcpy( console_data +CONSOLE_SIZE +CONSOLE_HEIGHT-1 -inlen, console_tmp2, inlen );

		// printf( "\n" ); for( int i=0; i<12; i++ ) printf( "%2d ", i ); printf( "\n" );  // addr
		// printf( "\n" ); for( int i=0; i<CONSOLE_SIZE+CONSOLE_HEIGHT; i++ ) printf( "%d ", (unsigned char)(console_data[i]) ); printf( "\n" );  // content
		// exit(1);
		
		// count
		int lines = 1;
		int ofs = CONSOLE_SIZE+CONSOLE_HEIGHT-1;		
		
		for( ; ofs >= 0 && ofs >CONSOLE_SIZE+CONSOLE_HEIGHT-1 -console_len ; ofs-- ){
			
			if( console_data[ofs] == '\n' ){
				
				if( lines == CONSOLE_HEIGHT ){
					ofs++;
					break;
				}
				lines ++;
			}

		}
		//printf( "[ofs %d]", ofs );
		
		// set
		console_len = CONSOLE_SIZE+CONSOLE_HEIGHT-1 -ofs;
		console = console_data +ofs ;
		console_changed = 1;

		//printf( "\n---console---\n%s\n---/console---", console );
		//exit(1);

		//ReleaseMutex( console_lock );
		}


	// -----------------------------------------------------------------------------------------------
	/*
	#define PRINT console_print

	int main( int argc, char **argv ){
		
		console_init();
		
		PRINT("AAABBBCCC");
		PRINT("AA\nBB\nCCC");
		PRINT("??");
		PRINT("a\n");
		PRINT("ab\n");
		PRINT("\n");
		PRINT("123123123");
		PRINT("AB");
		PRINT("AB\npp");
		
		printf("--- console ---\n%s\n--- /console --- \n", console );

		//char b [4] = { 0,1,2,3 };
		//printf( "%x %x %x %x \n", b[0], b[1], b[2], b[3] );
		//snprintf( b, 3, "AAAA" );
		//printf( "%x %x %x %x \n", b[0], b[1], b[2], b[3] );
	}
	*/
	
