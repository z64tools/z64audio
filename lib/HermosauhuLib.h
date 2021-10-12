#ifndef __HERMOSAUHU_LIB_H__
#define __HERMOSAUHU_LIB_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

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
	false,
	true
} CheckStatement;

typedef enum {
	PSL_DEBUG = -1,
	PSL_NONE,
	PSL_NO_INFO,
	PSL_NO_WARNING,
	PSL_NO_ERROR,
} PrintfSuppressLevel;

typedef enum {
	SWAP_U8  = 1,
	SWAP_U16 = 2,
	SWAP_U32 = 4,
	SWAP_U64 = 8,
	SWAP_F80 = 10
} SwapSize;

#ifndef __HERMOSAUHU_DATA_T__
#define __HERMOSAUHU_DATA_T__

typedef union {
	void* p;
	u8*   u8;
	u16*  u16;
	u32*  u32;
	u64*  u64;
	s8*   s8;
	s16*  s16;
	s32*  s32;
	s64*  s64;
	f32*  f32;
	f64*  f64;
} PointerCast;

typedef struct {
	union {
		void* data;
		PointerCast cast;
	};
	u32 memSize;
	u32 dataSize;
	u32 seekPoint;
} MemFile;

#endif

extern PrintfSuppressLevel gPrintfSuppress;

/* 👺 PRINTF 👺 */
void printf_SetSuppressLevel(PrintfSuppressLevel lvl);
void printf_SetPrefix(char* fmt);
void printf_toolinfo(const char* toolname, const char* fmt, ...);
void printf_debug(const char* fmt, ...);
void printf_warning(const char* fmt, ...);
void printf_error(const char* fmt, ...);
void printf_info(const char* fmt, ...);
void printf_WinFix();

/* 👺 LIB 👺 */
void* Lib_MemMem(const void* haystack, size_t haystackSize, const void* needle, size_t needleSize);
void* Lib_Malloc(void* data, s32 size);
void* Lib_Realloc(void* data, s32 size);
void Lib_ByteSwap(void* src, s32 size);

/* 👺 FILE 👺 */
s32 File_Load(void** dst, char* filepath);
void File_Save(char* filepath, void* src, s32 size);
s32 File_Load_ReqExt(void** dst, char* filepath, const char* ext);
void File_Save_ReqExt(char* filepath, void* src, s32 size, const char* ext);
s32 Lib_ParseArguments(char* argv[], char* arg, u32* parArg);

/* 👺 MEMFILE 👺 */
void MemFile_Malloc(MemFile* memFile, u32 size);
void MemFile_Realloc(MemFile* memFile, u32 size);
void MemFile_Rewind(MemFile* memFile);
void MemFile_Write(MemFile* dest, void* src, u32 size);
void MemFile_LoadFile(MemFile* memFile, char* filepath);
void MemFile_SaveFile(MemFile* memFile, char* filepath);
void MemFile_LoadFile_ReqExt(MemFile* memFile, char* filepath, const char* ext);
void MemFile_SaveFile_ReqExt(MemFile* memFile, char* filepath, s32 size, const char* ext);
void MemFile_Free(MemFile* memFile);

/* 👺 STRING 👺 */
u32 String_HexStrToInt(char* string);
u32 String_NumStrToInt(char* string);
f64 String_NumStrToF64(char* string);
s32 String_GetLineCount(char* str);
char* String_GetLine(char* str, s32 line);
char* String_GetWord(char* str, s32 word);
void String_CaseToLow(char* s, s32 i);
void String_CaseToUp(char* s, s32 i);
void String_GetPath(char* dst, char* src);
void String_GetBasename(char* dst, char* src);
void String_GetFilename(char* dst, char* src);

#define PRNT_DGRY "\e[90;2m"
#define PRNT_GRAY "\e[0;90m"
#define PRNT_REDD "\e[0;91m"
#define PRNT_DRED "\e[91;2m"
#define PRNT_YELW "\e[0;93m"
#define PRNT_BLUE "\e[0;94m"
#define PRNT_RSET "\e[m"
#define PRNT_NL   "\n"
#define PRNT_RNL  PRNT_RSET PRNT_NL
#define PRNT_TODO "\e[91;2m" "TODO"

#define ARRAY_COUNT(arr)     (s32)(sizeof(arr) / sizeof(arr[0]))
#define ABS(val)             (val < 0 ? -val : val)
#define CLAMP(val, min, max) ((val) < min ? min : (val) > max ? max : (val))
#define CLAMP_MIN(val, min)  ((val) < min ? min : (val))
#define CLAMP_MAX(val, max)  ((val) > max ? max : (val))

#define String_Copy(dst, src)   strcpy(dst, src)
#define String_Merge(dst, src)  strcat(dst, src)
#define String_Generate(string) strdup(string)

#define printf_debugExt(...) if (gPrintfSuppress <= PSL_DEBUG) {    \
		printf(PRNT_DGRY "[%s]: " PRNT_REDD "%s: " PRNT_GRAY "[%d]\n"PRNT_RSET, __FILE__, __FUNCTION__, __LINE__); \
		printf_debug(__VA_ARGS__);                                  \
}

#define Main(y1, y2) main(y1, y2)

#include <math.h>

#endif /* __HERMOSAUHU_LIB_H__ */