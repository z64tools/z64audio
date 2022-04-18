#include "AudioConvert.h"
#include "AudioTools.h"

NameParam gBinNameIndex;
u32 gSampleRate = 32000;
u32 gPrecisionFlag;
f32 gTuning = 1.0f;
bool gRomMode;

char* sBinName[][3] = {
	{
		".vadpcm.bin",
		".book.bin",
		".loopbook.bin",
	},
	{
		"_sample.bin",
		"_predictors.bin",
		"_looppredictors.bin",
	},
};

u32 Audio_ConvertBigEndianFloat80(AiffInfo* aiffInfo) {
	f80 float80 = 0;
	u8* pointer = (u8*)&float80;
	
	for (s32 i = 0; i < 10; i++) {
		pointer[i] = (u8)aiffInfo->sampleRate[9 - i];
	}
	
	return (s32)float80;
}

u32 Audio_ByteSwap_FromHighLow(u16* h, u16* l) {
	ByteSwap(h, SWAP_U16);
	ByteSwap(l, SWAP_U16);
	
	u8* db = (u8*)h;
	
	printf_debugExt("%02X%02X%02X%02X == %08X", db[0], db[1], db[2], db[3], (u32)(*h << 16) | *l);
	
	return (*h << 16) | *l;
}

void Audio_ByteSwap_ToHighLow(u32* source, u16* h) {
	h[0] = source[0] >> 16;
	h[1] = source[0];
	
	ByteSwap(h, SWAP_U16);
	ByteSwap(&h[1], SWAP_U16);
	
	u8* db = (u8*)h;
	
	printf_debugExt("%02X%02X%02X%02X == %08X", db[0], db[1], db[2], db[3], *source);
}

void Audio_ByteSwap(AudioSampleInfo* sampleInfo) {
	if (sampleInfo->bit == 16) {
		for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
			ByteSwap(&sampleInfo->audio.u16[i], sampleInfo->bit / 8);
		}
		
		return;
	}
	if (sampleInfo->bit == 24) {
		// @bug: Seems to some times behave oddly. Look into it later
		printf_warning("ByteSwapping 24-bit audio. This might cause slight issues when converting from AIFF to WAV");
		for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
			ByteSwap(&sampleInfo->audio.u8[3 * i], sampleInfo->bit / 8);
		}
		
		return;
	}
	if (sampleInfo->bit == 32) {
		for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
			ByteSwap(&sampleInfo->audio.u32[i], sampleInfo->bit / 8);
		}
		
		return;
	}
}

void Audio_Normalize(AudioSampleInfo* sampleInfo) {
	u32 sampleNum = sampleInfo->samplesNum;
	u32 channelNum = sampleInfo->channelNum;
	f32 max;
	f32 mult;
	
	printf_info_align("Normalizing", "");
	
	if (sampleInfo->bit == 16) {
		max = 0;
		
		for (s32 i = 0; i < sampleNum * channelNum; i++) {
			if (sampleInfo->audio.s16[i] > max) {
				max = Abs(sampleInfo->audio.s16[i]);
			}
			
			if (max >= __INT16_MAX__) {
				return;
			}
		}
		
		mult = __INT16_MAX__ / max;
		
		for (s32 i = 0; i < sampleNum * channelNum; i++) {
			sampleInfo->audio.s16[i] *= mult;
		}
		
		return;
	}
	
	if (sampleInfo->bit == 32) {
		max = 0;
		
		for (s32 i = 0; i < sampleNum * channelNum; i++) {
			if (sampleInfo->audio.f32[i] > max) {
				max = fabsf(sampleInfo->audio.f32[i]);
			}
			
			if (max == 1.0f) {
				return;
			}
		}
		
		mult = 1.0f / max;
		
		for (s32 i = 0; i < sampleNum * channelNum; i++) {
			sampleInfo->audio.f32[i] *= mult;
		}
	}
	
	printf_debugExt("normalizermax: %f", max);
}

void Audio_ConvertToMono(AudioSampleInfo* sampleInfo) {
	if (sampleInfo->channelNum != 2)
		return;
	
	printf_info_align("Mono", "");
	
	if (sampleInfo->bit == 24) {
		printf_error("%s: 24-bit not supported yet.", __FUNCTION__);
	}
	
	if (sampleInfo->bit == 16) {
		for (s32 i = 0, j = 0; i < sampleInfo->samplesNum; i++, j += 2) {
			sampleInfo->audio.s16[i] = ((f32)sampleInfo->audio.s16[j] + (f32)sampleInfo->audio.s16[j + 1]) * 0.5f;
		}
	}
	
	if (sampleInfo->bit == 32) {
		for (s32 i = 0, j = 0; i < sampleInfo->samplesNum; i++, j += 2) {
			sampleInfo->audio.f32[i] = (sampleInfo->audio.f32[j] + sampleInfo->audio.f32[j + 1]) * 0.5f;
		}
	}
	
	sampleInfo->size /= 2;
	sampleInfo->channelNum = 1;
}

void Audio_Downsample(AudioSampleInfo* sampleInfo) {
	printf_info_align("Downsampling", "%d-bit -> %d-bit", sampleInfo->bit, sampleInfo->targetBit);
	
	if (sampleInfo->targetBit == 16) {
		if (sampleInfo->bit == 24) {
			for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
				u16 samp = *(u16*)&sampleInfo->audio.u8[3 * i + 1];
				sampleInfo->audio.s16[i] = samp;
			}
			
			sampleInfo->bit = 16;
			sampleInfo->size *= (2.0 / 3.0);
		}
		
		if (sampleInfo->bit == 32) {
			if (sampleInfo->dataIsFloat == true) {
				for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
					sampleInfo->audio.s16[i] = sampleInfo->audio.f32[i] * __INT16_MAX__;
				}
				
				sampleInfo->bit = 16;
				sampleInfo->size *= 0.5;
				sampleInfo->dataIsFloat = false;
			} else {
				for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
					sampleInfo->audio.s16[i] = sampleInfo->audio.s32[i] * ((f32)__INT16_MAX__ / (f32)__INT32_MAX__);
				}
				
				sampleInfo->bit = 16;
				sampleInfo->size *= 0.5;
			}
		}
	}
	if (sampleInfo->targetBit == 24) {
		if (sampleInfo->bit == 32) {
			printf_error("Resample to 24-bit not supported yet.");
			// for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
			// 	sampleInfo->audio.data[i] = sampleInfo->audio.dataFloat[i] * __INT16_MAX__;
			// }
			//
			// sampleInfo->bit = 16;
			// sampleInfo->size *= 0.5;
		}
	}
}

