#include "lib/AudioConvert.h"
#include "lib/AudioTools.h"
#include "lib/AudioGui.h"

#define INCBIN_PREFIX
#include <incbin.h>

INCBIN(gFont_CascadiaCode, "assets/CascadiaCode-SemiBold.ttf");
INCBIN(gFont_NotoSand, "assets/NotoSans-Bold.ttf");

INCBIN(gCursor_ArrowU, "assets/arrow_up.ia16");
INCBIN(gCursor_ArrowL, "assets/arrow_left.ia16");
INCBIN(gCursor_ArrowD, "assets/arrow_down.ia16");
INCBIN(gCursor_ArrowR, "assets/arrow_right.ia16");
INCBIN(gCursor_ArrowH, "assets/arrow_horizontal.ia16");
INCBIN(gCursor_ArrowV, "assets/arrow_vertical.ia16");
INCBIN(gCursor_Crosshair, "assets/crosshair.ia16");
INCBIN(gCursor_Empty, "assets/empty.ia16");

typedef enum {
	FORMPARAM_BIN,
	FORMPARAM_WAV,
	FORMPARAM_CCC,
} FormatParam;

char* sToolName = {
	"z64audio" PRNT_GRAY " 2.1.0"
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

bool gVadPrev;
bool gRomForceLoop;
FormatParam sDefaultFormat;

// # # # # # # # # # # # # # # # # # # # #
// # Setup                               #
// # # # # # # # # # # # # # # # # # # # #

#define GenerParam(com1, name, defval, com2) MemFile_Printf( \
		param, \
		com1 \
		"\n%-15s = %-8s # %s\n\n", \
		# name, \
		# defval, \
		com2 \
)

void Main_Config_Generate(MemFile* param, char* file) {
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
	
	if (MemFile_SaveFile_String(param, file)) {
		printf_error("Could not create param file [%s]", file);
	}
}

void Main_Config(char* argv[]) {
	MemFile param = MemFile_Initialize();
	char file[256 * 4];
	u32 parArg;
	s32 integer;
	char* list[] = {
		"bin", "wav", "aiff", "c", NULL
	};
	
	strcpy(file, Sys_AppDir());
	strcat(file, "z64audio.cfg");
	
	Log("Get: %s", file);
	if (ParseArg("P")) {
		MemFile_LoadFile_String(&param, argv[parArg]);
	} else if (MemFile_LoadFile_String(&param, file)) {
		printf_info("Generating settings [%s]", file);
		Main_Config_Generate(&param, file);
	}
	
	gBinNameIndex = Config_GetBool(&param, "zaudio_zz_naming");
	integer = Config_GetOption(&param, "zaudio_def_dnd_fmt", list);
	
	if (integer != 404040404) {
		sDefaultFormat = integer;
	}
	
	MemFile_Free(&param);
	
	if (ParseArg("GenCfg")) {
		exit(65);
	}
}

void Main_LoadSampleConf(char* conf) {
	MemFile mem = MemFile_Initialize();
	char* param;
	
	if (conf == NULL || !Sys_Stat(conf))
		return;
	
	MemFile_LoadFile_String(&mem, conf);
	
	if ((param = Config_GetVariable(mem.str, "book_iteration"))) gTableDesignIteration = param;
	if ((param = Config_GetVariable(mem.str, "book_frame_size"))) gTableDesignFrameSize = param;
	if ((param = Config_GetVariable(mem.str, "book_bits"))) gTableDesignBits = param;
	if ((param = Config_GetVariable(mem.str, "book_order"))) gTableDesignOrder = param;
	if ((param = Config_GetVariable(mem.str, "book_threshold"))) gTableDesignThreshold = param;
	
	if ((param = Config_GetVariable(mem.str, "sample_rate"))) gSampleRate = String_GetInt(param);
	if ((param = Config_GetVariable(mem.str, "sample_tuning"))) gTuning = String_GetFloat(param);
	
	MemFile_Free(&mem);
}

// # # # # # # # # # # # # # # # # # # # #
// # MAIN                                #
// # # # # # # # # # # # # # # # # # # # #

