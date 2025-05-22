typedef signed char int8;
typedef signed short int16;
typedef unsigned char byte;
typedef unsigned short word;

#define false		0
#define true		1

#define NULL		((void *) 0)
#define ADDR(obj)	((word) (obj))
#define BYTE(addr)	(* (volatile byte *) (addr))
#define WORD(addr)	(* (volatile word *) (addr))
#define SIZE(array)	(sizeof(array) / sizeof(*(array)))
#define PTR(addr)	((byte *) (addr))

#define FRAME(x)	((x) << 5)
#define POS(x, y)	((byte) ((x) + ((y) << 4)))
#define X(pos)		((pos) & 0x0f)
#define Y(pos)		((pos) & 0xf0)

#define SCREEN(x)	PTR(0x4000 + (x))
#define COLOUR(x)	PTR(0x5800 + (x))
#define STAGING_AREA	PTR(0x5b00)

#define INK		(STAGING_AREA)
#define LEVEL		(INK + 0x260)
#define  TILE(id)		(id << 2)
#define  SET(id)		(id << 5)
#define  RIGHT			0
#define  LEFT			1
#define  FLIP			2
#define EMPTY		(LEVEL + 0xc0)
#define MOB(n)		(EMPTY + FRAME(64 + 8 * n))
#define  MOVING			0
#define  STANCE			1
#define  ATTACK			2
#define  BEATEN			4
#define  KILLED			5
#define  RESTED			6

#define MOVE_STAMINA	2
#define FULL_STAMINA	48
#define KILL_STAMINA	36

typedef byte(*Caller)(const void *, byte);

struct Mob {
    byte pos;
    byte img;
    byte ink;
    byte var;
};

typedef void(*Action)(struct Mob *);

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

extern struct Mob player;

void clear_message(void);
void game_idle(byte ticks);
void beep(word p, word len);
void advance_tile(byte pos);
byte is_walkable(byte place);
byte consume_stamina(byte amount);
void show_message(const char *msg);
void swoosh(int8 f, int8 n, int8 s);
byte load_room(const void *ptr, byte pos);
byte bump_msg(const void *text, byte ignore);
void draw_tile(byte *ptr, byte pos, byte id);
void *decompress(byte *dst, const byte *src);
void memcpy(void *dst, const void *src, word len);

void reset_choices(void);
void add_choice(byte cost, byte value);
byte pick_choice(void);

int8 a_star(byte src, byte dst);

void reset_mobs(void);
void shamble_mobs(void);
void startup_room(void);
void reset_actors(void);
void place_actors(void);
byte is_occupied(byte pos);
void hourglass(byte color);
byte is_dead(struct Mob *mob);
void add_actor(Action fn, struct Mob *mob);
void move_mob(struct Mob *mob, byte target);
void push_mob(struct Mob *mob, byte target);
void update_image(struct Mob *mob, byte tile);
void mob_direction(struct Mob *mob, int8 delta);
void animate_attack(struct Mob *mob, struct Mob *victim);
struct Mob *is_mob(byte pos);

void shoot_arrow(struct Mob *mob);
void shamble_beast(struct Mob *mob);
void shamble_ent(struct Mob *mob);
void shamble_rook(struct Mob *mob);
void shamble_bishop(struct Mob *mob);
void shamble_horse(struct Mob *mob);

enum {
/* dungeon */
    LARRY,
    BARRY,
    HARRY,
    
/* corridor */
    JURIS,
    ZIGIS,
    ROBIS,
    
/* hallway */
    ARROW1,
    ARROW2,
    
/* bailey */
    SKINBARK,
    LEAFLOCK,
    BREGALAD,
    BUSHKOPF,

/* ramparts */
    OSKAR,
    JAMES,

/* chancel */
    ISAAC,
    DAVID,

/* stables */
    PERSIJS,
    MARKUSS,

/* last entry */
    TOTAL_MOBS,
};
