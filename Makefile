GCC = gcc
GPP = g++
LD = ld
MAGIC = -Os
ADPCM = tools/sdk-tools/adpcm/

.PHONY: objects clean

default: z64audio

objects:
	@$(GPP) -std=c++11 -c $(MAGIC) -DNDEBUG tools/audiofile/audiofile.cpp -Itools/audiofile
	@$(GCC) -c tools/sdk-tools/tabledesign/*.c -s $(MAGIC) -flto -DNDEBUG -Itools/audiofile
	@$(GCC) -c $(ADPCM)quant.c $(ADPCM)sampleio.c $(ADPCM)util.c $(ADPCM)vpredictor.c $(ADPCM)vadpcm_enc.c $(ADPCM)vencode.c -s $(MAGIC) -flto -DNDEBUG -Itools/audiofile
	@mkdir -p bin/o/linux
	@mv *.o bin/o/linux

z64audio: z64snd.c include/z64snd.h
	@$(GCC) -c z64snd.c -Wall
	@$(GPP) -o z64audio.exe *.o bin/o/linux/*.o $(MAGIC) -s -flto -DNDEBUG
	@rm *.o

clean:
	@rm *.tsv *.bin
