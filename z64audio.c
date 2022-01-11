#include "lib/ExtLib.h"
#include "lib/AudioConvert.h"
#include "lib/AudioTools.h"

/* TODO:
 * Fix AudioTools_TableDesign, seems to output last pred row with slight difference
 * Implement Bicubic Resampling
 * Support OGG file
 * Support MP3 (maybe)
 * Basic Envelope Settings for C output
 * ZZRTLMode
 * Call TableDesign, VadpcmEnc or VadpcmDec
 * Instrument Designer (GUI) for Env Editing with preview playback
 */

#define ParseArg(xarg)         Lib_ParseArguments(argv, xarg, &parArg)
#define Z64ARGTITLE(xtitle)    "\e[96m" xtitle PRNT_RNL
#define Z64ARGX(xarg, comment) xarg "\r\033[18C" PRNT_GRAY "// " comment PRNT_RNL
#define Z64ARTD(xarg, comment) PRNT_DGRY "\e[9m" xarg PRNT_RSET "\r\033[18C" PRNT_DGRY "// " PRNT_RSET PRNT_TODO PRNT_RNL

char* sToolName = {
	"z64audio" PRNT_GRAY " 2.0"
};

char* sToolUsage = {
	Z64ARGTITLE("File:")
	Z64ARGX("-i [file]",     "Input:  .wav .aiff .aifc")
	Z64ARGX("-o [file]",     "Output: .wav .aiff .c")
	Z64ARGX("-c",            "Compare I [file] & O [file]")
	PRNT_NL
	Z64ARGTITLE("Audio Processing:")
	Z64ARGX("-b [ 16 ]",     "Target Bit Depth")
	Z64ARGX("-m",            "Mono")
	Z64ARGX("-n",            "Normalize")
	PRNT_NL
	Z64ARGTITLE("VADPCM:")
	Z64ARGX("-p",            "Use excisting predictors")
	// Z64ARTD("-v",            "Generate Vadpcm Files (Only with [.aiff] output)")
	Z64ARGX("-I [ 30 ]",     "TableDesign Refine Iteration")
	Z64ARGX("-F [ 16 ]",     "TableDesign Frame Size")
	Z64ARGX("-B [  2 ]",     "TableDesign Bits")
	Z64ARGX("-O [  2 ]",     "TableDesign Order")
	Z64ARGX("-T [ 10 ]",     "TableDesign Threshold")
	// PRNT_NL
	// Z64ARGTITLE("Internal Tools:")
	// Z64ARTD("TableDesign",   "[iteration] [input] [output]")
	// Z64ARTD("VadpcmEnc",     "[input] [table] [output]")
	// Z64ARTD("VadpcmDec",     "[input] [output]")
	PRNT_NL
	Z64ARGTITLE("Extra:")
	Z64ARGX("-D",            "Debug Print")
	Z64ARGX("-s",            "Silence")
	Z64ARGX("-N",            "Print Info of input [file]")
	// Z64ARTD("ZZRTLMode",     "DragNDrop [zzrpl] file on z64audio")
};

u32 gDefaultFormat;

void z64params(char* argv[]) {
	MemFile param;
	char file[256 * 4];
	char* ptr;
	
	String_Copy(file, String_GetPath(argv[0]));
	String_Merge(file, "z64audio.cfg");
	
	if (MemFile_LoadFile_String(&param, file)) {
		printf_info("Generating settings [%s]", file);
		MemFile_Malloc(&param, 0x1000);
		MemFile_Printf(
			&param,
			"# z64audio settings\n\n"
		);
		
		MemFile_Printf(
			&param,
			"# nameBook.bin vs name_predictors.bin, more suitable for Jared's\n"
			"# zzrtl script\n"
			"%-15s = %-8s # %s\n\n",
			/* param_name */ "zz_naming",
			/* defaultval */ "false",
			/* options    */ "[true/false]"
		);
		
		MemFile_Printf(
			&param,
			"# Default format to export when drag'n'dropping files on to z64audio\n"
			"%-15s = %-8s # %s\n\n",
			/* param_name */ "def_dnd_fmt",
			/* defaultval */ "\"bin\"",
			/* options    */ "[bin/wav/aiff/c]"
		);
		
		if (MemFile_SaveFile_String(&param, file)) {
			printf_error("Could not create param file [%s]", file);
		}
		
		MemFile_Free(&param);
		
		return;
	}
	
	gBinNameIndex = Config_GetBool(&param, "zz_naming");
	ptr = Config_GetString(&param, "def_dnd_fmt");
	
	if (ptr) {
		if (String_MemMem(ptr, "\"bin\"")) {
			gDefaultFormat = FORMPARAM_BIN;
		} else if (String_MemMem(ptr, "\"wav\"")) {
			gDefaultFormat = FORMPARAM_WAV;
		} else if (String_MemMem(ptr, "\"aiff\"")) {
			gDefaultFormat = FORMPARAM_AIF;
		} else if (String_MemMem(ptr, "\"c\"")) {
			gDefaultFormat = FORMPARAM_CCC;
		} else {
			printf_warning("[%s]", ptr);
			printf_error("Oh no... [%s]:[%s]:[%d]", __FILE__, __FUNCTION__, __LINE__);
		}
	}
		
	MemFile_Free(&param);
}

