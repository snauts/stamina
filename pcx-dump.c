#include <sys/stat.h>
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
    return 2 * size;
}

static int compress(unsigned char *dst, unsigned char *src, int size) {
    int max = estimate(size);
    unsigned char work[max];
    int best[max];

    for (int i = 0; i < max; i++) {
	best[i] = max;
    }

    void finalize(int len) {
	work[len++] = 0;
	if (len <= max) {
	    memcpy(dst, work, len);
	    max = len;
	}
    }

    bool match(unsigned char *ptr, int offset, int amount) {
	return amount <= offset
	    && ptr - src >= offset
	    && !memcmp(ptr - offset, ptr, amount);
    }

    void find_best(int pos, int len) {
	if (best[pos] > len) {
	    best[pos] = len;

	    if (pos >= size) {
		finalize(len);
	    }
	    else {
		unsigned char *ptr = src + pos;
		for (int amount = 1; amount < 128; amount++) {
		    if (amount <= size - pos) {
			work[len] = amount;
			memcpy(work + len + 1, ptr, amount);
			find_best(pos + amount, len + amount + 1);
		    }

		    for (int offset = 1; offset < 256; offset++) {
			if (match(ptr, offset, amount)) {
			    work[len + 0] = 0x80 | amount;
			    work[len + 1] = offset;
			    find_best(pos + amount, len + 2);
			}
		    }
		}
	    }
	}
    }

    find_best(0, 0);
    return max;
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
