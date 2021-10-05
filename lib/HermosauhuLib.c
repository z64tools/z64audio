#include "HermosauhuLib.h"

static PrintfSuppressLevel sPrintfSuppress = 0;

// printf
void printf_SetPrintfSuppressLevel(PrintfSuppressLevel lvl) {
	sPrintfSuppress = lvl;
}

void printf_debug(const char* fmt, ...) {
	if (sPrintfSuppress > PSL_DEBUG)
		return;
	va_list args;
        
	va_start(args, fmt);
	printf("👺🚬 [\e[0;90mdebug\e[m]:\t");
	vprintf(
		fmt,
		args
	);
	printf("\n");
	va_end(args);
}

void printf_warning(const char* fmt, ...) {
	if (sPrintfSuppress >= PSL_NO_WARNING)
		return;
	va_list args;
        
	va_start(args, fmt);
	printf("👺🚬 [\e[0;93mwarning\e[m]:\t");
	vprintf(
		fmt,
		args
	);
	printf("\n");
	va_end(args);
}

void printf_error(const char* fmt, ...) {
	if (sPrintfSuppress < PSL_NO_ERROR) {
		va_list args;
                
		va_start(args, fmt);
		printf("👺🚬 [\e[0;91mERROR\e[m]:\t");
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
	if (sPrintfSuppress >= PSL_NO_INFO)
		return;
	va_list args;
        
	va_start(args, fmt);
	printf("👺🚬 [\e[0;94minfo\e[m]:\t");
	vprintf(
		fmt,
		args
	);
	printf("\n");
	va_end(args);
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

// File
void File_LoadToMem(void** dest, char* src, int* retSize) {
	s32 size;
        
	FILE* file = fopen(src, "r");
        
	if (file == NULL) {
		printf_error("File_LoadToMem: Failed to fopen file [%s].", src);
	}
        
	fseek(file, 0, SEEK_END);
	size = ftell(file);
	if (*dest == NULL) {
		*dest = malloc(size);
		if (*dest == NULL) {
			printf_error("File_LoadToMem: Failed to malloc 0x%X bytes to store data from [%s].", size, src);
		}
	}
	rewind(file);
	fread(*dest, sizeof(char), size, file);
	fclose(file);
        
	if (retSize) {
		*retSize = size;
	}
}

// string
s32 String_GetLineCount(char* str) {
	s32 line = 1;
	s32 i = 0;
        
	while (str[i++] != '\0') {
		if (str[i] == '\n' && str[i + 1] != '\0')
			line++;
	}
        
	return line;
}

u32 String_ToHex(char* string) {
	return strtol(string, NULL, 16);
}

char* String_GetLine(char* str, s32 line) {
	static char buffer[1024] = { 0 };
	s32 iLine = 0;
	s32 i = 0;
	s32 j = 0;
        
	bzero(buffer, 1024);
        
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
        
	memmove(buffer, &str[i], j);
        
	return buffer;
}

char* String_GetWord(char* str, s32 word) {
	static char buffer[1024] = { 0 };
	s32 iWord = 0;
	s32 i = 0;
	s32 j = 0;
        
	bzero(buffer, 1024);
        
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
        
	memmove(buffer, &str[i], j);
        
	return buffer;
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

void String_GetPath(char* dst, char* src) {
	s32 point = 0;
	s32 slash = 0;
        
	String_GetSlashAndPoint(src, &slash, &point);
	bzero(dst, slash + 2);
	memmove(dst, src, slash + 1);
}

void String_GetBasename(char* dst, char* src) {
	s32 point = 0;
	s32 slash = 0;
        
	String_GetSlashAndPoint(src, &slash, &point);
	bzero(dst, point - slash);
	memmove(dst, &src[slash + 1], point - slash - 1);
}

void String_GetFilename(char* dst, char* src) {
	s32 point = 0;
	s32 slash = 0;
        
	String_GetSlashAndPoint(src, &slash, &point);
	bzero(dst, strlen(src) - slash);
	memmove(dst, &src[slash + 1], strlen(src) - slash - 1);
}
