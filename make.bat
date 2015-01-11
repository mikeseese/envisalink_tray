cl -D_WIN32_WINNT=0x0501 /MD /EHsc /I\usr\include src\main.cpp /link /LIBPATH:\usr\lib /Fbin/main.exe
cl -D_WIN32_WINNT=0x0501 /MD /EHsc /I\usr\include src\server.cpp /link /LIBPATH:\usr\lib /Fbin/server.exe
cl -D_WIN32_WINNT=0x0501 /MD /EHsc /I\usr\include src\client.cpp /link /LIBPATH:\usr\lib /Fbin/client.exe
