#ifndef _Z64AUDIO_SND_H_
#define _Z64AUDIO_SND_H_

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "types.h"
#include "macros.h"

#ifndef __Z64AUDIO_TERMINAL__
	#define WOW_GUI_IMPLEMENTATION
	#define WOW_IMPLEMENTATION
#include "../wowlib/wow_gui.h"
#else
	#define WOW_IMPLEMENTATION
#include "../wowlib/wow.h"
#endif

extern int tabledesign(int argc, char** argv, FILE* outstream);
extern int vadpcm_enc(int argc, char** argv);

static s8 STATE_DEBUG_PRINT;
static s8 STATE_FABULOUS;
static s8 gWaitAtExit = 0;
static z64audioState gAudioState = {
	.cleanState = {
		.aiff = false,
		.aifc = false,
		.table = true,
	},
	.ftype = NONE,
	.mode = Z64AUDIOMODE_UNSET,
};

#include "print.h"

/* returns the byte difference between two pointers */
intptr_t ptrDiff(const void* minuend, const void* subtrahend) {
	const u8* m = minuend;
	const u8* s = subtrahend;
	
	return m - s;
}

/* return u32 representation of float */
u32 hexFloat(float v) {
	/* TODO this might not work on platforms with different endianness */
	return *(u32*)&v;
}

int File_TestIfExists(const char* fn) {
	FILE* fp = fopen(fn, "rb");
	
	if (!fp)
		return 0;
	fclose(fp);
	
	return 1;
}

int FormatSampleNameExists(char* dst, const char* wav, const char* sample) {
	char* s;
	char* slash = dst;
	
	strcpy(dst, wav);
	
	/* get last slash in file path (if there is one) */
	if ((s = strrchr(dst, '/')) && s > slash)
		slash = s;
	if ((s = strrchr(dst, '\\')) && s > slash)
		slash = s;
	
	/* get the period at the end and ensure it is the extension */
	if ((s = strrchr(dst, '.')) && s < slash)
		PrintFail("Filename '%s' has no extension!", wav);
	
	/* make room for '.sample' before '.wav' */
	memmove(s + 1 /*'.'*/ + strlen(sample), s, strlen(s) + 1 /*'\0'*/);
	
	/* insert '.sample' */
	memcpy(s + 1 /*'.'*/, sample, strlen(sample));
	
	//fprintf(stderr, "'%s'\n", dst);
	
	return File_TestIfExists(dst);
}

char* GetPointer(void* _src, const char* fmt) {
	return strstr(_src, fmt);
}

void GetFilename(char* _src, char* _dest, char* _path) {
	char temporName[FILENAME_BUFFER] = { 0 };
	
	strcpy(temporName, _src);
	s32 sizeOfName = 0;
	s32 slashPoint = 0;
	
	s32 extensionPoint = 0;
	
	while (temporName[sizeOfName] != 0)
		sizeOfName++;
	
	while (temporName[sizeOfName - extensionPoint] != '.')
		extensionPoint++;
	
	for (s32 i = 0; i < sizeOfName; i++) {
		if (temporName[i] == '/' || temporName[i] == '\\')
			slashPoint = i;
	}
	
	if (slashPoint != 0)
		slashPoint++;
	else if (_path != NULL)
		_path[0] = 0;
	
	for (s32 i = 0; i <= sizeOfName - slashPoint - 1 - extensionPoint; i++) {
		_dest[i] = temporName[slashPoint + i];
		_dest[i + 1] = '\0';
	}
	
	if (_path != NULL)
		for (s32 i = 0; i < slashPoint; i++) {
			_path[i] = _src[i];
			_path[i + 1] = '\0';
		}
}

s8 File_FindAdditionalSamples(const char* prev, const char* secn, char* wav, char* wow, char* file[3], s8 f) {
	s8 fileCount = f;
	char fname[FILENAME_BUFFER];
	char fnameT[FILENAME_BUFFER];
	char path[FILENAME_BUFFER];
	
	if (strstr(wav, prev) || strstr(wav, secn)) {
		char* p = strstr(wav, prev);
		
		if (p == NULL) {
			p = strstr(wav, secn);
			if (p == NULL) {
				PrintWarning("Could not confirm filename. Processing only one file");
				
				return fileCount;
			}
		}
		
		DebugPrint("GetFilenames");
		GetFilename(wav, fname, path);
		GetFilename(wav, fnameT, path);
		
		for (s32 i = 0; i < strlen(fnameT); i++) {
			if (fnameT[i] == '.') {
				fnameT[i] = '\0';
				break;
			}
		}
		
		snprintf(fname, strlen(wav), "%s%s.wav", path, fnameT);
		DebugPrint("Looking for %s", fname);
		
		if (File_TestIfExists(fname) == 0) {
			PrintWarning("Could not find %s. Processing only one file", fname);
			
			return fileCount;
		}
		
		file[0] = strdup(fname);
		wav = strdup(fname);
	}
	
	/* secondary */
	if (FormatSampleNameExists(wow, wav, secn)) {
		++fileCount;
		file[1] = strdup(wow);
	}
	
	/* previous */
	if (FormatSampleNameExists(wow, wav, prev)) {
		++fileCount;
		file[2] = strdup(wow);
	}
	
	return fileCount;
}

