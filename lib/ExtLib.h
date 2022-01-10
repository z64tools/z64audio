#ifndef __EXTLIB_H__
#define __EXTLIB_H__

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <ctype.h>

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
typedef uintptr_t uPtr;
typedef intptr_t sPtr;

typedef struct {
	u8 hue;
	u8 saturation;
	u8 luminance;
} HLS8;

typedef struct {
	u8 hue;
	u8 saturation;
	u8 luminance;
	u8 alpha;
} HLSA8;

typedef struct {
	union {
		struct {
			u8 r;
			u8 g;
			u8 b;
		};
		u8 c[3];
	};
} RGB8;

typedef struct {
	union {
		struct {
			u8 r;
			u8 g;
			u8 b;
			u8 a;
		};
		u8 c[4];
	};
} RGBA8;

typedef struct {
	f32 r;
	f32 g;
	f32 b;
} RGB32;

typedef struct {
	f32 r;
	f32 g;
	f32 b;
	f32 a;
} RGBA32;

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

typedef struct Node {
	struct Node* prev;
	struct Node* next;
} Node;

typedef struct {
	union {
		void* data;
		PointerCast cast;
	};
	u32 memSize;
	u32 dataSize;
	u32 seekPoint;
	struct {
		f64   age;
		char* name;
	} info;
} MemFile;

void printf_SetSuppressLevel(PrintfSuppressLevel lvl);
void printf_SetPrefix(char* fmt);
void printf_SetPrintfTypes(const char* d, const char* w, const char* e, const char* i);
void printf_toolinfo(const char* toolname, const char* fmt, ...);
void printf_debug(const char* fmt, ...);
void printf_warning(const char* fmt, ...);
void printf_error(const char* fmt, ...);
void printf_info(const char* fmt, ...);
void printf_align(const char* info, const char* fmt, ...);
void printf_WinFix();

void* Lib_MemMem(const void* haystack, size_t haystackSize, const void* needle, size_t needleSize);
void* Lib_MemMemIgnCase(const void* haystack, size_t haystackSize, const void* needle, size_t needleSize);
void* Lib_Malloc(void* data, s32 size);
void* Lib_Calloc(void* data, s32 size);
void* Lib_Realloc(void* data, s32 size);
void* Lib_Free(void* data);
s32 Lib_Touch(char* file);
void Lib_ByteSwap(void* src, s32 size);

void* File_Load(void* destSize, char* filepath);
void File_Save(char* filepath, void* src, s32 size);
void* File_Load_ReqExt(void* size, char* filepath, const char* ext);
void File_Save_ReqExt(char* filepath, void* src, s32 size, const char* ext);
s32 Lib_ParseArguments(char* argv[], char* arg, u32* parArg);

MemFile MemFile_Initialize();
void MemFile_Malloc(MemFile* memFile, u32 size);
void MemFile_Realloc(MemFile* memFile, u32 size);
void MemFile_Rewind(MemFile* memFile);
s32 MemFile_Write(MemFile* dest, void* src, u32 size);
s32 MemFile_Printf(MemFile* dest, const char* fmt, ...);
s32 MemFile_Read(MemFile* src, void* dest, u32 size);
s32 MemFile_LoadFile(MemFile* memFile, char* filepath);
s32 MemFile_LoadFile_String(MemFile* memFile, char* filepath);
s32 MemFile_SaveFile(MemFile* memFile, char* filepath);
s32 MemFile_SaveFile_String(MemFile* memFile, char* filepath);
s32 MemFile_LoadFile_ReqExt(MemFile* memFile, char* filepath, const char* ext);
s32 MemFile_SaveFile_ReqExt(MemFile* memFile, char* filepath, s32 size, const char* ext);
void MemFile_Free(MemFile* memFile);
void MemFile_Clear(MemFile* memFile);

#define String_MemMem(src, comp) Lib_MemMem(src, strlen(src), comp, strlen(comp))
u32 String_HexStrToInt(char* string);
u32 String_NumStrToInt(char* string);
f64 String_NumStrToF64(char* string);
s32 String_GetLineCount(char* str);
char* String_Line(char* str, s32 line);
char* String_Word(char* str, s32 word);
char* String_GetLine(char* str, s32 line);
char* String_GetWord(char* str, s32 word);
void String_GetLine2(char* dest, char* str, s32 line);
void String_GetWord2(char* dest, char* str, s32 word);
void String_CaseToLow(char* s, s32 i);
void String_CaseToUp(char* s, s32 i);
char* String_GetPath(char* src);
char* String_GetBasename(char* src);
char* String_GetFilename(char* src);
void String_Insert(char* point, char* insert);
void String_Remove(char* point, s32 amount);
void String_SwapExtension(char* dest, char* src, const char* ext);
char* String_GetSpacedArg(char* argv[], s32 cur);

#define Node_Add(head, node) { \
		OsAssert(node != NULL) \
		typeof(node) lastNode = head; \
		s32 __nodePos = 0; \
		if (lastNode == NULL) { \
			head = node; \
		} else { \
			while (lastNode->next) { \
				lastNode = lastNode->next; \
				__nodePos++; \
			} \
			lastNode->next = node; \
			node->prev = lastNode; \
		} \
}

#define Node_Kill(head, node) { \
		OsAssert(node != NULL) \
		if (node->next) { \
			node->next->prev = node->prev; \
		} \
		if (node->prev) { \
			node->prev->next = node->next; \
		} else { \
			head = node->next; \
		} \
		free(node); \
		node = NULL; \
}

