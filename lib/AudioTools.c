#include "AudioTools.h"
#include "AudioSDK.h"

char* gTableDesignIteration = "2";
char* gTableDesignFrameSize = "16";
char* gTableDesignBits = "2";
char* gTableDesignOrder = "2";
char* gTableDesignThreshold = "10.0";

void AudioTools_RunTableDesign(AudioSampleInfo* sampleInfo) {
	char buffer[256];
	char sys[526];
	
	String_SwapExtension(buffer, sampleInfo->output, "Book.txt");
	
	strcpy(sys, "./tabledesign");
	strcat(sys, " -i ");
	strcat(sys, gTableDesignIteration);
	strcat(sys, " -f ");
	strcat(sys, gTableDesignFrameSize);
	strcat(sys, " -s ");
	strcat(sys, gTableDesignBits);
	strcat(sys, " -o ");
	strcat(sys, gTableDesignBits);
	strcat(sys, " -o ");
	strcat(sys, gTableDesignOrder);
	strcat(sys, " ");
	strcat(sys, sampleInfo->output);
	strcat(sys, " > ");
	strcat(sys, buffer);
	
	Log("%s", sys);
	
	if (system(sys))
		printf_error("TableDesign has failed");
	Log("TableDesign %s", buffer);
}

void AudioTools_RunVadpcmEnc(AudioSampleInfo* sampleInfo) {
	char buffer[256];
	char sys[256 * 2];
	
	strcpy(sys, "./vadpcm_enc -c");
	strcat(sys, " ");
	
	String_SwapExtension(buffer, sampleInfo->output, "Book.txt");
	strcat(sys, buffer);
	strcat(sys, " ");
	
	strcat(sys, sampleInfo->output);
	strcat(sys, " ");
	
	String_SwapExtension(buffer, sampleInfo->output, ".aifc");
	strcat(sys, buffer);
	
	Log("[%s]", sys);
	
	if (system(sys))
		printf_error("VadpcmEnc has failed");
	Log("VadpcmEnc %s", buffer);
}

static void AudioTools_VencodeBrute(u8* out, s16* inBuffer, s32* origState, s32*** coefTable, s32 order, s32 npredictors, u32 framesize) {
	s16 ix[16];
	s32 prediction[16];
	s32 inVector[16];
	s32 optimalp = 0;
	s32 scale;
	s32 encBits = (framesize == 5 ? 2 : 4);
	s32 llevel = -(1 << (encBits - 1));
	s32 ulevel = -llevel - 1;
	s32 ie[16];
	f32 e[16];
	f32 min = 1e30;
	s32 scaleFactor = 16 - encBits;
	
	for (s32 k = 0; k < npredictors; k++) {
		for (s32 j = 0; j < 2; j++) {
			for (s32 i = 0; i < order; i++) {
				inVector[i] = (j == 0 ? origState[16 - order + i] : inBuffer[8 - order + i]);
			}
			
			for (s32 i = 0; i < 8; i++) {
				prediction[j * 8 + i] = inner_product(order + i, coefTable[k][i], inVector);
				inVector[i + order] = inBuffer[j * 8 + i] - prediction[j * 8 + i];
				e[j * 8 + i] = (f32) inVector[i + order];
			}
		}
		
		f32 se = 0.0f;
		for (s32 j = 0; j < 16; j++) {
			se += e[j] * e[j];
		}
		
		if (se < min) {
			min = se;
			optimalp = k;
		}
	}
	
	for (s32 j = 0; j < 2; j++) {
		for (s32 i = 0; i < order; i++) {
			inVector[i] = (j == 0 ? origState[16 - order + i] : inBuffer[8 - order + i]);
		}
		
		for (s32 i = 0; i < 8; i++) {
			prediction[j * 8 + i] = inner_product(order + i, coefTable[optimalp][i], inVector);
			e[j * 8 + i] = inVector[i + order] = inBuffer[j * 8 + i] - prediction[j * 8 + i];
		}
	}
	
	clamp_to_s16(e, ie);
	
	s32 max = 0;
	
	for (s32 i = 0; i < 16; i++) {
		if (Abs(ie[i]) > Abs(max)) {
			max = ie[i];
		}
	}
	
	for (scale = 0; scale <= scaleFactor; scale++) {
		if (max <= ulevel && max >= llevel) break;
		max /= 2;
	}
	
	s32 state[16];
	
	for (s32 i = 0; i < 16; i++) {
		state[i] = origState[i];
	}
	
	s32 nIter, again;
	
	for (nIter = 0, again = 1; nIter < 2 && again; nIter++) {
		again = 0;
		if (nIter == 1) scale++;
		if (scale > scaleFactor) {
			scale = scaleFactor;
		}
		
		for (s32 j = 0; j < 2; j++) {
			s32 base = j * 8;
			for (s32 i = 0; i < order; i++) {
				inVector[i] = (j == 0 ?
					origState[16 - order + i] : state[8 - order + i]);
			}
			
			for (s32 i = 0; i < 8; i++) {
				prediction[base + i] = inner_product(order + i, coefTable[optimalp][i], inVector);
				f32 se = (f32) inBuffer[base + i] - (f32) prediction[base + i];
				ix[base + i] = qsample(se, 1 << scale);
				s32 cV = clamp_bits(ix[base + i], encBits) - ix[base + i];
				if (cV > 1 || cV < -1) again = 1;
				ix[base + i] += cV;
				inVector[i + order] = ix[base + i] * (1 << scale);
				state[base + i] = prediction[base + i] + inVector[i + order];
			}
		}
	}
	
	u8 header = (scale << 4) | (optimalp & 0xf);
	
	out[0] = header;
	if (framesize == 5) {
		for (s32 i = 0; i < 16; i += 4) {
			u8 c = ((ix[i] & 0x3) << 6) | ((ix[i + 1] & 0x3) << 4) | ((ix[i + 2] & 0x3) << 2) | (ix[i + 3] & 0x3);
			out[1 + i / 4] = c;
		}
	} else {
		for (s32 i = 0; i < 16; i += 2) {
			u8 c = ((ix[i] & 0xf) << 4) | (ix[i + 1] & 0xf);
			out[1 + i / 2] = c;
		}
	}
}

