#include "AudioSDK.h"

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
