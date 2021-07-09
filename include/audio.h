#ifndef _Z64AUDIO_AUDIO_H_
#define _Z64AUDIO_AUDIO_H_

#include "z64snd.h"

/* Gets ADSR values from Seconds */
ADSR Audio_ADSR(double release) {
	ADSR adsr = { 0 };
	double calcRelease = 255.0f;
	
	calcRelease -= 60.0f * CLAMP(release, 0.0, 2.0);
	calcRelease -= 35 * CLAMP_SUBMIN(release, 2.0, 3.0);
	calcRelease -= 15.4 * CLAMP_SUBMIN(release, 3.0, 8.0);
	calcRelease -= 9.0 * CLAMP_SUBMIN(release, 8.0, 9.0);
	calcRelease -= 2.0 * CLAMP_SUBMIN(release, 9.0, 22.0) / 2;
	calcRelease = floor(calcRelease);
	
	adsr.release = CLAMP(calcRelease, 0, 255);
	
	return adsr;
}

void Audio_Clean(char* file) {
	char buffer[FILENAME_BUFFER] = { 0 };
	char fname[FILENAME_BUFFER] = { 0 };
	char path[FILENAME_BUFFER] = { 0 };
	s32 siz = 0;
	char state[2][8] = {
		"FALSE\0",
		"TRUE\0"
	};
	
	GetFilename(file, fname, path, &siz);
	
	DebugPrint(
		"Cleaning:\n\t%s.aifc\t%s\n\t%s.aiff\t%s\n\t%s.table\t%s",
		fname,
		state[gAudioState.cleanState.aifc],
		fname,
		state[gAudioState.cleanState.aiff],
		fname,
		state[gAudioState.cleanState.table]
	);
	
	if (fname[0] == 0) {
		PrintWarning("Failed to get filename for cleaning.");
		
		return;
	}
	
	if (gAudioState.cleanState.aiff == true) {
		BufferPrint("%s%s.aiff", path, fname);
		if (remove(buffer))
			PrintWarning("%s could not be removed", buffer);
		else
			DebugPrint("%s removed succesfully", buffer);
	}
	
	if (gAudioState.cleanState.aifc == true) {
		BufferPrint("%s%s.aifc", path, fname);
		if (remove(buffer))
			PrintWarning("%s could not be removed", buffer);
		else
			DebugPrint("%s removed succesfully", buffer);
	}
	
	if (gAudioState.cleanState.table == true) {
		BufferPrint("%s%s.table", path, fname);
		if (remove(buffer))
			PrintWarning("%s could not be removed", buffer);
		else
			DebugPrint("%s removed succesfully", buffer);
	}
}

void* Audio_WavDataLoad(const char* filename, u32* size) {
	FILE* getF;
	void* out = 0;
	
	getF = fopen(filename, MODE_READ);
	if (!getF)
		PrintFail("Could not open %s", filename);
	fseek(getF, 0L, SEEK_END);
	*size = ftell(getF);
	rewind(getF);
	
	if ((out = malloc(*size)) == NULL)
		PrintFail("Couldn't malloc %d", *size);
	fread(out, *size, 1, getF);
	fclose(getF);
	
	return out;
}

