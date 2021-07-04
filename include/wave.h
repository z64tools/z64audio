#ifndef _WAVE_H_
#define _WAVE_H_

#include "int.h"

typedef struct {
	char ID[4];
	s32  size;
} WaveChunk;

typedef struct {
	WaveChunk chunk;
	s16      data[];
} WaveData;

typedef struct {
	WaveChunk chunk;
	char      format[4];
	// fmt
	WaveChunk subChunk_1;
	u16       audioFormat;
	u16       numChannels;
	u32       sampleRate;
	u32       byteRate;
	u16       blockAlign;
	u16       bitsPerSamp;
	// // data
	// WaveChunk subChunk_2;
	// s16       data[];
} WaveHeader;

typedef struct {
	u32 cuePointID;
	u32 type;
	u32 start;
	u32 end;
	u32 fraction;
	u32 count;
} SampleLoop;

typedef struct {
	WaveChunk  chunk; // Chunk Data Size == 36 + (Num Sample Loops * 24) + Sampler Data
	u32        manufacturer;
	u32        product;
	u32        samplePeriod;
	u32        unityNote; // 0-127
	u32        pitchFraction;
	u32        format;
	u32        offset;
	u32        numSampleLoops;
	u32        samplerData;
	SampleLoop loopData[];
} SampleHeader;

typedef struct {
	WaveChunk chunk;
	s8 note;
	s8 fineTune;
	s8 gain;
	s8 lowNote;
	s8 hiNote;
	s8 lowVel;
	s8 hiVel;
} InstrumentHeader;

#endif