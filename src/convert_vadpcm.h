#ifndef CONVERT_VADPCM_H
#define CONVERT_VADPCM_H

#include <ext_type.h>
#include "convert.h"

extern char* gTableDesignIteration;
extern char* gTableDesignFrameSize;
extern char* gTableDesignBits;
extern char* gTableDesignOrder;
extern char* gTableDesignThreshold;

void Audio_VadpcmTableDesign(AudioSample* sampleInfo);
void Audio_VadpcmEncode(AudioSample* sampleInfo);
void Audio_VadpcmDecode(AudioSample* sampleInfo);

void Audio_VadpcmBookLoad(AudioSample* sampleInfo, const char* file);

#endif
