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

static bool load_room(const void *ptr, byte pos);

#define LOAD_ROOM(from, dir, room, dst) \
    { .pos = from, .delta = dir, .fn = &load_room, .ptr = room, .arg = dst }

static const struct Bump dungeon_bump[] = {
    LOAD_ROOM(POS(13, 9), 1, &tunnel, POS(2, 9)),
};

static const struct Room dungeon = {
    .msg = "Dreadful Dungeon",
    .map = map_of_dungeon,
    .bump = dungeon_bump,
    .count = SIZE(dungeon_bump),
    .setup = NULL,
};

static const struct Bump tunnel_bump[] = {
    LOAD_ROOM(POS(2, 9), -1, &dungeon, POS(13, 9)),
};

static const struct Room tunnel = {
    .msg = "Dungeon Tunnel",
    .map = map_of_tunnel,
    .bump = tunnel_bump,
    .count = SIZE(tunnel_bump),
    .setup = NULL,
};

static const struct Bump prison_bump[] = {
    LOAD_ROOM(POS(13, 9), 1, &tunnel, POS(2, 9)),
};

static const struct Room prison = {
    .msg = "Prison Cell",
    .map = map_of_prison,
    .bump = prison_bump,
    .count = SIZE(prison_bump),
    .setup = NULL,
};
