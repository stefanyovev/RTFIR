

    #define title "RTFIR"

    #include <stdio.h>
    #include <stdlib.h>
    #include <string.h>
    #include <math.h>
    #include <windows.h>
    #include <portaudio.h>

    void PRINT( char *format, ... );
    
    // ############################################################################################################ //

    void PaUtil_InitializeClock( void );
    double PaUtil_GetTime( void );
    double T0;
    
    #define SAMPLERATE 48000 // [samples/second]
    #define SAMPLESIZE sizeof(float)

    #define NOW ((long)(ceil((PaUtil_GetTime()-T0)*SAMPLERATE))) // [samples]   
     
    #define POINTSMAX 1000  // should be even
    #define POINTSMIN 20
    
    #define DIFFSMAX 100
    
    #define MSIZE 250000 // keep one second


    struct graph {
        POINT points[POINTSMAX];
        int full;
        long cursor;
        int min_i, max_i; };

    struct port {
        int channels_count;
        PaStream *stream;
        long t0, len;        
        struct graph graph; }

    INPORT, OUTPORT;

    long cursor;
    
    struct diff {
        int mag;
        long count;
    }
    
    diffs[DIFFSMAX];
    
    int diffs_i;
    int diffs_full;


    float *canvas;
    
    struct out {
        int src_chan;
        float *k;
        int kn;
        float *outp;
        int frameCount;
    }
    
    *map;

    int shutdown;

    // ####################### FILTERS ###############################################################################
    // ####################### FILTERS ###############################################################################
    // ####################### FILTERS ###############################################################################
    
    struct filter {
        char *name;
        float* k;
        int kn;
    }
    
    *filters[11]; // null terminated list of pointers
    
    void load_filters(){
        // TODO: mallocs without if check null
        memset( filters, 0, sizeof(struct filter *) );
        filters[0] = malloc( sizeof( struct filter ) );
        filters[0]->name = "None";
        filters[0]->k = malloc( SAMPLESIZE * 1  );
        filters[0]->k[0] = 1.0;
        filters[0]->kn = 1;
        WIN32_FIND_DATA r;
        HANDLE h = FindFirstFile( "filters\\*", &r );
        if( h == INVALID_HANDLE_VALUE )
            return;
        int i = 1;
        do if( !(r.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && r.cFileName[0] != '.' ){
            char fname[100] = "";
            sprintf( fname, "filters\\%s", r.cFileName );
            
            FILE *f;
            f = fopen( fname, "r" );
            if( !f )
                continue;
                
            int count = 0;
            float num;
            while( fscanf( f, "%f", &num ) == 1 )
                count ++;
                
            if( count == 0 )
                continue;

            fseek( f, 0, SEEK_SET );
            
            float *data = malloc( SAMPLESIZE * count  );
            int j=0 ;
            while( fscanf( f, "%f", data + (j++) ) == 1 );
            
            PRINT( "loaded %s %d taps \n", r.cFileName, count );
            
            fclose(f);
            
            // push to list
            filters[i] = malloc( sizeof( struct filter ) );
            filters[i]->name = malloc( strlen( fname )+1 );
            strcpy( filters[i]->name, r.cFileName );
            filters[i]->k = data;
            filters[i]->kn = count;
            
            i++;
        } while( FindNextFile( h, &r ) );
        FindClose(h);
        PRINT( "loaded %d filters \n", i-1 );
    }

    struct filter * get_filter( char *name ){
        for( int i=0; filters[i]; i++ ){
            if( strcmp( filters[i]->name, name ) == 0 )
                return filters[i];
        }
        PRINT( "TATOAL FAIKL \n" );
        return filters[0];
    }
    
    void worker( struct out *o ){
        while( 1 ){
            while( !(o->outp) )
                if( shutdown )
                    return;
            int ofs = cursor % MSIZE;
            float *sig = canvas + o->src_chan*MSIZE*4 +MSIZE +ofs;
            for( int n=0; n < o->frameCount; n++ )
                for( int kn=0; kn < o->kn; kn++ )
                    o->outp[n] += o->k[kn]*sig[n-kn];
            o->outp = 0;
        }
    }
    
    // ####################### FILTERS ###############################################################################
    // ####################### FILTERS ###############################################################################
    // ####################### FILTERS ###############################################################################


    void aftermath( int sel, long t, int avail_after, int frameCount ){
        char *portname = ( sel == 0 ? "input" : "output" );
        struct graph *g = ( sel == 0 ? &(INPORT.graph) : &(OUTPORT.graph) );
        
        // make a stat
        g->points[g->cursor].x = t;
        g->points[g->cursor].y = avail_after -frameCount; // avail before insert
        g->cursor++;
        
        g->points[g->cursor].x = t;
        g->points[g->cursor].y = avail_after;
        g->cursor++;
        
        // graph end ?
        if( g->cursor == POINTSMAX ){
            g->cursor = 0;
            if( g->full == 0 )
                g->full = 1;
        }

        // find min/max (every time for constant cpu usage and ease)
        int pi = 0;
        int gi = ( g->full ? g->cursor : 0 );
        int count = ( g->full ? POINTSMAX : g->cursor );
        int min = 999999;
        int max = -999999;
        for( int i=0; i<count; i++ ){
            if( g->points[gi].y < min ){
                min = g->points[gi].y;
                g->min_i = gi;
            }
            if( g->points[gi].y > max ){
                max = g->points[gi].y;
                g->max_i = gi;
            }
            gi ++;
            gi %= POINTSMAX;
        }        

        // cursor ?
        if( (INPORT.graph.full || INPORT.graph.cursor > POINTSMIN) && (OUTPORT.graph.full || OUTPORT.graph.cursor > POINTSMIN) ){
        
            long new_cursor_candidate = 
                OUTPORT.t0 -INPORT.t0 // output.t0 as input lane address
                +OUTPORT.len // where is output end on the input
                +INPORT.graph.points[INPORT.graph.min_i].y -OUTPORT.graph.points[OUTPORT.graph.max_i].y; // our latency
                
            if( new_cursor_candidate >= 0 ){
            
                if( cursor == -1 ){
                    cursor = new_cursor_candidate;
                    PRINT( "curosr initialized (%d) \n", cursor );
                    
                } else if( cursor - new_cursor_candidate != 0 ){
                    // push a diff to think about
                    long diff = cursor - new_cursor_candidate;
                    if( diff == diffs[diffs_i].mag ){
                        diffs[diffs_i].count ++;
                    } else {
                        diffs_i ++;
                        if( diffs_i == DIFFSMAX ){
                            diffs_i = 0;
                            diffs_full = 1;
                        }
                        diffs[diffs_i].mag = diff;
                        diffs[diffs_i].count = 1;
                    }
                }
            }
        }
    }

    void correct_cursor_if_necessary(){
        if( diffs_full || diffs_i > 0 ){
            long diffs_sum = 0;
            long diffs_total = 0;
            int di = ( diffs_full ? diffs_i : 0 );
            int count = ( diffs_full ? DIFFSMAX : diffs_i );
            for( int i=0; i<count; i++ ){
                diffs_sum += diffs[di].mag * diffs[di].count;
                diffs_total += diffs[di].count;
                di ++; di %= DIFFSMAX;
            }
            if( diffs_sum != 0 ){ // may cancel each other
                double diffs_avg = ((double)diffs_sum)/((double)diffs_total);
                if( diffs_avg > 0 )
                    diffs_sum = (long)( floor( diffs_avg ) );
                else
                    diffs_sum = (long)( ceil( diffs_avg ) );
                if( diffs_sum != 0 ){ // may round to zero
                    cursor -= diffs_sum;
                    PRINT( "correction of %d samples \n", diffs_sum );
                    diffs_full = diffs_i = 0;
                }
            }
        }
    }

    PaStreamCallbackResult device_tick(                            // RECEIVE
        float **input,
        float **output,
        unsigned long frameCount,
        const PaStreamCallbackTimeInfo *timeInfo,
        PaStreamCallbackFlags statusFlags,
        void *userdata ){
        
        long now;

        if( statusFlags ){
            if( paInputUnderflow & statusFlags ) PRINT( "Input Underflow \n" );
            if( paInputOverflow & statusFlags ) PRINT( "Input Overflow \n" );
            if( paOutputUnderflow & statusFlags ) PRINT( "Output Underflow \n" );
            if( paOutputOverflow & statusFlags ) PRINT( "Output Overflow \n" );
            if( paPrimingOutput & statusFlags ) PRINT( "Priming Output \n" );}
        
        if( input && output )
            PRINT( "strange \n" );

        if( input ){ // PRINT("i");
        
            // write
            int ofs = INPORT.len % MSIZE;
            for( int i=0; i<INPORT.channels_count; i++ )
                for( int j=0; j<3; j++ )
                    memcpy( canvas +i*MSIZE*4 +j*MSIZE +ofs, input[i], frameCount*SAMPLESIZE );
        
            // stamp
            now = NOW;
            
            if( INPORT.t0 == -1 )
                INPORT.t0 = now;
                
            // commit
            INPORT.len += frameCount;
            
            // log
            aftermath( 0, now, INPORT.t0 + INPORT.len -now, frameCount );
        }
        
        if( output ){ // PRINT("o");

            if( cursor > -1 ){
                // copy 
                for( int i=0; i<OUTPORT.channels_count; i++ ){
                    memset( output[i], 0, frameCount*SAMPLESIZE );
                    if( map[i].src_chan == -1 )
                        continue;
                    map[i].frameCount = frameCount;
                    map[i].outp = output[i];
                }

                int done = 0;
                while( !done ){
                    done = 1;
                    for( int i=0; i<OUTPORT.channels_count; i++ ){
                        if( map[i].src_chan == -1 )
                            continue;
                        if( map[i].outp ){
                            done = 0;
                            break;
                        }
                    }
                }

                if( cursor + frameCount > INPORT.len )
                    PRINT( "glitch %d \n", cursor -INPORT.len );
                cursor += frameCount;
            }

            // stamp
            now = NOW;
            
            if( OUTPORT.t0 == -1 )
                OUTPORT.t0 = now;

            // commit
            OUTPORT.len += frameCount;

            // log
            aftermath( 1, now, OUTPORT.t0 + OUTPORT.len -now, frameCount );
        }

        return paContinue; }


    int start( int input_device_id, int output_device_id ){
        for( int i=1; i>-1; i-- ){
            PRINT( "starting %s %d ... \n", ( i ? "input" : "output" ), ( i ? input_device_id : output_device_id ) );
        
            int device_id = ( i ? input_device_id : output_device_id );
            const PaDeviceInfo *device_info = Pa_GetDeviceInfo( device_id );
            static PaStreamParameters params;
            PaStream **stream = ( i ? &(INPORT.stream) : &(OUTPORT.stream) );
            
            params.device = device_id;
            params.sampleFormat = paFloat32|paNonInterleaved;
            params.hostApiSpecificStreamInfo = 0;            
            params.suggestedLatency = ( i ? device_info->defaultLowInputLatency : device_info->defaultLowOutputLatency ); // "buffer size" [seconds]
            params.channelCount = ( i ? device_info->maxInputChannels : device_info->maxOutputChannels );
            
            PaError err = Pa_OpenStream(
                stream,
                i ? &params : 0,
                i ? 0 : &params,
                SAMPLERATE,
                paFramesPerBufferUnspecified,
                paClipOff | paDitherOff | paPrimeOutputBuffersUsingStreamCallback, // paNeverDropInput is for full-duplex only
                &device_tick,
                0 );

            if( err != paNoError ){
                if( err != paUnanticipatedHostError ) {
                    PRINT( "ERROR 1: %s \n", Pa_GetErrorText( err ) );
                    return 0; }
                else {
                    const PaHostErrorInfo* herr = Pa_GetLastHostErrorInfo();
                    PRINT( "ERROR 2: %s \n", herr->errorText );
                    return 0; }}
            
            if( i ){
                INPORT.channels_count = device_info->maxInputChannels;
                PRINT( "%d channels \n", INPORT.channels_count );
                canvas = malloc( INPORT.channels_count * MSIZE*4 * SAMPLESIZE );
                if( !canvas )
                    PRINT( "ERROR: could not allocate memory" );
            } else {
                OUTPORT.channels_count = device_info->maxOutputChannels;
                PRINT( "%d channels \n", OUTPORT.channels_count );
                map = malloc( OUTPORT.channels_count * sizeof(struct out) );
                if( !map )
                    PRINT( "ERROR: could not allocate memory" );
                for( int i=0; i<OUTPORT.channels_count; i++ ){
                    map[i].src_chan = -1;
                    map[i].k = filters[0]->k;
                    map[i].kn = filters[0]->kn;
                    map[i].outp = 0;
                    CreateThread( 0, 0, &worker, map+i, 0, 0 );
                }
            }

            err = Pa_StartStream( *stream );
            if( err != paNoError ){
                PRINT( "ERROR 3: %s \n", Pa_GetErrorText( err ) );
                return 0; }

            PaStreamInfo *stream_info = Pa_GetStreamInfo( *stream );            
            PRINT( "%d / %d \n",
                (int)round(stream_info->sampleRate),
                (int)round( i ? (stream_info->inputLatency)*SAMPLERATE : (stream_info->outputLatency)*SAMPLERATE ) );
        }

        // done
        PRINT( "both started. \n" );
        return 1; }



    // ############################################################################################################ //


    const int width = 600;
    const int height = 700;
    const int WW = 574, HH = 200;

    HWND hwnd;
    HDC hdc;
    
    RECT rc;
    MSG msg;
    BOOL done = FALSE;


    #define CMB1 (555)
    #define CMB2 (556)
    #define BTN1 (123)
    
    #define LB1 (110)
    #define CB1 (220)
    #define CB2 (330)
    
    #define EDT (400)

    HWND hCombo1, hCombo2, hBtn;
    HWND cbs[10];
    HWND cbs2[10];
    HWND hEdit;

    void PRINT( char *format, ... ){
    
        char str[1000] = "";
        va_list( args );
        va_start( args, format );
        vsprintf( str, format, args );
        va_end( args );
        if( str[0] == 0 )
            return;

        int index = GetWindowTextLength( hEdit );
        // SetFocus (hEdit); // set focus
        SendMessageA( hEdit, EM_SETSEL, (WPARAM)index, (LPARAM)index ); // set selection - end of text
        SendMessageA( hEdit, EM_REPLACESEL, 0, (LPARAM)str ); // append!
    }

    LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ){
        if( msg == WM_COMMAND ){
            if( LOWORD(wParam) == BTN1 ){

                int sd, dd;
                char txt[300];

                GetDlgItemText( hwnd, CMB1, txt, 255 );
                sscanf( txt, "  %3d", &sd );

                GetDlgItemText( hwnd, CMB2, txt, 255 );
                sscanf( txt, "  %3d", &dd );

                if( start( sd, dd ) ){
                    for( int i=0; i<OUTPORT.channels_count; i++ )
                        map[i].src_chan = i % 2; // LR LR LR ..
                    EnableWindow( hCombo1, 0 );
                    EnableWindow( hCombo2, 0 );
                    EnableWindow( hBtn, 0 );
                    for( int i=0; i<OUTPORT.channels_count; i++ ){
                        if( i < 10 ){
                            EnableWindow( cbs[i], 1 );
                            for( int j=0; j<INPORT.channels_count; j++ ){
                                char str[3] = "nn";
                                if( j+1 != 10 ) { str[0] = 48+j+1; str[1] = 0; }
                                else { str[0] = 49; str[1] = 48; str[2] = 0; }
                                SendMessage( cbs[i], CB_ADDSTRING, 0, str );
                            }
                            SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)map[i].src_chan+1, (LPARAM)0 );
                            EnableWindow( cbs2[i], 1 );
                        }
                    }
                }
            } else if( (LOWORD(wParam) >= CB1) && LOWORD(wParam) <= CB1+10 && CBN_SELCHANGE == HIWORD(wParam) ){
            
                int out = LOWORD(wParam) - CB1;
                
                char  txt[20];
                GetDlgItemText( hwnd, CB1+out, txt, 20 );
                
                int chan = txt[0]-48-1;
                if( chan < 10 ) {
                    map[out].src_chan = chan;
                    PRINT( "mapped out %d to in %d\n", out+1, chan+1 ); }
                else {
                    map[out].src_chan = -1;
                    PRINT( "muted out %d \n", out+1 ); }
                
                
            } else if( (LOWORD(wParam) >= CB2) && LOWORD(wParam) <= CB2+10 && CBN_SELCHANGE == HIWORD(wParam) ){
                
                int out = LOWORD(wParam) - CB2;
                
                char  txt[100];
                GetDlgItemText( hwnd, CB2+out, txt, 100 );

                PRINT( "mapped out %d to filter %s \n", out+1, txt );
                
                struct filter *fl= get_filter( txt );
                map[out].k = fl->k;
                map[out].kn = fl->kn;
            }
         }

        else if( msg == WM_CLOSE )
            DestroyWindow( hwnd );
        else if( msg == WM_DESTROY ){
            shutdown = 1;
            PostQuitMessage( 0 );
        }
        return DefWindowProc( hwnd, msg, wParam, lParam ); }


    int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow ){                                    // MAIN2

        // SetProcessDPIAware();
        // UI
        WNDCLASSEX wc;
        memset( &wc, 0, sizeof(wc) );
        wc.cbSize = sizeof(wc);
        wc.hInstance = hInstance;
        wc.lpfnWndProc = WndProc;
        wc.lpszClassName = "mainwindow";
        wc.hbrBackground = COLOR_WINDOW; //CreateSolidBrush( RGB(64, 64, 64) );
        wc.hCursor = LoadCursor( 0, IDC_ARROW );

        if( !RegisterClassEx(&wc) ){
            MessageBox( 0, "Failed to register window class.", "Error", MB_OK );
            return 0; }

        hwnd = CreateWindowEx( WS_EX_APPWINDOW, "mainwindow", title, WS_MINIMIZEBOX | WS_SYSMENU | WS_POPUP | WS_CAPTION, 300, 200, width, height, 0, 0, hInstance, 0 );
        hdc = GetDC( hwnd );

        hCombo1 = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST, 10, 10, 490, 8000, hwnd, CMB1, NULL, NULL);
        hCombo2 = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST, 10, 40, 490, 8000, hwnd, CMB2, NULL, NULL);
        hBtn = CreateWindowEx( 0, "Button", "Play >", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_DEFPUSHBUTTON, 507, 10, 77, 53, hwnd, BTN1, NULL, NULL);
        hEdit = CreateWindowEx( 0, "EDIT", 0, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 10, 460, WW, HH, hwnd, EDT, NULL, NULL);

        // init core
        if( Pa_Initialize() )
            PRINT( "ERROR: Could not initialize PortAudio. \n" );
        if( Pa_GetDeviceCount() <= 0 )
            PRINT( "ERROR: No Devices Found. \n" );
            
        // PRINT( "%s\n\n", Pa_GetVersionInfo()->versionText );            
        PRINT( "SAMPLERATE %d \n", SAMPLERATE );
        
        PaUtil_InitializeClock();
        T0 = PaUtil_GetTime();

        // globals
        struct port* ports [2] = { &INPORT, &OUTPORT };
        for( int i=0; i<2; i++ ){
            struct port *p = ports[i];
            memset( p, 0, sizeof(struct port) );
            memset( p, 0, sizeof(struct port) );
            p->t0 = -1; // invalid
            p->len = 0;
            p->graph.cursor = 0;
            p->graph.full = 0;
            p->graph.min_i = -1; // invalid
            p->graph.max_i = -1; // invalid
        }
        cursor = -1; // invalid
        diffs_full = 0;
        diffs_i = 0;
        shutdown = 0;

        // load filters
        load_filters();

        // add map labels and dropdowns
        for( int i=0; i<10; i++ ){
        
            char str[7] = "out nn";
            if( i+1 != 10 ) { str[4] = 48+i+1; str[5] = 0; }
            else { str[4] = 49; str[5] = 48; str[6] = 0;}
            CreateWindowEx( 0, "static", str, WS_VISIBLE|WS_CHILD, 50, 105+i*30, 50, 80, hwnd, LB1+i, NULL, NULL);
        
            cbs[i] = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST, 100, 100+i*30, 90, 800, hwnd, CB1+i, NULL, NULL);
            SendMessage( cbs[i], CB_ADDSTRING, 0, "None");
            SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
            EnableWindow( cbs[i], 0 );
            
            CreateWindowEx( 0, "static", "filter", WS_VISIBLE|WS_CHILD, 250, 105+i*30, 50, 80, hwnd, LB1+10+i, NULL, NULL);            
        
            cbs2[i] = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST, 300, 100+i*30, 130, 800, hwnd, CB2+i, NULL, NULL);            
            int j=0;
            while( filters[j] )
                SendMessage( cbs2[i], CB_ADDSTRING, 0, filters[j++]->name );
            SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
            EnableWindow( cbs2[i], 0 );

        }

        // SHOW
        ShowWindow( hwnd, SW_SHOW );

        // populate device dropdowns
        char str[1000], txt[100000];
        for( int i=0; i<Pa_GetDeviceCount(); i++ ){
            PaDeviceInfo *info = Pa_GetDeviceInfo(i);
            strcpy( str, Pa_GetHostApiInfo( info->hostApi )->name );
            sprintf( txt, " %3d  /  %s  /  %s ", i, strstr( str, "Windows" ) ? str+8 : str, info->name );
            if( info->maxInputChannels ) SendMessage( hCombo1, CB_ADDSTRING, 0, txt );
            if( info->maxOutputChannels ) SendMessage( hCombo2, CB_ADDSTRING, 0, txt );
            SendMessage( hCombo1, CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
            SendMessage( hCombo2, CB_SETCURSEL, (WPARAM)0, (LPARAM)0 ); }

        // loop
        double last_correction_time = PaUtil_GetTime(); // queryperformancecounter de-facto
        // TODO: use NOW; get once in the loop; give it to draw
        while( !done ){
            if( PeekMessage( &msg, NULL, 0, 0, PM_REMOVE ) ){
                if( msg.message == WM_QUIT )
                    done = TRUE;
                else {
                    TranslateMessage( &msg );
                    DispatchMessage( &msg ); }}
            else if( PaUtil_GetTime() - last_correction_time > 1.0 ){ // once per second
                correct_cursor_if_necessary();
                last_correction_time = PaUtil_GetTime();
                if( cursor > 0 )
                    PRINT( "load %d%% \n", (int)ceil((Pa_GetStreamCpuLoad(INPORT.stream)+Pa_GetStreamCpuLoad(OUTPORT.stream))*100.0) );
            }
        }
        
        return 0; }
