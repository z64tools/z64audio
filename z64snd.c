#define __FLOAT80_SUCKS__

#include "include/z64snd.h"

#ifdef _WIN32
 #include <windows.h>
#endif

static void showModes(void);
static void assertMode(z64audioMode mode);
static z64audioMode requestMode(void);
static void showArgs(void);
static void shiftArgs(int* argc, char* argv[], int i);
void z64audioAtExit(void);

static int gWaitAtExit = 0;

static inline void win32icon(void)
{
#ifdef _WIN32
#include "icon.h"
{
	HWND win = GetActiveWindow();
	if( win )
	{
		SendMessage(
			win
			, WM_SETICON
			, ICON_BIG
			, (LPARAM)LoadImage(
					GetModuleHandle(NULL)
					, MAKEINTRESOURCE(IDI_ICON)
					, IMAGE_ICON
					, 32//GetSystemMetrics(SM_CXSMICON)
					, 32//GetSystemMetrics(SM_CXSMICON)
					, 0
				)
		);
		SendMessage(
			win
			, WM_SETICON
			, ICON_SMALL
			, (LPARAM)LoadImage(
					GetModuleHandle(NULL)
					, MAKEINTRESOURCE(IDI_ICON)
					, IMAGE_ICON
					, 16//GetSystemMetrics(SM_CXSMICON)
					, 16//GetSystemMetrics(SM_CXSMICON)
					, 0
				)
		);
	}
}
#endif
}

int wow_main(argc, argv) {
	wow_main_args(argc, argv);
	s8 fileCount = 0;
	int i;
	char* file[3] = { 0 };
	char* infile = 0;
	ALADPCMloop loopInfo[3] = { 0 };
	InstrumentChunk instInfo[3] = { 0 };
	CommonChunk commInfo[3] = { 0 };
	u32 sampleRate[3] = { 0 };
	
	win32icon();
	
	atexit(z64audioAtExit);
	
	STATE_DEBUG_PRINT = true;
	STATE_FABULOUS = false;
	
	#ifdef _WIN32
		char magic[] = { " \0" };
		// This fixes the coloring of text printed in CMD or PowerShell
		if (system(magic) != 0)
			PrintFail("Intro has failed.\n");
	#endif
	
	fprintf(
		stderr,
		"/******************************\n"
		" * \e[0;36mz64audio 1.0.0\e[m             *\n"
		" *   by rankaisija and z64me  *\n"
		" ******************************/\n\n"
	);
	
	/*{
	char cwd[1024];
	DebugPrint("cwd: '%s'\n", wow_getcwd(cwd, sizeof(cwd)));
	}*/
	
	if (argc == 1) {
		showArgs();
	}
	/* one input file = guided mode */
	else if (argc == 2 && memcmp(argv[1], "--", 2)) {
		// TODO revert for final release
		// gAudioState.mode = requestMode();
		gAudioState.mode = Z64AUDIOMODE_WAV_TO_ZZRTL; // For testing purposes
		infile = argv[1];
	#ifdef _WIN32
		gWaitAtExit = true;
	#endif
	}
	/* multiple arguments */
	else{
		int i;
		for (i = 1; i < argc; i += 2) {
			char* arg = argv[i];
			char* next = argv[i + 1];
			
			if (!strcmp(arg, "--tabledesign")) {
				shiftArgs(&argc, argv, i);
				
				return tabledesign(argc, argv, stdout);
			}else if (!strcmp(arg, "--vadpcm_enc")) {
				shiftArgs(&argc, argv, i);
				
				return vadpcm_enc(argc, argv);
			}
			
			if (!next)
				PrintFail("Argument '%s' is missing parameter\n", arg);
			
			if (!strcmp(arg, "--wav"))
				infile = next;
			else if (!strcmp(arg, "--mode"))
				gAudioState.mode = atoi(next);
			else
				PrintFail("Unknown argument '%s'\n", arg);
		}
	}
	
	/* ensure provided mode is acceptable */
	assertMode(gAudioState.mode);
	
	if ((fileCount = File_GetAndSort(infile, file)) <= 0)
		PrintFail("Something has gone terribly wrong... fileCount == %d", fileCount);
	
	for (s32 i = 0; i < fileCount; i++)
		Audio_Process(file[i], i, &loopInfo[i], &instInfo[i], &commInfo[i], &sampleRate[i]);
	
	DebugPrint("Starting clean for %d file(s)\n", fileCount);
	
	for (s32 i = 0; i < fileCount; i++) {
		Audio_Clean(file[i]);
	}
	
	Audio_GenerateInstrumentConf(file[0], fileCount, instInfo, sampleRate);
	
	for (i = 0; i < (sizeof(file) / sizeof(*file)); ++i)
		if (file[i])
			free(file[i]);
	
	DebugPrint("File free\t\t\tOK\n");
	
	return 0;
}

static void showModes(void) {
#define P(X) fprintf(stderr, X "\n")
	P("      1: wav to zzrtl instrument");
	P("      2: wav to aiff");
#undef P
}

static void assertMode(z64audioMode mode) {
	if (mode == Z64AUDIOMODE_UNSET
	    || mode < 0
	    || mode >= Z64AUDIOMODE_LAST
	) {
		fprintf(stderr, "invalid mode %d; valid modes are as follows:\n", mode);
		showModes();
		exit(EXIT_FAILURE);
	}
}

static z64audioMode requestMode(void) {
	int c;
	
	fprintf(stderr, "select one of the following modes:\n");
	showModes();
	c = getchar();
	fflush(stdin);
	
	return 1 + (c - '1');
}

static void showArgs(void) {
#define P(X) fprintf(stderr, X "\n")
	P("arguments:");
	P("  guided mode:");
	P("    z64audio \"input.wav\"");
	P("  command line mode:");
	P("    z64audio --wav \"input.wav\" --mode X");
	P("    where X is one of the following modes:");
	P("  extras:");
	P("    --tabledesign");
	P("    --vadpcm_enc");
	P("    example: z64audio --tabledesign -i 30 \"wow.aiff\" > \"wow.table\"");
	#ifdef _WIN32 /* helps users unfamiliar with command line */
		P("");
		P("Alternatively, Windows users can close this window and drop");
		P("a .wav file directly onto the z64audio executable. If you use");
		P("z64audio often, consider right-clicking a .wav, selecting");
		P("'Open With', and then z64audio.");
		fflush(stdin);
		getchar();
	#endif
#undef P
	exit(EXIT_FAILURE);
}

static void shiftArgs(int* argc, char* argv[], int i) {
	int newArgc = *argc - i;
	int k;
	
	for (k = 0; i <= *argc; ++k, ++i)
		argv[k] = argv[i];
	*argc = newArgc;
}

void z64audioAtExit(void)
{
	if (gWaitAtExit)
	{
		fflush(stdin);
		DebugPrint("\nPress ENTER to exit...\n");
		getchar();
	}
}

