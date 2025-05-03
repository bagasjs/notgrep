CC := clang
CFLAGS := -Wall -Wextra -pedantic -D_CRT_SECURE_NO_WARNINGS -Iinclude -g -fsanitize=address

all: grep.exe test.exe

test.exe: ./src/test.c
	$(CC) $(CFLAGS) -o $@ $^

grep.exe: ./src/notgrep.c
	$(CC) $(CFLAGS) -o $@ $^ -lShlwapi

