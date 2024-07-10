CFLAGS=-ggdb
LDFLAGS=-lssl -lcrypto
SOURCES=$(wildcard *.c)

copde: $(SOURCES)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

run:
	./copde

.PHONY: run
