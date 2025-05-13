void start_up(void) __naked {
    __asm__("di");
    __asm__("jp _reset");
}

typedef signed char int8;
typedef signed short int16;
typedef unsigned char byte;
typedef unsigned short word;

#define false		0
#define true		1

#define NULL		((void *) 0)
#define ADDR(obj)	((word) (obj))
#define BYTE(addr)	(* (volatile byte *) (addr))
#define WORD(addr)	(* (volatile word *) (addr))
#define SIZE(array)	(sizeof(array) / sizeof(*(array)))
#define PTR(addr)	((byte *) (addr))

#define FRAME(x)	((x) << 5)
#define POS(x, y)	((byte) ((x) + ((y) << 4)))

static const struct Room *room;
static volatile byte vblank;
static byte *map_y[192];

static const void *respawn;
static byte spawn_pos;

#if defined(ZXS)
#define SETUP_STACK()	__asm__("ld sp, #0xfdfc")
#define SCREEN(x)	PTR(0x4000 + (x))
#define COLOUR(x)	PTR(0x5800 + (x))
#define STAGING_AREA	PTR(0x5b00)
#define FONT_ADDRESS	PTR(0x3c00)
#define IRQ_BASE	0xfe00
#endif

#define INK		(STAGING_AREA)
#define LEVEL		(INK + 0x260)
#define  TILE(id)		(id << 2)
#define  SET(id)		(id << 5)
#define  RIGHT			0
#define  LEFT			1
#define EMPTY		(LEVEL + 0xc0)
#define MOB(n)		(EMPTY + FRAME(64 + 8 * n))
#define  MOVING			0
#define  STANCE			1
#define  ATTACK			2
#define  BEATEN			4
#define  KILLED			5
#define  RESTED			6

#define	CTRL_FIRE	0x10
#define	CTRL_UP		0x08
#define	CTRL_DOWN	0x04
#define	CTRL_LEFT	0x02
#define	CTRL_RIGHT	0x01

#define MOVE_STAMINA	2
#define FULL_STAMINA	48

enum { X = 0, Y = 1 };

static void thud_sound(void);
static void clear_message(void);
static void game_idle(byte ticks);
static void beep(word p, word len);
static byte is_walkable(byte place);
static byte consume_stamina(byte amount);
static void show_message(const char *msg);
static byte load_room(const void *ptr, byte pos);
static byte bump_msg(const void *text, byte ignore);
static void draw_tile(byte *ptr, byte pos, byte id);
static void *decompress(byte *dst, const byte *src);
static void memcpy(void *dst, const void *src, word len);

static byte bump_msg(const void *text, byte value) {
    show_message(text);
    return value;
}

#include "data.h"
#include "mobs.h"
#include "room.h"

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

static void __sdcc_call_iy(void) __naked {
    __asm__("jp (iy)");
}

static void __sdcc_call_hl(void) __naked {
    __asm__("jp (hl)");
}

static void memset(byte *ptr, byte data, word len) {
    while (len-- > 0) { *ptr++ = data; }
}

