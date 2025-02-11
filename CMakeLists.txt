
macro(SET_OPTION option value)
  set(${option} ${value} CACHE "" INTERNAL FORCE)
endmacro()

SET_OPTION(WIN32 1)
SET_OPTION(PA_USE_ASIO ON)
SET_OPTION(LINK_PRIVATE_SYMBOLS ON)
SET_OPTION(BUILD_SHARED_LIBS OFF)
SET_OPTION(PA_USE_DS OFF)
SET_OPTION(PA_USE_WMME OFF)

add_subdirectory(portaudio)

add_compile_options(-std=c99 -march=native -O3 -fPIC -fpermissive -w)
add_executable(RTFIR main.c main.rc)
target_link_libraries(RTFIR PortAudio synchronization gdi32 stdc++ -mwindows -static)

