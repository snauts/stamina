void start_up(void) __naked {
    __asm__("di");
    __asm__("jp _reset");
}

typedef signed char int8;
typedef signed short int16;
typedef unsigned char byte;
typedef unsigned short word;

#if defined(ZXS)
#define SETUP_STACK()	__asm__("ld sp, #0xfdfc")
#endif

void reset(void) {
    SETUP_STACK();
    for (;;) { }
}
