# this is for building a version using external executables
# for tabledesign/vadpcm_enc, which makes benchmarking and
# debugging z64audio-specific code easier

# cleanup
rm *.o

# compile
#gcc -c z64snd.c -Wall -g -Og -DZ64AUDIO_EXTERNAL_DEPENDENCIES
gcc -c z64snd.c -Wall -g -Og

# link compiled objects into executable using g++
mkdir -p bin/debug
g++ -o bin/debug/z64audio *.o bin/o/linux/*.o




