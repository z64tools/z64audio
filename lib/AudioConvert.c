#include "AudioConvert.h"
#include "HermosauhuLib.h"

void Audio_ByteSwapData(AudioSampleInfo* sampleInfo) {
	printf_debug("Audio_ByteSwapData: SampleNum 0x%X", sampleInfo->samplesNum);
	printf_debug("Audio_ByteSwapData: Size 0x%X", sampleInfo->size);
	
	if (sampleInfo->audio.data == NULL) {
		printf_error("Audio_ByteSwapData: Can't swap audio, data is NULL");
	}
	
	for (s32 i = 0; i < sampleInfo->samplesNum; i++) {
		Lib_ByteSwap(&sampleInfo->audio.data32[i], sampleInfo->bit / 8);
	}
	
	printf_debug("Audio_ByteSwapData: Swap done");
}

u32 Audio_ConvertFloat80(AiffInfo* aiffInfo) {
	f80 float80 = 0;
	u8* pointer = (u8*)&float80;
	
	for (s32 i = 0; i < 10; i++) {
		pointer[i] = (u8)aiffInfo->sampleRate[9 - i];
	}
	
	return (s32)float80;
}

void Audio_LoadWav_GetSampleInfo(WaveInstrumentInfo** waveInstInfo, WaveSampleInfo** waveSampleInfo, WaveHeader* header, u32 fileSize) {
	WaveInfo* wavInfo = Lib_MemMem(header, fileSize, "fmt ", 4);
	WaveDataInfo* wavData = Lib_MemMem(header, fileSize, "data", 4);
	u32 startOffset = sizeof(WaveHeader) + sizeof(WaveChunk) + wavInfo->chunk.size + sizeof(WaveChunk) + wavData->chunk.size;
	u32 searchLength = header->chunk.size - startOffset;
	u8* hayStart = ((u8*)header) + startOffset;
	
	printf_debug("Audio_LoadWav_GetSampleInfo: StartOffset  [0x%08X]", startOffset);
	printf_debug("Audio_LoadWav_GetSampleInfo: SearchLength [0x%08X]", searchLength);
	
	*waveInstInfo = Lib_MemMem(hayStart, searchLength, "inst", 4);
	*waveSampleInfo = Lib_MemMem(hayStart, searchLength, "smpl", 4);
	
	if (*waveInstInfo) {
		printf_debug("Audio_LoadWav_GetSampleInfo: Found WaveInstrumentInfo");
	}
	
	if (*waveSampleInfo) {
		printf_debug("Audio_LoadWav_GetSampleInfo: Found WaveSampleInfo");
	}
}

void Audio_LoadAiff_GetSampleInfo(AiffInstrumentInfo** aiffInstInfo, AiffMarkerInfo** aiffMarkerInfo, AiffHeader* header, u32 fileSize) {
	AiffInfo* aiffInfo = Lib_MemMem(header, fileSize, "COMM", 4);
	u32 startOffset = sizeof(AiffHeader) + aiffInfo->chunk.size;
	u32 searchLength = header->chunk.size - startOffset;
	u8* hayStart = ((u8*)header) + startOffset;
	
	printf_debug("Audio_LoadAiff_GetSampleInfo: StartOffset  [0x%08X]", startOffset);
	printf_debug("Audio_LoadAiff_GetSampleInfo: SearchLength [0x%08X]", searchLength);
	
	*aiffInstInfo = Lib_MemMem(hayStart, searchLength, "INST", 4);
	*aiffMarkerInfo = Lib_MemMem(hayStart, searchLength, "MARK", 4);
	
	if (*aiffInstInfo) {
		printf_debug("Audio_LoadAiff_GetSampleInfo: Found WaveInstrumentInfo");
	}
	
	if (*aiffMarkerInfo) {
		printf_debug("Audio_LoadAiff_GetSampleInfo: Found WaveSampleInfo");
	}
}

void Audio_LoadWav(void** dst, char* file, AudioSampleInfo* sampleInfo) {
	u32 fileSize = File_LoadToMem_ReqExt(dst, file, ".wav");
	WaveHeader* waveHeader = *dst;
	WaveDataInfo* waveData;
	WaveInfo* waveInfo;
	
	printf_debug("Audio_LoadWav: File [%s] loaded to memory", file);
	
	if (fileSize == 0)
		printf_error("Audio_LoadWav: Something has gone wrong loading file [%s]", file);
	waveInfo = Lib_MemMem(waveHeader, fileSize, "fmt ", 4);
	waveData = Lib_MemMem(waveHeader, fileSize, "data", 4);
	if (!Lib_MemMem(waveHeader->chunk.name, 4, "RIFF", 4))
		printf_error("Audio_LoadWav: Chunk header [%c%c%c%c] instead of [RIFF]",waveHeader->chunk.name[0],waveHeader->chunk.name[1],waveHeader->chunk.name[2],waveHeader->chunk.name[3]);
	if (!waveInfo || !waveData) {
		if (!waveData) {
			printf_error(
				"Audio_LoadWav: could not locate [data] from [%s]",
				file
			);
		}
		if (!waveInfo) {
			printf_error(
				"Audio_LoadWav: could not locate [fmt] from [%s]",
				file
			);
		}
	}
	
	sampleInfo->channelNum = waveInfo->channelNum;
	sampleInfo->bit = waveInfo->bit;
	sampleInfo->sampleRate = waveInfo->sampleRate;
	sampleInfo->samplesNum = waveData->chunk.size / (waveInfo->bit / 8) / waveInfo->channelNum;
	sampleInfo->size = waveData->chunk.size;
	sampleInfo->audio.data = waveData->data;
	sampleInfo->instrument.loop.end = sampleInfo->samplesNum;
	
	WaveInstrumentInfo* waveInstInfo;
	WaveSampleInfo* waveSampleInfo;
	
	Audio_LoadWav_GetSampleInfo(&waveInstInfo, &waveSampleInfo, waveHeader, fileSize);
	Audio_ByteSwapData(sampleInfo);
	
	if (waveInstInfo) {
		sampleInfo->instrument.fineTune = waveInstInfo->fineTune;
		sampleInfo->instrument.note = waveInstInfo->note;
		sampleInfo->instrument.highNote = waveInstInfo->hiNote;
		sampleInfo->instrument.lowNote = waveInstInfo->lowNote;
		
		if (waveSampleInfo && waveSampleInfo->numSampleLoops) {
			sampleInfo->instrument.loop.start = waveSampleInfo->loopData[0].start;
			sampleInfo->instrument.loop.end = waveSampleInfo->loopData[0].end;
			sampleInfo->instrument.loop.count = waveSampleInfo->loopData[0].count;
		}
	}
}

