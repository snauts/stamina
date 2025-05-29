#include "main.h"
#include "data.h"

extern struct Mob mobs[];
extern const void *respawn;
extern byte spawn_pos;
extern byte *struck;

static byte door_broken;

#define KING_PROGRESS mobs[FERDINAND].var

enum { SOLDIER, HORSE, BISHOP, ROOK, QUEEN, ALL };
static const void* const lieutenants[] = {
    soldier, horse, bishop, rook, queen, king,
};
static byte beaten[ALL];

#define MAKE_BUMP(from, dir, action, data, param) \
    { .pos = from, .delta = dir, .fn = action, .ptr = data, .arg = param }

static byte change_room(const void *new_room, byte pos) {
    return consume_stamina(MOVE_STAMINA) && load_room(new_room, pos);
}

static byte break_door(const void *ptr, byte pos) {
    if (door_broken) {
	return change_room(ptr, pos);
    }
    else if (consume_stamina(FULL_STAMINA)) {
	show_message("You break down the door");
	update_image(&player, TILE(MOVING));
	advance_tile(POS(10, 6));
	door_broken = true;
	swoosh(3, 3, -1);
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
	show_message("Central fire of awakening");
	swoosh(60, 20, 5);
    }
    return true;
}

static void setup_prison(void) {
    if (door_broken) LEVEL[POS(10, 6)] += TILE(1);
}

static void setup_dungeon(void) {
    decompress(MOB(1), beast);
    add_actor(mobs + BARRY);
    add_actor(mobs + LARRY);
    add_actor(mobs + HARRY);
}

static void setup_corridor(void) {
    decompress(MOB(1), beast);
    add_actor(mobs + JURIS);
    add_actor(mobs + ZIGIS);
    add_actor(mobs + ROBIS);
}

static void setup_hallway(void) {
    decompress(MOB(1), arrow);
    add_actor(mobs + ARROW1);
    add_actor(mobs + ARROW2);
}

static void setup_bailey(void) {
    decompress(MOB(1), ent);
    add_actor(mobs + SKINBARK);
    add_actor(mobs + LEAFLOCK);
    add_actor(mobs + BREGALAD);
    add_actor(mobs + BUSHKOPF);
}

static void setup_furniture(byte i) {
    decompress(MOB(1), lieutenants[i]);
    memcpy(SPRITE(2, 4), SPRITE(1, 6), 32);
    struck = beaten + i;
}

static void setup_rampart(void) {
    setup_furniture(ROOK);
    add_actor(mobs + PILE1);
    add_actor(mobs + PILE2);
    if (!beaten[ROOK]) {
	add_actor(mobs + JAMES);
	add_actor(mobs + OSKAR);
    }
}

static void setup_chancel(void) {
    setup_furniture(BISHOP);
    add_actor(mobs + WILLY);
    add_actor(mobs + TOMMY);
    if (!beaten[BISHOP]) {
	add_actor(mobs + ISAAC);
	add_actor(mobs + DAVID);
    }
}

static void setup_stables(void) {
    setup_furniture(HORSE);
    if (!beaten[HORSE]) {
	add_actor(mobs + PERSIJS);
	add_actor(mobs + MARKUSS);
    }
}

static void setup_bedroom(void) {
    setup_furniture(QUEEN);
    add_actor(mobs + CHAIR1);
    add_actor(mobs + CHAIR2);
    add_actor(mobs + CHAIR3);
    add_actor(mobs + CHAIR4);
    if (!beaten[QUEEN]) {
	add_actor(mobs + JEZEBEL);
    }
}

static void setup_training(void) {
    setup_furniture(SOLDIER);
    decompress(MOB(3), arrow);
    add_actor(mobs + DUMMY1);
    add_actor(mobs + DUMMY2);
    if (!beaten[SOLDIER]) {
	add_actor(mobs + JOE);
	add_actor(mobs + BOB);
	add_actor(mobs + SID);
	add_actor(mobs + UDO);
    }
}

static void painting(byte pos, byte frame) {
    memcpy(EMPTY + FRAME(frame), MOB(1), 32);
    LEVEL[pos] = frame << 2;
}

