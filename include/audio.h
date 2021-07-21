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

void Audio_Clean(char* fname, char* path) {
	char buffer[FILENAME_BUFFER] = { 0 };
	
	if (fname[0] == 0) {
		PrintWarning("Failed to get filename for cleaning.");
		
		return;
	}
	
	if (gAudioState.cleanState.aiff == true) {
		BufferPrint("%s%s.aiff", path, fname);
		if (remove(buffer))
			PrintWarning("%s could not be removed", buffer);
	}
	
	if (gAudioState.cleanState.aifc == true) {
		BufferPrint("%s%s.aifc", path, fname);
		if (remove(buffer))
			PrintWarning("%s could not be removed", buffer);
	}
	
	if (gAudioState.cleanState.table == true) {
		BufferPrint("%s%s.table", path, fname);
		if (remove(buffer))
			PrintWarning("%s could not be removed", buffer);
	}
}

void* Audio_WavDataLoad(const char* file, u32* size) {
	FILE* getF;
	void* out = 0;
	
	getF = fopen(file, MODE_READ);
	if (!getF)
		PrintFail("audio.h %d: Could not open %s", __LINE__, file);
	fseek(getF, 0L, SEEK_END);
	*size = ftell(getF);
	rewind(getF);
	
	if ((out = malloc(*size)) == NULL)
		PrintFail("Couldn't malloc %d", *size);
	fread(out, *size, 1, getF);
	fclose(getF);
	
	return out;
}

