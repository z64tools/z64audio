#ifndef _Z64AUDIO_MACROS_H_
#define _Z64AUDIO_MACROS_H_

#define FILENAME_BUFFER 2048

#define BSWAP16(x)                x = __builtin_bswap16(x);
#define BSWAP32(x)                x = __builtin_bswap32(x);
#define CLAMP(x, min, max)        (x < min ? min : x > max ? max : x)
#define CLAMP_SUBMIN(x, min, max) (x < min ? (min - min) : x > max ? (max - min) : (x - min))
#define BufferPrint(fmt, ...)     snprintf(buffer, sizeof(buffer), fmt, __VA_ARGS__)
#define ABS(a)                    (a < 0 ? -a : a)
#define WIN32_CMD_FIX   char magic[] = { " \0" }; \
	if (system(magic) != 0) \
	PrintFail("Intro has failed.\n")

#endif