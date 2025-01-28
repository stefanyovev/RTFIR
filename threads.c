

	// Thread-Pool
	// threads_init(n); threads_submit( &func, &arg, argsize ); threads_wait();

	// tasks queue:
	// 001011122222222000000000000000
	//   T    B       H
	//  tail  body    head

	// TODO: queue overflow ? add 1 global var to track queue usage %/MB; notify on failure
	// TODO: dont use waitonaddress; only events... create/set/reset/waitforsingleobject.. sig_task_available || sig_all_done || tail_only

	#include <stdlib.h>   // memcpy
	#include <windows.h>  // CreateThread, CreateMutex, WaitForSingleObject, WaitOnAddress, WakeByAddressSingle, HANDLE


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
	volatile int threads_queue_empty;
	
	volatile struct threads_task volatile
		*threads_queue_head,  // where to put the new one / first with status 0
		*threads_queue_body,  // which to take / first with status 2 / can be null
		*threads_queue_tail   // the oldest / first with status 1 / can be null
		;


	// MAIN
	inline static void threads_queue_lock(){ WaitForSingleObject( threads_queue_mutex, INFINITE ); }  // should return 0; case WAIT_ABANDONED ?	
	inline static void threads_queue_unlock(){ ReleaseMutex( threads_queue_mutex ); }
	inline static void threads_waitforend(){ int undesired_value = 0; WaitOnAddress( &threads_queue_empty, &undesired_value, sizeof(int), INFINITE ); }
	inline static void threads_wakeforend(){ WakeByAddressSingle( &threads_queue_empty ); }
	inline static void threads_waitfortask(){ struct threads_task *undesired_value = 0; WaitOnAddress( &threads_queue_body, &undesired_value, sizeof(struct threads_task *), INFINITE ); }
	inline static void threads_wakefortask(){ WakeByAddressSingle( &threads_queue_body ); }
	// NOTE: WaitOnAddress returns without being signalled very often

	void threads_submit( void *function, void *arg, int argsize ){

		// put
		// overflowed if threads_queue_head->status != 0. usage = (head > tail) ? (head-tail)/20MB : (tail-head)/20MB		
		threads_queue_lock();
		memcpy( &(threads_queue_head->arg), arg, argsize );
		threads_queue_head->argsize = argsize;
		threads_queue_head->function = function;
		threads_queue_head->next = (void*)threads_queue_head +sizeof(struct threads_task) -sizeof(int) +argsize;
		if( ( (void*)(threads_queue_head->next) - (void*)( threads_queue ) ) > THREADS_QUEUE_SIZE * 1024 * 1024 )
			threads_queue_head->next = threads_queue;	
		threads_queue_head->status = 2;  // queued
		
		if( !threads_queue_body )
			threads_queue_body = threads_queue_head;

		threads_queue_empty = 0;
		threads_queue_head = threads_queue_head->next;
		threads_queue_unlock();

		threads_wakefortask();		
	}

	void threads_main( volatile struct threads_thread *self ){
		struct threads_task *T, *t;
		while( 1 ){
			
			if( self->status == 3 ){
				self->status = -1;
				break;
				}

			self->status = 0;  // waiting
			threads_waitfortask(); // may be fake alarm
			
			// get
			threads_queue_lock();
			if( !threads_queue_body ) { threads_queue_unlock(); continue; }  // fake alarm
			T = threads_queue_body;
			T->status = 1;  // taken
			threads_queue_body = T->next->status == 2 ? T->next : 0;
			if( !threads_queue_tail )
				threads_queue_tail = T;
			threads_queue_unlock();
			
			// call
			self->status = 1;  // working
			T->function( &(T->arg) );
			
			// done/remove
			threads_queue_lock();
			if( T == threads_queue_tail ){
				t = T;
				while( t = t->next ){

					if( t->status == 1 ){
						threads_queue_tail = t;
						break; }

					if( t->status == 2 ){
						threads_queue_tail = 0;
						break; }
						
					if(  t->status == 0 ){
						threads_queue_tail = 0;
						threads_queue_empty = 1;
						threads_wakeforend();
						break; }
				}
			}
			memset( T, 0, sizeof(struct threads_task) -sizeof(int) +T->argsize );
			threads_queue_unlock();
		}
	}

	void threads_wait(){
		while(1){
			threads_waitforend(); // may be fake alarm
			if( threads_queue_empty ) // indeed empty
				return;
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
		threads_queue_empty = 1;
		threads_queue_head = threads_queue;
		threads_queue_body = 0;
		threads_queue_tail = 0;
		threads_queue_mutex = CreateMutex( 0, 0, 0 );
		threads_count = 0;
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