void Audio_WavConvert(char* file, char* fname, char* path, s32 iter) {
	FILE* AIFF;
	char* WAV = 0;
	char* wavAudioData = 0;
	WaveChunk* wavDataChunk = 0;
	SampleHeader* smplHeader = 0;
	InstrumentHeader* instHeader = 0;
	WaveHeader* wavHeader = 0;
	char buffer[FILENAME_BUFFER] = { 0 };
	u32 size = 0;
	s8 bitDepth, channelFlag = 0;
	u32 frames = 0;
	
	DebugPrint("filename:  [%s]", fname);
	DebugPrint("path:      %s", path);
	
	WAV = Audio_WavDataLoad(file, &size);
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
			
			if (smplHeader->numSampleLoops >= 1) {
				gAudioState.instLoop[iter] = true;
			}
		}
	}
	
	wavHeader = (WaveHeader*)WAV;
	bitDepth = wavHeader->bitsPerSamp;
	channelFlag = wavHeader->numChannels != 1 ? 1 : 0;
	gAudioState.sampleRate[iter] = wavHeader->sampleRate;
	
	if (bitDepth != 16 && bitDepth != 32)
		PrintFail("Z64Audio does not support %d-bit audio.", bitDepth);
	if (wavHeader->numChannels > 2)
		PrintFail("Z64Audio does not support audio with %d channels.", wavHeader->numChannels);
	
	DebugPrint("WAV:       %d-bits", wavHeader->bitsPerSamp);
	DebugPrint("WAV:       %d-channel(s)", wavHeader->numChannels);
	printf("\n");
	
	BufferPrint("%s%s.aiff", path, fname);
	ColorPrint(1, "[%s]", buffer);
	
	AIFF = fopen(buffer, MODE_WRITE);
	if (!AIFF)
		PrintFail("Output file [%s.aiff] could not be opened", fname);
	
	DebugPrint("File stream opened!");
	
	Chunk formChunk = { 0 };
	ChunkHeader chunkHeader = { 0 };
	CommonChunk commonChunk = { 0 };
	Marker marker[2] = { 0 };
	InstrumentChunk instChunk = { 0 };
	
	ColorPrint(0, "FormChunk");
	{
		formChunk = (Chunk) {
			.ckID = 0x464F524D,    // FORM
			.ckSize = sizeof(Chunk) + sizeof(ChunkHeader) + sizeof(CommonChunk) + sizeof(ChunkHeader),
			.formType = 0x41494646 // AIFF
		};
		u32 chunkSize = wavDataChunk->size;
		
		chunkSize *= channelFlag == true ? 0.5 : 1;
		chunkSize *= bitDepth == 32 ? 0.5 : 1;
		formChunk.ckSize += chunkSize;
		if (gAudioState.instDataFlag[iter] == true)
			formChunk.ckSize += sizeof(ChunkHeader) + sizeof(Marker) * 2 + sizeof(u16) + sizeof(ChunkHeader) + sizeof(InstrumentChunk);
		
		BSWAP32(formChunk.ckID);
		BSWAP32(formChunk.ckSize);
		BSWAP32(formChunk.formType);
		
		fwrite(&formChunk, sizeof(Chunk), 1, AIFF);
	}
	
	ColorPrint(0, "CommonChunk");
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
		*numFrames *= bitDepth == 32 ? 0.5 : 1;
		frames = *numFrames;
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
		ColorPrint(0, "MarkChunk");
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
		ColorPrint(0, "InstrumentChunk");
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
	
	ColorPrint(0, "SoundChunk");
	{
		u64 temp = 0;
		chunkHeader = (ChunkHeader) {
			.ckID = 0x53534E44, // SSND
			.ckSize = wavDataChunk->size
		};
		
		if (channelFlag)
			chunkHeader.ckSize *= 0.5;
		chunkHeader.ckSize *= bitDepth == 32 ? 0.5 : 1;
		
		chunkHeader.ckSize += 8;
		
		BSWAP32(chunkHeader.ckID);
		BSWAP32(chunkHeader.ckSize);
		fwrite(&chunkHeader, sizeof(ChunkHeader), 1, AIFF);
		fwrite(&temp, sizeof(u64), 1, AIFF);
		
		s32 mallSize = frames;
		float* audioData;
		float max = 0;
		
		// mallSize *= bitDepth == 32 ? 0.5 : 1;
		audioData = malloc(sizeof(float) * (mallSize) * 2);
		
		if (bitDepth == 16) {
			s32 a = 0;
			for (s32 i = 0; a < frames; i++) {
				float tempData = ((s16*)wavAudioData)[i];
				float medianator;
				
				if (channelFlag) {
					medianator = ((s16*)wavAudioData)[i + 1];
					tempData = (tempData + medianator) * 0.5;
				}
				
				if (ABS(tempData) > max)
					max = ABS(tempData);
				
				audioData[a] = tempData;
				
				if (channelFlag)
					i++;
				a++;
			}
			
			for (s32 i = 0; i < frames; i++) {
				// Normalize
				if (ABS(max) < 32767 || ABS(max) > 32768) {
					float sample = audioData[i];
					sample *= 32767.0 / max;
					sample = CLAMP(sample, -32767, 32768);
					audioData[i] = sample;
				}
				
				s16 smpl = audioData[i];
				BSWAP16(smpl);
				fwrite(&smpl, sizeof(s16), 1, AIFF);
			}
		} else if (bitDepth == 32) {
			s32 a = 0;
			for (s32 i = 0; a < frames; i++) {
				float* f32 = (float*)wavAudioData;
				float tempData = f32[i] * 32766;
				
				if (channelFlag) {
					float wow_2 = f32[i + 1] * 32766;
					tempData = ((float)tempData + (float)wow_2) * 0.5;
				}
				
				if (ABS(tempData) > max)
					max = ABS(tempData);
				
				audioData[a] = tempData;
				
				if(channelFlag)
					i++;
				a++;
			}
			
			for (s32 i = 0; i < frames; i++) {
				// Normalize
				if (ABS(max) < 32767 || ABS(max) > 32768) {
					float sample = audioData[i];
					sample *= 32767.0 / max;
					sample = CLAMP(sample, -32767, 32768);
					audioData[i] = sample;
				}
				
				s16 smpl = audioData[i];
				BSWAP16(smpl);
				fwrite(&smpl, sizeof(s16), 1, AIFF);
			}
		}
		
		if (audioData)
			free(audioData);
	}
	
	{
		rewind(AIFF);
		formChunk = (Chunk) {
			.ckID = 0x464F524D,    // FORM
			.ckSize = sizeof(Chunk) + sizeof(ChunkHeader) + sizeof(CommonChunk) + sizeof(ChunkHeader),
			.formType = 0x41494646 // AIFF
		};
		u32 chunkSize = frames * sizeof(s16);
		
		formChunk.ckSize += chunkSize;
		if (gAudioState.instDataFlag[iter] == true)
			formChunk.ckSize += sizeof(ChunkHeader) + sizeof(Marker) * 2 + sizeof(u16) + sizeof(ChunkHeader) + sizeof(InstrumentChunk);
		
		BSWAP32(formChunk.ckID);
		BSWAP32(formChunk.ckSize);
		BSWAP32(formChunk.formType);
		
		fwrite(&formChunk, sizeof(Chunk), 1, AIFF);
	}
	
	fclose(AIFF);
	
	if (WAV)
		free(WAV);
	
	BufferPrint("%s%s.aiff", path, fname);
	if (gAudioState.mode == Z64AUDIOMODE_WAV_TO_AIFF) {
		if (File_TestIfExists(buffer))
			exit(EXIT_SUCCESS);
		else {
			PrintFail("%s does not exist. Something has gone terribly wrong.", buffer);
		}
	}
}

