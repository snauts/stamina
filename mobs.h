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
    { .pos = POS(10, 7), .img = TILE(0) },
    { .pos = POS(10, 5), .img = TILE(1) },
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

static void add_actor(Action fn, struct Mob *mob) {
    struct Actor *ptr = actors + actor_count;
    ptr->fn = fn;
    ptr->mob = mob;
    actor_count++;
}

static void place_actors(void) {
    for (byte i = 0; i < actor_count; i++) {
	struct Mob *mob = actors[i].mob;
	draw_tile(ENEMY(0), mob->pos, mob->img);
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
    mob->img = (mob->img & 7) ^ TILE(STANCE);
}

static void change_image(struct Mob *mob, byte tile) {
    mob->img = (mob->img & 7) | tile;
}

static void shamble_beast(struct Mob *mob) {
    mob;
}
