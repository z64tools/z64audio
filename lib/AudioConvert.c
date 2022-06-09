#include "AudioConvert.h"
#include "AudioTools.h"
#include <minimp3/minimp3_ex.h>

NameParam gBinNameIndex;
u32 gBinSampleRate = 32000;
u32 gPrecisionFlag;
f32 gTuning = 1.0f;
RawValues gRaw;

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

void Audio_InitSample(AudioSample* sampleInfo, char* input, char* output) {
	memset(sampleInfo, 0, sizeof(*sampleInfo));
	sampleInfo->instrument.note = 60;
	sampleInfo->instrument.highNote = __INT8_MAX__;
	sampleInfo->instrument.lowNote = 0;
	sampleInfo->input = input;
	sampleInfo->output = output;
}

void Audio_FreeSample(AudioSample* sampleInfo) {
	MemFile_Free(&sampleInfo->vadBook);
	MemFile_Free(&sampleInfo->vadLoopBook);
	MemFile_Free(&sampleInfo->memFile);
	
	*sampleInfo = (AudioSample) { 0 };
}

static u32 Audio_ConvertBigEndianFloat80(AiffComm* aiffInfo) {
	f80 float80 = 0;
	u8* pointer = (u8*)&float80;
	
	for (s32 i = 0; i < 10; i++) {
		pointer[i] = (u8)aiffInfo->sampleRate[9 - i];
	}
	
	return (s32)float80;
}

static u32 Audio_ByteSwap_FromHighLow(u16* h, u16* l) {
	ByteSwap(h, SWAP_U16);
	ByteSwap(l, SWAP_U16);
	
	return (*h << 16) | *l;
}

static void Audio_ByteSwap_ToHighLow(u32* source, u16* h) {
	h[0] = source[0] >> 16;
	h[1] = source[0];
	
	ByteSwap(h, SWAP_U16);
	ByteSwap(&h[1], SWAP_U16);
}