void Audio_Upsample(AudioSampleInfo* sampleInfo) {
	MemFile newMem = MemFile_Initialize();
	u32 samplesNum = sampleInfo->samplesNum;
	u32 channelNum = sampleInfo->channelNum;
	
	printf_info_align("Upsampling", "%d-bit -> 32-bit", sampleInfo->bit);
	printf_debugExt("Upsampling will iterate %d times. %d x %d", samplesNum * channelNum, samplesNum, channelNum);
	
	MemFile_Malloc(&newMem, samplesNum * sizeof(f32) * channelNum);
	
	if (sampleInfo->bit == 16) {
		for (s32 i = 0; i < samplesNum * channelNum; i++) {
			newMem.cast.f32[i] = (f32)sampleInfo->audio.s16[i] / __INT16_MAX__;
		}
	}
	
	if (sampleInfo->bit == 24) {
		for (s32 i = 0; i < samplesNum * channelNum; i++) {
			newMem.cast.f32[i] = (f32)(*(s16*)&sampleInfo->audio.u8[3 * i + 1]) / __INT16_MAX__;
		}
	}
	
	printf_debugExt("Upsampling done.");
	
	newMem.dataSize = sampleInfo->size =
		sampleInfo->samplesNum *
		sampleInfo->channelNum *
		sizeof(f32);
	sampleInfo->bit = 32;
	sampleInfo->dataIsFloat = true;
	MemFile_Free(&sampleInfo->memFile);
	sampleInfo->memFile = newMem;
	sampleInfo->audio.p = newMem.data;
}

void Audio_Resample(AudioSampleInfo* sampleInfo) {
	if (sampleInfo->targetBit > sampleInfo->bit) {
		Audio_Upsample(sampleInfo);
	}
	
	if (sampleInfo->targetBit != sampleInfo->bit)
		Audio_Downsample(sampleInfo);
}

void Audio_Compare(AudioSampleInfo* sampleA, AudioSampleInfo* sampleB) {
	char* addString[] = {
		"",
		" when compared by the frame count of smaller file"
	};
	u32 flag = 0;
	u32 sampleNum = sampleA->samplesNum;
	
	if (sampleA->sampleRate != sampleB->sampleRate) {
		printf_error("SampleRate for given files does not match. Giving up on comparison.");
	}
	
	if (sampleA->samplesNum != sampleB->samplesNum) {
		printf_warning("Frame count for given files does not match.");
		printf_debugExt("samplesNum: [ %8d ] " PRNT_GRAY "[%s]" PRNT_RSET, sampleA->samplesNum, sampleA->input);
		printf_debug("samplesNum: [ %8d ] " PRNT_GRAY "[%s]" PRNT_RSET, sampleB->samplesNum, sampleB->input);
		flag = 1;
		
		if (sampleB->samplesNum < sampleA->samplesNum) {
			sampleNum = sampleB->samplesNum;
		}
	}
	
	if (sampleA->bit != sampleB->bit || sampleA->dataIsFloat != sampleB->dataIsFloat) {
		printf_warning("Bit depth for given files does not match. Converting to 16-bit. Might affect to the results.");
		sampleA->targetBit = 16;
		sampleB->targetBit = 16;
		Audio_Downsample(sampleA);
		Audio_Downsample(sampleB);
	}
	
	if (sampleA->channelNum != sampleB->channelNum) {
		printf_warning("Channel amount for given files does not match. Giving up on comparison.");
		Audio_ConvertToMono(sampleA);
		Audio_ConvertToMono(sampleB);
	}
	
	u32 diffCount = 0;
	
	for (s32 i = 0; i < sampleNum * sampleA->channelNum; i++) {
		if (sampleA->bit == 16) {
			if (sampleA->audio.s16[i] != sampleB->audio.s16[i]) {
				printf("\rDiff count [ %d / %d ]          ", ++diffCount, sampleNum * sampleA->channelNum);
			}
		}
		if (sampleA->bit == 24) {
			if (*(s16*)&sampleA->audio.u8[3 * i + 1] != *(s16*)&sampleB->audio.u8[3 * i + 1]) {
				printf("\rDiff count [ %d / %d ]          ", ++diffCount, sampleNum * sampleA->channelNum);
			}
		}
		if (sampleA->bit == 32) {
			if (sampleA->dataIsFloat) {
				if (sampleA->audio.f32[i] != sampleB->audio.f32[i]) {
					printf("\rDiff count [ %d / %d ]          ", ++diffCount, sampleNum * sampleA->channelNum);
				}
			} else {
				if (sampleA->audio.s32[i] != sampleB->audio.s32[i]) {
					printf("\rDiff count [ %d / %d ]          ", ++diffCount, sampleNum * sampleA->channelNum);
				}
			}
		}
	}
	
	if (diffCount) {
		printf("\n");
	} else {
		printf_info_align("Data matches", "%s", addString[flag]);
	}
}

void Audio_InitSampleInfo(AudioSampleInfo* sampleInfo, char* input, char* output) {
	memset(sampleInfo, 0, sizeof(*sampleInfo));
	sampleInfo->instrument.note = 60;
	sampleInfo->instrument.highNote = __INT8_MAX__;
	sampleInfo->instrument.lowNote = 0;
	sampleInfo->targetBit = 16;
	sampleInfo->input = input;
	sampleInfo->output = output;
}

void Audio_FreeSample(AudioSampleInfo* sampleInfo) {
	MemFile_Free(&sampleInfo->vadBook);
	MemFile_Free(&sampleInfo->vadLoopBook);
	MemFile_Free(&sampleInfo->memFile);
	
	*sampleInfo = (AudioSampleInfo) { 0 };
}

