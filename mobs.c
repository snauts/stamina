#include "main.h"

extern const int8 nearest[];
extern const struct Room *room;

struct Mob player, mobs[TOTAL_MOBS];

#define IMG(set, tile, rest) \
    SET(set) | TILE(tile) | rest

static const struct Mob mobs_reset[TOTAL_MOBS] = {
    /* dungeon */
    { .pos = POS(10, 8), .ink = 0x02, .img = IMG(1, 0, LEFT) },
    { .pos = POS(10, 6), .ink = 0x02, .img = IMG(1, 1, LEFT) },
    { .pos = POS(10, 4), .ink = 0x02, .img = IMG(1, 0, LEFT) },

    /* corridor */
    { .pos = POS(7, 4), .ink = 0x02, .img = IMG(1, 1, LEFT) },
    { .pos = POS(6, 8), .ink = 0x02, .img = IMG(1, 0, LEFT) },
    { .pos = POS(8, 8), .ink = 0x02, .img = IMG(1, 0, LEFT) },

    /* hallway */
    { .pos = POS(10, 9), .ink = 0x02, .img = IMG(1, 4, BOTH),  .var = 4 },
    { .pos = POS( 5, 5), .ink = 0x02, .img = IMG(1, 4, RIGHT), .var = 3 },

    /* bailey */
    { .pos = POS(11, 6), .ink = 0x44, .img = IMG(1, 6, LEFT),  },
    { .pos = POS(11, 8), .ink = 0x44, .img = IMG(1, 6, RIGHT), },
    { .pos = POS( 4, 6), .ink = 0x44, .img = IMG(1, 6, RIGHT), },
    { .pos = POS( 4, 8), .ink = 0x44, .img = IMG(1, 6, LEFT),  },

    /* ramparts */
    { .pos = POS( 1, 5), .ink = 0x05, .img = IMG(1, 0, RIGHT), .var = 0x00 },
    { .pos = POS(14, 5), .ink = 0x03, .img = IMG(1, 0, LEFT),  .var = 0x81 },

    /* chancel */
    { .pos = POS(6, 3), .ink = 0x03, .img = IMG(1, 0, RIGHT), .var = 0 },
    { .pos = POS(9, 3), .ink = 0x05, .img = IMG(1, 0, LEFT),  .var = 1 },
    { .pos = POS(6, 5), .ink = 0x05, .img = IMG(2, 4, RIGHT), .var = 0 },
    { .pos = POS(9, 5), .ink = 0x03, .img = IMG(2, 4, LEFT),  .var = 1 },

    /* stables */
    { .pos = POS(2, 5), .ink = 0x03, .img = IMG(1, 0, RIGHT), .var = 0 },
    { .pos = POS(2, 8), .ink = 0x05, .img = IMG(1, 1, RIGHT), .var = 1 },

    /* bedroom */
    { .pos = POS( 5, 8), .ink = 0x42, .img = IMG(1, 0, RIGHT) },
    { .pos = POS( 7, 4), .ink = 0x02, .img = IMG(2, 4, RIGHT) },
    { .pos = POS( 7, 5), .ink = 0x02, .img = IMG(2, 4, RIGHT) },
    { .pos = POS(10, 5), .ink = 0x02, .img = IMG(2, 4, LEFT) },
    { .pos = POS(10, 4), .ink = 0x02, .img = IMG(2, 4, LEFT) },

    /* training */
    { .pos = POS( 3, 4), .ink = 0x03, .img = IMG(2, 4, LEFT) },
    { .pos = POS( 9, 4), .ink = 0x43, .img = IMG(2, 4, LEFT) },
    { .pos = POS( 3, 9), .ink = 0x05, .img = IMG(1, 0, LEFT) },
    { .pos = POS( 6, 9), .ink = 0x05, .img = IMG(1, 1, LEFT) },
    { .pos = POS( 9, 9), .ink = 0x05, .img = IMG(1, 0, LEFT) },
    { .pos = POS(12, 9), .ink = 0x05, .img = IMG(1, 1, LEFT) },
};

