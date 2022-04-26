CC = gcc
CFLAGS = -std=gnu99 -Wall -Wextra -Werror -pthread -pedantic

.PHONY: all run clean pack

all: proj2

proj2: proj2.c
	$(CC) $(CFLAGS) $^ -o $@

run: proj2
	./proj2 3 5 100 100

clean:
	rm -f *.o *.out *.zip proj2

pack:
	zip proj2.zip *.c *.h Makefile