typedef enum {
	AUDIO_WAV,
	AUDIO_AIFF,
} AudioFlag;

void* Audio_GetChunk(AudioSampleInfo* sampleInfo, AudioFlag param, const char* type) {
	s8* data = sampleInfo->memFile.cast.s8 + 0xC;
	u32 fileSize = sampleInfo->memFile.cast.u32[1];
	
	if (param == AUDIO_AIFF) SwapBE(fileSize);
	
	for (;;) {
		struct {
			char type[4];
			u32  size;
		} chunkHead;
		
		memcpy(&chunkHead, data, 0x8);
		if (param == AUDIO_AIFF) SwapBE(chunkHead.size);
		
		if (MemMem(type, 4, chunkHead.type, 4)) {
			printf_debug_align("Chunk:", "%.4s", chunkHead.type);
			
			return data;
		}
		
		data += 0x8 + chunkHead.size;
		
		if ((uPtr)data - (uPtr)sampleInfo->memFile.data >= fileSize)
			return NULL;
	}
}

void Audio_LoadSample_Wav(AudioSampleInfo* sampleInfo) {
	MemFile_LoadFile_ReqExt(&sampleInfo->memFile, sampleInfo->input, ".wav");
	WaveHeader* waveHeader = sampleInfo->memFile.data;
	WaveDataInfo* waveData;
	WaveInfo* waveInfo;
	WaveInstrumentInfo* waveInstInfo;
	WaveSampleInfo* waveSampleInfo;
	
	if (!sampleInfo->memFile.data) {
		printf_error("Closing.");
	}
	
	if (!MemMem(waveHeader, 0x10, "WAVE", 4) || !MemMem(waveHeader, 0x10, "RIFF", 4)) {
		char headerA[5] = { 0 };
		char headerB[5] = { 0 };
		
		memcpy(headerA, waveHeader->chunk.name, 4);
		memcpy(headerB, waveHeader->format, 4);
		
		printf_error(
			"[%s] header does not match what's expected\n"
			"\t\tChunkHeader: \e[0;91m[%s]\e[m -> [RIFF]\n"
			"\t\tFormat:      \e[0;91m[%s]\e[m -> [WAVE]",
			sampleInfo->input,
			headerA,
			headerB
		);
	}
	if ((waveInfo = Audio_GetChunk(sampleInfo, AUDIO_WAV, "fmt ")) == NULL) printf_error("Wave: No [fmt]");
	if ((waveData = Audio_GetChunk(sampleInfo, AUDIO_WAV, "data")) == NULL) printf_error("Wave: No [data]");
	waveInstInfo = Audio_GetChunk(sampleInfo, AUDIO_WAV, "inst");
	waveSampleInfo = Audio_GetChunk(sampleInfo, AUDIO_WAV, "smpl");
	
	sampleInfo->channelNum = waveInfo->channelNum;
	sampleInfo->bit = waveInfo->bit;
	sampleInfo->sampleRate = waveInfo->sampleRate;
	sampleInfo->samplesNum = waveData->chunk.size / (waveInfo->bit / 8) / waveInfo->channelNum;
	sampleInfo->size = waveData->chunk.size;
	sampleInfo->audio.p = waveData->data;
	sampleInfo->instrument.loop.end = sampleInfo->samplesNum;
	
	if (waveInfo->format == IEEE_FLOAT) {
		printf_debugExt("32-bit Float");
		sampleInfo->dataIsFloat = true;
	}
	
	if (waveInstInfo) {
		sampleInfo->instrument.fineTune = waveInstInfo->fineTune;
		sampleInfo->instrument.note = waveInstInfo->note;
		sampleInfo->instrument.highNote = waveInstInfo->hiNote;
		sampleInfo->instrument.lowNote = waveInstInfo->lowNote;
	}
	
	if (waveSampleInfo && waveSampleInfo->numSampleLoops) {
		sampleInfo->instrument.loop.start = waveSampleInfo->loopData[0].start;
		sampleInfo->instrument.loop.end = waveSampleInfo->loopData[0].end;
		sampleInfo->instrument.loop.count = 0xFFFFFFFF;
	}
}

