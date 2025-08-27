.DEFAULT_GOAL := build

SRC_DIR := src
SRC := $(wildcard $(SRC_DIR)/*.c)

build: $(SRC)
	gcc -Iinclude -g $(SRC)

debug: $(SRC)
	gcc -Iinclude -DDEBUG=1 -g -fsanitize=address -Wall $(SRC)

.PHONY: clean
clean:
	rm a.out