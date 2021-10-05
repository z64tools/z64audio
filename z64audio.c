#include "lib/AudioConvert.h"
#include "lib/HermosauhuLib.h"
#include "lib/AudioTools.h"

s32 main(const s32 argc, const char* argv[]) {
	MemFile wav = { 0 };
	MemFile aiff = { 0 };
	AudioSampleInfo sampleInfo = { 0 };
	
	printf_SetSuppressLevel(PSL_DEBUG);
	
	Audio_LoadWav(&wav, "test.wav", &sampleInfo);
	Audio_WriteAiff(&wav, &sampleInfo);
	
	free(wav.data);
	free(aiff.data);
	
	return 0;
}