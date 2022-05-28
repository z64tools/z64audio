#ifndef __AUDIO_SDK_H__
#define __AUDIO_SDK_H__
#include <ExtLib.h>

s32 myrand();
s16 qsample(f32 x, s32 scale);
void clamp_to_s16(f32* in, s32* out);
s16 clamp_bits(s32 x, s32 bits);

s32 inner_product(s32 length, s32* v1, s32* v2);
void permute(s32* out, s32* in, s32* decompressed, s32 scale, u32 framesize);
void get_bounds(s32* in, s32* decompressed, s32 scale, s32* minVals, s32* maxVals, u32 framesize);
s64 scored_encode(s32* inBuffer, s32* origState, s32*** coefTable, s32 order, s32 npredictors, s32 wantedPredictor, s32 wantedScale, s32 wantedIx[16], u32 framesize);
s32 descent(s32 guess[16], s32 minVals[16], s32 maxVals[16], u8 input[9], s32 lastState[16], s32*** coefTable, s32 order, s32 npredictors, s32 wantedPredictor, s32 wantedScale, s32 wantedIx[16], u32 framesize);
s32 bruteforce(s32 guess[16], u8 input[9], s32 decoded[16], s32 decompressed[16], s32 lastState[16], s32*** coefTable, s32 order, s32 npredictors, u32 framesize);

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
void refine(double** table, int order, int npredictors, double** data, int dataSize, int refineIters);

#endif