static void open_seal(void) {
    LEVEL[POS(7, 5)] += TILE(1);
    LEVEL[POS(6, 6)] += TILE(1);
    LEVEL[POS(7, 6)] += TILE(1);
    LEVEL[POS(8, 6)] += TILE(1);
    INK[0xEE] = INK[0xEF] = 5;
}

static byte total_beaten;
static byte sealed_room(const void *new_room, byte pos) {
    if (total_beaten == ALL) {
	return change_room(new_room, pos);
    }
    else {
	show_message("Alas, curse holds this door locked");
    }
    return true;
}

static void setup_passage(void) {
    total_beaten = 0;
    byte pos = POS(1, 3);
    byte frame = 64 - ALL;
    for (byte i = 0; i < ALL; i++) {
	setup_furniture(i);
	if (*struck) {
	    painting(pos, frame);
	    total_beaten++;
	}
	pos += POS(3, 0);
	frame++;
    }
    if (total_beaten == ALL) {
	open_seal();
    }
}

static void update_tile(byte pos, byte change) {
    LEVEL[pos] -= change;
    restore_tile(pos);
}

static void king_leaves_throne(void) {
    update_tile(POS(7, 2), TILE(1));
    update_tile(POS(7, 3), TILE(1));
    update_tile(POS(8, 2), TILE(2));
    update_tile(POS(8, 3), TILE(2));
}

static const char * const king_dialogue[] = {
    "Prince Richard...",
    "Suprised to see you so far up North",
    "This ill-conceived journey of yours ends here",
};

static const Action shamblers[] = {
    shamble_soldier,
    shamble_horse,
    shamble_bishop,
    shamble_rook,
    shamble_queen,
    shamble_king,
};

static const struct Mob * const defenders[] = {
    mobs + JOE,
    mobs + PERSIJS,
    mobs + ISAAC,
    mobs + OSKAR,
    mobs + JEZEBEL,
    mobs + FERDINAND,
};

static int8 current_henchman(void) {
    return KING_PROGRESS - SIZE(king_dialogue);
}

static void shamble_throne(struct Mob *mob) {
    shamblers[current_henchman()](mob);
}

static struct Mob *henchman;
static void add_henchman(byte pos) {
    byte i = current_henchman();

    henchman = (void *) defenders[i];
    henchman->pos = pos;

    byte set = i + 1;
    decompress(MOB(set), lieutenants[i]);
    henchman->img = SET(set) | (X(player.pos) < X(pos) ? LEFT : 0);
    add_actor(henchman);

    lightning_strike(pos);
}

static byte king_cutscene(const void *ptr, byte pos) {
    if (KING_PROGRESS < SIZE(king_dialogue)) {
	show_message(king_dialogue[KING_PROGRESS]);
	if (++KING_PROGRESS == SIZE(king_dialogue)) {
	    add_henchman(POS(14, 9));
	    decompress(MOB(3), arrow);
	}
	return true;
    }
    return false; ptr; pos;
}

static void next_henchman(void) {
    if (current_henchman() == ALL) {
	king_leaves_throne();
    }
    add_henchman(free_spot());
}

static void throne_turn(void) {
    if (is_dead(henchman)) {
	int8 index = current_henchman();
	if (index >= 0 && index < ALL) {
	    KING_PROGRESS++;
	    next_henchman();
	}
    }
}

/*** Prison ***/

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

/*** Tunnel ***/

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
    .msg = "Tunnel of Advice",
    .map = map_of_tunnel,
    .bump = tunnel_bump,
    .count = SIZE(tunnel_bump),
};

/*** Dungeon ***/

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
    .shamble = shamble_beast,
};

/*** Corridor ***/

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
    .shamble = shamble_beast,
};

/*** Courtyard ***/

