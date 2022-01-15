#include "AudioSDK.h"

extern u32 gFrameSizeFlag;
s32 myrand() {
	static u64 state = 1619236481962341ULL;
	
	state *= 3123692312237ULL;
	state += 1;
	
	return state >> 33;
}

void clamp_to_s16(f32* in, s32* out) {
	f32 llevel = -0x8000;
	f32 ulevel = 0x7fff;
	
	for (s32 i = 0; i < 16; i++) {
		if (in[i] > ulevel) in[i] = ulevel;
		if (in[i] < llevel) in[i] = llevel;
		
		if (in[i] > 0.0f) {
			out[i] = (s32) (in[i] + 0.5);
		} else {
			out[i] = (s32) (in[i] - 0.5);
		}
	}
}

s16 clamp_bits(s32 x, s32 bits) {
	s32 lim = 1 << (bits - 1);
	
	if (x < -lim) return -lim;
	if (x > lim - 1) return lim - 1;
	
	return x;
}

void permute(s32* out, s32* in, s32* decompressed, s32 scale, u32 framesize) {
	int normal = myrand() % 3 == 0;
	
	for (s32 i = 0; i < 16; i++) {
		s32 lo = in[i] - scale / 2;
		s32 hi = in[i] + scale / 2;
		if (framesize == 9) {
			if (decompressed[i] == -8 && myrand() % 10 == 0) lo -= scale * 3 / 2;
			else if (decompressed[i] == 7 && myrand() % 10 == 0) hi += scale * 3 / 2;
		} else if (decompressed[i] == -2 && myrand() % 7 == 0) lo -= scale * 3 / 2;
		else if (decompressed[i] == 1 && myrand() % 10 == 0) hi += scale * 3 / 2;
		else if (normal) {
		} else if (decompressed[i] == 0) {
			if (myrand() % 3) {
				lo = in[i] - scale / 5;
				hi = in[i] + scale / 5;
			} else if (myrand() % 2) {
				lo = in[i] - scale / 3;
				hi = in[i] + scale / 3;
			}
		} else if (myrand() % 3) {
			if (decompressed[i] < 0) lo = in[i] + scale / 4;
			if (decompressed[i] > 0) hi = in[i] - scale / 4;
		} else if (myrand() % 2) {
			if (decompressed[i] < 0) lo = in[i] - scale / 4;
			if (decompressed[i] > 0) hi = in[i] + scale / 4;
		}
		out[i] = clamp_bits(lo + myrand() % (hi - lo + 1), 16);
	}
}

void get_bounds(s32* in, s32* decompressed, s32 scale, s32* minVals, s32* maxVals, u32 framesize) {
	s32 minv, maxv;
	
	if (framesize == 9) {
		minv = -8;
		maxv = 7;
	} else {
		minv = -2;
		maxv = 1;
	}
	for (s32 i = 0; i < 16; i++) {
		s32 lo = in[i] - scale / 2;
		s32 hi = in[i] + scale / 2;
		lo -= scale;
		hi += scale;
		if (decompressed[i] == minv) lo -= scale;
		else if (decompressed[i] == maxv) hi += scale;
		minVals[i] = lo;
		maxVals[i] = hi;
	}
}

s64 scored_encode(s32* inBuffer, s32* origState, s32*** coefTable, s32 order, s32 npredictors, s32 wantedPredictor, s32 wantedScale, s32 wantedIx[16], u32 framesize) {
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
	f32 errs[4];
	s64 scoreA = 0, scoreB = 0, scoreC = 0;
	
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
		
		errs[k] = se;
		
		if (se < min) {
			min = se;
			optimalp = k;
		}
	}
	
	for (s32 k = 0; k < npredictors; k++) {
		if (errs[k] < errs[wantedPredictor]) {
			scoreA += (s64)(errs[wantedPredictor] - errs[k]);
		}
	}
	if (optimalp != wantedPredictor) {
		// probably penalized above, but add extra penalty in case the error
		// values were the exact same
		scoreA += 1;
	}
	optimalp = wantedPredictor;
	
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
		if (abs(ie[i]) > abs(max)) {
			max = ie[i];
		}
	}
	
	for (scale = 0; scale <= scaleFactor; scale++) {
		if (max <= ulevel && max >= llevel) break;
		max /= 2;
	}
	
	// Preliminary ix computation, computes whether scale needs to be incremented
	s32 state[16];
	s32 again = 0;
	
	for (s32 j = 0; j < 2; j++) {
		s32 base = j * 8;
		for (s32 i = 0; i < order; i++) {
			inVector[i] = (j == 0 ?
				origState[16 - order + i] : state[8 - order + i]);
		}
		
		for (s32 i = 0; i < 8; i++) {
			prediction[base + i] = inner_product(order + i, coefTable[optimalp][i], inVector);
			f32 se = (f32) inBuffer[base + i] - (f32) prediction[base + i];
			s32 ix = qsample(se, 1 << scale);
			s32 clampedIx = clamp_bits(ix, encBits);
			s32 cV = clampedIx - ix;
			if (cV > 1 || cV < -1) {
				again = 1;
			}
			inVector[i + order] = clampedIx * (1 << scale);
			state[base + i] = prediction[base + i] + inVector[i + order];
		}
	}
	
	if (again && scale < scaleFactor) {
		scale++;
	}
	
	if (scale != wantedScale) {
		// We could do math to penalize scale mismatches accurately, but it's
		// simpler to leave it as a constraint by setting an infinite penalty.
		scoreB += 100000000;
		scale = wantedScale;
	}
	
	// Then again for real, but now also with penalty computation
	for (s32 j = 0; j < 2; j++) {
		s32 base = j * 8;
		for (s32 i = 0; i < order; i++) {
			inVector[i] = (j == 0 ?
				origState[16 - order + i] : state[8 - order + i]);
		}
		
		for (s32 i = 0; i < 8; i++) {
			prediction[base + i] = inner_product(order + i, coefTable[optimalp][i], inVector);
			s64 ise = (s64) inBuffer[base + i] - (s64) prediction[base + i];
			f32 se = (f32) inBuffer[base + i] - (f32) prediction[base + i];
			s32 ix = qsample(se, 1 << scale);
			s32 clampedIx = clamp_bits(ix, encBits);
			s32 val = wantedIx[base + i] * (1 << scale);
			if (clampedIx != wantedIx[base + i]) {
				assert(ix != wantedIx[base + i]);
				s32 lo = val - (1 << scale) / 2;
				s32 hi = val + (1 << scale) / 2;
				s64 diff = 0;
				if (ise < lo) diff = lo - ise;
				else if (ise > hi) diff = ise - hi;
				scoreC += diff * diff + 1;
			}
			inVector[i + order] = val;
			state[base + i] = prediction[base + i] + val;
		}
	}
	
	// Penalties for going outside s16
	for (s32 i = 0; i < 16; i++) {
		s64 diff = 0;
		if (inBuffer[i] < -0x8000) diff = -0x8000 - inBuffer[i];
		if (inBuffer[i] > 0x7fff) diff = inBuffer[i] - 0x7fff;
		scoreC += diff * diff;
	}
	
	return scoreA + scoreB + 10 * scoreC;
}

