#ifndef _Z64AUDIO_SND_H_
#define _Z64AUDIO_SND_H_

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include "types.h"
#include "macros.h"
#define WOW_IMPLEMENTATION
#include <wow.h>

extern int tabledesign(int argc, char** argv, FILE* outstream);
extern int vadpcm_enc(int argc, char** argv);

static s8 STATE_DEBUG_PRINT;
static s8 STATE_FABULOUS;
static z64audioState gAudioState = {
	.cleanState = {
		.aiff = false,
		.aifc = false,
		.table = true,
	},
	.ftype = NONE,
	.mode = Z64AUDIOMODE_UNSET,
	.instDataFlag = { 0, 0, 0 },
	.sampleRate = { 0, 0, 0 },
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

void GetFilename(char* _src, char* _dest, char* _path, s32* sizeStore) {
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
	else
		_path[0] = 0;
	
	for (s32 i = 0; i <= sizeOfName - slashPoint - 1 - extensionPoint; i++) {
		_dest[i] = temporName[slashPoint + i];
		*sizeStore = i;
		_dest[i + 1] = '\0';
	}
	
	for (s32 i = 0; i < slashPoint; i++) {
		_path[i] = _src[i];
		_path[i + 1] = '\0';
	}
	
	*sizeStore += 1;
}

void SetFilename(char* argv, char* fname, char* path, char* fname_AIFF, char* fname_AIFC, char* fname_TABLE) {
	s32 fnameSize = 0;
	
	if (!argv || !fname || !path)
		PrintFail("SetFilename line %d", __LINE__);
	
	GetFilename(argv, fname, path, &fnameSize);
	
	if (fname_AIFF == 0 && fname_AIFC == 0 && fname_TABLE == 0)
		return;
	
#define DO_FNAME(WHICH, EXTENSION) \
	if (WHICH) {                    \
		strcpy(WHICH, path);         \
		strcat(WHICH, fname);        \
		strcat(WHICH, EXTENSION);    \
	}
	DO_FNAME(fname_AIFF,  ".aiff" )
	DO_FNAME(fname_AIFC,  ".aifc" )
	DO_FNAME(fname_TABLE, ".table")
#undef DO_FNAME
}

s8 File_GetAndSort(char* wav, char** file) {
	s8 fileCount = 1;
	char* wow = malloc(strlen(wav) + 32);
	char fname[FILENAME_BUFFER];
	char fnameT[FILENAME_BUFFER];
	char path[FILENAME_BUFFER];
	s32 temp = 0;
	
	/* primary */
	file[0] = strdup(wav);
	
	if (strstr(wav, ".previous") || strstr(wav, ".secondary")) {
		char* p = strstr(wav, ".previous");
		
		if (p == NULL && (*p = strstr(wav, ".secondary") == NULL)) {
			PrintWarning("Could not confirm filename. Processing only one file");
			
			return fileCount;
		}
		
		GetFilename(wav, fname, path, &temp);
		GetFilename(wav, fnameT, path, &temp);
		
		fnameT[ptrDiff(p, wav) - strlen(path)] = '\0';
		
		snprintf(fname, 5 + strlen(fname), "%s.wav", fnameT);
		DebugPrint("Looking for %s", fname);
		
		if (File_TestIfExists(fname) == 0) {
			PrintWarning("Could not find %s. Processing only one file", fname);
			
			return fileCount;
		}
		
		file[0] = strdup(fname);
		wav = strdup(fname);
	}
	
	/* secondary */
	if (FormatSampleNameExists(wow, wav, "secondary")) {
		++fileCount;
		file[1] = strdup(wow);
		//fprintf(stderr, "secondary = '%s'\n", wow);
	}
	
	/* previous */
	if (FormatSampleNameExists(wow, wav, "previous")) {
		++fileCount;
		file[2] = strdup(wow);
		//fprintf(stderr, "previous = '%s'\n", wow);
	}
	
	/* TODO logic for handling 'previous' without 'secondary' */
	if (file[2] && !file[1])
		PrintFail("'%s' found but not '%s'", file[2], file[1]);
	
	free(wow);
	
	if (strstr(wav, ".wav")) {
		gAudioState.ftype = WAV;
		DebugPrint("Filetype: .wav");
	}else if (strstr(wav, ".aiff")) {
		gAudioState.ftype = AIFF;
		DebugPrint("Filetype: .aiff");
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

#include "audio.h"

#endif