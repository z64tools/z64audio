#define __EXTLIB_C__
#include "ExtLib.h"
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>

// ExtendedLibrary

// Print
PrintfSuppressLevel gPrintfSuppress = 0;
char* sPrintfPrefix = "ExtLib";

void printf_SetSuppressLevel(PrintfSuppressLevel lvl) {
	gPrintfSuppress = lvl;
}

void printf_SetPrefix(char* fmt) {
	sPrintfPrefix = fmt;
}

void printf_toolinfo(const char* toolname, const char* fmt, ...) {
	if (gPrintfSuppress >= PSL_NO_INFO)
		return;
	va_list args;
	
	// [0;36m%s\e[m
	va_start(args, fmt);
	#if 1
		printf(
			"\e[90;2m"
			"=----------------------------------=\n"
			"|                                  |\n"
		);
		printf("\033[1A" "\033[3C");
		printf("\e[0;96m%s\e[90;2m", toolname);
		printf(
			"\n"
			"=----------------------------------=\e[m\n"
		);
	#else
		printf(
			"\e[90;2m"
			"╔══════════════════════════════════╗\n"
			"║                                  ║\n"
		);
		printf("\033[1A" "\033[3C");
		printf("\e[0;96m%s\e[90;2m", toolname);
		printf(
			"\n"
			"╚══════════════════════════════════╝\e[m\n"
		);
	#endif
	vprintf(
		fmt,
		args
	);
	va_end(args);
}

void printf_debug(const char* fmt, ...) {
	if (gPrintfSuppress > PSL_DEBUG)
		return;
	va_list args;
	
	va_start(args, fmt);
	printf(
		"%s"
		"\e[90;2m"
		"[\e[0;90mdebg\e[90;2m]:\t"
		"\e[m",
		sPrintfPrefix
	);
	vprintf(
		fmt,
		args
	);
	printf("\n");
	va_end(args);
}

void printf_warning(const char* fmt, ...) {
	if (gPrintfSuppress >= PSL_NO_WARNING)
		return;
	va_list args;
	
	va_start(args, fmt);
	printf(
		"%s"
		"\e[90;2m"
		"[\e[0;93mwarn\e[90;2m]:\t"
		"\e[m",
		sPrintfPrefix
	);
	vprintf(
		fmt,
		args
	);
	printf("\n");
	va_end(args);
}

void printf_error(const char* fmt, ...) {
	if (gPrintfSuppress < PSL_NO_ERROR) {
		va_list args;
		
		va_start(args, fmt);
		printf(
			"%s"
			"\e[90;2m"
			"[\e[0;91merr!\e[90;2m]:\t"
			"\e[m",
			sPrintfPrefix
		);
		vprintf(
			fmt,
			args
		);
		printf("\n");
		va_end(args);
	}
	exit(EXIT_FAILURE);
}

void printf_info(const char* fmt, ...) {
	if (gPrintfSuppress >= PSL_NO_INFO)
		return;
	va_list args;
	
	va_start(args, fmt);
	printf(
		"%s"
		"\e[90;2m"
		"[\e[0;94minfo\e[90;2m]:\t"
		"\e[m",
		sPrintfPrefix
	);
	vprintf(
		fmt,
		args
	);
	printf("\n");
	va_end(args);
}

void printf_WinFix() {
	#ifdef _WIN32
		system("\0");
	#endif
}

// Lib
void* Lib_MemMem(const void* haystack, size_t haystackSize, const void* needle, size_t needleSize) {
	if (haystack == NULL || needle == NULL)
		return NULL;
	register char* cur, * last;
	const char* cl = (const char*)haystack;
	const char* cs = (const char*)needle;
	
	/* we need something to compare */
	if (haystackSize == 0 || needleSize == 0)
		return NULL;
	
	/* "s" must be smaller or equal to "l" */
	if (haystackSize < needleSize)
		return NULL;
	
	/* special case where s_len == 1 */
	if (needleSize == 1)
		return memchr(haystack, (int)*cs, haystackSize);
	
	/* the last position where its possible to find "s" in "l" */
	last = (char*)cl + haystackSize - needleSize;
	
	for (cur = (char*)cl; cur <= last; cur++) {
		if (*cur == *cs && memcmp(cur, cs, needleSize) == 0)
			return cur;
	}
	
	return NULL;
}

