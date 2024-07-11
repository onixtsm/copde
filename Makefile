CFLAGS=-ggdb -fsanitize=address
LDFLAGS=-lssl -lcrypto
SOURCES=$(wildcard *.c)

copde: $(SOURCES)
	gcc -o $@ $^ $(CFLAGS) $(LDFLAGS)

release: $(SOURCES)
	gcc -o copde $^ -O2 $(LDFLAGS)

run:
	./copde

.PHONY: run release