void Audio_LoadSample_Aiff(AudioSampleInfo* sampleInfo) {
	MemFile_LoadFile_ReqExt(&sampleInfo->memFile, sampleInfo->input, ".aiff");
	AiffHeader* aiffHeader = sampleInfo->memFile.data;
	AiffDataInfo* aiffData;
	AiffInfo* aiffInfo;
	AiffInstrumentInfo* aiffInstInfo;
	AiffMarkerInfo* aiffMarkerInfo;
	
	if (!sampleInfo->memFile.data) {
		printf_error("Closing.");
	}
	
	if (!MemMem(aiffHeader, 0x10, "FORM", 4) || !MemMem(aiffHeader, 0x10, "AIFF", 4)) {
		printf_error(
			"[%s] header does not match what's expected\n"
			"\t\tChunkHeader: \e[0;91m[%.4s]\e[m -> [FORM]\n"
			"\t\tFormat:      \e[0;91m[%.4s]\e[m -> [AIFF]",
			sampleInfo->input,
			aiffHeader->chunk.name,
			aiffHeader->formType
		);
	}
	
	if (sampleInfo->memFile.dataSize == 0)
		printf_error("Something has gone wrong loading file [%s]", sampleInfo->input);
	if ((aiffInfo = Audio_GetChunk(sampleInfo, AUDIO_AIFF, "COMM")) == NULL) printf_error("Aiff: No [COMM]");
	if ((aiffData = Audio_GetChunk(sampleInfo, AUDIO_AIFF, "SSND")) == NULL) printf_error("Aiff: No [SSND]");
	aiffInstInfo = Audio_GetChunk(sampleInfo, AUDIO_AIFF, "INST");
	aiffMarkerInfo = Audio_GetChunk(sampleInfo, AUDIO_AIFF, "MARK");
	
	// Swap Chunk Sizes
	SwapBE(aiffHeader->chunk.size);
	SwapBE(aiffInfo->chunk.size);
	SwapBE(aiffData->chunk.size);
	
	// Swap INFO
	SwapBE(aiffInfo->channelNum);
	SwapBE(aiffInfo->sampleNumH);
	SwapBE(aiffInfo->sampleNumL);
	SwapBE(aiffInfo->bit);
	
	// Swap DATA
	SwapBE(aiffData->offset);
	SwapBE(aiffData->blockSize);
	
	u32 sampleNum = aiffInfo->sampleNumL + (aiffInfo->sampleNumH << 16);
	
	sampleInfo->channelNum = aiffInfo->channelNum;
	sampleInfo->bit = aiffInfo->bit;
	sampleInfo->sampleRate = Audio_ConvertBigEndianFloat80(aiffInfo);
	sampleInfo->samplesNum = sampleNum;
	sampleInfo->size = aiffData->chunk.size - 0x8;
	sampleInfo->audio.p = aiffData->data;
	
	sampleInfo->instrument.loop.end = sampleInfo->samplesNum;
	
	printf_debug("sos");
	
	if (aiffInstInfo) {
		sampleInfo->instrument.note = aiffInstInfo->baseNote;
		sampleInfo->instrument.fineTune = aiffInstInfo->detune;
		sampleInfo->instrument.highNote = aiffInstInfo->highNote;
		sampleInfo->instrument.lowNote = aiffInstInfo->lowNote;
		
		if (aiffMarkerInfo && aiffInstInfo->sustainLoop.playMode >= 1) {
			u16 startIndex = ReadBE(aiffInstInfo->sustainLoop.start) - 1;
			u16 endIndex = ReadBE(aiffInstInfo->sustainLoop.end) - 1;
			u16 loopStartH = aiffMarkerInfo->marker[startIndex].positionH;
			u16 loopStartL = aiffMarkerInfo->marker[startIndex].positionL;
			u16 loopEndH = aiffMarkerInfo->marker[endIndex].positionH;
			u16 loopEndL = aiffMarkerInfo->marker[endIndex].positionL;
			
			printf_debugExt("Loop: loopId %04X loopId %04X Gain %04X", (u16)startIndex, (u16)endIndex, aiffInstInfo->gain);
			
			sampleInfo->instrument.loop.start = Audio_ByteSwap_FromHighLow(&loopStartH, &loopStartL);
			sampleInfo->instrument.loop.end = Audio_ByteSwap_FromHighLow(&loopEndH, &loopEndL);
			sampleInfo->instrument.loop.count = 0xFFFFFFFF;
		}
	}
	
	Audio_ByteSwap(sampleInfo);
}

void Audio_LoadSample_AifcVadpcm(AudioSampleInfo* sampleInfo) {
	MemFile_LoadFile_ReqExt(&sampleInfo->memFile, sampleInfo->input, ".aifc");
	AiffHeader* aiffHeader = sampleInfo->memFile.data;
	AiffDataInfo* aiffData;
	AiffInfo* aiffInfo;
	AiffInstrumentInfo* aiffInstInfo;
	
	if (!sampleInfo->memFile.data) {
		printf_error("Closing.");
	}
	
	if (!MemMem(aiffHeader, 0x90, "VAPC\x0BVADPCM", 10)) {
		printf_error("[%s] does not appear to be using VADPCM compression type.", sampleInfo->input);
	}
	
	if (!MemMem(aiffHeader, 0x10, "FORM", 4) || !MemMem(aiffHeader, 0x10, "AIFC", 4)) {
		printf_error(
			"[%s] header does not match what's expected\n"
			"\t\tChunkHeader: \e[0;91m[%.4s]\e[m -> [FORM]\n"
			"\t\tFormat:      \e[0;91m[%.4s]\e[m -> [AIFC]",
			sampleInfo->input,
			aiffHeader->chunk.name,
			aiffHeader->formType
		);
	}
	
	if (sampleInfo->memFile.dataSize == 0)
		printf_error("Something has gone wrong loading file [%s]", sampleInfo->input);
	if ((aiffInfo = Audio_GetChunk(sampleInfo, AUDIO_AIFF, "COMM")) == NULL) printf_error("Aiff: No [COMM]");
	if ((aiffData = Audio_GetChunk(sampleInfo, AUDIO_AIFF, "SSND")) == NULL) printf_error("Aiff: No [SSND]");
	aiffInstInfo = Audio_GetChunk(sampleInfo, AUDIO_AIFF, "INST");
	
	// Swap Chunk Sizes
	SwapBE(aiffHeader->chunk.size);
	SwapBE(aiffInfo->chunk.size);
	SwapBE(aiffData->chunk.size);
	
	// Swap INFO
	SwapBE(aiffInfo->channelNum);
	SwapBE(aiffInfo->sampleNumH);
	SwapBE(aiffInfo->sampleNumL);
	SwapBE(aiffInfo->bit);
	
	// Swap DATA
	SwapBE(aiffData->offset);
	SwapBE(aiffData->blockSize);
	
	u32 sampleNum = aiffInfo->sampleNumL + (aiffInfo->sampleNumH << 16);
	
	sampleInfo->channelNum = aiffInfo->channelNum;
	sampleInfo->bit = aiffInfo->bit;
	sampleInfo->sampleRate = Audio_ConvertBigEndianFloat80(aiffInfo);
	sampleInfo->samplesNum = sampleNum;
	sampleInfo->size = aiffData->chunk.size - 0x8;
	sampleInfo->audio.p = aiffData->data;
	
	sampleInfo->instrument.loop.end = sampleInfo->samplesNum;
	
	if (aiffInstInfo) {
		sampleInfo->instrument.note = aiffInstInfo->baseNote;
		sampleInfo->instrument.fineTune = aiffInstInfo->detune;
		sampleInfo->instrument.highNote = aiffInstInfo->highNote;
		sampleInfo->instrument.lowNote = aiffInstInfo->lowNote;
	}
	
	// VADPCM PREDICTORS
	VadpcmInfo* pred = MemMem(sampleInfo->memFile.data, sampleInfo->memFile.dataSize, "APPL", 4);
	MemFile* vadBook = &sampleInfo->vadBook;
	MemFile* vadLoop = &sampleInfo->vadLoopBook;
	
	if (pred && MemMem(pred, 24, "VADPCMCODES", 11)) {
		u16 order = ReadBE(((u16*)pred->data)[1]);
		u16 nPred = ReadBE(((u16*)pred->data)[2]);
		u32 predSize = sizeof(u16) * (0x8 * order * nPred + 2);
		
		printf_debugExt("order: [%d] nPred: [%d] size: [0x%X]", order, nPred, predSize);
		
		MemFile_Malloc(vadBook, predSize);
		MemFile_Write(vadBook, &((u16*)pred->data)[1], predSize);
		
		for (s32 i = 0; i < 0x8 * nPred * order + 2; i++) {
			SwapBE(vadBook->cast.u16[i]);
		}
		
		if (gPrintfSuppress <= PSL_DEBUG) {
			printf_debugExt("%04X %04X", vadBook->cast.u16[0], vadBook->cast.u16[1]);
			
			for (s32 i = 0; i < nPred * order; i++) {
				printf_debug(
					"%04X %04X %04X %04X %04X %04X %04X %04X",
					vadBook->cast.u16[2 + i + 0],
					vadBook->cast.u16[2 + i + 1],
					vadBook->cast.u16[2 + i + 2],
					vadBook->cast.u16[2 + i + 3],
					vadBook->cast.u16[2 + i + 4],
					vadBook->cast.u16[2 + i + 5],
					vadBook->cast.u16[2 + i + 6],
					vadBook->cast.u16[2 + i + 7]
				);
			}
		}
	}
	
	// VADPCM LOOP PREDICTORS
	u32 size = sampleInfo->memFile.dataSize - sampleInfo->size;
	u8* next = (void*)aiffHeader;
	
	printf_debugExt("Search Size [0x%X]", size);
	pred = MemMem(next + sampleInfo->size, size, "APPL", 4);
	
	if (pred && MemMem(pred, 24, "VADPCMLOOPS", 11)) {
		printf_debugExt("Found  [VADPCMLOOPS]");
		MemFile_Malloc(&sampleInfo->vadLoopBook, sizeof(s16) * 16);
		MemFile_Write(vadLoop, &((u16*)pred->data)[8], sizeof(s16) * 16);
		for (s32 i = 0; i < 16; i++) {
			ByteSwap(&sampleInfo->vadLoopBook.cast.u16[i], SWAP_U16);
		}
		
		sampleInfo->instrument.loop.count = ((u32*)pred->data)[3];
		sampleInfo->instrument.loop.start = ((u32*)pred->data)[1];
		sampleInfo->instrument.loop.end = ((u32*)pred->data)[2];
		ByteSwap(&sampleInfo->instrument.loop.count, SWAP_U32);
		ByteSwap(&sampleInfo->instrument.loop.start, SWAP_U32);
		ByteSwap(&sampleInfo->instrument.loop.end, SWAP_U32);
	}
	
	AudioTools_VadpcmDec(sampleInfo);
}