void Audio_RunTableDesign(char* file, char* fname, char* path) {
	char buffer[FILENAME_BUFFER];
	
	#ifdef Z64AUDIO_EXTERNAL_DEPENDENCIES
		BufferPrint("tabledesign -i 30 \"%s%s.aiff\" > \"%s%s.table\"", path, fname, path, fname);
		if (system(buffer))
			PrintFail("tabledesign has failed...");
	#else
		BufferPrint("%s%s.table", path, fname);
		FILE* fp = fopen(buffer, "w");
		if (gAudioState.ftype == WAV)
			BufferPrint("%s%s.aiff", path, fname);
		else
			BufferPrint("%s", file);
		char* argv[] = {
			/*0*/ strdup("tabledesign")
			/*1*/, strdup("-i")
			/*2*/, strdup("30")
			/*3*/, buffer
			/*4*/, 0
		};
		if (tabledesign(4, argv, fp))
			PrintFail("tabledesign has failed...");
		DebugPrint("%s.table Generated", fname);
		fclose(fp);
		free(argv[0]);
		free(argv[1]);
		free(argv[2]);
	#endif
}

void Audio_RunVadpcmEnc(char* file, char* fname, char* path) {
	#ifdef Z64AUDIO_EXTERNAL_DEPENDENCIES
		char buffer[FILENAME_BUFFER];
		BufferPrint(
			"vadpcm_enc -c \"%s%s.table\" \"%s%s.aiff\" \"%s%s.aifc\"",
			path,
			fname,
			path,
			fname,
			path,
			fname
		);
		if (system(buffer))
			PrintFail("vadpcm_enc has failed...");
	#else
		char fname_TABLE[FILENAME_BUFFER];
		char fname_AIFF[FILENAME_BUFFER];
		char fname_AIFC[FILENAME_BUFFER];
		
		snprintf(fname_TABLE, FILENAME_BUFFER, "%s%s.table", path, fname);
		if (gAudioState.ftype == WAV)
			snprintf(fname_AIFF, FILENAME_BUFFER, "%s%s.aiff", path, fname);
		else
			snprintf(fname_AIFF, FILENAME_BUFFER, "%s", file);
		snprintf(fname_AIFC, FILENAME_BUFFER, "%s%s.aifc", path, fname);
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
		DebugPrint("%s Converted to AIFC", fname_AIFF);
		for (i = 0; argv[i]; ++i)
			free(argv[i]);
	#endif
}