#define Swap(a, b) { \
		typeof(a) y = a; \
		a = b; \
		b = y; \
}

#define WrapF(x, min, max) ({ \
		typeof(x) r = (x); \
		typeof(x) range = (max) - (min) + 1; \
		if (r < (min)) { \
			r += range * (((min) - r) / range + 1); \
		} \
		(min) + fmod((r - (min)), range); \
	})

#define Wrap(x, min, max) ({ \
		typeof(x) r = (x); \
		typeof(x) range = (max) - (min) + 1; \
		if (r < (min)) { \
			r += range * (((min) - r) / range + 1); \
		} \
		(min) + (r - (min)) % range; \
	})

// Checks endianess with tst & tstP
#define ReadBE(in) ({ \
		typeof(in) out; \
		s32 tst = 1; \
		u8* tstP = (u8*)&tst; \
		if (tstP[0] != 0) { \
			s32 size = sizeof(in); \
			if (size == 2) { \
				out = __builtin_bswap16(in); \
			} else if (size == 4) { \
				out = __builtin_bswap32(in); \
			} else if (size == 8) { \
				out = __builtin_bswap64(in); \
			} else { \
				out = in; \
			} \
		} else { \
			out = in; \
		} \
		out; \
	} \
)

#define WriteBE(dest, set) { \
		typeof(dest) get = set; \
		dest = ReadBE(get); \
}

#define SwapBE(in) WriteBE(in, in)

#define Decr(x) (x -= (x > 0) ? 1 : 0)
#define Incr(x) (x += (x < 0) ? 1 : 0)

extern PrintfSuppressLevel gPrintfSuppress;

#define PRNT_DGRY "\e[90;2m"
#define PRNT_GRAY "\e[0;90m"
#define PRNT_DRED "\e[91;2m"
#define PRNT_REDD "\e[0;91m"
#define PRNT_GREN "\e[0;92m"
#define PRNT_YELW "\e[0;93m"
#define PRNT_BLUE "\e[0;94m"
#define PRNT_PRPL "\e[0;95m"
#define PRNT_CYAN "\e[0;96m"
#define PRNT_RSET "\e[m"
#define PRNT_NL   "\n"
#define PRNT_RNL  PRNT_RSET PRNT_NL
#define PRNT_TODO "\e[91;2m" "TODO"

#ifndef NDEBUG
	#define OsPrintf     printf_debug
	#define OsPrintfEx   printf_debugExt
	#define OsPrintfLine printf_debugInfo
	#define OsAssert(exp) if (!(exp)) { \
			printf(PRNT_DGRY "[%s]: " PRNT_REDD "%s: " PRNT_GRAY "[%d]\n"PRNT_RSET, __FILE__, __FUNCTION__, __LINE__); \
			printf_debug(PRNT_YELW "OsAssert(\a " PRNT_RSET # exp PRNT_YELW " );"); \
			exit(EXIT_FAILURE); \
	}
	
    #ifndef __EXTLIB_C__
		
		#define Lib_Malloc(data, size) Lib_Malloc(data, size); \
			OsPrintfEx("Lib_Malloc: size [0x%X]", size);
		
		#define Lib_Calloc(data, size) Lib_Calloc(data, size); \
			OsPrintfEx("Lib_Calloc: size [0x%X]", size);
		
    #endif
	
#else
	#define OsPrintf(...)   if (0) {}
	#define OsPrintfEx(...) if (0) {}
	#define OsAssert(exp)   if (0) {}
#endif

#define MAX(a, b)            ((a) > (b) ? (a) : (b))
#define MIN(a, b)            ((a) < (b) ? (a) : (b))
#define ABS_MAX(a, b)        (ABS(a) > ABS(b) ? (a) : (b))
#define ABS_MIN(a, b)        (ABS(a) < ABS(b) ? (a) : (b))
#define ARRAY_COUNT(arr)     (s32)(sizeof(arr) / sizeof(arr[0]))
#define ABS(val)             ((val) < 0 ? -(val) : (val))
#define CLAMP(val, min, max) ((val) < (min) ? (min) : (val) > (max) ? (max) : (val))
#define CLAMP_MIN(val, min)  ((val) < (min) ? (min) : (val))
#define CLAMP_MAX(val, max)  ((val) > (max) ? (max) : (val))
#define ArrayCount(arr)      (u32)(sizeof(arr) / sizeof(arr[0]))

#define String_Copy(dst, src)   strcpy(dst, src)
#define String_Merge(dst, src)  strcat(dst, src)
#define String_Generate(string) strdup(string)

#define printf_debugExt(...) if (gPrintfSuppress <= PSL_DEBUG) { \
		printf(PRNT_DGRY "[%s]: " PRNT_REDD "%s: " PRNT_GRAY "[%d]\n"PRNT_RSET, __FILE__, __FUNCTION__, __LINE__); \
		printf_debug(__VA_ARGS__); \
}

#define printf_debugInfo(x) if (gPrintfSuppress <= PSL_DEBUG) { \
		printf(PRNT_DGRY "[%s]: " PRNT_REDD "%s: " PRNT_GRAY "[%d] " PRNT_RSET "%s\n", __FILE__, __FUNCTION__, __LINE__, # x); \
		x; \
}

#define Main(y1, y2) main(y1, y2)

#endif /* __EXTLIB_H__ */