void Audio_LoadSample_Bin(AudioSampleInfo* sampleInfo) {
	MemFile config = MemFile_Initialize();
	u32 loopEnd;
	u32 tailEnd;
	
	MemFile_LoadFile_ReqExt(&sampleInfo->memFile, sampleInfo->input, ".bin");
	
	Dir_Set(String_GetPath(sampleInfo->input));
	MemFile_LoadFile(&sampleInfo->vadBook, Dir_File("*.book.bin"));
	MemFile_LoadFile_String(&config, Dir_File("config.cfg"));
	
	loopEnd = Config_GetInt(&config, "loop_end");
	tailEnd = Config_GetInt(&config, "tail_end");
	
	sampleInfo->channelNum = 1;
	sampleInfo->bit = 16;
	sampleInfo->sampleRate = gSampleRate;
	sampleInfo->samplesNum = tailEnd ? tailEnd : loopEnd;
	sampleInfo->size = sampleInfo->memFile.dataSize;
	sampleInfo->audio.p = sampleInfo->memFile.data;
	
	sampleInfo->instrument.loop.start = Config_GetInt(&config, "loop_start");
	sampleInfo->instrument.loop.end = Config_GetInt(&config, "loop_end");
	gPrecisionFlag = Config_GetInt(&config, "codec");
	sampleInfo->instrument.loop.count = sampleInfo->instrument.loop.start ? -1 : 0;
	
	// Thanks Sauraen!
	f32 y = 12.0f * log(32000.0f * gTuning / (f32)gSampleRate) / log(2.0f);
	
	sampleInfo->instrument.note = 60 - (s32)round(y);
	sampleInfo->instrument.fineTune = (y - round(y)) * __INT8_MAX__;
	printf_debug("%f, %d", y - (s32)round(y), sampleInfo->instrument.fineTune);
	
	sampleInfo->vadBook.cast.u16[0] = ReadBE(sampleInfo->vadBook.cast.u16[1]);
	sampleInfo->vadBook.cast.u16[1] = ReadBE(sampleInfo->vadBook.cast.u16[3]);
	for (s32 i = 0; i < 8 * sampleInfo->vadBook.cast.u16[0] * sampleInfo->vadBook.cast.u16[1]; i++) {
		sampleInfo->vadBook.cast.u16[i + 2] = ReadBE(sampleInfo->vadBook.cast.u16[i + 4]);
	}
	
	AudioTools_VadpcmDec(sampleInfo);
}

void Audio_LoadSample(AudioSampleInfo* sampleInfo) {
	char* keyword[] = {
		".wav",
		".aiff",
		".aifc",
		".bin"
	};
	AudioFunc loadSample[] = {
		Audio_LoadSample_Wav,
		Audio_LoadSample_Aiff,
		Audio_LoadSample_AifcVadpcm,
		Audio_LoadSample_Bin,
		NULL
	};
	
	if (!sampleInfo->input)
		printf_error("Audio_LoadSample: No input file set");
	
	for (s32 i = 0;; i++) {
		if (loadSample[i] == NULL) {
			printf_warning("[%s] does not match expected extensions. This tool supports these extensions as input:", sampleInfo->input);
			for (s32 j = 0; j < ArrayCount(keyword); j++) {
				printf("[%s] ", keyword[j]);
			}
			printf("\n");
			printf_error("Closing.");
		}
		if (StrStr(sampleInfo->input, keyword[i])) {
			printf_info_align("Load Sample", "%s", sampleInfo->input);
			loadSample[i](sampleInfo);
			break;
		}
	}
}

