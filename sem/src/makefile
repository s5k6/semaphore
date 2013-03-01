# /src/makefile


# version information

version ?= $(shell cat VERSION)-$(shell svnversion -n || echo exported)
compiled_date ?= $(shell date)
compiled_by ?= $(USER)@$(shell uname -n -o -m)


# variables for install

ifeq ($(USER), root)
    MODE ?= 755
    GROUP ?= root
    PREFIX ?= /usr/local
else
    MODE ?= 700
    GROUP ?= $(shell groups | sed 's/ .*//g')
    PREFIX ?= $(HOME)/opt
endif


# the c compiler

vpcc = gcc -std=c99 -Wall -Werror -O2


# phony targets

.PHONY: all clean install

all : sem

clean :
	rm -rf sem *.inc

install : all
	install -o $(USER) -g $(GROUP) -m $(MODE) -D sem $(PREFIX)/bin/sem


# production targets

sem : main.c short.inc license.inc
	$(vpcc) -o sem -DVERSION='"$(version)"' -DCOMPILED_DATE='"$(compiled_date)"' -DCOMPILED_BY='"$(compiled_by)"' main.c -lrt
	strip sem

%.inc : %.txt
	cat $< | sed 's/"/\\"/g' | sed 's/.*/"&\\n"/' > $@

killme : killme.c
	$(vpcc) -o killme killme.c
