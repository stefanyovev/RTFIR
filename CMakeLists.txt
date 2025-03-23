


include_directories(build/inc)  ## ?
include_directories(build/include)  ## ?
link_directories(build/lib)  ## ?

add_executable(RTFIR src/main.c)

target_link_libraries( RTFIR

	build/lib/portaudio
	
	build/lib/casino
	build/lib/core
	build/lib/draw2d
	build/lib/geom2d
	build/lib/gui
	build/lib/inet
	build/lib/ogl3d
	build/lib/osapp
	build/lib/osbs
	build/lib/osgui
	build/lib/sewer
	
gdi32
	
	)
	
	
	
	include( nappgui_src/prj/NAppProject.cmake )
	include( nappgui_src/prj/NAppCompilers.cmake )
	
	nap_config_compiler()
	nap_project_desktop_app(napphello hello)