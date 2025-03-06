.DEFAULT_GOAL := run

CC?=clang

CFLAGS=-I. -I./deps/arena -I./deps/arrays -I./deps/string_map -I./deps/strnstr

SOURCES = $(wildcard *.c)
SOURCES+= $(wildcard deps/arena/*.c)
SOURCES+= $(wildcard deps/arrays/*.c)
SOURCES+= $(wildcard deps/string_map/*.c)
SOURCES+= $(wildcard deps/strnstr/*.c)
OBJECTS=$(SOURCES:%.c=%.o)

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -rf $(OBJECTS) tests

tests: ./t/tests.c $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $< $(OBJECTS)

run: clean $(OBJECTS) tests
	./tests
