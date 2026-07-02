CC := cc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS :=
LDLIBS := -lcurl

SRC := $(wildcard src/*.c)
OBJ := $(patsubst src/%.c,bin/obj/%.o,$(SRC))

TARGET := bin/optimi-sftp-watcher

.PHONY: all clean test

all: $(TARGET)

$(TARGET): $(OBJ)
	@mkdir -p bin
	$(CC) $(OBJ) $(LDFLAGS) $(LDLIBS) -o $@


bin/obj/%.o: src/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

test:
	@echo "Test scaffold only. No tests implemented yet."

clean:
	rm -rf bin