static int memcasecmp(const char* vs1, const char* vs2, size_t n) {
	size_t i;
	char const* s1 = vs1;
	char const* s2 = vs2;
	
	for (i = 0; i < n; i++) {
		unsigned char u1 = s1[i];
		unsigned char u2 = s2[i];
		int U1 = toupper(u1);
		int U2 = toupper(u2);
		int diff = (255 <= __INT_MAX__ ? U1 - U2
	    : U1 < U2 ? -1 : U2 < U1);
		if (diff)
			return diff;
	}
	
	return 0;
}

void* Lib_MemMemIgnCase(const void* haystack, size_t haystackSize, const void* needle, size_t needleSize) {
	if (haystack == NULL || needle == NULL)
		return NULL;
	register char* cur, * last;
	const char* cl = (const char*)haystack;
	const char* cs = (const char*)needle;
	
	/* we need something to compare */
	if (haystackSize == 0 || needleSize == 0)
		return NULL;
	
	/* "s" must be smaller or equal to "l" */
	if (haystackSize < needleSize)
		return NULL;
	
	/* special case where s_len == 1 */
	if (needleSize == 1)
		return memchr(haystack, (int)*cs, haystackSize);
	
	/* the last position where its possible to find "s" in "l" */
	last = (char*)cl + haystackSize - needleSize;
	
	for (cur = (char*)cl; cur <= last; cur++) {
		char lowCur = *cur;
		
		if (!(lowCur >= 'a' && lowCur <= 'z') && !(lowCur >= 'A' && lowCur <= 'Z')) {
			continue;
		}
		
		lowCur = lowCur < 'a' ? lowCur + 32 : lowCur;
		
		if (lowCur == *cs && memcasecmp(cur, cs, needleSize) == 0)
			return cur;
	}
	
	return NULL;
}

void Lib_ByteSwap(void* src, s32 size) {
	u32 buffer[16] = { 0 };
	u8* temp = (u8*)buffer;
	u8* srcp = src;
	
	for (s32 i = 0; i < size; i++) {
		temp[size - i - 1] = srcp[i];
	}
	
	for (s32 i = 0; i < size; i++) {
		srcp[i] = temp[i];
	}
}

void* Lib_Malloc(void* data, s32 size) {
	data = malloc(size);
	
	if (data == NULL) {
		printf_error("Could not allocate [0x%X] bytes.", size);
	}
	
	return data;
}

void* Lib_Calloc(void* data, s32 size) {
	data = Lib_Malloc(data, size);
	memset(data, 0, size);
	
	return data;
}

void* Lib_Realloc(void* data, s32 size) {
	data = realloc(data, size);
	
	if (data == NULL) {
		printf_error("Could not reallocate to [0x%X] bytes.", size);
	}
	
	return data;
}

void* Lib_Free(void* data) {
	if (data)
		free(data);
	
	return NULL;
}

s32 Lib_Touch(char* file) {
	MemFile mem = MemFile_Initialize();
	
	if (MemFile_LoadFile(&mem, file)) {
		return 1;
	}
	
	if (MemFile_SaveFile(&mem, file)) {
		MemFile_Free(&mem);
		
		return 1;
	}
	
	MemFile_Free(&mem);
	
	return 0;
}

s32 Lib_ParseArguments(char* argv[], char* arg, u32* parArg) {
	s32 i = 1;
	
	if (parArg != NULL)
		*parArg = 0;
	
	while (argv[i] != NULL) {
		if (Lib_MemMem(argv[i], strlen(arg), arg, strlen(arg))) {
			if (parArg != NULL)
				*parArg =  i + 1;
			
			return i + 1;
		}
		
		i++;
	}
	
	return 0;
}

// File
void* File_Load(void* destSize, char* filepath) {
	s32 size;
	void* dest;
	s32* ss = destSize;
	
	FILE* file = fopen(filepath, "rb");
	
	if (file == NULL) {
		printf_error("Could not open file [%s]", filepath);
	}
	
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	dest = Lib_Malloc(0, size);
	if (dest == NULL) {
		printf_error("Failed to malloc [0x%X] bytes to store data from [%s].", size, filepath);
	}
	rewind(file);
	fread(dest, sizeof(char), size, file);
	fclose(file);
	
	ss[0] = size;
	
	return dest;
}

