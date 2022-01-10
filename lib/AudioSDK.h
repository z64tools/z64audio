#ifndef __AUDIO_SDK_H__
#define __AUDIO_SDK_H__
#include "ExtLib.h"

s32 inner_product(s32 length, s32* v1, s32* v2);
void clamp(s32 fs, f32* e, s32* ie, s32 bits);
s16 qsample(f32 x, s32 scale);
s32 clip(s32 ix, s32 llevel, s32 ulevel);
void acvect(short* in, int n, int m, double* out);
void acmat(short* in, int n, int m, double** out);
int lud(double** a, int n, int* indx, int* d);
void lubksb(double** a, int n, int* indx, double* b);
int kfroma(double* in, double* out, int n);
void afromk(double* in, double* out, int n);
void rfroma(double* arg0, int n, double* arg2);
int durbin(double* arg0, int n, double* arg2, double* arg3, double* outSomething);
void split(double** table, double* delta, int order, int npredictors, double scale);
double model_dist(double* arg0, double* arg1, int n);
void refine(double** table, int order, int npredictors, double** data, int dataSize, int refineIters, double unused);

#endif