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

static byte visited(byte *map, byte move, byte head) {
    while (head-- > 0) {
	if (*map++ == move) return true;
    }
    return false;
}

int8 a_star(byte src, byte dst) {
    static const int8 deltas[] = { -16, 16, -1, 1 };
    static byte map[160];
    byte head, tail;
    head = tail = 0;

    map[head++] = dst;
    while (head > tail) {
	byte next = map[tail++];
	for (byte i = 0; i < SIZE(deltas); i++) {
	    int8 delta = deltas[i];
	    byte move = next + delta;
	    if (move == src) return -delta;
	    if (is_occupied(move)) continue;
	    if (visited(map, move, head)) continue;
	    map[head++] = move;
	}
    }
    return 0;
}
