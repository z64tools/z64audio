#ifndef __AUDIOTOOLS_H__
#define __AUDIOTOOLS_H__

#include "HermosauhuLib.h"
#include "AudioConvert.h"

extern char* gTableDesignIteration;

s32 vadpcm_dec(s32 argc, char** argv);
s32 vadpcm_enc(s32 argc, char** argv);
s32 tabledesign(s32 argc, char** argv, FILE* outstream);
void AudioTools_TableDesign(AudioSampleInfo* sampleInfo);
void AudioTools_VadpcmEnc(AudioSampleInfo* sampleInfo);

#endif