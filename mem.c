


	// Memory Allocation 
	// single free for normal and aligned pointers

	// TODO: the list only grows

	#if __INCLUDE_LEVEL__ == 0
		#include <stdio.h>
		#define print printf
		void error( char *msg ){
			print( "error: %s", msg );
			exit(1); }
	#endif


	struct mem_item {
		void *a;  // returned to user
		void *b;  // returned by the os
		};

	struct mem_item* mem_list;  // all allocations
	int              mem_list_len;


	#define MEM_LIST_PAGE 100   // entries

	void mem_list_init(){
		static int done = 0;
		if( !done ){
			mem_list = malloc( MEM_LIST_PAGE * sizeof(struct mem_item) );
			if( !mem_list ) error( "Out of memory." );
			memset( mem_list, 0, MEM_LIST_PAGE * sizeof(struct mem_item) );
			mem_list_len = 0;
			done = 1; } }

	void mem_list_add( void *a, void *b ){
		mem_list[mem_list_len].a = a;
		mem_list[mem_list_len].b = b;
		mem_list_len ++;
		if( mem_list_len % MEM_LIST_PAGE == 0 ){
			void *p = malloc( (mem_list_len+MEM_LIST_PAGE) * sizeof(struct mem_item) );
			if( !p ) error( "Out of memory" );
			memset( p, 0, (mem_list_len+MEM_LIST_PAGE) * sizeof(struct mem_item) );
			memcpy( p, mem_list, mem_list_len * sizeof(struct mem_item) );
			free( mem_list );
			mem_list = p; } }

	#undef MEM_LIST_PAGE
	
	
	struct mem_item* mem_list_find( void *a ){
		for( int i=0; i<mem_list_len; i++ )
			if( mem_list[i].a == a )
				return &mem_list[i];
		return 0; }

	int mem_list_remove( void *a ){
		memset( mem_list_find(a), 0, sizeof(struct mem_item) ); }


	void* mem( uint64_t count ){
		mem_list_init();
		void *p = malloc( count );
		if( !p ) error( "Out of memory." );
		mem_list_add( p, p );
		memset( p, 0, count );
		return p; }

	void* mem_aligned( uint64_t bytes, uint64_t alignment ) {
		mem_list_init();
		void *p1, *p2; p1 = mem( bytes + alignment + sizeof(size_t) );
		size_t addr = (size_t)p1 +alignment +sizeof(size_t);
		p2 = (void *)(addr -(addr%alignment));
		*((size_t *)p2-1) = (size_t)p1;
		mem_list_add( p2, p1 );
		return p2; }

	void mem_free( void *a ){
		if( !a ) error( "Freeing null pointer." );
		struct mem_item *p = mem_list_find( a );
		if( !p ) error( "Freeing unallocated pointer." );
		free( p->b );
		mem_list_remove( a ); }



	#if __INCLUDE_LEVEL__ == 0
	
		int main( int argc, char **argv ){

		}
	
	#endif