void File_Save(char* filepath, void* src, s32 size) {
	FILE* file = fopen(filepath, "w");
	
	if (file == NULL) {
		printf_error("Failed to fopen file [%s].", filepath);
	}
	
	fwrite(src, sizeof(u8), size, file);
	fclose(file);
}

void* File_Load_ReqExt(void* size, char* filepath, const char* ext) {
	if (Lib_MemMem(filepath, strlen(filepath), ext, strlen(ext))) {
		return File_Load(size, filepath);
	}
	printf_error("[%s] does not match extension [%s]", filepath, ext);
	
	return 0;
}

void File_Save_ReqExt(char* filepath, void* src, s32 size, const char* ext) {
	if (Lib_MemMem(filepath, strlen(filepath), ext, strlen(ext))) {
		File_Save(filepath, src, size);
		
		return;
	}
	
	printf_error("[%s] does not match extension [%s]", src, ext);
}

// MemFile
MemFile MemFile_Initialize() {
	return (MemFile) { 0 };
}

void MemFile_Malloc(MemFile* memFile, u32 size) {
	memset(memFile, 0, sizeof(MemFile));
	memFile->data = malloc(size);
	memset(memFile->data, 0, size);
	
	if (memFile->data == NULL) {
		printf_warning("Failed to malloc [0x%X] bytes.", size);
	}
	
	memFile->memSize = size;
}

void MemFile_Realloc(MemFile* memFile, u32 size) {
	printf_debugExt("memSize:", memFile->memSize);
	printf_debug("reqSize", size);
	// Make sure to have enough space
	if (size < memFile->memSize + 0x10000) {
		size += 0x10000;
	}
	
	memFile->data = realloc(memFile->data, size);
	memFile->memSize = size;
	printf_debug("newSize", size);
}

void MemFile_Rewind(MemFile* memFile) {
	memFile->seekPoint = 0;
}

s32 MemFile_Write(MemFile* dest, void* src, u32 size) {
	if (dest->seekPoint + size > dest->memSize) {
		printf_warning("DataSize exceeded MemSize while writing to MemFile.\n\tDataSize: [0x%X]\n\tMemSize: [0x%X]", dest->dataSize, dest->memSize);
		
		return 1;
	}
	if (dest->seekPoint + size > dest->dataSize) {
		dest->dataSize = dest->seekPoint + size;
	}
	memcpy(&dest->cast.u8[dest->seekPoint], src, size);
	dest->seekPoint += size;
	
	return 0;
}

s32 MemFile_Read(MemFile* src, void* dest, u32 size) {
	if (src->seekPoint + size > src->dataSize) {
		OsPrintfEx("Extended dataSize");
		
		return 1;
	}
	
	memcpy(dest, &src->cast.u8[src->seekPoint], size);
	src->seekPoint += size;
	
	return 0;
}

s32 MemFile_LoadFile(MemFile* memFile, char* filepath) {
	u32 tempSize;
	FILE* file = fopen(filepath, "rb");
	struct stat sta;
	
	if (file == NULL) {
		printf_warning("Failed to fopen file [%s].", filepath);
		
		return 1;
	}
	
	fseek(file, 0, SEEK_END);
	tempSize = ftell(file);
	
	if (memFile->data == NULL) {
		MemFile_Malloc(memFile, tempSize);
		memFile->memSize = memFile->dataSize = tempSize;
		if (memFile->data == NULL) {
			printf_warning("Failed to malloc MemFile.\n\tAttempted size is [0x%X] bytes to store data from [%s].", tempSize, filepath);
			
			return 1;
		}
	} else if (memFile->memSize < tempSize) {
		MemFile_Realloc(memFile, tempSize);
	}
	memFile->dataSize = tempSize;
	
	rewind(file);
	fread(memFile->data, 1, memFile->dataSize, file);
	fclose(file);
	stat(filepath, &sta);
	memFile->info.age = sta.st_mtime;
	if (memFile->info.name)
		free(memFile->info.name);
	memFile->info.name = String_Generate(filepath);
	
	printf_debugExt("File: [%s]", filepath);
	printf_debug("Ptr: %08X", memFile->data);
	printf_debug("Size: %08X", memFile->dataSize);
	
	return 0;
}

