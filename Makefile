GCC = gcc
GPP = g++
WIN_GCC = ~/c/mxe/usr/bin/i686-w64-mingw32.static-gcc
WIN_GPP = ~/c/mxe/usr/bin/i686-w64-mingw32.static-g++
MAGIC = -Ofast
ADPCM = tools/sdk-tools/adpcm/

.PHONY: objlinux objwin32 clean objects all

default: win32 linux

objects: objwin32 objlinux

linux: z64audio
win32: z64audio.exe

all: objects default

objlinux:
	@$(GPP) -std=c++11 -c $(MAGIC) -DNDEBUG tools/audiofile/audiofile.cpp -Itools/audiofile
	@$(GCC) -c tools/sdk-tools/tabledesign/*.c -s $(MAGIC) -flto -DNDEBUG -Itools/audiofile
	@$(GCC) -c $(ADPCM)quant.c $(ADPCM)sampleio.c $(ADPCM)util.c $(ADPCM)vpredictor.c $(ADPCM)vadpcm_enc.c $(ADPCM)vencode.c -s $(MAGIC) -flto -DNDEBUG -Itools/audiofile
	@mkdir -p bin/o/linux
	@mv *.o bin/o/linux

z64audio: z64snd.c include/z64snd.h
	@$(GCC) -c z64snd.c -Wall
	@$(GPP) -o z64audio *.o bin/o/linux/*.o $(MAGIC) -s -flto -DNDEBUG
	@rm *.o

objwin32:
	@$(GPP) -std=c++11 -c $(MAGIC) -DNDEBUG tools/audiofile/audiofile.cpp -Itools/audiofile
	@$(GCC) -c tools/sdk-tools/tabledesign/*.c -s $(MAGIC) -flto -DNDEBUG -Itools/audiofile
	@$(GCC) -c $(ADPCM)quant.c $(ADPCM)sampleio.c $(ADPCM)util.c $(ADPCM)vpredictor.c $(ADPCM)vadpcm_enc.c $(ADPCM)vencode.c -s $(MAGIC) -flto -DNDEBUG -Itools/audiofile
	@mkdir -p bin/o/win32
	@mv *.o bin/o/win32

z64audio.exe: z64snd.c include/z64snd.h
	@$(GCC) -c z64snd.c -Wall
	@$(GPP) -o z64audio.exe *.o bin/o/win32/*.o $(MAGIC) -s -flto -DNDEBUG
	@rm *.o

clean:
	@rm *.tsv *.bin
