#include "AudioConvert.h"
#include "AudioTools.h"

u32 Audio_ConvertBigEndianFloat80(AiffInfo* aiffInfo) {
	f80 float80 = 0;
	u8* pointer = (u8*)&float80;
	
	for (s32 i = 0; i < 10; i++) {
		pointer[i] = (u8)aiffInfo->sampleRate[9 - i];
	}
	
	return (s32)float80;
}
u32 Audio_ByteSwap_FromHighLow(u16* h, u16* l) {
	Lib_ByteSwap(h, SWAP_U16);
	Lib_ByteSwap(l, SWAP_U16);
	
	u8* db = (u8*)h;
	
	OsPrintfEx("%02X%02X%02X%02X == %08X", db[0], db[1], db[2], db[3], (u32)(*h << 16) | *l);
	
	return (*h << 16) | *l;
}
void Audio_ByteSwap_ToHighLow(u32* source, u16* h) {
	h[0] = source[0] >> 16;
	h[1] = source[0];
	
	Lib_ByteSwap(h, SWAP_U16);
	Lib_ByteSwap(&h[1], SWAP_U16);
	
	u8* db = (u8*)h;
	
	OsPrintfEx("%02X%02X%02X%02X == %08X", db[0], db[1], db[2], db[3], *source);
}

