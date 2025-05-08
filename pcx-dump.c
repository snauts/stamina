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

static int report(int size, int total) {
    int percent = total * 100 / size;
    fprintf(stderr, "compress from %d to %d (%d%)\n", size, total, percent);
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

    return report(size, encode(dst, src, nodes, size));
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
    fprintf(stderr, "dumping binary file %s\n", file);
    compress_and_save(name, buf, length);
    free(buf);
}

static int ink_index(int i) {
    return (i / header.w / 8) * (header.w / 8) + i % header.w / 8;
}

static unsigned short encode_pixel(unsigned char a, unsigned char b) {
    return a > b ? (b << 8) | a : (a << 8) | b;
}

static unsigned char is_bright(unsigned char f, unsigned char b) {
    return (f > 7 || b > 7) ? 0x40 : 0x00;
}

static unsigned char encode_ink(unsigned short colors) {
    unsigned char b = colors >> 8;
    unsigned char f = colors & 0xff;
    return is_bright(f, b) | (f & 7) | ((b & 7) << 3);
}

static unsigned char consume_pixels(unsigned char *buf, unsigned char on) {
    unsigned char ret = 0;
    for (int i = 0; i < 8; i++) {
	ret = ret << 1;
	ret |= (buf[i] == on || (buf[i] != 0 && option == 'p')) ? 1 : 0;
    }
    return ret;
}

static unsigned short on_pixel(unsigned char *buf, int i, int w) {
    unsigned char pixel = buf[i];
    for (int y = 0; y < 8; y++) {
	for (int x = 0; x < 8; x++) {
	    unsigned char next = buf[i + x];
	    if (next != pixel) {
		return encode_pixel(next, pixel);
	    }
	}
	i += w;
    }
    return pixel == 0 ? 5 : pixel;
}

static void save_image(const char *name, unsigned char *data, int size) {
    char variable_name[256];
    remove_extension(variable_name, name);
    fprintf(stderr, "dumping pcx image %s\n", name);
    compress_and_save(variable_name, data, size);
}

static void serialize_tiles(void *ptr, int size) {
    int index = 0;
    unsigned short *buf = ptr;
    unsigned short tiles[size / 2];
    for (int y = 0; y < header.h; y += 16) {
	for (int x = 0; x < header.w; x += 16) {
	    for (int i = 0; i < 16; i++) {
		int offset = ((y + i) * header.w + x) / 8;
		tiles[index++] = buf[offset / 2];
	    }
	}
    }
    memcpy(ptr, tiles, size);
}

static unsigned short flip_bits(unsigned short source) {
    unsigned short result = 0;
    for (int i = 0; i < 16; i++) {
	result = result << 1;
	result |= source & 1;
	source = source >> 1;
    }
    return result;
}

static void flip_vertical(void *ptr, int size) {
    int index = 0;
    unsigned short *buf = ptr;
    unsigned short tiles[size / 2];
    for (int i = 0; i < size; i += 32) {
	for (int y = 15; y >= 0; y--) {
	    tiles[index++] = buf[i / 2 + y];
	}
    }
    memcpy(ptr, tiles, size);
}

static void flip_horizontal(void *ptr, int size) {
    int index = 0;
    unsigned short *buf = ptr;
    unsigned short tiles[size / 2];
    for (int i = 0; i < size / 2; i++) {
	tiles[i] = flip_bits(buf[i]);
    }
    memcpy(ptr, tiles, size);
}

static int get_tile_id(void *tile, void *set, int size) {
    for (int offset = 0; offset < size; offset += 32) {
	if (memcmp(tile, set + offset, 32) == 0) return offset / 32;
    }
    return -1;
}

static int image_size(void) {
    return header.w * header.h;
}

static int pixel_size(void) {
    return image_size() / 8;
}

static int color_size(void) {
    return image_size() / 64;
}

static int total_size(void) {
    return pixel_size() + color_size();
}

static void *convert_bitmap(unsigned char *buf) {
    int j = 0;
    unsigned char pixel[total_size()];
    unsigned char *color = pixel + pixel_size();
    unsigned short on[color_size()];

    for (int i = 0; i < image_size(); i += 8) {
	if (i / header.w % 8 == 0) {
	    on[j++] = on_pixel(buf, i, header.w);
	}
	unsigned char data = on[ink_index(i)] & 0xff;
	pixel[i / 8] = consume_pixels(buf + i, data);
    }
    for (int i = 0; i < color_size(); i++) {
	color[i] = encode_ink(on[i]);
    }

    memcpy(buf, pixel, total_size());
    return buf;
}

static void *load_bitmap(const char *name) {
    return convert_bitmap(read_pcx(name));
}

static void save_bitmap(const char *name) {
    unsigned char *buf = load_bitmap(name);

    switch (option) {
    case 't':
	serialize_tiles(buf, pixel_size());
	save_image(name, buf, pixel_size());
	break;
    case 'c':
	save_image(name, buf, total_size());
	break;
    }

    free(buf);
}

int main(int argc, char **argv) {
    if (argc < 3) {
	fprintf(stderr, "USAGE: pcx-dump [option] file.pcx [extra]\n");
	fprintf(stderr, "  -c   dump compressed image\n");
	fprintf(stderr, "  -t   dump compressed tiles\n");
	fprintf(stderr, "  -f   dump compressed file\n");
	return 0;
    }

    option = argv[1][1];

    switch (option) {
    case 'c':
    case 't':
	save_bitmap(argv[2]);
	break;
    case 'f':
	compress_file(argv[2]);
	break;
    }
    return 0;
}
