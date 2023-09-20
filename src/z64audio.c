#include <ext_lib.h>
#include "convert.h"
#include "convert_vadpcm.h"

typedef enum {
	FORMPARAM_BIN,
	FORMPARAM_WAV,
	FORMPARAM_CCC,
} FormatParam;

char* sToolName = {
	"z64audio" PRNT_GRAY " 2.2.0"
};

char* sToolUsage = {
	EXT_INFO_TITLE("File:")
	EXT_INFO("--i [file]",       16, "Input:  .wav .aiff")
	EXT_INFO("--o [file]",       16, "Output: .wav .aiff .bin .c")
	EXT_INFO("--play",           16, "'--play vadpcm' is also an option")
	PRNT_NL
	EXT_INFO_TITLE("Audio Processing:")
	EXT_INFO("--b",              16, "Target Bit Depth, '32f' for float target")
	EXT_INFO("--m",              16, "Mono")
	EXT_INFO("--n",              16, "Normalize")
	PRNT_NL
	EXT_INFO_TITLE("Arguments for [.bin] input:")
	EXT_INFO("--srate",          16, "Set Samplerate")
	EXT_INFO("--tuning",         16, "Set Tuning")
	EXT_INFO("--split-hi",       16, "Set Low Split")
	EXT_INFO("--split-lo",       16, "Set Hi Split")
	EXT_INFO("--half-precision", 22, "Saves space by halfing framesize")
	PRNT_NL
	EXT_INFO_TITLE("VADPCM:")
	EXT_INFO("--p [file]",       16, "Use excisting predictors")
	EXT_INFO("--I [ 30 ]",       16, "Override TableDesign Refine Iteration")
	EXT_INFO("--F [ 16 ]",       16, "Override TableDesign Frame Size")
	EXT_INFO("--B [  2 ]",       16, "Override TableDesign Bits")
	EXT_INFO("--O [  2 ]",       16, "Override TableDesign Order")
	EXT_INFO("--T [ 10 ]",       16, "Override TableDesign Threshold")
	PRNT_NL
	EXT_INFO_TITLE("Extra:")
	EXT_INFO("--P",              16, "Load separate settings [.cfg]")
	EXT_INFO("--log",            16, "Print Debug osLog")
	EXT_INFO("--S",              16, "Silence")
	EXT_INFO("--N",              16, "Print Info of input [file]")
};

bool gVadPrev;
bool gRomForceLoop;
FormatParam sDefaultFormat;
extern s32 gOverrideConfig;
extern bool gForgiving;

///////////////////////////////////////////////////////////////////////////////

#define GenerParam(com1, name, defval, com2) Memfile_Fmt( \
			param, \
			com1 \
			"\n%-15s = %-8s # %s\n\n", \
			# name, \
			# defval, \
			com2 \
)

static void GenerateConfig(Memfile* param, char* file) {
	Memfile_Alloc(param, 0x1000);
	
	Memfile_Fmt(
		param,
		"### z64audio settings ###\n\n"
	);
	GenerParam(
		"# Specialized for z64rom tool usage, do not set unless this is paired\n# with z64rom!",
		zaudio_z64rom_mode,
		false,
		"[true/false]"
	);
	GenerParam(
		"# nameBook.bin vs name_predictors.bin, more suitable for Jared's\n# zzrtl script.",
		zaudio_zz_naming,
		false,
		"[true/false]"
	);
	GenerParam(
		"# Default format to export when drag'n'dropping files on to z64audio.",
		zaudio_def_dnd_fmt,
		"bin",
		"[bin/wav/aiff/c]"
	);
	
	if (Memfile_SaveStr(param, file)) {
		errr("Could not create param file [%s]", file);
	}
}

static void Config(const char* argv[]) {
	Memfile param = Memfile_New();
	char file[256 * 4];
	u32 argID;
	s32 integer;
	char* list[] = {
		"bin", "wav", "aiff", "c", NULL
	};
	
	strcpy(file, sys_appdir());
	strcat(file, "z64audio.cfg");
	
	osLog("Get: %s", file);
	if ((argID = strarg(argv, "P"))) {
		Memfile_LoadStr(&param, argv[argID]);
	} else if (Memfile_LoadStr(&param, file)) {
		info("Generating settings [%s]", file);
		GenerateConfig(&param, file);
	}
	
	gBinNameIndex = Ini_GetBool(&param, "zaudio_zz_naming");
	integer = Ini_GetOpt(&param, "zaudio_def_dnd_fmt", list);
	
	if (integer != 404040404) {
		sDefaultFormat = integer;
	}
	
	Memfile_Free(&param);
	
	if ((argID = strarg(argv, "GenCfg"))) {
		exit(65);
	}
}

static void LoadSampleConfig(const char* conf) {
	Memfile mem = Memfile_New();
	char* param;
	
	if (conf == NULL || !sys_stat(conf))
		return;
	
	Memfile_LoadStr(&mem, conf);
	
	if ((param = Ini_GetVar(mem.str, "book_iteration"))) gTableDesignIteration = param;
	if ((param = Ini_GetVar(mem.str, "book_frame_size"))) gTableDesignFrameSize = param;
	if ((param = Ini_GetVar(mem.str, "book_bits"))) gTableDesignBits = param;
	if ((param = Ini_GetVar(mem.str, "book_order"))) gTableDesignOrder = param;
	if ((param = Ini_GetVar(mem.str, "book_threshold"))) gTableDesignThreshold = param;
	
	if ((param = Ini_GetVar(mem.str, "sample_rate"))) gBinSampleRate = sint(param);
	if ((param = Ini_GetVar(mem.str, "sample_tuning"))) gTuning = sfloat(param);
	
	Memfile_Free(&mem);
}