static struct Mob *actors[8];

void reset_mobs(void) {
    memcpy(mobs, mobs_reset, sizeof(mobs));
}

void reset_actors(void) {
    *actors = NULL;
}

static word pos_to_ink(byte pos) {
    return (X(pos) << 1) | (Y(pos) << 2);
}

static byte diff(byte a, byte b) {
    return a > b ? a - b : b - a;
}

static byte distance_x(byte a, byte b) {
    return diff(X(a), X(b));
}

static byte distance_y(byte a, byte b) {
    return diff(Y(a), Y(b)) >> 4;
}

static byte manhattan(byte a, byte b) {
    return distance_x(a, b) + distance_y(a, b);
}

static byte range(byte target) {
    return manhattan(target, player.pos);
}

static void set_tile_ink(byte pos, byte ink) {
    byte *ptr = COLOUR(pos_to_ink(pos));
    *ptr = ink; ptr += 0x01;
    *ptr = ink; ptr += 0x1f;
    *ptr = ink; ptr += 0x01;
    *ptr = ink;
}

static void draw_mob(struct Mob *mob) {
    draw_tile(MOB(0), mob->pos, mob->img);
    set_tile_ink(mob->pos, mob->ink);
}

static void reset_mob(struct Mob *mob) {
    const byte *src = (const byte *) mobs_reset;
    src += (word) mob - (word) mobs;
    memcpy(mob, src, sizeof(struct Mob));
}

struct Mob **last(void) {
    struct Mob **ptr = actors;
    while (*ptr) ptr++;
    return ptr;
}

void add_actor(struct Mob *mob) {
    struct Mob **ptr = last();
    ptr[0] = mob;
    ptr[1] = NULL;
}

void place_actors(void) {
    struct Mob **ptr = actors;
    while (*ptr) { draw_mob(*ptr++); }
}

struct Mob *is_mob(byte pos) {
    struct Mob **ptr = actors;
    while (*ptr) {
	struct Mob *mob = *ptr++;
	if (mob->pos == pos) return mob;
    }
    return NULL;
}

byte is_dead(struct Mob *mob) {
    return (mob->img & 0x18) == TILE(BEATEN);
}

static byte all_dead(void) {
    struct Mob **ptr = actors;
    while (*ptr) {
	if (!is_dead(*ptr++)) return 0;
    }
    return 1;
}

static byte is_tile(struct Mob *mob, byte tile) {
    return (mob->img & 0x1c) == tile;
}

static int8 mob_side(struct Mob *mob) {
    return mob->img & LEFT ? -1 : 1;
}

static void activate_mobs(void) {
    Action shamble = room->shamble;
    struct Mob **ptr = actors;
    while (*ptr) {
	shamble(*ptr++);
	if (is_dead(&player)) {
	    break;
	}
    }
}

void hourglass(byte color) {
    * COLOUR(0x01) = color;
    * COLOUR(0x21) = color;
}

void shamble_mobs(void) {
    clear_message();
    hourglass(0x5);
    activate_mobs();
    call(room->turn);
    hourglass(0x0);
}

static void change_image(struct Mob *mob, byte tile) {
    mob->img = (mob->img & 0xE3) | tile;
}

void update_image(struct Mob *mob, byte tile) {
    change_image(mob, tile);
    draw_mob(mob);
}

static void change_stance(struct Mob *mob, byte tile) {
    byte stance = mob->img & TILE(STANCE);
    if (!stance) tile |= TILE(STANCE);
    update_image(mob, tile);
}

void mob_direction(struct Mob *mob, int8 delta) {
    if (delta == 1) {
	mob->img &= ~1;
    }
    else if (delta == -1) {
	mob->img |= 1;
    }
}

static void restore_color(byte pos) {
    word index = pos_to_ink(pos);
    byte *dst = COLOUR(index);
    byte *src = INK + index - 0x80;
    *dst = *src; dst += 0x01; src += 0x01;
    *dst = *src; dst += 0x1f; src += 0x1f;
    *dst = *src; dst += 0x01; src += 0x01;
    *dst = *src;
}

