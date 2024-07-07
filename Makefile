CFLAGS=-ggdb
LDFLAGS=-lssl -lcrypto

main: main.c
	gcc -o $@ $< $(CFLAGS) $(LDFLAGS)