static void AudioTools_ReadCodeBook(AudioSampleInfo* sampleInfo, s32**** table, s32* destOrder, s32* destNPred) {
	s32** tableEntry;
	
	s32 order = *destOrder = sampleInfo->vadBook.cast.u16[0];
	s32 nPred = *destNPred = sampleInfo->vadBook.cast.u16[1];
	
	*table = malloc(sizeof(s32 * *) * nPred);
	for (s32 i = 0; i < nPred; i++) {
		(*table)[i] = malloc(sizeof(s32*) * 8);
		for (s32 j = 0; j < 8; j++) {
			(*table)[i][j] = malloc(sizeof(s32) * (order + 8));
		}
	}
	
	s32 p = 0;
	
	for (s32 i = 0; i < nPred; i++) {
		tableEntry = (*table)[i];
		
		for (s32 j = 0; j < order; j++) {
			for (s32 k = 0; k < 8; k++) {
				tableEntry[k][j] = sampleInfo->vadBook.cast.s16[2 + p++];
			}
		}
		
		for (s32 k = 1; k < 8; k++) {
			tableEntry[k][order] = tableEntry[k - 1][order - 1];
		}
		
		tableEntry[0][order] = 1 << 11;
		
		for (s32 k = 1; k < 8; k++) {
			s32 j = 0;
			for (j = 0; j < k; j++) {
				tableEntry[j][k + order] = 0;
			}
			
			for (; j < 8; j++) {
				tableEntry[j][k + order] = tableEntry[j - k][order];
			}
		}
	}
}

