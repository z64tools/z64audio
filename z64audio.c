#include "lib/HermosauhuLib.h"
#include "lib/AudioConvert.h"
#include "lib/AudioTools.h"

char* sToolName = "z64audio 2.0 cli rc1";

s32 main(s32 argc, char* argv[]) {
	AudioSampleInfo sample;
	char* input = NULL;
	char* output = NULL;
	u32 parArg;
	
	printf_SetPrefix("");
	
	if (Lib_ParseArguments(argv, "-i", &parArg) || Lib_ParseArguments(argv, "--i", &parArg)) {
		input = argv[parArg];
	}
	
	if (Lib_ParseArguments(argv, "-o", &parArg) || Lib_ParseArguments(argv, "--o", &parArg)) {
		output = argv[parArg];
	}
	
	if (Lib_ParseArguments(argv, "-D", &parArg) || Lib_ParseArguments(argv, "--D", &parArg)) {
		printf_SetSuppressLevel(PSL_DEBUG);
	}
	
	if (Lib_ParseArguments(argv, "-s", &parArg) || Lib_ParseArguments(argv, "--s", &parArg)) {
		printf_SetSuppressLevel(PSL_NO_WARNING);
	}
	
	if (input == NULL || output == NULL) {
		printf_toolinfo(
			sToolName,
			"--i [file] ─ Input\n"
			"--o [file] ─ Output\n"
			
			"\nAudio processing:\n"
			"--b [16]   ─ Bit depth\n"
			"--m        ─ Mono\n"
			"--n        ─ Normalize\n"
			
			"\nVadpcm Args:\n"
			"--v        ─ Generate Vadpcm AIFC\n"
			"--I [30]   ─ tabledesign iteration count\n"
			
			"\nInternal tools:\n"
			"[TODO] tabledesign [iteration] [input] [output]\n"
			"[TODO] vadpcm_enc [input] [table] [output]\n"
			
			"\nPrintf:\n"
			"--D ─ Debug\n"
			"--s ─ Silence\n"
		);
		
		return 1;
	}
	
	printf_toolinfo(
		sToolName,
		0
	);
	
	Audio_InitSampleInfo(&sample, input, output);
	Audio_LoadSample(&sample);
	if (Lib_ParseArguments(argv, "-b", &parArg) || Lib_ParseArguments(argv, "--b", &parArg)) {
		sample.targetBit = String_NumStrToInt(argv[parArg]);
		
		if (sample.targetBit != 16 && sample.targetBit != 32) {
			u32 temp = sample.targetBit;
			Audio_FreeSample(&sample);
			printf_error("Bit depth [%d] is not supported.", temp);
		}
		
		Audio_Resample(&sample);
	}
	if (Lib_ParseArguments(argv, "-m", &parArg) || Lib_ParseArguments(argv, "--m", &parArg)) {
		Audio_ConvertToMono(&sample);
	}
	if (Lib_ParseArguments(argv, "-n", &parArg) || Lib_ParseArguments(argv, "--n", &parArg)) {
		Audio_Normalize(&sample);
	}
	Audio_SaveSample(&sample);
	
	if (Lib_ParseArguments(argv, "-v", &parArg) || Lib_ParseArguments(argv, "--v", &parArg)) {
		if (Lib_ParseArguments(argv, "-I", &parArg) || Lib_ParseArguments(argv, "--I", &parArg)) {
			gTableDesignIteration = argv[parArg];
		}
		if (!Lib_MemMem(sample.output, strlen(sample.output), ".aiff", 5)) {
			printf_warning("Output isn't [.aiff] file. Skipping generating vadpcm files");
		} else {
			AudioTools_RunTableDesign(&sample);
			AudioTools_RunVadpcmEnc(&sample);
		}
	}
	Audio_FreeSample(&sample);
	
	return 0;
}