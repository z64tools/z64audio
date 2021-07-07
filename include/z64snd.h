#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <stdint.h>

#include "vadpcm.h"
#include "wave.h"

#ifndef __FLOAT80_SUCKS__
 #define __float80 float
#endif

enum {
	false,
	true
};

typedef enum {
	NONE = 0
	, WAV
	, AIFF
} FileExten;

typedef enum  {
	Z64AUDIOMODE_UNSET = 0
	, Z64AUDIOMODE_WAV_TO_ZZRTL
	, Z64AUDIOMODE_WAV_TO_AIFF
	, Z64AUDIOMODE_LAST
} z64audioMode;

typedef struct {
	s8        aiff;
	s8        aifc;
	s8        table;
	FileExten ftype;
	z64audioMode mode;
    s8 instDataFlag[3];
} z64audioState;

static z64audioState gAudioState = {
	.aiff = false,
	.aifc = false,
	.table = true,
	.ftype = NONE,
	.mode = Z64AUDIOMODE_UNSET,
    .instDataFlag = { 0, 0, 0 },
};

const char sHeaders[3][6] = {
	{ ".aiff" },
	{ ".table" },
	{ ".aifc" }
};

extern int tabledesign(int argc, char** argv, FILE* outstream);
extern int vadpcm_enc(int argc, char** argv);
static s8 STATE_DEBUG_PRINT;
static s8 STATE_FABULOUS;
#define BSWAP16(x) x = __builtin_bswap16(x);
#define BSWAP32(x) x = __builtin_bswap32(x);

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

