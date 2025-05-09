ARCH ?= -mz80

MAKE := make --no-print-directory

CFLAGS += --nostdinc --nostdlib --no-std-crt0
CFLAGS += --code-loc $(CODE) --data-loc $(DATA)

all:	msg zxs
	@echo stamina build built

msg:
	@echo building stamina build

pcx:
	@gcc $(TYPE) pcx-dump.c -o pcx-dump
	@./pcx-dump -c bar.pcx > data.h
	@./pcx-dump -c title.pcx >> data.h
	@./pcx-dump -t richard.pcx >> data.h
	@./pcx-dump -t special.pcx >> data.h
	@./pcx-dump -t bricks.pcx >> data.h
	@./pcx-dump -l dungeon.pcx special.pcx bricks.pcx >> data.h

zxs:
	@$(MAKE) CODE=0x8000 DATA=0x7000 TYPE=-DZXS prg
	@bin2tap -b stamina.bin

prg: pcx
	@sdcc $(ARCH) $(CFLAGS) $(TYPE) main.c -o stamina.ihx
	@hex2bin stamina.ihx > /dev/null

fuse: zxs
	@echo running fuse emulator...
	@fuse --machine 128 --no-confirm-actions stamina.tap >/dev/null

clean:
	rm -f pcx-dump data.h stamina*
