#ifndef VADPCM_H
#define VADPCM_H

#include <ext_type.h>

s32 myrand();
void split(double** table, double* delta, int order, int npredictors, double scale);

void refine(double** table, int order, int npredictors, double** data, int dataSize, int refineIters);
void rfroma(double* arg0, int n, double* arg2);
int durbin(double* arg0, int n, double* arg2, double* arg3, double* outSomething);
void acvect(short* in, int n, int m, double* out);
void acmat(short* in, int n, int m, double** out);
int lud(double** a, int n, int* indx, int* d);
void lubksb(double** a, int n, int* indx, double* b);
int kfroma(double* in, double* out, int n);
void afromk(double* in, double* out, int n);
s32 clip(s32 ix, s32 llevel, s32 ulevel);
void clamp_wow(s32 fs, f32* e, s32* ie, s32 bits);
s16 clamp_bits(s32 x, s32 bits);
s16 qsample(f32 x, s32 scale);
void clamp_to_s16(f32* in, s32* out);
s32 inner_product(s32 length, s32* v1, s32* v2);
s32 bruteforce(s32 guess[16], u8 input[9], s32 decoded[16], s32 decompressed[16], s32 lastState[16], s32*** coefTable, s32 order, s32 npredictors, u32 framesize);

#endif
