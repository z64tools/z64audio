#include "AudioConvert.h"

u32 Audio_BSwapFloat80(f80* float80) {
    u8* byteReverse = (void*)float80;
    u8 bswap[10];
    for (s32 i = 0; i < 10; i++)
        bswap[i] = byteReverse[ 9 - i];
    
    for (s32 i = 0; i < 10; i++)
        ((u8*)float80)[i] = bswap[i];
}