#ifndef __AUDIOCONVERT_H__
#define __AUDIOCONVERT_H__

#include "HermosauhuLib.h"

typedef long double f80;

struct AudioSampleInfo;

typedef void (* AudioFunc)(struct AudioSampleInfo*);

typedef struct {
	u32 start;
	u32 end;
	u32 count;
} SampleLoop;

typedef struct {
	s8 note;
	s8 fineTune;
	u8 highNote;
	u8 lowNote;
	SampleLoop loop;
} SampleInstrument;

typedef struct {
	PointerCast pred;
	PointerCast loopPred;
	u32 sizePred;
	u32 sizePredLoop;
} SampleVadpcmInfo;

typedef struct AudioSampleInfo {
	char*   input;
	char*   output;
	MemFile memFile;
	u8  channelNum;
	u8  bit;
	u32 sampleRate;
	u32 samplesNum;
	u32 size;
	u16 targetBit;
	PointerCast audio;
	u32 dataIsFloat;
	SampleInstrument instrument;
	SampleVadpcmInfo vadpcm;
} AudioSampleInfo;

#endif /* __Z64AUDIO_HEADER__ */

#ifndef __WAVE_HEADER__
#define __WAVE_HEADER__

typedef struct {
	char name[4];
	u32  size;
} WaveChunk;

typedef enum {
	PCM        = 1,
	IEEE_FLOAT = 3
} WaveFormat;

typedef struct {
	WaveChunk chunk;
	u16 format; // PCM = 1 (uncompressed)
	u16 channelNum; // Mono = 1, Stereo = 2, etc.
	u32 sampleRate;
	u32 byteRate; // == sampleRate * channelNum * bit
	u16 blockAlign; // == numChannels * (bit / 8)
	u16 bit;
} WaveInfo;

typedef struct {
	WaveChunk chunk;
	u8 data[];
} WaveDataInfo;

typedef struct {
	WaveChunk chunk;
	char format[4];
} WaveHeader;

typedef struct {
	u32 cuePointID;
	u32 type;
	u32 start;
	u32 end;
	u32 fraction;
	u32 count;
} WaveSampleLoop;

typedef struct {
	WaveChunk chunk; // Chunk Data Size == 36 + (Num Sample Loops * 24) + Sampler Data
	u32 manufacturer;
	u32 product;
	u32 samplePeriod;
	u32 unityNote; // 0-127
	u32 pitchFraction;
	u32 format;
	u32 offset;
	u32 numSampleLoops;
	u32 samplerData;
	WaveSampleLoop loopData[];
} WaveSampleInfo;

typedef struct {
	WaveChunk chunk;
	s8 note;
	s8 fineTune;
	s8 gain;
	s8 lowNote;
	s8 hiNote;
	s8 lowVel;
	s8 hiVel;
} WaveInstrumentInfo;

#endif /* __WAVE_HEADER__ */

#ifndef __AIFF_HEADER__
#define __AIFF_HEADER__

typedef struct {
	char name[4];
	u32  size;
} AiffChunk;

typedef struct {
	AiffChunk chunk; // FORM
	char formType[4]; // AIFF
} AiffHeader;

typedef struct {
	AiffChunk chunk; // COMM
	u16  channelNum;
	u16  sampleNumH;
	u16  sampleNumL;
	u16  bit;
	u8   sampleRate[10]; // 80-bit float
	char compressionType[4];
} AiffInfo;

typedef struct {
	u16 index;
	u16 positionH;
	u16 positionL;
	u16 pad;
} AiffMarker;

typedef struct {
	AiffChunk  chunk; // MARK
	u16 markerNum;
	AiffMarker marker[];
} AiffMarkerInfo;

typedef struct {
	u16 playMode;
	u16 start;
	u16 end;
} AiffLoop;

typedef struct {
	AiffChunk chunk; // INST
	s8  baseNote;
	s8  detune;
	s8  lowNote;
	s8  highNote;
	s8  lowVelocity;
	s8  highVelocity;
	s16 gain;
	AiffLoop sustainLoop;
	AiffLoop releaseLoop;
} AiffInstrumentInfo;

