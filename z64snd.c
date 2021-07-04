// Build in terminal for windows
// sudo chmod 777 -R ./* && x86_64-w64-mingw32-gcc -o z64snd z64snd.c
// sudo chmod 777 -R ./* && i686-w64-mingw32-gcc -Wno-pointer-to-int-cast -o z64snd z64snd.c

// sudo chmod 777 -R ./* && gcc z64snd.c -o z64snd

// REGEX
// ^([a-zA-Z0-9*_]+)\s+([a-zA-Z0-9_]+)(([()a-zA-Z0-9*_ ,]|\s)+)?.

#define __FLOAT80_SUCKS__

#include "include/z64snd.h"

int main(int argc, char** argv) {
	char fname[64];
	char path[128];
	char buffer[1024 * 4] = { 0 };
	ALADPCMloop loopf1 = { 0 };
	InstrumentChunk instf1 = { 0 };
	int opt;
	
	if (system(buffer) == -1)
		PrintFail("Intro has failed.\n", 0);
	
	Audio_Process(argv[1], 0, &loopf1, &instf1);
	SetFilename(argv[1], fname, path, NULL, NULL, NULL);
	Audio_Clean(path, fname);
	
	printf("%X\n", loopf1.start);
	printf("%X\n", instf1.baseNote);
	
	return 1;
}