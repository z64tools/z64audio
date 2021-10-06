#include "lib/AudioConvert.h"
#include "lib/HermosauhuLib.h"
#include "lib/AudioTools.h"

s32 main(const s32 argc, const char* argv[]) {
	AudioSampleInfo sample;
	
	printf_SetSuppressLevel(PSL_DEBUG);
	
	Audio_InitSampleInfo(&sample, "test.wav", "new_file.aiff");
	Audio_LoadSample(&sample);
	Audio_ConvertToMono(&sample);
	Audio_Normalize(&sample);
	Audio_SaveSample_Aiff(&sample);
	
	Audio_TableDesign(&sample);
	Audio_VadpcmEnc(&sample);
	
	Audio_FreeSample(&sample);
	
	return 0;
}