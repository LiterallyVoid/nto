CGFLAGS = -Wall -Wextra -O0 -g -fsanitize=undefined
CFLAGS = $(CGFLAGS)
LDFLAGS = $(CGFLAGS)

SRC = src/main.c
OBJ = $(SRC:%.c=build/%.o)

EXE = ds

build/%.o: %.c
	@mkdir -p $(@D)
	$(CC) $^ -c -o $@ $(CFLAGS)

$(EXE): $(OBJ)
	@mkdir -p $(@D)
	$(CC) $^ -o $@ $(LDFLAGS)
