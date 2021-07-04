// Build in terminal for windows
// sudo chmod 777 -R ./* && x86_64-w64-mingw32-gcc -o z64snd z64snd.c
// sudo chmod 777 -R ./* && i686-w64-mingw32-gcc -Wno-pointer-to-int-cast -o z64snd z64snd.c

// sudo chmod 777 -R ./* && gcc z64snd.c -o z64snd

// REGEX
// ^([a-zA-Z0-9*_]+)\s+([a-zA-Z0-9_]+)(([()a-zA-Z0-9*_ ,]|\s)+)?.

#define __FLOAT80_SUCKS__

#include "include/z64snd.h"

int main(int argc, char** argv) {
	char fname[64] = { 0 };
	char path[128] = { 0 };
	char buffer[4] = { 0 };
	s32 inputId = 0;
	
	char* t[3];
	char* file[3];
	
	ALADPCMloop loopf[3] = { 0 };
	InstrumentChunk instf[3] = { 0 };
	
	if (system(buffer) == -1)
		PrintFail("Intro has failed.\n", 0);
	
	if (argc == 1)
		PrintFail("No Files provided\n");
	
	/* Get Filenames */
	for (s32 i = 1; i < argc; i++) {
		if (strstr(argv[i], ".wav")) {
			inputId++;
			file[i - 1] = t[i - 1] = argv[i];
		} else if (strstr(argv[i], ".WAV")) {
			inputId++;
			file[i - 1] = t[i - 1] = argv[i];
		} else
			PrintFail("No wav files\n");
	}
	
	if (argc > 2){
		/* Sort */
		for (s32 i = 0; i < inputId; i++) {
			if (strstr(t[i], "_1"))
				file[0] = t[i];
			else if (strstr(t[i], "_2"))
				file[1] = t[i];
			else if (strstr(t[i], "_3"))
				file[2] = t[i];
		}
	}
	
	for (s32 i = 0; i < inputId; i++) {
		Audio_Process(file[i], 0, &loopf[i], &instf[i]);
	}
	
	for (s32 i = 0; i < inputId; i++) {
		SetFilename(file[i], fname, path, NULL, NULL, NULL);
		Audio_Clean(path, fname);
	}
	
	return 1;
}