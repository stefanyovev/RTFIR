
    char * conf_filename = "";

    struct conf_item {
        char * key;
        char * value;
        struct conf_item *next; }

    *conf = 0;

    struct conf_item * conf_p( char *key ){
        struct conf_item *p = conf;
        while( p ){
            if( strcmp( p->key, key ) == 0 )
                return p;
            p = p->next; }
        return 0; }

    char * conf_get( char *key ){
        struct conf_item *p = conf_p( key );
        if( p ) return p->value;
        return 0; }

    struct conf_item * conf_new_item( char * key, char * value ){
        struct conf_item *p = malloc( sizeof( struct conf_item ) );
        p->key = malloc( strlen( key ) +1 );
        p->value = malloc( strlen( value ) +1 );
        p->next = 0;
        strcpy( p->key, key );
        strcpy( p->value, value );    
        return p; }

    void conf_set( char * key, char * value ){
        if( !conf ){ conf = conf_new_item( key, value ); return; }
        struct conf_item *q;
        if( q = conf_p( key ) ){
            free( q->value );
            q->value = malloc( strlen( value ) +1 );
            strcpy( q->value, value );
            return; }
        q = conf;
        while( q->next ) q = q->next;
        q->next = conf_new_item( key, value ); }

    void conf_load( char* filename ){
        conf_filename = filename;
        FILE *f = fopen( filename, "r" );
        if( !f ) return;        
        char k[200] = "", v[200] = "";
        int ki=0, vi=0, into=0;
        char c = 0;
        while ( fread( &c, 1, 1, f ) ){
            if( c == '\r' ) continue;
            if( c == '=' && into == 0 && ki ){
                k[ki] = 0;
                into = 1;
                continue; }
            if( c == '\n' ){
                v[vi] = 0;
                if( ki ) conf_set( k, v );
                ki = vi = into = 0;
                continue; }
            if( into == 0 ) k[ki++] = c;
            else v[vi++] = c; }
        if( ki && vi ) {
            v[vi] = 0;
            conf_set( k, v ); }
        fclose( f ); }

    void conf_save(){
        FILE *f = fopen( conf_filename, "w" );
        if( !f ) return;
        struct conf_item *p = conf;
        while( p ){
            fprintf( f, "%s=%s\n", p->key, p->value );
            p = p->next; }
        fclose( f ); }
