

	// fast print function
	// prints to memory / exposes an alwayss valid string for reading / may mix content but not crash or block
	
	// init() first
	// max print bytes = width*height else no-operation.
	// set label text *console
	
	// data len = width*height + place for endbyte + the maximum count of newlines we may put to wrap (height-1)

	// tests are for 3x3
	
	// TODO: when wrapped its more than HEIGHT
	// TODO: mode without wrapping
	// TOOD: remove wrapping too complicated for no gain
	// TODO: rm init - if static initialilzed
	// TODO: when printing a single cahracter does nothing sometimes
	// TODO: so we can log and print - make it just a list of 250byte blocks. cursor pointing first line.
	//       to print replace the string pointer by cursor and cursor++

	#if __INCLUDE_LEVEL__ == 0
		#include <stdio.h>
		#include <stdlib.h>
		#include <string.h>
	#endif


	#define WIDTH 80
	#define HEIGHT 15
	#define SIZE (WIDTH*HEIGHT)

	volatile char volatile *console;
	volatile char *console_data, *console_tmp1, *console_tmp2;
	volatile int console_len, console_lll;  // static
	volatile int console_changed;


	void print ( char *format, ... ){  // max width*height
		
		static int initialized=0;
		if( !initialized ){
			console_data = malloc( SIZE +HEIGHT-1 +1 );  // Width*Height +maxnewlines +endbyte
			console_data [ SIZE+HEIGHT-1 ] = 0;              // last addr 
			
			memset( console_data, 65, SIZE );  // unused
			memset( console_data +SIZE, '\n', HEIGHT-1 );  // "\n\n\0"
			
			//printf( "\n" ); for( int i=0; i<12; i++ ) printf( "%2d ", i ); printf( "\n" );  // addr
			//printf( "\n" ); for( int i=0; i<SIZE+HEIGHT; i++ ) printf( "%d ", (unsigned char)(console_data[i]) ); printf( "\n" );  // content
			//exit(1);
			
			console = console_data +SIZE;    // start = last addr -newlines (first newline addr)
			
			//printf( "\n---console---\n%s\n---/console---", console );
			//exit(1);
					
			console_tmp1 = malloc( SIZE +1 ); // same as data without maxnewlines; +endbyte (so you can printf it when its full / vsn may not put null)
			console_tmp1 [SIZE] = 0;          // last addr
			memset( console_tmp1, 65, SIZE );  // unused

			//printf( "\n---tmp1---\n%s\n---/tmp1---", console_tmp1 ); // should say AAAA x SIZE not crash
			//exit(1);
					
			console_tmp2 = malloc( SIZE +HEIGHT );  // same as data
			console_tmp2 [SIZE+HEIGHT-1] = 0;          // last addr
			memset( console_tmp2, 65, SIZE+HEIGHT-1 );  // unused

			//printf( "\n---tmp2---\n%s\n---/tmp2---", console_tmp2 ); // should say AAAA x (SIZE+(height-1)) not crash
			//exit(1);

			console_len = HEIGHT-1;  // content-length; without the null; now full with newlines x(height-1)
			console_lll = 0;  // lastlinelen
			console_changed = 1;

			initialized = 1;
		}
		
		int inlen;
	
		// expand "%".
		va_list( args );
		va_start( args, format );
		inlen = vsnprintf( console_tmp1, SIZE, format, args );  // max width*height expanded also
		// vsn may or may not put null at end but retval is always without
		va_end( args );
		
		if( inlen <= 0 )  // error during formatting (<0) or too much input (==0)
			return;  // give up
		
		// expand newlines
		int j=0;
		for( int i=0; i<inlen; i++ ){
			
			if( console_lll == WIDTH && console_tmp1[i] != '\n' ){
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
		if( inlen == SIZE+HEIGHT-1 ){  // totally expanded len == total storage (without the null)
			memcpy( console_data, console_tmp2, SIZE +HEIGHT-1 );
			console = console_data;
			//printf( "\n---console---\n%s\n---/console---", console );
			//exit(1);
			console_changed = 1;
			return;  // done
			}
		
		// scroll
		memmove( console_data, console_data +inlen, SIZE +HEIGHT-1 -inlen );
			
		// copy
		memcpy( console_data +SIZE +HEIGHT-1 -inlen, console_tmp2, inlen );

		// printf( "\n" ); for( int i=0; i<12; i++ ) printf( "%2d ", i ); printf( "\n" );  // addr
		// printf( "\n" ); for( int i=0; i<SIZE+HEIGHT; i++ ) printf( "%d ", (unsigned char)(console_data[i]) ); printf( "\n" );  // content
		// exit(1);
		
		// count
		int lines = 1;
		int ofs = SIZE+HEIGHT-1;		
		
		for( ; ofs >= 0 && ofs >SIZE+HEIGHT-1 -console_len ; ofs-- ){
			
			if( console_data[ofs] == '\n' ){
				
				if( lines == HEIGHT ){
					ofs++;
					break;
				}
				lines ++;
			}

		}
		//printf( "[ofs %d]", ofs );
		
		// set
		console_len = SIZE+HEIGHT-1 -ofs;
		console = console_data +ofs ;
		console_changed = 1;

		//printf( "\n---console---\n%s\n---/console---", console );
		//exit(1);

		}

	#undef WIDTH
	#undef HEIGHT
	#undef SIZE



	#if __INCLUDE_LEVEL__ == 0
	
		int main( int argc, char **argv ){
			
			console_init();
			
			print("AAABBBCCC");
			print("AA\nBB\nCCC");
			print("??");
			print("a\n");
			print("ab\n");
			print("\n");
			print("123123123");
			print("AB");
			print("AB\npp");
			
			printf("--- console ---\n%s\n--- /console --- \n", console );

		}
	
	#endif
	
