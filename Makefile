CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -std=c11 -O2 -march=native
CFLAGS  += -D_GNU_SOURCE

SRC     := kg.c
TARGET  := kg

DEBUG_TARGET := kg_debug
DEBUG_CFLAGS  := -O0 -g -fsanitize=address,undefined

.PHONY: all clean debug run rund valgrind rebuild help

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $<
	@echo "Build complete → ./$(TARGET)"

debug: $(DEBUG_TARGET)

$(DEBUG_TARGET): $(SRC)
	$(CC) $(CFLAGS) $(DEBUG_CFLAGS) -o $@ $<
	@echo "Debug build with sanitizers → ./$(DEBUG_TARGET)"

run: $(TARGET)
	./$(TARGET)

rund: debug
	./$(DEBUG_TARGET)

valgrind: debug
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(DEBUG_TARGET)

clean:
	rm -f $(TARGET) $(DEBUG_TARGET)

rebuild: clean all

help:
	@echo "Targets:"
	@echo "  all      – normal optimized build"
	@echo "  debug    – build with -fsanitize=address,undefined"
	@echo "  run      – build + run normal"
	@echo "  rund     – debug + run"
	@echo "  valgrind – run under valgrind"
	@echo "  clean    – remove binaries"
	@echo "  rebuild  – clean + build"