void Audio_SaveSample_Wav(AudioSampleInfo* sampleInfo) {
	WaveHeader header = { 0 };
	WaveInfo info = { 0 };
	WaveDataInfo dataInfo = { 0 };
	WaveInstrumentInfo instrument = { 0 };
	WaveSampleInfo sample = { 0 };
	WaveSampleLoop loop = { 0 };
	MemFile output = MemFile_Initialize();
	
	/* Write chunk headers */ {
		header.chunk.size = 4 +
			sizeof(WaveInfo) +
			sizeof(WaveDataInfo) +
			sampleInfo->size +
			sizeof(WaveInstrumentInfo) +
			sizeof(WaveSampleInfo) +
			sizeof(WaveSampleLoop);
		info.chunk.size = sizeof(WaveInfo) - sizeof(WaveChunk);
		dataInfo.chunk.size = sampleInfo->size;
		instrument.chunk.size = sizeof(WaveInstrumentInfo) - sizeof(WaveChunk) - 1;
		sample.chunk.size = sizeof(WaveSampleInfo) - sizeof(WaveChunk) + sizeof(WaveSampleLoop);
		
		info.format = PCM;
		if (sampleInfo->bit == 32 && sampleInfo->dataIsFloat)
			info.format = IEEE_FLOAT;
		info.channelNum = sampleInfo->channelNum;
		info.sampleRate = sampleInfo->sampleRate;
		info.byteRate = (sampleInfo->sampleRate * sampleInfo->channelNum * sampleInfo->bit) / 8;
		info.blockAlign = sampleInfo->channelNum * (sampleInfo->bit / 8);
		info.bit = sampleInfo->bit;
		
		instrument.note = sampleInfo->instrument.note;
		instrument.fineTune = sampleInfo->instrument.fineTune;
		
		if (sampleInfo->instrument.fineTune < 0) {
			sampleInfo->instrument.note++;
			sampleInfo->instrument.fineTune = 0x7F - Abs(sampleInfo->instrument.fineTune);
		}
		
		sample.unityNote = sampleInfo->instrument.note;
		sample.fineTune = sampleInfo->instrument.fineTune * 2;
		
		instrument.gain = __INT8_MAX__;
		instrument.lowNote = sampleInfo->instrument.lowNote;
		instrument.hiNote = sampleInfo->instrument.highNote;
		instrument.lowNote = 0;
		instrument.hiVel = __INT8_MAX__;
		
		if (sampleInfo->instrument.loop.count) {
			sample.numSampleLoops = 1;
			loop.start = sampleInfo->instrument.loop.start;
			loop.end = sampleInfo->instrument.loop.end;
		}
		
		memcpy(header.chunk.name, "RIFF", 4);
		memcpy(header.format, "WAVE", 4);
		memcpy(info.chunk.name, "fmt ", 4);
		memcpy(dataInfo.chunk.name, "data", 4);
		memcpy(sample.chunk.name, "smpl", 4);
		memcpy(instrument.chunk.name, "inst", 4);
	}
	
	MemFile_Malloc(&output, sampleInfo->size * 2);
	
	MemFile_Write(&output, &header, sizeof(header));
	MemFile_Write(&output, &info, sizeof(info));
	MemFile_Write(&output, &dataInfo, sizeof(dataInfo));
	MemFile_Write(&output, sampleInfo->audio.p, sampleInfo->size);
	MemFile_Write(&output, &instrument, sizeof(instrument));
	MemFile_Write(&output, &sample, sizeof(sample));
	MemFile_Write(&output, &loop, sizeof(loop));
	
	MemFile_SaveFile(&output, sampleInfo->output);
	MemFile_Free(&output);
}

