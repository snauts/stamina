struct Mob {
    byte pos;
    byte sprite;
};

enum {
    LARRY,
    BARRY,
    /* this should be last */
    TOTAL_MOBS,
};

static struct Mob mobs[TOTAL_MOBS];

static const struct Mob mobs_reset[TOTAL_MOBS] = {
    { .pos = POS(10, 7), .sprite = 0 },
    { .pos = POS(10, 5), .sprite = 0 },
};

typedef void(*Action)(struct Mob *);

struct Actor {
    Action fn;
    struct Mob *mob;
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
    draw_tile(ENEMY(0), mob->pos, mob->sprite);
    ptr->fn = fn;
    ptr->mob = mob;
    actor_count++;
}

static void shamble_mobs(void) {
    struct Actor *ptr = actors;
    for (byte i = 0; i < actor_count; i++) {
	ptr->fn(ptr->mob);
	ptr++;
    }
}

static void shamble_beast(struct Mob *mob) {
    mob;
}