static void Audio_ByteSwap(AudioSample* sampleInfo) {
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

static void Audio_BitDepth_Lower(AudioSample* sampleInfo) {
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

static void Audio_BitDepth_Raise(AudioSample* sampleInfo) {
	MemFile newMem = MemFile_Initialize();
	u32 samplesNum = sampleInfo->samplesNum;
	u32 channelNum = sampleInfo->channelNum;
	
	printf_info_align("Upsampling", "%d-bit -> 32-bit", sampleInfo->bit);
	Log("Upsampling will iterate %d times. %d x %d", samplesNum * channelNum, samplesNum, channelNum);
	
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
	
	Log("Upsampling done.");
	
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

void Audio_Mono(AudioSample* sampleInfo) {
	if (sampleInfo->channelNum != 2)
		return;
	
	printf_info_align("Mono", "");
	
	if (sampleInfo->bit == 24) {
		printf_error("%s: 24-bit not supported yet.", __FUNCTION__);
	}
	
	if (sampleInfo->bit == 16) {
		s16 high = 0;
		s16 newHigh = 0;
		f32 mult;
		
		Log("To Mono 16-bit");
		
		for (s32 i = 0, j = 0; i < sampleInfo->samplesNum; i++, j += 2) {
			high = Abs(MaxAbs(MaxAbs(sampleInfo->audio.s16[j], sampleInfo->audio.s16[j + 1]), high));
			sampleInfo->audio.s16[i] = Lerp(0.5, sampleInfo->audio.s16[j], sampleInfo->audio.s16[j + 1]);
			newHigh = Abs(MaxAbs(sampleInfo->audio.s16[i], newHigh));
		}
		
		if (newHigh < high) {
			mult = (f32)high / (f32)newHigh;
			Log("Mult %f, high %d, newHigh %d", mult, high, newHigh);
			
			for (s32 i = 0; i < sampleInfo->samplesNum; i++) {
				sampleInfo->audio.s16[i] = ClampS16(sampleInfo->audio.s16[i] * mult);
			}
		}
	}
	
	if (sampleInfo->bit == 32) {
		if (sampleInfo->dataIsFloat == true) {
			f32 high = 0;
			f32 newHigh = 0;
			f32 mult;
			
			Log("To Mono 32-bit float");
			for (s32 i = 0, j = 0; i < sampleInfo->samplesNum; i++, j += 2) {
				high = Abs(MaxAbs(MaxAbs(sampleInfo->audio.f32[j], sampleInfo->audio.f32[j + 1]), high));
				sampleInfo->audio.f32[i] = Lerp(0.5, sampleInfo->audio.f32[j], sampleInfo->audio.f32[j + 1]);
				newHigh = Abs(MaxAbs(sampleInfo->audio.f32[i], newHigh));
			}
			
			if (newHigh < high) {
				mult = (f32)high / (f32)newHigh;
				
				for (s32 i = 0; i < sampleInfo->samplesNum; i++) {
					sampleInfo->audio.f32[i] *= mult;
				}
			}
		} else {
			s32 high = 0;
			s32 newHigh = 0;
			f32 mult;
			
			Log("To Mono 32-bit int");
			for (s32 i = 0, j = 0; i < sampleInfo->samplesNum; i++, j += 2) {
				high = Abs(MaxAbs(MaxAbs(sampleInfo->audio.s32[j], sampleInfo->audio.s32[j + 1]), high));
				sampleInfo->audio.s32[i] = Lerp(0.5, sampleInfo->audio.s32[j], sampleInfo->audio.s32[j + 1]);
				newHigh = Abs(MaxAbs(sampleInfo->audio.s32[i], newHigh));
			}
			
			if (newHigh < high) {
				mult = (f32)high / (f32)newHigh;
				
				for (s32 i = 0; i < sampleInfo->samplesNum; i++) {
					sampleInfo->audio.s32[i] = ClampS32(sampleInfo->audio.s32[i] * mult);
				}
			}
		}
	}
	
	sampleInfo->size /= 2;
	sampleInfo->channelNum = 1;
}

void Audio_Normalize(AudioSample* sampleInfo) {
	u32 sampleNum = sampleInfo->samplesNum;
	u32 channelNum = sampleInfo->channelNum;
	f32 max = 0;
	f32 mult = 0;
	
	switch (sampleInfo->bit) {
		case 16:
			Log("Bitdepth: 16-bit");
			for (s32 i = 0; i < sampleNum * channelNum; i++) {
				max = Abs(MaxAbs(sampleInfo->audio.s16[i], max));
				
				if (max >= __INT16_MAX__) {
					Log("Max Reached");
					
					return;
				}
			}
			mult = (f32)__INT16_MAX__ / max;
			
			for (s32 i = 0; i < sampleNum * channelNum; i++) {
				sampleInfo->audio.s16[i] *= mult;
			}
			break;
		case 32:
			if (sampleInfo->dataIsFloat) {
				Log("Bitdepth: 32-bit float");
				for (s32 i = 0; i < sampleNum * channelNum; i++) {
					max = Abs(MaxAbs(sampleInfo->audio.f32[i], max));
					
					if (max >= 1.0f) {
						Log("Max Reached");
						
						return;
					}
				}
				
				mult = 1.0f / max;
				
				for (s32 i = 0; i < sampleNum * channelNum; i++) {
					sampleInfo->audio.f32[i] *= mult;
				}
			} else {
				Log("Bitdepth: 32-bit int");
				for (s32 i = 0; i < sampleNum * channelNum; i++) {
					max = Abs(MaxAbs(sampleInfo->audio.s32[i], max));
					
					if (max >= (f32)__INT32_MAX__) {
						Log("Max Reached");
						
						return;
					}
				}
				
				mult = (f32)__INT32_MAX__ / max;
				
				for (s32 i = 0; i < sampleNum * channelNum; i++) {
					sampleInfo->audio.s32[i] *= mult;
				}
			}
			break;
	}
	
	Log("Multiplier: %f", mult);
}

void Audio_BitDepth(AudioSample* sampleInfo) {
	if (sampleInfo->targetBit) {
		if (sampleInfo->targetBit > sampleInfo->bit)
			Audio_BitDepth_Raise(sampleInfo);
		if (sampleInfo->targetBit != sampleInfo->bit)
			Audio_BitDepth_Lower(sampleInfo);
	}
	
	if (sampleInfo->bit == 32 && sampleInfo->dataIsFloat == true && sampleInfo->targetIsFloat == false) {
		for (s32 i = 0; i < sampleInfo->samplesNum * sampleInfo->channelNum; i++) {
			sampleInfo->audio.s32[i] = sampleInfo->audio.f32[i] * __INT32_MAX__;
		}
		
		Log("Converting from 32-float to 32-int");
		
		sampleInfo->dataIsFloat = false;
	}
}

static void* Audio_GetChunk(AudioSample* sampleInfo, const char* type) {
	s8* data = &sampleInfo->memFile.cast.s8[0xC];
	u32 fileSize = sampleInfo->memFile.cast.u32[1];
	u32 isAiff = (StrStrCase(sampleInfo->input, ".aiff") != 0);
	
	if (isAiff) SwapBE(fileSize);
	
	for (s32 i = 1;; i++) {
		struct {
			char type[4];
			u32  size;
		} chunkHead;
		
		memcpy(&chunkHead, data, 0x8);
		if (isAiff) SwapBE(chunkHead.size);
		if (MemMem(type, 4, chunkHead.type, 4)) {
			Log("%d - Chunk: %.4s Offset: 0x%08X", i, chunkHead.type, (uPtr)data - (uPtr)sampleInfo->memFile.data);
			
			return data;
		}
		
		data += 0x8 + chunkHead.size;
		
		// In case of padding between chunks
		while (data[0] == 0) {
			data++;
			if ((uPtr)data - (uPtr)sampleInfo->memFile.data >= fileSize)
				return NULL;
		}
		
		if ((uPtr)data - (uPtr)sampleInfo->memFile.data >= fileSize)
			return NULL;
	}
}

void Audio_LoadSample_Wav(AudioSample* sampleInfo) {
	MemFile_LoadFile(&sampleInfo->memFile, sampleInfo->input);
	WaveHeader* waveHeader = sampleInfo->memFile.data;
	WaveData* waveData;
	WaveFmt* waveInfo;
	WaveInst* waveInstInfo;
	WaveSmpl* waveSampleInfo;
	
	if (sampleInfo->memFile.data == NULL)
		printf_error("Could not load file. Closing!");
	
	if (!StrStrNum(waveHeader->chunk.name, "RIFF", 4) || !StrStrNum(waveHeader->format, "WAVE", 4))
		printf_error("Expected RIFF WAVE Header: [%.4s] [%.4s]", waveHeader->chunk.name, waveHeader->format);
	if ((waveInfo = Audio_GetChunk(sampleInfo, "fmt ")) == NULL) printf_error("Wave: No [fmt]");
	if ((waveData = Audio_GetChunk(sampleInfo, "data")) == NULL) printf_error("Wave: No [data]");
	waveInstInfo = Audio_GetChunk(sampleInfo, "inst");
	waveSampleInfo = Audio_GetChunk(sampleInfo, "smpl");
	
	sampleInfo->channelNum = waveInfo->channelNum;
	sampleInfo->bit = waveInfo->bit;
	sampleInfo->sampleRate = waveInfo->sampleRate;
	sampleInfo->samplesNum = waveData->chunk.size / (waveInfo->bit / 8) / waveInfo->channelNum;
	sampleInfo->size = waveData->chunk.size;
	sampleInfo->audio.p = waveData->data;
	sampleInfo->instrument.loop.end = sampleInfo->samplesNum;
	
	if (waveInfo->format == IEEE_FLOAT) {
		Log("[%s] 32-BitFloat", sampleInfo->input);
		sampleInfo->dataIsFloat = true;
		if (sampleInfo->targetBit == 0)
			sampleInfo->targetIsFloat = sampleInfo->dataIsFloat;
	}
	
	if (waveInstInfo) {
		sampleInfo->instrument.fineTune = waveInstInfo->fineTune;
		sampleInfo->instrument.note = waveInstInfo->note;
		sampleInfo->instrument.highNote = waveInstInfo->hiNote;
		sampleInfo->instrument.lowNote = waveInstInfo->lowNote;
	}
	
	if (waveSampleInfo && waveSampleInfo->numSampleLoops) {
		WaveSmplLoop* loop = (void*)&waveSampleInfo[1];
		sampleInfo->instrument.loop.start = loop->start;
		sampleInfo->instrument.loop.end = loop->end;
		sampleInfo->instrument.loop.count = 0xFFFFFFFF;
	}
}

void Audio_LoadSample_Aiff(AudioSample* sampleInfo) {
	MemFile_LoadFile(&sampleInfo->memFile, sampleInfo->input);
	AiffHeader* aiffHeader = sampleInfo->memFile.data;
	AiffSsnd* aiffData;
	AiffComm* aiffInfo;
	AiffInst* aiffInstInfo;
	AiffMark* aiffMarkerInfo;
	
	if (sampleInfo->memFile.data == NULL)
		printf_error("Could not load file. Closing!");
	
	if (!StrStrNum(aiffHeader->chunk.name, "FORM", 4) || !StrStrNum(aiffHeader->formType, "AIFF", 4))
		printf_error("Expected FORM AIFF Header: [%.4s] [%.4s]", aiffHeader->chunk.name, aiffHeader->formType);
	if ((aiffInfo = Audio_GetChunk(sampleInfo, "COMM")) == NULL) printf_error("Aiff: No [COMM]");
	if ((aiffData = Audio_GetChunk(sampleInfo, "SSND")) == NULL) printf_error("Aiff: No [SSND]");
	aiffInstInfo = Audio_GetChunk(sampleInfo, "INST");
	aiffMarkerInfo = Audio_GetChunk(sampleInfo, "MARK");
	
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
		
		if (aiffMarkerInfo && aiffInstInfo->sustainLoop.playMode >= 1) {
			u16 startIndex = ReadBE(aiffInstInfo->sustainLoop.start) - 1;
			u16 endIndex = ReadBE(aiffInstInfo->sustainLoop.end) - 1;
			u16 loopStartH = aiffMarkerInfo->marker[startIndex].positionH;
			u16 loopStartL = aiffMarkerInfo->marker[startIndex].positionL;
			u16 loopEndH = aiffMarkerInfo->marker[endIndex].positionH;
			u16 loopEndL = aiffMarkerInfo->marker[endIndex].positionL;
			
			Log("Loop: loopId %04X loopId %04X Gain %04X", (u16)startIndex, (u16)endIndex, aiffInstInfo->gain);
			
			sampleInfo->instrument.loop.start = Audio_ByteSwap_FromHighLow(&loopStartH, &loopStartL);
			sampleInfo->instrument.loop.end = Audio_ByteSwap_FromHighLow(&loopEndH, &loopEndL);
			sampleInfo->instrument.loop.count = 0xFFFFFFFF;
		}
	}
	
	Audio_ByteSwap(sampleInfo);
}

void Audio_LoadSample_Bin(AudioSample* sampleInfo) {
	MemFile config = MemFile_Initialize();
	u32 loopEnd;
	u32 tailEnd;
	char* basename = Basename(Basename(sampleInfo->input));
	char* book;
	
	MemFile_LoadFile(&sampleInfo->memFile, sampleInfo->input);
	
	if (sampleInfo->memFile.data == NULL)
		printf_error("Could not load file. Closing!");
	
	FileSys_Path(Path(sampleInfo->input));
	
	book = FileSys_File(HeapPrint("%s.book.bin", basename));
	if (!Sys_Stat(book))
		book = FileSys_FindFile("*.book.bin");
	if (!Sys_Stat(book))
		printf_error("Could not locate [*.book.bin]");
	if (!Sys_Stat(FileSys_File("config.toml")))
		printf_error("Could not locate [config.toml]");
	
	MemFile_LoadFile(&sampleInfo->vadBook, book);
	MemFile_LoadFile_String(&config, FileSys_File("config.toml"));
	
	loopEnd = Toml_GetInt(&config, "loop_end");
	tailEnd = Toml_GetInt(&config, "tail_end");
	
	sampleInfo->channelNum = 1;
	sampleInfo->bit = 16;
	sampleInfo->sampleRate = gBinSampleRate;
	sampleInfo->size = sampleInfo->memFile.dataSize;
	sampleInfo->samplesNum = tailEnd ? tailEnd : loopEnd;
	sampleInfo->audio.p = sampleInfo->memFile.data;
	
	sampleInfo->instrument.loop.start = Toml_GetInt(&config, "loop_start");
	sampleInfo->instrument.loop.end = Toml_GetInt(&config, "loop_end");
	gPrecisionFlag = Toml_GetInt(&config, "codec");
	sampleInfo->instrument.loop.count = sampleInfo->instrument.loop.start ? -1 : 0;
	
	// Thanks Sauraen!
	f32 y = 12.0f * log(32000.0f * gTuning / (f32)gBinSampleRate) / log(2.0f);
	
	sampleInfo->instrument.note = 60 - (s32)round(y);
	sampleInfo->instrument.fineTune = (y - round(y)) * __INT8_MAX__;
	Log("%f, %d", y - (s32)round(y), sampleInfo->instrument.fineTune);
	
	sampleInfo->vadBook.cast.u16[0] = ReadBE(sampleInfo->vadBook.cast.u16[1]);
	sampleInfo->vadBook.cast.u16[1] = ReadBE(sampleInfo->vadBook.cast.u16[3]);
	for (s32 i = 0; i < 8 * sampleInfo->vadBook.cast.u16[0] * sampleInfo->vadBook.cast.u16[1]; i++) {
		sampleInfo->vadBook.cast.u16[i + 2] = ReadBE(sampleInfo->vadBook.cast.u16[i + 4]);
	}
	
	MemFile_Free(&config);
	FileSys_Free();
	AudioTools_VadpcmDec(sampleInfo);
}

void Audio_LoadSample_Mp3(AudioSample* sampleInfo) {
	mp3dec_t mp3d;
	mp3dec_file_info_t info;
	
	if (mp3dec_load(&mp3d, sampleInfo->input, &info, NULL, NULL))
		printf_error("Failed to load/convert [%s]", sampleInfo->input);
	
	sampleInfo->bit = 16;
	sampleInfo->samplesNum = info.samples / info.channels;
	sampleInfo->channelNum = info.channels;
	sampleInfo->sampleRate = info.hz;
	
	MemFile_Malloc(&sampleInfo->memFile, sizeof(s16) * info.samples * 2);
	MemFile_Write(&sampleInfo->memFile, info.buffer, sizeof(s16) * info.samples);
	
	sampleInfo->audio.p = sampleInfo->memFile.data;
	sampleInfo->size = sampleInfo->memFile.dataSize;
	
	Free(info.buffer);
	
	// Get rid of start padding
	if (true) {
		s32 i = 0;
		for (; i < sampleInfo->samplesNum; i++)
			if (sampleInfo->audio.s16[i] != 0)
				break;
		i -= i % sampleInfo->channelNum;
		
		memmove(sampleInfo->audio.s16, &sampleInfo->audio.s16[i], sampleInfo->size - sizeof(s16) * i);
		sampleInfo->samplesNum -= i / sampleInfo->channelNum;
		sampleInfo->size -= sizeof(s16) * i;
		sampleInfo->memFile.dataSize = sampleInfo->size;
	}
	
	sampleInfo->instrument.loop.end = sampleInfo->samplesNum;
}

void Audio_LoadSample_Raw(AudioSample* sampleInfo) {
	u32 size = 0;
	
	MemFile_LoadFile(&sampleInfo->memFile, sampleInfo->input);
	
	if (gRaw.sampleRate == 0 || gRaw.channelNum == 0 || gRaw.bit == 0)
		printf_error("Raw sample loading requires [raw-rate], [raw-channels], [raw-bit]");
	
	sampleInfo->sampleRate = gRaw.sampleRate;
	sampleInfo->dataIsFloat = gRaw.dataIsFloat;
	sampleInfo->channelNum = gRaw.channelNum;
	sampleInfo->bit = gRaw.bit;
	sampleInfo->size = sampleInfo->memFile.dataSize;
	
	sampleInfo->audio.p = sampleInfo->memFile.data;
	
	if (sampleInfo->bit == 16) {
		size = sizeof(s16) * sampleInfo->channelNum;
	} else if (sampleInfo->bit == 32) {
		size = sizeof(s32) * sampleInfo->channelNum;
	}
	
	if (size == 0)
		printf_error("Size is 0");
	
	sampleInfo->samplesNum = sampleInfo->size / size;
}

void Audio_LoadSample(AudioSample* sampleInfo) {
	char* keyword[] = {
		".wav",
		".aiff",
		".bin",
		".mp3",
		".raw"
	};
	AudioFunc loadSample[] = {
		Audio_LoadSample_Wav,
		Audio_LoadSample_Aiff,
		Audio_LoadSample_Bin,
		Audio_LoadSample_Mp3,
		Audio_LoadSample_Raw,
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
		if (StrEndCase(sampleInfo->input, keyword[i])) {
			Log("Load Sample", "%s", sampleInfo->input);
			printf_info_align("Load Sample", "%s", sampleInfo->input);
			loadSample[i](sampleInfo);
			break;
		}
	}
}

static void Wave_WriteChunk_Riff(AudioSample* sampleInfo, MemFile* output) {
	MemFile_Printf(output, "RIFF____WAVE");
}

static void Wave_WriteChunk_Fmt(AudioSample* sampleInfo, MemFile* output) {
	WaveFmt fmt = { 0 };
	
	fmt.chunk.size = sizeof(WaveFmt) - sizeof(WaveChunk);
	fmt.format = sampleInfo->dataIsFloat ? IEEE_FLOAT : PCM;
	fmt.channelNum = sampleInfo->channelNum;
	fmt.sampleRate = sampleInfo->sampleRate;
	fmt.byteRate = (sampleInfo->sampleRate * sampleInfo->channelNum * sampleInfo->bit) / 8;
	fmt.blockAlign = sampleInfo->channelNum * (sampleInfo->bit / 8);
	fmt.bit = sampleInfo->bit;
	
	memcpy(fmt.chunk.name, "fmt ", 4);
	MemFile_Write(output, &fmt, sizeof(fmt));
}

static void Wave_WriteChunk_Data(AudioSample* sampleInfo, MemFile* output) {
	WaveData data = { 0 };
	
	data.chunk.size = sampleInfo->size;
	
	memcpy(data.chunk.name, "data", 4);
	MemFile_Write(output, &data, 8);
	MemFile_Write(output, sampleInfo->audio.p, sampleInfo->size);
}

static void Wave_WriteChunk_Inst(AudioSample* sampleInfo, MemFile* output) {
	WaveInst inst = { 0 };
	
	inst.chunk.size = sizeof(WaveInst) - sizeof(WaveChunk) - 1;
	inst.note = sampleInfo->instrument.note;
	inst.fineTune = sampleInfo->instrument.fineTune;
	inst.gain = __INT8_MAX__;
	inst.lowNote = sampleInfo->instrument.lowNote;
	inst.hiNote = sampleInfo->instrument.highNote;
	inst.lowNote = 0;
	inst.hiVel = __INT8_MAX__;
	
	memcpy(inst.chunk.name, "inst", 4);
	MemFile_Write(output, &inst, sizeof(inst));
}

static void Wave_WriteChunk_Smpl(AudioSample* sampleInfo, MemFile* output) {
	WaveSmpl sample = { 0 };
	WaveSmplLoop loop = { 0 };
	
	if (sampleInfo->instrument.fineTune < 0) {
		sampleInfo->instrument.note++;
		sampleInfo->instrument.fineTune = 0x7F - Abs(sampleInfo->instrument.fineTune);
	}
	
	sample.chunk.size = sizeof(WaveSmpl) + sizeof(WaveSmplLoop) - sizeof(WaveChunk);
	sample.unityNote = sampleInfo->instrument.note;
	sample.fineTune = sampleInfo->instrument.fineTune * 2;
	
	if (sampleInfo->instrument.loop.count) {
		sample.numSampleLoops = 1;
		loop.start = sampleInfo->instrument.loop.start;
		loop.end = sampleInfo->instrument.loop.end;
	}
	
	memcpy(&sample.manufacturer, "z64audio", 8);
	memcpy(sample.chunk.name, "smpl", 4);
	printf_info("%08X / %08X", MemFile_Write(output, &sample, sizeof(WaveSmpl)), sizeof(WaveSmpl));
	printf_info("%08X / %08X", MemFile_Write(output, &loop, sizeof(WaveSmplLoop)), sizeof(WaveSmplLoop));
}

void Audio_SaveSample_Wav(AudioSample* sampleInfo) {
	MemFile output = MemFile_Initialize();
	
	Log("Malloc %fMB", BinToMb(sampleInfo->size * 2));
	MemFile_Malloc(&output, sampleInfo->size * 2);
	MemFile_Params(&output, MEM_REALLOC, true, MEM_END);
	
	Log("WriteChunk " PRNT_BLUE "RIFF"); Wave_WriteChunk_Riff(sampleInfo, &output);
	Log("WriteChunk " PRNT_BLUE "FMT "); Wave_WriteChunk_Fmt(sampleInfo, &output);
	Log("WriteChunk " PRNT_BLUE "DATA"); Wave_WriteChunk_Data(sampleInfo, &output);
	Log("WriteChunk " PRNT_BLUE "INST"); Wave_WriteChunk_Inst(sampleInfo, &output);
	Log("WriteChunk " PRNT_BLUE "SMPL"); Wave_WriteChunk_Smpl(sampleInfo, &output);
	output.cast.u32[1] = output.dataSize - 0x8;
	
	MemFile_SaveFile(&output, sampleInfo->output);
	MemFile_Free(&output);
}

static void Aiff_WriteChunk_Form(AudioSample* sampleInfo, MemFile* output) {
	MemFile_Printf(output, "FORM____AIFF");
}

static void Aiff_WriteChunk_Comm(AudioSample* sampleInfo, MemFile* output) {
	AiffComm comm = { 0 };
	f80 sampleRate = sampleInfo->sampleRate;
	u8* p = (u8*)&sampleRate;
	
	for (s32 i = 0; i < 10; i++) comm.sampleRate[i] = p[9 - i];
	WriteBE(comm.chunk.size, sizeof(AiffComm) - sizeof(AiffChunk) - 2);
	WriteBE(comm.channelNum, sampleInfo->channelNum);
	WriteBE(comm.bit, sampleInfo->bit);
	Audio_ByteSwap_ToHighLow(&sampleInfo->samplesNum, &comm.sampleNumH);
	
	memcpy(comm.compressionType, "NONE", 4);
	if (sampleInfo->bit == 32 && sampleInfo->dataIsFloat) {
		memcpy(comm.compressionType, "FL32", 4);
		memcpy(&output->cast.u32[2], "AIFC", 4);
	}
	
	memcpy(comm.chunk.name, "COMM", 4);
	MemFile_Write(output, &comm, sizeof(comm) - 2);
}

static void Aiff_WriteChunk_Mark(AudioSample* sampleInfo, MemFile* output) {
	AiffMark markerInfo = { 0 };
	AiffMarkIndex marker[2] = { 0 };
	
	WriteBE(markerInfo.chunk.size, sizeof(u16) + sizeof(AiffMarkIndex) * 2);
	WriteBE(markerInfo.markerNum, 2);
	WriteBE(marker[0].index, 1);
	WriteBE(marker[1].index, 2);
	Audio_ByteSwap_ToHighLow(&sampleInfo->instrument.loop.start, &marker[0].positionH);
	Audio_ByteSwap_ToHighLow(&sampleInfo->instrument.loop.end, &marker[1].positionH);
	
	memcpy(markerInfo.chunk.name, "MARK", 4);
	MemFile_Write(output, &markerInfo, sizeof(markerInfo) - 2);
	MemFile_Write(output, &marker, sizeof(marker));
}

static void Aiff_WriteChunk_Inst(AudioSample* sampleInfo, MemFile* output) {
	AiffInst instrument = { 0 };
	
	instrument.chunk.size = sizeof(AiffInst) - sizeof(AiffChunk);
	instrument.baseNote = sampleInfo->instrument.note;
	instrument.detune = sampleInfo->instrument.fineTune;
	instrument.lowNote = sampleInfo->instrument.lowNote;
	instrument.highNote = sampleInfo->instrument.highNote;
	instrument.lowVelocity = 0;
	instrument.highNote = 127;
	instrument.gain = __INT16_MAX__;
	
	SwapBE(instrument.gain);
	SwapBE(instrument.chunk.size);
	
	if (sampleInfo->instrument.loop.count) {
		WriteBE(instrument.sustainLoop.start, 1);
		WriteBE(instrument.sustainLoop.end, 2);
		WriteBE(instrument.sustainLoop.playMode, 1);
	}
	
	memcpy(instrument.chunk.name, "INST", 4);
	MemFile_Write(output, &instrument, sizeof(instrument));
}

static void Aiff_WriteChunk_Ssnd(AudioSample* sampleInfo, MemFile* output) {
	AiffChunk ssnd = { 0 };
	u32 pad[2] = { 0 };
	
	memcpy(ssnd.name, "SSND", 4);
	WriteBE(ssnd.size, sampleInfo->size + 0x8);
	
	MemFile_Write(output, &ssnd, sizeof(ssnd));
	MemFile_Write(output, &pad, sizeof(pad));
	Audio_ByteSwap(sampleInfo);
	MemFile_Write(output, sampleInfo->audio.p, sampleInfo->size);
}

void Audio_SaveSample_Aiff(AudioSample* sampleInfo) {
	MemFile output = MemFile_Initialize();
	
	Log("Malloc %fMB", BinToMb(sampleInfo->size * 2));
	MemFile_Malloc(&output, sampleInfo->size * 2);
	
	Log("WriteChunk " PRNT_BLUE "FORM"); Aiff_WriteChunk_Form(sampleInfo, &output);
	Log("WriteChunk " PRNT_BLUE "COMM"); Aiff_WriteChunk_Comm(sampleInfo, &output);
	Log("WriteChunk " PRNT_BLUE "MARK"); Aiff_WriteChunk_Mark(sampleInfo, &output);
	Log("WriteChunk " PRNT_BLUE "INST"); Aiff_WriteChunk_Inst(sampleInfo, &output);
	Log("WriteChunk " PRNT_BLUE "SSND"); Aiff_WriteChunk_Ssnd(sampleInfo, &output);
	WriteBE(output.cast.u32[1], output.dataSize - 0x8);
	
	MemFile_SaveFile(&output, sampleInfo->output);
	MemFile_Free(&output);
}

void Audio_SaveSample_Binary(AudioSample* sampleInfo) {
	MemFile output = MemFile_Initialize();
	u16 emp = 0;
	char buffer[265 * 4];
	
	Log("AudioTools_VadpcmEnc(sampleInfo);");
	AudioTools_VadpcmEnc(sampleInfo);
	
	MemFile_Malloc(&output, sampleInfo->size * 2);
	
	String_SwapExtension(buffer, sampleInfo->output, sBinName[gBinNameIndex][0]);
	MemFile_Clear(&output);
	MemFile_Write(&output, sampleInfo->audio.p, sampleInfo->size);
	MemFile_SaveFile(&output, buffer);
	Log("Save Sample", "%s", buffer);
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
		Log("Save Book", "%s", buffer);
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
		Log("Save Loop Book", "%s", buffer);
		printf_info_align("Save Loop Book", "%s", buffer);
	}
	
	MemFile_Clear(&output);
	
	MemFile* config = &output;
	f32 tuning = (f32)((f32)sampleInfo->sampleRate / 32000.0f) * pow(
		pow(2, 1.0 / 12.0),
		60.0 - (f64)sampleInfo->instrument.note +
		0.01 * sampleInfo->instrument.fineTune
	);
	
	char* file;
	
	file = HeapPrint("%sconfig.toml", Path(sampleInfo->output));
	
	if (Sys_Stat(file)) {
		MemFile_LoadFile_String(config, file);
		
		Toml_ReplaceVariable(config, "codec", "%d", gPrecisionFlag);
		Toml_ReplaceVariable(config, "loop_start", "%d", sampleInfo->instrument.loop.start);
		Toml_ReplaceVariable(config, "loop_end", "%d", sampleInfo->instrument.loop.end);
		Toml_ReplaceVariable(config, "loop_count", "%d", sampleInfo->instrument.loop.count);
		Toml_ReplaceVariable(config, "tail_end", "%d", sampleInfo->instrument.loop.oldEnd && sampleInfo->instrument.loop.oldEnd != sampleInfo->instrument.loop.end ? sampleInfo->instrument.loop.oldEnd : 0);
		
		Toml_ReplaceVariable(config, "basenote", "%d", sampleInfo->instrument.note);
		Toml_ReplaceVariable(config, "finetune", "%d", sampleInfo->instrument.fineTune);
		Toml_ReplaceVariable(config, "tuning", "%f", tuning);
	} else {
		Toml_WriteInt(config, "codec", gPrecisionFlag, NO_COMMENT);
		Toml_WriteInt(config, "medium", 0, NO_COMMENT);
		Toml_WriteInt(config, "bitA", 0, NO_COMMENT);
		Toml_WriteInt(config, "bitB", 0, NO_COMMENT);
		
		Toml_Print(config, "\n");
		Toml_WriteInt(config, "loop_start", sampleInfo->instrument.loop.start, NO_COMMENT);
		Toml_WriteInt(config, "loop_end", sampleInfo->instrument.loop.end, NO_COMMENT);
		Toml_WriteInt(config, "loop_count", sampleInfo->instrument.loop.count, NO_COMMENT);
		Toml_WriteInt(config, "tail_end", sampleInfo->instrument.loop.oldEnd && sampleInfo->instrument.loop.oldEnd != sampleInfo->instrument.loop.end ? sampleInfo->instrument.loop.oldEnd : 0, NO_COMMENT);
		
		Toml_Print(config, "\n");
		Toml_WriteInt(config, "basenote", sampleInfo->instrument.note, NO_COMMENT);
		Toml_WriteInt(config, "finetune", sampleInfo->instrument.fineTune, NO_COMMENT);
		Toml_WriteFloat(config, "tuning", tuning, NO_COMMENT);
	}
	
	Log("Save Config");
	MemFile_SaveFile_String(config, file);
	MemFile_Free(&output);
}

void Audio_SaveSample_VadpcmC(AudioSample* sampleInfo) {
	char* basename;
	MemFile output;
	u32 order;
	u32 numPred;
	
	AudioTools_VadpcmEnc(sampleInfo);
	
	MemFile_Malloc(&output, 0x40000);
	order = sampleInfo->vadBook.cast.u16[0];
	numPred = sampleInfo->vadBook.cast.u16[1];
	basename = Basename(sampleInfo->output);
	basename[0] = tolower(basename[0]);
	
	MemFile_Printf(
		&output,
		"#include \"include/z64.h\"\n\n"
	);
	
	MemFile_Printf(
		&output,
		"AdpcmBook %sBook = {\n"
		"	.order       = %d,\n"
		"	.npredictors = %d,\n"
		"	.book        = {\n",
		basename,
		order,
		numPred
	);
	
	for (s32 j = 0; j < numPred; j++) {
		MemFile_Printf(&output, "\t\t");
		for (s32 i = 0; i < 8 * order; i++) {
			if (i % 8 == 0 && i)
				MemFile_Printf(&output, "\n\t\t");
			MemFile_Printf(&output, "%6d,", sampleInfo->vadBook.cast.s16[2 + i + ((8 * order) * j)]);
		}
		MemFile_Printf(&output, "\n");
	}
	
	MemFile_Printf(
		&output,
		"	},\n"
		"};\n\n"
	);
	
	u32 loopStateLen = sampleInfo->instrument.loop.oldEnd && sampleInfo->instrument.loop.oldEnd != sampleInfo->instrument.loop.end ? sampleInfo->instrument.loop.oldEnd : 0;
	u8* loopStatePtr = (u8*)&loopStateLen;
	
	MemFile_Printf(
		&output,
		"AdpcmLoop %sLoop = {\n"
		"	.start  = %d,\n"
		"	.end    = %d,\n"
		"	.count  = %d,\n"
		"	.unk_0C = {\n\t\t0x%X, 0x%X, 0x%X, 0x%X\n\t},\n",
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
			"	.state  = {\n"
		);
		
		MemFile_Printf(
			&output,
			"		%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,\n"
			"		%6d,%6d,%6d,%6d,%6d,%6d,%6d,%6d,\n",
			sampleInfo->vadLoopBook.cast.s16[0],
			sampleInfo->vadLoopBook.cast.s16[1],
			sampleInfo->vadLoopBook.cast.s16[2],
			sampleInfo->vadLoopBook.cast.s16[3],
			sampleInfo->vadLoopBook.cast.s16[4],
			sampleInfo->vadLoopBook.cast.s16[5],
			sampleInfo->vadLoopBook.cast.s16[6],
			sampleInfo->vadLoopBook.cast.s16[7],
			sampleInfo->vadLoopBook.cast.s16[8],
			sampleInfo->vadLoopBook.cast.s16[9],
			sampleInfo->vadLoopBook.cast.s16[10],
			sampleInfo->vadLoopBook.cast.s16[11],
			sampleInfo->vadLoopBook.cast.s16[12],
			sampleInfo->vadLoopBook.cast.s16[13],
			sampleInfo->vadLoopBook.cast.s16[14],
			sampleInfo->vadLoopBook.cast.s16[15]
		);
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
		"SoundFontSample %sSample = {\n"
		"	.codec     = %d,\n"
		"	.medium    = 0,\n"
		"	.unk_bit26 = 0,\n"
		"	.unk_bit25 = 0,\n"
		"	.size      = %d,\n"
		"	.loop      = &%sLoop,\n"
		"	.book      = &%sBook,\n"
		"};\n\n",
		basename,
		gPrecisionFlag,
		sampleInfo->size,
		basename,
		basename
	);
	
	MemFile_Printf(
		&output,
		"SoundFontSound %sSound = {\n"
		"	.sample = &%sSample,\n"
		"	.tuning = %ff\n"
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

void Audio_SaveSample(AudioSample* sampleInfo) {
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
		
		if (StrEndCase(sampleInfo->output, keyword[i])) {
			if (i != 3) {
				Log("Save Sample", "%s", sampleInfo->output);
				printf_info_align("Save Sample", "%s", sampleInfo->output);
			}
			saveSample[i](sampleInfo);
			break;
		}
	}
}

void Audio_Playback(void* ctx, void* output, u32 frameCount) {
	AudioSample* sample = ctx;
	PointerCast data = {
		.p = output
	};
	SampleLoop* loopInfo = &sample->instrument.loop;
	u32 noEscape = 0;
	
	if (sample->doPlay < 1)
		return;
	
	for (s32 i = 0; i < frameCount; i++) {
		if (sample->playFrame >= sample->samplesNum) {
			sample->doPlay = -1;
			break;
		}
		
		for (s32 j = 0; j < sample->channelNum; j++) {
			if (sample->bit == 16)
				data.s16[i * sample->channelNum + j] = sample->audio.s16[sample->playFrame * sample->channelNum + j];
			
			if (sample->bit == 32 && sample->dataIsFloat == false)
				data.s32[i * sample->channelNum + j] = sample->audio.s32[sample->playFrame * sample->channelNum + j];
			
			if (sample->bit == 32 && sample->dataIsFloat == true)
				data.f32[i * sample->channelNum + j] = sample->audio.f32[sample->playFrame * sample->channelNum + j];
		}
		
		sample->playFrame++;
		
		if (sample->selectEnd != sample->selectStart) {
			if (sample->playbackState.repA != sample->selectStart || sample->playbackState.repB != sample->selectEnd)
				noEscape = 1024;
			
			if (IsBetween(sample->playFrame, sample->selectStart, sample->selectEnd + noEscape)) {
				sample->playFrame = WrapS(sample->playFrame, sample->selectStart, sample->selectEnd);
				sample->playbackState.repA = sample->selectStart;
				sample->playbackState.repB = sample->selectEnd;
				continue;
			}
		}
		
		if (loopInfo->count) {
			if (sample->playbackState.repA != loopInfo->start || sample->playbackState.repB != loopInfo->end)
				noEscape = 1024;
			
			if (IsBetween(sample->playFrame, loopInfo->start, loopInfo->end + noEscape)) {
				sample->playFrame = WrapS(sample->playFrame, loopInfo->start, loopInfo->end);
				sample->playbackState.repA = loopInfo->start;
				sample->playbackState.repB = loopInfo->end;
				continue;
			}
		}
	}
}