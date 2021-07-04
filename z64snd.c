// Build in terminal for windows
// sudo chmod 777 -R ./* && x86_64-w64-mingw32-gcc -o z64snd z64snd.c
// sudo chmod 777 -R ./* && i686-w64-mingw32-gcc -Wno-pointer-to-int-cast -o z64snd z64snd.c

// sudo chmod 777 -R ./* && gcc z64snd.c -o z64snd

// REGEX
// ^([a-zA-Z0-9*_]+)\s+([a-zA-Z0-9_]+)(([()a-zA-Z0-9*_ ,]|\s)+)?.

#define __FLOAT80_SUCKS__

#include "include/z64snd.h"

const char sExtension[] = {
	".wav"
};

int main(int argc, char** argv) {
	char fname[64] = { 0 };
	char path[128] = { 0 };
	char buffer[128] = { 0 };
	s8 procCount = 0;
	
	char* t[3];
	char* file[3];
	
	static ALADPCMloop loopf[3] = { 0 };
	static InstrumentChunk instf[3] = { 0 };
	
	STATE_DEBUG_PRINT = true;
	STATE_FABULOUS = false;
	
	if (system(buffer) == -1)
		PrintFail("Intro has failed.\n", 0);
	if (argc == 1)
		PrintFail("No Files provided\n");
	if ((procCount = File_GetAndSort(argv, argc, t, file)) <= 0)
		PrintFail("Something has gone terribly wrong... fileCount == %d", procCount);
	for (s32 i = 0; i < procCount; i++)
		Audio_Process(file[i], 0, &loopf[i], &instf[i]);
	
	for (s32 i = 0; i < procCount; i++) {
		SetFilename(file[i], fname, path, NULL, NULL, NULL);
		Audio_Clean(path, fname);
	}
	
	FILE* conf;
	
	if (procCount > 1) {
		char* p;
	
		snprintf(buffer, sizeof(buffer), "%s", fname);
	
		if ((p = strstr(buffer, "_1")) != 0) {
		} else if ((p = strstr(buffer, "_2")) != 0) {
		} else if ((p = strstr(buffer, "_3")) != 0) {
		}
	
		snprintf(p, sizeof(buffer), "_inst.tsv");
	} else {
		snprintf(buffer, sizeof(buffer), "%s_inst.tsv", fname);
	}
	
	conf = fopen(buffer, MODE_WRITE);
	
	fprintf(conf, "split1\tsplit2\tsplit3\trelease\tatkrate\tatklvl\tdcy1rt\tdcy1lvl\tdcy2rt\tdcy2lvl\n");
	
	switch (procCount) {
	    case 1: {
		    fprintf(conf, "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n");
	    } break;
	}
	
	fclose(conf);
	
	return 1;
}