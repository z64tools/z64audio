typedef signed char s8;
typedef unsigned char u8;
typedef signed short int s16;
typedef unsigned short int u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long int s64;
typedef unsigned long long int u64;
typedef float f32;
typedef double f64;
typedef long double f80;

#ifndef __Z64AUDIO_HEADER__
#define __Z64AUDIO_HEADER__

typedef union {
	u8*  data;
	s16* data16;
	s32* data32;
	f32* dataFloat;
} AudioSample;

typedef struct {
	u32 start;
	u32 end;
	u32 count;
} AudioLoop;

typedef struct {
	s8 note;
	s8 fineTune;
	u8 highNote;
	u8 lowNote;
	u8 loopNum;
	AudioLoop loop;
} AudioInstrument;

typedef struct {
	u8  channelNum;
	u8  bit;
	u32 sampleRate;
	u32 samplesNum;
	u32 size;
	AudioSample audio;
	AudioInstrument* instrument;
} AudioSampleInfo;

#endif

#ifndef __WAVE_HEADER__
#define __WAVE_HEADER__

typedef struct {
	char name[4];
	u32  size;
} WaveChunk;

typedef struct {
	WaveChunk chunk;
	u16 format; // PCM = 1 (uncompressed)
	u16 channelNum; // Mono = 1, Stereo = 2, etc.
	u32 sampleRate;
	u32 byteRate; // == sampleRate * channelNum * bit
	u16 blockAlign;
	u16 bit;
} WaveInfo;

typedef struct {
	WaveChunk chunk;
	u8 data[];
} WaveData;

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

#endif

#ifndef __AIFF_HEADER__
#define __AIFF_HEADER__

typedef struct {
	char name[4];
	u32  size;
} AiffChunk;

typedef struct {
	AiffChunk chunk;
	u32 formType;
} AiffChunkHeader;

typedef struct {
	s16 numChannels;
	u16 numFramesH;
	u16 numFramesL;
	s16 sampleSize;
	s8  sampleRate[10]; // 80-bit float
	u16 compressionTypeH;
	u16 compressionTypeL;
} CommonChunk;

typedef struct {
	s16 MarkerID;
	u16 positionH;
	u16 positionL;
	u16 string;
} Marker;

typedef struct {
	s16 playMode;
	s16 beginLoop;
	s16 endLoop;
} Loop;

typedef struct {
	s8   baseNote;
	s8   detune;
	s8   lowNote;
	s8   highNote;
	s8   lowVelocity;
	s8   highVelocity;
	s16  gain;
	Loop sustainLoop;
	Loop releaseLoop;
} InstrumentChunk;

typedef struct {
	s32 offset;
	s32 blockSize;
} SoundDataChunk;

typedef struct {
	s16 version;
	s16 order;
	s16 nEntries;
} CodeChunk;

typedef struct {
	u32 start;
	u32 end;
	u32 count;
	s16 state[16];
} ALADPCMloop;

#endif

void Audio_ByteSwapFloat80(f80* float80);
void Audio_ByteSwapData(AudioSampleInfo* audioInfo);
void Audio_LoadWav(void** dst, char* file, AudioSampleInfo* sampleInfo);