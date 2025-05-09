typedef bool(*Caller)(const void *, byte);

struct Bump {
    byte pos;
    int8 delta;
    Caller fn;
    void *ptr;
    byte arg;
};

struct Room {
    const char *msg;
    const void **map;
    void (*setup)(void);
    const void *bump;
    byte count;
};

static byte consume_stamina(byte amount);
static void show_message(const char *msg);
static bool load_room(const void *ptr, byte pos);
static void draw_tile(byte *ptr, byte pos, byte id);

#define MAKE_BUMP(from, dir, action, data, param) \
    { .pos = from, .delta = dir, .fn = action, .ptr = data, .arg = param }

static const struct Bump dungeon_bump[] = {
    MAKE_BUMP(POS(2, 6), -1, &load_room, &tunnel, POS(13, 6)),
};

static const struct Room dungeon = {
    .msg = "Dreadful Dungeon",
    .map = map_of_dungeon,
    .bump = dungeon_bump,
    .count = SIZE(dungeon_bump),
    .setup = NULL,
};

static const struct Bump tunnel_bump[] = {
    MAKE_BUMP(POS(2, 6), -1, &load_room, &prison, POS(9, 6)),
    MAKE_BUMP(POS(13, 6), 1, &load_room, &dungeon, POS(2, 6)),
};

static const struct Room tunnel = {
    .msg = "Dungeon Tunnel",
    .map = map_of_tunnel,
    .bump = tunnel_bump,
    .count = SIZE(tunnel_bump),
    .setup = NULL,
};

static const struct Bump prison_bump[] = {
    MAKE_BUMP(POS(9, 6), 1, &break_door, &tunnel, POS(2, 6)),
};

static void setup_prison(void);
static const struct Room prison = {
    .msg = "Prison Cell",
    .map = map_of_prison,
    .bump = prison_bump,
    .count = SIZE(prison_bump),
    .setup = &setup_prison,
};

static bool door_broken;
static bool break_door(const void *ptr, byte pos) {
    if (door_broken) {
	load_room(ptr, pos);
    }
    else if (consume_stamina(42)) {
	draw_tile(EMPTY, POS(10, 6), TILE(6));
	door_broken = true;
    }
    else {
	show_message("Rest to replenish stamina");
    }
    return true;
}

static void setup_prison(void) {
    if (door_broken) LEVEL[POS(10, 6)] = TILE(6);
}