void Audio_AiffConvert(char* file, char* fname, char* path, s32 iter) {
	Audio_RunTableDesign(file, fname, path);
	Audio_RunVadpcmEnc(file, fname, path);
}

void Audio_AifcParseZzrtl(char* file, char* fname, char* path, s32 iter) {
	char buffer[FILENAME_BUFFER];
	FILE* AIFC;
	char* vadpcm = 0;
	char* w;
	FILE* PRED;
	FILE* SMPL;
	FILE* CONF;
	s32 size;
	ALADPCMloop* loopInfo;
	InstrumentChunk* instData;
	CommonChunk* comm;
	
	if (gAudioState.ftype <= AIFF)
		BufferPrint("%s%s.aifc", path, fname);
	else
		BufferPrint("%s", file);
	AIFC = fopen(buffer, "rb");
	if (AIFC == NULL)
		PrintFail("Could not open %s", buffer);
	fseek(AIFC, 0, SEEK_END);
	size = ftell(AIFC);
	rewind(AIFC);
	w = vadpcm = malloc(size);
	if (vadpcm == NULL)
		PrintFail("Could not malloc vadpcm");
	fread(vadpcm, size, 1, AIFC);
	fclose(AIFC);
	
	comm = (CommonChunk*)(vadpcm + 0x14);
	memcpy(&gAudioState.vadpcmInfo.commChunk[iter], comm, sizeof(CommonChunk));
	
	if (gAudioState.ftype != WAV) {
		if (SearchByteString(vadpcm, size, "VADPCMLOOPS", 0xB)) {
			gAudioState.instDataFlag[iter] = gAudioState.instLoop[iter] = true;
			DebugPrint("Found loop data");
		} else {
			gAudioState.instDataFlag[iter] = true;
			DebugPrint("No loop data");
		}
		
		float80 sampleRate = 0;
		u8* pointer = (u8*)&sampleRate;
		
		for (s32 i = 0; i < 10; i++) {
			pointer[i] = (u8)comm->sampleRate[9 - i];
		}
		
		gAudioState.sampleRate[iter] = (s32)sampleRate;
	}
	
	/* PREDICTOR */
	ColorPrint(0, "PREDICTOR"); {
		void* endOfPredTable;
		s8 predictorHead[8] = { 0 };
		
		#ifndef __Z64AUDIO_TERMINAL__
			BufferPrint("%spredictors.bin", path);
		#else
			BufferPrint("%s%s.predictors.bin", path, fname);
		#endif
		PRED = fopen(buffer, MODE_WRITE);
		if (PRED == NULL)
			PrintFail("Could not create/open %s", buffer);
		w = SearchByteString(vadpcm, size, "CODES", 5);
		if (w == NULL)
			PrintFail("Could not locate CODES from AIFC-file");
		w += 7;
		predictorHead[3] = w[1];
		predictorHead[7] = w[3];
		endOfPredTable = SearchByteString(vadpcm, size, "SSND", 4);
		fwrite(&predictorHead, sizeof(predictorHead), 1, PRED);
		w += 4;
		while ((intptr_t)w < (intptr_t)endOfPredTable) {
			fwrite(w, 1, 1, PRED);
			w++;
		}
		fclose(PRED);
	}
	
	/* SAMPLE */
	ColorPrint(0, "SAMPLE"); {
		u32 ssndSize;
		
		#ifndef __Z64AUDIO_TERMINAL__
			BufferPrint("%ssample.bin", path);
		#else
			BufferPrint("%s%s.sample.bin", path, fname);
		#endif
		SMPL = fopen(buffer, MODE_WRITE);
		if (SMPL == NULL)
			PrintFail("Could not create/open %s", buffer);
		w += 4;
		ssndSize = ((u32*)w)[0];
		BSWAP32(ssndSize);
		w += 0xC;
		for (s32 i = 0; i < ssndSize - 8; i++) {
			fwrite(w, 1, 1, SMPL);
			w++;
		}
		fclose(SMPL);
	}
	
	/* LOOPPREDICTORS */ {
		FILE* LOOPRD;
		
		if (gAudioState.instLoop[iter] == true) {
			w = SearchByteString(vadpcm, size, "VADPCMLOOPS", 0xB);
			if (w == NULL)
				gAudioState.instLoop[iter] = false;
			else {
				ColorPrint(0, "LOOPPREDICTORS");
				w += 0xF;
				loopInfo = (void*)w;
				memcpy(&gAudioState.vadpcmInfo.loopInfo[iter], loopInfo, sizeof(ALADPCMloop));
				BSWAP32(loopInfo->start);
				BSWAP32(loopInfo->end);
				BSWAP32(loopInfo->count);
				
				#ifndef __Z64AUDIO_TERMINAL__
					BufferPrint("%slooppredictors.bin", path);
				#else
					BufferPrint("%s%s.looppredictors.bin", path, fname);
				#endif
				LOOPRD = fopen(buffer, MODE_WRITE);
				for (s32 i = 0; i < 16; i++)
					fwrite(&loopInfo->state[i], sizeof(s16), 1, LOOPRD);
				fclose(LOOPRD);
			}
		}
	}
	
	/* CONFIG */
	ColorPrint(0, "CONFIG"); {
		u32 numFrame;
		u16 tempFrames[2] = {
			comm->numFramesH,
			comm->numFramesL
		};
		
		#ifndef __Z64AUDIO_TERMINAL__
			BufferPrint("%sconfig.tsv", path);
		#else
			BufferPrint("%s%s.config.tsv", path, fname);
		#endif
		CONF = fopen(buffer, MODE_WRITE);
		
		w = SearchByteString(vadpcm, size, "INST\0\0", 6);
		if (w == NULL)
			PrintFail("Could not locate INST from AIFC-file");
		w += 8;
		instData = (void*)w;
		memcpy(&gAudioState.vadpcmInfo.instChunk[iter], instData, sizeof(InstrumentChunk));
		
		BSWAP16(tempFrames[0]);
		BSWAP16(tempFrames[1]);
		numFrame = (tempFrames[0] << 16) | tempFrames[1];
		
		fprintf(CONF, "precision\tloopstart\tloopend  \tloopcount\tlooptail \n%08X\t", 0);
		if (gAudioState.instLoop[iter] == true && loopInfo) {
			/* Found Loop */
			fprintf(CONF, "%08X\t", loopInfo->start);
			fprintf(CONF, "%08X\t", loopInfo->end);
			fprintf(CONF, "%08X\t", loopInfo->count);
			if (numFrame <= loopInfo->end)
				numFrame = 0;
			fprintf(CONF, "%08X\n", numFrame);
		} else {
			fprintf(CONF, "%08X\t", 0);
			fprintf(CONF, "%08X\t", numFrame);
			fprintf(CONF, "%08X\t", 0);
			fprintf(CONF, "%08X\n", 0);
		}
		
		fclose(CONF);
	}
	
	if (vadpcm)
		free(vadpcm);
}

