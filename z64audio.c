#include "lib/AudioConvert.h"
#include "lib/HermosauhuLib.h"
#include "lib/AudioTools.h"

s32 main(const s32 argc, const char* argv[]) {
	void* audio = NULL;
	AudioSampleInfo sampleInfo = { 0 };
	
	printf_SetSuppressLevel(PSL_DEBUG);
	Audio_LoadWav(&audio, "test.wav", &sampleInfo);
	
	printf_debug(
		"SampleInfo:\n"
		"\t\tChannels:   %d\n"
		"\t\tBit:        %d\n"
		"\t\tSampleRate: %d\n"
		"\t\tSamplesNum: %d\n"
		"\t\tSize:       0x%X\n"
		"\t\tData:       %p\n"
		"\t\tInstrument: %p",
		sampleInfo.channelNum,
		sampleInfo.bit,
		sampleInfo.sampleRate,
		sampleInfo.samplesNum,
		sampleInfo.size,
		sampleInfo.audio.data,
		sampleInfo.instrument
	);
	
	free(audio);
	
	return 0;
}