void Audio_SaveSample_Aiff(AudioSampleInfo* sampleInfo) {
	AiffHeader header = { 0 };
	AiffInfo info = { 0 };
	AiffDataInfo dataInfo = { 0 };
	AiffMarker marker[2] = { 0 };
	AiffMarkerInfo markerInfo = { 0 };
	AiffInstrumentInfo instrument = { 0 };
	MemFile output = MemFile_Initialize();
	
	// AIFF 32-bit == s32
	if (sampleInfo->bit == 32 && sampleInfo->dataIsFloat) {
		for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
			sampleInfo->audio.s32[i] = sampleInfo->audio.f32[i] * 2147483000.0f;
		}
	}
	
	Audio_ByteSwap(sampleInfo);
	
	/* Write chunk headers */ {
		info.chunk.size = sizeof(AiffInfo) - sizeof(AiffChunk) - 2;
		dataInfo.chunk.size = sampleInfo->size + 0x8;
		header.chunk.size = info.chunk.size +
			dataInfo.chunk.size +
			sizeof(AiffChunk) +
			sizeof(AiffChunk) + 4;
		
		if (sampleInfo->instrument.loop.count) {
			markerInfo.chunk.size = sizeof(u16) + sizeof(AiffMarker) * 2;
			instrument.chunk.size = sizeof(AiffInstrumentInfo) - sizeof(AiffChunk);
			header.chunk.size += sizeof(AiffChunk) * 2;
			header.chunk.size += markerInfo.chunk.size;
			header.chunk.size += instrument.chunk.size;
			SwapBE(markerInfo.chunk.size);
			SwapBE(instrument.chunk.size);
			WriteBE(instrument.sustainLoop.start, 1);
			WriteBE(instrument.sustainLoop.end, 2);
			WriteBE(instrument.sustainLoop.playMode, 1);
			
			markerInfo.markerNum = 2;
			marker[0].index = 1;
			Audio_ByteSwap_ToHighLow(&sampleInfo->instrument.loop.start, &marker[0].positionH);
			marker[1].index = 2;
			Audio_ByteSwap_ToHighLow(&sampleInfo->instrument.loop.end, &marker[1].positionH);
			
			SwapBE(markerInfo.markerNum);
			SwapBE(marker[0].index);
			SwapBE(marker[1].index);
			
			instrument.baseNote = sampleInfo->instrument.note;
			instrument.detune = sampleInfo->instrument.fineTune;
			instrument.lowNote = sampleInfo->instrument.lowNote;
			instrument.highNote = sampleInfo->instrument.highNote;
			instrument.lowVelocity = 0;
			instrument.highNote = 127;
			instrument.gain = __INT16_MAX__;
			SwapBE(instrument.gain);
			
			memcpy(markerInfo.chunk.name, "MARK", 4);
			memcpy(instrument.chunk.name, "INST", 4);
		}
		
		SwapBE(info.chunk.size);
		SwapBE(dataInfo.chunk.size);
		SwapBE(header.chunk.size);
		
		f80 sampleRate = sampleInfo->sampleRate;
		u8* p = (u8*)&sampleRate;
		
		for (s32 i = 0; i < 10; i++) {
			info.sampleRate[i] = p[9 - i];
		}
		
		info.channelNum = sampleInfo->channelNum;
		Audio_ByteSwap_ToHighLow(&sampleInfo->samplesNum, &info.sampleNumH);
		info.bit = sampleInfo->bit;
		SwapBE(info.channelNum);
		SwapBE(info.bit);
		
		memcpy(header.chunk.name, "FORM", 4);
		memcpy(header.formType, "AIFF", 4);
		memcpy(info.chunk.name, "COMM", 4);
		memcpy(info.compressionType, "NONE", 4);
		memcpy(dataInfo.chunk.name, "SSND", 4);
	}
	
	MemFile_Malloc(&output, sampleInfo->size * 2);
	
	MemFile_Write(&output, &header, sizeof(header));
	MemFile_Write(&output, &info, 0x16 + sizeof(AiffChunk));
	
	if (sampleInfo->instrument.loop.count) {
		MemFile_Write(&output, &markerInfo, 0xA);
		MemFile_Write(&output, &marker[0], sizeof(AiffMarker));
		MemFile_Write(&output, &marker[1], sizeof(AiffMarker));
		MemFile_Write(&output, &instrument, sizeof(instrument));
	}
	
	MemFile_Write(&output, &dataInfo, 16);
	MemFile_Write(&output, sampleInfo->audio.p, sampleInfo->size);
	
	MemFile_SaveFile(&output, sampleInfo->output);
	MemFile_Free(&output);
}

void Audio_SaveSample_Binary(AudioSampleInfo* sampleInfo) {
	MemFile output = MemFile_Initialize();
	u16 emp = 0;
	char buffer[265 * 4];
	
	AudioTools_VadpcmEnc(sampleInfo);
	
	if (gRomMode) {
		Dir_Set(String_GetPath(sampleInfo->input));
		if (Dir_File("*.vadpcm.bin"))
			remove(Dir_File("*.vadpcm.bin"));
		if (sampleInfo->useExistingPred == false)
			if (Dir_File("*.book.bin"))
				remove(Dir_File("*.book.bin"));
		if (Dir_File("*.loopbook.bin"))
			remove(Dir_File("*.loopbook.bin"));
		if (Dir_File("config.cfg"))
			remove(Dir_File("config.cfg"));
	}
	
	MemFile_Malloc(&output, sampleInfo->size * 2);
	
	String_SwapExtension(buffer, sampleInfo->output, sBinName[gBinNameIndex][0]);
	MemFile_Clear(&output);
	MemFile_Write(&output, sampleInfo->audio.p, sampleInfo->size);
	MemFile_SaveFile(&output, buffer);
	printf_info_align("Save Sample", "%s", buffer);
	
	if (sampleInfo->useExistingPred == false) {
		String_SwapExtension(buffer, sampleInfo->output, sBinName[gBinNameIndex][1]);
		MemFile_Clear(&output);
		for (s32 i = 0; i < sampleInfo->vadBook.dataSize / 2; i++) {
			SwapBE(sampleInfo->vadBook.cast.s16[i]);
		}
		MemFile_Write(&output, &emp, sizeof(u16));
		MemFile_Write(&output, &sampleInfo->vadBook.cast.u16[0], sizeof(u16));
		MemFile_Write(&output, &emp, sizeof(u16));
		MemFile_Write(&output, &sampleInfo->vadBook.cast.u16[1], sizeof(u16));
		MemFile_Write(&output, &sampleInfo->vadBook.cast.u16[2], sampleInfo->vadBook.dataSize - sizeof(u16) * 2);
		MemFile_SaveFile(&output, buffer);
		printf_info_align("Save Book", "%s", buffer);
	}
	
	if (sampleInfo->vadLoopBook.data) {
		String_SwapExtension(buffer, sampleInfo->output, sBinName[gBinNameIndex][2]);
		MemFile_Clear(&output);
		
		for (s32 i = 0; i < 16; i++) {
			ByteSwap(&sampleInfo->vadLoopBook.cast.s16[i], SWAP_U16);
		}
		
		MemFile_Write(&output, sampleInfo->vadLoopBook.cast.p, 16 * 2);
		MemFile_SaveFile(&output, buffer);
		printf_info_align("Save Loop Book", "%s", buffer);
	}
	
	MemFile_Clear(&output);
	
	MemFile* config = &output;
	f32 tuning = (f32)((f32)sampleInfo->sampleRate / 32000.0f) * pow(
		pow(2, 1.0 / 12.0),
		60.0 - (f64)sampleInfo->instrument.note +
		0.01 * sampleInfo->instrument.fineTune
	);
	
	Config_WriteTitle_Str(String_GetBasename(sampleInfo->output));
	
	Config_WriteVar_Int("codec", gPrecisionFlag);
	Config_WriteVar_Int("medium", 0);
	Config_WriteVar_Int("bitA", 0);
	Config_WriteVar_Int("bitB", 0);
	
	Config_SPrintf("\n");
	Config_WriteTitle_Str("Loop");
	Config_WriteVar_Int("loop_start", sampleInfo->instrument.loop.start);
	Config_WriteVar_Int("loop_end", sampleInfo->instrument.loop.end);
	Config_WriteVar_Int("loop_count", sampleInfo->instrument.loop.count);
	Config_WriteVar_Int("tail_end", sampleInfo->samplesNum > sampleInfo->instrument.loop.end ? sampleInfo->samplesNum : 0);
	
	Config_SPrintf("\n");
	Config_WriteTitle_Str("Instrument Info");
	Config_WriteVar_Flo("tuning", tuning);
	
	MemFile_SaveFile_String(config, Tmp_Printf("%sconfig.cfg", String_GetPath(sampleInfo->output)));
	MemFile_Free(&output);
}