#include "audiobank.h"

void Audio_AifcParseC(char* file, char* fname, char* path, s32 iter) {
	char buffer[FILENAME_BUFFER];
	FILE* AIFC;
	char* vadpcm = 0;
	char* w;
	FILE* PRED;
	FILE* SMPL;
	FILE* CONF;
	s32 size;
	ALADPCMloop* loopInfo;
	InstrumentChunk* instData;
	CommonChunk* comm;
	
	ALADPCMLoopMain loopMain = { 0 };
	ALADPCMLoopTail loopTail = { 0 };
	ALADPCMBook* book;
	ABSample sample = { 0 };
	ABEnvelope envelope = { 0 };
	ABInstrument instrument = { 0 };
	
	if (gAudioState.ftype <= AIFF)
		BufferPrint("%s%s.aifc", path, fname);
	else
		BufferPrint("%s", file);
	AIFC = fopen(buffer, "rb");
	if (AIFC == NULL)
		PrintFail("Could not open %s", buffer);
	fseek(AIFC, 0, SEEK_END);
	size = ftell(AIFC);
	rewind(AIFC);
	w = vadpcm = malloc(size);
	if (vadpcm == NULL)
		PrintFail("Could not malloc vadpcm");
	fread(vadpcm, size, 1, AIFC);
	fclose(AIFC);
	
	comm = (CommonChunk*)(vadpcm + 0x14);
	
	w = SearchByteString(vadpcm, size, "VADPCMLOOPS", 11);
}

