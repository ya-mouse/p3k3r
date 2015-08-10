CFLAGS := -Wall -O2 -fomit-frame-pointer

SRC := p3k3r.c crc32.c z85.c
OBJ := $(SRC:.c=.o)

all: p3k3r

p3k3r: $(OBJ) Makefile
	$(CC) -o $@ $(OBJ)

clean:
	@rm -f $(OBJ) p3k3r
