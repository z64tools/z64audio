#include "AudioConvert.h"
#include "HermosauhuLib.h"

void Audio_ByteSwapFloat80(f80* float80) {
	u8* byteReverse = (void*)float80;
	u8 bswap[10];
	
	for (s32 i = 0; i < 10; i++)
		bswap[i] = byteReverse[ 9 - i];
	
	for (s32 i = 0; i < 10; i++)
		((u8*)float80)[i] = bswap[i];
}

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

void Audio_LoadWav_GetSampleInfo(WaveInstrumentInfo** waveInstInfo, WaveSampleInfo** waveSampleInfo, WaveHeader* header, u32 fileSize) {
	WaveInfo* wavInfo = Lib_MemMem(header, fileSize, "fmt ", 4);
	WaveData* wavData = Lib_MemMem(header, fileSize, "data", 4);
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

void Audio_LoadWav(void** dst, char* file, AudioSampleInfo* sampleInfo) {
	u32 fileSize = File_LoadToMem_ReqExt(dst, file, ".wav");
	WaveHeader* wavHeader = *dst;
	WaveData* wavData;
	WaveInfo* wavInfo;
	WaveInstrumentInfo* waveInstInfo;
	WaveSampleInfo* waveSampleInfo;
	
	printf_debug("Audio_LoadWav: File [%s] loaded to memory", file);
	
	if (fileSize == 0)
		printf_error("Audio_LoadWav: Something has gone wrong loading file [%s]", file);
	wavInfo = Lib_MemMem(wavHeader, fileSize, "fmt ", 4);
	wavData = Lib_MemMem(wavHeader, fileSize, "data", 4);
	if (!Lib_MemMem(wavHeader->chunk.name, 4, "RIFF", 4))
		printf_error("Audio_LoadWav: Chunk header [%c%c%c%c] instead of [RIFF]",wavHeader->chunk.name[0],wavHeader->chunk.name[1],wavHeader->chunk.name[2],wavHeader->chunk.name[3]);
	if (!wavInfo || !wavData) {
		if (!wavData) {
			printf_error(
				"Audio_LoadWav: could not locate [data] from [%s]",
				file
			);
		}
		if (!wavInfo) {
			printf_error(
				"Audio_LoadWav: could not locate [fmt] from [%s]",
				file
			);
		}
	}
	
	sampleInfo->channelNum = wavInfo->channelNum;
	sampleInfo->bit = wavInfo->bit;
	sampleInfo->sampleRate = wavInfo->sampleRate;
	sampleInfo->samplesNum = wavData->chunk.size / (wavInfo->bit / 8);
	sampleInfo->size = wavData->chunk.size;
	sampleInfo->audio.data = wavData->data;
	sampleInfo->instrument = NULL;
	
	Audio_LoadWav_GetSampleInfo(&waveInstInfo, &waveSampleInfo, wavHeader, fileSize);
	Audio_ByteSwapData(sampleInfo);
	
	if (waveInstInfo) {
		sampleInfo->instrument = Lib_Malloc(sizeof(AudioInstrument));
		sampleInfo->instrument->fineTune = waveInstInfo->fineTune;
		sampleInfo->instrument->note = waveInstInfo->note;
		sampleInfo->instrument->highNote = waveInstInfo->hiNote;
		sampleInfo->instrument->lowNote = waveInstInfo->lowNote;
		
		if (waveSampleInfo) {
			sampleInfo->instrument->loop.start = waveSampleInfo->loopData[0].start;
			sampleInfo->instrument->loop.end = waveSampleInfo->loopData[0].end;
			sampleInfo->instrument->loop.count = waveSampleInfo->loopData[0].count;
		}
	}
}