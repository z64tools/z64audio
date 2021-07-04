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
	char buffer[4] = { 0 };
	s32 fileCount = 0;
	
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
			fileCount++;
			file[i - 1] = t[i - 1] = argv[i];
		} else
			PrintFail("No wav files\n");
	}
	
	if (File_TestIfExists(argv[1]) == 0) {
		PrintFail("File not found\n");
	}
	
	if (fileCount == 1) {
		u64 offset = 0;
		s32 foundNum = 0;
		s32 sizeOf = sizeof(argv[1]);
		char A[128] = { 0 };
		char B[128] = { 0 };
		s8 A_ID = 0;
		s8 B_ID = 0;
		s8 A_EX = 0;
		s8 B_EX = 0;
		
		if (strstr(argv[1], "_1"))
			foundNum = 1;
		else if (strstr(argv[1], "_2"))
			foundNum = 2;
		else if (strstr(argv[1], "_3"))
			foundNum = 3;
		
		if (foundNum) {
			DebugPrint("File has number at the end. Finding more samples...\n", 0);
			for (s32 i = 0; i < 3; i++) {
				if (i  != foundNum - 1) {
					if (A[0] == 0) {
						bcopy(argv[1], A, sizeof(argv[1]));
						snprintf(&A[sizeOf], 12, "%d.wav\0", i + 1);
						A_ID = i + 1;
					} else if (B[0] == 0) {
						bcopy(argv[1], B, sizeof(argv[1]));
						snprintf(&B[sizeOf], 12, "%d.wav\0", i + 1);
						B_ID = i + 1;
					}
				}
			}
			if ((A_EX = File_TestIfExists(A)) != 0) {
				fileCount++;
			}
			if ((B_EX = File_TestIfExists(B)) != 0) {
				fileCount++;
			}
			
			if ((A_ID < foundNum && A_EX == false) || (B_ID < foundNum && B_EX == false)) {
				PrintFail("Could not find file with smaller ID\n");
			} else {
				t[1] = A_EX ? A : 0;
				t[2] = B_EX ? B : 0;
			}
			DebugPrint("More samples found!\n", 0);
		}
	}
	
	if (fileCount > 1) {
		/* Sort */
		for (s32 i = 0; i < fileCount; i++) {
			if (strstr(t[i], "_1"))
				file[0] = t[i];
			else if (strstr(t[i], "_2"))
				file[1] = t[i];
			else if (strstr(t[i], "_3"))
				file[2] = t[i];
		}
	}
	
	for (s32 i = 0; i < fileCount; i++) {
		Audio_Process(file[i], 0, &loopf[i], &instf[i]);
	}
	
	for (s32 i = 0; i < fileCount; i++) {
		SetFilename(file[i], fname, path, NULL, NULL, NULL);
		Audio_Clean(path, fname);
	}
	
	return 1;
}