void Audio_SaveSample_VadpcmC(AudioSampleInfo* sampleInfo) {
	char* basename;
	MemFile output;
	u32 order;
	u32 numPred;
	
	AudioTools_VadpcmEnc(sampleInfo);
	
	MemFile_Malloc(&output, 0x40000);
	order = sampleInfo->vadBook.cast.u16[0];
	numPred = sampleInfo->vadBook.cast.u16[1];
	basename = String_GetBasename(sampleInfo->output);
	basename[0] = toupper(basename[0]);
	
	MemFile_Printf(
		&output,
		"#include \"include/z64.h\"\n\n"
	);
	
	MemFile_Printf(
		&output,
		"AdpcmBook s%sBook = {\n"
		"	%d, %d,\n"
		"	{\n",
		basename,
		order,
		numPred
	);
	
	for (s32 j = 0; j < numPred; j++) {
		for (s32 i = 0; i < 0x10; i++) {
			char* indent[] = {
				"\t",
				" ",
				" ",
				" "
			};
			char* nl[] = {
				"",
				"",
				"",
				"\n"
			};
			char buf[128];
			
			sprintf(buf, "%d,", sampleInfo->vadBook.cast.s16[2 + i + 0x10 * j]);
			MemFile_Printf(&output, "%s%-6s%s", indent[i % 4], buf, nl[i % 4]);
		}
	}
	
	MemFile_Printf(
		&output,
		"	},\n"
		"};\n\n"
	);
	
	u32 loopStateLen = sampleInfo->samplesNum > sampleInfo->instrument.loop.end ? sampleInfo->samplesNum : 0;
	u8* loopStatePtr = (u8*)&loopStateLen;
	
	MemFile_Printf(
		&output,
		"AdpcmLoop s%sLoop = {\n"
		/* start */ "	%d,"
		/* end   */ " %d,\n"
		/* count */ "	%d,\n"
		/* state */ "	{ %d, %d, %d, %d, },\n",
		basename,
		sampleInfo->instrument.loop.start,
		sampleInfo->instrument.loop.end,
		sampleInfo->instrument.loop.count,
		loopStatePtr[3],
		loopStatePtr[2],
		loopStatePtr[1],
		loopStatePtr[0]
	);
	
	if (sampleInfo->vadLoopBook.cast.p != NULL) {
		MemFile_Printf(
			&output,
			"	{\n"
		);
		
		for (s32 i = 0; i < 0x10; i++) {
			MemFile_Printf(
				&output,
				"		%d,\n",
				sampleInfo->vadLoopBook.cast.s16[i]
			);
		}
		MemFile_Printf(
			&output,
			"	},\n"
		);
	}
	MemFile_Printf(
		&output,
		"};\n\n"
	);
	
	MemFile_Printf(
		&output,
		"SoundFontSample s%sSample = {\n"
		/* codec  */ "	0,"
		/* medium */ " 0,"
		/* unk    */ " 0,"
		/* unk    */ " 0,\n"
		/* size   */ "	%d,\n"
		/* loop   */ "	&s%sLoop,\n"
		/* book   */ "	&s%sBook,\n"
		"};\n\n",
		basename,
		sampleInfo->size,
		basename,
		basename
	);
	
	MemFile_Printf(
		&output,
		"SoundFontSound s%sSound = {\n"
		"	&s%sSample,\n"
		"	%ff\n"
		"};\n",
		basename,
		basename,
		((f32)sampleInfo->sampleRate / 32000.0f) * pow(
			pow(2, 1.0 / 12.0),
			60.0 - (f64)sampleInfo->instrument.note +
			0.01 * sampleInfo->instrument.fineTune
		)
	);
	
	MemFile_SaveFile_String(&output, sampleInfo->output);
	MemFile_Free(&output);
	Audio_SaveSample_Binary(sampleInfo);
}

void Audio_SaveSample(AudioSampleInfo* sampleInfo) {
	char* keyword[] = {
		".wav",
		".aiff",
		".c",
		".bin",
	};
	AudioFunc saveSample[] = {
		Audio_SaveSample_Wav,
		Audio_SaveSample_Aiff,
		Audio_SaveSample_VadpcmC,
		Audio_SaveSample_Binary,
		NULL
	};
	char* funcName[] = {
		"Audio_SaveSample_Wav",
		"Audio_SaveSample_Aiff",
		"Audio_SaveSample_VadpcmC",
		"Audio_SaveSample_Binary"
	};
	
	if (!sampleInfo->output)
		printf_error("Audio_LoadSample: No output file set");
	
	for (s32 i = 0; i < ArrayCount(saveSample); i++) {
		if (saveSample[i] == NULL) {
			printf_warning("[%s] does not match expected extensions. This tool supports these extensions as output:", sampleInfo->output);
			for (s32 j = 0; j < ArrayCount(keyword); j++) {
				printf("[%s] ", keyword[j]);
			}
			printf("\n");
			printf_error("Closing.");
		}
		
		if (StrStr(sampleInfo->output, keyword[i])) {
			printf_debugExt("%s", funcName[i]);
			
			saveSample[i](sampleInfo);
			if (i != 3)
				printf_info_align("Save Sample", "%s", sampleInfo->output);
			printf_debug("Loop: [%X] - [%X]", sampleInfo->instrument.loop.start, sampleInfo->instrument.loop.end);
			break;
		}
	}
}

void Audio_ZZRTLMode(AudioSampleInfo* sampleInfo, char* input) {
	if (!MemMem(input, strlen(input), ".zzrpl", 6)) {
		printf_error("[%s] does not appear to be [zzrpl] file.", input);
	}
}