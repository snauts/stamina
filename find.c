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

static const byte *deltas;
static byte map[160];
static byte count;
static byte head;

static byte visited(byte move, byte amount) {
    byte *node = map;
    while (amount-- > 0) {
	if (*node++ == move) return true;
    }
    return false;
}

void reset_a_star(const byte *moves) {
    head = 0;
    count = 0;
    deltas = moves;
    while (deltas[count++]);
}

void add_a_star_target(byte dst) {
    map[head++] = dst;
}

int8 a_star_search(byte src) {
    byte tail = 0;

    while (head > tail) {
	byte next = map[tail++];
	for (byte i = 0; i < count; i++) {
	    int8 delta = deltas[i];
	    byte move = next + delta;
	    if (move == src) return -delta;
	    if (is_occupied(move)) continue;
	    if (visited(move, head)) continue;
	    map[head++] = move;
	}
    }
    return 0;
}

static const int8 nearest[] = { -16, 16, -1, 1, 0 };

int8 a_star(byte src, byte dst) {
    reset_a_star(nearest);
    add_a_star_target(dst);
    return a_star_search(src);
}
