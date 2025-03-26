
nap_desktop_app(RTFIR "" NRC_NONE)

if( WIN32 )
  target_link_libraries(RTFIR portaudio MSVCRT LIBCMT)
else()
  target_link_libraries(RTFIR portaudio stdc++)
endif()