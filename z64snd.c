// Build in terminal for windows
// sudo chmod 777 -R ./* && x86_64-w64-mingw32-gcc -o z64snd z64snd.c
// sudo chmod 777 -R ./* && i686-w64-mingw32-gcc -Wno-pointer-to-int-cast -o z64snd z64snd.c

// sudo chmod 777 -R ./* && gcc z64snd.c -o z64snd

// REGEX
// ^([a-zA-Z0-9*_]+)\s+([a-zA-Z0-9_]+)(([()a-zA-Z0-9*_ ,]|\s)+)?.

#define __FLOAT80_SUCKS__

#include "include/z64snd.h"

const char sExtension[] = {
	".wav"
};

static void showModes(void)
{
#define P(X) fprintf(stderr, X "\n")
	P("      1: wav to zzrtl instrument");
	P("      2: wav to aiff");
#undef P
}

static void assertMode(enum z64audioMode mode)
{
	if (mode == Z64AUDIOMODE_UNSET
		|| mode < 0
		|| mode >= Z64AUDIOMODE_LAST
	)
	{
		fprintf(stderr, "invalid mode %d; valid modes are as follows:\n", mode);
		showModes();
		exit(EXIT_FAILURE);
	}
}

static enum z64audioMode requestMode(void)
{
	int c;
	
	fprintf(stderr, "select one of the following modes:\n");
	showModes();
	c = getchar();
	fflush(stdin);
	
	return 1 + (c - '1');
}

static void showArgs(void)
{
#define P(X) fprintf(stderr, X "\n")
	P("arguments:");
	P("  guided mode:");
	P("    z64audio \"input.wav\"");
	P("  command line mode:");
	P("    z64audio --wav \"input.wav\" --mode X");
	P("    where X is one of the following modes:");
#ifdef _WIN32 /* helps users unfamiliar with command line */
	P("");
	P("Alternatively, Windows users can close this window and drop");
	P("a .wav file directly onto the z64audio executable. If you use");
	P("z64audio often, consider right-clicking a .wav, selecting");
	P("'Open With', and then z64audio.");
	getchar();
#endif
#undef P
	exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
	char fname[1024] = { 0 };
	char path[128] = { 0 };
	char buffer[128] = { 0 };
	s8 procCount = 0;
	int i;
	enum z64audioMode mode = Z64AUDIOMODE_UNSET;
	
	char* file[3] = {0};
	char* infile = 0;
	
	ALADPCMloop loopInfo[3] = { 0 };
	InstrumentChunk instInfo[3] = { 0 };
	CommonChunk commInfo[3] = { 0 };
	u32 sampleRate[3] = { 0 };
	
	STATE_DEBUG_PRINT = true;
	STATE_FABULOUS = false;
	
	fprintf(stderr, 
		"/*****************************\n"
		" * z64audio 1.0.0            *\n"
		" *   by rankaisija and z64me *\n"
		" *****************************/\n"
	);
	
	if (argc == 1)
	{
		showArgs();
	}
	/* one input file = guided mode */
	else if (argc == 2)
	{
		mode = requestMode();
		infile = argv[1];
	}
	/* multiple arguments */
	else
	{
		int i;
		for (i = 1; i < argc; i += 2)
		{
			char *arg = argv[i];
			char *next = argv[i + 1];
			
			if (!next)
				PrintFail("Argument '%s' is missing parameter", arg);
			
			if (!strcmp(arg, "--wav"))
				infile = next;
			else if (!strcmp(arg, "--mode"))
				mode = atoi(next);
			else
				PrintFail("Unknown argument '%s'", arg);
		}
	}
	
	/* ensure provided mode is acceptable */
	assertMode(mode);
	
	if ((procCount = File_GetAndSort(infile, file)) <= 0)
		PrintFail("Something has gone terribly wrong... fileCount == %d", procCount);
	for (s32 i = 0; i < procCount; i++)
		Audio_Process(file[i], 0, &loopInfo[i], &instInfo[i], &commInfo[i], &sampleRate[i]);
	
	for (s32 i = 0; i < procCount; i++) {
		SetFilename(file[i], fname, path, NULL, NULL, NULL);
		Audio_Clean(path, fname);
	}
	
	Audio_GenerateInstrumentConf(fname, procCount, instInfo);
	
	for (i = 0; i < (sizeof(file) / sizeof(*file)); ++i)
		if (file[i])
			free(file[i]);
	
#ifdef _WIN32
	if (argc == 2)
	{
		fflush(stdin);
		getchar();
	}
#endif
	
	return 0;
}

