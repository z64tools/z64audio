#include "AudioTools.h"

char* gTableDesignIteration = "30";
char* gTableDesignFrameSize = "16";
char* gTableDesignBits = "2";
char* gTableDesignOrder = "2";

void AudioTools_RunTableDesign(AudioSampleInfo* sampleInfo) {
	char basename[256];
	char path[256];
	char buffer[256];
	char* tableDesignArgv[] = {
		"tabledesign",
		"-i",
		gTableDesignIteration,
		"-f",
		gTableDesignFrameSize,
		"-s",
		gTableDesignBits,
		"-o",
		gTableDesignOrder,
		sampleInfo->output
	};
	
	String_GetBasename(basename, sampleInfo->output);
	String_GetPath(path, sampleInfo->output);
	
	String_Copy(buffer, path);
	String_Merge(buffer, basename);
	String_Merge(buffer, ".table");
	
	printf_info("Saving [%s]", buffer);
	
	FILE* table = fopen(buffer, "w");
	
	if (table == NULL) {
		printf_error("Audio_TableDesign: Could not create/open file [%s]", buffer);
	}
	
	printf_debug(
		"Run:%s %s %s %s %s %s %s %s %s %s",
		tableDesignArgv[0],
		tableDesignArgv[1],
		tableDesignArgv[2],
		tableDesignArgv[3],
		tableDesignArgv[4],
		tableDesignArgv[5],
		tableDesignArgv[6],
		tableDesignArgv[7],
		tableDesignArgv[8],
		tableDesignArgv[9]
	);
	
	if (tabledesign(ARRAY_COUNT(tableDesignArgv), tableDesignArgv, table))
		printf_error("TableDesign has failed");
	
	fclose(table);
}
void AudioTools_RunVadpcmEnc(AudioSampleInfo* sampleInfo) {
	char basename[256];
	char path[256];
	char buffer[256];
	char* vadpcmArgv[] = {
		"vadpcm_enc",
		"-c",
		NULL,
		sampleInfo->output,
		NULL,
		NULL
	};
	
	String_GetBasename(basename, sampleInfo->output);
	String_GetPath(path, sampleInfo->output);
	String_Copy(buffer, path);
	String_Merge(buffer, basename);
	String_Merge(buffer, ".table");
	vadpcmArgv[2] = String_Generate(buffer);
	
	String_GetBasename(basename, sampleInfo->output);
	String_GetPath(path, sampleInfo->output);
	String_Copy(buffer, path);
	String_Merge(buffer, basename);
	String_Merge(buffer, ".aifc");
	vadpcmArgv[4] = String_Generate(buffer);
	
	printf_info("Saving [%s]", buffer);
	
	FILE* table = fopen(buffer, "w");
	
	if (table == NULL) {
		printf_error("Audio_TableDesign: Could not create/open file [%s]", buffer);
	}
	
	printf_debug(
		"%s %s %s %s %s",
		vadpcmArgv[0],
		vadpcmArgv[1],
		vadpcmArgv[2],
		vadpcmArgv[3],
		vadpcmArgv[4]
	);
	
	if (vadpcm_enc(5, vadpcmArgv))
		printf_error("VadpcmEnc has failed");
	
	fclose(table);
	free(vadpcmArgv[2]);
	free(vadpcmArgv[4]);
}

#include "sdk-tools/tabledesign/tabledesign.h"

