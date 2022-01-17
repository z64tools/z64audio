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

void z64param_Generate(MemFile* param, char* file);
void z64params(char* argv[]);

char* sToolName = {
	"z64audio" PRNT_GRAY " 2.0"
};
char* sToolUsage = {
	EXT_INFO_TITLE("File:")
	EXT_INFO("--i [file]", 16, "Input:  .wav .aiff .aifc")
	EXT_INFO("--o [file]", 16, "Output: .wav .aiff .bin .c")
	EXT_INFO("--c", 16, "Compare I [file] & O [file]")
	PRNT_NL
	EXT_INFO_TITLE("Audio Processing:")
	EXT_INFO("--b [ 16 ]", 16, "Target Bit Depth")
	EXT_INFO("--m", 16, "Mono")
	EXT_INFO("--n", 16, "Normalize")
	PRNT_NL
	EXT_INFO_TITLE("Arguments for [.bin] input:")
	EXT_INFO("--srate", 16, "Set Samplerate")
	EXT_INFO("--tuning", 16, "Set Tuning")
	EXT_INFO("--split-hi", 16, "Set Low Split")
	EXT_INFO("--split-lo", 16, "Set Hi Split")
	EXT_INFO("--half-precision", 22, "Saves space by halfing framesize")
	PRNT_NL
	EXT_INFO_TITLE("VADPCM:")
	EXT_INFO("--p [file]", 16, "Use excisting predictors")
	EXT_INFO("--I [ 30 ]", 16, "Override TableDesign Refine Iteration")
	EXT_INFO("--F [ 16 ]", 16, "Override TableDesign Frame Size")
	EXT_INFO("--B [  2 ]", 16, "Override TableDesign Bits")
	EXT_INFO("--O [  2 ]", 16, "Override TableDesign Order")
	EXT_INFO("--T [ 10 ]", 16, "Override TableDesign Threshold")
	PRNT_NL
	EXT_INFO_TITLE("Extra:")
	EXT_INFO("--P", 16, "Load separate settings [.cfg]")
	EXT_INFO("--D", 16, "Debug Print")
	EXT_INFO("--S", 16, "Silence")
	EXT_INFO("--N", 16, "Print Info of input [file]")
	// EXT_TDO("ZZRTLMode",     "DragNDrop [zzrpl] file on z64audio")
};
FormatParam sDefaultFormat;

s32 Main(s32 argc, char* argv[]) {
	AudioSampleInfo sample;
	char* input = NULL;
	char* output = NULL;
	u32 parArg;
	
	printf_WinFix();
	printf_SetPrefix("");
	
	if (ParseArg("--D")) {
		printf_SetSuppressLevel(PSL_DEBUG);
	}
	
	if (ParseArg("--S")) {
		printf_SetSuppressLevel(PSL_NO_WARNING);
	}
	
	z64params(argv);
	
	if (ParseArg("--i")) {
		input = String_GetSpacedArg(argv, parArg);
	}
	
	if (ParseArg("--o")) {
		output = String_GetSpacedArg(argv, parArg);
	}
	
	if (ParseArg("--c")) {
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
	
	printf_toolinfo(sToolName, "");
	Audio_InitSampleInfo(&sample, input, output);
	
	if (ParseArg("--p")) {
		AudioTools_LoadCodeBook(&sample, argv[parArg]);
		sample.useExistingPred = 1;
	} else {
		if (ParseArg("--I")) gTableDesignIteration = argv[parArg];
		if (ParseArg("--F")) gTableDesignFrameSize = argv[parArg];
		if (ParseArg("--B")) gTableDesignBits = argv[parArg];
		if (ParseArg("--O")) gTableDesignOrder = argv[parArg];
		if (ParseArg("--T")) gTableDesignThreshold = argv[parArg];
	}
	
	if (ParseArg("--srate")) gSampleRate = String_GetInt(argv[parArg]);
	if (ParseArg("--tuning")) gTuning = String_GetFloat(argv[parArg]);
	
	printf_debugExt("Audio_LoadSample");
	Audio_LoadSample(&sample);
	
	if (ParseArg("--split-hi")) sample.instrument.highNote = String_GetInt(argv[parArg]);
	if (ParseArg("--split-lo")) sample.instrument.lowNote = String_GetInt(argv[parArg]);
	if (ParseArg("--half-precision")) gPrecisionFlag = 3;
	
	if (ParseArg("--N")) {
		printf_info_align("BitDepth", "%10d", sample.bit);
		printf_info_align("Sample Rate", "%10d", sample.sampleRate);
		printf_info_align("Channels", "%10d", sample.channelNum);
		printf_info_align("Frames", "%10d", sample.samplesNum);
		printf_info_align("Data Size", "%10d", sample.size);
		printf_info_align("File Size", "%10d", sample.memFile.dataSize);
		printf_info_align("Loop Start", "%10d", sample.instrument.loop.start);
		printf_info_align("Loop End", "%10d", sample.instrument.loop.end);
		
		if (output == NULL)
			return 0;
	}
	
	if (output == NULL) {
		printf_error("No output specified!");
	}
	
	if (ParseArg("--b")) {
		sample.targetBit = String_GetInt(argv[parArg]);
		
		if (sample.targetBit != 16 && sample.targetBit != 32) {
			u32 temp = sample.targetBit;
			Audio_FreeSample(&sample);
			printf_error("Bit depth [%d] is not supported.", temp);
		}
		
		Audio_Resample(&sample);
	}
	
	if (ParseArg("--m")) {
		Audio_ConvertToMono(&sample);
	}
	
	if (ParseArg("--n")) {
		Audio_Normalize(&sample);
	}
	
	if (ParseArg("--v")) {
		Audio_Resample(&sample);
		Audio_ConvertToMono(&sample);
	}
	
	Audio_SaveSample(&sample);
	
	if (ParseArg("--v")) {
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
	MemFile param = MemFile_Initialize();
	char file[256 * 4];
	u32 parArg;
	s32 integer;
	char* list[] = {
		"bin", "wav", "aiff", "c", NULL
	};
	
	String_Copy(file, String_GetPath(argv[0]));
	String_Merge(file, "z64audio.cfg");
	
	if (ParseArg("--P")) {
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