static void AudioTools_VencodeFrame(MemFile* mem, s16* buffer, s32* state, s32*** coefTable, s32 order, s32 npredictors, u32 framesize, u8 precision) {
	s32 nPred = npredictors;
	s16 ix[16] = { 0 };
	s32 prediction[16];
	s32 inVector[16];
	s32 saveState[16];
	f32 e[16];
	s32 ie[16];
	f32 se;
	s32 encBits = (precision == 5 ? 2 : 4);
	s32 llevel = -(1 << (encBits - 1));
	s32 ulevel = -llevel - 1;
	f32 min = 1e30;
	s32 optimalp = 0;
	s32 scaleFactor = 16 - encBits;
	
	for (s32 i = framesize; i < 16; i++) {
		buffer[i] = 0;
	}
	
	// Determine the best-fitting predictor.
	for (s32 k = 0; k < nPred; k++) {
		for (s32 i = 0; i < order; i++) {
			inVector[i] = state[16 - order + i];
		}
		
		// For 8 samples...
		for (s32 i = 0; i < 8; i++) {
			// Compute a prediction based on 'order' values from the old state,
			// plus previous errors in this chunk, as an inner product with the
			// coefficient table.
			prediction[i] = inner_product(order + i, coefTable[k][i], inVector);
			// Record the error in inVector (thus, its first 'order' samples
			// will contain actual values, the rest will be error terms), and
			// in floating point form in e (for no particularly good reason).
			inVector[i + order] = buffer[i] - prediction[i];
			e[i] = (f32) inVector[i + order];
		}
		
		// For the next 8 samples, start with 'order' values from the end of
		// the previous 8-sample chunk of inBuffer. (The code is equivalent to
		// inVector[i] = inBuffer[8 - order + i].)
		for (s32 i = 0; i < order; i++) {
			inVector[i] = prediction[8 - order + i] + inVector[8 + i];
		}
		
		// ... and do the same thing as before to get predictions.
		for (s32 i = 0; i < 8; i++) {
			prediction[8 + i] = inner_product(order + i, coefTable[k][i], inVector);
			inVector[i + order] = buffer[8 + i] - prediction[8 + i];
			e[8 + i] = (f32) inVector[i + order];
		}
		
		// Compute the L2 norm of the errors; the lowest norm decides which
		// predictor to use.
		se = 0.0f;
		for (s32 j = 0; j < 16; j++) {
			se += e[j] * e[j];
		}
		
		if (se < min) {
			min = se;
			optimalp = k;
		}
	}
	
	// Do exactly the same thing again, for real.
	for (s32 i = 0; i < order; i++) {
		inVector[i] = state[16 - order + i];
	}
	
	for (s32 i = 0; i < 8; i++) {
		prediction[i] = inner_product(order + i, coefTable[optimalp][i], inVector);
		inVector[i + order] = buffer[i] - prediction[i];
		e[i] = (f32) inVector[i + order];
	}
	
	for (s32 i = 0; i < order; i++) {
		inVector[i] = prediction[8 - order + i] + inVector[8 + i];
	}
	
	for (s32 i = 0; i < 8; i++) {
		prediction[8 + i] = inner_product(order + i, coefTable[optimalp][i], inVector);
		inVector[i + order] = buffer[8 + i] - prediction[8 + i];
		e[8 + i] = (f32) inVector[i + order];
	}
	
	// Clamp the errors to 16-bit signed ints, and put them in ie.
	clamp(16, e, ie, 16);
	
	// Find a value with highest absolute value.
	// @bug If this first finds -2^n and later 2^n, it should set 'max' to the
	// latter, which needs a higher value for 'scale'.
	s32 max = 0;
	
	for (s32 i = 0; i < 16; i++) {
		if (Abs(ie[i]) > Abs(max)) {
			max = ie[i];
		}
	}
	
	// Compute which power of two we need to scale down by in order to make
	// all values representable as 4-bit signed integers (i.e. be in [-8, 7]).
	// The worst-case 'max' is -2^15, so this will be at most 12.
	s32 scale;
	
	for (scale = 0; scale <= scaleFactor; scale++) {
		if (max <= ulevel && max >= llevel) {
			break;
		}
		max /= 2;
	}
	
	for (s32 i = 0; i < 16; i++) {
		saveState[i] = state[i];
	}
	
	// Try with the computed scale, but if it turns out we don't fit in 4 bits
	// (if some |cV| >= 2), use scale + 1 instead (i.e. downscaling by another
	// factor of 2).
	scale--;
	s32 nIter = 0;
	s32 maxClip = 0;
	
	do {
		s32 cV;
		
		nIter++;
		scale++;
		maxClip = 0;
		scale = ClampMax(scale, 12);
		
		// Copy over the last 'order' samples from the previous output.
		for (s32 i = 0; i < order; i++) {
			inVector[i] = saveState[16 - order + i];
		}
		
		// For 8 samples...
		for (s32 i = 0; i < 8; i++) {
			// Compute a prediction based on 'order' values from the old state,
			// plus previous *quantized* errors in this chunk (because that's
			// all the decoder will have available).
			prediction[i] = inner_product(order + i, coefTable[optimalp][i], inVector);
			
			// Compute the error, and divide it by 2^scale, rounding to the
			// nearest integer. This should ideally result in a 4-bit integer.
			se = (f32) buffer[i] - (f32) prediction[i];
			ix[i] = qsample(se, 1 << scale);
			
			// Clamp the error to a 4-bit signed integer, and record what delta
			// was needed for that.
			cV = (s16) clip(ix[i], llevel, ulevel) - ix[i];
			if (maxClip < Abs(cV)) {
				maxClip = Abs(cV);
			}
			ix[i] += cV;
			
			// Record the quantized error in inVector for later predictions,
			// and the quantized (decoded) output in state (for use in the next
			// batch of 8 samples).
			inVector[i + order] = ix[i] * (1 << scale);
			state[i] = prediction[i] + inVector[i + order];
		}
		
		// Copy over the last 'order' decoded samples from the above chunk.
		for (s32 i = 0; i < order; i++) {
			inVector[i] = state[8 - order + i];
		}
		
		// ... and do the same thing as before.
		for (s32 i = 0; i < 8; i++) {
			prediction[8 + i] = inner_product(order + i, coefTable[optimalp][i], inVector);
			se = (f32) buffer[8 + i] - (f32) prediction[8 + i];
			ix[8 + i] = qsample(se, 1 << scale);
			cV = (s16) clip(ix[8 + i], llevel, ulevel) - ix[8 + i];
			if (maxClip < Abs(cV)) {
				maxClip = Abs(cV);
			}
			ix[8 + i] += cV;
			inVector[i + order] = ix[8 + i] * (1 << scale);
			state[8 + i] = prediction[8 + i] + inVector[i + order];
		}
	} while (maxClip >= 2 && nIter < 2);
	
	u8 header = (scale << 4) | (optimalp & 0xf);
	
	MemFile_Write(mem, &header, sizeof(u8));
	
	if (precision == 9) {
		for (s32 i = 0; i < 16; i += 2) {
			u8 c = (ix[i] << 4) | (ix[i + 1] & 0xf);
			MemFile_Write(mem, &c, sizeof(u8));
		}
	} else {
		for (s32 i = 0; i < 16; i += 4) {
			u8 c = ((ix[i] & 0x3) << 6) | ((ix[i + 1] & 0x3) << 4) | ((ix[i + 2] & 0x3) << 2) | (ix[i + 3] & 0x3);
			MemFile_Write(mem, &c, sizeof(u8));
		}
	}
}

