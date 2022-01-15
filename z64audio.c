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

typedef enum {
	FORMPARAM_BIN,
	FORMPARAM_WAV,
	FORMPARAM_AIF,
	FORMPARAM_CCC,
} FormatParam;

#define ParseArg(xarg)         Lib_ParseArguments(argv, xarg, &parArg)
#define Z64ARGTITLE(xtitle)    "\e[96m" xtitle PRNT_RNL
#define Z64ARGX(xarg, comment) xarg "\r\033[18C" PRNT_GRAY "// " comment PRNT_RNL
#define Z64ARTD(xarg, comment) PRNT_DGRY "\e[9m" xarg PRNT_RSET "\r\033[18C" PRNT_DGRY "// " PRNT_RSET PRNT_TODO PRNT_RNL

void z64param_Generate(MemFile* param, char* file);
void z64params(char* argv[]);

char* sToolName = {
	"z64audio" PRNT_GRAY " 2.0"
};
char* sToolUsage = {
	Z64ARGTITLE("File:")
	Z64ARGX("-i [file]",     "Input:  .wav .aiff .aifc")
	Z64ARGX("-o [file]",     "Output: .wav .aiff .bin .c")
	Z64ARGX("-c",            "Compare I [file] & O [file]")
	PRNT_NL
	Z64ARGTITLE("Audio Processing:")
	Z64ARGX("-b [ 16 ]",     "Target Bit Depth")
	Z64ARGX("-m",            "Mono")
	Z64ARGX("-n",            "Normalize")
	PRNT_NL
	Z64ARGTITLE("VADPCM:")
	Z64ARGX("-p [file]",     "Use excisting predictors")
	// Z64ARTD("-v",            "Generate Vadpcm Files (Only with [.aiff] output)")
	Z64ARGX("-I [ 30 ]",     "Override TableDesign Refine Iteration")
	Z64ARGX("-F [ 16 ]",     "Override TableDesign Frame Size")
	Z64ARGX("-B [  2 ]",     "Override TableDesign Bits")
	Z64ARGX("-O [  2 ]",     "Override TableDesign Order")
	Z64ARGX("-T [ 10 ]",     "Override TableDesign Threshold")
	// PRNT_NL
	// Z64ARGTITLE("Internal Tools:")
	// Z64ARTD("TableDesign",   "[iteration] [input] [output]")
	// Z64ARTD("VadpcmEnc",     "[input] [table] [output]")
	// Z64ARTD("VadpcmDec",     "[input] [output]")
	PRNT_NL
	Z64ARGTITLE("Extra:")
	Z64ARGX("-P",            "Load separate settings [.cfg]")
	Z64ARGX("-D",            "Debug Print")
	Z64ARGX("-s",            "Silence")
	Z64ARGX("-N",            "Print Info of input [file]")
	// Z64ARTD("ZZRTLMode",     "DragNDrop [zzrpl] file on z64audio")
};
FormatParam sDefaultFormat;

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
			
			String_SwapExtension(outbuf, argv[1], format[sDefaultFormat]);
			
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

#define GenerParam(com1, name, defval, com2) MemFile_Printf( \
		param, \
		com1 \
		"\n%-15s = %-8s # %s\n\n", \
		# name, \
		# defval, \
		com2 \
)

void z64param_Generate(MemFile* param, char* file) {
	MemFile_Malloc(param, 0x1000);
	
	MemFile_Printf(
		param,
		"### z64audio settings ###\n\n"
	);
	GenerParam(
		"# nameBook.bin vs name_predictors.bin, more suitable for Jared's\n# zzrtl script",
		zaudio_zz_naming,
		false,
		"[true/false]"
	);
	GenerParam(
		"# Default format to export when drag'n'dropping files on to z64audio",
		zaudio_def_dnd_fmt,
		"bin",
		"[bin/wav/aiff/c]"
	);
	
	MemFile_Printf(
		param,
		"### TableDesign settings ###\n\n"
	);
	GenerParam(
		"# Refine Iteration",
		tbl_ref_iter,
		30,
		"[integer]"
	);
	GenerParam(
		"# Frame Size",
		tbl_frm_sz,
		16,
		"[integer]"
	);
	GenerParam(
		"# Bits",
		tbl_bits,
		2,
		"[integer]"
	);
	GenerParam(
		"# Order",
		tbl_order,
		2,
		"[integer]"
	);
	GenerParam(
		"# Threshold",
		tbl_thresh,
		10,
		"[integer]"
	);
	
	if (MemFile_SaveFile_String(param, file)) {
		printf_error("Could not create param file [%s]", file);
	}
}

void z64params(char* argv[]) {
	MemFile param;
	char file[256 * 4];
	u32 parArg;
	s32 integer;
	char* list[] = {
		"bin", "wav", "aiff", "c", NULL
	};
	
	String_Copy(file, String_GetPath(argv[0]));
	String_Merge(file, "z64audio.cfg");
	
	if (ParseArg("-P") || ParseArg("--P")) {
		MemFile_LoadFile_String(&param, argv[parArg]);
	} else if (MemFile_LoadFile_String(&param, file)) {
		printf_info("Generating settings [%s]", file);
		z64param_Generate(&param, file);
	}
	
	gBinNameIndex = Config_GetBool(&param, "zaudio_zz_naming");
	integer = Config_GetOption(&param, "zaudio_def_dnd_fmt", list);
	gTableDesignIteration = Config_GetString(&param, "tbl_ref_iter");
	gTableDesignFrameSize = Config_GetString(&param, "tbl_frm_sz");
	gTableDesignBits = Config_GetString(&param, "tbl_bits");
	gTableDesignOrder = Config_GetString(&param, "tbl_order");
	gTableDesignThreshold = Config_GetString(&param, "tbl_thresh");
	
	if (integer != 404040404) {
		sDefaultFormat = integer;
	}
	
	MemFile_Free(&param);
}