///////////////////////////////////////////////////////////////////////////////

s32 main(s32 argc, const char* argv[]) {
	AudioSample sample;
	char* input = NULL;
	char* output = NULL;
	u32 argID;
	
	Config(argv);
	if ((argID = strarg(argv, "S"))) IO_SetLevel(PSL_NO_WARNING);
	if ((argID = strarg(argv, "i"))) input = String_GetSpacedArg(argv, argID);
	if ((argID = strarg(argv, "o"))) output = String_GetSpacedArg(argv, argID);
	
	if (argc == 2 /* DragNDrop */) {
		static char outbuf[256 * 2];
		
		IO_SetLevel(PSL_NO_WARNING);
		
		if (striend(argv[1], ".wav") || striend(argv[1], ".aiff") || striend(argv[1], ".mp3")) {
			char* format[] = {
				".bin",
				".wav",
				".aiff",
				".c"
			};
			
			strswapext(outbuf, argv[1], format[sDefaultFormat]);
			osLog("Input: %s", argv[1]);
			osLog("Output: %s", argv[1]);
			
			input = qxf(strdup(argv[1]));
			output = outbuf;
		}
	}
	
	if (input == NULL) {
		info_title(
			sToolName,
			sToolUsage
		);
		
		return 1;
	}
	
	info_title(sToolName, "\n");
	Audio_InitSample(&sample, input, output);
	
	if (strarg(argv, "config-override")) gOverrideConfig = true;
	
	if ((argID = strarg(argv, "design"))) {
		LoadSampleConfig(argv[argID]);
	} else if ((argID = strarg(argv, "book")) && sample.useExistingPred == 0) {
		Audio_VadpcmBookLoad(&sample, argv[argID]);
		sample.useExistingPred = true;
	} else {
		if ((argID = strarg(argv, "table-iter"))) gTableDesignIteration = qxf(strdup(argv[argID]));
		if ((argID = strarg(argv, "table-frame"))) gTableDesignFrameSize = qxf(strdup(argv[argID]));
		if ((argID = strarg(argv, "table-bits"))) gTableDesignBits = qxf(strdup(argv[argID]));
		if ((argID = strarg(argv, "table-order"))) gTableDesignOrder = qxf(strdup(argv[argID]));
		if ((argID = strarg(argv, "table-threshold"))) gTableDesignThreshold = qxf(strdup(argv[argID]));
	}
	
	if ((argID = strarg(argv, "srate"))) gBinSampleRate = sint(argv[argID]);
	if ((argID = strarg(argv, "tuning"))) gTuning = sfloat(argv[argID]);
	if ((argID = strarg(argv, "slack"))) gForgiving = true;
	
	if ((argID = strarg(argv, "raw-channel"))) gRaw.channelNum = sint(argv[argID]);
	if ((argID = strarg(argv, "raw-samplerate"))) gRaw.sampleRate = sint(argv[argID]);
	if ((argID = strarg(argv, "raw-bit"))) {
		gRaw.bit = sint(argv[argID]);
		if (stristr(argv[argID], "f"))
			gRaw.dataIsFloat = true;
	}
	
	Audio_LoadSample(&sample);
	
	if ((argID = strarg(argv, "basenote"))) sample.instrument.note = sint(argv[argID]);
	if ((argID = strarg(argv, "finetune"))) sample.instrument.fineTune = sint(argv[argID]);
	
	if ((argID = strarg(argv, "split-hi"))) sample.instrument.highNote = sint(argv[argID]);
	if ((argID = strarg(argv, "split-lo"))) sample.instrument.lowNote = sint(argv[argID]);
	if ((argID = strarg(argv, "half-precision"))) gPrecisionFlag = 3;
	
	if (output == NULL) errr("No output specified!");
	if ((argID = strarg(argv, "b"))) {
		if (strstr(argv[argID], "32"))
			sample.targetBit = 32;
		if (strstr(argv[argID], "16"))
			sample.targetBit = 16;
		
		sample.targetIsFloat = false;
		if (sample.targetBit) {
			if (strstr(argv[argID], "f"))
				sample.targetIsFloat = true;
		}
	}
	if ((argID = strarg(argv, "N"))) {
		info_align("BitDepth", "%10d", sample.bit);
		info_align("Sample Rate", "%10d", sample.sampleRate);
		info_align("Channels", "%10d", sample.channelNum);
		info_align("Frames", "%10d", sample.samplesNum);
		info_align("Data Size", "%10d", sample.size);
		info_align("File Size", "%10d", sample.memFile.size);
		info_align("Loop Start", "%10d", sample.instrument.loop.start);
		info_align("Loop End", "%10d", sample.instrument.loop.end);
		
		if (output == NULL)
			return 0;
	}
	
	Audio_BitDepth(&sample);
	if ((argID = strarg(argv, "m"))) Audio_Mono(&sample);
	if ((argID = strarg(argv, "n"))) Audio_Normalize(&sample);
	
	Audio_SaveSample(&sample);
	
	Audio_FreeSample(&sample);
	
	return 0;
}
