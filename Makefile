.PHONY: clean all
all: bin/audiofile.o z64audio

AudioTools := $(shell find lib/*/*/* -type f -name '*.c')

bin/audiofile.o: $(AudioTools)
	mkdir -p bin/
	cd bin && g++ -std=c++11 -c -s -Ofast -DNDEBUG ../lib/audiofile/audiofile.cpp -I../lib/audiofile
	cd bin && gcc -c ../lib/sdk-tools/tabledesign/*.c -s -Ofast -DNDEBUG -I../lib/audiofile
	cd bin && gcc -c ../lib/sdk-tools/adpcm/*.c -s -Ofast -DNDEBUG -I../lib/audiofile

z64audio: z64audio.c lib/HermosauhuLib.c
	gcc -c z64audio.c lib/HermosauhuLib.c -Wall -Ofast -s -flto -DNDEBUG
	g++ -o $@ z64audio.o HermosauhuLib.o bin/*.o -Ofast -s -flto -DNDEBUG

clean:
	rm bin/*.o *.o