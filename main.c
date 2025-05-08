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
#define PTR(addr)	((byte *) (addr))

static volatile byte vblank;
static byte *map_y[192];

#if defined(ZXS)
#define SETUP_STACK()	__asm__("ld sp, #0xfdfc")
#define SCREEN(x)	PTR(0x4000 + (x))
#define COLOUR(x)	PTR(0x5800 + (x))
#define STAGING_AREA	PTR(0x5b00)
#define FONT_ADDRESS	PTR(0x3c00)
#define IRQ_BASE	0xfe00
#endif

#define FRAME(x)	((x) << 5)

#define EMPTY		(STAGING_AREA + FRAME(0))
#define RICHARD(x)	(STAGING_AREA + FRAME(1 + (x)))

#define	CTRL_FIRE	0x10
#define	CTRL_UP		0x08
#define	CTRL_DOWN	0x04
#define	CTRL_LEFT	0x02
#define	CTRL_RIGHT	0x01

enum { X = 0, Y = 1 };

static void interrupt(void) __naked {
    __asm__("di");
    __asm__("push af");

    __asm__("ld a, #1");
    __asm__("ld (_vblank), a");

    __asm__("pop af");
    __asm__("ei");
    __asm__("reti");
}

static void setup_irq(byte base) {
    __asm__("di");
    __asm__("ld i, a"); base;
    __asm__("im 2");
    __asm__("ei");
}

static void wait_vblank(void) {
    vblank = 0;
    while (!vblank) { }
}

static byte in_key(byte a) {
#ifdef ZXS
    __asm__("in a, (#0xfe)");
#endif
    return a;
}

static byte in_joy(byte a) {
#ifdef ZXS
    __asm__("in a, (#0x1f)"); a;
#endif
    return a;
}

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

