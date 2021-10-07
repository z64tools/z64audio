#include "AudioTools.h"

char* gTableDesignIteration = "30";

void AudioTools_TableDesign(AudioSampleInfo* sampleInfo) {
	char basename[256];
	char path[256];
	char buffer[256];
	char* tableDesignArgv[] = {
		"tabledesign",
		"-i",
		gTableDesignIteration,
		sampleInfo->output,
		NULL
	};
	
	String_GetBasename(basename, sampleInfo->output);
	String_GetPath(path, sampleInfo->output);
	
	String_Copy(buffer, path);
	String_Combine(buffer, basename);
	String_Combine(buffer, ".table");
	
	FILE* table = fopen(buffer, "w");
	
	if (table == NULL) {
		printf_error("Audio_TableDesign: Could not create/open file [%s]", buffer);
	}
	
	printf_debug(
		"%s %s %s %s",
		tableDesignArgv[0],
		tableDesignArgv[1],
		tableDesignArgv[2],
		tableDesignArgv[3]
	);
	
	if (tabledesign(4, tableDesignArgv, table))
		printf_error("TableDesign has failed");
	
	fclose(table);
}
void AudioTools_VadpcmEnc(AudioSampleInfo* sampleInfo) {
	char basename[256];
	char path[256];
	char buffer[256];
	char* vadpcmArgv[] = {
		"vadpcm_enc",
		"-c",
		NULL,
		sampleInfo->output,
		NULL,
		NULL
	};
	
	String_GetBasename(basename, sampleInfo->output);
	String_GetPath(path, sampleInfo->output);
	String_Copy(buffer, path);
	String_Combine(buffer, basename);
	String_Combine(buffer, ".table");
	vadpcmArgv[2] = String_Generate(buffer);
	
	String_GetBasename(basename, sampleInfo->output);
	String_GetPath(path, sampleInfo->output);
	String_Copy(buffer, path);
	String_Combine(buffer, basename);
	String_Combine(buffer, ".aifc");
	vadpcmArgv[4] = String_Generate(buffer);
	
	FILE* table = fopen(buffer, "w");
	
	if (table == NULL) {
		printf_error("Audio_TableDesign: Could not create/open file [%s]", buffer);
	}
	
	printf_debug(
		"%s %s %s %s %s",
		vadpcmArgv[0],
		vadpcmArgv[1],
		vadpcmArgv[2],
		vadpcmArgv[3],
		vadpcmArgv[4]
	);
	
	if (vadpcm_enc(5, vadpcmArgv))
		printf_error("VadpcmEnc has failed");
	
	fclose(table);
	free(vadpcmArgv[2]);
	free(vadpcmArgv[4]);
}