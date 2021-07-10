#ifndef _Z64AUDIO_PRINT_H_
#define _Z64AUDIO_PRINT_H_

void PrintFail(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
void PrintWarning(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));
void DebugPrint(const char* fmt, ...) __attribute__ ((format (printf, 1, 2)));

#include "z64snd.h"

void PrintFail(const char* fmt, ...) {
	va_list args;
	
	va_start(args, fmt);
	printf(
		"\7\e[0;91m[<]: "
		"Error: \e[m"
	);
	vprintf(
		fmt,
		args
	);
	printf("\n");
	va_end(args);
	exit(EXIT_FAILURE);
}

void PrintWarning(const char* fmt, ...) {
	va_list args;
	
	va_start(args, fmt);
	printf(
		"\7\e[0;93m[<]: "
		"Warning: \e[m"
	);
	vprintf(
		fmt,
		args
	);
	printf("\n");
	va_end(args);
}

void DebugPrint(const char* fmt, ...) {
	static s8 i = 3;
	va_list args;
	
	if (!STATE_DEBUG_PRINT)
		return;
	
	const char colors[][64] = {
		"\e[0;31m[<]: \e[m\0",
		"\e[0;33m[<]: \e[m\0",
		"\e[0;32m[<]: \e[m\0",
		"\e[0;36m[<]: \e[m\0",
		"\e[0;34m[<]: \e[m\0",
		"\e[0;35m[<]: \e[m\0",
	};
	
	i += STATE_FABULOUS ? i < 5 ? 1 : -5 : 0;
	
	va_start(args, fmt);
	printf(colors[i]);
	vprintf(fmt, args);
	printf("\n");
	va_end(args);
}

void ColorPrint(s8 c, const char* fmt, ...) {
	static s8 i = 3;
	va_list args;
	
	if (!STATE_DEBUG_PRINT)
		return;
	
	const char colors[][64] = {
		"\e[0;31m",
		"\e[0;33m",
		"\e[0;32m",
		"\e[0;36m",
		"\e[0;34m",
		"\e[0;35m",
	};
	
	i += STATE_FABULOUS ? i < 5 ? 1 : -5 : 0;
	
	va_start(args, fmt);
	printf("\e[0;36m[<]: \e[m");
	printf(colors[CLAMP(c, 0, 5)]);
	vprintf(fmt, args);
	printf("\e[m");
	printf("\n");
	va_end(args);
}

#endif