static const struct Bump courtyard_bump[] = {
    MAKE_BUMP(POS(2, 7), -1, &change_room, &corridor, POS(13, 6)),
    MAKE_BUMP(POS(14, 7), 1, &change_room, &cathedral, POS(1, 11)),
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

/*** Hallway ***/

static const char elephant_notice[] = "Elephants stop by their favorite food";

static const struct Bump hallway_bump[] = {
    MAKE_BUMP(POS(7, 11), 16, &change_room, &courtyard, POS(7, 3)),
    MAKE_BUMP(POS(8, 11), 16, &change_room, &courtyard, POS(8, 3)),
    MAKE_BUMP(POS(1, 3), -1, &change_room, &rampart, POS(14, 11)),
    MAKE_BUMP(POS(14, 3), 1, &change_room, &rampart, POS(1, 11)),
    MAKE_BUMP(POS(5, 7), -1, &change_room, &bedroom, POS(12, 7)),
    MAKE_BUMP(POS(10, 7), 1, &change_room, &passage, POS(1, 8)),
    MAKE_BUMP(POS(8, 3), -16, &bump_msg, elephant_notice, true),
};

static const struct Room hallway = {
    .msg = "Haunted Hallway",
    .map = map_of_hallway,
    .bump = hallway_bump,
    .count = SIZE(hallway_bump),
    .setup = &setup_hallway,
    .shamble = shamble_arrow,
};

/*** Bailey ***/

static const struct Bump bailey_bump[] = {
    MAKE_BUMP(POS(6, 2), -16, &change_room, &courtyard, POS(6, 11)),
    MAKE_BUMP(POS(7, 2), -16, &change_room, &courtyard, POS(7, 11)),
    MAKE_BUMP(POS(8, 2), -16, &change_room, &courtyard, POS(8, 11)),
    MAKE_BUMP(POS(9, 2), -16, &change_room, &courtyard, POS(9, 11)),
    MAKE_BUMP(POS(1, 6), -1, &change_room, &stables, POS(15, 6)),
    MAKE_BUMP(POS(1, 7), -1, &change_room, &stables, POS(15, 7)),
    MAKE_BUMP(POS(1, 8), -1, &change_room, &stables, POS(15, 8)),
    MAKE_BUMP(POS(14, 6), 1, &change_room, &training, POS(0, 6)),
    MAKE_BUMP(POS(14, 7), 1, &change_room, &training, POS(0, 7)),
    MAKE_BUMP(POS(14, 8), 1, &change_room, &training, POS(0, 8)),
};

static const struct Room bailey = {
    .msg = "Flooded Bailey",
    .map = map_of_bailey,
    .bump = bailey_bump,
    .count = SIZE(bailey_bump),
    .setup = setup_bailey,
    .shamble = shamble_ent,
};

/*** Cathedral ***/

static const struct Bump cathedral_bump[] = {
    MAKE_BUMP(POS(1, 7),  -1, &change_room, &courtyard, POS(14, 7)),
    MAKE_BUMP(POS(1, 8),  -1, &change_room, &courtyard, POS(14, 7)),
    MAKE_BUMP(POS(1, 9),  -1, &change_room, &courtyard, POS(14, 7)),
    MAKE_BUMP(POS(1, 10), -1, &change_room, &courtyard, POS(14, 7)),
    MAKE_BUMP(POS(1, 11), -1, &change_room, &courtyard, POS(14, 7)),
    MAKE_BUMP(POS(7, 7), -16, &change_room, &chancel, POS(7, 11)),
    MAKE_BUMP(POS(8, 7), -16, &change_room, &chancel, POS(8, 11)),
};

static const struct Room cathedral = {
    .msg = "Cathedral Entrance",
    .map = map_of_cathedral,
    .bump = cathedral_bump,
    .count = SIZE(cathedral_bump),
};

/*** Rampart ***/

static const struct Bump rampart_bump[] = {
    MAKE_BUMP(POS(1, 11), -1, &change_room, &hallway, POS(14, 3)),
    MAKE_BUMP(POS(14, 11), 1, &change_room, &hallway, POS(1, 3)),
};

static const struct Room rampart = {
    .msg = "Ravaged Rampart",
    .map = map_of_rampart,
    .bump = rampart_bump,
    .count = SIZE(rampart_bump),
    .setup = setup_rampart,
    .shamble = shamble_rook,
    .turn = strike_bosses,
};

/*** Chancel ***/

static const struct Bump chancel_bump[] = {
    MAKE_BUMP(POS(7, 11), 16, &change_room, &cathedral, POS(7, 7)),
    MAKE_BUMP(POS(8, 11), 16, &change_room, &cathedral, POS(8, 7)),
};

static const struct Room chancel = {
    .msg = "Altar of Duality",
    .map = map_of_chancel,
    .bump = chancel_bump,
    .count = SIZE(chancel_bump),
    .setup = setup_chancel,
    .shamble = shamble_bishop,
    .turn = strike_bosses,
};

/*** Stables ***/

static const struct Bump stables_bump[] = {
    MAKE_BUMP(POS(15, 6), 1, &change_room, &bailey, POS(1, 6)),
    MAKE_BUMP(POS(15, 7), 1, &change_room, &bailey, POS(1, 7)),
    MAKE_BUMP(POS(15, 8), 1, &change_room, &bailey, POS(1, 8)),
};

static const struct Room stables = {
    .msg = "Soiled Stables",
    .map = map_of_stables,
    .bump = stables_bump,
    .count = SIZE(stables_bump),
    .setup = setup_stables,
    .shamble = shamble_horse,
    .turn = strike_bosses,
};

/*** Training ***/

static const struct Bump training_bump[] = {
    MAKE_BUMP(POS(0, 6), -1, &change_room, &bailey, POS(14, 6)),
    MAKE_BUMP(POS(0, 7), -1, &change_room, &bailey, POS(14, 7)),
    MAKE_BUMP(POS(0, 8), -1, &change_room, &bailey, POS(14, 8)),
};

static const struct Room training = {
    .msg = "Archery Range",
    .map = map_of_training,
    .bump = training_bump,
    .count = SIZE(training_bump),
    .setup = setup_training,
    .shamble = shamble_soldier,
    .turn = strike_bosses,
};

/*** Bedroom ***/

static const char queen_line[] = "Looks like queen does all kinds of lines";

static const struct Bump bedroom_bump[] = {
    MAKE_BUMP(POS(12, 7), 1, &change_room, &hallway, POS(5, 7)),
    MAKE_BUMP(POS(5, 8), -1, &bump_msg, &queen_line, true),
};

static const struct Room bedroom = {
    .msg = "Queen's Boudoir",
    .map = map_of_bedroom,
    .bump = bedroom_bump,
    .count = SIZE(bedroom_bump),
    .setup = setup_bedroom,
    .shamble = shamble_queen,
    .turn = strike_bosses,
};

/*** Passage ***/

static const struct Bump passage_bump[] = {
    MAKE_BUMP(POS(1, 8), -1, &change_room, &hallway, POS(10, 7)),
    MAKE_BUMP(POS(7, 7), -16, &sealed_room, &throne, POS(1, 11)),
};

static const struct Room passage = {
    .msg = "Passage of Progress",
    .map = map_of_passage,
    .bump = passage_bump,
    .count = SIZE(passage_bump),
    .setup = setup_passage,
};

/*** Throne ***/

static const struct Bump throne_bump[] = {
    MAKE_BUMP(POS(1, 11), 0, &king_cutscene, NULL, 0),
    MAKE_BUMP(POS(1, 11), 1, &king_cutscene, NULL, 0),
    MAKE_BUMP(POS(1, 11), -16, &king_cutscene, NULL, 0),
    MAKE_BUMP(POS(1, 11), 16, &change_room, &passage, POS(7, 7)),
};

static const struct Room throne = {
    .msg = "Throne of Thorns",
    .map = map_of_throne,
    .bump = throne_bump,
    .count = SIZE(throne_bump),
    .shamble = shamble_throne,
    .turn = throne_turn,
};

static void setup_courtyard(void) {
    if (respawn != &courtyard) LEVEL[POS(8, 7)] -= TILE(1);
}

void startup_room(void) {
    door_broken = false;
    henchman = mobs + JOE;
    memset(beaten, 0, ALL);
    spawn_pos = POS(6, 6);
    respawn = &prison;
}