void Audio_WavConvert(char* filename, s32 iter) {
	FILE* AIFF;
	char* WAV = 0;
	char* wavAudioData = 0;
	WaveChunk* wavDataChunk = 0;
	SampleHeader* smplHeader = 0;
	InstrumentHeader* instHeader = 0;
	WaveHeader* wavHeader = 0;
	
	char fname[FILENAME_BUFFER] = { 0 };
	char path[FILENAME_BUFFER] = { 0 };
	char buffer[FILENAME_BUFFER] = { 0 };
	
	u32 size = 0;
	s32 wowie = 0;
	
	s8 bitFlag, channelFlag = 0;
	
	GetFilename(filename, fname, path, &wowie);
	DebugPrint("filename:  [%s]", fname);
	DebugPrint("path:      %s", path);
	
	WAV = Audio_WavDataLoad(filename, &size);
	wavDataChunk = SearchByteString(WAV, size, "data", 4);
	if (wavDataChunk == NULL)
		PrintFail("Could not find 'data' from WAV-file.");
	
	wavAudioData = (char*)((WaveData*)wavDataChunk)->data;
	
	if (size - ptrDiff(wavAudioData, WAV) > wavDataChunk->size) {
		gAudioState.instDataFlag[iter] = true;
		
		smplHeader = SearchByteString(WAV, size, "smpl", 4);
		if (smplHeader == NULL) {
			PrintWarning("Could not find 'smpl' from WAV-file.");
			gAudioState.instDataFlag[iter] = false;
		} else {
			instHeader = SearchByteString(WAV, size, "inst", 4);
			if (instHeader == NULL) {
				PrintWarning("Could not find 'inst' from WAV-file.");
				gAudioState.instDataFlag[iter] = false;
			}
		}
	}
	
	wavHeader = (WaveHeader*)WAV;
	bitFlag = wavHeader->bitsPerSamp != 16 ? 1 : 0;
	channelFlag = wavHeader->numChannels != 1 ? 1 : 0;
	gAudioState.sampleRate[iter] = wavHeader->sampleRate;
	
	if (wavHeader->bitsPerSamp != 16 && wavHeader->bitsPerSamp != 32)
		PrintFail("Z64Audio does not support %d-bit audio.", wavHeader->bitsPerSamp);
	if (wavHeader->numChannels > 2)
		PrintFail("Z64Audio does not support audio with %d channels.", wavHeader->numChannels);
	
	DebugPrint("WAV:       %d-bits", wavHeader->bitsPerSamp);
	DebugPrint("WAV:       %d-channel(s)", wavHeader->numChannels);
	printf("\n");
	
	BufferPrint("%s%s.aiff", path, fname);
	DebugPrint("[%s]", buffer);
	
	AIFF = fopen(buffer, MODE_WRITE);
	if (!AIFF)
		PrintFail("Output file [%s.aiff] could not be opened", fname);
	
	DebugPrint("File stream opened to %s", buffer);
	
	Chunk formChunk = { 0 };
	ChunkHeader chunkHeader = { 0 };
	CommonChunk commonChunk = { 0 };
	Marker marker[2] = { 0 };
	InstrumentChunk instChunk = { 0 };
	
	DebugPrint("FormChunk");
	{
		formChunk = (Chunk) {
			.ckID = 0x464F524D,    // FORM
			.ckSize = sizeof(Chunk) + sizeof(ChunkHeader) + sizeof(CommonChunk) + sizeof(ChunkHeader),
			.formType = 0x41494646 // AIFF
		};
		u32 chunkSize = wavDataChunk->size;
		
		chunkSize *= channelFlag == true ? 0.5 : 1;
		chunkSize *= bitFlag == true ? 0.5 : 1;
		formChunk.ckSize += chunkSize;
		if (gAudioState.instDataFlag[iter] == true)
			formChunk.ckSize += sizeof(ChunkHeader) + sizeof(Marker) * 2 + sizeof(u16) + sizeof(ChunkHeader) + sizeof(InstrumentChunk);
		
		BSWAP32(formChunk.ckID);
		BSWAP32(formChunk.ckSize);
		BSWAP32(formChunk.formType);
		
		fwrite(&formChunk, sizeof(Chunk), 1, AIFF);
	}
	
	DebugPrint("CommonChunk");
	{
		chunkHeader = (ChunkHeader) {
			.ckID = 0x434F4D4D, // COMM
			.ckSize = sizeof(CommonChunk)
		};
		commonChunk = (CommonChunk) {
			.numChannels = 1,
			.numFramesH = ((wavDataChunk->size & 0xFFFF0000) >> 8) * 0.5,
			.numFramesL = (wavDataChunk->size & 0x0000FFFF) * 0.5,
			.sampleSize = 16,
			.compressionTypeH = 0x4e4f,
			.compressionTypeL = 0x4e45,
		};
		
		u32* numFrames = (u32*)&commonChunk.numFramesH;
		*numFrames = wavDataChunk->size / 2;
		
		if (channelFlag) {
			*numFrames *= 0.5;
		}
		if (bitFlag) {
			*numFrames *= 0.5;
		}
		
		BSWAP32(*numFrames);
		
		float80 longDouble = wavHeader->sampleRate;
		u8* byteReverse = (void*)&longDouble;
		for (s32 i = 0; i < 10; i++)
			commonChunk.sampleRate[i] = byteReverse[ 9 - i];
		
		BSWAP32(chunkHeader.ckID);
		BSWAP32(chunkHeader.ckSize);
		BSWAP16(commonChunk.numChannels);
		BSWAP16(commonChunk.sampleSize);
		BSWAP16(commonChunk.compressionTypeH);
		BSWAP16(commonChunk.compressionTypeL);
		
		fwrite(&chunkHeader, sizeof(ChunkHeader), 1, AIFF);
		fwrite(&commonChunk, sizeof(CommonChunk), 1, AIFF);
	}
	
	if (gAudioState.instDataFlag[iter] == true) {
		DebugPrint("MarkChunk");
		{
			marker[0] = (Marker) {
				.MarkerID = 1,
				.positionH = (smplHeader->loopData[0].start & 0xFFFF0000) >> 8,
				.positionL = (smplHeader->loopData[0].start & 0xFFFF),
			};
			marker[1] = (Marker) {
				.MarkerID = 2,
				.positionH = (smplHeader->loopData[0].end & 0xFFFF0000) >> 8,
				.positionL = (smplHeader->loopData[0].end & 0xFFFF),
			};
			chunkHeader = (ChunkHeader) {
				.ckID = 0x4d41524b, // MARK
				.ckSize = sizeof(Marker) * 2 + sizeof(u16)
			};
			u16 numMarkers = 0x0200;
			
			if (smplHeader->numSampleLoops == 0)
				marker[0].positionH = marker[0].positionL = 0;
			
			BSWAP32(chunkHeader.ckID);
			BSWAP32(chunkHeader.ckSize);
			BSWAP16(marker[0].MarkerID);
			BSWAP16(marker[0].positionL);
			BSWAP16(marker[1].MarkerID);
			BSWAP16(marker[1].positionL);
			
			fwrite(&chunkHeader, sizeof(ChunkHeader), 1, AIFF);
			fwrite(&numMarkers, sizeof(u16), 1, AIFF);
			fwrite(&marker, sizeof(Marker) * 2, 1, AIFF);
		}
		DebugPrint("InstrumentChunk");
		{
			instChunk = (InstrumentChunk) {
				.baseNote = instHeader->note,
				.detune = instHeader->fineTune,
				.lowNote = instHeader->lowNote,
				.highNote = instHeader->hiNote,
				.gain = instHeader->gain,
				.lowVelocity = instHeader->lowVel,
				.highVelocity = instHeader->hiVel,
				.sustainLoop = {
					.playMode = 1,
					.beginLoop = 1,
					.endLoop = 2
				},
				.releaseLoop = {
					.playMode = 0
				}
			};
			chunkHeader = (ChunkHeader) {
				.ckID = 0x494e5354, // INST
				.ckSize = sizeof(InstrumentChunk)
			};
			
			if (smplHeader->numSampleLoops == 0) {
				instChunk.sustainLoop.playMode = 0;
				instChunk.releaseLoop.playMode = 0;
			}
			
			BSWAP32(chunkHeader.ckID);
			BSWAP32(chunkHeader.ckSize);
			BSWAP16(instChunk.sustainLoop.playMode);
			BSWAP16(instChunk.sustainLoop.beginLoop);
			BSWAP16(instChunk.sustainLoop.endLoop);
			
			fwrite(&chunkHeader, sizeof(ChunkHeader), 1, AIFF);
			fwrite(&instChunk, sizeof(InstrumentChunk), 1, AIFF);
		}
	}
	
	DebugPrint("SoundChunk");
	{
		chunkHeader = (ChunkHeader) {
			.ckID = 0x53534E44, // SSND
			.ckSize = wavDataChunk->size
		};
		
		if (channelFlag)
			chunkHeader.ckSize *= 0.5;
		if (bitFlag)
			chunkHeader.ckSize *= 0.5;
		
		chunkHeader.ckSize += 8;
		
		BSWAP32(chunkHeader.ckID);
		BSWAP32(chunkHeader.ckSize);
		fwrite(&chunkHeader, sizeof(ChunkHeader), 1, AIFF);
		
		u64 temp = 0;
		
		fwrite(&temp, sizeof(u64), 1, AIFF);
		
		if (!bitFlag) {
			for (s32 i = 0; i < wavDataChunk->size / 2; i++) {
				s16 tempData = ((s16*)wavAudioData)[i];
				s16 medianator;
				
				if (channelFlag) {
					medianator = ((s16*)wavAudioData)[i + 1];
					tempData = ((float)tempData + (float)medianator) * 0.5;
				}
				
				BSWAP16(tempData);
				fwrite(&tempData, sizeof(s16), 1, AIFF);
				
				if (channelFlag)
					i += 1;
			}
		} else {
			for (s32 i = 0; i < wavDataChunk->size / 2 / 2; i++) {
				float* f32 = (float*)wavAudioData;
				s16 wow = f32[i] * 32766;
				
				if (channelFlag) {
					s16 wow_2 = f32[i + 1] * 32766;
					wow = ((float)wow + (float)wow_2) * 0.5;
					
					i += 1;
				}
				
				BSWAP16(wow);
				
				fwrite(&wow, sizeof(s16), 1, AIFF);
			}
		}
	}
	
	fclose(AIFF);
	
	if (WAV)
		free(WAV);
}