static void AudioTools_VdecodeFrame(u8* frame, s32* decompressed, s32* state, s32 order, s32*** coefTable, u32 framesize) {
	s32 ix[16];
	
	u8 header = frame[0];
	s32 scale = 1 << (header >> 4);
	s32 optimalp = header & 0xf;
	
	if (framesize == 5) {
		for (s32 i = 0; i < 16; i += 4) {
			u8 c = frame[1 + i / 4];
			ix[i] = c >> 6;
			ix[i + 1] = (c >> 4) & 0x3;
			ix[i + 2] = (c >> 2) & 0x3;
			ix[i + 3] = c & 0x3;
		}
	} else {
		for (s32 i = 0; i < 16; i += 2) {
			u8 c = frame[1 + i / 2];
			ix[i] = c >> 4;
			ix[i + 1] = c & 0xf;
		}
	}
	
	for (s32 i = 0; i < 16; i++) {
		if (framesize == 5) {
			if (ix[i] >= 2) ix[i] -= 4;
		} else {
			if (ix[i] >= 8) ix[i] -= 16;
		}
		decompressed[i] = ix[i];
		ix[i] *= scale;
	}
	
	for (s32 j = 0; j < 2; j++) {
		s32 in_vec[16];
		if (j == 0) {
			for (s32 i = 0; i < order; i++) {
				in_vec[i] = state[16 - order + i];
			}
		} else {
			for (s32 i = 0; i < order; i++) {
				in_vec[i] = state[8 - order + i];
			}
		}
		
		for (s32 i = 0; i < 8; i++) {
			s32 ind = j * 8 + i;
			in_vec[order + i] = ix[ind];
			state[ind] = inner_product(order + i, coefTable[optimalp][i], in_vec) + ix[ind];
		}
	}
}

