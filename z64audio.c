#include "lib/ExtLib.h"
#include "lib/AudioConvert.h"
#include "lib/AudioTools.h"

void sleep(u32);

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
	"z64audio-cli 2.0 beta"
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

s32 Main(s32 argc, char* argv[]) {
	AudioSampleInfo sample;
	char* input = NULL;
	char* output = NULL;
	u32 parArg;
	u8 wait = 0;
	
	printf_WinFix();
	printf_SetPrefix("");
	
	if (ParseArg("-D") || ParseArg("--D")) {
		printf_SetSuppressLevel(PSL_DEBUG);
	}
	
	if (ParseArg("-s") || ParseArg("--s")) {
		printf_SetSuppressLevel(PSL_NO_WARNING);
	}
	
	if (ParseArg("-i") || ParseArg("--i")) {
		input = argv[parArg];
	}
	
	if (ParseArg("-o") || ParseArg("--o")) {
		output = argv[parArg];
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
		if (String_MemMem(argv[1], ".wav")) {
			String_Copy(outbuf, String_GetPath(argv[1]));
			String_Merge(outbuf, String_GetBasename(argv[1]));
			String_Merge(outbuf, ".aiff");
			
			input = argv[1];
			output = outbuf;
		}
		if (String_MemMem(argv[1], ".aiff")) {
			String_Copy(outbuf, String_GetPath(argv[1]));
			String_Merge(outbuf, String_GetBasename(argv[1]));
			String_Merge(outbuf, ".wav");
			
			input = argv[1];
			output = outbuf;
		}
		
		wait++;
	}
	
	if (input == NULL) {
		printf_toolinfo(
			sToolName,
			sToolUsage
		);
		#ifdef _WIN32
			getchar();
		#endif
		
		return 1;
	}
	
	printf_toolinfo(
		sToolName,
		""
	);
	
	OsPrintfEx("Audio_InitSampleInfo");
	Audio_InitSampleInfo(&sample, input, output);
	OsPrintfEx("Audio_LoadSample");
	Audio_LoadSample(&sample);
	
	if (ParseArg("-N") || ParseArg("--N")) {
		printf_info("BitDepth    [ %8d ] " PRNT_GRAY "[%s]", sample.bit, sample.input);
		printf_info("Sample Rate [ %8d ] " PRNT_GRAY "[%s]", sample.sampleRate, sample.input);
		printf_info("Channels    [ %8d ] " PRNT_GRAY "[%s]", sample.channelNum, sample.input);
		printf_info("Frames      [ %8d ] " PRNT_GRAY "[%s]", sample.samplesNum, sample.input);
		printf_info("Data Size   [ %8d ] " PRNT_GRAY "[%s]", sample.size, sample.input);
		printf_info("File Size   [ %8d ] " PRNT_GRAY "[%s]", sample.memFile.dataSize, sample.input);
		
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
	
	if (ParseArg("-p") || ParseArg("--p")) {
		AudioTools_LoadCodeBook(&sample, argv[parArg]);
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
	
	Audio_SaveSample(&sample);
	
	if (ParseArg("-v") || ParseArg("--v")) {
		if (!Lib_MemMem(sample.output, strlen(sample.output), ".aiff", 5)) {
			printf_warning("Output isn't [.aiff] file. Skipping generating vadpcm files");
		} else {
			// AudioTools_RunTableDesign(&sample);
			// AudioTools_RunVadpcmEnc(&sample);
		}
	}
	
	Audio_FreeSample(&sample);
	
	if (wait) {
		#ifdef _WIN32
			sleep(2);
		#endif
	}
	
	return 0;
}