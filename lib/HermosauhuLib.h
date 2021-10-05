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

void Lib_SetPrinfSuppressLevel(PrintfSuppressLevel lvl);

void printf_debug(const char* fmt, ...);
void printf_warning(const char* fmt, ...);
void printf_error(const char* fmt, ...);
void printf_info(const char* fmt, ...);

u32 Lib_Str_HexToInt(char* string);
s32 Lib_Str_GetLineCount(char* str);
char* Lib_Str_GetLine(char* str, s32 line);
char* Lib_Str_GetWord(char* str, s32 word);
void Lib_Str_CaseToLow(char* s, s32 i);
void Lib_Str_CaseToUpper(char* s, s32 i);
void Lib_Str_GetFilePathAndName(char* _src, char* _dest, char* _path);

void* Lib_MemMem(const void* haystack, size_t haystackSize, const void* needle, size_t needleSize);
void* Lib_GetFile(char* filename, int* size);