void AudioTools_TableDesign(AudioSampleInfo* sampleInfo) {
	#define FREE_PP(PP, NUM) \
		if (PP) { \
			for (s32 i = 0; i < (NUM); ++i) { \
				if (PP[i]) { \
					free(PP[i]); \
				} \
			} \
			free(PP); \
		}
	#define FREE_P(P) if (P) free(P)
	
	u32 refineIteration = String_GetInt(gTableDesignIteration);
	u32 frameSize = String_GetInt(gTableDesignFrameSize);
	u32 bits = String_GetInt(gTableDesignBits);
	u32 order = String_GetInt(gTableDesignOrder);
	f64 threshold = String_GetFloat(gTableDesignThreshold);
	u32 frameCount = sampleInfo->samplesNum;
	
	Log(
		"%s: params:\n"
		"\tIter: %d\n"
		"\tFrmS: %d\n"
		"\tBits: %d\n"
		"\tOrdr: %d\n"
		"\tThrs: %g",
		__FUNCTION__,
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
						spF4[i] = Clamp(spF4[i], -0.9999999999, 0.9999999999);
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
		spF4[i] = Clamp(spF4[i], -0.9999999999, 0.9999999999);
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
	u32 numOverflows = 0;
	
	MemFile* memBook = &sampleInfo->vadBook;
	MemFile_Malloc(memBook, sizeof(u16) * 0x8 * order * nPredictors + sizeof(u16) * 2);
	MemFile_Write(memBook, &order, sizeof(u16));
	MemFile_Write(memBook, &nPredictors, sizeof(u16));
	
	f64** table;
	table = malloc(sizeof(f64*) * 8);
	for (s32 i = 0; i < 8; i++) {
		table[i] = malloc(sizeof(double) * order);
	}
	
	for (s32 X = 0; X < nPredictors; X++) {
		f64 fval;
		s32 ival;
		f64* row = tempS1[X];
		
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
				MemFile_Write(memBook, &ival, sizeof(u16));
			}
		}
	}
	
	if (numOverflows) {
		printf_warning("\aTableDesign seems to have " PRNT_REDD "overflown" PRNT_RSET " [%d] times!", numOverflows);
	}
	
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

