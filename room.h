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

static byte set_bonfire(const void *ptr, byte pos);
static byte change_room(const void *new_room, byte pos) {
    return consume_stamina(MOVE_STAMINA) && load_room(new_room, pos);
}

static const struct Bump dungeon_bump[] = {
    MAKE_BUMP(POS(2, 6), -1, &change_room, &tunnel, POS(13, 6)),
    MAKE_BUMP(POS(14, 6), 1, &change_room, &corridor, POS(1, 6)),
};

static void setup_dungeon(void);
static const struct Room dungeon = {
    .msg = "Dreadful Dungeon",
    .map = map_of_dungeon,
    .bump = dungeon_bump,
    .count = SIZE(dungeon_bump),
    .setup = &setup_dungeon,
};

static const char tutorial1[] = "Attacking takes most of your stamina";
static const char tutorial2[] = "Decimate King's army to break the curse";
static const char tutorial3[] = "That is a fine rapier you have there";
static const char tutorial4[] = "Push corpses into wall to displace them";

static const struct Bump tunnel_bump[] = {
    MAKE_BUMP(POS( 2, 6), -1, &change_room, &prison, POS(9, 6)),
    MAKE_BUMP(POS(13, 6),  1, &change_room, &dungeon, POS(2, 6)),
    MAKE_BUMP(POS( 5, 4), -16, &bump_msg, tutorial1, true),
    MAKE_BUMP(POS( 5, 8),  16, &bump_msg, tutorial2, true),
    MAKE_BUMP(POS(10, 4), -16, &bump_msg, tutorial3, true),
    MAKE_BUMP(POS(10, 8),  16, &bump_msg, tutorial4, true),
};

static const struct Room tunnel = {
    .msg = "Tunnel of Advices",
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

static const struct Bump corridor_bump[] = {
    MAKE_BUMP(POS(1, 6), -1, &change_room, &dungeon, POS(14, 6)),
    MAKE_BUMP(POS(13, 6), 1, &change_room, &courtyard, POS(2, 7)),
};

static void setup_corridor(void);
static const struct Room corridor = {
    .msg = "Creepy Corridor",
    .map = map_of_corridor,
    .bump = corridor_bump,
    .count = SIZE(corridor_bump),
    .setup = &setup_corridor,
};

static const struct Bump courtyard_bump[] = {
    MAKE_BUMP(POS(2, 7), -1, &change_room, &corridor, POS(13, 6)),
    MAKE_BUMP(POS(7, 7),  1, &set_bonfire, &courtyard, POS(7, 7)),
    MAKE_BUMP(POS(7, 3), -16, &change_room, &hallway, POS(7, 11)),
    MAKE_BUMP(POS(8, 3), -16, &change_room, &hallway, POS(8, 11)),
    MAKE_BUMP(POS(6, 11), 16, &change_room, &bailey, POS(6, 2)),
    MAKE_BUMP(POS(7, 11), 16, &change_room, &bailey, POS(7, 2)),
    MAKE_BUMP(POS(8, 11), 16, &change_room, &bailey, POS(8, 2)),
    MAKE_BUMP(POS(9, 11), 16, &change_room, &bailey, POS(9, 2)),
};

static void setup_courtyard(void);
static const struct Room courtyard = {
    .msg = "Central Courtyard",
    .map = map_of_courtyard,
    .bump = courtyard_bump,
    .count = SIZE(courtyard_bump),
    .setup = &setup_courtyard,
};

static const struct Bump hallway_bump[] = {
    MAKE_BUMP(POS(7, 11), 16, &change_room, &courtyard, POS(7, 3)),
    MAKE_BUMP(POS(8, 11), 16, &change_room, &courtyard, POS(8, 3)),
};

static const struct Room hallway = {
    .msg = "Haunted Hallway",
    .map = map_of_hallway,
    .bump = hallway_bump,
    .count = SIZE(hallway_bump),
    .setup = NULL,
};

static const struct Bump bailey_bump[] = {
    MAKE_BUMP(POS(6, 2), -16, &change_room, &courtyard, POS(6, 11)),
    MAKE_BUMP(POS(7, 2), -16, &change_room, &courtyard, POS(7, 11)),
    MAKE_BUMP(POS(8, 2), -16, &change_room, &courtyard, POS(8, 11)),
    MAKE_BUMP(POS(9, 2), -16, &change_room, &courtyard, POS(9, 11)),
};

static const struct Room bailey = {
    .msg = "Besieged Bailey",
    .map = map_of_bailey,
    .bump = bailey_bump,
    .count = SIZE(bailey_bump),
    .setup = NULL,
};

static byte door_broken;
static byte break_door(const void *ptr, byte pos) {
    if (door_broken) {
	return change_room(ptr, pos);
    }
    else if (consume_stamina(FULL_STAMINA)) {
	show_message("You break down the door");
	update_image(&player, TILE(MOVING));
	advance_tile(POS(10, 6));
	door_broken = true;
	swoosh(5, 5, -1);
    }
    else {
	show_message("Rest to replenish stamina");
    }
    return true;
}

static byte set_bonfire(const void *ptr, byte pos) {
    if (respawn != ptr) {
	respawn = ptr;
	spawn_pos = pos;
	advance_tile(POS(8, 7));
	show_message("Fire of North lit");
	swoosh(60, 20, 2);
    }
    return true;
}

static void setup_prison(void) {
    if (door_broken) LEVEL[POS(10, 6)] += TILE(1);
}

static void setup_dungeon(void) {
    decompress(MOB(1), beast);
    add_actor(&shamble_beast, mobs + BARRY);
    add_actor(&shamble_beast, mobs + LARRY);
    add_actor(&shamble_beast, mobs + HARRY);
}

static void setup_corridor(void) {
    decompress(MOB(1), beast);
    add_actor(&shamble_beast, mobs + JURIS);
    add_actor(&shamble_beast, mobs + ZIGIS);
    add_actor(&shamble_beast, mobs + ROBIS);
}

static void setup_courtyard(void) {
    if (respawn != &courtyard) LEVEL[POS(8, 7)] -= TILE(1);
}