static void setup_system(void) {
    byte top = (byte) ((IRQ_BASE >> 8) - 1);
    word jmp_addr = (top << 8) | top;
    BYTE(jmp_addr + 0) = 0xc3;
    WORD(jmp_addr + 1) = ADDR(&interrupt);
    memset((byte *) IRQ_BASE, top, 0x101);
    setup_irq(IRQ_BASE >> 8);
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

static void put_char(char symbol, byte x, byte y) {
    byte shift = x & 7;
    byte offset = x >> 3;
    byte *addr = FONT_ADDRESS + (symbol << 3);
    for (byte i = 0; i < 8; i++) {
	byte data = *addr++;
	byte *ptr = map_y[y + i] + offset;
	ptr[0] |= (data >> shift);
	ptr[1] |= (data << (8 - shift));
    }
}

static byte char_mask(char symbol) {
    byte mask = 0;
    byte *addr = FONT_ADDRESS + (symbol << 3);
    for (byte i = 0; i < 8; i++) {
	mask |= *addr++;
    }
    return mask;
}

static byte leading(char symbol) {
    byte i;
    byte mask = char_mask(symbol);
    for (i = 0; i < 8; i++) {
	if (mask & 0x80) goto done;
	mask = mask << 1;
    }
  done:
    return i - 1;
}

static byte trailing(char symbol) {
    byte i;
    byte mask = char_mask(symbol);
    for (i = 0; i < 8; i++) {
	if (mask & 1) goto done;
	mask = mask >> 1;
    }
  done:
    return 8 - i;
}

static void put_str(const char *msg, byte x, byte y) {
    while (*msg != 0) {
	char symbol = *(msg++);
	if (symbol == ' ') {
	    x = x + 4;
	}
	else {
	    byte lead = leading(symbol);
	    if (lead <= x) x -= lead;
	    put_char(symbol, x, y);
	    x += trailing(symbol);
	}
    }
}

static byte str_len(const char *msg) {
    byte len = 0;
    while (*msg != 0) {
	char symbol = *(msg++);
	len += symbol == ' ' ? 4 : trailing(symbol) - leading(symbol);
    }
    return len;
}

static byte str_offset(const char *msg, byte from) {
    return from - (str_len(msg) >> 1);
}

static void show_msg(const char *msg) {
    put_str(msg, str_offset(msg, 128), 20);
}

static void clear_msg(void) {
    for (byte y = 20; y < 28; y++) memset(map_y[y], 0, 32);
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

static byte read_1_or_2(void) {
    return ~in_key(0xf7) & 3;
}

static byte use_joy;
static byte last_input;

static byte input_change(byte input) {
    byte change = input & (input ^ last_input);
    last_input = input;
    return change;
}

static void wait_1_or_2(void) {
    last_input = read_1_or_2();
    while (!input_change(read_1_or_2())) { }
    use_joy = last_input & 2;
}

static void show_title(void) {
    put_str("Have", 12, 0);
    memset(COLOUR(0x00), 4, 0x20);
    show_block(title, 8, 40);
    put_str("Stamina?", 192, 48);
    memset(COLOUR(0xc0), 4, 0x240);
    put_str("Game by Snauts", 0, 184);
    put_str("1 - QAOP+Space", 81, 104);
    put_str("2 - Joystick", 80, 120);
    wait_1_or_2();
}

#define FULL_STAMINA 48

static byte stamina;
static byte slider;
static byte richard_pos[2];

static void stamina_bar_update(byte pos, byte update) {
    BYTE(COLOUR(36 + (pos >> 1))) = (slider & 1) ? 0x25 : update;
}

static void update_bar(void) {
    if (slider < stamina) {
	stamina_bar_update(slider++, 0x65);
    }
    if (slider > stamina) {
	stamina_bar_update(--slider, 0x00);
    }
}

static void game_idle(byte ticks) {
    for (byte i = 0; i < ticks; i++) {
	wait_vblank();
	update_bar();
    }
}

static byte consume_stamina(byte amount) {
    byte enough = (stamina >= amount);
    if (enough) stamina -= amount;
    return enough;
}

static void replenish_stamina(byte amount) {
    stamina = stamina + amount;
    if (stamina > FULL_STAMINA) {
	stamina = FULL_STAMINA;
    }
}

static byte read_QAOP(void) {
    byte ret = 0;
    ret |= (in_key(0x7f) & 1);
    ret <<= 1;
    ret |= (in_key(0xfb) & 1);
    ret <<= 1;
    ret |= (in_key(0xfd) & 1);
    ret <<= 2;
    ret |= (in_key(0xdf) & 3);
    return ~ret;
}

static byte read_input(void) {
    return use_joy ? in_joy(0) : read_QAOP();
}

static void draw_tile(byte *ptr, const byte *pos) {
    byte x = pos[X] >> 3;
    byte y = pos[Y];
    for (byte i = 0; i < 16; i++) {
	byte *where = map_y[y++] + x;
	where[0] = *ptr++;
	where[1] = *ptr++;
    }
}

static void place_richard(byte x, byte y) {
    richard_pos[X] = x;
    richard_pos[Y] = y;
    draw_tile(RICHARD(0), richard_pos);
}

static void move_tile(byte *ptr, byte *pos, int8 dx, int8 dy) {
    draw_tile(EMPTY, pos);
    pos[X] += dx;
    pos[Y] += dy;
    draw_tile(ptr, pos);
}

static void roll_richard(int8 dx, int8 dy) {
    if (consume_stamina(6)) {
	draw_tile(RICHARD(1), richard_pos);
	game_idle(2);
	move_tile(RICHARD(2), richard_pos, dx, dy);
	game_idle(2);
	move_tile(RICHARD(1), richard_pos, dx, dy);
	game_idle(2);
	draw_tile(RICHARD(0), richard_pos);
    }
}

static void move_richard(void) {
    byte change = input_change(read_input());

    if (change & CTRL_FIRE) {
	replenish_stamina(24);
    }
    else if (change & CTRL_UP) {
	roll_richard(0, -8);
    }
    else if (change & CTRL_DOWN) {
	roll_richard(0, 8);
    }
    else if (change & CTRL_LEFT) {
	roll_richard(-8, 0);
    }
    else if (change & CTRL_RIGHT) {
	roll_richard(8, 0);
    }
}

static void game_loop(void) {
    for (;;) {
	move_richard();
	game_idle(1);
    }
}

static void start_game(void) {
    memset(EMPTY, 0, 32);
    show_block(bar, 0, 24);
    last_input = read_input();
    memset(COLOUR(96), 0x5, 32);
    stamina = slider = FULL_STAMINA;

    memset(COLOUR(0x80), 0x1, 0x280);
    decompress(RICHARD(0), richard);
    place_richard(32, 96);

    game_loop();
}

void reset(void) {
    SETUP_STACK();
    setup_system();
    clear_screen();
    precalculate();
    show_title();
    clear_screen();
    start_game();
    start_up();
}
