Rem arcajs bootstrap script for Windows 10 using the MinGW-w64 compiler, obtainable at https://sourceforge.net/projects/mingw-w64/

curl.exe http://libsdl.org/release/SDL2-devel-2.0.14-mingw.tar.gz -o SDL2-devel-2.0.14-mingw.tar.gz
tar.exe -xf .\SDL2-devel-2.0.14-mingw.tar.gz
ren SDL2-2.0.14 SDL2
cd SDL2
md lib
md lib\win32-x64
copy x86_64-w64-mingw32\lib\libSDL2.a lib\win32-x64\
copy x86_64-w64-mingw32\lib\libSDL2main.a lib\win32-x64\
move x86_64-w64-mingw32\include\SDL2 .
ren SDL2 include
cd ..

curl.exe -L https://github.com/eludi/arcajs/archive/master.zip -o arcajs.zip
tar.exe -xf .\arcajs.zip
ren arcajs-master arcajs
cd arcajs
mingw32-make.exe static
del /s *.o
