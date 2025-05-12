struct Mob {
    byte pos;
    byte img;
    byte ink;
};

enum {
    LARRY,
    BARRY,
    HARRY,
    /* this should be last */
    TOTAL_MOBS,
};

static struct Mob player;

static struct Mob mobs[TOTAL_MOBS];

static const struct Mob mobs_reset[TOTAL_MOBS] = {
    { .pos = POS(10, 8), .ink = 0x02, .img = SET(1) | TILE(0) | LEFT },
    { .pos = POS(10, 6), .ink = 0x42, .img = SET(1) | TILE(1) | LEFT },
    { .pos = POS(10, 4), .ink = 0x02, .img = SET(1) | TILE(0) | LEFT },
};

typedef void(*Action)(struct Mob *);

struct Actor {
    struct Mob *mob;
    Action fn;
};

static byte actor_count;
static struct Actor actors[4];

static void reset_mobs(void) {
    memcpy(mobs, mobs_reset, sizeof(mobs));
}

static void reset_actors(void) {
    actor_count = 0;
}

static word pos_to_ink(byte pos) {
    return ((pos & 0x0f) << 1) | ((pos & 0xf0) << 2);
}

static byte diff(byte a, byte b) {
    return a > b ? a - b : b - a;
}

static byte distance_x(byte a, byte b) {
    return diff(a & 0x0f, b & 0xf);
}

static byte distance_y(byte a, byte b) {
    return diff(a & 0xf0, b & 0xf0) >> 4;
}

static int8 step_x(byte a, byte b) {
    return (a & 0x0f) > (b & 0x0f) ? -0x01 : 0x01;
}

static int8 step_y(byte a, byte b) {
    return (a & 0xf0) > (b & 0xf0) ? -0x10 : 0x10;
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

static void add_actor(Action fn, struct Mob *mob) {
    struct Actor *ptr = actors + actor_count;
    ptr->fn = fn;
    ptr->mob = mob;
    actor_count++;
}

static void place_actors(void) {
    for (byte i = 0; i < actor_count; i++) {
	draw_mob(actors[i].mob);
    }
}

static struct Mob *is_mob(byte pos) {
    for (byte i = 0; i < actor_count; i++) {
	struct Mob *mob = actors[i].mob;
	if (mob->pos == pos) return mob;
    }
    return NULL;
}

static byte is_dead(struct Mob *mob) {
    return (mob->img & 0x18) == TILE(BEATEN);
}

static void shamble_mobs(void) {
    struct Actor *ptr = actors;
    for (byte i = 0; i < actor_count; i++) {
	struct Mob *mob = ptr->mob;
	if (!is_dead(mob)) ptr->fn(mob);
	if (is_dead(&player)) {
	    game_idle(25);
	    break;
	}
	ptr++;
    }
}

static void update_image(struct Mob *mob, byte tile) {
    mob->img = (mob->img & 0xE3) | tile;
    draw_mob(mob);
}

static void change_stance(struct Mob *mob, byte tile) {
    byte stance = mob->img & TILE(STANCE);
    if (!stance) tile |= TILE(STANCE);
    update_image(mob, tile);
}

static void mob_direction(struct Mob *mob, int8 delta) {
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
    restore_color(pos);
    draw_tile(EMPTY, pos, LEVEL[pos]);
}

static void move_mob(struct Mob *mob, byte target) {
    clear_mob(mob, target);
    change_stance(mob, TILE(MOVING));
}

static void push_mob(struct Mob *mob, byte target) {
    clear_mob(mob, target);
    draw_mob(mob);
}

static void beat_victim(struct Mob *victim, byte frame) {
    if (victim != NULL) update_image(victim, TILE(BEATEN) + frame);
}

static void animate_attack(struct Mob *mob, struct Mob *victim) {
    beat_victim(victim, TILE(0));
    for (byte i = 0; i < 5; i++) {
	if (i == 1) beat_victim(victim, TILE(1));
	change_stance(mob, TILE(ATTACK));
	game_idle(10);
    }
    update_image(mob, TILE(MOVING));
}

static byte is_occupied(byte pos) {
    return !is_walkable(pos) || is_mob(pos) != NULL;
}

static void shamble_beast(struct Mob *mob) {
    byte pos = mob->pos;
    byte dx = distance_x(pos, player.pos);
    byte dy = distance_y(pos, player.pos);

    if (dx + dy == 1) {
	animate_attack(mob, &player);
    }
    else {
	int8 delta = step_x(pos, player.pos);
	int8 other = step_y(pos, player.pos);
	if (dx < dy) {
	    byte tmp = other;
	    other = delta;
	    delta = tmp;
	}
	if (is_occupied(pos + delta)) {
	    if (is_occupied(pos + other)) return;
	    delta = other;
	}
	mob_direction(mob, delta);
	pos = pos + delta;
	move_mob(mob, pos);
    }
}
