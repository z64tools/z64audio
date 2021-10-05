#include "HermosauhuLib.h"

static PrintfSuppressLevel sPrintfSuppress = 0;

void Lib_SetPrinfSuppressLevel(PrintfSuppressLevel lvl) {
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

s32 Lib_Str_GetLineCount(char* str) {
	s32 line = 1;
	s32 i = 0;
	
	while (str[i++] != '\0') {
		if (str[i] == '\n' && str[i + 1] != '\0')
			line++;
	}
	
	return line;
}

u32 Lib_Str_HexToInt(char* string) {
	return strtol(string, NULL, 16);
}

char* Lib_Str_GetLine(char* str, s32 line) {
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

char* Lib_Str_GetWord(char* str, s32 word) {
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

void Lib_Str_CaseToLow(char* s, s32 i) {
	for (s32 k = 0; k < i; k++) {
		if (s[k] >= 'A' && s[k] <= 'Z') {
			s[k] = s[k] + 32;
		}
	}
}

void Lib_Str_CaseToUpper(char* s, s32 i) {
	for (s32 k = 0; k < i; k++) {
		if (s[k] >= 'a' && s[k] <= 'z') {
			s[k] = s[k] - 32;
		}
	}
}

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

void* Lib_GetFile(char* filename, int* size) {
	void* data = NULL;
	FILE* file = fopen(filename, "r");
	
	if (file == NULL) {
		printf_error("Lib_GetFile: Failed to fopen file [%s].", filename);
	}
	
	fseek(file, 0, SEEK_END);
	*size = ftell(file);
	data = malloc(*size);
	if (data == NULL) {
		printf_error("Lib_GetFile: Failed to malloc 0x%X bytes to store data from [%s].", *size, filename);
	}
	rewind(file);
	fread(data, sizeof(char), *size, file);
	fclose(file);
	
	return data;
}

void Lib_Str_GetFilenameWithExt(char* dst, char* str) {
	char buffer[1024];
	int i = 0;
	int j = 0;
	int strLen = strlen(str);
	
	bzero(buffer, 1024);
	
	while (str[strLen - i - 1] != '\\' && str[strLen - i - 1] != '/')
		i++;
	
	while (i != 0) {
		buffer[j] = str[strLen - i];
		j++;
		i--;
	}
	
	memmove(dst, buffer, strlen(buffer));
}

void Lib_Str_GetFilePathAndName(char* _src, char* _dest, char* _path) {
	char temporName[2048] = { 0 };
	
	strcpy(temporName, _src);
	s32 sizeOfName = 0;
	s32 slashPoint = 0;
	
	s32 extensionPoint = 0;
	
	while (temporName[sizeOfName] != 0)
		sizeOfName++;
	
	while (temporName[sizeOfName - extensionPoint] != '.') {
		extensionPoint++;
		if (temporName[sizeOfName - extensionPoint] != '\\' && temporName[sizeOfName - extensionPoint] != '/' && extensionPoint < sizeOfName) {
			extensionPoint = 0;
			break;
		}
	}
	
	for (s32 i = 0; i < sizeOfName; i++) {
		if (temporName[i] == '/' || temporName[i] == '\\')
			slashPoint = i;
	}
	
	if (slashPoint != 0)
		slashPoint++;
	else if (_path != NULL)
		_path[0] = 0;
	
	for (s32 i = 0; i <= sizeOfName - slashPoint - 1 - extensionPoint; i++) {
		_dest[i] = temporName[slashPoint + i];
		_dest[i + 1] = '\0';
	}
	
	if (_path != NULL)
		for (s32 i = 0; i < slashPoint; i++) {
			_path[i] = _src[i];
			_path[i + 1] = '\0';
		}
}