s32 MemFile_LoadFile_String(MemFile* memFile, char* filepath) {
	u32 tempSize;
	FILE* file = fopen(filepath, "r");
	struct stat sta;
	
	if (file == NULL) {
		printf_warning("Failed to fopen file [%s].", filepath);
		
		return 1;
	}
	
	fseek(file, 0, SEEK_END);
	tempSize = ftell(file);
	
	if (memFile->data == NULL) {
		MemFile_Malloc(memFile, tempSize);
		memFile->memSize = memFile->dataSize = tempSize;
		if (memFile->data == NULL) {
			printf_warning("Failed to malloc MemFile.\n\tAttempted size is [0x%X] bytes to store data from [%s].", tempSize, filepath);
			
			return 1;
		}
	} else if (memFile->memSize < tempSize) {
		MemFile_Realloc(memFile, tempSize);
	}
	memFile->dataSize = tempSize;
	
	rewind(file);
	fread(memFile->data, 1, memFile->dataSize, file);
	fclose(file);
	stat(filepath, &sta);
	memFile->info.age = sta.st_mtime;
	if (memFile->info.name)
		free(memFile->info.name);
	memFile->info.name = String_Generate(filepath);
	
	printf_debugExt("File: [%s]", filepath);
	printf_debug("Ptr: %08X", memFile->data);
	printf_debug("Size: %08X", memFile->dataSize);
	
	return 0;
}

s32 MemFile_SaveFile(MemFile* memFile, char* filepath) {
	FILE* file = fopen(filepath, "wb");
	
	if (file == NULL) {
		printf_warning("Failed to fopen file [%s] for writing.", filepath);
		
		return 1;
	}
	
	fwrite(memFile->data, sizeof(u8), memFile->dataSize, file);
	fclose(file);
	
	return 0;
}

s32 MemFile_SaveFile_String(MemFile* memFile, char* filepath) {
	FILE* file = fopen(filepath, "w");
	
	if (file == NULL) {
		printf_warning("Failed to fopen file [%s] for writing.", filepath);
		
		return 1;
	}
	
	fwrite(memFile->data, sizeof(u8), memFile->dataSize, file);
	fclose(file);
	
	return 0;
}

s32 MemFile_LoadFile_ReqExt(MemFile* memFile, char* filepath, const char* ext) {
	if (Lib_MemMem(filepath, strlen(filepath), ext, strlen(ext))) {
		return MemFile_LoadFile(memFile, filepath);
	}
	printf_warning("[%s] does not match extension [%s]", filepath, ext);
	
	return 1;
}

s32 MemFile_SaveFile_ReqExt(MemFile* memFile, char* filepath, s32 size, const char* ext) {
	if (Lib_MemMem(filepath, strlen(filepath), ext, strlen(ext))) {
		return MemFile_SaveFile(memFile, filepath);
	}
	
	printf_warning("[%s] does not match extension [%s]", filepath, ext);
	
	return 1;
}

void MemFile_Free(MemFile* memFile) {
	if (memFile->data) {
		if (memFile->info.name)
			free(memFile->info.name);
		free(memFile->data);
		
		memset(memFile, 0, sizeof(MemFile));
	}
}

#define __EXT_STR_MAX 32
// String
u32 String_HexStrToInt(char* string) {
	return strtol(string, NULL, 16);
}

u32 String_NumStrToInt(char* string) {
	return strtol(string, NULL, 10);
}

f64 String_NumStrToF64(char* string) {
	return strtod(string, NULL);
}

s32 String_GetLineCount(char* str) {
	s32 line = 1;
	s32 i = 0;
	
	while (str[i++] != '\0') {
		if (str[i] == '\n' && str[i + 1] != '\0')
			line++;
	}
	
	return line;
}

char* String_GetLine(char* str, s32 line) {
	#define __EXT_BUFFER(a) buffer[Wrap(index + a, 0, __EXT_STR_MAX - 1)]
	static char* buffer[__EXT_STR_MAX];
	static s32 index;
	s32 iLine = -1;
	s32 i = 0;
	s32 j = 0;
	char* ret;
	
	while (str[i] != '\0') {
		j = 0;
		if (str[i] != '\n') {
			while (str[i + j] != '\n' && str[i + j] != '\0') {
				j++;
			}
			
			iLine++;
			
			if (iLine == line) {
				break;
			}
			
			i += j;
		} else {
			i++;
		}
	}
	
	__EXT_BUFFER(0) = Lib_Calloc(0, j * 2);
	memcpy(__EXT_BUFFER(0), &str[i], j);
	Lib_Free(__EXT_BUFFER(-(__EXT_STR_MAX / 2)));
	ret = __EXT_BUFFER(0);
	index = Wrap(index + 1, 0, __EXT_STR_MAX - 1);
	
	return ret;
	#undef __EXT_BUFFER
}

