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


# documentation files

doc = sem.html sem.1


# phony rules

.PHONY: all clean install

all : $(doc)

clean :
	rm -f $(doc)

install : all
	install -o $(USER) -g $(GROUP) -m $(MODE) -D sem.1 $(PREFIX)/man/man1/sem.1


# production rules

sem.html : sem.md pandoc-template.xhtml
	pandoc -f markdown -t html -s --base-header-level=2 --template=pandoc-template.xhtml -o $@ $<

sem.1 : sem.md
	pandoc -f markdown -t man -s -o $@ $<
