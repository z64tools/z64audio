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
} FExt;

typedef enum  {
	Z64AUDIOMODE_UNSET = 0
	, Z64AUDIOMODE_WAV_TO_ZZRTL
	, Z64AUDIOMODE_WAV_TO_AIFF
	, Z64AUDIOMODE_LAST
} z64audioMode;

typedef struct {
	struct {
		s8 aiff;
		s8 aifc;
		s8 table;
	} cleanState;
	FExt         ftype;
	z64audioMode mode;
	s8 instDataFlag[3];
	s32          sampleRate[3];
} z64audioState;

typedef struct {
	u8 release;
} ADSR;

#endif
