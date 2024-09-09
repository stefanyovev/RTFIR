
@set LIBRARY_PATH=%~dp0%lib
@set PATH=%PATH%%LIBRARY_PATH%;

@tcc -lportaudio -lconvolve -luser32 -lgdi32 main.c -o RTFIR.exe

@if %errorlevel% neq 0 goto error

@RTFIR.exe
@goto endd

:error
@echo ERROR compiling main.c
@pause

:endd