void AudioTools_VadpcmEnc(AudioSampleInfo* sampleInfo) {
	if (sampleInfo->channelNum != 16) {
		sampleInfo->targetBit = 16;
		Audio_BitDepth(sampleInfo);
	}
	if (sampleInfo->channelNum != 1)
		Audio_Mono(sampleInfo);
	
	if (sampleInfo->vadBook.data == NULL) {
		AudioTools_TableDesign(sampleInfo);
	}
	MemFile memEnc = MemFile_Initialize();
	s32 minLoopLength = 30;
	u32 loopStart = 0;
	u32 loopEnd = 0;
	u32 newEnd = 0;
	u32 pos = 0;
	s32 order;
	s32 nPred;
	u32 nSam;
	s32 nBytes = 0;
	s32 state[16] = { 0 };
	s16 buffer[16] = { 0 };
	s32*** table = NULL;
	u32 nFrames = sampleInfo->samplesNum;
	s32 prec = 9; // 5 half VADPCM Precision
	s32 nRepeats = 0;
	
	if (gPrecisionFlag == 3)
		prec = 5;
	
	MemFile_Malloc(&memEnc, sampleInfo->size * 2);
	AudioTools_ReadCodeBook(sampleInfo, &table, &order, &nPred);
	
	// Process Loop
	if (sampleInfo->instrument.loop.count) {
		MemFile_Malloc(&sampleInfo->vadLoopBook, sizeof(s16) * 16);
		
		loopStart = sampleInfo->instrument.loop.start;
		loopEnd = sampleInfo->instrument.loop.end;
		newEnd = sampleInfo->instrument.loop.end;
		sampleInfo->instrument.loop.oldEnd = loopEnd;
		
		while (newEnd - loopStart < minLoopLength) {
			nRepeats++;
			newEnd += loopEnd - loopStart;
		}
		
		sampleInfo->instrument.loop.end = newEnd;
		
		while (pos <= loopStart) {
			memcpy(buffer, &sampleInfo->audio.s16[pos], sizeof(s16) * 16);
			AudioTools_VencodeFrame(&memEnc, buffer, state, table, order, nPred, 16, prec);
			pos += 16;
			nBytes += prec;
		}
		
		for (s32 i = 0; i < 16; i++) {
			if (state[i] >= 0x8000) state[i] = 0x7fff;
			if (state[i] < -0x7fff) state[i] = -0x7fff;
			sampleInfo->vadLoopBook.cast.s16[i] = state[i];
		}
		
		while (nRepeats > 0) {
			for (; pos + 16 < loopEnd; pos += 16) {
				memcpy(buffer, &sampleInfo->audio.s16[pos], sizeof(s16) * 16);
				AudioTools_VencodeFrame(&memEnc, buffer, state, table, order, nPred, 16, prec);
				nBytes += prec;
			}
			
			s32 left = loopEnd - pos;
			memcpy(buffer, &sampleInfo->audio.s16[pos], sizeof(s16) * left);
			memcpy(&buffer[left], &sampleInfo->audio.s16[loopStart], sizeof(s16) * (16 - left));
			AudioTools_VencodeFrame(&memEnc, buffer, state, table, order, nPred, 16, prec);
			pos = loopStart - left + 16;
			nBytes += prec;
			nRepeats--;
		}
	}
	
	while (pos < nFrames) {
		if (nFrames - pos < 16) {
			nSam = nFrames - pos;
		} else {
			nSam = 16;
		}
		
		if (nFrames - pos >= nSam) {
			memcpy(buffer, &sampleInfo->audio.s16[pos], sizeof(s16) * 16);
			AudioTools_VencodeFrame(&memEnc, buffer, state, table, order, nPred, nSam, prec);
			pos += nSam;
			nBytes += prec;
		} else {
			printf_warning("VadpcmEnc: Missed a frame!");
			break;
		}
	}
	
	if (nBytes % 2) {
		nBytes++;
		u16 ts = 0;
		MemFile_Write(&memEnc, &ts, 2);
	}
	
	if (table) {
		int i;
		int k;
		for (i = 0; i < nPred; ++i) {
			if (!table[i])
				continue;
			for (k = 0; k < 8; ++k) {
				if (table[i][k])
					free(table[i][k]);
			}
			free(table[i]);
		}
		free(table);
	}
	
	Log("Old MemFile Size [0x%X]", sampleInfo->size);
	
	MemFile_Free(&sampleInfo->memFile);
	sampleInfo->memFile = memEnc;
	sampleInfo->audio.p = memEnc.data;
	sampleInfo->size = nBytes;
	
	Log("New MemFile Size [0x%X]", sampleInfo->size);
	
	Log("LoopBook:");
	
	if (gPrintfSuppress <= PSL_DEBUG) {
		for (s32 i = 0; i < (0x8 * sampleInfo->vadBook.cast.u16[0]) * sampleInfo->vadBook.cast.u16[1]; i++) {
			printf("%04X ", sampleInfo->vadBook.cast.u16[2 + i]);
			if ((i % 8) == 7)
				printf("\n");
		}
	}
}

