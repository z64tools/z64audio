#include "lib/AudioConvert.h"
#include "lib/HermosauhuLib.h"
#include "lib/AudioTools.h"

s32 main(s32 argc, char* argv[]) {
	AudioSampleInfo sample;
	
	// printf_SetSuppressLevel(PSL_DEBUG);
	
	Audio_InitSampleInfo(&sample, argv[1], argv[2]);
	Audio_LoadSample(&sample);
	Audio_ConvertToMono(&sample);
	Audio_Normalize(&sample);
	Audio_SaveSample_Aiff(&sample);
	
	Audio_TableDesign(&sample);
	Audio_VadpcmEnc(&sample);
	
	Audio_FreeSample(&sample);
	
	return 0;
}