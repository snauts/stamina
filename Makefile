ARCH ?= -mz80

MAKE := make --no-print-directory
SIZE := ls -l stamina.tap | cut -d " " -f 5

CFLAGS += --nostdinc --nostdlib --no-std-crt0
CFLAGS += --code-loc $(CODE) --data-loc $(DATA)

LFLAGS += -n -m -i -b _CODE=$(CODE) -b _DATA=$(DATA)

SRC := main.c room.c mobs.c
OBJ := $(subst .c,.o,$(SRC))

all:	msg zxs
	@echo stamina build built
	@echo zxs tape size $(shell $(SIZE))

msg:
	@echo building stamina build

pcx:
	@gcc $(TYPE) pcx-dump.c -o pcx-dump

	@./pcx-dump -c bar.pcx > data.h
	@./pcx-dump -c title.pcx >> data.h

	@./pcx-dump -t richard.pcx >> data.h
	@./pcx-dump -t diagonal.pcx >> data.h
	@./pcx-dump -t special.pcx >> data.h
	@./pcx-dump -t bricks.pcx >> data.h
	@./pcx-dump -t garden.pcx >> data.h
	@./pcx-dump -t bishop.pcx >> data.h
	@./pcx-dump -t walls.pcx >> data.h
	@./pcx-dump -t beast.pcx >> data.h
	@./pcx-dump -t horse.pcx >> data.h
	@./pcx-dump -t arrow.pcx >> data.h
	@./pcx-dump -t glass.pcx >> data.h
	@./pcx-dump -t tower.pcx >> data.h
	@./pcx-dump -t rook.pcx >> data.h
	@./pcx-dump -t ent.pcx >> data.h

	@./pcx-dump -l prison.pcx special.pcx bricks.pcx >> data.h
	@./pcx-dump -l tunnel.pcx special.pcx bricks.pcx >> data.h
	@./pcx-dump -l hallway.pcx special.pcx walls.pcx >> data.h
	@./pcx-dump -l dungeon.pcx special.pcx bricks.pcx >> data.h
	@./pcx-dump -l corridor.pcx special.pcx bricks.pcx >> data.h
	@./pcx-dump -l bailey.pcx special.pcx walls.pcx garden.pcx >> data.h
	@./pcx-dump -l courtyard.pcx special.pcx walls.pcx garden.pcx >> data.h
	@./pcx-dump -l cathedral.pcx special.pcx glass.pcx >> data.h
	@./pcx-dump -l rampart.pcx special.pcx tower.pcx >> data.h
	@./pcx-dump -l chancel.pcx diagonal.pcx glass.pcx >> data.h
	@./pcx-dump -l stables.pcx special.pcx garden.pcx >> data.h

zxs:
	@$(MAKE) CODE=0x8000 DATA=0x7000 TYPE=-DZXS prg
	@bin2tap -b stamina.bin

room.o: data.h

%.o: %.c main.h
	@echo compile source file $<
	@sdcc $(ARCH) $(CFLAGS) $(TYPE) -c $< -o $@

prg: pcx $(OBJ)
	@sdld $(LFLAGS) stamina.ihx $(OBJ)
	@hex2bin stamina.ihx > /dev/null

fuse: zxs
	@echo running fuse emulator...
	@fuse --machine 128 --no-confirm-actions stamina.tap >/dev/null

clean:
	rm -f pcx-dump data.h stamina* *.asm *.lst *.sym *.o
