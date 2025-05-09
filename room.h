struct Room {
    const char *msg;
    const void **map;
};

static const struct Room dungeon = {
    .msg = "Prison Cell",
    .map = map_of_dungeon,
};
