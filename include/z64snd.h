#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

#include "vadpcm.h"
#include "wave.h"

#ifndef __FLOAT80_SUCKS__
 #define __float80 float
#endif

enum {
	false,
	true
};

const char sHeaders[3][6] = {
	{ ".aiff" },
	{ ".table" },
	{ ".adpcm" }
};

#define DebugPrint(s, ...) fprintf(stdout, "\e[0;94m[*] \e[m" s, __VA_ARGS__)

#  define BSWAP16(x)       x = __builtin_bswap16(x);
#  define BSWAP32(x)       x = __builtin_bswap32(x);
#  define BSWAPFLOAT(x)    x = __builtin_bswapfloat(x);

void PrintFail(const char* fmt, ...) {
	va_list args;
	
	va_start(args, fmt);
	printf(
		"\7\e[0;91m[*] "
		"Error: \e[m"
	);
	printf(
		fmt,
		args
	);
	va_end(args);
	exit(EXIT_FAILURE);
}

void Audio_Clean(char* path, char* fname) {
	char buffer[128];
	
	if (path[0] != 0) {
		snprintf(buffer, sizeof(buffer), "rm %s%s.adpcm", path, fname);
		if (system(buffer) == -1)
			PrintFail("cleaning has failed...\n", 0);
		DebugPrint("%s.adpcm cleaned succesfully\n", fname);
		
		snprintf(buffer, sizeof(buffer), "rm %s%s.table %s.aiff", path, fname, path, fname);
		if (system(buffer) == -1)
			PrintFail("cleaning has failed...\n", 0);
		DebugPrint("%s.table and %s.aiff cleaned succesfully\n", fname, fname);
	} else {
		snprintf(buffer, sizeof(buffer), "rm %s.adpcm", fname);
		if (system(buffer) == -1)
			PrintFail("cleaning has failed...\n", 0);
		DebugPrint("%s.adpcm cleaned succesfully\n", fname);
		
		snprintf(buffer, sizeof(buffer), "rm %s.table %s.aiff", fname, fname);
		if (system(buffer) == -1)
			PrintFail("cleaning has failed...\n", 0);
		DebugPrint("%s.table and %s.aiff cleaned succesfully\n", fname, fname);
	}
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

void GetFilename(char* _src, char* _dest, char* _path, s32* sizeStore) {
	char temporName[1024] = { 0 };
	
	strcpy(temporName, _src);
	s32 sizeOfName = 0;
	s32 slashPoint = 0;
	
	while (temporName[sizeOfName] != 0)
		sizeOfName++;
	
	for (s32 i = 0; i < sizeOfName; i++) {
		if (temporName[i] == '/')
			slashPoint = i;
	}
	
	if (slashPoint != 0)
		slashPoint++;
	else
		_path[0] = 0;
	
	for (s32 i = 0; i <= sizeOfName - slashPoint - 5; i++) {
		_dest[i] = temporName[slashPoint + i];
		*sizeStore = i;
	}
	
	for (s32 i = 0; i < slashPoint; i++)
		_path[i] = _src[i];
	
	*sizeStore += 1;
}

void SetFilename(char* argv, char* fname, char* path, char* nameAiff, char* nameAdpcm, char* nameTable) {
	s32 fnameSize = 0;
	
	GetFilename(argv, fname, path, &fnameSize);
	
	if ((u64)nameAiff + (u64)nameAdpcm + (u64)nameTable == 0)
		return;
	
	/* Get Filenames to nameVars */
	for (s32 i = 0; i < fnameSize; i++) {
		if (nameAiff != NULL)
			nameAiff[i] = fname[i];
		
		if (nameAdpcm != NULL)
			nameTable[i] = fname[i];
		
		if (nameTable != NULL)
			nameAdpcm[i] = fname[i];
		
		if (i == fnameSize - 1) {
			i++;
			for (s32 j = 0; j < 6; j++) {
				if (nameAiff != NULL)
					nameAiff[i + j] = sHeaders[0][j];
				
				if (nameAdpcm != NULL)
					nameTable[i + j] = sHeaders[1][j];
				
				if (nameTable != NULL)
					nameAdpcm[i + j] = sHeaders[2][j];
			}
		}
	}
}

void Audio_ConvertWAVtoAIFF(char* fileInput, char* nameAiff) {
	FILE* f;
	WaveHeader* wav;
	FILE* o;
	s32 bitProcess = 0;
	s32 channelProcess = 0;
	s32 wasHasExtraData = 0;
	ChunkHeader chunkHeader;
	WaveChunk* wavDataChunk;
	InstrumentHeader* waveInst;
	SampleHeader* waveSmpl;
	s16* data;
	
	/* FILE LOAD */ {
		WaveHeader wave;
		f = fopen(fileInput, MODE_READ);
		fread(&wave, sizeof(WaveHeader), 1, f);
		fclose(f);
		
		f = fopen(fileInput, MODE_READ);
		wav = malloc(wave.chunk.size + 8);
		if (wav == NULL)
			PrintFail("Malloc Fail\n");
		fread(wav, wave.chunk.size + 8, 1, f);
		fclose(f);
		
		wavDataChunk = (WaveChunk*)wav;
		
		while (!(wavDataChunk->ID[0] == 0x64 && wavDataChunk->ID[1] == 0x61 && wavDataChunk->ID[2] == 0x74 && wavDataChunk->ID[3] == 0x61)) {
			char* s = (char*)wavDataChunk;
			s += 1;
			wavDataChunk = (WaveChunk*)s;
			
			if ((u64)wavDataChunk > (u64)wav + 8 + wav->chunk.size) {
				DebugPrint("%X / %X\n", (u64)wavDataChunk - (u64)wav, 8 + wav->chunk.size);
				PrintFail("SampleChunk: Out of range. Better stop before everything goes bonkers.\n", 0);
			}
		}
		
		data = ((WaveData*)wavDataChunk)->data;
		
		if (wav->chunk.size - 0x24 > wavDataChunk->size)
			wasHasExtraData = true;
	}
	
	if ((o = fopen(nameAiff, MODE_WRITE)) == NULL) {
		PrintFail("Output file [%s] could not be opened.\n", nameAiff);
	}
	
	if (wasHasExtraData == true) {
		waveSmpl = (SampleHeader*)((char*)data + wavDataChunk->size);
		while (!(waveSmpl->chunk.ID[0] == 0x73 && waveSmpl->chunk.ID[1] == 0x6D && waveSmpl->chunk.ID[2] == 0x70 && waveSmpl->chunk.ID[3] == 0x6C)) {
			char* s = (char*)waveSmpl;
			s += 1;
			waveSmpl = (SampleHeader*)s;
			
			if ((u64)waveSmpl > (u64)wav + 8 + wav->chunk.size)
				PrintFail("SampleInfo: Out of range. Better stop before everything goes bonkers.\n", 0);
		}
		
		waveInst = (InstrumentHeader*)((char*)data + wavDataChunk->size);
		while (!(waveInst->chunk.ID[0] == 0x69 && waveInst->chunk.ID[1] == 0x6E && waveInst->chunk.ID[2] == 0x73 && waveInst->chunk.ID[3] == 0x74)) {
			char* s = (char*)waveInst;
			s += 1;
			waveInst = (InstrumentHeader*)s;
			
			if ((u64)waveInst > (u64)wav + 8 + wav->chunk.size)
				PrintFail("InstrumentInfo: Out of range. Better stop before everything goes bonkers.\n", 0);
		}
	}
	
    DebugPrint("BitRate\t\t\t%d-bit.\n", wav->bitsPerSamp);
	if (wav->bitsPerSamp != 16) {
		bitProcess = true;
		DebugPrint("Warning: Provided WAV isn't 16bit.\n", 0);
		// if (wav->bitsPerSamp != 32)
		PrintFail("Bit depth is %X. That's too much of an hassle to work with...\7\n", wav->bitsPerSamp);
	}
	
    DebugPrint("Channels\t\t\t%d\n", wav->numChannels);
	if (wav->numChannels > 1) {
		channelProcess = true;
		DebugPrint("Warning: Provided WAV isn't mono.\nSelecting left channel for conversion.\n", 0);
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
		
		if (wasHasExtraData)
			formChunk.ckSize += sizeof(ChunkHeader) + sizeof(Marker) * 2 + sizeof(u16) + sizeof(ChunkHeader) + sizeof(InstrumentChunk);
		
		BSWAP32(formChunk.ckID);
		BSWAP32(formChunk.ckSize);
		BSWAP32(formChunk.formType);
		fwrite(&formChunk, sizeof(Chunk), 1, o);
	}
	DebugPrint("FormChunk\t\t\tDone\n", 0);
	
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
	DebugPrint("CommonChunk\t\t\tDone\n", 0);
	
	if (wasHasExtraData == true) {      /* MARK && INST CHUNK */
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
		
		BSWAP16(marker[0].MarkerID);
		BSWAP16(marker[0].positionH);
		BSWAP16(marker[0].positionL);
		BSWAP16(marker[1].MarkerID);
		BSWAP16(marker[1].positionH);
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
				.playMode = 1,
				.beginLoop = 0,
				.endLoop = 1
			}
		};
		
		BSWAP16(instChunk.sustainLoop.playMode);
		BSWAP16(instChunk.sustainLoop.beginLoop);
		BSWAP16(instChunk.sustainLoop.endLoop);
		fwrite(&instChunk, sizeof(InstrumentChunk), 1, o);
		DebugPrint("ExtraChunk\t\t\tDone\n", 0);
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
				
				BSWAP16(tempData);
				fwrite(&tempData, sizeof(s16), 1, o);
				
				if (channelProcess)
					i += 1;
			}
			;
		} else {
			for (s32 i = 0; i < wavDataChunk->size / 2 / 2; i++) {
				s32* tempData = (s32*)data;
				
				s32 tempS32 = tempData[i];
				BSWAP32(tempS32);
				s16 tempS16 = Audio_Downsample(tempS32);
                // BSWAP16(tempS16);
				
				fwrite(&tempS32, sizeof(s16), 1, o);
				
				if (channelProcess)
					i += 1;
			}
			;
		}
	}
	DebugPrint("SoundChunk\t\t\tDone\n", 0);
	DebugPrint("WAV to AIFF conversion\tOK\n\n", 0);
	fclose(o);
}

