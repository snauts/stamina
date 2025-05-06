ARCH ?= -mz80

CFLAGS += --nostdinc --nostdlib --no-std-crt0
CFLAGS += --code-loc $(CODE) --data-loc $(DATA)

all:	zxs
	@echo Building up Stamina

pcx:
	@gcc $(TYPE) pcx-dump.c -o pcx-dump

zxs:
	CODE=0x8000 DATA=0x7000 TYPE=-DZXS make prg
	@bin2tap -b stamina.bin

prg: pcx
	@sdcc $(ARCH) $(CFLAGS) $(TYPE) main.c -o stamina.ihx
	@hex2bin stamina.ihx > /dev/null

fuse: zxs
	fuse --machine 128 --no-confirm-actions stamina.tap

clean:
	rm -f pcx-dump stamina*
