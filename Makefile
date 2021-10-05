.PHONY: clean all
all: bin/audiofile.o z64audio

AudioToolsDep := $(shell find lib/*/*/* -type f -name '*.c')
z64AudioDep := z64audio.c lib/HermosauhuLib.c lib/AudioConvert.c lib/HermosauhuLib.h lib/AudioConvert.h lib/AudioTools.h

bin/audiofile.o: $(AudioToolsDep)
	mkdir -p bin/
	cd bin && g++ -std=c++11 -c -s -Ofast -DNDEBUG ../lib/audiofile/audiofile.cpp -I../lib/audiofile
	cd bin && gcc -c ../lib/sdk-tools/tabledesign/*.c -s -Ofast -DNDEBUG -I../lib/audiofile
	cd bin && gcc -c ../lib/sdk-tools/adpcm/*.c -s -Ofast -DNDEBUG -I../lib/audiofile

z64audio: $(z64AudioDep)
	gcc -c z64audio.c lib/HermosauhuLib.c lib/AudioConvert.c -Wall -Ofast -s -flto -DNDEBUG
	g++ -o $@ z64audio.o HermosauhuLib.o AudioConvert.o bin/*.o -Ofast -s -flto -DNDEBUG

clean:
	rm bin/*.o *.o