typedef void(*Action)(void *);

struct Actor {
    Action fn;
    void *mob;
};

static byte actor_count;
static struct Actor actors[4];

static void reset_actors(void) {
    actor_count = 0;
}

static void add_actor(Action fn, void *mob) {
    struct Actor *ptr = actors + actor_count;
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
