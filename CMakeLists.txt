
project(RTFIR)
set(nappgui_Root nappgui_src)
find_package(nappgui REQUIRED)

include("${NAPPGUI_ROOT_PATH}/prj/NAppProject.cmake")
include("${NAPPGUI_ROOT_PATH}/prj/NAppCompilers.cmake")
nap_config_compiler()
nap_project_desktop_app(RTFIR src)

