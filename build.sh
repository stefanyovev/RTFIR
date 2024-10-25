
# msys2

pacman -S mingw-w64-ucrt-x86_64-gcc --needed
pacman -S make --needed
pacman -S cmake --needed

cp -rf main.c portaudio
cp -rf conf.c portaudio
cp -rf main.ico portaudio

echo "
main.ico ICON \"main.ico\"
1 VERSIONINFO
FILEVERSION     1,0,0,0
BEGIN
  BLOCK \"StringFileInfo\"
  BEGIN
    BLOCK \"080904E4\"
    BEGIN      
      VALUE \"ProductName\", \"RTFIR\"
      VALUE \"ProductVersion\", \"9\"
      VALUE \"LegalCopyright\", \"RTFIR.com\"
      VALUE \"OriginalFilename\", \"RTFIR.exe\"
      VALUE \"FileDescription\", \"Realtime Sound EQ\"
    END
  END
  BLOCK \"VarFileInfo\"
  BEGIN
    VALUE \"Translation\", 0x809, 1252
  END
END
" > portaudio/main.rc

# -------------------------------------------------
if [ ! -f portaudio/CMakeLists.txt.changed ]; then
    echo "
    add_compile_options(-std=c99 -march=native -O3 -msse3 -mavx -fno-aggressive-loop-optimizations -fPIC -fpermissive -w)
    add_executable(RTFIR main.c main.rc)
    target_link_libraries(RTFIR PortAudio gdi32 stdc++ -mwindows)
    " > portaudio/CMakeLists.txt.changed
    cat portaudio/CMakeLists.txt.changed >> portaudio/CMakeLists.txt
fi

# -------------------------------------------------
cd portaudio
rm -f CMakeCache.txt
rm -rf CMakeFiles
cmake -D WIN32=1 -D PA_USE_ASIO=ON -D PA_USE_WMME=OFF -D PA_USE_DS=OFF -D BUILD_SHARED_LIBS=OFF -D LINK_PRIVATE_SYMBOLS=ON .
cmake --build .
if [ $? -ne 0  ]; then exit; fi;
cd ..

# -------------------------------------------------
rm -rf dist
mkdir dist
cp -r filters dist
cp portaudio/RTFIR.exe dist

# -------------------------------------------------
./dist/RTFIR.exe&
