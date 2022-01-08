#include "AudioTools.h"

s32 inner_product(s32 length, s32* v1, s32* v2) {
	s32 j;
	s32 dout;
	s32 fiout;
	s32 out;
	
	j = 0;
	out = 0;
	for (; j < length; j++) {
		out += *v1++ **v2++;
	}
	
	// Compute "out / 2^11", rounded down.
	dout = out / (1 << 11);
	fiout = dout * (1 << 11);
	if (out - fiout < 0) {
		return dout - 1;
	} else {
		return dout;
	}
}
void clamp(s32 fs, f32* e, s32* ie, s32 bits) {
	s32 i;
	f32 ulevel;
	f32 llevel;
	
	llevel = -(f32) (1 << (bits - 1));
	ulevel = -llevel - 1.0f;
	for (i = 0; i < fs; i++) {
		if (e[i] > ulevel) {
			e[i] = ulevel;
		}
		if (e[i] < llevel) {
			e[i] = llevel;
		}
		
		if (e[i] > 0.0f) {
			ie[i] = (s32) (e[i] + 0.5);
		} else {
			ie[i] = (s32) (e[i] - 0.5);
		}
	}
}
s16 qsample(f32 x, s32 scale) {
	if (x > 0.0f) {
		return (s16) ((x / scale) + 0.4999999);
	} else {
		return (s16) ((x / scale) - 0.4999999);
	}
}
s32 clip(s32 ix, s32 llevel, s32 ulevel) {
	if (ix < llevel || ix > ulevel) {
		if (ix < llevel) {
			return llevel;
		}
		if (ix > ulevel) {
			return ulevel;
		}
	}
	
	return ix;
}
void acvect(short* in, int n, int m, double* out) {
	int i, j;
	
	for (i = 0; i <= n; i++) {
		out[i] = 0.0;
		for (j = 0; j < m; j++) {
			out[i] -= in[j - i] * in[j];
		}
	}
}
void acmat(short* in, int n, int m, double** out) {
	int i, j, k;
	
	for (i = 1; i <= n; i++) {
		for (j = 1; j <= n; j++) {
			out[i][j] = 0.0;
			for (k = 0; k < m; k++) {
				out[i][j] += in[k - i] * in[k - j];
			}
		}
	}
}
int lud(double** a, int n, int* indx, int* d) {
	int i, imax, j, k;
	double big, dum, sum, temp;
	double min, max;
	double* vv;
	
	vv = malloc((n + 1) * sizeof(double));
	*d = 1;
	for (i = 1; i<=n; i++) {
		big = 0.0;
		for (j = 1; j<=n; j++)
			if ((temp = fabs(a[i][j])) > big) big = temp;
		if (big == 0.0) return 1;
		vv[i] = 1.0 / big;
	}
	for (j = 1; j<=n; j++) {
		for (i = 1; i<j; i++) {
			sum = a[i][j];
			for (k = 1; k<i; k++) sum -= a[i][k] * a[k][j];
			a[i][j] = sum;
		}
		big = 0.0;
		for (i = j; i<=n; i++) {
			sum = a[i][j];
			for (k = 1; k<j; k++)
				sum -= a[i][k] * a[k][j];
			a[i][j] = sum;
			if ( (dum = vv[i] * fabs(sum)) >= big) {
				big = dum;
				imax = i;
			}
		}
		if (j != imax) {
			for (k = 1; k<=n; k++) {
				dum = a[imax][k];
				a[imax][k] = a[j][k];
				a[j][k] = dum;
			}
			*d = -(*d);
			vv[imax] = vv[j];
		}
		indx[j] = imax;
		if (a[j][j] == 0.0) return 1;
		if (j != n) {
			dum = 1.0 / (a[j][j]);
			for (i = j + 1; i<=n; i++) a[i][j] *= dum;
		}
	}
	free(vv);
	
	min = 1e10;
	max = 0.0;
	for (i = 1; i <= n; i++) {
		temp = fabs(a[i][i]);
		if (temp < min) min = temp;
		if (temp > max) max = temp;
	}
	
	return min / max < 1e-10 ? 1 : 0;
}
void lubksb(double** a, int n, int* indx, double* b) {
	int i, ii = 0, ip, j;
	double sum;
	
	for (i = 1; i<=n; i++) {
		ip = indx[i];
		sum = b[ip];
		b[ip] = b[i];
		if (ii)
			for (j = ii; j<=i - 1; j++) sum -= a[i][j] * b[j];
		else if (sum) ii = i;
		b[i] = sum;
	}
	for (i = n; i>=1; i--) {
		sum = b[i];
		for (j = i + 1; j<=n; j++) sum -= a[i][j] * b[j];
		b[i] = sum / a[i][i];
	}
}
int kfroma(double* in, double* out, int n) {
	int i, j;
	double div;
	double temp;
	double* next;
	int ret;
	
	ret = 0;
	next = malloc((n + 1) * sizeof(double));
	
	out[n] = in[n];
	for (i = n - 1; i >= 1; i--) {
		for (j = 0; j <= i; j++) {
			temp = out[i + 1];
			div = 1.0 - (temp * temp);
			if (div == 0.0) {
				free(next);
				
				return 1;
			}
			next[j] = (in[j] - in[i + 1 - j] * temp) / div;
		}
		
		for (j = 0; j <= i; j++) {
			in[j] = next[j];
		}
		
		out[i] = next[i];
		if (fabs(out[i]) > 1.0) {
			ret++;
		}
	}
	
	free(next);
	
	return ret;
}
void afromk(double* in, double* out, int n) {
	int i, j;
	
	out[0] = 1.0;
	for (i = 1; i <= n; i++) {
		out[i] = in[i];
		for (j = 1; j <= i - 1; j++) {
			out[j] += out[i - j] * out[i];
		}
	}
}
void rfroma(double* arg0, int n, double* arg2) {
	int i, j;
	double** mat;
	double div;
	
	mat = malloc((n + 1) * sizeof(double*));
	mat[n] = malloc((n + 1) * sizeof(double));
	mat[n][0] = 1.0;
	for (i = 1; i <= n; i++) {
		mat[n][i] = -arg0[i];
	}
	
	for (i = n; i >= 1; i--) {
		mat[i - 1] = malloc(i * sizeof(double));
		div = 1.0 - mat[i][i] * mat[i][i];
		for (j = 1; j <= i - 1; j++) {
			mat[i - 1][j] = (mat[i][i - j] * mat[i][i] + mat[i][j]) / div;
		}
	}
	
	arg2[0] = 1.0;
	for (i = 1; i <= n; i++) {
		arg2[i] = 0.0;
		for (j = 1; j <= i; j++) {
			arg2[i] += mat[i][j] * arg2[i - j];
		}
	}
	
	free(mat[n]);
	for (i = n; i > 0; i--) {
		free(mat[i - 1]);
	}
	free(mat);
}
int durbin(double* arg0, int n, double* arg2, double* arg3, double* outSomething) {
	int i, j;
	double sum, div;
	int ret;
	
	arg3[0] = 1.0;
	div = arg0[0];
	ret = 0;
	
	for (i = 1; i <= n; i++) {
		sum = 0.0;
		for (j = 1; j <= i - 1; j++) {
			sum += arg3[j] * arg0[i - j];
		}
		
		arg3[i] = (div > 0.0 ? -(arg0[i] + sum) / div : 0.0);
		arg2[i] = arg3[i];
		
		if (fabs(arg2[i]) > 1.0) {
			ret++;
		}
		
		for (j = 1; j < i; j++) {
			arg3[j] += arg3[i - j] * arg3[i];
		}
		
		div *= 1.0 - arg3[i] * arg3[i];
	}
	*outSomething = div;
	
	return ret;
}
void split(double** table, double* delta, int order, int npredictors, double scale) {
	int i, j;
	
	for (i = 0; i < npredictors; i++) {
		for (j = 0; j <= order; j++) {
			table[i + npredictors][j] = table[i][j] + delta[j] * scale;
		}
	}
}
double model_dist(double* arg0, double* arg1, int n) {
	double* sp3C;
	double* sp38;
	double ret;
	int i, j;
	
	sp3C = malloc((n + 1) * sizeof(double));
	sp38 = malloc((n + 1) * sizeof(double));
	rfroma(arg1, n, sp3C);
	
	for (i = 0; i <= n; i++) {
		sp38[i] = 0.0;
		for (j = 0; j <= n - i; j++) {
			sp38[i] += arg0[j] * arg0[i + j];
		}
	}
	
	ret = sp38[0] * sp3C[0];
	for (i = 1; i <= n; i++) {
		ret += 2 * sp3C[i] * sp38[i];
	}
	
	free(sp3C);
	free(sp38);
	
	return ret;
}
void refine(double** table, int order, int npredictors, double** data, int dataSize, int refineIters, double unused) {
	int iter; // spD8
	double** rsums;
	int* counts; // spD0
	double* temp_s7;
	double dist;
	double dummy; // spC0
	double bestValue;
	int bestIndex;
	int i, j;
	
	rsums = malloc(npredictors * sizeof(double*));
	for (i = 0; i < npredictors; i++) {
		rsums[i] = malloc((order + 1) * sizeof(double));
	}
	
	counts = malloc(npredictors * sizeof(int));
	temp_s7 = malloc((order + 1) * sizeof(double));
	
	for (iter = 0; iter < refineIters; iter++) {
		for (i = 0; i < npredictors; i++) {
			counts[i] = 0;
			for (j = 0; j <= order; j++) {
				rsums[i][j] = 0.0;
			}
		}
		
		for (i = 0; i < dataSize; i++) {
			bestValue = 1e30;
			bestIndex = 0;
			
			for (j = 0; j < npredictors; j++) {
				dist = model_dist(table[j], data[i], order);
				if (dist < bestValue) {
					bestValue = dist;
					bestIndex = j;
				}
			}
			
			counts[bestIndex]++;
			rfroma(data[i], order, temp_s7);
			for (j = 0; j <= order; j++) {
				rsums[bestIndex][j] += temp_s7[j];
			}
		}
		
		for (i = 0; i < npredictors; i++) {
			if (counts[i] > 0) {
				for (j = 0; j <= order; j++) {
					rsums[i][j] /= counts[i];
				}
			}
		}
		
		for (i = 0; i < npredictors; i++) {
			durbin(rsums[i], order, temp_s7, table[i], &dummy);
			
			for (j = 1; j <= order; j++) {
				if (temp_s7[j] >=  1.0) temp_s7[j] =  0.9999999999;
				if (temp_s7[j] <= -1.0) temp_s7[j] = -0.9999999999;
			}
			
			afromk(temp_s7, table[i], order);
		}
	}
	
	free(counts);
	for (i = 0; i < npredictors; i++) {
		free(rsums[i]);
	}
	free(rsums);
	free(temp_s7);
}

