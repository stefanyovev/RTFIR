
# msys2

pacman -S mingw-w64-ucrt-x86_64-gcc --needed --noconfirm
pacman -S make --needed --noconfirm
pacman -S cmake --needed --noconfirm

rm -rf portaudio/CMakeFiles

cmake .
cmake --build .

if [ $? -ne 0  ]; then exit; fi;

./RTFIR.exe&
