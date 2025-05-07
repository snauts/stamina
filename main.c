void start_up(void) __naked {
    __asm__("di");
    __asm__("jp _reset");
}

typedef signed char int8;
typedef signed short int16;
typedef unsigned char byte;
typedef unsigned short word;

#include "data.h"

#define NULL		((void *) 0)
#define ADDR(obj)	((word) (obj))
#define BYTE(addr)	(* (volatile byte *) (addr))
#define WORD(addr)	(* (volatile word *) (addr))
#define SIZE(array)	(sizeof(array) / sizeof(*(array)))
#define PTR(addr)	((void *) (addr))

static byte *map_y[192];

#if defined(ZXS)
#define SETUP_STACK()	__asm__("ld sp, #0xfdfc")
#define SCREEN(x)	PTR(0x4000 + (x))
#define COLOUR(x)	PTR(0x5800 + (x))
#define STAGING_AREA	COLOUR(0x300)
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

static void precalculate(void) {
#if defined(ZXS)
    for (byte y = 0; y < 192; y++) {
	byte f = ((y & 7) << 3) | ((y >> 3) & 7) | (y & 0xc0);
	map_y[y] = SCREEN(f << 5);
    }
#endif
}

static void clear_screen(void) {
#if defined(ZXS)
    memset(COLOUR(0), 0x00, 0x300);
    memset(SCREEN(0), 0x00, 0x1800);
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

static void show_block(const void *src, byte y, byte n) {
    byte *ptr = STAGING_AREA;
    decompress(ptr, src);
    for (byte i = 0; i < n; i++) {
	memcpy(map_y[y + i], ptr, 32);
	ptr += 32;
    }
    memcpy(COLOUR(y << 2), ptr, n << 2);
}

static void show_title(void) {
    show_block(title, 64, 64);
}

void reset(void) {
    SETUP_STACK();
    clear_screen();
    precalculate();
    show_title();
    for (;;) { }
}
