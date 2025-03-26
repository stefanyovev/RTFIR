
project(RTFIR)

find_package(nappgui REQUIRED)
find_package(portaudio REQUIRED)

include("nappgui_src/prj/NAppProject.cmake")
include("nappgui_src/prj/NAppCompilers.cmake")

include_directories(build/include)

nap_config_compiler()
nap_project_desktop_app(RTFIR src)

