CC      := gcc
CFLAGS  := -Wall -Wextra -Wpedantic -O2 -march=native
LDLIBS  := -lm
SDL     := $(shell pkg-config --cflags --libs sdl2 2>/dev/null || echo "-lSDL2 -lm")

BUILD   := build

all: $(BUILD)/kg $(BUILD)/kg_vis

$(BUILD)/kg: src/kg.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

$(BUILD)/kg_vis: src/kg_vis.c | $(BUILD)
	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS) $(SDL)

$(BUILD):
	mkdir -p $(BUILD)

run: $(BUILD)/kg
	$(BUILD)/kg text.txt

vis: $(BUILD)/kg_vis
	$(BUILD)/kg_vis text.txt

clean:
	rm -rf $(BUILD)