#ifdef _Z64AUDIO_OLD_FUNC_C_
	void Audio_Convert_WavToAiff(char* fileInput, s32 iMain) {
		FILE* f;
		WaveHeader* wav = 0;
		FILE* o;
		s32 bitProcess = 0;
		s32 channelProcess = 0;
		ChunkHeader chunkHeader;
		WaveChunk* wavDataChunk;
		InstrumentHeader* waveInst = 0;
		SampleHeader* waveSmpl = 0;
		s16* data;
		char fname[FILENAME_BUFFER] = { 0 };
		char fname_AIFF[FILENAME_BUFFER] = { 0 };
		char path[FILENAME_BUFFER] = { 0 };
		
		DebugPrint("Audio_Convert_WavToAiff('%s')", fileInput);
		
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
				PrintFail("Malloc Fail");
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
					DebugPrint("%X / %X", (unsigned)ptrDiff(wavDataChunk, wav), 8 + wav->chunk.size);
					PrintFail("SampleChunk: Out of range. Better stop before everything goes bonkers.");
				}
			}
			
			data = ((WaveData*)wavDataChunk)->data;
			
			DebugPrint("ExtraData comparison result: %d > %d", (int)(fsize - ((intptr_t)data - (intptr_t)wav)), wavDataChunk->size);
			if (fsize - ((intptr_t)data - (intptr_t)wav) > wavDataChunk->size) {
				gAudioState.instDataFlag[iMain] = true;
			}
		}
		
		DebugPrint("opening output file '%s'", fname_AIFF);
		if ((o = fopen(fname_AIFF, MODE_WRITE)) == NULL) {
			PrintFail("Output file [%s] could not be opened.", fname_AIFF);
		}
		
		if (gAudioState.instDataFlag[iMain]) {
			waveSmpl = (SampleHeader*)((char*)data + wavDataChunk->size);
			while (!(waveSmpl->chunk.ID[0] == 0x73 && waveSmpl->chunk.ID[1] == 0x6D && waveSmpl->chunk.ID[2] == 0x70 && waveSmpl->chunk.ID[3] == 0x6C)) {
				char* s = (char*)waveSmpl;
				s += 1;
				waveSmpl = (SampleHeader*)s;
				
				if ((u8*)waveSmpl > ((u8*)wav) + 8 + wav->chunk.size)
					PrintFail("SampleInfo: Out of range. Better stop before everything goes bonkers.");
			}
			
			waveInst = (InstrumentHeader*)((char*)data + wavDataChunk->size);
			while (!(waveInst->chunk.ID[0] == 0x69 && waveInst->chunk.ID[1] == 0x6E && waveInst->chunk.ID[2] == 0x73 && waveInst->chunk.ID[3] == 0x74)) {
				char* s = (char*)waveInst;
				s += 1;
				waveInst = (InstrumentHeader*)s;
				
				if ((u8*)waveInst > ((u8*)wav) + 8 + wav->chunk.size)
					PrintFail("InstrumentInfo: Out of range. Better stop before everything goes bonkers.");
			}
		}
		
		DebugPrint("BitRate\t\t\t%d-bit.", wav->bitsPerSamp);
		
		if (wav->bitsPerSamp == 32) {
			bitProcess = true;
		}
		
		if (wav->bitsPerSamp != 16 && wav->bitsPerSamp != 32) {
			DebugPrint("Warning: Non supported bit depth. Try 16-bit or 32-bit.");
		}
		
		DebugPrint("Channels\t\t\t%d", wav->numChannels);
		if (wav->numChannels > 1) {
			channelProcess = true;
			PrintWarning("Warning: Provided WAV is not MONO.");
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
		DebugPrint("FormChunk\t\t\tDone");
		
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
			
			gAudioState.sampleRate[iMain] = wav->sampleRate;
			float80 s = wav->sampleRate;
			u8* wow = (void*)&s;
			for (s32 i = 0; i < 10; i++)
				commonChunk.sampleRate[i] = wow[ 9 - i];
			BSWAP16(commonChunk.numChannels);
			BSWAP16(commonChunk.sampleSize);
			BSWAP16(commonChunk.compressionTypeH);
			BSWAP16(commonChunk.compressionTypeL);
			fwrite(&commonChunk, sizeof(CommonChunk), 1, o);
		}
		DebugPrint("CommonChunk\t\tDone");
		
		if (gAudioState.instDataFlag[iMain]) { /* MARK && INST CHUNK */
			chunkHeader = (ChunkHeader) {
				.ckID = 0x4d41524b,    // MARK
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
			
			if (!waveInst)
				PrintFail("waveInst unintialized line %d", __LINE__);
			
			if (!waveSmpl)
				PrintFail("waveSmpl unintialized line %d", __LINE__);
			
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
			DebugPrint("ExtraChunk\t\t\tDone");
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
		DebugPrint("SoundChunk\t\t\tDone");
		DebugPrint("WAV to AIFF conversion\tOK\n");
		fclose(o);
		if (wav)
			free(wav);
	}
#endif

void Audio_Process(char* argv, s32 iter, ALADPCMloop* _destLoop, InstrumentChunk* _destInst, CommonChunk* _destComm) {
	char fname[FILENAME_BUFFER] = { 0 };
	char path[FILENAME_BUFFER] = { 0 };
	char fname_AIFF[FILENAME_BUFFER] = { 0 };
	char fname_TABLE[FILENAME_BUFFER] = { 0 };
	char fname_AIFC[FILENAME_BUFFER] = { 0 };
	char buffer[FILENAME_BUFFER];
	
	SetFilename(argv,fname,path,fname_AIFF,fname_AIFC,fname_TABLE);
	
	if (gAudioState.ftype == WAV)
		Audio_WavConvert(argv, iter);
	
	if (gAudioState.mode == Z64AUDIOMODE_WAV_TO_AIFF) {
		if (File_TestIfExists(fname_AIFF))
			exit(EXIT_SUCCESS);
		else {
			PrintFail("%s does not exist. Something has gone terribly wrong.", fname_AIFF);
		}
	}
	
	/* run tabledesign */
	{
		#ifdef Z64AUDIO_EXTERNAL_DEPENDENCIES
			BufferPrint("tabledesign -i 30 \"%s\" > \"%s\"", fname_AIFF, fname_TABLE);
			if (system(buffer))
				PrintFail("tabledesign has failed...");
		#else
			strcpy(buffer, fname_TABLE);
			FILE* fp = fopen(buffer, "w");
			char* argv[] = {
				/*0*/ strdup("tabledesign")
				/*1*/, strdup("-i")
				/*2*/, strdup("30")
				/*3*/, buffer
				/*4*/, 0
			};
			BufferPrint("%s", fname_AIFF);
			if (tabledesign(4, argv, fp))
				PrintFail("tabledesign has failed...");
			DebugPrint("%s generated succesfully", fname_TABLE);
			fclose(fp);
			free(argv[0]);
			free(argv[1]);
			free(argv[2]);
		#endif
	}
	
	/* run vadpcm_enc */
	{
		#ifdef Z64AUDIO_EXTERNAL_DEPENDENCIES
			BufferPrint("vadpcm_enc -c \"%s\" \"%s\" \"%s\"", fname_TABLE, fname_AIFF, fname_AIFC);
			if (system(buffer))
				PrintFail("vadpcm_enc has failed...");
		#else
			int i;
			char* argv[] = {
				/*0*/ strdup("vadpcm_enc")
				/*1*/, strdup("-c")
				/*2*/, strdup(fname_TABLE)
				/*3*/, strdup(fname_AIFF)
				/*4*/, strdup(fname_AIFC)
				/*5*/, 0
			};
			if (vadpcm_enc(5, argv))
				PrintFail("vadpcm_enc has failed...");
			DebugPrint("%s converted to %s succesfully", fname_AIFF, fname_AIFC);
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
		PrintFail("malloc fail");
	fread(adpcm, allocSize, 1, f);
	fclose(f);
	
	/* PREDICTOR */
	BufferPrint("%s%s.predictors.bin", path, fname);
	pred = fopen(buffer, MODE_WRITE);
	p += 8;
	while (!(p[-7] == 'C' && p[-6] == 'O' && p[-5] == 'D' && p[-4] == 'E' && p[-3] == 'S')) {
		p++;
		if (ptrDiff(p, adpcm) > allocSize) {
			PrintFail("Out of bounds while searchin:\t'CODES'");
		}
	}
	
	s8 head[] = { 0, 0, 0, p[1], 0, 0, 0, p[3] };
	
	fwrite(&head, sizeof(head), 1, pred);
	p += 4;
	
	while (!(p[0] == 'S' && p[1] == 'S' && p[2] == 'N' && p[3] == 'D')) {
		fwrite(p, 1, 1, pred);
		p++;
		if (ptrDiff(p, adpcm) > allocSize) {
			PrintFail("Out of bounds while searchin:\t'SSND'");
		}
	}
	
	fclose(pred);
	
	/* SAMPLE */
	BufferPrint("%s%s.sample.bin", path, fname);
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
	DebugPrint("%s.predictors.bin\tOK", fname);
	DebugPrint("%s.sample.bin\t\tOK", fname);
	
	ALADPCMloop* loopInfo = 0;
	InstrumentChunk* instData;
	CommonChunk* comm = (CommonChunk*)(adpcm + 0x14);
	int lflag = true;
	
	memcpy(_destComm, comm, sizeof(CommonChunk));
	
	BufferPrint("%s%s.config.tsv", path, fname);
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
		
		BufferPrint("%s%s.looppredictors.bin", path, fname);
		
		FILE* fLoopPred = fopen(buffer, MODE_WRITE);
		
		for (s32 i = 0; i < 16; i++)
			fwrite(&loopInfo->state[i], sizeof(s16), 1, fLoopPred);
		
		fclose(fLoopPred);
	}
	
	p = adpcm + 8;
	while (!(p[-8] == 'I' && p[-7] == 'N' && p[-6] == 'S' && p[-5] == 'T')) {
		p++;
		if (ptrDiff(p, adpcm) > allocSize) {
			PrintFail("Out of bounds while searchin:\t'INST'");
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
	
	DebugPrint("%s.config.tsv\t\tOK\n", fname);
	
	fclose(conf);
	if (adpcm)
		free(adpcm);
}

void Audio_GenerateInstrumentConf(char* file, s32 fileCount, InstrumentChunk* instInfo) {
	char buffer[FILENAME_BUFFER] = { 0 };
	char fname[FILENAME_BUFFER] = { 0 };
	char path[FILENAME_BUFFER] = { 0 };
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
	DebugPrint("Generating inst.tsv");
	
	SetFilename(file, fname, path, NULL, NULL, NULL);
	
	if (path[0] == 0)
		BufferPrint("%s.inst.tsv", fname);
	else
		BufferPrint("%s%s.inst.tsv", path, fname);
	
	float pitch[3] = { 0 };
	
	for (s32 i = 0; i < fileCount; i++) {
		s8 nn = instInfo[i].baseNote % 12;
		u32 f = floor((double)instInfo[i].baseNote / 12);
		
		pitch[i] = ((float)gAudioState.sampleRate[i]) / 32000.0f;
		
		if (gAudioState.instDataFlag[i]) {
			pitch[i] *= pow(pow(2, 1.0 / 12), 60 - instInfo[i].baseNote);
		}
		
		DebugPrint("note %s%d\t%2.1f kHz\t%f", note[nn], (u32)f, (float)gAudioState.sampleRate[i] * 0.001, pitch[i]);
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
			    snprintf(split[2], 4, "%d", instInfo[0].highNote - 21);
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
			    snprintf(split[1], 4, "%d", instInfo[0].lowNote - 21);
			    snprintf(split[2], 4, "%d", instInfo[0].highNote - 21);
		    }
	    } break;
	}
	
	fprintf(conf, "%s\t%s\t%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n", split[0], split[1], split[2], 238, 2, 32700, 1, 32700, 32700, 29430);
	fprintf(conf, "sample1\tpitch1  \tsample2\tpitch2  \tsample3\tpitch3  \n");
	fprintf(conf, "%s\t%s\t%s\t%s\t%s\t%s", sampleID[0], floats[0], sampleID[1], floats[1], sampleID[2], floats[2]);
	
	fclose(conf);
	
	DebugPrint("inst.tsv\t\t\tOK");
}

#endif