void restore_tile(byte pos) {
    struct Mob *corpse;
    corpse = is_mob(pos);
    if (corpse != NULL) {
	draw_mob(corpse);
    }
    else {
	restore_color(pos);
	draw_tile(EMPTY, pos, LEVEL[pos]);
    }
}

static void clear_mob(struct Mob *mob, byte target) {
    byte pos = mob->pos;
    mob->pos = target;
    restore_tile(pos);
}

void move_mob(struct Mob *mob, byte target) {
    clear_mob(mob, target);
    change_stance(mob, TILE(MOVING));
}

void push_mob(struct Mob *mob, byte target) {
    clear_mob(mob, target);
    draw_mob(mob);
}

static void beat_victim(struct Mob *mob, struct Mob *victim, byte frame) {
    if (victim != NULL) {
	change_image(victim, TILE(BEATEN) + frame);
	if (mob->pos != victim->pos) draw_mob(victim);
    }
}

static void animate_raw_attack(struct Mob *mob, struct Mob *victim) {
    beat_victim(mob, victim, TILE(0));
    for (byte i = 0; i < 5; i++) {
	if (i == 1) beat_victim(mob, victim, TILE(1));
	change_stance(mob, TILE(ATTACK));
	swoosh(i & 1 ? 6 : 2, 1, 0);
	game_idle(10);
    }
}

void animate_attack(struct Mob *mob, struct Mob *victim) {
    animate_raw_attack(mob, victim);
    update_image(mob, TILE(MOVING));
}

byte is_occupied(byte pos) {
    return !is_walkable(pos) || pos == player.pos || is_mob(pos) != NULL;
}

static void animate_mob_shamble(struct Mob *mob) {
    for (byte i = 0; i < 2; i++) {
	change_stance(mob, TILE(MOVING));
	game_idle(5);
    }
}

static void walk_mob(struct Mob *mob, int8 delta) {
    if (delta) {
	mob_direction(mob, delta);
	move_mob(mob, mob->pos + delta);
    }
}

void shamble_beast(struct Mob *mob) {
    if (is_dead(mob)) return;
    animate_mob_shamble(mob);

    byte src = mob->pos;
    byte dst = player.pos;
    if (range(src) == 1) {
	animate_attack(mob, &player);
    }
    else {
	walk_mob(mob, a_star_move(nearest, src, dst));
    }
}

static byte is_other(struct Mob *self) {
    struct Mob **ptr = actors;
    while (*ptr) {
	struct Mob *mob = *ptr++;
	if (mob->pos == self->pos && mob != self) {
	    return true;
	}
    }
    return false;
}

static void fly_arrow(struct Mob *mob, int8 delta) {
    for (;;) {
	byte pos = mob->pos;
	if (player.pos == pos) {
	    animate_raw_attack(mob, &player);
	    return;
	}
	if (!is_walkable(pos) || is_other(mob)) {
	    return;
	}
	draw_mob(mob);
	game_idle(5);
	mob->pos = pos + delta;
	restore_tile(pos);
    }
}

void shamble_arrow(struct Mob *mob) {
    mob->var--;
    if (mob->var == 1) {
	update_image(mob, TILE(5));
    }
    if (mob->var != 0) return;

    byte pos = mob->pos;
    int8 delta = mob_side(mob);
    change_image(mob, TILE(1));
    mob->pos = pos + delta;
    restore_tile(pos);
    fly_arrow(mob, delta);
    reset_mob(mob);
    draw_mob(mob);
}

static int8 ent_movement(byte src, byte dst) {
    reset_a_star(nearest);
    add_a_star_target(dst - 2);
    add_a_star_target(dst + 2);
    return a_star(src);
}

