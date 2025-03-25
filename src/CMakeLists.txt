
nap_desktop_app(RTFIR "" NRC_NONE)
#nap_add_dependency(RTFIR portaudio)

if (WIN32)
  target_link_libraries(RTFIR portaudio MSVCRT LIBCMT)
else()
  target_link_libraries(RTFIR portaudio stdc++)
endif()