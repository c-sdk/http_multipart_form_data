.DEFAULT_GOAL := run

DEBUG?=0

CC?=clang

CFLAGS=-I.
CFLAGS+= $(foreach X,$(shell ls deps), -I./deps/$(X))

ifeq ("$(DEBUG)", "1")
CFLAGS+= -g
CFLAGS+= -DLOG_HTTP_MULTIPART_FORM_DATA=1
endif

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
