#include "lib/AudioConvert.h"
#include "lib/AudioTools.h"

typedef enum {
	FORMPARAM_BIN,
	FORMPARAM_WAV,
	FORMPARAM_CCC,
} FormatParam;

void z64param_Generate(MemFile* param, char* file);
void z64params(char* argv[]);

char* sToolName = {
	"z64audio" PRNT_GRAY " 2.0.2"
};

char* sToolUsage = {
	EXT_INFO_TITLE("File:")
	EXT_INFO("--i [file]", 16, "Input:  .wav .aiff")
	EXT_INFO("--o [file]", 16, "Output: .wav .aiff .bin .c")
	EXT_INFO("--play",     16, "'--play vadpcm' is also an option")
	PRNT_NL
	EXT_INFO_TITLE("Audio Processing:")
	EXT_INFO("--b",        16, "Target Bit Depth, '32f' for float target")
	EXT_INFO("--m",        16, "Mono")
	EXT_INFO("--n",        16, "Normalize")
	PRNT_NL
	EXT_INFO_TITLE("Arguments for [.bin] input:")
	EXT_INFO("--srate",    16, "Set Samplerate")
	EXT_INFO("--tuning",   16, "Set Tuning")
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
	EXT_INFO("--P",        16, "Load separate settings [.cfg]")
	EXT_INFO("--log",      16, "Print Debug Log")
	EXT_INFO("--S",        16, "Silence")
	EXT_INFO("--N",        16, "Print Info of input [file]")
};

DirCtx gDir;

bool gVadPrev;

FormatParam sDefaultFormat;

s32 Main(s32 argc, char* argv[]) {
	AudioSampleInfo sample;
	char* input = NULL;
	char* output = NULL;
	u32 parArg;
	u32 callSignal = 0;
	
	Log_Init();
	printf_WinFix();
	printf_SetPrefix("");
	
	z64params(argv);
	if (ParseArg("log")) callSignal = true;
	if (ParseArg("S")) printf_SetSuppressLevel(PSL_NO_WARNING);
	if (ParseArg("i")) input = String_GetSpacedArg(argv, parArg);
	if (ParseArg("o")) output = String_GetSpacedArg(argv, parArg);
	
	if (argc == 2 /* DragNDrop */) {
		static char outbuf[256 * 2];
		
		printf_SetSuppressLevel(PSL_NO_WARNING);
		
		if (StrStrCase(argv[1], ".wav") || StrStrCase(argv[1], ".aiff")) {
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
	
	printf_toolinfo(sToolName, "\n");
	Audio_InitSample(&sample, input, output);
	
	if (gRomMode) {
		Dir_Set(&gDir, String_GetPath(sample.input));
		if (Sys_Stat(Dir_File(&gDir, "*.book.bin"))) {
			MemFile config = MemFile_Initialize();
			
			MemFile_LoadFile_String(&config, Dir_File(&gDir, "config.cfg"));
			gPrecisionFlag = Config_GetInt(&config, "codec") ? 3 : 0;
			
			AudioTools_LoadCodeBook(&sample, Dir_File(&gDir, "sample.book.bin"));
			sample.useExistingPred = true;
			MemFile_Free(&config);
		}
	} else {
		if (ParseArg("p") && sample.useExistingPred == 0) {
			AudioTools_LoadCodeBook(&sample, argv[parArg]);
			sample.useExistingPred = true;
		} else {
			if (ParseArg("I")) gTableDesignIteration = argv[parArg];
			if (ParseArg("F")) gTableDesignFrameSize = argv[parArg];
			if (ParseArg("B")) gTableDesignBits = argv[parArg];
			if (ParseArg("O")) gTableDesignOrder = argv[parArg];
			if (ParseArg("T")) gTableDesignThreshold = argv[parArg];
		}
	}
	
	if (ParseArg("srate")) gSampleRate = String_GetInt(argv[parArg]);
	if (ParseArg("tuning")) gTuning = String_GetFloat(argv[parArg]);
	
	Audio_LoadSample(&sample);
	
	if (ParseArg("split-hi")) sample.instrument.highNote = String_GetInt(argv[parArg]);
	if (ParseArg("split-lo")) sample.instrument.lowNote = String_GetInt(argv[parArg]);
	if (ParseArg("half-precision")) gPrecisionFlag = 3;
	
	if (ParseArg("play")) {
		if (argv[parArg] && !strcmp(argv[parArg], "vadpcm")) {
			printf_info_align("Play:", "vadpcm preview");
			gVadPrev = true;
		}
		Audio_PlaySample(&sample);
		if (output == NULL)
			goto free;
	}
	
	if (output == NULL) printf_error("No output specified!");
	if (ParseArg("b")) {
		if (StrStr(argv[parArg], "32"))
			sample.targetBit = 32;
		if (StrStr(argv[parArg], "16"))
			sample.targetBit = 16;
		
		if (sample.targetBit) {
			if (StrStr(argv[parArg], "f"))
				sample.targetIsFloat = true;
		}
	}
	if (ParseArg("N")) {
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
	
	Audio_BitDepth(&sample);
	if (ParseArg("m")) Audio_Mono(&sample);
	if (ParseArg("n")) Audio_Normalize(&sample);
	
	Audio_SaveSample(&sample);
	
	Audio_FreeSample(&sample);
	
free:
	if (callSignal) Log_Print();
	Log_Free();
	
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
	
	strcpy(file, Sys_AppDir());
	strcat(file, "z64audio.cfg");
	
	if (ParseArg("P")) {
		MemFile_LoadFile_String(&param, argv[parArg]);
	} else if (MemFile_LoadFile_String(&param, file)) {
		printf_info("Generating settings [%s]", file);
		z64param_Generate(&param, file);
	}
	
	gRomMode = Config_GetBool(&param, "zaudio_z64rom_mode");
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
	
	if (ParseArg("GenCfg")) {
		exit(65);
	}
}