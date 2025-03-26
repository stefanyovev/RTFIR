echo #########################################################################
echo ### NAppGUI #############################################################
cd nappgui_src
cmake .
cmake --build . --config Release
cmake --install . --config Release --prefix ../build
cd ..
echo #########################################################################
echo #### PortAudio ##########################################################
cd portaudio
cmake .
cmake --build . --config Release
cmake --install . --config Release --prefix ../build
cd ..
echo #########################################################################
echo ### SELF ################################################################
cmake . -B build
cmake --build build --config Release