void shamble_ent(struct Mob *mob) {
    if (is_dead(mob)) return;

    byte src = mob->pos;
    byte dst = player.pos;

    if (is_tile(mob, TILE(RESTED))) {
	if (range(src) < 5) {
	    animate_mob_shamble(mob);
	}
    }
    else if (diff(src, dst) == 2) {
	int8 delta = src > dst ? -1 : 1;
	byte middle = src + delta;
	mob_direction(mob, delta);
	if (!is_occupied(middle)) {
	    update_image(mob, TILE(7));
	    mob->pos = middle;
	    animate_raw_attack(mob, &player);
	}
	else {
	    animate_mob_shamble(mob);
	}
    }
    else {
	walk_mob(mob, ent_movement(src, dst));
    }
}

static int8 step_x(byte src, byte dst) {
    byte x1 = X(src);
    byte x2 = X(dst);
    if (x2 > x1) return  0x01;
    if (x2 < x1) return -0x01;
    return 0x00;
}

static int8 step_y(byte src, byte dst) {
    byte y1 = Y(src);
    byte y2 = Y(dst);
    if (y2 > y1) return  0x10;
    if (y2 < y1) return -0x10;
    return 0x00;
}

static int8 step(byte src, byte dst) {
    return step_x(src, dst) + step_y(src, dst);
}

static int8 slide(byte src, byte dst) {
    int8 dx = step_x(src, dst);
    return dx ? dx : step_y(src, dst);
}

static void move_line(struct Mob *mob, byte dst) {
    int8 delta = step(mob->pos, dst);
    while (dst && mob->pos != dst) {
	byte next = mob->pos + delta;
	if (is_occupied(next)) break;
	walk_mob(mob, delta);
	game_idle(5);
    }
}

static void long_attack(struct Mob *mob, byte len) {
    byte dst = player.pos;
    move_line(mob, dst);
    byte src = mob->pos;
    if (!len || range(src) == len) {
	mob->pos = dst;
	player.pos = src;
	animate_raw_attack(mob, &player);
    }
}

static int8 (*line_of_sight)(byte, byte);

static void probe_direction(byte pos, int8 dir) {
    pos = pos + dir;
    while (!is_occupied(pos)) {
	if (line_of_sight(pos, player.pos)) {
	    add_choice(-range(pos), pos);
	}
	pos += dir;
    }
}

static void move_direction(struct Mob *mob, const int8 *dir) {
    reset_choices();
    while (*dir) {
	probe_direction(mob->pos, *dir++);
    }
    move_line(mob, pick_choice());
}

static void shamble_direction(struct Mob *mob, const int8 *dir, byte len) {
    if (is_dead(mob)) return;

    byte dst = player.pos;
    if (line_of_sight(mob->pos, dst)) {
	long_attack(mob, len);
    }
    else {
	move_direction(mob, dir);
    }
}

static int8 check_line(int8 delta, byte src, byte dst) {
    do {
	src = src + delta;
	if (src == dst) return delta;
    } while (!is_occupied(src));
    return 0;
}

static int8 bishop_line(byte src, byte dst) {
    return distance_x(src, dst) == distance_y(src, dst);
}

void shamble_bishop(struct Mob *mob) {
    static const int8 dir[] = { 15, 17, -15, -17, 0 };
    line_of_sight = &bishop_line;
    shamble_direction(mob, dir, 2);
}

static byte should_rook_move(struct Mob *mob) {
    mob->var ^= 0x80;
    return (mob->var & 0x80) && !is_dead(mob);
}

static int8 rook_line(byte src) {
    return check_line(slide(src, player.pos), src, player.pos);
}

static byte rook_move(struct Mob *mob, int8 dir) {
    byte pos = mob->pos;
    byte dst = 0;
    do {
	pos += dir;
	if (range(pos) == 1) continue;
	if (is_occupied(pos)) break;
	dst = pos;
    } while (!rook_line(dst));
    return dst;
}

static void rook_carousel(struct Mob *mob) {
    static const int8 round[] = {
	1, 16, -1, -16, -1, 16, 1, -16, 0
    };
    for (byte i = 0; i < 4; i++) {
	int8 dir = round[mob->var & 7];
	byte pos = rook_move(mob, dir);
	if (pos) {
	    move_line(mob, pos);
	    break;
	}
	else {
	    byte var = mob->var;
	    mob->var = (var & ~3) | ((var + 1) & 3);
	}
    }
}

