# cleanup
rm *.o

# compile
gcc -c z64snd.c -Wall -Os -s -flto -DNDEBUG

# link compiled objects into executable using g++
mkdir -p bin/release
g++ -o bin/release/z64audio-linux-64bit *.o bin/o/linux/*.o -Os -s -flto -DNDEBUG