s32 Main(s32 argc, char* argv[]) {
	AudioSample sample;
	char* input = NULL;
	char* output = NULL;
	u32 parArg;
	u32 callSignal = 0;
	
	Log_Init();
	printf_WinFix();
	printf_SetPrefix("");
	
	if (argc == 1) {
		WindowContext* winCtx = Calloc(0, sizeof(WindowContext));
		
		printf_SetSuppressLevel(PSL_DEBUG);
		winCtx->vg = Interface_Init("z64audio", &winCtx->app, &winCtx->input, winCtx, (void*)Window_Update, (void*)Window_Draw, Window_DropCallback, 980, 480, 2);
		
		winCtx->geoGrid.passArg = winCtx;
		winCtx->geoGrid.taskTable = gTaskTable;
		winCtx->geoGrid.taskTableNum = ArrayCount(gTaskTable);
		
		nvgCreateFontMem(winCtx->vg, "font-basic", (void*)gFont_CascadiaCodeData, gFont_CascadiaCodeSize, 0);
		nvgCreateFontMem(winCtx->vg, "font-bold", (void*)gFont_NotoSandData, gFont_NotoSandSize, 0);
		
		Theme_Init(0);
		GeoGrid_Init(&winCtx->geoGrid, &winCtx->app.winDim, &winCtx->input, winCtx->vg);
		Cursor_Init(&winCtx->cursor);
		Cursor_CreateCursor(&winCtx->cursor, CURSOR_ARROW_U, gCursor_ArrowUData, 24, 12, 12);
		Cursor_CreateCursor(&winCtx->cursor, CURSOR_ARROW_D, gCursor_ArrowDData, 24, 12, 12);
		Cursor_CreateCursor(&winCtx->cursor, CURSOR_ARROW_L, gCursor_ArrowLData, 24, 12, 12);
		Cursor_CreateCursor(&winCtx->cursor, CURSOR_ARROW_R, gCursor_ArrowRData, 24, 12, 12);
		Cursor_CreateCursor(&winCtx->cursor, CURSOR_ARROW_H, gCursor_ArrowHData, 32, 16, 16);
		Cursor_CreateCursor(&winCtx->cursor, CURSOR_ARROW_V, gCursor_ArrowVData, 32, 16, 16);
		Cursor_CreateCursor(&winCtx->cursor, CURSOR_CROSSHAIR, gCursor_CrosshairData, 40, 19, 20);
		Cursor_CreateCursor(&winCtx->cursor, CURSOR_EMPTY, gCursor_EmptyData, 16, 0, 0);
		
		Rectf32 size = {
			winCtx->geoGrid.workRect.x,
			winCtx->geoGrid.workRect.y,
			winCtx->geoGrid.workRect.w,
			winCtx->geoGrid.workRect.h
		};
		
		GeoGrid_AddSplit(&winCtx->geoGrid, &size)->id = 1;
		
		ThreadLock_Init();
		Interface_Main();
		
		ThreadLock_Free();
		glfwTerminate();
		
		return 0;
	}
	
	Main_Config(argv);
	if (ParseArg("log")) callSignal = true;
	if (ParseArg("S")) printf_SetSuppressLevel(PSL_NO_WARNING);
	if (ParseArg("i")) input = String_GetSpacedArg(argv, parArg);
	if (ParseArg("o")) output = String_GetSpacedArg(argv, parArg);
	
	if (argc == 2 /* DragNDrop */) {
		static char outbuf[256 * 2];
		
		printf_SetSuppressLevel(PSL_NO_WARNING);
		
		if (StrEndCase(argv[1], ".wav") || StrEndCase(argv[1], ".aiff") || StrEndCase(argv[1], ".mp3")) {
			char* format[] = {
				".bin",
				".wav",
				".aiff",
				".c"
			};
			
			String_SwapExtension(outbuf, argv[1], format[sDefaultFormat]);
			Log("Input: %s", argv[1]);
			Log("Output: %s", argv[1]);
			
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
	
	if (ParseArg("design")) {
		Main_LoadSampleConf(argv[parArg]);
	} else if (ParseArg("book") && sample.useExistingPred == 0) {
		AudioTools_LoadCodeBook(&sample, argv[parArg]);
		sample.useExistingPred = true;
	} else {
		if (ParseArg("table-iter")) gTableDesignIteration = argv[parArg];
		if (ParseArg("table-frame")) gTableDesignFrameSize = argv[parArg];
		if (ParseArg("table-bits")) gTableDesignBits = argv[parArg];
		if (ParseArg("table-order")) gTableDesignOrder = argv[parArg];
		if (ParseArg("table-threshold")) gTableDesignThreshold = argv[parArg];
	}
	
	if (ParseArg("srate")) gSampleRate = String_GetInt(argv[parArg]);
	if (ParseArg("tuning")) gTuning = String_GetFloat(argv[parArg]);
	
	Audio_LoadSample(&sample);
	
	if (ParseArg("z64rom")) {
		if (sample.instrument.note == 60 &&
			sample.instrument.fineTune == 0) {
			goto noteinfo;
		}
	} else {
noteinfo:
		if (ParseArg("basenote")) sample.instrument.note = String_GetInt(argv[parArg]);
		if (ParseArg("finetune")) sample.instrument.fineTune = String_GetInt(argv[parArg]);
	}
	
	if (ParseArg("split-hi")) sample.instrument.highNote = String_GetInt(argv[parArg]);
	if (ParseArg("split-lo")) sample.instrument.lowNote = String_GetInt(argv[parArg]);
	if (ParseArg("half-precision")) gPrecisionFlag = 3;
	
	if (output == NULL) printf_error("No output specified!");
	if (ParseArg("b")) {
		if (StrStr(argv[parArg], "32"))
			sample.targetBit = 32;
		if (StrStr(argv[parArg], "16"))
			sample.targetBit = 16;
		
		sample.targetIsFloat = false;
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
	
	if (callSignal) Log_Print();
	Log_Free();
	
	return 0;
}