void AudioTools_VadpcmDec(AudioSampleInfo* sampleInfo) {
	MemFile memDec = MemFile_Initialize();
	u32 pos = 0;
	s32 order;
	s32 npredictors;
	s32*** coefTable = NULL;
	u32 nSamples = sampleInfo->samplesNum;
	void* storePoint = sampleInfo->memFile.data;
	MemFile encode = MemFile_Initialize();
	u32 framesize = gPrecisionFlag ? 5 : 9;
	s32 decompressed[16];
	s32 state[16];
	
	Log("MemFile_Malloc(memDec, 0x%X);", nSamples * sizeof(s32) + 0x100);
	MemFile_Malloc(&memDec, nSamples * sizeof(s32) + 0x100);
	MemFile_Malloc(&encode, sizeof(s16) * 16);
	AudioTools_ReadCodeBook(sampleInfo, &coefTable, &order, &npredictors);
	
	MemFile_Rewind(&sampleInfo->memFile);
	sampleInfo->memFile.data = sampleInfo->audio.p;
	
	for (s32 i = 0; i < order; i++) {
		state[15 - i] = 0;
	}
	
	while (pos < nSamples) {
		u8 input[9];
		u8 encoded[9];
		s32 lastState[16];
		s32 decoded[16];
		s16 guess[16];
		s16 origGuess[16];
		
		memcpy(lastState, state, sizeof(state));
		MemFile_Read(&sampleInfo->memFile, input, framesize);
		
		// Decode for real
		AudioTools_VdecodeFrame(input, decompressed, state, order, coefTable, framesize);
		memcpy(decoded, state, sizeof(state));
		
		// Create a guess from that, by clamping to 16 bits
		for (s32 i = 0; i < 16; i++) {
			guess[i] = origGuess[i] = state[i];
		}
		
		AudioTools_VencodeBrute(encoded, guess, lastState, coefTable, order, npredictors, framesize);
		
		// If it doesn't match, bruteforce the matching.
		if (memcmp(input, encoded, framesize) != 0) {
			s32 guess32[16];
			if (bruteforce(guess32, input, decoded, decompressed, lastState, coefTable, order, npredictors, framesize)) {
				for (int i = 0; i < 16; i++) {
					if (!(-0x8000 <= guess32[i] && guess32[i] <= 0x7fff))
						printf_warning("-0x8000 <= guess32[i] && guess32[i] <= 0x7fff");
					guess[i] = Clamp(guess32[i], -__INT16_MAX__, __INT16_MAX__);
				}
				AudioTools_VencodeBrute(encoded, guess, lastState, coefTable, order, npredictors, framesize);
				if (memcmp(input, encoded, framesize) != 0)
					printf_warning("Could not bruteforce sample %d", pos);
			}
			
			// Bring the matching closer to the original decode (not strictly
			// necessary, but it will move us closer to the target on average).
			for (s32 failures = 0; failures < 50; failures++) {
				s32 ind = myrand() % 16;
				s32 old = guess[ind];
				if (old == origGuess[ind]) continue;
				guess[ind] = origGuess[ind];
				if (myrand() % 2) guess[ind] += (old - origGuess[ind]) / 2;
				AudioTools_VencodeBrute(encoded, guess, lastState, coefTable, order, npredictors, framesize);
				if (memcmp(input, encoded, framesize) == 0) {
					failures = -1;
				} else {
					guess[ind] = old;
				}
			}
		}
		
		memcpy(state, decoded, sizeof(state));
		MemFile_Write(&memDec, guess, sizeof(guess));
		pos += 16;
	}
	
	sampleInfo->memFile.data = storePoint;
	
	if (coefTable) {
		int i;
		int k;
		for (i = 0; i < npredictors; ++i) {
			if (!coefTable[i])
				continue;
			for (k = 0; k < 8; ++k) {
				if (coefTable[i][k])
					free(coefTable[i][k]);
			}
			free(coefTable[i]);
		}
		free(coefTable);
	}
	
	Log("Old MemFile Size [0x%X]", sampleInfo->size);
	
	MemFile_Free(&sampleInfo->memFile);
	MemFile_Free(&encode);
	sampleInfo->channelNum = 1;
	sampleInfo->samplesNum = nSamples;
	sampleInfo->memFile = memDec;
	sampleInfo->audio.p = memDec.data;
	sampleInfo->size = memDec.dataSize;
	sampleInfo->bit = 16;
	sampleInfo->targetBit = 16;
	
	Log("New MemFile Size [0x%X]", sampleInfo->size);
}

void AudioTools_LoadCodeBook(AudioSampleInfo* sampleInfo, char* file) {
	MemFile temp = MemFile_Initialize();
	MemFile* vadBook = &sampleInfo->vadBook;
	u32 size;
	u16 order;
	u16 nPred;
	
	if (vadBook->data) {
		Log("VadBook already exists");
	}
	
	MemFile_LoadFile(&temp, file);
	for (s32 i = 0; i < temp.dataSize / 2; i++) {
		SwapBE(temp.cast.u16[i]);
	}
	order = temp.cast.u16[1];
	nPred = temp.cast.u16[3];
	size = sizeof(u16) * 8 * order * nPred + sizeof(u16) * 2;
	MemFile_Malloc(vadBook, size);
	MemFile_Write(vadBook, &order, 2);
	MemFile_Write(vadBook, &nPred, 2);
	for (s32 i = 0; i < (8 * order * nPred); i++) {
		MemFile_Write(vadBook, &temp.cast.s16[4 + i], 2);
	}
	
	MemFile_Free(&temp);
	
	printf_info_align("Load Book", "%s", file);
}
