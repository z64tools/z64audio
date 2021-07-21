#ifndef _AUDIOBANK_H_
#define _AUDIOBANK_H_

#include "types.h"

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
	ALADPCMTail     tail;
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
	u32          len;
	void*        addr; //Address within selected Audiotable
	ALADPCMLoopU* loop;
	ALADPCMBook* book; //Codebook
} ABSample;

typedef struct {
	ABSample* sample;
	f32       tuning;
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
	u8          unk_00;
	u8          unk_01;
	u8          unk_02; //0
	u8          unk_03; //0
	ABSound     sound;
	ABEnvelope* envelope;
} ABDrum;

typedef struct {
	u8          unk_00;
	u8          splitLow;
	u8          splitHigh;
	u8          releaseRate;
	ABEnvelope* envelope;
	ABSound     splits[3];
} ABInstrument;

typedef struct {
	ABDrum**      drums;         //size of array-of-pointers is numDrum
	ABSound*      sfx;           //size of array is numSfx;
	ABInstrument* instruments[]; //size of array is numInst
} ABBank;

typedef struct {
	ABBank* addr;          //Pointer to bank, relative to start of Audiobank
	u32     len;           //Bank Length
	s8      loadLocation;  //00 == RAM, 02 == ROM, maybe 01 or 03 are disk
	s8      seqPlayerIdx;  //00 == SFX, 01 == fanfares, 02 == BGM, 03 == cutscene SFX
	u8      audiotableIdx; //Which Audiotable is used for this bank
	u8      unk_0D;        //0xFF
	u8      numInst;
	u8      numDrum;
	u8      unk_0E;        //0x00
	u8      numSfx;
} ABIndexEntry;

#endif
