# version information

version ?= $(shell git describe --dirty --always --tags || cat VERSION)
compiled_date ?= $(shell date -Iseconds)
compiled_by ?= $(USER)@$(shell uname -n -o -m)

export version compiled_by compiled_date


# variables for install

ifeq ($(USER),root)
    MODE ?= 755
    GROUP ?= root
    PREFIX ?= /usr/local
else
    MODE ?= 700
    GROUP ?= $(shell id -gn)
    PREFIX ?= $(HOME)/.local
endif

export USER MODE GROUP PREFIX 


# the very pick c compiler

vpcc = gcc -std=c99 -O2 -Wall -Werror -Wextra -Wpedantic -Wbad-function-cast \
	-Wconversion -Wwrite-strings -Wstrict-prototypes

versionflags = -DVERSION='"$(version)"' -DCOMPILED_DATE='"$(compiled_date)"' \
	-DCOMPILED_BY='"$(compiled_by)"'


# phony targets

.PHONY: all clean install

all : sem

clean :
	rm -rf sem *.inc

install : all
	install -o $(USER) -g $(GROUP) -m $(MODE) -D sem $(PREFIX)/bin/sem


# production targets

sem : main.c short.inc license.inc
	$(vpcc) $(versionflags) -o sem main.c -lrt -pthread
	strip sem

%.inc : %.txt
	sed 's/"/\\"/g; s/.*/"&\\n"/' $< >$@

