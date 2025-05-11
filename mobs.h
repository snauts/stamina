struct Mob {
    byte pos;
    byte img;
};

enum {
    LARRY,
    BARRY,
    /* this should be last */
    TOTAL_MOBS,
};

static struct Mob mobs[TOTAL_MOBS];

static const struct Mob mobs_reset[TOTAL_MOBS] = {
    { .pos = POS(10, 7), .img = SET(1) | TILE(0) | LEFT },
    { .pos = POS(10, 5), .img = SET(1) | TILE(1) | LEFT },
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

static void draw_mob(struct Mob *mob) {
    draw_tile(MOB(0), mob->pos, mob->img);
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

static void shamble_mobs(void) {
    struct Actor *ptr = actors;
    for (byte i = 0; i < actor_count; i++) {
	ptr->fn(ptr->mob);
	ptr++;
    }
}

static void change_stance(struct Mob *mob) {
    mob->img = mob->img ^ TILE(STANCE);
}

static byte is_dead(struct Mob *mob) {
    return (mob->img & 0x18) == TILE(BEATEN);
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

static void move_mob(struct Mob *mob, byte target) {
    draw_tile(EMPTY, mob->pos, LEVEL[mob->pos]);
    mob->pos = target;
    change_stance(mob);
    update_image(mob, TILE(MOVING));
}

static void shamble_beast(struct Mob *mob) {
    mob;
}
