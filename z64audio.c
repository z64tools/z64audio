#include "lib/AudioConvert.h"
#include "lib/HermosauhuLib.h"
#include "lib/AudioTools.h"

s32 main(s32 argc, char* argv[]) {
	AudioSampleInfo sample;
	char* tarBit = NULL;
	
	if (argc < 3)
		printf_error("Whoops!");
	
	if (argc == 4)
		tarBit = argv[3];
	
	printf_SetSuppressLevel(PSL_DEBUG);
	
	Audio_InitSampleInfo(&sample, argv[1], argv[2], tarBit);
	Audio_LoadSample(&sample);
	Audio_Resample(&sample);
	Audio_ConvertToMono(&sample);
	Audio_Normalize(&sample);
	Audio_SaveSample(&sample);
	
	// Audio_TableDesign(&sample);
	// Audio_VadpcmEnc(&sample);
	
	Audio_FreeSample(&sample);
	
	return 0;
}