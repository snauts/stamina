#include "main.h"

extern const int8 nearest[];

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
    { .pos = POS(10, 9), .ink = 0x02, .img = IMG(1, 4, FLIP), .var = 4 },
    { .pos = POS( 5, 5), .ink = 0x02, .img = IMG(1, 4, LEFT), .var = 3 },

    /* bailey */
    { .pos = POS(11, 6), .ink = 0x44, .img = IMG(1, 6, LEFT),  },
    { .pos = POS(11, 8), .ink = 0x44, .img = IMG(1, 6, RIGHT),  },
    { .pos = POS( 4, 6), .ink = 0x44, .img = IMG(1, 6, RIGHT), },
    { .pos = POS( 4, 8), .ink = 0x44, .img = IMG(1, 6, LEFT), },

    /* ramparts */
    { .pos = POS( 1, 5), .ink = 0x05, .img = IMG(1, 0, RIGHT), .var = 0 },
    { .pos = POS(14, 5), .ink = 0x03, .img = IMG(1, 0, LEFT),  .var = 1 },

    /* chancel */
    { .pos = POS(6, 3), .ink = 0x03, .img = IMG(1, 0, RIGHT), .var = 0 },
    { .pos = POS(9, 3), .ink = 0x05, .img = IMG(1, 0, LEFT),  .var = 1 },

    /* stables */
    { .pos = POS(2, 5), .ink = 0x03, .img = IMG(1, 0, RIGHT), .var = 0 },
    { .pos = POS(2, 8), .ink = 0x05, .img = IMG(1, 1, RIGHT),  .var = 1 },
};

struct Actor {
    struct Mob *mob;
    Action fn;
};

static byte actor_count;
static struct Actor actors[4];

void reset_mobs(void) {
    memcpy(mobs, mobs_reset, sizeof(mobs));
}

void reset_actors(void) {
    actor_count = 0;
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

static void set_mob_ink(struct Mob *mob) {
    byte ink = mob->ink;
    byte *ptr = COLOUR(pos_to_ink(mob->pos));
    *ptr = ink; ptr += 0x01;
    *ptr = ink; ptr += 0x1f;
    *ptr = ink; ptr += 0x01;
    *ptr = ink;
}

static void draw_mob(struct Mob *mob) {
    draw_tile(MOB(0), mob->pos, mob->img);
    set_mob_ink(mob);
}

static void reset_mob(struct Mob *mob) {
    const byte *src = (const byte *) mobs_reset;
    src += (word) mob - (word) mobs;
    memcpy(mob, src, sizeof(struct Mob));
}

void add_actor(Action fn, struct Mob *mob) {
    struct Actor *ptr = actors + actor_count;
    ptr->mob = mob;
    ptr->fn = fn;
    actor_count++;
}

void place_actors(void) {
    for (byte i = 0; i < actor_count; i++) {
	draw_mob(actors[i].mob);
    }
}

struct Mob *is_mob(byte pos) {
    for (byte i = 0; i < actor_count; i++) {
	struct Mob *mob = actors[i].mob;
	if (mob->pos == pos) return mob;
    }
    return NULL;
}

byte is_dead(struct Mob *mob) {
    return (mob->img & 0x18) == TILE(BEATEN);
}

static byte is_tile(struct Mob *mob, byte tile) {
    return (mob->img & 0x1c) == tile;
}

static int8 mob_side(struct Mob *mob) {
    return mob->img & LEFT ? 1 : -1;
}

static void activate_mobs(void) {
    for (byte i = 0; i < actor_count; i++) {
	struct Mob *mob = actors[i].mob;
	actors[i].fn(mob);
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

static void clear_mob(struct Mob *mob, byte target) {
    byte pos = mob->pos;
    mob->pos = target;

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

void move_mob(struct Mob *mob, byte target) {
    clear_mob(mob, target);
    change_stance(mob, TILE(MOVING));
}

void push_mob(struct Mob *mob, byte target) {
    clear_mob(mob, target);
    draw_mob(mob);
}

static void beat_victim(struct Mob *mob, struct Mob *victim, byte frame) {
    if (victim != NULL && mob->pos != victim->pos) {
	update_image(victim, TILE(BEATEN) + frame);
    }
}

static void animate_raw_attack(struct Mob *mob, struct Mob *victim) {
    beat_victim(mob, victim, TILE(0));
    for (byte i = 0; i < 5; i++) {
	if (i == 1) beat_victim(mob, victim, TILE(1));
	change_stance(mob, TILE(ATTACK));
	beep(i & 1 ? 1500 : 500, 500);
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
	game_idle(10);
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
    if (manhattan(src, dst) == 1) {
	animate_attack(mob, &player);
    }
    else {
	walk_mob(mob, a_star_near(nearest, src, dst));
    }
}

void shoot_arrow(struct Mob *mob) {
    mob->var--;
    if (mob->var == 1) {
	update_image(mob, TILE(5));
    }
    if (mob->var != 0) return;

    byte pos = mob->pos;
    int8 delta = mob_side(mob);
    change_image(mob, TILE(1));

    for (;;) {
	pos += delta;
	if (!is_walkable(pos)) {
	    clear_mob(mob, pos);
	    break;
	}

	push_mob(mob, pos);
	if (player.pos == pos) {
	    animate_raw_attack(mob, &player);
	    return;
	}
	game_idle(5);
    }
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
	if (manhattan(src, dst) < 5) {
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

static int8 step(byte src, byte dst) {
    int8 delta = 0;
    byte x1 = X(src);
    byte x2 = X(dst);
    if (x2 > x1) delta += 0x01;
    if (x2 < x1) delta -= 0x01;
    byte y1 = Y(src);
    byte y2 = Y(dst);
    if (y2 > y1) delta += 0x10;
    if (y2 < y1) delta -= 0x10;
    return delta;
}

static void move_rook(struct Mob *mob, byte dst) {
    int8 delta = step(mob->pos, dst);
    while (mob->pos != dst) {
	byte next = mob->pos + delta;
	if (is_occupied(next)) break;
	walk_mob(mob, delta);
	game_idle(3);
    }
}

static byte combine(byte p1, byte p2) {
    return Y(p1) | X(p2);
}

static void add_rook_move(byte dst, byte pos) {
    if (!is_occupied(pos)) add_choice(-manhattan(dst, pos), pos);
}

static void long_attack(struct Mob *mob, byte dst, byte len) {
    move_rook(mob, dst);
    byte src = mob->pos;
    if (manhattan(src, dst) == len) {
	mob->pos = dst;
	player.pos = src;
	animate_raw_attack(mob, &player);
    }
}

void shamble_rook(struct Mob *mob) {
    if (is_dead(mob) || mob->var-- > 0) return;

    mob->var = 2;

    byte src = mob->pos;
    byte dst = player.pos;

    byte dx = distance_x(src, dst);
    byte dy = distance_y(src, dst);

    if (dx == 0 || dy == 0) {
	long_attack(mob, dst, 1);
    }
    else {
	reset_choices();
	add_rook_move(dst, combine(src, dst));
	add_rook_move(dst, combine(dst, src));
	move_rook(mob, pick_choice());
    }
}

void shamble_bishop(struct Mob *mob) {
    byte src = mob->pos;
    byte dst = player.pos;

    byte dx = distance_x(src, dst);
    byte dy = distance_y(src, dst);

    if (dx == dy) {
	long_attack(mob, dst, 2);
    }
}

const int8 horsing[] = { -33, 33, -31, 31, -18, 18, -14, 14, 0 };

void shamble_horse(struct Mob *mob) {
    mob;
}
