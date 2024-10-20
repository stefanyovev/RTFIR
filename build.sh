
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

gcc main.c main.res -o RTFIR -std=c99 -O3 -march=native -mwindows -msse3 -mavx -fno-aggressive-loop-optimizations -fPIC -fpermissive -w

if [ $? -eq 0 ]; then
    ./RTFIR.exe &
fi

rm main.res