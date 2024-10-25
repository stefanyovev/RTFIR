
cd portaudio
cmake -D WIN32=1 -D PA_USE_ASIO=ON -D PA_USE_MME=OFF -D PA_USE_DS=OFF -D BUILD_SHARED_LIBS=ON -D BUILD_STATIC_LIBS=ON -D LINK_PRIVATE_SYMBOLS=ON -D WINDOWS_EXPORT_ALL_SYMBOLS=ON .
cmake --build .
cp msys-portaudio-2.dll ../
cd ..
mv msys-portaudio-2.dll portaudio.dll


if [ -f RTFIR.exe ]; then
    rm RTFIR.exe
fi

g++ -c main.c -std=c99 -lstdc++ -march=native -O3 -mwindows -msse3 -mavx -fno-aggressive-loop-optimizations -fPIC -fpermissive -w

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
" >> main.rc

windres main.rc -O coff -o main.res
rm main.rc
g++ main.o main.res -lstdc++ -lgdi32 -o RTFIR
rm main.res
rm main.o

if [ -f RTFIR.exe ]; then
    ./RTFIR.exe &
fi
