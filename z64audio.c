#include "lib/HermosauhuLib.h"
#include "lib/AudioConvert.h"
#include "lib/AudioTools.h"

#define ParseArg(xarg)         Lib_ParseArguments(argv, xarg, &parArg)
#define Z64ARGTITLE(xtitle)    "\e[96m" xtitle PRNT_RNL
#define Z64ARGX(xarg, comment) xarg "\r\033[18C" PRNT_GRAY "// " comment PRNT_RNL
#define Z64ARTD(xarg, comment) PRNT_DGRY "\e[9m" xarg PRNT_RSET "\r\033[18C" PRNT_DGRY "// " comment PRNT_RSET " " PRNT_TODO PRNT_RNL

char* sToolName = {
	"ᴢ64ᴀᴜᴅɪᴏ 2.0 ᴄʟɪ ʀᴄ2"
};

char* sToolUsage = {
	Z64ARGTITLE("Aʀɢᴜᴍᴇɴᴛꜱ:")
	Z64ARGX("-i [file]", "Input:  .wav .aiff .aifc")
	Z64ARGX("-o [file]", "Output: .wav .aiff .c")
	PRNT_NL
	Z64ARGTITLE("Aᴜᴅɪᴏ Pʀᴏᴄᴇꜱꜱɪɴɢ:")
	Z64ARGX("-b [d:16]", "Target Bit Depth")
	Z64ARGX("-m",      "Mono")
	Z64ARGX("-n",      "Normalize")
	PRNT_NL
	Z64ARGTITLE("Vᴀᴅᴘᴄᴍ:")
	Z64ARGX("-v",      "Generate Vadpcm AIFC")
	Z64ARGTITLE("Aᴅᴅɪᴛɪᴏɴᴀʟ:")
	Z64ARGX("-I [d:30]", "TableDesign Refine Iteration")
	Z64ARGX("-F [d:16]", "TableDesign Frame Size")
	Z64ARGX("-B [d:2]", "TableDesign Bits")
	Z64ARGX("-O [d:2]", "TableDesign Order")
	PRNT_NL
	Z64ARGTITLE("Iɴᴛᴇʀɴᴀʟ Tᴏᴏʟꜱ:")
	Z64ARTD("TableDesign", "[iteration] [input] [output]")
	Z64ARTD("VadpcmEnc", "[input] [table] [output]")
	Z64ARTD("VadpcmDec", "[input] [output]")
	PRNT_NL
	Z64ARGTITLE("Exᴛʀᴀ:")
	Z64ARGX("-D", "Debug Print")
	Z64ARGX("-s", "Silence")
	Z64ARTD("ZZRTLMode", "DragNDrop [zzrpl] file on z64audio")
};

s32 Main(s32 argc, char* argv[]) {
	AudioSampleInfo sample;
	char* input = NULL;
	char* output = NULL;
	u32 parArg;
	
	printf_WinFix();
	printf_SetPrefix("");
	
	if (ParseArg("-i") || ParseArg("--i")) {
		input = argv[parArg];
	}
	
	if (ParseArg("-o") || ParseArg("--o")) {
		output = argv[parArg];
	}
	
	if (ParseArg("-D") || ParseArg("--D")) {
		printf_SetSuppressLevel(PSL_DEBUG);
	}
	
	if (ParseArg("-s") || ParseArg("--s")) {
		printf_SetSuppressLevel(PSL_NO_WARNING);
	}
	
	if (argc == 2 && Lib_MemMem(argv[1], strlen(argv[1]), ".zzrpl", 6)) {
		printf_toolinfo(
			sToolName,
			0
		);
		Audio_ZZRTLMode(&sample, argv[1]);
		
		return 0;
	}
	
	if (input == NULL || output == NULL) {
		printf_toolinfo(
			sToolName,
			sToolUsage
		);
		
		return 1;
	}
	
	printf_toolinfo(
		sToolName,
		0
	);
	
	Audio_InitSampleInfo(&sample, input, output);
	Audio_LoadSample(&sample);
	if (ParseArg("-b") || ParseArg("--b")) {
		sample.targetBit = String_NumStrToInt(argv[parArg]);
		
		if (sample.targetBit != 16 && sample.targetBit != 32) {
			u32 temp = sample.targetBit;
			Audio_FreeSample(&sample);
			printf_error("Bit depth [%d] is not supported.", temp);
		}
		
		Audio_Resample(&sample);
	}
	if (ParseArg("-m") || ParseArg("--m")) {
		Audio_ConvertToMono(&sample);
	}
	if (ParseArg("-n") || ParseArg("--n")) {
		Audio_Normalize(&sample);
	}
	Audio_SaveSample(&sample);
	
	if (ParseArg("-v") || ParseArg("--v")) {
		if (ParseArg("-I") || ParseArg("--I")) {
			gTableDesignIteration = argv[parArg];
		}
		if (ParseArg("-F") || ParseArg("--F")) {
			gTableDesignFrameSize = argv[parArg];
		}
		if (ParseArg("-B") || ParseArg("--B")) {
			gTableDesignBits = argv[parArg];
		}
		if (ParseArg("-O") || ParseArg("--O")) {
			gTableDesignOrder = argv[parArg];
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