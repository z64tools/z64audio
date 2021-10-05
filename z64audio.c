#include "lib/HermosauhuLib.h"
#include "lib/Audiotools.h"

char path[] = {
	"../../pathto/the_weirdest/file.exact"
};

s32 main(const s32 argc, const char* argv[]) {
	printf_SetPrintfSuppressLevel(PSL_DEBUG);
        
	char pathName[1024];
	char fileName[1024];
	char fileNameExt[1024];
        
	String_GetPath(pathName, path);
	String_GetBasename(fileName, path);
    String_GetFilename(fileNameExt, path);

    void* file = NULL;
    File_LoadToMem(&file, "z64audio.c", 0);
        
	printf_info("\n%s\n%s\n%s", pathName, fileName, fileNameExt);
    printf("%s\n", (char*)file);
        
	return 0;
}