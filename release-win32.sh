# cleanup
rm *.o

# compile
~/c/mxe/usr/bin/i686-w64-mingw32.static-gcc -c z64snd.c -Wall -Os -s -flto -DNDEBUG

# icon
~/c/mxe/usr/bin/i686-w64-mingw32.static-windres icon.rc -o bin/o/win32/icon.o

# link compiled objects into executable using g++
mkdir -p bin/release
~/c/mxe/usr/bin/i686-w64-mingw32.static-g++ -o bin/release/z64audio-windows-32bit.exe *.o bin/o/win32/*.o -Os -s -flto -DNDEBUG