char* String_GetWord(char* str, s32 word) {
	#define __EXT_BUFFER(a) buffer[Wrap(index + a, 0, __EXT_STR_MAX - 1)]
	static char* buffer[__EXT_STR_MAX];
	static s32 index;
	s32 iWord = -1;
	s32 i = 0;
	s32 j = 0;
	char* ret;
	
	while (str[i] != '\0') {
		j = 0;
		if (str[i + j] > ' ') {
			while (str[i + j] > ' ') {
				j++;
			}
			
			iWord++;
			
			if (iWord == word) {
				break;
			}
			
			i += j;
		} else {
			i++;
		}
	}
	
	__EXT_BUFFER(0) = Lib_Calloc(0, j * 2);
	memcpy(__EXT_BUFFER(0), &str[i], j);
	Lib_Free(__EXT_BUFFER(-(__EXT_STR_MAX / 2)));
	ret = __EXT_BUFFER(0);
	index = Wrap(index + 1, 0, __EXT_STR_MAX - 1);
	
	return ret;
	#undef __EXT_BUFFER
}

char* String_Line(char* str, s32 line) {
	s32 iLine = -1;
	s32 i = 0;
	s32 j = 0;
	
	while (str[i] != '\0') {
		j = 0;
		if (str[i] != '\n') {
			while (str[i + j] != '\n' && str[i + j] != '\0') {
				j++;
			}
			
			iLine++;
			
			if (iLine == line) {
				break;
			}
			
			i += j;
		} else {
			i++;
		}
	}
	
	return &str[i];
}

char* String_Word(char* str, s32 word) {
	s32 iWord = -1;
	s32 i = 0;
	s32 j = 0;
	
	while (str[i] != '\0') {
		j = 0;
		if (str[i + j] > ' ') {
			while (str[i + j] > ' ') {
				j++;
			}
			
			iWord++;
			
			if (iWord == word) {
				break;
			}
			
			i += j;
		} else {
			i++;
		}
	}
	
	return &str[i];
}

void String_GetLine2(char* dest, char* str, s32 line) {
	s32 iLine = -1;
	s32 i = 0;
	s32 j = 0;
	
	while (str[i] != '\0') {
		j = 0;
		if (str[i] != '\n') {
			while (str[i + j] != '\n' && str[i + j] != '\0') {
				j++;
			}
			
			iLine++;
			
			if (iLine == line) {
				break;
			}
			
			i += j;
		} else {
			i++;
		}
	}
	
	memcpy(dest, &str[i], j);
}

void String_GetWord2(char* dest, char* str, s32 word) {
	s32 iWord = -1;
	s32 i = 0;
	s32 j = 0;
	
	while (str[i] != '\0') {
		j = 0;
		if (str[i] != ' ' && str[i] != '\t') {
			while (str[i + j] != ' ' && str[i + j] != '\t' && str[i + j] != '\0') {
				j++;
			}
			
			iWord++;
			
			if (iWord == word) {
				break;
			}
			
			i += j;
		} else {
			i++;
		}
	}
	
	memcpy(dest, &str[i], j);
}

void String_CaseToLow(char* s, s32 i) {
	for (s32 k = 0; k < i; k++) {
		if (s[k] >= 'A' && s[k] <= 'Z') {
			s[k] = s[k] + 32;
		}
	}
}

void String_CaseToUp(char* s, s32 i) {
	for (s32 k = 0; k < i; k++) {
		if (s[k] >= 'a' && s[k] <= 'z') {
			s[k] = s[k] - 32;
		}
	}
}

void String_GetSlashAndPoint(char* src, s32* slash, s32* point) {
	s32 strSize = strlen(src);
	
	for (s32 i = strSize; i > 0; i--) {
		if (*point == 0 && src[i] == '.') {
			*point = i;
		}
		if (src[i] == '/' || src[i] == '\\') {
			*slash = i;
			break;
		}
	}
}