void Audio_GenerateInstrumentConf(char* file,
    char* path
    , s32 fileCount
#ifndef __Z64AUDIO_TERMINAL__
	    , char* outputFile
#endif
) {
	char buffer[FILENAME_BUFFER] = { 0 };
	char fname[FILENAME_BUFFER] = { 0 };
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
	ColorPrint(1, "Generating inst.tsv");
	
	GetFilename(file, fname, NULL);
	BufferPrint("%s%s.inst.tsv", path, fname);
	
	float pitch[3] = { 0 };
	
	for (s32 i = 0; i < fileCount; i++) {
		s8 nn = gAudioState.vadpcmInfo.instChunk[i].baseNote % 12;
		u32 f = floor((double)gAudioState.vadpcmInfo.instChunk[i].baseNote / 12);
		
		pitch[i] = ((float)gAudioState.sampleRate[i]) / 32000.0f;
		
		if (gAudioState.instDataFlag[i]) {
			pitch[i] *= pow(pow(2, 1.0 / 12), 60.0 - (double)gAudioState.vadpcmInfo.instChunk[i].baseNote + 0.01 * gAudioState.vadpcmInfo.instChunk[i].detune);
		}
		
		DebugPrint("note %s%d\t%2.1f kHz\t%f", note[nn], (u32)f, (float)gAudioState.sampleRate[i] * 0.001, pitch[i]);
	}
	#ifndef __Z64AUDIO_TERMINAL__
		conf = fopen(outputFile, MODE_WRITE);
	#else
		conf = fopen(buffer, MODE_WRITE);
	#endif
	
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
		    snprintf(sampleID[1], 5, "%d", gAudioState.sampleID[0]);
	    } break;
	    
	    /* NULL Prim Secn */
	    case 2: {
		    snprintf(floats[1], 9, "%08X", hexFloat(pitch[0]));
		    snprintf(sampleID[1], 5, "%d", gAudioState.sampleID[0]);
		    
		    snprintf(floats[2], 9, "%08X", hexFloat(pitch[1]));
		    snprintf(sampleID[2], 4, "%d", gAudioState.sampleID[1]);
		    
		    if (gAudioState.vadpcmInfo.instChunk[0].highNote != 0)
			    snprintf(split[2], 4, "%d", gAudioState.vadpcmInfo.instChunk[0].highNote - 21);
	    } break;
	    
	    /* Prev Prim Secn */
	    case 3: {
		    snprintf(floats[0], 9, "%08X", hexFloat(pitch[2]));
		    snprintf(sampleID[0], 5, "%d", gAudioState.sampleID[2]);
		    
		    snprintf(floats[1], 9, "%08X", hexFloat(pitch[0]));
		    snprintf(sampleID[1], 5, "%d", gAudioState.sampleID[0]);
		    
		    snprintf(floats[2], 9, "%08X", hexFloat(pitch[1]));
		    snprintf(sampleID[2], 5, "%d", gAudioState.sampleID[1]);
		    
		    if (gAudioState.vadpcmInfo.instChunk[0].highNote != 0) {
			    snprintf(split[1], 4, "%d", gAudioState.vadpcmInfo.instChunk[0].lowNote - 21);
			    snprintf(split[2], 4, "%d", gAudioState.vadpcmInfo.instChunk[0].highNote - 21);
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