void PrintFail(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
void PrintWarning(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
void DebugPrint(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));

void PrintFail(const char* fmt, ...) {
	va_list args;
	
	va_start(args, fmt);
	printf(
		"\7\e[0;91m[<]: "
		"Error: \e[m"
	);
	vprintf(
		fmt,
		args
	);
	va_end(args);
	exit(EXIT_FAILURE);
}

void PrintWarning(const char* fmt, ...) {
	va_list args;
	
	va_start(args, fmt);
	printf(
		"\7\e[0;93m[<]: "
		"Warning: \e[m"
	);
	vprintf(
		fmt,
		args
	);
	va_end(args);
}

void DebugPrint(const char* fmt, ...) {
	if (!STATE_DEBUG_PRINT)
		return;
	const char colors[][64] = {
		"\e[0;31m[<]: \e[m\0",
		"\e[0;33m[<]: \e[m\0",
		"\e[0;32m[<]: \e[m\0",
		"\e[0;36m[<]: \e[m\0",
		"\e[0;34m[<]: \e[m\0",
		"\e[0;35m[<]: \e[m\0",
	};
	static s8 i = 0;
	
	if (STATE_FABULOUS)
		i += i < 5 ? 1 : -5;
	
	va_list args;
	
	va_start(args, fmt);
	if (STATE_FABULOUS)
		printf(colors[i]);
	else
		printf(colors[3]);
	vprintf(
		fmt,
		args
	);
	va_end(args);
}

s16 Audio_Downsample(s32 wow) {
	// s32 sign = (wow > 0) ? 1 : -1;
	// u32 ok = (wow > 0) ? wow : -wow;
	// s16 result = ok & 0xFFFF;
	//
	// result *= sign;
	
	long double temp = wow;
	
	temp *= 0.00003014885;
	
	return (s16)temp;
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
		PrintFail("Filename '%s' has no extension!\n", wav);
	
	/* make room for '.sample' before '.wav' */
	memmove(s + 1 /*'.'*/ + strlen(sample), s, strlen(s) + 1 /*'\0'*/);
	
	/* insert '.sample' */
	memcpy(s + 1 /*'.'*/, sample, strlen(sample));
	
	//fprintf(stderr, "'%s'\n", dst);
	
	return File_TestIfExists(dst);
}

void GetFilename(char* _src, char* _dest, char* _path, s32* sizeStore) {
	char temporName[1024] = { 0 };
	
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
	
	GetFilename(argv, fname, path, &fnameSize);
	
	if (fname_AIFF == 0 && fname_AIFC == 0 && fname_TABLE == 0)
		return;
	
	/* Get Filenames to nameVars */
	for (s32 i = 0; i < fnameSize; i++) {
		if (fname_AIFF != NULL)
			fname_AIFF[i] = fname[i];
		
		if (fname_TABLE != NULL)
			fname_TABLE[i] = fname[i];
		
		if (fname_AIFC != NULL)
			fname_AIFC[i] = fname[i];
		
		if (i == fnameSize - 1) {
			i++;
			for (s32 j = 0; j < 6; j++) {
				if (fname_AIFF != NULL)
					fname_AIFF[i + j] = sHeaders[0][j];
				
				if (fname_TABLE != NULL)
					fname_TABLE[i + j] = sHeaders[1][j];
				
				if (fname_AIFC != NULL)
					fname_AIFC[i + j] = sHeaders[2][j];
			}
		}
	}
}

s8 File_GetAndSort(char* wav, char** file) {
	s8 fileCount = 1;
	char* wow = malloc(strlen(wav) + 32);
	char fname[128];
	char fnameT[128];
	char path[128];
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
		DebugPrint("Looking for %s\n", fname);
		
		if (File_TestIfExists(fname) == 0) {
			PrintWarning("Could not find %s. Processing only one file\n", fname);
			
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
		PrintFail("'%s' found but not '%s'\n", file[2], file[1]);
	
	free(wow);
	
	if (strstr(wav, ".wav")) {
		gAudioState.ftype = WAV;
		DebugPrint("Filetype: .wav\n");
	}else if (strstr(wav, ".aiff")) {
		gAudioState.ftype = AIFF;
		DebugPrint("Filetype: .aiff\n");
	}
	
	return fileCount;
}

void Audio_Clean(char* file) {
	char buffer[1024] = { 0 };
	char fname[128] = { 0 };
	char path[128] = { 0 };
	s32 siz = 0;
	char state[2][8] = {
		"FALSE\0",
		"TRUE\0"
	};
	
	GetFilename(file, fname, path, &siz);
	
	DebugPrint("Cleaning:\n\t%s.aifc\t%s\n\t%s.aiff\t%s\n\t%s.table\t%s\n", fname, state[gAudioState.aifc], fname, state[gAudioState.aiff], fname, state[gAudioState.table]);
	
	if (fname[0] == 0) {
		PrintWarning("Failed to get filename for cleaning.");
		
		return;
	}
	
	if (gAudioState.aiff == true) {
		snprintf(buffer, sizeof(buffer), "%s%s.aiff", path, fname);
		if (remove(buffer))
			PrintWarning("%s could not be removed\n", buffer);
		else
			DebugPrint("%s removed succesfully\n", buffer);
	}
	
	if (gAudioState.aifc == true) {
		snprintf(buffer, sizeof(buffer), "%s%s.aifc", path, fname);
		if (remove(buffer))
			PrintWarning("%s could not be removed\n", buffer);
		else
			DebugPrint("%s removed succesfully\n", buffer);
	}
	
	if (gAudioState.table == true) {
		snprintf(buffer, sizeof(buffer), "%s%s.table", path, fname);
		if (remove(buffer))
			PrintWarning("%s could not be removed\n", buffer);
		else
			DebugPrint("%s removed succesfully\n", buffer);
	}
}

void Audio_Convert_WavToAiff(char* fileInput, u32* _sampleRate, s32 iMain) {
	FILE* f;
	WaveHeader* wav = 0;
	FILE* o;
	s32 bitProcess = 0;
	s32 channelProcess = 0;
	ChunkHeader chunkHeader;
	WaveChunk* wavDataChunk;
	InstrumentHeader* waveInst;
	SampleHeader* waveSmpl;
	s16* data;
	char fname[128] = { 0 };
	char fname_AIFF[128] = { 0 };
	char path[128] = { 0 };
	
	SetFilename(fileInput, fname, path, fname_AIFF, NULL, NULL);
	
	/* FILE LOAD */ {
		WaveHeader wave;
        u32 fsize = 0;
		f = fopen(fileInput, MODE_READ);
		fread(&wave, sizeof(WaveHeader), 1, f);
		fclose(f);
		
		f = fopen(fileInput, MODE_READ);
		wav = malloc(wave.chunk.size + 8);
		if (wav == NULL)
			PrintFail("Malloc Fail\n");
		fread(wav, wave.chunk.size + 8, 1, f);
        
        rewind(f);
        fseek(f, 0L, SEEK_END);
        fsize = ftell(f);
        
		fclose(f);
		
		wavDataChunk = (WaveChunk*)wav;
		
		while (!(wavDataChunk->ID[0] == 0x64 && wavDataChunk->ID[1] == 0x61 && wavDataChunk->ID[2] == 0x74 && wavDataChunk->ID[3] == 0x61)) {
			char* s = (char*)wavDataChunk;
			s += 1;
			wavDataChunk = (WaveChunk*)s;
			
			if ((u8*)wavDataChunk > (u8*)wav + 8 + wav->chunk.size) {
				DebugPrint("%X / %X\n", (unsigned)ptrDiff(wavDataChunk, wav), 8 + wav->chunk.size);
				PrintFail("SampleChunk: Out of range. Better stop before everything goes bonkers.\n");
			}
		}
		
		data = ((WaveData*)wavDataChunk)->data;
        
		DebugPrint("ExtraData comparison result: %d > %d\n", fsize - ((intptr_t)data - (intptr_t)wav), wavDataChunk->size);
		if (fsize - ((intptr_t)data - (intptr_t)wav) > wavDataChunk->size) {
			gAudioState.instDataFlag[iMain] = true;
        }
	}
	
	if ((o = fopen(fname_AIFF, MODE_WRITE)) == NULL) {
		PrintFail("Output file [%s] could not be opened.\n", fname_AIFF);
	}
	
	if (gAudioState.instDataFlag[iMain]) {
		waveSmpl = (SampleHeader*)((char*)data + wavDataChunk->size);
		while (!(waveSmpl->chunk.ID[0] == 0x73 && waveSmpl->chunk.ID[1] == 0x6D && waveSmpl->chunk.ID[2] == 0x70 && waveSmpl->chunk.ID[3] == 0x6C)) {
			char* s = (char*)waveSmpl;
			s += 1;
			waveSmpl = (SampleHeader*)s;
			
			if ((u8*)waveSmpl > ((u8*)wav) + 8 + wav->chunk.size)
				PrintFail("SampleInfo: Out of range. Better stop before everything goes bonkers.\n");
		}
		
		waveInst = (InstrumentHeader*)((char*)data + wavDataChunk->size);
		while (!(waveInst->chunk.ID[0] == 0x69 && waveInst->chunk.ID[1] == 0x6E && waveInst->chunk.ID[2] == 0x73 && waveInst->chunk.ID[3] == 0x74)) {
			char* s = (char*)waveInst;
			s += 1;
			waveInst = (InstrumentHeader*)s;
			
			if ((u8*)waveInst > ((u8*)wav) + 8 + wav->chunk.size)
				PrintFail("InstrumentInfo: Out of range. Better stop before everything goes bonkers.\n");
		}
	}
	
	DebugPrint("BitRate\t\t\t%d-bit.\n", wav->bitsPerSamp);
	
	if (wav->bitsPerSamp == 32) {
		bitProcess = true;
	}
	
	if (wav->bitsPerSamp != 16 && wav->bitsPerSamp != 32) {
		DebugPrint("Warning: Non supported bit depth. Try 16-bit or 32-bit.\n");
	}
	
	DebugPrint("Channels\t\t\t%d\n", wav->numChannels);
	if (wav->numChannels > 1) {
		channelProcess = true;
		PrintWarning("Warning: Provided WAV is not MONO.\n");
	}
	
	/* FORM CHUNK */ {
		Chunk formChunk = {
			.ckID = 0x464F524D,    // FORM
			.ckSize = sizeof(Chunk) + sizeof(ChunkHeader) + sizeof(CommonChunk) + sizeof(ChunkHeader),
			.formType = 0x41494646 // AIFF
		};
		u32 chunkSize = wavDataChunk->size;
		chunkSize *= channelProcess == true ? 0.5 : 1;
		chunkSize *= bitProcess == true ? 0.5 : 1;
		
		formChunk.ckSize += chunkSize;
		
		if (gAudioState.instDataFlag[iMain])
			formChunk.ckSize += sizeof(ChunkHeader) + sizeof(Marker) * 2 + sizeof(u16) + sizeof(ChunkHeader) + sizeof(InstrumentChunk);
		
		BSWAP32(formChunk.ckID);
		BSWAP32(formChunk.ckSize);
		BSWAP32(formChunk.formType);
		fwrite(&formChunk, sizeof(Chunk), 1, o);
	}
	DebugPrint("FormChunk\t\t\tDone\n");
	
	/* COMMON CHUNK */ {
		chunkHeader = (ChunkHeader) {
			.ckID = 0x434F4D4D, // COMM
			.ckSize = sizeof(CommonChunk)
		};
		BSWAP32(chunkHeader.ckID);
		BSWAP32(chunkHeader.ckSize);
		fwrite(&chunkHeader, sizeof(ChunkHeader), 1, o);
		
		CommonChunk commonChunk = {
			.numChannels = 1,
			.numFramesH = ((wavDataChunk->size & 0xFFFF0000) >> 8) * 0.5,
			.numFramesL = (wavDataChunk->size & 0x0000FFFF) * 0.5,
			.sampleSize = 16,
			.compressionTypeH = 0x4e4f,
			.compressionTypeL = 0x4e45,
		};
		
		u32* numFrames = (u32*)&commonChunk.numFramesH;
		*numFrames = wavDataChunk->size / 2;
		
		if (channelProcess) {
			*numFrames *= 0.5;
		}
		if (bitProcess) {
			*numFrames *= 0.5;
		}
		
		BSWAP32(*numFrames);
		
		*_sampleRate = wav->sampleRate;
		__float80 s = wav->sampleRate;
		u8* wow = (void*)&s;
		for (s32 i = 0; i < 10; i++)
			commonChunk.sampleRate[i] = wow[ 9 - i];
		BSWAP16(commonChunk.numChannels);
		BSWAP16(commonChunk.sampleSize);
		BSWAP16(commonChunk.compressionTypeH);
		BSWAP16(commonChunk.compressionTypeL);
		fwrite(&commonChunk, sizeof(CommonChunk), 1, o);
	}
	DebugPrint("CommonChunk\t\tDone\n");
	
	if (gAudioState.instDataFlag[iMain]) {      /* MARK && INST CHUNK */
		chunkHeader = (ChunkHeader) {
			.ckID = 0x4d41524b, // MARK
			.ckSize = sizeof(Marker) * 2 + sizeof(u16)
		};
		BSWAP32(chunkHeader.ckID);
		BSWAP32(chunkHeader.ckSize);
		fwrite(&chunkHeader, sizeof(ChunkHeader), 1, o);
		
		u16 numMarkers = 0x0200;
		
		fwrite(&numMarkers, sizeof(u16), 1, o);
		
		Marker marker[] = {
			{
				.MarkerID = 1,
				.positionH = (waveSmpl->loopData[0].start & 0xFFFF0000) >> 8,
				.positionL = (waveSmpl->loopData[0].start & 0xFFFF),
			},
			{
				.MarkerID = 2,
				.positionH = (waveSmpl->loopData[0].end & 0xFFFF0000) >> 8,
				.positionL = (waveSmpl->loopData[0].end & 0xFFFF),
			},
		};
		
		if(waveSmpl->numSampleLoops == 0) {
			marker[0].positionH = marker[0].positionL = 0;
		}
		;
		
		BSWAP16(marker[0].MarkerID);
		BSWAP16(marker[0].positionL);
		BSWAP16(marker[1].MarkerID);
		BSWAP16(marker[1].positionL);
		fwrite(&marker, sizeof(Marker) * 2, 1, o);
		
		chunkHeader = (ChunkHeader) {
			.ckID = 0x494e5354, // INST
			.ckSize = sizeof(InstrumentChunk)
		};
		BSWAP32(chunkHeader.ckID);
		BSWAP32(chunkHeader.ckSize);
		fwrite(&chunkHeader, sizeof(ChunkHeader), 1, o);
		
		InstrumentChunk instChunk = {
			.baseNote = waveInst->note,
			.detune = waveInst->fineTune,
			.lowNote = waveInst->lowNote,
			.highNote = waveInst->hiNote,
			.gain = waveInst->gain,
			.lowVelocity = waveInst->lowVel,
			.highVelocity = waveInst->hiVel,
			.sustainLoop = {
				.playMode = 1,
				.beginLoop = 1,
				.endLoop = 2
			},
			.releaseLoop = {
				.playMode = 0
			}
		};
		
		if (waveSmpl->numSampleLoops == 0) {
			instChunk.sustainLoop.playMode = 0;
			instChunk.releaseLoop.playMode = 0;
		}
		
		BSWAP16(instChunk.sustainLoop.playMode);
		BSWAP16(instChunk.sustainLoop.beginLoop);
		BSWAP16(instChunk.sustainLoop.endLoop);
		fwrite(&instChunk, sizeof(InstrumentChunk), 1, o);
		DebugPrint("ExtraChunk\t\t\tDone\n");
	}
	
	/* SOUND CHUNK */ {
		chunkHeader = (ChunkHeader) {
			.ckID = 0x53534E44, // SSND
			.ckSize = wavDataChunk->size
		};
		
		if (channelProcess)
			chunkHeader.ckSize *= 0.5;
		if (bitProcess)
			chunkHeader.ckSize *= 0.5;
		
		chunkHeader.ckSize += 8;
		
		BSWAP32(chunkHeader.ckID);
		BSWAP32(chunkHeader.ckSize);
		fwrite(&chunkHeader, sizeof(ChunkHeader), 1, o);
		
		u64 temp = 0;
		
		fwrite(&temp, sizeof(u64), 1, o);
		
		if (!bitProcess) {
			for (s32 i = 0; i < wavDataChunk->size / 2; i++) {
				s16 tempData = ((s16*)data)[i];
				s16 medianator;
				
				if (channelProcess) {
					medianator = ((s16*)data)[i + 1];
					tempData = ((float)tempData + (float)medianator) * 0.5;
				}
				
				BSWAP16(tempData);
				fwrite(&tempData, sizeof(s16), 1, o);
				
				if (channelProcess)
					i += 1;
			}
		} else {
			for (s32 i = 0; i < wavDataChunk->size / 2 / 2; i++) {
				float* f32 = (float*)data;
				s16 wow = f32[i] * 32766;
				
				if (channelProcess) {
					s16 wow_2 = f32[i + 1] * 32766;
					wow = ((float)wow + (float)wow_2) * 0.5;
					
					i += 1;
				}
				
				BSWAP16(wow);
				
				fwrite(&wow, sizeof(s16), 1, o);
			}
		}
	}
	DebugPrint("SoundChunk\t\t\tDone\n");
	DebugPrint("WAV to AIFF conversion\tOK\n\n");
	fclose(o);
	if (wav)
		free(wav);
}

void Audio_Process(char* argv, s32 iMain, ALADPCMloop* _destLoop, InstrumentChunk* _destInst, CommonChunk* _destComm, u32* _sr) {
	char fname[64] = { 0 };
	char path[128] = { 0 };
	char fname_AIFF[64] = { 0 };
	char fname_TABLE[64] = { 0 };
	char fname_AIFC[64] = { 0 };
	char buffer[1024];
	
	SetFilename(argv,fname,path,fname_AIFF,fname_AIFC,fname_TABLE);
	DebugPrint("Name: %s\n", fname);
	DebugPrint("Path: %s\n\n", path);
	
	if (gAudioState.ftype == WAV)
		Audio_Convert_WavToAiff(argv, _sr, iMain);
	
	if (gAudioState.mode == Z64AUDIOMODE_WAV_TO_AIFF) {
		if (File_TestIfExists(fname_AIFF))
			exit(EXIT_SUCCESS);
		else {
			PrintFail("%s does not exist. Something has gone terribly wrong.\n", fname_AIFF);
		}
	}
	
	/* run tabledesign */
	{
#ifdef Z64AUDIO_EXTERNAL_DEPENDENCIES
    	snprintf(buffer, sizeof(buffer), "tabledesign -i 30 \"%s%s\" > \"%s%s\"", path, fname_AIFF, path, fname_TABLE);
    	if (system(buffer))
    		PrintFail("tabledesign has failed...\n");
#else
    	strcpy(buffer, path); strcat(buffer, fname_TABLE);
    	FILE* fp = fopen(buffer, "w");
    	char* argv[] = {
    		/*0*/ strdup("tabledesign")
    		/*1*/, strdup("-i")
    		/*2*/, strdup("30")
    		/*3*/, buffer
    		/*4*/, 0
    	};
    	snprintf(buffer, sizeof(buffer), "%s%s", path, fname_AIFF);
    	if (tabledesign(4, argv, fp))
    		PrintFail("tabledesign has failed...\n");
    	DebugPrint("%s generated succesfully\n", fname_TABLE);
    	fclose(fp);
    	free(argv[0]);
    	free(argv[1]);
    	free(argv[2]);
#endif
	}
	
	/* run vadpcm_enc */
	{
#ifdef Z64AUDIO_EXTERNAL_DEPENDENCIES
		snprintf(buffer, sizeof(buffer), "vadpcm_enc -c \"%s%s\" \"%s%s\" \"%s%s\"", path, fname_TABLE, path, fname_AIFF, path, fname_AIFC);
		if (system(buffer))
			PrintFail("vadpcm_enc has failed...\n");
#else
		int i;
		int pathMax = 1024;
		char* argv[] = {
			/*0*/ strdup("vadpcm_enc")
			/*1*/, strdup("-c")
			/*2*/, malloc(pathMax)
			/*3*/, malloc(pathMax)
			/*4*/, malloc(pathMax)
			/*5*/, 0
		};
		snprintf(argv[2], pathMax, "%s%s", path, fname_TABLE);
		snprintf(argv[3], pathMax, "%s%s", path, fname_AIFF);
		snprintf(argv[4], pathMax, "%s%s", path, fname_AIFC);
		if (vadpcm_enc(5, argv))
			PrintFail("vadpcm_enc has failed...\n");
		DebugPrint("%s converted to %s succesfully\n", fname_AIFF, fname_AIFC);
		for (i = 0; argv[i]; ++i)
			free(argv[i]);
#endif
	}
	
	FILE* f;
	FILE* pred;
	FILE* samp;
	FILE* conf;
	char* adpcm = 0;
	char* p = 0;
	s32 allocSize;
	
	f = fopen(fname_AIFC, "rb");
	fseek(f, 0, SEEK_END);
	allocSize = ftell(f);
	rewind(f);
	
	p = adpcm = malloc(allocSize);
	if (adpcm == 0)
		PrintFail("malloc fail\n");
	fread(adpcm, allocSize, 1, f);
	fclose(f);
	
	/* PREDICTOR */
	snprintf(buffer, sizeof(buffer), "%s%s.predictors.bin", path,fname);
	pred = fopen(buffer, MODE_WRITE);
	p += 8;
	while (!(p[-7] == 'C' && p[-6] == 'O' && p[-5] == 'D' && p[-4] == 'E' && p[-3] == 'S')) {
		p++;
		if (ptrDiff(p, adpcm) > allocSize) {
			PrintFail("Out of bounds while searchin:\t'CODES'\n");
		}
	}
	
	s8 head[] = { 0, 0, 0, p[1], 0, 0, 0, p[3] };
	
	fwrite(&head, sizeof(head), 1, pred);
	p += 4;
	
	while (!(p[0] == 'S' && p[1] == 'S' && p[2] == 'N' && p[3] == 'D')) {
		fwrite(p, 1, 1, pred);
		p++;
		if (ptrDiff(p, adpcm) > allocSize) {
			PrintFail("Out of bounds while searchin:\t'SSND'\n");
		}
	}
	
	fclose(pred);
	
	/* SAMPLE */
	snprintf(buffer, sizeof(buffer), "%s%s.sample.bin", path,fname);
	samp = fopen(buffer, MODE_WRITE);
	p += 4;
	
	u32 ssndSize = ((u32*)p)[0];
	
	BSWAP32(ssndSize);
	p += 0xC;
	
	for (s32 i = 0; i < ssndSize - 8; i++) {
		fwrite(p, 1, 1, samp);
		p++;
	}
	
	fclose(samp);
	
	if (STATE_DEBUG_PRINT)
		printf("\n");
	DebugPrint("%s.predictors.bin\tOK\n", fname);
	DebugPrint("%s.sample.bin\tOK\n", fname);
	
	ALADPCMloop* loopInfo = 0;
	InstrumentChunk* instData;
	CommonChunk* comm = (CommonChunk*)(adpcm + 0x14);
	int lflag = true;
	
	memcpy(_destComm, comm, sizeof(CommonChunk));
	
	snprintf(buffer, sizeof(buffer), "%s%s.config.tsv", path, fname);
	conf = fopen(buffer, MODE_WRITE);
	
	while (!(p[-8] == 'O' && p[-7] == 'O' && p[-6] == 'P' && p[-5] == 'S')) {
		p++;
		if (ptrDiff(p, adpcm) > allocSize) {
			lflag = false;
			break;
		}
	}
	
	if (lflag) {
		loopInfo = (ALADPCMloop*)p;
		memcpy(_destLoop, loopInfo, sizeof(ALADPCMloop));
		BSWAP32(loopInfo->start);
		BSWAP32(loopInfo->end);
		BSWAP32(loopInfo->count);
		
		snprintf(buffer, sizeof(buffer), "%s.looppredictors.bin", fname);
		
		FILE* fLoopPred = fopen(buffer, MODE_WRITE);
		
		for (s32 i = 0; i < 16; i++)
			fwrite(&loopInfo->state[i], sizeof(s16), 1, fLoopPred);
		
		fclose(fLoopPred);
	}
	
	p = adpcm + 8;
	while (!(p[-8] == 'I' && p[-7] == 'N' && p[-6] == 'S' && p[-5] == 'T')) {
		p++;
		if (ptrDiff(p, adpcm) > allocSize) {
			PrintFail("Out of bounds while searchin:\t'INST'\n");
		}
	}
	
	instData = (InstrumentChunk*)p;
	memcpy(_destInst, instData, sizeof(InstrumentChunk));
	
	u32 numFrame = 0;
	u16 numFrames[2] = {
		comm->numFramesH,
		comm->numFramesL
	};
	
	BSWAP16(numFrames[0]);
	BSWAP16(numFrames[1]);
	
	numFrame = (numFrames[0] << 16) | numFrames[1];
	
	fprintf(conf, "precision\tloopstart\tloopend  \tloopcount\tlooptail \n%08X\t", 0);
	if (lflag && loopInfo) {
		/* Found Loop */
		fprintf(conf, "%08X\t", loopInfo->start);
		fprintf(conf, "%08X\t", loopInfo->end);
		fprintf(conf, "%08X\t", loopInfo->count);
		if (numFrame <= loopInfo->end)
			numFrame = 0;
		fprintf(conf, "%08X\n", numFrame);
	} else {
		fprintf(conf, "%08X\t", 0);
		fprintf(conf, "%08X\t", numFrame);
		fprintf(conf, "%08X\t", 0);
		fprintf(conf, "%08X\n", 0);
	}
	
	DebugPrint("%s.config.tsv\tOK\n\n", fname);
	
	fclose(conf);
	if (adpcm)
		free(adpcm);
}

void Audio_GenerateInstrumentConf(char* file, s32 fileCount, InstrumentChunk* instInfo, u32* sampleRate) {
	char buffer[1024] = { 0 };
	char fname[128] = { 0 };
	char path[128] = { 0 };
	FILE* conf;
	char note[12][4] = {
		"C",
		"C#",
		"D",
		"D#",
		"E",
		"F",
		"F#",
		"G",
		"G#",
		"A",
		"A#",
		"H",
	};
	
	printf("\n");
	DebugPrint("Generating inst.tsv\n");
	
	SetFilename(file, fname, path, NULL, NULL, NULL);
	
	if (path[0] == 0)
		snprintf(buffer, sizeof(buffer), "%s.inst.tsv", fname);
	else
		snprintf(buffer, sizeof(buffer), "%s%s.inst.tsv", path, fname);
	
	float pitch[3] = { 0 };
	
	for (s32 i = 0; i < fileCount; i++) {
        s8 nn = instInfo[i].baseNote % 12;
        u32 f = floor((double)instInfo[i].baseNote / 12);
        
		pitch[i] = ((float)sampleRate[i]) / 32000.0f;
        
        if (gAudioState.instDataFlag[i]) {
    		pitch[i] *= pow(pow(2, 1.0 / 12), 60 - instInfo[i].baseNote);
        }
        
        DebugPrint("note %s%d\t%2.1f kHz\t%f\n", note[nn], (u32)f, (float)sampleRate[i] * 0.001, pitch[i]);
	}
	
	conf = fopen(buffer, MODE_WRITE);
	fprintf(conf, "split1\tsplit2\tsplit3\trelease\tatkrate\tatklvl\tdcy1rt\tdcy1lvl\tdcy2rt\tdcy2lvl\n");
	char floats[3][9] = {
		"NULLNULL\0",
		"NULLNULL\0",
		"NULLNULL\0",
	};
	char sampleID[3][5] = {
		"NULL\0",
		"NULL\0",
		"NULL\0",
	};
	
	char split[3][4] = {
		"0",
		"0",
		"127"
	};
	
	switch (fileCount) {
	    /* NULL Prim NULL */
	    case 1: {
		    snprintf(floats[1], 9, "%08X", hexFloat(pitch[0]));
		    snprintf(sampleID[1], 5, "%d", 0);
	    } break;
	    
	    /* NULL Prim Secn */
	    case 2: {
		    snprintf(floats[1], 9, "%08X", hexFloat(pitch[0]));
		    snprintf(sampleID[1], 5, "%d", 0);
		    
		    snprintf(floats[2], 9, "%08X", hexFloat(pitch[1]));
		    snprintf(sampleID[2], 4, "%d", 0);
		    
            if (instInfo[0].highNote != 0)
    		    snprintf(split[2], 9, "%d", instInfo[0].highNote - 21);
	    } break;
	    
	    /* Prev Prim Secn */
	    case 3: {
		    snprintf(floats[0], 9, "%08X", hexFloat(pitch[2]));
		    snprintf(sampleID[0], 5, "%d", 0);
		    
		    snprintf(floats[1], 9, "%08X", hexFloat(pitch[0]));
		    snprintf(sampleID[1], 5, "%d", 0);
		    
		    snprintf(floats[2], 9, "%08X", hexFloat(pitch[1]));
		    snprintf(sampleID[2], 5, "%d", 0);
		    
            if (instInfo[0].highNote != 0) {
    		    snprintf(split[1], 9, "%d", instInfo[0].lowNote - 21);
    		    snprintf(split[2], 9, "%d", instInfo[0].highNote - 21);
            }
	    } break;
	}
	
	fprintf(conf, "%s\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", split[0], split[1], split[2], 238, 2, 32700, 1, 32700, 32700, 29430);
	fprintf(conf, "sample1\tpitch1  \tsample2\tpitch2  \tsample3\tpitch3  \n");
	fprintf(conf, "%s\t%s\t%s\t%s\t%s\t%s", sampleID[0], floats[0], sampleID[1], floats[1], sampleID[2], floats[2]);
	
	fclose(conf);
	
	DebugPrint("inst.tsv\t\t\tOK\n");
}
