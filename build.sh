windres main.rc -O coff -o main.res
gcc main.c main.res -o RTFIR -std=c99 -O3 -march=native -mwindows -msse3 -mavx -fno-aggressive-loop-optimizations -fPIC -fpermissive -w
rm main.res