void shamble_rook(struct Mob *mob) {
    if (should_rook_move(mob)) {
	if (rook_line(mob->pos)) {
	    long_attack(mob, 1);
	}
	else {
	    rook_carousel(mob);
	}
    }
}

static const int8 horsing[] = { -33, 33, -31, 31, -18, 18, -14, 14, 0 };

void shamble_horse(struct Mob *mob) {
    if (is_dead(mob)) return;

    animate_mob_shamble(mob);

    byte src = mob->pos;
    byte dst = player.pos;

    walk_mob(mob, a_star_move(horsing, src, dst));

    if (mob->pos == player.pos) {
	animate_attack(mob, &player);
    }
}

static int8 queen_line(byte src, byte dst) {
    return check_line(step(src, dst), src, dst);
}

void shamble_queen(struct Mob *mob) {
    static const int8 dir[] = { -1, 1, -16, 16, -15, 15, -17, 17, 0 };
    line_of_sight = &queen_line;
    shamble_direction(mob, dir, 0);
}

static byte is_empty_row(byte dst) {
    struct Mob *mob, **ptr = actors;
    while (*ptr) {
	mob = *ptr++;
	byte pos = mob->pos;
	if (!is_dead(mob) && Y(pos) == Y(dst)) {
	    return false;
	}
    }
    return true;
}

static void update_set(struct Mob *mob, byte ink, byte set) {
    mob->img = (mob->img & 1) | set;
    mob->ink = ink;
}

void soldier_shoot(struct Mob *mob, int8 dir) {
    byte old = mob->pos;
    update_image(mob, TILE(2));
    game_idle(20);
    update_image(mob, TILE(3));
    update_set(mob, 2, SET(3) | TILE(1));
    mob->pos += dir;
    fly_arrow(mob, dir);
    mob->pos = old;
    update_set(mob, 5, SET(1) | TILE(0));
}

void shamble_soldier(struct Mob *mob) {
    if (is_dead(mob)) return;
    byte src = mob->pos;
    byte x1 = X(src);
    byte y1 = Y(src);
    byte x2 = X(player.pos);
    byte y2 = Y(player.pos);
    int8 side = x1 > x2 ? -1 : 1;

    mob_direction(mob, side);

    if (y1 == y2) {
	soldier_shoot(mob, side);
    }
    else {
	int8 dir = y1 > y2 ? -16 : 16;
	if (range(src) < 5) {
	    dir = -dir;
	}
	else if (!is_empty_row(src + dir)) {
	    return;
	}
	if (!is_occupied(src + dir)) walk_mob(mob, dir);
    }
}

static byte lightning_flash(byte eep, byte dst, byte flip) {
    byte src = POS(X(dst), 2);
    for (byte pos = src; pos <= dst; pos += POS(0, 1)) {
	byte tile = pos == dst ? TILE(3) : TILE(2);
	draw_tile(EMPTY, pos, tile | flip);
	set_tile_ink(pos, 0x47);
    }
    swoosh(eep++, 1, 0);
    for (byte pos = src; pos <= dst; pos += POS(0, 1)) {
	restore_tile(pos);
    }
    draw_mob(&player);
    swoosh(eep++, 1, 0);
    return eep;
}

static void lightning_strike(byte pos) {
    byte eep = 5;
    for (byte i = 0; i < 8; i++) {
	eep = lightning_flash(eep, pos, i & 1);
    }
}

byte *struck;
void strike_bosses(void) {
    if (all_dead()) {
	struct Mob **ptr = last();
	while (ptr-- != actors) {
	    struct Mob *mob = *ptr;
	    if ((mob->img & SET(7)) == SET(1)) {
		byte pos = mob->pos;
		lightning_strike(pos);
		mob->pos = POS(0, 0);
		restore_tile(pos);
		game_idle(20);
		*struck = true;
		*ptr = NULL;
	    }
	}
    }
}
