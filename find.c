#include "main.h"

static byte *ptr;
static byte data[8];

void reset_choices(void) {
    ptr = data - 1;
}

void add_choice(byte cost, byte value) {
    *(++ptr) = value;
    *(++ptr) = cost;
}

byte pick_choice(void) {
    byte value = 0;
    byte best = 255;
    while (ptr > data) {
	byte candidate = *ptr--;
	if (candidate <= best) {
	    value = *ptr;
	    best = candidate;
	}
	ptr--;
    }
    return value;
}

static const int8 *moves;
static byte map[160];
static byte head;

static byte visited(byte move, byte amount) {
    byte *node = map;
    while (amount-- > 0) {
	if (*node++ == move) return true;
    }
    return false;
}

void reset_a_star(const int8 *move_set) {
    moves = move_set;
    head = 0;
}

void add_a_star_target(byte dst) {
    if (!is_occupied(dst)) map[head++] = dst;
}

int8 a_star(byte src) {
    byte tail = 0;

    while (head > tail) {
	byte next = map[tail++];
	const int8 *deltas = moves;
	while (*deltas) {
	    int8 delta = *deltas++;
	    byte move = next + delta;
	    if (move == src) return -delta;
	    if (is_occupied(move)) continue;
	    if (visited(move, head)) continue;
	    map[head++] = move;
	}
    }
    return 0;
}

const int8 nearest[] = { -16, 16, -1, 1, 0 };

static void add_delta_targets(byte dst, const int8 *deltas) {
    do { add_a_star_target(dst + *deltas++); } while (*deltas);
}

int8 a_star_near(const int8 *move_set, byte src, byte dst) {
    reset_a_star(move_set);
    add_delta_targets(dst, nearest);
    return a_star(src);
}
