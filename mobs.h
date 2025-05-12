struct Mob {
    byte pos;
    byte img;
    byte ink;
};

enum {
    LARRY,
    BARRY,
    /* this should be last */
    TOTAL_MOBS,
};

static struct Mob player;

static struct Mob mobs[TOTAL_MOBS];

static const struct Mob mobs_reset[TOTAL_MOBS] = {
    { .pos = POS(10, 7), .ink = 2, .img = SET(1) | TILE(0) | LEFT },
    { .pos = POS(10, 5), .ink = 2, .img = SET(1) | TILE(1) | LEFT },
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

static byte dx(byte a, byte b) {
    return diff(a & 0x0f, b & 0xf);
}

static byte dy(byte a, byte b) {
    return diff(a & 0xf0, b & 0xf0) >> 4;
}

static byte manhattan(byte a, byte b) {
    return dx(a, b) + dy(a, b);
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
	if (is_dead(&player)) break;
	ptr++;
    }
}

static void change_stance(struct Mob *mob) {
    mob->img = mob->img ^ TILE(STANCE);
}

static void update_image(struct Mob *mob, byte tile) {
    mob->img = (mob->img & 0xE7) | tile;
    draw_mob(mob);
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
    byte *src = INK + index;
    *dst = *src; dst += 0x01; src += 0x01;
    *dst = *src; dst += 0x1f; src += 0x1f;
    *dst = *src; dst += 0x01; src += 0x01;
    *dst = *src;
}

static void move_stance(struct Mob *mob) {
    change_stance(mob);
    update_image(mob, TILE(MOVING));
}

static void move_mob(struct Mob *mob, byte target) {
    draw_tile(EMPTY, mob->pos, LEVEL[mob->pos]);
    restore_color(mob->pos);
    mob->pos = target;
    move_stance(mob);
}

static void beat_victim(struct Mob *victim) {
    if (victim != NULL) update_image(victim, TILE(BEATEN));
}

static void animate_attack(struct Mob *mob, struct Mob *victim) {
    for (byte i = 0; i < 4; i++) {
	if (i == 1) beat_victim(victim);
	update_image(mob, TILE(ATTACK));
	change_stance(mob);
	game_idle(8);
    }
    update_image(mob, TILE(MOVING));
}

static byte not_occupied(byte pos) {
    return is_walkable(pos) && is_mob(pos) == NULL;
}

static void shamble_beast(struct Mob *mob) {
    mob;
}
