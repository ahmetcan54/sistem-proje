CC      = gcc
CFLAGS  = -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L

all: soru1 soru2

soru1: soru1.c
	$(CC) $(CFLAGS) -o soru1 soru1.c

soru2: soru2.c
	$(CC) $(CFLAGS) -o soru2 soru2.c

clean:
	rm -f soru1 soru2
