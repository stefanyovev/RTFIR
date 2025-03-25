
project(RTFIR)
#set(nappgui_Root nappgui_src)
find_package(nappgui REQUIRED)
find_package(portaudio REQUIRED)

include("nappgui_src/prj/NAppProject.cmake")
include("nappgui_src/prj/NAppCompilers.cmake")

nap_config_compiler()
nap_project_desktop_app(RTFIR src)

