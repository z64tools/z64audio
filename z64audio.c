#include "lib/AudioConvert.h"
#include "lib/HermosauhuLib.h"
#include "lib/AudioTools.h"

s32 main(const s32 argc, const char* argv[]) {
	void* wav = NULL;
	void* aiff = NULL;
	AudioSampleInfo sampleInfo = { 0 };
	
	printf_SetSuppressLevel(PSL_DEBUG);
	Audio_LoadWav(&wav, "test.wav", &sampleInfo);
	Audio_LoadAiff(&aiff, "test.aiff", &sampleInfo);
	
	printf_info(
		"%d - %d\n%d-bit\n",
		sampleInfo.instrument.loop.start,
		sampleInfo.instrument.loop.end,
		sampleInfo.bit
	);
	
	free(wav);
	free(aiff);
	
	return 0;
}