void Audio_ByteSwap(AudioSampleInfo* sampleInfo) {
	if (sampleInfo->bit == 16) {
		for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
			Lib_ByteSwap(&sampleInfo->audio.u16[i], sampleInfo->bit / 8);
		}
		
		return;
	}
	if (sampleInfo->bit == 24) {
		// @bug: Seems to some times behave oddly. Look into it later
		printf_warning("ByteSwapping 24-bit audio. This might cause slight issues when converting from AIFF to WAV");
		for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
			Lib_ByteSwap(&sampleInfo->audio.u8[3 * i], sampleInfo->bit / 8);
		}
		
		return;
	}
	if (sampleInfo->bit == 32) {
		for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
			Lib_ByteSwap(&sampleInfo->audio.u32[i], sampleInfo->bit / 8);
		}
		
		return;
	}
}
void Audio_Normalize(AudioSampleInfo* sampleInfo) {
	u32 sampleNum = sampleInfo->samplesNum;
	u32 channelNum = sampleInfo->channelNum;
	f32 max;
	f32 mult;
	
	printf_info("Normalizing.");
	
	if (sampleInfo->bit == 16) {
		max = 0;
		
		for (s32 i = 0; i < sampleNum * channelNum; i++) {
			if (sampleInfo->audio.s16[i] > max) {
				max = ABS(sampleInfo->audio.s16[i]);
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
	
	OsPrintfEx("normalizermax: %f", max);
}
void Audio_ConvertToMono(AudioSampleInfo* sampleInfo) {
	if (sampleInfo->channelNum != 2)
		return;
	
	printf_info("Converting to mono.");
	
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
	if (sampleInfo->targetBit == 16) {
		if (sampleInfo->bit == 24) {
			printf_info("Resampling from 24-bit to 16-bit.");
			for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
				u16 samp = *(u16*)&sampleInfo->audio.u8[3 * i + 1];
				sampleInfo->audio.s16[i] = samp;
			}
			
			sampleInfo->bit = 16;
			sampleInfo->size *= (2.0 / 3.0);
		}
		
		if (sampleInfo->bit == 32) {
			if (sampleInfo->dataIsFloat == true) {
				printf_info("Resampling from 32-bit float to 16-bit.");
				for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
					sampleInfo->audio.s16[i] = sampleInfo->audio.f32[i] * __INT16_MAX__;
				}
				
				sampleInfo->bit = 16;
				sampleInfo->size *= 0.5;
				sampleInfo->dataIsFloat = false;
			} else {
				printf_info("Resampling from 32-bit int to 16-bit.");
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
			// printf_info("Resampling from 32-bit to 16-bit.");
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
	MemFile newMem;
	u32 samplesNum = sampleInfo->samplesNum;
	u32 channelNum = sampleInfo->channelNum;
	
	printf_info("Target bitrate higher than source. Upsampling.");
	OsPrintfEx("Upsampling will iterate %d times. %d x %d", samplesNum * channelNum, samplesNum, channelNum);
	
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
	
	OsPrintfEx("Upsampling done.");
	
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
		OsPrintfEx("samplesNum: [ %8d ] " PRNT_GRAY "[%s]" PRNT_RSET, sampleA->samplesNum, sampleA->input);
		OsPrintf("samplesNum: [ %8d ] " PRNT_GRAY "[%s]" PRNT_RSET, sampleB->samplesNum, sampleB->input);
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
		printf_info("Data matches%s.", addString[flag]);
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

void Audio_GetSampleInfo_Wav(WaveInstrumentInfo** waveInstInfo, WaveSampleInfo** waveSampleInfo, WaveHeader* header, u32 fileSize) {
	WaveInfo* wavInfo = Lib_MemMem(header, fileSize, "fmt ", 4);
	WaveDataInfo* wavData = Lib_MemMem(header, fileSize, "data", 4);
	s32 startOffset = sizeof(WaveHeader) + sizeof(WaveChunk) + wavInfo->chunk.size + sizeof(WaveChunk) + wavData->chunk.size;
	s32 searchLength = header->chunk.size - startOffset;
	u8* hayStart = ((u8*)header) + startOffset;
	
	OsPrintfEx("StartOffset  [0x%08X]", startOffset);
	OsPrintf("SearchLength [0x%08X]", searchLength);
	OsPrintf("FileSize [0x%08X]", fileSize);
	
	if (searchLength <= 0) {
		OsPrintf("searchLength <= 0", fileSize);
		*waveInstInfo = NULL;
		*waveSampleInfo = NULL;
		
		return;
	}
	
	*waveInstInfo = Lib_MemMem(hayStart, searchLength, "inst", 4);
	*waveSampleInfo = Lib_MemMem(hayStart, searchLength, "smpl", 4);
	
	if (*waveInstInfo) {
		OsPrintfEx("Found InstrumentInfo");
	}
	
	if (*waveSampleInfo) {
		OsPrintfEx("Found SampleInfo");
	}
}
void Audio_GetSampleInfo_Aiff(AiffInstrumentInfo** aiffInstInfo, AiffMarkerInfo** aiffMarkerInfo, AiffHeader* header, u32 fileSize) {
	AiffInfo* aiffInfo = Lib_MemMem(header, fileSize, "COMM", 4);
	s32 startOffset = sizeof(AiffHeader) + aiffInfo->chunk.size;
	s32 searchLength = header->chunk.size - startOffset;
	u8* hayStart = ((u8*)header) + startOffset;
	
	OsPrintfEx("StartOffset  [0x%08X]", startOffset);
	OsPrintfEx("SearchLength [0x%08X]", searchLength);
	OsPrintf("FileSize [0x%08X]", fileSize);
	
	if (searchLength <= 0) {
		OsPrintf("searchLength <= 0", fileSize);
		*aiffInstInfo = NULL;
		*aiffMarkerInfo = NULL;
		
		return;
	}
	
	*aiffInstInfo = Lib_MemMem(hayStart, searchLength, "INST", 4);
	*aiffMarkerInfo = Lib_MemMem(hayStart, searchLength, "MARK", 4);
	
	if (*aiffInstInfo) {
		OsPrintfEx("Found InstrumentInfo");
	}
	
	if (*aiffMarkerInfo) {
		OsPrintfEx("Found SampleInfo");
	}
}
void Audio_GetSampleInfo_AifcVadpcm(AiffInstrumentInfo** aiffInstInfo, AiffHeader* header, u32 fileSize) {
	*aiffInstInfo = Lib_MemMem(header, fileSize, "INST", 4);
	
	if (*aiffInstInfo) {
		OsPrintfEx("Found InstrumentInfo");
	}
}

void Audio_LoadSample_Wav(AudioSampleInfo* sampleInfo) {
	MemFile_LoadFile_ReqExt(&sampleInfo->memFile, sampleInfo->input, ".wav");
	WaveHeader* waveHeader = sampleInfo->memFile.data;
	WaveDataInfo* waveData;
	WaveInfo* waveInfo;
	
	if (!sampleInfo->memFile.data) {
		printf_error("Closing.");
	}
	
	if (!Lib_MemMem(waveHeader, 0x10, "WAVE", 4) || !Lib_MemMem(waveHeader, 0x10, "RIFF", 4)) {
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
	
	OsPrintfEx("File [%s] loaded to memory", sampleInfo->input);
	
	if (sampleInfo->memFile.dataSize == 0)
		printf_error("Something has gone wrong loading file [%s]", sampleInfo->input);
	waveInfo = Lib_MemMem(waveHeader, sampleInfo->memFile.dataSize, "fmt ", 4);
	waveData = Lib_MemMem(waveHeader, sampleInfo->memFile.dataSize, "data", 4);
	if (!waveInfo || !waveData) {
		if (!waveData) {
			printf_error(
				"Could not locate [data] from [%s]",
				sampleInfo->input
			);
		}
		if (!waveInfo) {
			printf_error(
				"Could not locate [fmt] from [%s]",
				sampleInfo->input
			);
		}
	}
	
	sampleInfo->channelNum = waveInfo->channelNum;
	sampleInfo->bit = waveInfo->bit;
	sampleInfo->sampleRate = waveInfo->sampleRate;
	sampleInfo->samplesNum = waveData->chunk.size / (waveInfo->bit / 8) / waveInfo->channelNum;
	sampleInfo->size = waveData->chunk.size;
	sampleInfo->audio.p = waveData->data;
	sampleInfo->instrument.loop.end = sampleInfo->samplesNum;
	
	WaveInstrumentInfo* waveInstInfo;
	WaveSampleInfo* waveSampleInfo;
	
	Audio_GetSampleInfo_Wav(&waveInstInfo, &waveSampleInfo, waveHeader, sampleInfo->memFile.dataSize);
	
	if (waveInfo->format == IEEE_FLOAT) {
		printf_info("32-bit Float");
		sampleInfo->dataIsFloat = true;
	}
	if (waveInstInfo) {
		sampleInfo->instrument.fineTune = waveInstInfo->fineTune;
		sampleInfo->instrument.note = waveInstInfo->note;
		sampleInfo->instrument.highNote = waveInstInfo->hiNote;
		sampleInfo->instrument.lowNote = waveInstInfo->lowNote;
		
		if (waveSampleInfo && waveSampleInfo->numSampleLoops) {
			sampleInfo->instrument.loop.start = waveSampleInfo->loopData[0].start;
			sampleInfo->instrument.loop.end = waveSampleInfo->loopData[0].end;
			sampleInfo->instrument.loop.count = 0xFFFFFFFF;
		}
	}
}
void Audio_LoadSample_Aiff(AudioSampleInfo* sampleInfo) {
	MemFile_LoadFile_ReqExt(&sampleInfo->memFile, sampleInfo->input, ".aiff");
	AiffHeader* aiffHeader = sampleInfo->memFile.data;
	AiffDataInfo* aiffData;
	AiffInfo* aiffInfo;
	
	if (!sampleInfo->memFile.data) {
		printf_error("Closing.");
	}
	
	if (!Lib_MemMem(aiffHeader, 0x10, "FORM", 4) || !Lib_MemMem(aiffHeader, 0x10, "AIFF", 4)) {
		char headerA[5] = { 0 };
		char headerB[5] = { 0 };
		
		memcpy(headerA, aiffHeader->chunk.name, 4);
		memcpy(headerB, aiffHeader->formType, 4);
		
		printf_error(
			"[%s] header does not match what's expected\n"
			"\t\tChunkHeader: \e[0;91m[%s]\e[m -> [FORM]\n"
			"\t\tFormat:      \e[0;91m[%s]\e[m -> [AIFF]",
			sampleInfo->input,
			headerA,
			headerB
		);
	}
	
	OsPrintfEx("File [%s] loaded to memory", sampleInfo->input);
	
	if (sampleInfo->memFile.dataSize == 0)
		printf_error("Something has gone wrong loading file [%s]", sampleInfo->input);
	aiffInfo = Lib_MemMem(aiffHeader, sampleInfo->memFile.dataSize, "COMM", 4);
	aiffData = Lib_MemMem(aiffHeader, sampleInfo->memFile.dataSize, "SSND", 4);
	if (!aiffInfo || !aiffData) {
		if (!aiffData) {
			printf_error(
				"Could not locate [SSND] from [%s]",
				sampleInfo->input
			);
		}
		if (!aiffInfo) {
			printf_error(
				"Could not locate [COMM] from [%s]",
				sampleInfo->input
			);
		}
	}
	
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
	
	AiffInstrumentInfo* aiffInstInfo;
	AiffMarkerInfo* aiffMarkerInfo;
	
	Audio_GetSampleInfo_Aiff(&aiffInstInfo, &aiffMarkerInfo, aiffHeader, sampleInfo->memFile.dataSize);
	
	if (aiffInstInfo) {
		sampleInfo->instrument.note = aiffInstInfo->baseNote;
		sampleInfo->instrument.fineTune = aiffInstInfo->detune;
		sampleInfo->instrument.highNote = aiffInstInfo->highNote;
		sampleInfo->instrument.lowNote = aiffInstInfo->lowNote;
	}
	
	if (aiffMarkerInfo && aiffInstInfo && aiffInstInfo->sustainLoop.playMode >= 1) {
		u16 startIndex = ReadBE(aiffInstInfo->sustainLoop.start) - 1;
		u16 endIndex = ReadBE(aiffInstInfo->sustainLoop.end) - 1;
		u16 loopStartH = aiffMarkerInfo->marker[startIndex].positionH;
		u16 loopStartL = aiffMarkerInfo->marker[startIndex].positionL;
		u16 loopEndH = aiffMarkerInfo->marker[endIndex].positionH;
		u16 loopEndL = aiffMarkerInfo->marker[endIndex].positionL;
		
		OsPrintfEx("Loop: loopId %04X loopId %04X Gain %04X", (u16)startIndex, (u16)endIndex, aiffInstInfo->gain);
		
		sampleInfo->instrument.loop.start = Audio_ByteSwap_FromHighLow(&loopStartH, &loopStartL);
		sampleInfo->instrument.loop.end = Audio_ByteSwap_FromHighLow(&loopEndH, &loopEndL);
		sampleInfo->instrument.loop.count = 0xFFFFFFFF;
	}
	
	Audio_ByteSwap(sampleInfo);
}
void Audio_LoadSample_AifcVadpcm(AudioSampleInfo* sampleInfo) {
	MemFile_LoadFile_ReqExt(&sampleInfo->memFile, sampleInfo->input, ".aifc");
	AiffHeader* aiffHeader = sampleInfo->memFile.data;
	AiffDataInfo* aiffData;
	AiffInfo* aiffInfo;
	
	if (!sampleInfo->memFile.data) {
		printf_error("Closing.");
	}
	
	if (!Lib_MemMem(aiffHeader, 0x10, "FORM", 4) || !Lib_MemMem(aiffHeader, 0x10, "AIFC", 4)) {
		char headerA[5] = { 0 };
		char headerB[5] = { 0 };
		
		memcpy(headerA, aiffHeader->chunk.name, 4);
		memcpy(headerB, aiffHeader->formType, 4);
		
		printf_error(
			"[%s] header does not match what's expected\n"
			"\t\tChunkHeader: \e[0;91m[%s]\e[m -> [FORM]\n"
			"\t\tFormat:      \e[0;91m[%s]\e[m -> [AIFC]",
			sampleInfo->input,
			headerA,
			headerB
		);
	}
	
	if (!Lib_MemMem(aiffHeader, 0x90, "VAPC\x0BVADPCM", 10)) {
		printf_error("[%s] does not appear to be using VADPCM compression type.", sampleInfo->input);
	}
	
	OsPrintfEx("File [%s] loaded to memory", sampleInfo->input);
	
	if (sampleInfo->memFile.dataSize == 0)
		printf_error("Audio_LoadSample_AifcVadpcm: Something has gone wrong loading file [%s]", sampleInfo->input);
	aiffInfo = Lib_MemMem(aiffHeader, sampleInfo->memFile.dataSize, "COMM", 4);
	aiffData = Lib_MemMem(aiffHeader, sampleInfo->memFile.dataSize, "SSND", 4);
	if (!aiffInfo || !aiffData) {
		if (!aiffData) {
			printf_error(
				"Could not locate [SSND] from [%s]",
				sampleInfo->input
			);
		}
		if (!aiffInfo) {
			printf_error(
				"Could not locate [COMM] from [%s]",
				sampleInfo->input
			);
		}
	}
	
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
	
	AiffInstrumentInfo* aiffInstInfo = NULL;
	
	Audio_GetSampleInfo_AifcVadpcm(&aiffInstInfo, aiffHeader, sampleInfo->memFile.dataSize);
	
	if (aiffInstInfo) {
		sampleInfo->instrument.note = aiffInstInfo->baseNote;
		sampleInfo->instrument.fineTune = aiffInstInfo->detune;
		sampleInfo->instrument.highNote = aiffInstInfo->highNote;
		sampleInfo->instrument.lowNote = aiffInstInfo->lowNote;
	}
	
	// VADPCM PREDICTORS
	VadpcmInfo* pred = Lib_MemMem(sampleInfo->memFile.data, sampleInfo->memFile.dataSize, "APPL", 4);
	MemFile* vadBook = &sampleInfo->vadBook;
	MemFile* vadLoop = &sampleInfo->vadLoopBook;
	
	if (pred && Lib_MemMem(pred, 24, "VADPCMCODES", 11)) {
		u16 order = ReadBE(((u16*)pred->data)[1]);
		u16 nPred = ReadBE(((u16*)pred->data)[2]);
		u32 predSize = sizeof(u16) * (0x8 * order * nPred + 2);
		
		OsPrintfEx("order: [%d] nPred: [%d] size: [0x%X]", order, nPred, predSize);
		
		MemFile_Malloc(vadBook, predSize);
		MemFile_Write(vadBook, &((u16*)pred->data)[1], predSize);
		
		for (s32 i = 0; i < 0x8 * nPred * order + 2; i++) {
			SwapBE(vadBook->cast.u16[i]);
		}
		
		if (gPrintfSuppress <= PSL_DEBUG) {
			OsPrintfEx("%04X %04X", vadBook->cast.u16[0], vadBook->cast.u16[1]);
			
			for (s32 i = 0; i < nPred * order; i++) {
				OsPrintf(
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
	
	OsPrintfEx("Search Size [0x%X]", size);
	pred = Lib_MemMem(next + sampleInfo->size, size, "APPL", 4);
	
	if (pred && Lib_MemMem(pred, 24, "VADPCMLOOPS", 11)) {
		OsPrintfEx("Found  [VADPCMLOOPS]");
		MemFile_Malloc(&sampleInfo->vadLoopBook, sizeof(s16) * 16);
		MemFile_Write(vadLoop, &((u16*)pred->data)[8], sizeof(s16) * 16);
		for (s32 i = 0; i < 16; i++) {
			Lib_ByteSwap(&sampleInfo->vadLoopBook.cast.u16[i], SWAP_U16);
		}
		
		sampleInfo->instrument.loop.count = ((u32*)pred->data)[3];
		sampleInfo->instrument.loop.start = ((u32*)pred->data)[1];
		sampleInfo->instrument.loop.end = ((u32*)pred->data)[2];
		Lib_ByteSwap(&sampleInfo->instrument.loop.count, SWAP_U32);
		Lib_ByteSwap(&sampleInfo->instrument.loop.start, SWAP_U32);
		Lib_ByteSwap(&sampleInfo->instrument.loop.end, SWAP_U32);
	}
	
	AudioTools_VadpcmDec(sampleInfo);
}
void Audio_LoadSample(AudioSampleInfo* sampleInfo) {
	char* keyword[] = {
		".wav",
		".aiff",
		".aifc",
	};
	AudioFunc loadSample[] = {
		Audio_LoadSample_Wav,
		Audio_LoadSample_Aiff,
		Audio_LoadSample_AifcVadpcm,
		NULL
	};
	
	if (!sampleInfo->input)
		printf_error("Audio_LoadSample: No input file set");
	
	for (s32 i = 0; i < ARRAY_COUNT(loadSample); i++) {
		if (loadSample[i] == NULL) {
			printf_warning("[%s] does not match expected extensions. This tool supports these extensions as input:", sampleInfo->input);
			for (s32 j = 0; j < ARRAY_COUNT(keyword); j++) {
				printf("[%s] ", keyword[j]);
			}
			printf("\n");
			printf_error("Closing.");
		}
		if (Lib_MemMem(sampleInfo->input, strlen(sampleInfo->input), keyword[i], strlen(keyword[i]))) {
			char* basename;
			
			basename = String_GetBasename(sampleInfo->input);
			printf_info("Loading [%s%s]", basename, keyword[i]);
			
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
		instrument.chunk.size = sizeof(WaveInstrumentInfo) - sizeof(WaveChunk);
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
	}
	
	memcpy(header.chunk.name, "RIFF", 4);
	memcpy(header.format, "WAVE", 4);
	memcpy(info.chunk.name, "fmt ", 4);
	memcpy(dataInfo.chunk.name, "data", 4);
	memcpy(sample.chunk.name, "smpl", 4);
	memcpy(instrument.chunk.name, "inst", 4);
	
	FILE* output = fopen(sampleInfo->output, "wb");
	
	if (output == NULL)
		printf_error("Audio_SaveSample_Wav: Could not open outputfile [%s]", sampleInfo->output);
	
	fwrite(&header, 1, sizeof(header), output);
	fwrite(&info, 1, sizeof(info), output);
	fwrite(&dataInfo, 1, sizeof(dataInfo), output);
	fwrite(sampleInfo->audio.p, 1, sampleInfo->size, output);
	fwrite(&instrument, 1, sizeof(instrument), output);
	fwrite(&sample, 1, sizeof(sample), output);
	fwrite(&loop, 1, sizeof(loop), output);
	
	fclose(output);
}
void Audio_SaveSample_Aiff(AudioSampleInfo* sampleInfo) {
	AiffHeader header = { 0 };
	AiffInfo info = { 0 };
	AiffDataInfo dataInfo = { 0 };
	AiffMarker marker[2] = { 0 };
	AiffMarkerInfo markerInfo = { 0 };
	AiffInstrumentInfo instrument = { 0 };
	
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
	
	FILE* output = fopen(sampleInfo->output, "wb");
	
	if (output == NULL)
		printf_error("Audio_SaveSample_Aiff: Could not open outputfile [%s]", sampleInfo->output);
	
	fwrite(&header, 1, sizeof(header), output);
	fwrite(&info, 1, 0x16 + sizeof(AiffChunk), output);
	if (sampleInfo->instrument.loop.count) {
		fwrite(&markerInfo, 1, 0xA, output);
		fwrite(&marker[0], 1, sizeof(AiffMarker), output);
		fwrite(&marker[1], 1, sizeof(AiffMarker), output);
		fwrite(&instrument, 1, sizeof(instrument), output);
	}
	fwrite(&dataInfo, 1, 16, output);
	fwrite(sampleInfo->audio.p, 1, sampleInfo->size, output);
	fclose(output);
}
void Audio_SaveSample_VadpcmC(AudioSampleInfo* sampleInfo) {
	char* basename;
	FILE* output;
	u32 order;
	u32 numPred;
	
	AudioTools_VadpcmEnc(sampleInfo);
	output = fopen(sampleInfo->output, "w");
	order = sampleInfo->vadBook.cast.u16[0];
	numPred = sampleInfo->vadBook.cast.u16[1];
	basename = String_GetBasename(sampleInfo->output);
	basename[0] = toupper(basename[0]);
	
	fprintf(
		output,
		"#include \"include/z64.h\"\n\n"
	);
	
	fprintf(
		output,
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
			fprintf(output, "%s%-6s%s", indent[i % 4], buf, nl[i % 4]);
		}
	}
	
	fprintf(
		output,
		"	},\n"
		"};\n\n"
	);
	
	u32 loopStateLen = sampleInfo->samplesNum > sampleInfo->instrument.loop.end ? sampleInfo->samplesNum : 0;
	u8* loopStatePtr = (u8*)&loopStateLen;
	
	fprintf(
		output,
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
		fprintf(
			output,
			"	{\n"
		);
		
		for (s32 i = 0; i < 0x10; i++) {
			fprintf(
				output,
				"		%d,\n",
				sampleInfo->vadLoopBook.cast.s16[i]
			);
		}
		fprintf(
			output,
			"	},\n"
		);
	}
	fprintf(
		output,
		"};\n\n"
	);
	
	fprintf(
		output,
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
	
	fprintf(
		output,
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
	
	fclose(output);
	printf_info("Saving [%s]", sampleInfo->output);
	
	u16 emp = 0;
	char* path;
	char buffer[265];
	
	path = String_GetPath(sampleInfo->output);
	basename = String_GetBasename(sampleInfo->output);
	String_Copy(buffer, path);
	String_Merge(buffer, basename);
	String_Merge(buffer, "Sample.bin");
	
	output = fopen(buffer, "wb");
	fwrite(sampleInfo->audio.p, 1, sampleInfo->size, output);
	fclose(output);
	printf_info("Saving [%s]", buffer);
	
	String_Copy(buffer, path);
	String_Merge(buffer, basename);
	String_Merge(buffer, "Book.bin");
	output = fopen(buffer, "wb");
	for (s32 i = 0; i < sampleInfo->vadBook.dataSize / 2; i++) {
		Lib_ByteSwap(&sampleInfo->vadBook.cast.s16[i], SWAP_U16);
	}
	fwrite(&emp, sizeof(u16), 1, output);
	fwrite(&sampleInfo->vadBook.cast.u16[0], sizeof(u16), 1, output);
	fwrite(&emp, sizeof(u16), 1, output);
	fwrite(&sampleInfo->vadBook.cast.u16[1], sizeof(u16), 1, output);
	fwrite(&sampleInfo->vadBook.cast.u16[2], 1, sampleInfo->vadBook.dataSize - sizeof(u16) * 2, output);
	fclose(output);
	printf_info("Saving [%s]", buffer);
	
	if (sampleInfo->vadLoopBook.data) {
		String_Copy(buffer, path);
		String_Merge(buffer, basename);
		String_Merge(buffer, "LoopBook.bin");
		output = fopen(buffer, "wb");
		
		for (s32 i = 0; i < 16; i++) {
			Lib_ByteSwap(&sampleInfo->vadLoopBook.cast.s16[i], SWAP_U16);
		}
		
		fwrite(sampleInfo->vadLoopBook.cast.p, 2, 16, output);
		fclose(output);
		printf_info("Saving [%s]", buffer);
	}
}
void Audio_SaveSample(AudioSampleInfo* sampleInfo) {
	char* keyword[] = {
		".wav",
		".aiff",
		".c",
	};
	AudioFunc saveSample[] = {
		Audio_SaveSample_Wav,
		Audio_SaveSample_Aiff,
		Audio_SaveSample_VadpcmC,
		NULL
	};
	char* funcName[] = {
		"Audio_SaveSample_Wav",
		"Audio_SaveSample_Aiff",
		"Audio_SaveSample_VadpcmC"
	};
	
	if (!sampleInfo->output)
		printf_error("Audio_LoadSample: No output file set");
	
	for (s32 i = 0; i < ARRAY_COUNT(saveSample); i++) {
		if (saveSample[i] == NULL) {
			printf_warning("[%s] does not match expected extensions. This tool supports these extensions as output:", sampleInfo->output);
			for (s32 j = 0; j < ARRAY_COUNT(keyword); j++) {
				printf("[%s] ", keyword[j]);
			}
			printf("\n");
			printf_error("Closing.");
		}
		
		if (String_MemMem(sampleInfo->output, keyword[i])) {
			char* basename;
			
			basename = String_GetBasename(sampleInfo->output);
			if (i != 2)
				printf_info("Saving [%s%s]", basename, keyword[i]);
			OsPrintfEx("%s", funcName[i]);
			
			saveSample[i](sampleInfo);
			OsPrintf("Loop: [%X] - [%X]", sampleInfo->instrument.loop.start, sampleInfo->instrument.loop.end);
			break;
		}
	}
}

void Audio_ZZRTLMode(AudioSampleInfo* sampleInfo, char* input) {
	if (!Lib_MemMem(input, strlen(input), ".zzrpl", 6)) {
		printf_error("[%s] does not appear to be [zzrpl] file.", input);
	}
}