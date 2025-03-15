


	// ############################################################################################################ //
	// ########################################## GUI ############################################################# //

	#include "conf.c"

	#define R1 (1)
	#define R2 (2)
	#define R3 (3)
	#define R4 (4)
	#define LL (5)
	#define BTN01 (121)
	#define BTN02 (122)
	#define CMB1 (555)
	#define CMB2 (556)
	#define BTN1 (123)
	#define BTN4 (126)
	#define LB1 (110)
	#define CB1 (220)
	#define CB2 (330)
	#define CB3 (440)
	#define EDT (400)

	HWND hMain; // handle main window
	MSG msg;
	
	HWND hr1, hr2, hr3, hr4, hLL, hbctl1, hbctl2, hCombo1, hCombo2, hBtn, hBtn4;
	HWND cbs[10];
	HWND cbs2[10];
	HWND cbs3[10];
	HWND hEdit;

	HFONT hfont1, hfont2, hfont3, hfont4;
	LOGFONT font1 = {0}, font2 = {0}, font3 = {0}, font4 = {0};


	// ------------------------------------------------------------------------------------------------------------ //
	// ------------ names ----------------------------------------------------------------------------------------- //

	char * names = 0;
	char * device_name( int device_id ){
		if( !names ){ 
			names = mem( Pa_GetDeviceCount() * 200 );
			char drv_name[50] = "", dev_name[150] = "";
			for( int i=0; i<Pa_GetDeviceCount(); i++ ){
				strcpy( dev_name, Pa_GetDeviceInfo( i )->name );
				strcpy( drv_name, Pa_GetHostApiInfo( Pa_GetDeviceInfo( i )->hostApi )->name );
				sprintf( names +i*200, "  %s  /  %s  ", strstr( drv_name, "Windows" ) ? drv_name+8 : drv_name, dev_name ); } }
		return names +device_id*200; }

	int device_id( char * device_name ){
		for( int i=0; i<Pa_GetDeviceCount(); i++ )
			if( strcmp( names +i*200, device_name ) == 0 )
				return i;
		return -1; }

	// ------------------------------------------------------------------------------------------------------------ //
	// -------------- filter -------------------------------------------------------------------------------------- //

	struct filter {
		char *name;
		int samplerate;
		float* k;
		int kn; 
	};
	
	struct filter ** filters = 0; // null terminated list of pointers
	
	struct filter * filter_p( char * name ){
		for( int i=0; filters[i]; i++ )
			if( strcmp( filters[i]->name, name ) == 0 )
				return filters[i];
		return 0; }

	int filter_i( char * name ){
		for( int i=0; filters[i]; i++ )
			if( strcmp( filters[i]->name, name ) == 0 )
				return i;
		return -1; }

	void load_filters( int samplerate ){
		if( filters ){
			for( int i=0; filters[i]; i++ ){
				mem_free( filters[i]->name );
				mem_free( filters[i]->k ); }
			mem_free( filters ); filters = 0; }
		filters = mem( sizeof(struct filter *) * 100 );
		// add 'None' - unity kernel
		filters[0] = mem( sizeof( struct filter ) );
		filters[0]->name = mem( 5 ); strcpy( filters[0]->name, "None" );
		filters[0]->k = mem( sizeof(float) * 1 );
		filters[0]->k[0] = 1.0;
		filters[0]->kn = 1;
		// add files
		char folder[30] = ""; sprintf( folder, "filters\\%d\\*", samplerate );
		WIN32_FIND_DATA r;
		HANDLE h = FindFirstFile( folder, &r );
		if( h == INVALID_HANDLE_VALUE ){
			print( "Folder filters\\%d does not exist. \r\n", samplerate );
			return;
			}
		int i = 1;
		do if( !(r.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) && r.cFileName[0] != '.' ){
			char fname[100] = ""; sprintf( fname, "filters\\%d\\%s", samplerate, r.cFileName );
			FILE *f = fopen( fname, "r" );
			if( !f ) continue;
			int count = 0;
			float num; while( fscanf( f, "%f", &num ) == 1 ) count ++;
			if( count == 0 ) continue;
			if( count > samplerate ){
				print( "NOT loaded %s exceeds %d taps \r\n", r.cFileName, samplerate );
				fclose(f);
				continue; }
			fseek( f, 0, SEEK_SET );
			float *data = mem( sizeof(float) * count );
			for( int j=0; j<count; j++ )
				fscanf( f, "%f", data +j );
			fclose(f);
			// push to list
			filters[i] = mem( sizeof( struct filter ) );
			filters[i]->name = mem( strlen( fname )+1 );
			strcpy( filters[i]->name, r.cFileName );
			filters[i]->k = data;
			filters[i]->kn = count;
			i++;
			print( "Loaded %s - %d taps \r\n", r.cFileName, count );
		} while( FindNextFile( h, &r ) );
		FindClose(h);
		if( i == 1 )
			print( "No filters found in folder filters\\%d. \r\n", samplerate );
		else
			print( "Loaded %d filters \r\n", i-1 );
	}

	// ------------------------------------------------------------------------------------------------------------ //
	// ----------- conf ------------------------------------------------------------------------------------------- //

	int devices_as_in_conf(){ // whether the two devices selected are as in the conf
		char a[250], b[250];
		if( !conf_get( "device1" ) || !conf_get( "device2" ) ) return 0;
		GetDlgItemText( hMain, CMB1, a, 250);
		GetDlgItemText( hMain, CMB2, b, 250);
		return strcmp( a, conf_get( "device1" ) ) == 0 && strcmp( b, conf_get( "device2" ) ) == 0; }

	int samplerate_as_in_conf(){ // whether the selected samplerate is as in the conf
		if( !conf_get( "samplerate" ) ) return 0;
		int samplerate_from_conf = -1;
		return sscanf( conf_get( "samplerate" ), "%d", &samplerate_from_conf ) == 1 && samplerate_from_conf == samplerate; }

	void show_conf(){ // sets the bottom combos as in conf or clear them; routing if devices are same and filters if devs&SR are same

		// fill left combos
		char txt1[250], txt2[250];
		GetDlgItemText( hMain, CMB1, txt1, 250 );
		GetDlgItemText( hMain, CMB2, txt2, 250 );
		for( int i=0; i<Pa_GetDeviceInfo( device_id( txt2 ) )->maxOutputChannels; i++ ){
			SendMessage( cbs[i], CB_RESETCONTENT, (WPARAM)0, (LPARAM)0 );
			SendMessage( cbs[i], CB_ADDSTRING, (WPARAM)0, (LPARAM)"" );
			SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
			for( int j=0; j<Pa_GetDeviceInfo( device_id( txt1 ) )->maxInputChannels; j++ ){
				char n[3]; sprintf( n, "%d", j+1 );
				SendMessage( cbs[i], CB_ADDSTRING, (WPARAM)0, (LPARAM)n ); } }

		// fill right combos
		for( int i=0; i<10; i++ ){
			SendMessage( cbs2[i], CB_RESETCONTENT, (WPARAM)0, (LPARAM)0 );
			SendMessage( cbs2[i], CB_ADDSTRING, (WPARAM)0, (LPARAM)"" );
			for( int j=0; filters[j]; j++ ) SendMessage( cbs2[i], CB_ADDSTRING, 0, filters[j]->name );
			SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 ); }
			
		if( !devices_as_in_conf() ){ // clear selections
			for( int i=0; i<10; i++ ){
				SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
				SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 ); }
			return;
			}

		char key[20]; int val;
		for( int i=0; i<10; i++ ){

			sprintf( key, "out%d", i+1 );
			if( conf_get( key ) && sscanf( conf_get( key ), "%d", &val ) == 1 )
				SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)val, (LPARAM)0 );
			else SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );

			sprintf( key, "filter%d", i+1 );
			if( samplerate_as_in_conf() && conf_get( key ) && filter_p( conf_get( key ) ) )
				SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)(filter_i( conf_get( key ) )+1), (LPARAM)0 );
			else SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)0, (LPARAM)0 ); 

			sprintf( key, "delay%d", i+1 );
			if( samplerate_as_in_conf() && conf_get( key ) && sscanf( conf_get( key ), "%d", &val ) == 1 && val > -1 && val <= samplerate )
				SendMessage( cbs3[i], WM_SETTEXT, (WPARAM)0, conf_get( key ) );
			//else
				//SendMessage( cbs3[i], WM_SETTEXT, (WPARAM)0, "0" );

			} }

	void save_conf(){
		char txt[250], key[20];
		sprintf( txt, "%d", samplerate );
		conf_set( "samplerate", txt );
		GetDlgItemText( hMain, CMB1, txt, 250 );
		conf_set( "device1", txt );
		GetDlgItemText( hMain, CMB2, txt, 250 );
		conf_set( "device2", txt );
		for( int i=0; i<10; i++ ){
			sprintf( key, "out%d", i+1 );
			GetDlgItemText( hMain, CB1+i, txt, 250 );
			conf_set( key, txt );
			sprintf( key, "filter%d", i+1 );
			GetDlgItemText( hMain, CB2+i, txt, 250 );
			conf_set( key, txt );
			sprintf( key, "delay%d", i+1 );
			GetDlgItemText( hMain, CB3+i, txt, 250 );
			conf_set( key, txt );
			}
		conf_save(); }

	// -------------------------------------------------------------------------------------------------------------- //
	// ------------------- get_cpu_cores ---------------------------------------------------------------------------- //

	int get_cpu_cores(){
		typedef BOOL (WINAPI *LPFN_GLPI)( PSYSTEM_LOGICAL_PROCESSOR_INFORMATION, PDWORD);
		LPFN_GLPI glpi = GetProcAddress( GetModuleHandle("kernel32"), "GetLogicalProcessorInformation");
		if( !glpi ) { return 1; } // winxp?
		PSYSTEM_LOGICAL_PROCESSOR_INFORMATION buffer, ptr = NULL;
		DWORD returnLength = 0;
		glpi( buffer, &returnLength );
		buffer = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION) mem( returnLength );
		BOOL r = glpi( buffer, &returnLength );
		if( !r ) { return 1; } // ???
		ptr = buffer;
		DWORD byteOffset = 0;
		int processorCoreCount = 0;
		while( byteOffset + sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION) <= returnLength ){
			if( ptr->Relationship == RelationProcessorCore ) processorCoreCount++;
			byteOffset += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
			ptr ++; }
		return processorCoreCount; }

	// -------------------------------------------------------------------------------------------------------------- //
	// ------------ EditNumber -------------------------------------------------------------------------------------- //
	
	int EditNumber_out = -1;
	static WNDPROC EditNumber_oldproc;


	int EditNumber_value_next( char ch ){  // what value would be if we insert ch, at current pos, replacing selection
		char txt[7]; SendMessage( cbs3[EditNumber_out], WM_GETTEXT, 7, txt );		
		int sel, selstart, selend;
		char newtxt[7]; int newval;
		
		sel = SendMessage( cbs3[EditNumber_out], EM_GETSEL, 0, 0 );
		selstart = LOWORD(sel);
		selend = HIWORD(sel);
		memcpy( newtxt, txt, selstart );
		newtxt[selstart] = ch;
		memcpy( newtxt+selstart+1, txt+selend, strlen(txt)-selend );
		newtxt[selstart+1 +strlen(txt)-selend] = 0;

		if( sscanf( newtxt, "%d", &newval ) == 1 && newval > -1 && newval <= samplerate )
			return newval;
		
		//print( " INVALID \"%s\" \r\n", newtxt );
		return -1;

	}

	LRESULT CALLBACK EditNumber_proc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ){
		if( msg == WM_CHAR ){

			if( wParam == VK_ESCAPE ){
				EditNumber_rollback();
				SetFocus( hMain );
				return 0; }

			if( wParam == VK_RETURN ){
				EditNumber_commit();
				return 0; }

			if( wParam != VK_BACK && EditNumber_value_next( wParam ) == -1 ){
				return 0; }

		}

		if( msg == WM_PASTE || msg == WM_CLEAR || msg == WM_CUT ){
			return 0; }

		return CallWindowProc( EditNumber_oldproc, hwnd, msg, wParam, lParam);
	}

	void EditNumber_begin( int out ){
		EditNumber_out = out;
		EditNumber_oldproc = SetWindowLongPtr( cbs3[out], GWLP_WNDPROC, EditNumber_proc );
		SendMessageA( cbs3[out], WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));  // repaint
	}

	void EditNumber_rollback(){
		int out = EditNumber_out;
		EditNumber_out = -1;
		SetWindowLongPtr( cbs3[out], GWLP_WNDPROC, EditNumber_oldproc );
		char txt[10]; sprintf( txt, "%d", map[out].delay );
		SendMessage( cbs3[out], WM_SETTEXT, (WPARAM)0, txt );
		SendMessageA( cbs3[out], WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));  // repaint
	}

	void EditNumber_commit(){
		char txt[7]; SendMessage( cbs3[EditNumber_out], WM_GETTEXT, 7, txt );
		//save
		int val; 
		if( sscanf( txt, "%d", &val ) == 1 && val > -1 && val <= samplerate ) {
			print( "SET DELAY out%d delay %d \r\n", EditNumber_out, val );
			map[EditNumber_out].delay = val;
			}
		// refresh reloading
		EditNumber_rollback(); 
		SetFocus( hMain );
	}

	// -------------------------------------------------------------------------------------------------------------- //
	// ---------- WNDPROC ------------------------------------------------------------------------------------------- //

	LRESULT CALLBACK WndProc( HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam ){
		if( msg == WM_COMMAND ){

			if( BN_CLICKED == HIWORD(wParam) && LOWORD(wParam) <= R4 ){  // samplerate clicked

				switch( LOWORD(wParam) ){
					case R1: samplerate = 44100; break;
					case R2: samplerate = 48000; break;
					case R3: samplerate = 96000; break;
					case R4: samplerate = 192000; }

				load_filters( samplerate );
				show_conf();

			} else if( BN_CLICKED == HIWORD(wParam) && (LOWORD(wParam) == BTN01 || LOWORD(wParam) == BTN02) ){  //  config device clicked
				char txt[250];
				GetDlgItemText( hMain, LOWORD(wParam) == BTN01 ? CMB1 : CMB2, txt, 250 );
				if( strstr( Pa_GetHostApiInfo( Pa_GetDeviceInfo( device_id( txt ) )->hostApi )->name, "ASIO" ) != 0 ){
					PaAsio_ShowControlPanel( device_id( txt ), hMain );
				} else {
					STARTUPINFO si; memset( &si, 0, sizeof(STARTUPINFO) );
					PROCESS_INFORMATION pi; memset( &pi, 0, sizeof(PROCESS_INFORMATION) );
					si.cb = sizeof(STARTUPINFO);
					char cmd[300] = "";
					GetWindowsDirectory( cmd, 300 );
					strcat( cmd, "\\System32\\control.exe mmsys.cpl,," );
					strcat( cmd, (LOWORD(wParam) == BTN01) ? "1" : "0" );
					CreateProcessA( 0, cmd, 0, 0, 0, 0, 0, 0, &si, &pi ); }

			} else if( BN_CLICKED == HIWORD(wParam) && LOWORD(wParam) == BTN4 ){  // print_modified_samples clicked
				if( IsDlgButtonChecked( hMain, BTN4 ) ){
					print_modified_samples = 0;
					CheckDlgButton( hMain, BTN4, BST_UNCHECKED );
					print( "print modified samples OFF \r\n" );
				} else {
					CheckDlgButton( hMain, BTN4, BST_CHECKED );
					print_modified_samples = 1;
					print( "print modified samples ON \r\n" );
				}

			} else if( CBN_SELCHANGE == HIWORD(wParam) && LOWORD(wParam) == CMB1 ){  // device 1 changed
				show_conf();

			} else if( CBN_SELCHANGE == HIWORD(wParam) && LOWORD(wParam) == CMB2 ){  // device 2 changed
				show_conf();

			} else if( LOWORD(wParam) == BTN1 ){                                     // play clicked
				int sd, dd;
				char txt[250];
				GetDlgItemText( hMain, CMB1, txt, 250 );
				sd = device_id( txt );
				GetDlgItemText( hMain, CMB2, txt, 250 );
				dd = device_id( txt );
				if( start( sd, dd, samplerate, get_cpu_cores() ) ){                   // START
					EnableWindow( hr1, 0 );
					EnableWindow( hr2, 0 );
					EnableWindow( hr3, 0 );
					EnableWindow( hr4, 0 );
					EnableWindow( hbctl1, 0 );
					EnableWindow( hbctl2, 0 );
					EnableWindow( hCombo1, 0 );
					EnableWindow( hCombo2, 0 );
					EnableWindow( hBtn, 0 );
					show_conf();
					for( int i=0; i<ports[1].channels_count; i++ ){
						if( i < 10 ){
							char key[6]; int val;

							EnableWindow( cbs[i], 1 );
							sprintf( key, "out%d", i+1 );
							if( devices_as_in_conf() && conf_get( key ) && sscanf( conf_get( key ), "%d", &val ) == 1 )
								map[i].src = val-1;
							SendMessage( cbs[i], CB_SETCURSEL, (WPARAM)map[i].src+1, (LPARAM)0 );

							EnableWindow( cbs2[i], 1 );
							sprintf( key, "filter%d", i+1 );
							if( samplerate_as_in_conf() && devices_as_in_conf() && conf_get( key ) && filter_p( conf_get( key ) )){
								struct filter *fl = filter_p( conf_get( key ) );
								set_filter( i, fl->k, fl->kn, fl->name );
								SendMessage( cbs2[i], CB_SETCURSEL, (WPARAM)(filter_i( conf_get( key ) )+1), (LPARAM)0 ); }

							EnableWindow( cbs3[i], 1 );
							sprintf( key, "delay%d", i+1 );
							if( samplerate_as_in_conf() && devices_as_in_conf() && conf_get( key ) && sscanf( conf_get( key ), "%d", &val ) == 1 && val > 0 && val <= samplerate ){
								map[i].delay = val; }
							sprintf( txt, "%d", map[i].delay );
							SendMessage( cbs3[i], WM_SETTEXT, (WPARAM)0, txt );
							
						}
					}
				}

			} else if( (LOWORD(wParam) >= CB1) && LOWORD(wParam) <= CB1+10 && CBN_SELCHANGE == HIWORD(wParam) ){  // out changed
				int out = LOWORD(wParam) - CB1;
				char  txt[20];
				GetDlgItemText( hMain, CB1+out, txt, 20 );
				if( txt[0] == 0 ){
					map[out].src = -1;
					print( "REMOVE SOURCE out%d \r\n", out+1 );
				}else {
					int chan = txt[0]-48-1;
					map[out].src = chan;
					print( "SET SOURCE out%d in%d\r\n", out+1, chan+1 );
				}

			} else if( (LOWORD(wParam) >= CB2) && LOWORD(wParam) <= CB2+10 && CBN_SELCHANGE == HIWORD(wParam) ){  // filter changed
				int out = LOWORD(wParam) - CB2;
				char txt[100];
				GetDlgItemText( hMain, CB2+out, txt, 100 );
				if( txt[0] == 0 ){
					set_filter( out, 0, 0, 0 );
					print( "REMOVE FILTER out%d \r\n", out );
				} else {
					struct filter *fl = filter_p( txt );
					set_filter( out, fl->k, fl->kn, fl->name );
					print( "SET FILTER out%d %s \r\n", out, txt );
				}

			} else if( (LOWORD(wParam) >= CB3) && LOWORD(wParam) <= CB3+10 ){  // delay change
				int out = LOWORD(wParam) - CB3;				
				switch( HIWORD(wParam) ){
					case EN_SETFOCUS:
						EditNumber_begin( out );
						break;
					case EN_KILLFOCUS:
						EditNumber_rollback();
						break;
				}
				
			}

		}

		else if( msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN ){
			if( EditNumber_out > -1 )
				EditNumber_rollback();
			SetFocus( hMain ); }

		else if( msg == WM_CTLCOLOREDIT ){
			int out = GetDlgCtrlID ( lParam ) -CB3;
			if( EditNumber_out > -1 && EditNumber_out == out )
				SetTextColor( (HDC)wParam, RGB(250, 0, 0));
			else
				SetTextColor( (HDC)wParam, RGB(0, 0, 0));
			return (LRESULT) GetStockObject(DC_BRUSH); }

		else if( msg == WM_CLOSE ){
			if( cursor > 0 ) save_conf();
			DestroyWindow( hwnd ); }

		else if( msg == WM_DESTROY ){
			PostQuitMessage( 0 ); }

		return DefWindowProc( hwnd, msg, wParam, lParam ); }


	// -------------------------------------------------------------------------------------------------------------- //
	// ---------------------------- draw ---------------------------------------------------------------------------- //
	HDC hdc, hdcMem;
	RECT rc;
	HBITMAP hbmp;
	void ** pixels;
	int VPw = 40000;        // ViewPort width
	int VPh = 2500;        // ViewPort height
	int VPx = 0;           // ViewPort pos x corner
	int VPy = 0;           // ViewPort pos y corner
	POINT points[192000*2];
	void draw_prepare(){
		BITMAPINFO bmi;
		memset( &bmi, 0, sizeof(bmi) );
		bmi.bmiHeader.biSize = sizeof(bmi);
		bmi.bmiHeader.biWidth = 200;
		bmi.bmiHeader.biHeight =  -200;         // Order pixels from top to bottom
		bmi.bmiHeader.biPlanes = 1;
		bmi.bmiHeader.biBitCount = 32;             // last byte not used, 32 bit for alignment
		bmi.bmiHeader.biCompression = BI_RGB;
		hdc = GetDC( hMain );
		hbmp = CreateDIBSection( hdc, &bmi, DIB_RGB_COLORS, &pixels, 0, 0 );
		hdcMem = CreateCompatibleDC( hdc );
		SelectObject( hdcMem, hbmp );
		SetBkMode( hdcMem, TRANSPARENT ); }
	void transform_point( POINT *p ){
		double Qw = ((double)200)/((double)VPw);
		double Qh = ((double)200)/((double)VPh);
		p->x = (long)round( (p->x - VPx) * Qw );
		p->y = (long)round( (p->y - VPy) * Qh );
		p->y = 200 - p->y; }
	void place_marker( int val, COLORREF c ){
		POINT p1 = { VPx, val };
		POINT p2 = { VPx+VPw, val };
		transform_point( &p1 );
		transform_point( &p2 );
		MoveToEx( hdcMem, p1.x, p1.y, 0 );
		SetDCPenColor(hdcMem, c);
		LineTo( hdcMem, p2.x, p2.y ); }
	void draw(){
		memset( pixels, 192, 200*200*4 );
	    SelectObject( hdcMem, GetStockObject(DC_PEN) );
		if( ports && ports[0].stats_len && ports[1].stats_len ){

			int now = clock_time();
			
			// move view
			///*
			if( now + 1000 > ports[0].t0 + VPw ){  // look 1000 samples in future
				VPx = now + 1000 - VPw; }
			else { VPx = now - VPw; }			
			//*/
			
			// draw 2 plylines
			for( int i=0; i<2; i++ ){

				int bias = (i==0 ? -ports[0].min +(ports[1].max-ports[1].min) : -ports[1].min );

				if( ports[i].stats_len ){
					SetDCPenColor(hdcMem, RGB( 199-i*199, i*66 ,0 ) );
					int pi=0;
					for( int si=ports[i].stats_len-1; si>=0; si-- ) {
						points[pi].x = ports[i].stats[si%ssize].t;
						points[pi].y = ports[i].stats[si%ssize].avail + bias;
						transform_point( points+pi );
						if( points[pi].x < 0 ) break;
						pi ++; }
					Polyline( hdcMem, points, pi );	
				}

				// and 2 lines
				//place_marker( ports[0].min+bias, RGB(255, 0,0) );
				//place_marker( ports[1].max+bias, RGB(0, 99, 0) );

			}
		}

		if( gstat_len ){
			SetDCPenColor(hdcMem, RGB(0,0,99));
			int pi=0;
			for( int i=gstat_len-1; i>0; i-- ) {
				points[pi].x = gstat[i%ssize].t;
				points[pi].y = gstat[i%ssize].avail;
				transform_point( points+pi );
				if( points[pi].x < 0 ) break;
				pi ++; }
			Polyline( hdcMem, points, pi );
			place_marker( G, RGB(255,255,00) );
		}
		if( lstat_len ){
			//place_marker( L, RGB(250,50,255) );
			SetDCPenColor( hdcMem, RGB(255,255,255));
			int pi=0;
			for( int i=lstat_len-1; i>0; i-- ) {
				points[pi].x = lstat[i%ssize].t;
				points[pi].y = lstat[i%ssize].avail;
				transform_point( points+pi );
				if( points[pi].x < 0 ) break;
				pi ++; }
			Polyline( hdcMem, points, pi );

		}
		place_marker( 0, RGB(0,0,0) );
		MoveToEx( hdcMem, 0, 0, 0 );
		SetDCPenColor(hdcMem, RGB(0,0,0));
		LineTo( hdcMem, 199, 0 ); LineTo( hdcMem, 199, 199 );
		LineTo( hdcMem, 0, 199 ); LineTo( hdcMem, 0, 0 );
		BitBlt( hdc, 382, 460, 200, 200, hdcMem, 0, 0, SRCCOPY );
	}

	// ------------------------------------------------------------------------------------------------------------ //
	// ------- refresh callbacks ---------------------------------------------------------------------------------- //

	void CALLBACK every_10ms( HWND hwnd, UINT uMsg, UINT timerId, DWORD dwTime ){
		draw(); }
		
	void CALLBACK every_100ms( HWND hwnd, UINT uMsg, UINT timerId, DWORD dwTime ){
		if( console_changed ){
			console_changed = 0;
			SetWindowText( hEdit, console ); }		
		if( cursor > -1 ){
			char txt[50]; sprintf( txt, "Load: %d%% \n", (int)ceil((Pa_GetStreamCpuLoad(ports[0].stream)+Pa_GetStreamCpuLoad(ports[1].stream))*100.0) );
			SetWindowText( hLL, txt ); } }

	// ############################################################################################################# //
	// ############### WINMAIN ##################################################################################### //

	int WINAPI WinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow ){

		// window class
		WNDCLASSEX wc;
		memset( &wc, 0, sizeof(wc) );
		wc.cbSize = sizeof(wc);
		wc.hInstance = hInstance;
		wc.lpfnWndProc = WndProc;
		wc.lpszClassName = "mainwindow";
		wc.hbrBackground = COLOR_WINDOW; //CreateSolidBrush( RGB(64, 64, 64) );
		wc.hCursor = LoadCursor( 0, IDC_ARROW );
		wc.hIconSm = LoadIcon(hInstance, "main.ico");
		RegisterClassEx(&wc);

		// fonts
		strcpy(font1.lfFaceName, "Wingdings");
		font1.lfCharSet = DEFAULT_CHARSET;
		hfont1 = CreateFontIndirect(&font1);
		strcpy(font2.lfFaceName, "Tahoma");
		font2.lfCharSet = DEFAULT_CHARSET;
		font2.lfHeight = 16;
		hfont2 = CreateFontIndirect(&font2);
		strcpy(font3.lfFaceName, "Tahoma");
		font3.lfCharSet = DEFAULT_CHARSET;
		font3.lfHeight = 18;
		hfont3 = CreateFontIndirect(&font3);
		strcpy(font4.lfFaceName, "Consolas");
		font4.lfCharSet = DEFAULT_CHARSET;
		font4.lfHeight = 13;
		hfont4 = CreateFontIndirect(&font4);

		// create
		hMain = CreateWindowEx( WS_EX_APPWINDOW, "mainwindow", "RTFIR", WS_MINIMIZEBOX | WS_SYSMENU | WS_POPUP | WS_CAPTION, 300, 200, 600, 700, 0, 0, hInstance, 0 );
		hr1 = CreateWindowEx( 0, "Button", "44.1k", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_AUTORADIOBUTTON|WS_GROUP, 70, 10, 50, 23, hMain, R1, 0, 0);
		hr2 = CreateWindowEx( 0, "Button", "48k", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_AUTORADIOBUTTON, 160, 10, 50, 23, hMain, R2, 0, 0);
		hr3 = CreateWindowEx( 0, "Button", "96k", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_AUTORADIOBUTTON, 230, 10, 50, 23, hMain, R3, 0, 0);
		hr4 = CreateWindowEx( 0, "Button", "192k", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_AUTORADIOBUTTON, 300, 10, 50, 23, hMain, R4, 0, 0);
		hLL = CreateWindowEx( 0, "static", "", WS_VISIBLE|WS_CHILD, 410, 13, 80, 15, hMain, LL, NULL, NULL);
		hbctl1 = CreateWindowEx( 0, "Button", "\x052", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON, 10, 40, 30, 23, hMain, BTN01, NULL, NULL);
		hbctl2 = CreateWindowEx( 0, "Button", "\x052", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_PUSHBUTTON, 10, 70, 30, 23, hMain, BTN02, NULL, NULL);
		hCombo1 = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST|CBS_SORT, 48, 40, 450, 8000, hMain, CMB1, NULL, NULL);
		hCombo2 = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST|CBS_SORT, 48, 70, 450, 8000, hMain, CMB2, NULL, NULL);
		hBtn = CreateWindowEx( 0, "Button", "Play >", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_DEFPUSHBUTTON, 507, 40, 77, 53, hMain, BTN1, NULL, NULL);
		hEdit = CreateWindowEx( 0, "static", "", WS_CHILD | WS_VISIBLE, 15, 460, 365, 200, hMain, EDT, NULL, NULL);
		hBtn4 = CreateWindowEx( 0, "Button", "print modified samples", WS_VISIBLE|WS_CHILD|WS_TABSTOP|BS_CHECKBOX, 15, 435, 150, 23, hMain, BTN4, 0, 0);

		// set fonts
		SendMessageA( hr1, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
		SendMessageA( hr2, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
		SendMessageA( hr3, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
		SendMessageA( hr4, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
		SendMessageA( hLL, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
		SendMessageA( hbctl1, WM_SETFONT, (WPARAM)hfont1, (LPARAM)MAKELONG(TRUE, 0));
		SendMessageA( hbctl2, WM_SETFONT, (WPARAM)hfont1, (LPARAM)MAKELONG(TRUE, 0));
		SendMessageA( hCombo1, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
		SendMessageA( hCombo2, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
		SendMessageA( hBtn, WM_SETFONT, (WPARAM)hfont3, (LPARAM)MAKELONG(TRUE, 0));
		SendMessageA( hBtn4, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
		SendMessageA( hEdit, WM_SETFONT, (WPARAM)hfont4, (LPARAM)MAKELONG(TRUE, 0));

		// create rows
		for( int i=0; i<10; i++ ){
			char str[50]; HANDLE h;
			// label
			sprintf( str, "out%d   %ssrc", i+1, i+1==10 ? "  " : "    " );
			h = CreateWindowEx( 0, "static", str, WS_VISIBLE|WS_CHILD, 49, 123+i*30, 80, 15, hMain, LB1+i, NULL, NULL);
			SendMessageA( h, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
			// COMBOBOX
			cbs[i] = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST, 127, 120+i*30, 50, 800, hMain, CB1+i, NULL, NULL);
			SendMessageA( cbs[i], WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
			EnableWindow( cbs[i], 0 );
			// label
			h = CreateWindowEx( 0, "static", "filter", WS_VISIBLE|WS_CHILD, 202, 123+i*30, 50, 15, hMain, LB1+10+i, NULL, NULL);
			SendMessageA( h, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
			// COMBOBOX
			cbs2[i] = CreateWindowEx( 0, "ComboBox", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|CBS_DROPDOWNLIST, 235, 120+i*30, 150, 800, hMain, CB2+i, NULL, NULL);
			SendMessageA( cbs2[i], WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
			EnableWindow( cbs2[i], 0 );
			// label
			h = CreateWindowEx( 0, "static", "delay", WS_VISIBLE|WS_CHILD, 407, 123+i*30, 50, 15, hMain, LB1+10+i, NULL, NULL);
			SendMessageA( h, WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
			// EDIT
			cbs3[i] = CreateWindowEx( WS_EX_CLIENTEDGE, "Edit", 0, WS_VISIBLE|WS_CHILD|WS_TABSTOP|ES_NUMBER|ES_CENTER, 445, 120+i*30, 60, 24, hMain, CB3+i, NULL, NULL);
			SendMessageA( cbs3[i], EM_SETLIMITTEXT, (WPARAM)6, (LPARAM)MAKELONG(TRUE, 0));
			SendMessageA( cbs3[i], WM_SETFONT, (WPARAM)hfont2, (LPARAM)MAKELONG(TRUE, 0));
			EnableWindow( cbs3[i], 0 );
			}

		// show
		ShowWindow( hMain, SW_SHOW );

		// init
		print( "Built " __DATE__ " " __TIME__ "\r\n" );
		init();
		conf_load( "RTFIR.conf" );

		// populate device dropdowns
		for( int i=0, ci; i<Pa_GetDeviceCount(); i++ ){
			if( Pa_GetDeviceInfo( i )->maxInputChannels ){
				ci = SendMessage( hCombo1, CB_ADDSTRING, 0, device_name( i ) );
				if( conf_get( "device1" ) && strcmp( conf_get( "device1" ), device_name( i ) ) == 0 )
					SendMessage( hCombo1, CB_SETCURSEL, (WPARAM)ci, (LPARAM)0 ); }
			if( Pa_GetDeviceInfo( i )->maxOutputChannels ){
				ci = SendMessage( hCombo2, CB_ADDSTRING, 0, device_name( i ) );
				if( conf_get( "device2" ) && strcmp( conf_get( "device2" ), device_name( i ) ) == 0 )
					SendMessage( hCombo2, CB_SETCURSEL, (WPARAM)ci, (LPARAM)0 ); } }
		// if no selection choose the first in the list
		if( SendMessage( hCombo1, CB_GETCURSEL, (WPARAM)0, (LPARAM)0 ) == CB_ERR )
			SendMessage( hCombo1, CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );
		if( SendMessage( hCombo2, CB_GETCURSEL, (WPARAM)0, (LPARAM)0 ) == CB_ERR )
			SendMessage( hCombo2, CB_SETCURSEL, (WPARAM)0, (LPARAM)0 );

		// populate samplerate radios
		int samplerate_from_conf = -1;
		if( conf_get( "samplerate" ) && sscanf( conf_get( "samplerate" ), "%d", &samplerate_from_conf ) == 1 ){
			switch( samplerate_from_conf ){
				case 44100: SendMessage( hr1, BM_CLICK, 0, 0 ); break;
				case 48000: SendMessage( hr2, BM_CLICK, 0, 0 ); break;
				case 96000: SendMessage( hr3, BM_CLICK, 0, 0 ); break;
				case 192000: SendMessage( hr4, BM_CLICK, 0, 0 ); }
		} else SendMessage( hr1, BM_CLICK, 0, 0 );

		draw_prepare();	
		SetTimer( 0, 0, 10, (TIMERPROC) &every_10ms );
		SetTimer( 0, 0, 100, (TIMERPROC) &every_100ms );

		// loop
		while( GetMessage( &msg, 0, 0, 0 ) > 0 ){
			TranslateMessage( &msg );
			DispatchMessage( &msg ); }

	return msg.wParam; }
