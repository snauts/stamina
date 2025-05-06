#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

static char option;

struct Header {
    char *name;
    short w, h;
} header;

void hexdump(unsigned char *buf, int size) {
    for (int i = 0; i < size; i++) {
	fprintf(stderr, "%02x ", buf[i]);
	if ((i & 0xf) == 0xf) {
	    fprintf(stderr, "\n");
	}
    }
    if ((size & 0xf) != 0x0) fprintf(stderr, "\n");
}

static unsigned char get_color(unsigned char *color) {
    unsigned char result = 0;
    if (color[0] >= 0x80) result |= 0x02;
    if (color[1] >= 0x80) result |= 0x04;
    if (color[2] >= 0x80) result |= 0x01;
    for (int i = 0; i < 3; i++) {
	if (color[i] > (result ? 0xf0 : 0x40)) {
	    result |= 0x40;
	    break;
	}
    }
    return result;
}

static unsigned char *read_pcx(const char *file) {
    struct stat st;
    int palette_offset = 16;
    if (stat(file, &st) != 0) {
	fprintf(stderr, "ERROR while opening PCX-file \"%s\"\n", file);
	return NULL;
    }
    unsigned char *buf = malloc(st.st_size);
    int in = open(file, O_RDONLY);
    read(in, buf, st.st_size);
    close(in);

    header.w = (* (unsigned short *) (buf + 0x8)) + 1;
    header.h = (* (unsigned short *) (buf + 0xa)) + 1;
    if (buf[3] == 8) palette_offset = st.st_size - 768;
    int unpacked_size = header.w * header.h / (buf[3] == 8 ? 1 : 2);
    unsigned char *pixels = malloc(unpacked_size);

    int i = 128, j = 0;
    while (j < unpacked_size) {
	if ((buf[i] & 0xc0) == 0xc0) {
	    int count = buf[i++] & 0x3f;
	    while (count-- > 0) {
		pixels[j++] = buf[i];
	    }
	    i++;
	}
	else {
	    pixels[j++] = buf[i++];
	}
    }

    for (i = 0; i < unpacked_size; i++) {
	int entry = palette_offset + 3 * pixels[i];
	pixels[i] = get_color(buf + entry);
    }

    free(buf);
    return pixels;
}

static int estimate(int size) {
    return 2 * size + 128;
}

static int min(int a, int b) {
    return a < b ? a : b;
}

struct Node {
    int weight;
    int length;
    int offset;
};

static int update(struct Node *next, int estimate, int i, int j) {
    if (next->weight > estimate) {
	next->weight = estimate;
	next->length = i;
	next->offset = j;
    }
}

static int encode(void *dst, void *src, struct Node *nodes, int i) {
    int total = nodes[i].weight + 1;
    memset(dst, 0, total);

    while (i > 0) {
	unsigned char *ptr = dst;
	struct Node *next = nodes + i;
	if (next->offset < 1) {
	    ptr += next->weight - next->length;
	    memcpy(ptr, src - next->offset, next->length);
	    ptr[-1] = next->length;
	}
	else {
	    ptr += next->weight - 2;
	    ptr[0] = 0x80 | next->length;
	    ptr[1] = next->offset;
	}
	i -= next->length;
    }

    return total;
}

static int compress(void *dst, void *src, int size) {
    int max = estimate(size);
    struct Node nodes[max];

    nodes[0].weight = 0;
    for (int i = 1; i < max; i++) {
	nodes[i].weight = max;
    }

    for (int pos = 0; pos <= size; pos++) {
	unsigned char *ptr = src + pos;
	struct Node *current = nodes + pos;
	for (int i = 1; i < min(128, size - pos + 1); i++) {
	    struct Node *next = current + i;
	    if (next->weight <= current->weight) break;
	    update(next, current->weight + i + 1, i, -pos);
	    for (int j = 1; j < min(256, pos + 1); j++) {
		if (i <= j && memcmp(ptr - j, ptr, i) == 0) {
		    update(next, current->weight + 2, i, j);
		}
	    }
	}
    }

    return encode(dst, src, nodes, size);
}

int main(int argc, char **argv) {
    if (argc < 3) {
	printf("USAGE: pcx-dump [option] file.pcx\n");
	printf("  -c   save compressed image\n");
	return 0;
    }

    option = argv[1][1];
    header.name = argv[2];

    void *buf = read_pcx(header.name);
    if (buf == NULL) return -ENOENT;

    free(buf);
    return 0;
}