static void memcpy(void *dst, const void *src, word len) __naked {
    __asm__("ex de, hl");
    __asm__("pop iy");
    __asm__("pop bc");
    __asm__("ld a, c");
    __asm__("or a, b");
    __asm__("jr Z, done");
    __asm__("push de");
    __asm__("ldir");
    __asm__("pop de");
    __asm__("done:");
    __asm__("jp (iy)");
    dst; src; len;
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

static void clear_block(byte y, byte h) {
    byte **row = map_y + y;
    for (byte i = 0; i < h; i++) {
	memset(*row++, 0, 32);
    }
}

static void clear_screen(void) {
#if defined(ZXS)
    memset(COLOUR(0), 0x00, 0x300);
    clear_block(0, 192);
    out_fe(0);
#endif
}

static void beep(word p, word len) {
    word c = 0;
    while (len-- != 0) {
	out_fe((c >> 11) & 0x10);
	c += p;
    }
    out_fe(0x00);
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

static byte has_message;

static void clear_message(void) {
    if (has_message) {
	clear_block(20, 8);
	has_message = 0;
    }
}

static void show_message(const char *msg) {
    clear_message();
    byte x = str_offset(msg, 128);
    put_str(msg, x, 20);
    has_message = 1;
}

static void *decompress(byte *dst, const byte *src) {
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
    return dst;
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
    put_str("not dying?", 176, 48);
    memset(COLOUR(0xc0), 4, 0x240);
    put_str("Game by Snauts", 0, 184);
    put_str("1 - QAOP+Space", 81, 104);
    put_str("2 - Joystick", 80, 120);
    wait_1_or_2();
}

static byte stamina;
static byte slider;

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

static byte flip_bits(byte source) {
    byte result = 0;
    for (byte i = 0; i < 8; i++) {
	result = result << 1;
	result |= source & 1;
	source = source >> 1;
    }
    return result;
}

static void draw_tile(byte *ptr, byte pos, byte id) {
    byte x = (pos & 0x0f) << 1;
    byte y = (pos & 0xf0);
    byte flip_h = id & 1;
    byte flip_v = id & 2;
    if (flip_v) y += 15;
    ptr += (id & ~0x3) << 3;
    for (byte i = 0; i < 16; i++) {
	byte b1 = *ptr++;
	byte b2 = *ptr++;
	if (flip_h) {
	    byte tmp = b1;
	    b1 = flip_bits(b2);
	    b2 = flip_bits(tmp);
	}
	byte *where = map_y[y] + x;
	*where++ = b1;
	*where++ = b2;
	if (flip_v) y--; else y++;
    }
}

static void horizontal_line(byte y) {
    memset(COLOUR((y & ~7) << 2), 5, 32);
    memset(map_y[y], 0xff, 32);
}

static void reload(void) {
    reset_mobs();
    load_room(respawn, spawn_pos);
    replenish_stamina(FULL_STAMINA);    
}

static void wait_space(void) {
    while (!(input_change(read_input()) & CTRL_FIRE)) { }
}

static void you_died(void) {
    memset(COLOUR(0x120), 0, 0xc0);
    clear_block(72, 56);
    show_block(title, 80, 40);
    horizontal_line(127);
    horizontal_line(72);
    wait_space();
    reload();
}

static void place_richard(byte pos) {
    player.ink = 5;
    player.pos = pos;
    update_image(&player, TILE(MOVING));
}

static byte is_walkable(byte place) {
    return LEVEL[place] == TILE(1);
}

static byte invoke_bump(Caller fn, void *ptr, byte arg) {
    return fn(ptr, arg);
}

static byte activate_bumps(int8 delta) {
    const struct Bump *bump = room->bump;
    byte count = room->count;
    while (count-- > 0) {
	if (player.pos == bump->pos && delta == bump->delta) {
	    /*
	     * there seems to be compiler bug:
	     * bump->fn(bump->ptr, bump->arg)
	     * fails to pass bump->ptr correctly
	     */
	    if (invoke_bump(bump->fn, bump->ptr, bump->arg)) return true;
	}
	bump++;
    }
    return false;
}

static byte push_corpse(struct Mob *mob, int8 delta) {
    byte target = mob->pos + delta;
    if (stamina >= MOVE_STAMINA && !is_occupied(target)) {
	push_mob(mob, target);
    }
    return true;
}

static byte move_corpse(struct Mob *mob, int8 delta) {
    return mob == NULL || (is_dead(mob) && push_corpse(mob, delta));
}

static byte should_attack(struct Mob *mob) {
    return mob != NULL && !is_dead(mob) && consume_stamina(FULL_STAMINA / 2);
}

static byte should_move(struct Mob *mob, byte target, int8 delta) {
    return is_walkable(target)
	&& move_corpse(mob, delta)
	&& consume_stamina(MOVE_STAMINA);
}

static void thud_sound(void) {
    for (byte p = 250; p > 50; p -= 50) beep(p << 1, 200);
}

static void roll_richard(int8 delta) {
    mob_direction(&player, delta);
    byte target = player.pos + delta;
    struct Mob *mob = is_mob(target);
    if (should_attack(mob)) {
	animate_attack(&player, mob);
	shamble_mobs();
    }
    else if (should_move(mob, target, delta)) {
	move_mob(&player, target);
	shamble_mobs();
    }
    else if (!activate_bumps(delta) && stamina == 0) {
	show_message("You feel exausted");
	beep(200, 1000);
	beep(500, 500);
    }
}

static void rest_richard(void) {
    update_image(&player, TILE(RESTED));
    replenish_stamina(FULL_STAMINA);
}

static void move_richard(void) {
    byte change = input_change(read_input());

    if (change & CTRL_FIRE) {
	rest_richard();
	shamble_mobs();
    }
    else if (change & CTRL_UP) {
	roll_richard(-16);
    }
    else if (change & CTRL_DOWN) {
	roll_richard(16);
    }
    else if (change & CTRL_LEFT) {
	roll_richard(-1);
    }
    else if (change & CTRL_RIGHT) {
	roll_richard(1);
    }

    if (is_dead(&player)) {
	game_idle(25);
	you_died();
    }
}

static void game_loop(void) {
    for (;;) {
	move_richard();
	game_idle(1);
    }
}

static byte load_room(const void *new_room, byte pos) {
    /* clear previous */
    const void *from = room;
    memset(COLOUR(0x80), 0, 0x280);
    clear_block(32, 160);
    reset_actors();

    /* unpack data */
    room = new_room;
    const void **map = room->map;
    decompress(INK, *map++);

    /* build tileset */
    void *dst = EMPTY;
    while (*map) dst = decompress(dst, *map++);

    if (room->setup) room->setup();

    for (byte pos = 32; pos < 192; pos++) {
	draw_tile(EMPTY, pos, LEVEL[pos]);
    }

    memcpy(COLOUR(0x80), INK, 0x280);

    place_actors();
    show_message(room->msg);

    struct Mob *block = is_mob(pos);
    if (!block || is_dead(block)) {
	place_richard(pos);
    }
    else {
	game_idle(50);
	load_room(from, player.pos);
	show_message("Path is blocked");
    }
    return true;
}

static void start_game(void) {
    has_message = 0;
    player.img = SET(0);
    door_broken = false;
    show_block(bar, 0, 24);
    last_input = read_input();
    memset(COLOUR(96), 0x5, 32);
    stamina = slider = FULL_STAMINA;
    decompress(MOB(0), richard);
    spawn_pos = POS(6, 6);
    respawn = &prison;
    reload();

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