s32 descent(s32 guess[16], s32 minVals[16], s32 maxVals[16], u8 input[9], s32 lastState[16], s32*** coefTable, s32 order, s32 npredictors, s32 wantedPredictor, s32 wantedScale, s32 wantedIx[32], u32 framesize) {
	const f64 inf = 1e100;
	s64 curScore = scored_encode(guess, lastState, coefTable, order, npredictors, wantedPredictor, wantedScale, wantedIx, framesize);
	
	for (;;) {
		f64 delta[16];
		if (curScore == 0) {
			return 1;
		}
		
		// Compute gradient, and how far to move along it at most.
		f64 maxMove = inf;
		for (s32 i = 0; i < 16; i++) {
			guess[i] += 1;
			s64 scoreUp = scored_encode(guess, lastState, coefTable, order, npredictors, wantedPredictor, wantedScale, wantedIx, framesize);
			guess[i] -= 2;
			s64 scoreDown = scored_encode(guess, lastState, coefTable, order, npredictors, wantedPredictor, wantedScale, wantedIx, framesize);
			guess[i] += 1;
			if (scoreUp >= curScore && scoreDown >= curScore) {
				// Don't touch this coordinate
				delta[i] = 0;
			} else if (scoreDown < scoreUp) {
				if (guess[i] == minVals[i]) {
					// Don't allow moving out of bounds
					delta[i] = 0;
				} else {
					delta[i] = -(f64)(curScore - scoreDown);
					maxMove = fmin(maxMove, (minVals[i] - guess[i]) / delta[i]);
				}
			} else {
				if (guess[i] == maxVals[i]) {
					delta[i] = 0;
				} else {
					delta[i] = (f64)(curScore - scoreUp);
					maxMove = fmin(maxMove, (maxVals[i] - guess[i]) / delta[i]);
				}
			}
		}
		if (maxMove == inf || maxMove <= 0) {
			return 0;
		}
		
		// Try exponentially spaced candidates along the gradient.
		s32 nguess[16];
		s32 bestGuess[16];
		s64 bestScore = curScore;
		for (;;) {
			s32 changed = 0;
			for (s32 i = 0; i < 16; i++) {
				nguess[i] = (s32)round(guess[i] + delta[i] * maxMove);
				if (nguess[i] != guess[i]) changed = 1;
			}
			if (!changed) break;
			s64 score = scored_encode(nguess, lastState, coefTable, order, npredictors, wantedPredictor, wantedScale, wantedIx, framesize);
			if (score < bestScore) {
				bestScore = score;
				memcpy(bestGuess, nguess, sizeof(nguess));
			}
			maxMove *= 0.7;
		}
		
		if (bestScore == curScore) {
			// No improvements along that line, give up.
			return 0;
		}
		curScore = bestScore;
		memcpy(guess, bestGuess, sizeof(bestGuess));
	}
}

s32 bruteforce(s32 guess[16], u8 input[9], s32 decoded[16], s32 decompressed[16], s32 lastState[16], s32*** coefTable, s32 order, s32 npredictors, u32 framesize) {
	s32 scale = input[0] >> 4, predictor = input[0] & 0xF;
	
	s32 minVals[16], maxVals[16];
	
	get_bounds(decoded, decompressed, 1 << scale, minVals, maxVals, framesize);
	
	for (;;) {
		s64 bestScore = -1;
		s32 bestGuess[16];
		for (s32 it = 0; it < 100; it++) {
			permute(guess, decoded, decompressed, 1 << scale, framesize);
			s64 score = scored_encode(guess, lastState, coefTable, order, npredictors, predictor, scale, decompressed, framesize);
			if (score == 0) {
				return 1;
			}
			if (bestScore == -1 || score < bestScore) {
				bestScore = score;
				memcpy(bestGuess, guess, sizeof(bestGuess));
			}
		}
		memcpy(guess, bestGuess, sizeof(bestGuess));
		if (descent(guess, minVals, maxVals, input, lastState, coefTable, order, npredictors, predictor, scale, decompressed, framesize)) {
			return 1;
		}
	}
}

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