typedef struct {
	AiffChunk chunk; // SSND
	u32 offset;
	u32 blockSize;
	u8  data[];
} AiffDataInfo;

typedef struct {
	AiffChunk chunk; // APPL
	char type[0x10]; // stoc\x0BVADPCMLOOPS stoc\x0BVADPCMCODES
	u8   data[];
} VadpcmInfo;

#endif /* __AIFF_HEADER__ */

#ifndef __AUDIOBANK__
#define __AUDIOBANK__

typedef struct {
	s16 data[0x10];
} ALADPCMTail;

typedef struct {
	u32 start;
	u32 end;
	u32 count;
	u32 adpcmState;
} ALADPCMLoopMain;

typedef struct {
	ALADPCMLoopMain main;
	ALADPCMTail tail;
} ALADPCMLoopTail;

typedef union {
	ALADPCMLoopMain notail;
	ALADPCMLoopTail withtail;
} ALADPCMLoopU;

typedef struct {
	s16 data[0x10]; //size of array is order?
} ALADPCMPredictor;

typedef struct {
	s32 order;
	s32 numPredictors;
	ALADPCMPredictor predictors[]; //size of array is numPredictors
} ALADPCMBook;

typedef struct {
	u32   len;
	void* addr; //Address within selected Audiotable
	ALADPCMLoopU* loop;
	ALADPCMBook*  book; //Codebook
} ABSample;

typedef struct {
	ABSample* sample;
	f32 tuning;
} ABSound;

typedef struct {
	s16 rate;
	u16 level;
} ABEnvelopePoint;

typedef struct {
	ABEnvelopePoint points[4]; //Usually 4 in music sounds, but actually defined
	//to be any number of points up to a point with rate -1 and level 0
} ABEnvelope;

typedef struct {
	u8 unk_00;
	u8 unk_01;
	u8 unk_02; //0
	u8 unk_03; //0
	ABSound sound;
	ABEnvelope* envelope;
} ABDrum;

typedef struct {
	u8 unk_00;
	u8 splitLow;
	u8 splitHigh;
	u8 releaseRate;
	ABEnvelope* envelope;
	ABSound splits[3];
} ABInstrument;

typedef struct {
	ABDrum** drums; //size of array-of-pointers is numDrum
	ABSound* sfx; //size of array is numSfx;
	ABInstrument* instruments[]; //size of array is numInst
} ABBank;

typedef struct {
	ABBank* addr; //Pointer to bank, relative to start of Audiobank
	u32 len; //Bank Length
	s8  loadLocation; //00 == RAM, 02 == ROM, maybe 01 or 03 are disk
	s8  seqPlayerIdx; //00 == SFX, 01 == fanfares, 02 == BGM, 03 == cutscene SFX
	u8  audiotableIdx; //Which Audiotable is used for this bank
	u8  unk_0D; //0xFF
	u8  numInst;
	u8  numDrum;
	u8  unk_0E; //0x00
	u8  numSfx;
} ABIndexEntry;

#endif

void Audio_ByteSwap(AudioSampleInfo* sampleInfo);
void Audio_Normalize(AudioSampleInfo* sampleInfo);
void Audio_ConvertToMono(AudioSampleInfo* sampleInfo);
void Audio_Resample(AudioSampleInfo* sampleInfo);

void Audio_InitSampleInfo(AudioSampleInfo* sampleInfo, char* input, char* output);
void Audio_FreeSample(AudioSampleInfo* sampleInfo);

void Audio_LoadSample_Wav(AudioSampleInfo* sampleInfo);
void Audio_LoadSample_Aiff(AudioSampleInfo* sampleInfo);
void Audio_LoadSample(AudioSampleInfo* sampleInfo);

void Audio_SaveSample_Aiff(AudioSampleInfo* sampleInfo);
void Audio_SaveSample_Wav(AudioSampleInfo* sampleInfo);
void Audio_SaveSample(AudioSampleInfo* sampleInfo);