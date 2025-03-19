

		// error
		void  os_error ( void* pmsg );

		// cores
		void* os_cores ();

		// mutex
		void* os_mutex ();
		void  os_mutex_up ( void* id );
		void  os_mutex_down ( void* id );

		// event
		void* os_event ();
		void  os_event_up ( void* id );
		void  os_event_down ( void* id );
		void  os_event_wait ( void* id );

		// thread
		void* os_thread( void* func, void* arg );

		// clock
		void* os_clock ();
		void* os_clock_freq ();

		// timer
		void* os_timer ( void* func, void* ms );

		// ui
		void* os_window ( void* title, void* width, void* height );
		void* os_font ( void* resource, void* size, void* color );
		void* os_bitmap ( void* parent, void* x, void* y, void* width, void* height );
		void* os_label ();
		void* os_button ( void* parent, void* title, void* x, void* y, void* w, void* h );
		void* os_checkbox ();
		void* os_combobox ();
		void* os_editbox ();

		// ui mainloop
		void  os_run ();