s8 File_GetAndSort(char* wav, char* file[3]) {
	s8 fileCount = 1;
	char* wow = malloc(strlen(wav) + 32);
	char prevAlias[4][64] = {
		"previous\0"
		,"prev\0"
		, "low\0"
		, "lo\0"
	};
	char secnAlias[4][64] = {
		"secondary\0"
		, "sec\0"
		, "high\0"
		, "hi\0"
	};
	
	/* primary */
	file[0] = strdup(wav);
	for (s32 i = 0; i < 4; i++) {
		s8 f = fileCount;
		fileCount = File_FindAdditionalSamples(prevAlias[i], secnAlias[i], wav, wow, file, fileCount);
		if (fileCount > f) {
			break;
		}
	}
	
	/* TODO logic for handling 'previous' without 'secondary' */
	if (file[2] && !file[1])
		PrintFail("'%s' found but not '%s'", file[2], file[1]);
	
	free(wow);
	
	if (strstr(wav, ".wav")) {
		gAudioState.ftype = WAV;
		DebugPrint("Filetype: .wav");
		// gAudioState.cleanState.aifc = true; Commented out for now.
		// gAudioState.cleanState.aiff = true;
		// gAudioState.cleanState.table = true;
	} else if (strstr(wav, ".aiff")) {
		gAudioState.ftype = AIFF;
		DebugPrint("Filetype: .aiff");
		// gAudioState.cleanState.aifc = true; Commented out for now.
		// gAudioState.cleanState.aiff = false;
		// gAudioState.cleanState.table = true;
	} else if (strstr(wav, ".aifc")) {
		gAudioState.ftype = AIFC;
		gAudioState.cleanState.aifc = false;
		gAudioState.cleanState.aiff = false;
		gAudioState.cleanState.table = false;
		DebugPrint("Filetype: .aifc");
	}
	
	return fileCount;
}

void* SearchByteString(const void* haystack, size_t haystackSize, const void* needle, size_t needleSize) {
	if(haystack==NULL || needle==NULL)
		return NULL;
	register char* cur, * last;
	const char* cl = (const char*)haystack;
	const char* cs = (const char*)needle;
	
	/* we need something to compare */
	if (haystackSize == 0 || needleSize == 0)
		return NULL;
	
	/* "s" must be smaller or equal to "l" */
	if (haystackSize < needleSize)
		return NULL;
	
	/* special case where s_len == 1 */
	if (needleSize == 1)
		return memchr(haystack, (int)*cs, haystackSize);
	
	/* the last position where its possible to find "s" in "l" */
	last = (char*)cl + haystackSize - needleSize;
	
	for (cur = (char*)cl; cur <= last; cur++)
		if (cur[0] == cs[0] && memcmp(cur, cs, needleSize) == 0)
			return cur;
	
	return NULL;
}

static void showModes(void) {
	#define P(X) fprintf(stderr, X "\n")
	P("      1: wav to zzrtl instrument");
	P("      2: wav to aiff");
	#undef P
}

static void assertMode(z64audioMode mode) {
	if (mode == Z64AUDIOMODE_UNSET
	    || mode < 0
	    || mode >= Z64AUDIOMODE_LAST
	) {
		fprintf(stderr, "invalid mode %d; valid modes are as follows:\n", mode);
		showModes();
		exit(EXIT_FAILURE);
	}
}

static z64audioMode requestMode(void) {
	int c;
	
	fprintf(stderr, "select one of the following modes:\n");
	showModes();
	c = getchar();
	fflush(stdin);
	
	return 1 + (c - '1');
}

static void showArgs(void) {
	#define P(X) fprintf(stderr, X "\n")
	P("arguments:");
	P("  guided mode:");
	P("    z64audio \"input.wav\"");
	P("  command line mode:");
	P("    z64audio --wav \"input.wav\" --mode X");
	P("    where X is one of the following modes:");
	P("  extras:");
	P("    --tabledesign");
	P("    --vadpcm_enc");
	P("    example: z64audio --tabledesign -i 30 \"wow.aiff\" > \"wow.table\"");
	#ifdef _WIN32 /* helps users unfamiliar with command line */
		P("");
		P("Alternatively, Windows users can close this window and drop");
		P("a .wav file directly onto the z64audio executable. If you use");
		P("z64audio often, consider right-clicking a .wav, selecting");
		P("'Open With', and then z64audio.");
		fflush(stdin);
		getchar();
	#endif
	#undef P
	exit(EXIT_FAILURE);
}

static void shiftArgs(int* argc, char* argv[], int i) {
	int newArgc = *argc - i;
	int k;
	
	for (k = 0; i <= *argc; ++k, ++i)
		argv[k] = argv[i];
	*argc = newArgc;
}

void z64audioAtExit(void) {
	if (gWaitAtExit) {
		fflush(stdin);
		DebugPrint("Press ENTER to exit...");
		getchar();
	}
}

static inline void win32icon(void) {
	#ifdef _WIN32
	#include "../icon.h"
		{
			HWND win = GetActiveWindow();
			if( win ) {
				SendMessage(
					win,
					WM_SETICON,
					ICON_BIG,
					(LPARAM)LoadImage(
						GetModuleHandle(NULL),
						MAKEINTRESOURCE(IDI_ICON),
						IMAGE_ICON,
						32, //GetSystemMetrics(SM_CXSMICON)
						32, //GetSystemMetrics(SM_CXSMICON)
						0
					)
				);
				SendMessage(
					win,
					WM_SETICON,
					ICON_SMALL,
					(LPARAM)LoadImage(
						GetModuleHandle(NULL),
						MAKEINTRESOURCE(IDI_ICON),
						IMAGE_ICON,
						16, //GetSystemMetrics(SM_CXSMICON)
						16, //GetSystemMetrics(SM_CXSMICON)
						0
					)
				);
			}
		}
	#endif
}

#include "audio.h"

#endif