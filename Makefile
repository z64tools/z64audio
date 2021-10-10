CFLAGS := -Ofast -s -flto -DNDEBUG
OBJCFLAGS := -Ofast -s -DNDEBUG
gccw32 = i686-w64-mingw32.static-gcc
g++w32 = i686-w64-mingw32.static-g++

AudioToolsDep := $(shell find lib/*/*/* -type f -name '*.c')
z64AudioDep := z64audio.c lib/HermosauhuLib.c lib/AudioConvert.c lib/HermosauhuLib.h lib/AudioConvert.h lib/AudioTools.h lib/AudioTools.c

.PHONY: clean all win lin
all: lin
win: bin-win/audiofile.o z64audio.exe
lin: bin/audiofile.o z64audio

clean:
	rm -f bin/*.o bin-win/*.o z64audio z64audio.exe *.table *.aifc

# LINUX BUILD
bin/audiofile.o: $(AudioToolsDep)
	mkdir -p bin/
	cd bin && g++ -std=c++11 -c ../lib/audiofile/audiofile.cpp -I../lib/audiofile $(OBJCFLAGS)
	cd bin && gcc -c ../lib/sdk-tools/tabledesign/*.c -I../lib/audiofile $(OBJCFLAGS)
	cd bin && gcc -c ../lib/sdk-tools/adpcm/*.c -I../lib/audiofile $(OBJCFLAGS)

z64audio: $(z64AudioDep)
	cd bin && gcc -c ../z64audio.c ../lib/HermosauhuLib.c ../lib/AudioConvert.c ../lib/AudioTools.c -Wall $(CFLAGS)
	g++ -o $@ bin/*.o $(CFLAGS)

# WINDOWS BUILD
bin-win/audiofile.o: $(AudioToolsDep)
	mkdir -p bin-win/
	cd bin-win && $(g++w32) -std=c++11 -c ../lib/audiofile/audiofile.cpp -I../lib/audiofile $(OBJCFLAGS) -D_WIN32
	cd bin-win && $(gccw32) -c ../lib/sdk-tools/tabledesign/*.c -I../lib/audiofile $(OBJCFLAGS) -D_WIN32
	cd bin-win && $(gccw32) -c ../lib/sdk-tools/adpcm/*.c -I../lib/audiofile $(OBJCFLAGS) -D_WIN32

z64audio.exe: $(z64AudioDep)
	cd bin-win && $(gccw32) -c ../z64audio.c ../lib/HermosauhuLib.c ../lib/AudioConvert.c ../lib/AudioTools.c -Wall $(CFLAGS) -D_WIN32
	$(g++w32) -o $@ bin-win/*.o $(CFLAGS) -D_WIN32