void Audio_Process(char* argv, int clean, ALADPCMloop* _destLoop, InstrumentChunk* _destInst) {
	char fname[64] = { 0 };
	char path[128] = { 0 };
	char nameAiff[64] = { 0 };
	char nameTable[64] = { 0 };
	char nameAdpcm[64] = { 0 };
	char buffer[128];
	
	SetFilename(argv,fname,path,nameAiff,nameAdpcm,nameTable);
	DebugPrint("Name: %s\n", fname);
    DebugPrint("Path: %s\n\n", path);
	Audio_ConvertWAVtoAIFF(argv, nameAiff);
    
    #ifdef _WIN32
    char TOOL_TABLEDESIGN[64] = {
        "tools\\tabledesign.exe\0"
        
    };
    char TOOL_VADPCM_ENC[64] = {
        "tools\\vadpcm_enc.exe\0"
    };
	#else
    char TOOL_TABLEDESIGN[64] = {
        "./tools/tabledesign\0"
        
    };
    char TOOL_VADPCM_ENC[64] = {
        "./tools/vadpcm_enc\0"
    };
    #endif
    
	if (path[0] != 0) {
        DebugPrint("%s -i 2000 %s%s > %s%s\n", TOOL_TABLEDESIGN, path, nameAiff, path, nameTable);
		snprintf(buffer, sizeof(buffer), "%s -i 2000 %s%s > %s%s\0", TOOL_TABLEDESIGN, path, nameAiff, path, nameTable);
		if (system(buffer) == -1)
			PrintFail("tabledesigner has failed...\n", 0);
		DebugPrint("%s generated succesfully\n", nameTable);
		
        DebugPrint("%s -c %s%s %s%s %s%s\n", TOOL_VADPCM_ENC, path, nameTable, path, nameAiff, path, nameAdpcm);
		snprintf(buffer, sizeof(buffer), "%s -c %s%s %s%s %s%s\0", TOOL_VADPCM_ENC, path, nameTable, path, nameAiff, path, nameAdpcm);
		if (system(buffer) == -1)
			PrintFail("vadpcm_enc has failed...\n", 0);
		DebugPrint("%s converted to %s succesfully\n", nameAiff, nameAdpcm);
	} else {
        DebugPrint("%s -i 2000 %s > %s\n", TOOL_TABLEDESIGN, nameAiff, nameTable);
		snprintf(buffer, sizeof(buffer), "%s -i 2000 %s > %s\0", TOOL_TABLEDESIGN, nameAiff, nameTable);
		if (system(buffer) == -1)
			PrintFail("tabledesigner has failed...\n", 0);
		DebugPrint("%s generated succesfully\n", nameTable);
		
        DebugPrint("%s -c %s %s %s\n", TOOL_VADPCM_ENC, nameTable, nameAiff, nameAdpcm);
		snprintf(buffer, sizeof(buffer), "%s -c %s %s %s\0", TOOL_VADPCM_ENC, nameTable, nameAiff, nameAdpcm);
		if (system(buffer) == -1)
			PrintFail("vadpcm_enc has failed...\n", 0);
		DebugPrint("%s converted to %s succesfully\n", nameAiff, nameAdpcm);
	}
	
	FILE* f;
	FILE* pred;
	FILE* samp;
	FILE* conf;
	FILE* inst;
	char* adpcm;
	char* p = 0;
	s32 allocSize;
	
	f = fopen(nameAdpcm, "r");
	fseek(f, 0, SEEK_END);
	allocSize = ftell(f);
	rewind(f);
	
	p = adpcm = malloc(allocSize);
	if (adpcm == 0)
		PrintFail("malloc fail\n");
    DebugPrint("adpcm malloc\t\tOK\n", 0);
	fread(adpcm, allocSize, 1, f);
	fclose(f);
	
	/* PREDICTOR */
	snprintf(buffer, sizeof(buffer), "%s%s_prd.bin", path,fname);
	pred = fopen(buffer, MODE_WRITE);
	while (!(p[-7] == 'C' && p[-6] == 'O' && p[-5] == 'D' && p[-4] == 'E' && p[-3] == 'S')) {
		p++;
        if ((u64)p - (u64)adpcm > allocSize) {
			PrintFail("Out of bounds while searchin:\t'CODES'\n");
		}
    }
	
	s8 head[] = { 0, 0, 0, p[1], 0, 0, 0, p[3] };
	
	fwrite(&head, sizeof(head), 1, pred);
	p += 4;
	
	s32 i = 0;
	
	while (!(p[0] == 'S' && p[1] == 'S' && p[2] == 'N' && p[3] == 'D')) {
		fwrite(p, 1, 1, pred);
		p++;
        if ((u64)p - (u64)adpcm > allocSize) {
			PrintFail("Out of bounds while searchin:\t'SSND'\n");
		}
	}
    
	fclose(pred);
	
	/* SAMPLE */
	snprintf(buffer, sizeof(buffer), "%s%s_smp.bin", path,fname);
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
	
	printf("\n");
	DebugPrint("%s_prd.bin\t\tOK\n", fname);
	DebugPrint("%s_smp.bin\t\tOK\n", fname);
	
	ALADPCMloop* loopInfo;
	InstrumentChunk* instData;
	CommonChunk* comm = (CommonChunk*)(adpcm + 14);
	int lflag = true;
	
	snprintf(buffer, sizeof(buffer), "%s%s_config.tsv", path, fname);
	conf = fopen(buffer, MODE_WRITE);
	
	while (!(p[-8] == 'O' && p[-7] == 'O' && p[-6] == 'P' && p[-5] == 'S')) {
		p++;
		if ((u64)p - (u64)adpcm > allocSize) {
			lflag = false;
			break;
		}
	}
	
	if (lflag) {
		loopInfo = (ALADPCMloop*)p;
        bcopy(loopInfo, _destLoop, sizeof(ALADPCMloop));
		BSWAP32(loopInfo->start);
		BSWAP32(loopInfo->end);
		BSWAP32(loopInfo->count);
	}
	
	u32 numFrame = ((comm->numFramesH) << 8) + comm->numFramesL;
	
	p = adpcm;
	
	while (!(p[0] == 'I' && p[1] == 'N' && p[2] == 'S' && p[3] == 'T')) {
		p++;
        if ((u64)p - (u64)adpcm > allocSize) {
			PrintFail("Out of bounds while searchin:\t'INST'\n");
		}
    }
    
	instData = (InstrumentChunk*)p;
    bcopy(instData, _destInst, sizeof(InstrumentChunk));
	
	fprintf(conf, "precision\tloopstart\tloopend  \tloopcount\tlooptail \n%08X\t", 0);
	if (lflag) {
		/* Found Loop */
		fprintf(conf, "%08X\t", loopInfo->start);
		fprintf(conf, "%08X\t", loopInfo->end);
		fprintf(conf, "%08X\t", loopInfo->count);
		fprintf(conf, "%08X\n", numFrame);
	} else {
		fprintf(conf, "%08X\t", 0);
		fprintf(conf, "%08X\t", numFrame);
		fprintf(conf, "%08X\t", 0);
		fprintf(conf, "%08X\n", 0);
	}
    
    DebugPrint("%s_config.tsv\t\tOK\n\n", fname);
	
	fclose(conf);
}