char* gTableDesignIteration = "2";
char* gTableDesignFrameSize = "16";
char* gTableDesignBits = "2";
char* gTableDesignOrder = "2";
char* gTableDesignThreshold = "10.0";

#if 0
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
#endif

void AudioTools_ReadCodeBook(AudioSampleInfo* sampleInfo, s32**** table, s32* destOrder, s32* destNPred) {
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
void AudioTools_VencodeFrame(AudioSampleInfo* sampleInfo, MemFile* mem, s16* buffer, s32* state, s32*** coefTable, s32 nSam) {
	s32 order = sampleInfo->vadBook.cast.u16[0];
	s32 nPred = sampleInfo->vadBook.cast.u16[1];
	
	// We are only given 'nsam' samples; pad with zeroes to 16.
	for (s32 i = nSam; i < 16; i++) {
		buffer[i] = 0;
	}
	
	s32 inVector[16];
	s32 saveState[16];
	s16 prediction[16];
	s16 ix[16];
	f32 e[16];
	s32 ie[16];
	f32 se;
	
	s32 llevel = -8;
	s32 ulevel = -llevel - 1;
	f32 min = 1e30;
	s32 optimalp = 0;
	
	// Determine the best-fitting predictor.
	for (s32 k = 0; k < nPred; k++) {
		// Copy over the last 'order' samples from the previous output.
		for (s32 i = 0; i < order; i++) {
			inVector[i] = state[16 - order + 1];
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
		if (ABS(ie[i]) > ABS(max)) {
			max = ie[i];
		}
	}
	
	// Compute which power of two we need to scale down by in order to make
	// all values representable as 4-bit signed integers (i.e. be in [-8, 7]).
	// The worst-case 'max' is -2^15, so this will be at most 12.
	s32 scale;
	
	for (scale = 0; scale <= 12; scale++) {
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
		nIter++;
		s32 maxClip = 0;
		s32 cV;
		scale++;
		if (scale > 12) {
			scale = 12;
		}
		
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
			if (maxClip < ABS(cV)) {
				maxClip = ABS(cV);
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
			if (maxClip < abs(cV)) {
				maxClip = abs(cV);
			}
			ix[8 + i] += cV;
			inVector[i + order] = ix[8 + i] * (1 << scale);
			state[8 + i] = prediction[8 + i] + inVector[i + order];
		}
	} while (maxClip >= 2 && nIter < 2);
	
	// The scale, the predictor index, and the 16 computed outputs are now all
	// 4-bit numbers. Write them out as 1 + 8 bytes.
	u8 header = (scale << 4) | (optimalp & 0xf);
	
	MemFile_Write(mem, &header, sizeof(u8));
	for (s32 i = 0; i < 16; i += 2) {
		u8 c = (ix[i] << 4) | (ix[i + 1] & 0xf);
		MemFile_Write(mem, &c, sizeof(u8));
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
	f64 threshold = String_NumStrToF64(gTableDesignThreshold);
	u32 frameCount = sampleInfo->samplesNum;
	
	printf_debug(
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
	if (sampleInfo->vadBook.data == NULL) {
		AudioTools_TableDesign(sampleInfo);
	}
	MemFile memEnc;
	s32 minLoopLength = 30;
	u32 aI = 0;
	u32 aO = 0;
	u32 nO = 0;
	u32 pos = 0;
	s32 order;
	s32 nPred;
	u32 nSam;
	
	s32 state[16] = { 0 };
	s16 buffer[16] = { 0 };
	s32*** table = NULL;
	u32 nFrames = sampleInfo->samplesNum;
	
	MemFile_Malloc(&memEnc, sampleInfo->size * 2);
	AudioTools_ReadCodeBook(sampleInfo, &table, &order, &nPred);
	
	// Process Loop
	if (sampleInfo->instrument.loop.count) {
		MemFile_Malloc(&sampleInfo->vadLoopBook, sizeof(s16) * 16);
		u32 nRepeats = 0;
		
		aI = sampleInfo->instrument.loop.start;
		aO = sampleInfo->instrument.loop.end;
		nO = sampleInfo->instrument.loop.end;
		
		while (nO - aI < minLoopLength) {
			nRepeats++;
			nO += aO - aI;
		}
		
		sampleInfo->instrument.loop.end = nO;
		
		while (pos <= aI) {
			memcpy(buffer, &sampleInfo->audio.s16[pos], sizeof(s16) * 16);
			AudioTools_VencodeFrame(sampleInfo, &memEnc, buffer, state, table, 16);
			pos += 16;
		}
		
		for (s32 i = 0; i < 16; i++) {
			state[i] = CLAMP(state[i], -0x7FFF, 0x7FFF);
			sampleInfo->vadLoopBook.cast.s16[i] = state[i];
		}
		
		while (nRepeats > 0) {
			for (; pos + 16 < aO; pos += 16) {
				memcpy(buffer, &sampleInfo->audio.s16[pos], sizeof(s16) * 16);
				AudioTools_VencodeFrame(sampleInfo, &memEnc, buffer, state, table, 16);
			}
			
			u32 left = aO - pos;
			memcpy(buffer, &sampleInfo->audio.s16[pos], sizeof(s16) * left);
			memcpy(&buffer[left], &sampleInfo->audio.s16[aI], sizeof(s16) * (16 - left));
			AudioTools_VencodeFrame(sampleInfo, &memEnc, buffer, state, table, 16);
			pos = aI - left + 16;
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
			AudioTools_VencodeFrame(sampleInfo, &memEnc, buffer, state, table, nSam);
			pos += nSam;
		} else {
			printf_warning("VadpcmEnc: Missed a frame!");
			break;
		}
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
	
	printf_debugExt("Old MemFile Size [0x%X]", sampleInfo->size);
	
	MemFile_Free(&sampleInfo->memFile);
	sampleInfo->memFile = memEnc;
	sampleInfo->audio.p = memEnc.data;
	sampleInfo->size = memEnc.dataSize;
	
	printf_debugExt("New MemFile Size [0x%X]", sampleInfo->size);
	
	printf_debugExt("LoopBook:");
	
	if (gPrintfSuppress <= PSL_DEBUG) {
		for (s32 i = 0; i < (0x8 * sampleInfo->vadBook.cast.u16[0]) * sampleInfo->vadBook.cast.u16[1]; i++) {
			printf("%6d", sampleInfo->vadBook.cast.s16[2 + i]);
			if ((i % 8) == 7)
				printf("\n");
		}
	}
}
void AudioTools_LoadCodeBook(AudioSampleInfo* sampleInfo, char* file) {
	MemFile temp = { 0 };
	MemFile* vadBook = &sampleInfo->vadBook;
	u32 size;
	u16 order;
	u16 nPred;
	
	if (vadBook) {
		printf_debugExt("VadBook already exists");
	}
	
	printf_debugExt("Loading Predictors [%s]", file);
	MemFile_LoadFile(&temp, file);
	for (s32 i = 0; i < temp.dataSize / 2; i++) {
		Lib_ByteSwap(&temp.cast.u16[i], SWAP_U16);
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
}