s32 Main(s32 argc, char* argv[]) {
	AudioSampleInfo sample;
	char* input = NULL;
	char* output = NULL;
	u32 parArg;
	
	printf_WinFix();
	printf_SetPrefix("");
	z64params(argv);
	
	if (ParseArg("-D") || ParseArg("--D")) {
		printf_SetSuppressLevel(PSL_DEBUG);
	}
	
	if (ParseArg("-s") || ParseArg("--s")) {
		printf_SetSuppressLevel(PSL_NO_WARNING);
	}
	
	if (ParseArg("-i") || ParseArg("--i")) {
		input = String_GetSpacedArg(argv, parArg);
	}
	
	if (ParseArg("-o") || ParseArg("--o")) {
		output = String_GetSpacedArg(argv, parArg);
	}
	
	if (ParseArg("-c") || ParseArg("--c")) {
		AudioSampleInfo sampleComp;
		
		printf_toolinfo(
			sToolName,
			0
		);
		
		Audio_InitSampleInfo(&sample, input, "none");
		Audio_InitSampleInfo(&sampleComp, output, "none");
		
		Audio_LoadSample(&sample);
		Audio_LoadSample(&sampleComp);
		
		Audio_Compare(&sample, &sampleComp);
		
		Audio_FreeSample(&sample);
		Audio_FreeSample(&sampleComp);
		
		return 0;
	}
	
	if (argc == 2 /* DragNDrop */) {
		static char outbuf[256 * 2];
		
		if (String_MemMem(argv[1], ".zzrpl")) {
			printf_toolinfo(
				sToolName,
				0
			);
			Audio_ZZRTLMode(&sample, argv[1]);
			
			return 0;
		}
		if (String_MemMem(argv[1], ".wav") || String_MemMem(argv[1], ".aiff")) {
			char* format[] = {
				".bin",
				".wav",
				".aiff",
				".c"
			};
			
			String_SwapExtension(outbuf, argv[1], format[gDefaultFormat]);
			
			input = argv[1];
			output = outbuf;
		}
	}
	
	if (input == NULL) {
		printf_toolinfo(
			sToolName,
			sToolUsage
		);
		
		return 1;
	}
	
	printf_toolinfo(
		sToolName,
		""
	);
	
	OsPrintfEx("Audio_InitSampleInfo");
	Audio_InitSampleInfo(&sample, input, output);
	
	if (ParseArg("-p") || ParseArg("--p")) {
		AudioTools_LoadCodeBook(&sample, argv[parArg]);
		sample.useExistingPred = 1;
	} else {
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
		if (ParseArg("-T") || ParseArg("--T")) {
			gTableDesignThreshold = argv[parArg];
		}
	}
	
	OsPrintfEx("Audio_LoadSample");
	Audio_LoadSample(&sample);
	
	if (ParseArg("-N") || ParseArg("--N")) {
		printf_align("BitDepth", "%10d", sample.bit);
		printf_align("Sample Rate", "%10d", sample.sampleRate);
		printf_align("Channels", "%10d", sample.channelNum);
		printf_align("Frames", "%10d", sample.samplesNum);
		printf_align("Data Size", "%10d", sample.size);
		printf_align("File Size", "%10d", sample.memFile.dataSize);
		
		if (output == NULL)
			return 0;
	} else {
		printf_debugExt("BitDepth   [ %8d ] " PRNT_GRAY "[%s]", sample.bit, sample.input);
		printf_debug("SampleRate [ %8d ] " PRNT_GRAY "[%s]", sample.sampleRate, sample.input);
		printf_debug("Channels   [ %8d ] " PRNT_GRAY "[%s]", sample.channelNum, sample.input);
		printf_debug("Frames     [ %8d ] " PRNT_GRAY "[%s]", sample.samplesNum, sample.input);
	}
	
	if (output == NULL) {
		printf_error("No output specified!");
	}
	
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
	
	if (ParseArg("-v") || ParseArg("--v")) {
		Audio_Resample(&sample);
		Audio_ConvertToMono(&sample);
	}
	
	Audio_SaveSample(&sample);
	
	if (ParseArg("-v") || ParseArg("--v")) {
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