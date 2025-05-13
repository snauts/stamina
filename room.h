typedef byte(*Caller)(const void *, byte);

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

#define MAKE_BUMP(from, dir, action, data, param) \
    { .pos = from, .delta = dir, .fn = action, .ptr = data, .arg = param }

static byte change_room(const void *new_room, byte pos) {
    return consume_stamina(MOVE_STAMINA) && load_room(new_room, pos);
}

static const struct Bump dungeon_bump[] = {
    MAKE_BUMP(POS(2, 6), -1, &change_room, &tunnel, POS(13, 6)),
};

static void setup_dungeon(void);
static const struct Room dungeon = {
    .msg = "Dreadful Dungeon",
    .map = map_of_dungeon,
    .bump = dungeon_bump,
    .count = SIZE(dungeon_bump),
    .setup = &setup_dungeon,
};

static const char tutorial1[] = "Attacking takes half of your stamina";
static const char tutorial2[] = "Decimate King's army to break the curse";
static const char tutorial3[] = "That is a fine rapier you have there";
static const char tutorial4[] = "Push corpses into wall to walk on them";

static const struct Bump tunnel_bump[] = {
    MAKE_BUMP(POS( 2, 6), -1, &change_room, &prison, POS(9, 6)),
    MAKE_BUMP(POS(13, 6),  1, &change_room, &dungeon, POS(2, 6)),
    MAKE_BUMP(POS( 5, 4), -16, &bump_msg, tutorial1, true),
    MAKE_BUMP(POS( 5, 8),  16, &bump_msg, tutorial2, true),
    MAKE_BUMP(POS(10, 4), -16, &bump_msg, tutorial3, true),
    MAKE_BUMP(POS(10, 8),  16, &bump_msg, tutorial4, true),
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

static byte door_broken;
static byte break_door(const void *ptr, byte pos) {
    if (door_broken) {
	return change_room(ptr, pos);
    }
    else if (consume_stamina(FULL_STAMINA)) {
	show_message("You break down the door");
	draw_tile(EMPTY, POS(10, 6), TILE(6));
	update_image(&player, TILE(MOVING));
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

static void setup_dungeon(void) {
    decompress(MOB(1), beast);
    add_actor(&shamble_beast, mobs + BARRY);
    add_actor(&shamble_beast, mobs + LARRY);
    add_actor(&shamble_beast, mobs + HARRY);
}
