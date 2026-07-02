CC := cc
CFLAGS := -std=c11 -Wall -Wextra -Wpedantic -Iinclude
LDFLAGS :=
LDLIBS := -lcurl -lcjson

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
	@mkdir -p bin
	$(CC) $(CFLAGS) tests/test_config.c src/config.c src/app_error.c $(LDFLAGS) $(LDLIBS) -o bin/test_config
	./bin/test_config

clean:
	rm -rf bin
