

	// Thread-Pool
	// threads_init(n); threads_submit( &func, &arg, argsize ); threads_wait();

	// tasks queue:
	// 001011122222222000000000000000
	//   T    B       H
	//  tail  body    head

	// TODO: track queue usage %/MB = (head > tail) ? (head-tail)/20MB : (tail-head)/20MB; notify on failure
	// TODO: use auto reset events to wake single

	#include <stdlib.h>   // memcpy
	#include <windows.h>  // CreateThread, CreateMutex, CreateEvent, WaitForSingleObject, SetEvent, ReleaseMutex, ResetEvent, HANDLE


	#define THREADS_MAX_COUNT 100
	#define THREADS_STACK_SIZE 10 // megabytes
	#define THREADS_QUEUE_SIZE 20 // megabytes


	struct threads_task {
		int status;  // 0- done; 1- started; 2- queued
		struct threads_task *next;  // iterably
		void (*function)( void * );
		int argsize;
		int arg;  // used for the address only
	};

	struct threads_thread {
		int status;  //  -1 stopped; 0 done/waiting; 1 work/working; 2 unused; 3 stop sig
		struct threads_task *task;
	};


	// GLOBALS
	volatile struct threads_thread threads [THREADS_MAX_COUNT];
	int threads_count;
	
	struct threads_task *threads_queue;  // all tasks queue
	HANDLE threads_queue_mutex;  // put | get | remove
	HANDLE threads_queue_empty; // event
	HANDLE threads_queue_avail; // event
	
	volatile struct threads_task volatile
		*threads_queue_head,  // where to put the new one / first with status 0
		*threads_queue_body,  // which to take / first with status 2 / can be null
		*threads_queue_tail   // the oldest / first with status 1 / can be null
		;


	// MAIN
	static void threads_queue_lock(){ while( WaitForSingleObject( threads_queue_mutex, INFINITE ) != 0 ) PRINT("X"); }
	static void threads_queue_unlock(){ ReleaseMutex( threads_queue_mutex ); }
	static void threads_wait_empty(){ while( WaitForSingleObject( threads_queue_empty, INFINITE ) != 0 ) PRINT("X"); }
	static void threads_wait_avail(){ while( WaitForSingleObject( threads_queue_avail, INFINITE ) != 0 ) PRINT("X"); }

	void threads_submit( void *function, void *arg, int argsize ){

		threads_queue_lock();
		memcpy( &(threads_queue_head->arg), arg, argsize );
		threads_queue_head->argsize = argsize;
		threads_queue_head->function = function;
		threads_queue_head->next = (void*)threads_queue_head +sizeof(struct threads_task) -sizeof(int) +argsize;
		if( ( (void*)(threads_queue_head->next) - (void*)( threads_queue ) ) > THREADS_QUEUE_SIZE * 1024 * 1024 )
			threads_queue_head->next = threads_queue;	
		threads_queue_head->status = 2;  // queued
		
		if( !threads_queue_body ){
			threads_queue_body = threads_queue_head;
			ResetEvent( threads_queue_empty );
			SetEvent( threads_queue_avail );
			}
		threads_queue_head = threads_queue_head->next;
		threads_queue_unlock();
	}

	struct threads_task * threads_queue_get(){  
		struct threads_task *T;
		while(1){
			threads_wait_avail();
			threads_queue_lock();
			if( !threads_queue_body ) { threads_queue_unlock(); continue; }  // fake alarm
			T = threads_queue_body;
			T->status = 1;  // taken
			if( T->next->status == 2 ){
				threads_queue_body = T->next; }
			else {
				threads_queue_body = 0;
				ResetEvent( threads_queue_avail );
			}
			if( !threads_queue_tail )
				threads_queue_tail = T;
			threads_queue_unlock();
			return T;
		}
	}
	
	void threads_queue_delete( struct threads_task *T ){  

		threads_queue_lock();
		if( T == threads_queue_tail ){
			threads_queue_tail = 0;
			struct threads_task *t = T;
			while( t = t->next ){
				if( t->status == 1 ){
					threads_queue_tail = t;
					break; }
				if( t->status == 2 )
					break;
				if( t == threads_queue_head ){
					SetEvent( threads_queue_empty );
					break; }
			}
		}
		T->status = 0 ;
		threads_queue_unlock();	
		}

	void threads_main( volatile struct threads_thread *self ){
		struct threads_task *T;
		while( 1 ){
			
			if( self->status == 3 ){
				self->status = -1;
				break; }

			self->status = 0;
			T = threads_queue_get();
			self->status = 1;
			T->function( &(T->arg) );
			threads_queue_delete( T );
		}
	}

	void threads_wait(){
		while(1){
			if( !threads_queue_tail && !threads_queue_body ){
				return;
			}
			threads_wait_empty();
			/*
			if( threads_queue_tail || threads_queue_body ){
				PRINT(" 4");
			}else{
				PRINT(" 5");
				return;
			}
			*/
		}
		}

	void threads_setcount( int count ){  // async
		for( int i=0; i<count; i++ )
			if( threads[i].status == -1 )
				CreateThread( 0, THREADS_STACK_SIZE * 1024 * 1024, &threads_main, threads+i, 0, 0 ); 
		for( int i=count; i<threads_count; i++ )
			threads[i].status = 3;
        threads_count = count;
		}

	int threads_init( int count ){ // -> bool; async
		for( int i=0; i<THREADS_MAX_COUNT; i++ )
			threads[i].status = -1;
		threads_queue = malloc( THREADS_QUEUE_SIZE * 1024 * 1024 );
		if( !threads_queue ) return 0;
		memset( threads_queue, 0, THREADS_QUEUE_SIZE * 1024 * 1024 );
		threads_queue_head = threads_queue;
		threads_queue_body = 0;
		threads_queue_tail = 0;
		threads_queue_mutex = CreateMutex( 0, 0, 0 );
		threads_queue_empty = CreateEvent( 0, 1, 1, 0 );
		threads_queue_avail = CreateEvent( 0, 1, 0, 0 );
		threads_setcount( count );
	}


	#undef THREADS_MAX_COUNT
	#undef THREADS_STACK_SIZE
	#undef THREADS_QUEUE_SIZE



//  gcc -w threads.c -fpermissive -o thr -lsynchronization -O3

#if __INCLUDE_LEVEL__ == 0
	
	#include <stdio.h>    // printf	
	// void (*print)( char *format, ... ) = &printf;

	/* should not cause any cpu usage
	int main( int, int ){
		threads_init( 5 );
		Sleep( 10 * 1000 );
		}
	*/

	/* should print hi 10 times
	void ff(){
		printf( "hi" ); }
	int main( int, int ){
		threads_init( 3 );
		for( int i=0; i<10; i++ )
			threads_submit( &ff, 0, 0 );
		threads_wait();
	}
	*/

	///* should print all the letters once. then 'done'.
	char *ch = "ABCDEFGHIJKLMNO\0";
	
	void thr( void *task ){
		printf( " doing %c \r\n", *((char*)task) ); }
	
	int main( int, int ){
		threads_init( 5 );
		for( int i=0; i<15; i++ )
			threads_submit( &thr, &ch[i], 1 );
		threads_wait(); 
		printf( " done \r\n" );
		}
	//*/
	
#endif