char* String_GetPath(char* src) {
	#define __EXT_BUFFER(a) buffer[Wrap(index + a, 0, __EXT_STR_MAX - 1)]
	static char* buffer[__EXT_STR_MAX];
	static s32 index;
	s32 point = 0;
	s32 slash = 0;
	char* ret;
	
	String_GetSlashAndPoint(src, &slash, &point);
	
	if (slash == 0)
		slash = -1;
	
	__EXT_BUFFER(0) = Lib_Calloc(0, slash + 0x10);
	memset(__EXT_BUFFER(0), 0, slash + 2);
	memcpy(__EXT_BUFFER(0), src, slash + 1);
	Lib_Free(__EXT_BUFFER(-(__EXT_STR_MAX / 2)));
	ret = __EXT_BUFFER(0);
	index = Wrap(index + 1, 0, __EXT_STR_MAX - 1);
	
	return ret;
}

char* String_GetBasename(char* src) {
	#define __EXT_BUFFER(a) buffer[Wrap(index + a, 0, __EXT_STR_MAX - 1)]
	static char* buffer[__EXT_STR_MAX];
	static s32 index;
	s32 point = 0;
	s32 slash = 0;
	char* ret;
	
	String_GetSlashAndPoint(src, &slash, &point);
	
	if (slash == 0)
		slash = -1;
	
	__EXT_BUFFER(0) = Lib_Calloc(0, slash + 0x10);
	memset(__EXT_BUFFER(0), 0, point - slash);
	memcpy(__EXT_BUFFER(0), &src[slash + 1], point - slash - 1);
	Lib_Free(__EXT_BUFFER(-(__EXT_STR_MAX / 2)));
	ret = __EXT_BUFFER(0);
	index = Wrap(index + 1, 0, __EXT_STR_MAX - 1);
	
	return ret;
}

char* String_GetFilename(char* src) {
	#define __EXT_BUFFER(a) buffer[Wrap(index + a, 0, __EXT_STR_MAX - 1)]
	static char* buffer[__EXT_STR_MAX];
	static s32 index;
	s32 point = 0;
	s32 slash = 0;
	char* ret;
	
	String_GetSlashAndPoint(src, &slash, &point);
	
	if (slash == 0)
		slash = -1;
	
	__EXT_BUFFER(0) = Lib_Calloc(0, slash + 0x10);
	memset(__EXT_BUFFER(0), 0, strlen(src) - slash);
	memcpy(__EXT_BUFFER(0), &src[slash + 1], strlen(src) - slash - 1);
	Lib_Free(__EXT_BUFFER(-(__EXT_STR_MAX / 2)));
	ret = __EXT_BUFFER(0);
	index = Wrap(index + 1, 0, __EXT_STR_MAX - 1);
	
	return ret;
	
	#undef __EXT_BUFFER
}

void String_Insert(char* point, char* insert) {
	s32 insLen = strlen(insert);
	char* insEnd = point + insLen;
	s32 remLen = strlen(point);
	
	memmove(insEnd, point, remLen);
	insEnd[remLen] = 0;
	memcpy(point, insert, insLen);
}

void String_Remove(char* point, s32 amount) {
	char* get = point + amount;
	s32 len = strlen(get);
	
	memcpy(point, get, strlen(get));
	point[len] = 0;
}

void String_SwapExtension(char* dest, char* src, const char* ext) {
	String_Copy(dest, String_GetPath(src));
	String_Merge(dest, String_GetBasename(src));
	String_Merge(dest, ext);
}

char* String_GetSpacedArg(char* argv[], s32 cur) {
	#define __EXT_BUFFER(a) buffer[Wrap(index + a, 0, __EXT_STR_MAX - 1)]
	static char* buffer[__EXT_STR_MAX];
	char tempBuf[1024];
	static s32 index;
	s32 i = cur + 1;
	
	if (argv[cur + 1] != NULL && argv[cur + 1][0] != '-') {
		if (__EXT_BUFFER(0) != NULL)
			free(__EXT_BUFFER(0));
		
		String_Copy(tempBuf, argv[cur]);
		
		while (argv[i] != NULL && argv[i][0] != '-') {
			String_Merge(tempBuf, " ");
			String_Merge(tempBuf, argv[i++]);
		}
		
		__EXT_BUFFER(0) = String_Generate(tempBuf);
		index++;
		
		return __EXT_BUFFER(-1);
	}
	
	return argv[cur];
	
	#undef __EXT_BUFFER
}