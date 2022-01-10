#ifndef __AUDIOTOOLS_H__
#define __AUDIOTOOLS_H__

#include "ExtLib.h"
#include "AudioConvert.h"

extern char* gTableDesignIteration;
extern char* gTableDesignFrameSize;
extern char* gTableDesignBits;
extern char* gTableDesignOrder;
extern char* gTableDesignThreshold;

#if 0
	s32 vadpcm_dec(s32 argc, char** argv);
	s32 vadpcm_enc(s32 argc, char** argv);
	s32 tabledesign(s32 argc, char** argv, FILE* outstream);
	void AudioTools_RunTableDesign(AudioSampleInfo* sampleInfo);
	void AudioTools_RunVadpcmEnc(AudioSampleInfo* sampleInfo);
#endif

void AudioTools_TableDesign(AudioSampleInfo* sampleInfo);
void AudioTools_VadpcmEnc(AudioSampleInfo* sampleInfo);
void AudioTools_VadpcmDec(AudioSampleInfo* sampleInfo);

void AudioTools_LoadCodeBook(AudioSampleInfo* sampleInfo, char* file);

#endif