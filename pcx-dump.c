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

static void __attribute__((unused)) hexdump(unsigned char *buf, int size) {
    for (int i = 0; i < size; i++) {
	fprintf(stderr, "%02x ", buf[i]);
	if ((i & 0xf) == 0xf) {
	    fprintf(stderr, "\n");
	}
    }
    if ((size & 0xf) != 0x0) fprintf(stderr, "\n");
}

static void dump_buffer(void *ptr, int size, int step) {
    for (int i = 0; i < size; i++) {
	if (step == 1) {
	    printf(" 0x%02x,", * (unsigned char *) ptr);
	}
	else {
	    printf(" 0x%04x,", * (unsigned short *) ptr);
	}
	if ((i & 7) == 7) printf("\n");
	ptr += step;
    }
    if ((size & 7) != 0) printf("\n");
}

static char replace_char(char c) {
    switch (c) {
    case '/':
	return '_';
    case '.':
	return 0;
    default:
	return c;
    }
}

static void remove_extension(char *dst, const char *src) {
    while (*src) { *dst++ = replace_char(*src++); }
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

static int read_file(const char *file, unsigned char **buf) {
    struct stat st;
    if (stat(file, &st) != 0) {
	fprintf(stderr, "ERROR file \"%s\" not found\n", file);
	exit(-ENOENT);
    }

    *buf = malloc(st.st_size);
    int in = open(file, O_RDONLY);
    read(in, *buf, st.st_size);
    close(in);

    return st.st_size;
}

static unsigned char *read_pcx(const char *file) {
    unsigned char *buf;
    int palette_offset = 16;
    int length = read_file(file, &buf);
    header.w = (* (unsigned short *) (buf + 0x8)) + 1;
    header.h = (* (unsigned short *) (buf + 0xa)) + 1;
    if (buf[3] == 8) palette_offset = length - 768;
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
		if (memcmp(ptr - j, ptr, i) == 0) {
		    update(next, current->weight + 2, i, j);
		}
	    }
	}
    }

    return encode(dst, src, nodes, size);
}

static void compress_and_save(const char *name, void *buf, int length) {
    unsigned char dst[estimate(length)];
    int size = compress(dst, buf, length);
    printf("static const byte %s[] = {\n", name);
    dump_buffer(dst, size, 1);
    printf("};\n");
}

static void compress_file(const char *file) {
    char name[256];
    unsigned char *buf;
    remove_extension(name, file);
    int length = read_file(file, &buf);
    compress_and_save(name, buf, length);
    free(buf);
}

int main(int argc, char **argv) {
    if (argc < 3) {
	printf("USAGE: pcx-dump [option] file.pcx\n");
	printf("  -c   save compressed image\n");
	printf("  -f   save compressed file\n");
	return 0;
    }

    option = argv[1][1];
    header.name = argv[2];

    switch (option) {
    case 'c':
	void *buf = read_pcx(header.name);
	if (buf == NULL) return -ENOENT;
	free(buf);
	break;
    case 'f':
	compress_file(header.name);
	break;
    }
    return 0;
}
