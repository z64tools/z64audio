#ifndef __AUDIOCONVERT_H__
#define __AUDIOCONVERT_H__

#include "ExtLib.h"

typedef long double f80;

struct AudioSampleInfo;

typedef void (* AudioFunc)(struct AudioSampleInfo*);

typedef struct {
	u32 start;
	u32 end;
	u32 oldEnd;
	u32 count;
} SampleLoop;

typedef struct {
	s8 note;
	s8 fineTune;
	u8 highNote;
	u8 lowNote;
	SampleLoop loop;
} SampleInstrument;

typedef struct AudioSampleInfo {
	char*   input;
	char*   output;
	bool    useExistingPred;
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
	MemFile vadBook;
	MemFile vadLoopBook;
} AudioSampleInfo;

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
		s8  _pad[3];
		s8  fineTune;
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

typedef enum {
	NAMEPARAM_DEFAULT,
	NAMEPARAM_ZZRTL,
} NameParam;

extern NameParam gBinNameIndex;
extern u32 gSampleRate;
extern u32 gPrecisionFlag;
extern f32 gTuning;
extern bool gRomMode;

void Audio_ByteSwap(AudioSampleInfo* sampleInfo);
void Audio_Normalize(AudioSampleInfo* sampleInfo);
void Audio_ConvertToMono(AudioSampleInfo* sampleInfo);
void Audio_Resample(AudioSampleInfo* sampleInfo);
void Audio_Compare(AudioSampleInfo* sampleA, AudioSampleInfo* sampleB);

void Audio_InitSampleInfo(AudioSampleInfo* sampleInfo, char* input, char* output);
void Audio_FreeSample(AudioSampleInfo* sampleInfo);

void Audio_LoadSample_Wav(AudioSampleInfo* sampleInfo);
void Audio_LoadSample_Aiff(AudioSampleInfo* sampleInfo);
void Audio_LoadSample(AudioSampleInfo* sampleInfo);

void Audio_SaveSample_Aiff(AudioSampleInfo* sampleInfo);
void Audio_SaveSample_Wav(AudioSampleInfo* sampleInfo);
void Audio_SaveSample(AudioSampleInfo* sampleInfo);

void Audio_ZZRTLMode(AudioSampleInfo* sampleInfo, char* input);

#endif /* __Z64AUDIO_HEADER__ */