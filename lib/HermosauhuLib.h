#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

/* 👺
      `..........................`
   `-:///////:---://////////////:-``
   `:////////-````.-////////////-.````
   ://///////:.`````.:////////:.`````.-
   ///////////:.``````-//////-``````-::
   ////////+syys/.`````.://:.`````-+o+:
   ///////smMMMMNd/`````.:-`````.omMNms
   //////+dMMMMMMMNy-````:-````:yNMMMMd
   //////+dMMMMMMMMMmy::://:/ohNMMMMMMd
   ///////smMMMMMMNs/+s////+dy+/yNMMMNs
   ////////sdMMMMNy```:////oy:``-yMMNy:
   /////////+ymNMMm:.-+++++oo+///oyyso+////////.         `
   ///////////+syhdhoooooooooooooooooooooooooooo.    ./+oso+-
   ///////////////++++oooooooooooooooooooooooooo+`  `os/://oso:
   //////::///////////+oooooooooooooooooooooooo+.   /h/ ````-hy
   /////-``.-::------::/+++//::::::::::::::::::`    -oo/os: .hs
   .////.`````````..--://+/::-..```````              `.:/.`.+y-
   :///.``````.-:/+ssyyyyysooo+/-.````                `-/sys-
   -///-````.:/+syyyyyyyyyssssssso+/---`          `:+++os+-.
    ://:.`.-/+oyyyyyyyyyysssssssssssydmyyo-.`     ohs-
    `://////+syyyyyyo++//+ossssssssshNmmmmmmdss/`.o/`
     -//////+syyyys+///////++osssssymmmmmmmmmmNy/:--.`
      -//////+osso////////////+oo/:/sddmmmmmmNd+:::::-`
       `://////////////////////:`   ``.:oohmmmo::::::.
        `-////////////////////-`          `..:--::::-`
          `.-//////////////-.`                  ````
             ``````````````
 */

typedef signed char s8;
typedef unsigned char u8;
typedef signed short int s16;
typedef unsigned short int u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long int s64;
typedef unsigned long long int u64;
typedef float f32;
typedef double f64;

typedef enum {
	PSL_DEBUG = -1,
	PSL_NONE,
	PSL_NO_INFO,
	PSL_NO_WARNING,
	PSL_NO_ERROR,
} PrintfSuppressLevel;

// printf
void printf_SetPrintfSuppressLevel(PrintfSuppressLevel lvl);
void printf_debug(const char* fmt, ...);
void printf_warning(const char* fmt, ...);
void printf_error(const char* fmt, ...);
void printf_info(const char* fmt, ...);

// Lib
void* Lib_MemMem(const void* haystack, size_t haystackSize, const void* needle, size_t needleSize);
void* Lib_Malloc(s32 size);

// File
s32 File_LoadToMem(void** dst, char* src);
s32 File_WriteToFromMem(char* dst, void* src, s32 size);
s32 File_LoadToMem_ReqExt(void** dst, char* src, const char* ext);
s32 File_WriteToFromMem_ReqExt(char* dst, void* src, s32 size, const char* ext);

// string
u32 String_ToHex(char* string);
s32 String_GetLineCount(char* str);
char* String_GetLine(char* str, s32 line);
char* String_GetWord(char* str, s32 word);
void String_CaseToLow(char* s, s32 i);
void String_CaseToUp(char* s, s32 i);
void String_GetPath(char* dst, char* src);
void String_GetBasename(char* dst, char* src);
void String_GetFilename(char* dst, char* src);
