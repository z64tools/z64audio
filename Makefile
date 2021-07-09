GCC         = gcc
GPP         = g++
WIN_GCC     = ~/c/mxe/usr/bin/i686-w64-mingw32.static-gcc
WIN_GPP     = ~/c/mxe/usr/bin/i686-w64-mingw32.static-g++
MAGIC       = -Ofast
ADPCM       = tools/sdk-tools/adpcm/

.PHONY: objlinux objwin32 clean objects all

default: win32 linux

objects: objwin32 objlinux

linux: z64audio
win32: z64audio.exe

all: objects default

objlinux:
	@rm -f bin/o/linux/*.o
	@$(GPP) -std=c++11 -c -s -Ofast -DNDEBUG tools/audiofile/audiofile.cpp -Itools/audiofile -Iwowlib -DWOW_OVERLOAD_FILE
	@$(GCC) -c tools/sdk-tools/tabledesign/*.c -s -Ofast -DNDEBUG -Itools/audiofile -Iwowlib -DWOW_OVERLOAD_FILE
	@$(GCC) -c $(ADPCM)quant.c $(ADPCM)sampleio.c $(ADPCM)util.c $(ADPCM)vpredictor.c $(ADPCM)vadpcm_enc.c $(ADPCM)vencode.c -s -Ofast -DNDEBUG -Itools/audiofile -Iwowlib -DWOW_OVERLOAD_FILE
	@mkdir -p bin/o/linux
	@mv *.o bin/o/linux

z64audio: z64snd.c include/z64snd.h
	@$(GCC) -c z64snd.c -Wall -Os -s -flto -DNDEBUG -Iwowlib -DWOW_OVERLOAD_FILE -Wno-format-truncation
	@$(GPP) -o z64audio z64snd.o bin/o/linux/*.o -Os -s -flto -DNDEBUG
	@rm -f *.o

objwin32:
	@rm -f bin/o/win32/*.o
	@$(WIN_GPP) -std=c++11 -c -Ofast -s -DNDEBUG tools/audiofile/audiofile.cpp -Itools/audiofile -Iwowlib -DWOW_OVERLOAD_FILE -municode -DUNICODE -D_UNICODE
	@$(WIN_GCC) -c tools/sdk-tools/tabledesign/*.c -Ofast -s -DNDEBUG -Itools/audiofile -Iwowlib -DWOW_OVERLOAD_FILE -municode -DUNICODE -D_UNICODE
	@$(WIN_GCC) -c $(ADPCM)quant.c $(ADPCM)sampleio.c $(ADPCM)util.c $(ADPCM)vpredictor.c $(ADPCM)vadpcm_enc.c $(ADPCM)vencode.c -Ofast -s -DNDEBUG -Itools/audiofile -Iwowlib -DWOW_OVERLOAD_FILE -municode -DUNICODE -D_UNICODE
	@mkdir -p bin/o/win32
	@mv *.o bin/o/win32

z64audio.exe: z64snd.c include/z64snd.h
	@$(WIN_GCC) -c z64snd.c -Wall -Os -s -flto -DNDEBUG -Iwowlib -DWOW_OVERLOAD_FILE -municode -DUNICODE -D_UNICODE
	@~/c/mxe/usr/bin/i686-w64-mingw32.static-windres icon.rc -o bin/o/win32/icon.o
	@$(WIN_GPP) -o z64audio.exe z64snd.o bin/o/win32/*.o -Os -s -flto -DNDEBUG -municode -DUNICODE -D_UNICODE
	@rm -f *.o

clean:
	@rm -f *.tsv *.bin *.aifc *.aiff *.table
