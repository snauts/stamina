void start_up(void) __naked {
    __asm__("di");
    __asm__("jp _reset");
}

typedef signed char int8;
typedef signed short int16;
typedef unsigned char byte;
typedef unsigned short word;

#include "data.h"

#if defined(ZXS)
#define SETUP_STACK()	__asm__("ld sp, #0xfdfc")
#endif

static void out_fe(byte data) {
    __asm__("out (#0xfe), a"); data;
}

static void __sdcc_call_hl(void) __naked {
    __asm__("jp (hl)");
}

static void memset(byte *ptr, byte data, word len) {
    while (len-- > 0) { *ptr++ = data; }
}

static void memcpy(byte *dst, const byte *src, word len) {
    while (len-- > 0) { *dst++ = *src++; }
}

static void clear_screen(void) {
#if defined(ZXS)
    memset((byte *) 0x5800, 0x00, 0x300);
    memset((byte *) 0x4000, 0x00, 0x1800);
    out_fe(0);
#endif
}

static void decompress(byte *dst, const byte *src) {
    while (*src) {
        byte n = *src & 0x7f;
        if (*(src++) & 0x80) {
	    while (n-- > 0) {
		*dst = *(dst - *src);
		dst++;
	    }
	    src++;
        }
        else {
	    while (n-- > 0) {
		*(dst++) = *(src++);
	    }
        }
    }
}

void reset(void) {
    SETUP_STACK();
    clear_screen();
    for (;;) { }
}