void AudioTools_TableDesign(AudioSampleInfo* sampleInfo) {
#define FREE_PP(PP, NUM)                  \
	if (PP) {                             \
		for (s32 i = 0; i < (NUM); ++i) { \
			if (PP[i]) {                  \
				free(PP[i]);              \
			}                             \
		}                                 \
		free(PP);                         \
	}
#define FREE_P(P) if (P) free(P)
	
	if (sampleInfo->channelNum != 16) {
		sampleInfo->targetBit = 16;
		Audio_Resample(sampleInfo);
	}
	if (sampleInfo->channelNum != 1)
		Audio_ConvertToMono(sampleInfo);
	
	u32 refineIteration = String_NumStrToInt(gTableDesignIteration);
	u32 frameSize = String_NumStrToInt(gTableDesignFrameSize);
	u32 bits = String_NumStrToInt(gTableDesignBits);
	u32 order = String_NumStrToInt(gTableDesignOrder);
	f64 threshold = 10.0;
	u32 frameCount = sampleInfo->samplesNum;
	
	printf_debug(
		"TableDesign params:\n"
		"\tIter: %d\n"
		"\tFrmS: %d\n"
		"\tBits: %d\n"
		"\tOrdr: %d\n"
		"\tThrs: %g",
		refineIteration,
		frameSize,
		bits,
		order,
		threshold
	);
	
	u32 tempS1Num = (1 << bits);
	u32 orderSize = order + 1;
	
	f64** tempS1 = malloc(sizeof(f64*) * tempS1Num);
	f64* splitDelta = malloc(sizeof(f64) * orderSize);
	s16* frames = malloc(sizeof(s16) * frameSize * 2);
	f64* vec = malloc(sizeof(f64) * orderSize);
	f64* spF4 = malloc(sizeof(f64) * orderSize);
	f64** mat = malloc(sizeof(f64*) * orderSize);
	s32* perm = malloc(sizeof(s32) * orderSize);
	f64** data = malloc(sizeof(f64*) * frameCount);
	
	for (s32 i = 0; i < frameSize * 2; i++) {
		frames[i] = 0;
	}
	for (s32 i = 0; i < tempS1Num; i++) {
		tempS1[i] = malloc(sizeof(f64) * orderSize);
	}
	for (s32 i = 0; i <= order; i++) {
		mat[i] = malloc(sizeof(double) * orderSize);
	}
	
	u32 dataSize = 0;
	u32 Z = 0;
	s32 permDet;
	f64 dummy;
	
	while (sampleInfo->samplesNum - Z - frameSize > frameSize) {
		memcpy(frames + frameSize, &sampleInfo->audio.s16[Z], sizeof(s16) * frameSize);
		acvect(frames + frameSize, order, frameSize, vec);
		if (fabs(vec[0]) > threshold) {
			acmat(frames + frameSize, order, frameSize, mat);
			if (lud(mat, order, perm, &permDet) == 0) {
				lubksb(mat, order, perm, vec);
				vec[0] = 1.0;
				if (kfroma(vec, spF4, order) == 0) {
					data[dataSize] = malloc((order + 1) * sizeof(double));
					data[dataSize][0] = 1.0;
					
					for (s32 i = 1; i <= order; i++) {
						spF4[i] = CLAMP(spF4[i], -0.9999999999, 0.9999999999);
					}
					
					afromk(spF4, data[dataSize], order);
					dataSize++;
				}
			}
		}
		
		for (s32 i = 0; i < frameSize; i++) {
			frames[i] = frames[i + frameSize];
		}
		
		Z += frameSize;
	}
	
	vec[0] = 1.0;
	for (s32 i = 1; i <= order; i++) {
		vec[i] = 0.0;
	}
	for (s32 i = 0; i < dataSize; i++) {
		rfroma(data[i], order, tempS1[0]);
		for (s32 j = 1; j <= order; j++) {
			vec[j] += tempS1[0][j];
		}
	}
	for (s32 i = 1; i <= order; i++) {
		vec[i] /= dataSize;
	}
	
	durbin(vec, order, spF4, tempS1[0], &dummy);
	
	for (s32 i = 1; i <= order; i++) {
		spF4[i] = CLAMP(spF4[i], -0.9999999999, 0.9999999999);
	}
	
	afromk(spF4, tempS1[0], order);
	u32 curBits = 0;
	
	while (curBits < bits) {
		for (s32 i = 0; i <= order; i++) {
			splitDelta[i] = 0.0;
		}
		splitDelta[order - 1] = -1.0;
		split(tempS1, splitDelta, order, (1 << curBits), 0.01);
		curBits++;
		refine(tempS1, order, (1 << curBits), data, dataSize, refineIteration, 0.0);
	}
	
	s32 nPredictors = (1 << curBits);
	PointerCast pred;
	u32 numOverflows = 0;
	u32 predWrite = 0;
	
	printf_debug("nPred %d", nPredictors);
	
	pred.p = malloc(sizeof(u16) * 0x8 * order * nPredictors);
	pred.u16[0] = order;
	pred.u16[1] = nPredictors;
	
	double** table;
	table = malloc(sizeof(f64*) * 8);
	for (s32 i = 0; i < 8; i++) {
		table[i] = malloc(sizeof(double) * order);
	}
	
	for (s32 X = 0; X < nPredictors; X++) {
		double fval;
		s32 ival;
		double* row = tempS1[X];
		
		for (s32 i = 0; i < order; i++) {
			for (s32 j = 0; j < i; j++) {
				table[i][j] = 0.0;
			}
			for (s32 j = i; j < order; j++) {
				table[i][j] = -row[order - j + i];
			}
		}
		
		for (s32 i = order; i < 8; i++) {
			for (s32 j = 0; j < order; j++) {
				table[i][j] = 0.0;
			}
		}
		
		for (s32 i = 1; i < 8; i++) {
			for (s32 j = 1; j <= order; j++) {
				if (i - j >= 0) {
					for (s32 k = 0; k < order; k++) {
						table[i][k] -= row[j] * table[i - j][k];
					}
				}
			}
		}
		
		for (s32 i = 0; i < order; i++) {
			for (s32 j = 0; j < 8; j++) {
				fval = table[j][i] * 2048.0;
				if (fval < 0.0) {
					ival = (s32)(fval - 0.5);
					if (ival < -0x8000) {
						numOverflows++;
					}
				} else {
					ival = (s32)(fval + 0.5);
					if (ival >= 0x8000) {
						numOverflows++;
					}
				}
				
				// Write table
				pred.s16[2 + predWrite++] = (s16)ival;
			}
		}
	}
	
	if (numOverflows) {
		printf_warning("TableDesign seems to have overflown!");
	}
	
	sampleInfo->vadpcm.pred = pred;
	
	FREE_PP(table, 8);
	FREE_PP(data, dataSize);
	FREE_PP(tempS1, tempS1Num);
	FREE_PP(mat, orderSize);
	FREE_P(vec);
	FREE_P(spF4);
	FREE_P(perm);
	FREE_P(frames);
	FREE_P(splitDelta);
}
