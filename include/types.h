#ifndef _Z64AUDIO_TYPES_H__
#define _Z64AUDIO_TYPES_H__

#include "vadpcm.h"
#include "wave.h"

typedef long double float80;

enum {
	false,
	true
};

typedef enum {
	NONE = 0
	, WAV
	, AIFF
	, AIFC
} FExt;

typedef enum  {
	Z64AUDIOMODE_UNSET = 0
	, Z64AUDIOMODE_WAV_TO_ZZRTL
	, Z64AUDIOMODE_WAV_TO_C
	, Z64AUDIOMODE_WAV_TO_AIFF
	, Z64AUDIOMODE_WAV_TO_AIFC
	, Z64AUDIOMODE_LAST
} z64audioMode;

typedef struct {
	struct {
		s8 aiff;
		s8 aifc;
		s8 table;
	} cleanState;
	z64audioMode mode;
	FExt         ftype;
	s32          sampleRate[3];
	s8 instDataFlag[3];
	s8 instLoop[3];
	struct {
		ALADPCMloop     loopInfo[3];
		InstrumentChunk instChunk[3];
		CommonChunk     commChunk[3];
	} vadpcmInfo;
	u16 sampleID[3];
} z64audioState;

typedef struct {
	s16 rate;
	s16 lvl;
} dcy;

typedef struct {
	u8  attack;
	dcy decay1;
	dcy decay2;
	u8  release;
} ADSR;

#endif
