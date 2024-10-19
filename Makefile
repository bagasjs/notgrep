CC := clang
CFLAGS := -Wall -Wextra -pedantic -D_CRT_SECURE_NO_WARNINGS

all: grep.exe

grep.exe: notgrep.c ./btk_fsutil.c
	$(CC) $(CFLAGS) -o $@ $^ -lShlwapi