void Audio_LoadAiff(void** dst, char* file, AudioSampleInfo* sampleInfo) {
	u32 fileSize = File_LoadToMem_ReqExt(dst, file, ".aiff");
	AiffHeader* aiffHeader = *dst;
	AiffDataInfo* aiffData;
	AiffInfo* aiffInfo;
	
	printf_debug("Audio_LoadAiff: File [%s] loaded to memory", file);
	
	if (fileSize == 0)
		printf_error("Audio_LoadAiff: Something has gone wrong loading file [%s]", file);
	aiffInfo = Lib_MemMem(aiffHeader, fileSize, "COMM", 4);
	aiffData = Lib_MemMem(aiffHeader, fileSize, "SSND", 4);
	if (!Lib_MemMem(aiffHeader->formType, 4, "AIFF", 4)) {
		printf_error(
			"Audio_LoadAiff: Chunk header [%c%c%c%c] instead of [AIFF]",
			aiffHeader->formType[0],
			aiffHeader->formType[1],
			aiffHeader->formType[2],
			aiffHeader->formType[3]
		);
	}
	if (!aiffInfo || !aiffData) {
		if (!aiffData) {
			printf_error(
				"Audio_LoadAiff: could not locate [SSND] from [%s]",
				file
			);
		}
		if (!aiffInfo) {
			printf_error(
				"Audio_LoadAiff: could not locate [COMM] from [%s]",
				file
			);
		}
	}
	
	// Swap Chunk Sizes
	Lib_ByteSwap(&aiffHeader->chunk.size, SWAP_U32);
	Lib_ByteSwap(&aiffInfo->chunk.size, SWAP_U32);
	Lib_ByteSwap(&aiffData->chunk.size, SWAP_U32);
	
	// Swap INFO
	Lib_ByteSwap(&aiffInfo->channelNum, SWAP_U16);
	Lib_ByteSwap(&aiffInfo->sampleNumH, SWAP_U16);
	Lib_ByteSwap(&aiffInfo->sampleNumL, SWAP_U16);
	Lib_ByteSwap(&aiffInfo->bit, SWAP_U16);
	
	// Swap DATA
	Lib_ByteSwap(&aiffData->offset, SWAP_U32);
	Lib_ByteSwap(&aiffData->blockSize, SWAP_U32);
	
	u32 sampleNum = aiffInfo->sampleNumL + (aiffInfo->sampleNumH << 8);
	
	sampleInfo->channelNum = aiffInfo->channelNum;
	sampleInfo->bit = aiffInfo->bit;
	sampleInfo->sampleRate = Audio_ConvertFloat80(aiffInfo);
	sampleInfo->samplesNum = sampleNum;
	sampleInfo->size = fileSize;
	sampleInfo->audio.data = aiffData->data;
	
	sampleInfo->instrument.loop.end = sampleInfo->samplesNum;
	
	AiffInstrumentInfo* aiffInstInfo;
	AiffMarkerInfo* aiffMarkerInfo;
	
	Audio_LoadAiff_GetSampleInfo(&aiffInstInfo, &aiffMarkerInfo, aiffHeader, fileSize);
	
	if (aiffInstInfo) {
		sampleInfo->instrument.note = aiffInstInfo->baseNote;
		sampleInfo->instrument.fineTune = aiffInstInfo->detune;
		sampleInfo->instrument.highNote = aiffInstInfo->highNote;
		sampleInfo->instrument.lowNote = aiffInstInfo->lowNote;
	}
	
	if (aiffMarkerInfo && aiffInstInfo && aiffInstInfo->sustainLoop.playMode >= 1) {
		s32 startIndex = aiffInstInfo->sustainLoop.start - 1;
		s32 endIndex = aiffInstInfo->sustainLoop.end - 1;
		sampleInfo->instrument.loop.start = aiffMarkerInfo->marker[startIndex].position;
		sampleInfo->instrument.loop.end = aiffMarkerInfo->marker[endIndex].position;
		sampleInfo->instrument.loop.count = 0xFFFFFFFF;
	}
}