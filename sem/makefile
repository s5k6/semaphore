
vpcc = gcc -std=c99 -Wall -Werror -O2
doc = sem.html sem.1

package = sem
archname = $(package)-$(shell cat VERSION)_$(shell svnversion | tr : .)
archfile = $(archname).tar.bz2

.PHONY: default all clean dist

default :
	cat README

all : sem $(doc)

clean :
	rm -rf sem *.inc $(doc)
	rm -f $(package)-* md5sum.txt

sem : main.c short.inc license.inc
	$(vpcc) -o sem -DVERSION='"$(shell cat VERSION)"' -DSVNVERSION='"$(shell svnversion)"' -DCOMPILED_DATE='"$(shell date)"' -DCOMPILED_BY='"$(USER)@$(shell uname -n -o -m)"' main.c -lrt
	strip sem

%.inc : %.txt
	cat $< | sed 's/"/\\"/g' | sed 's/.*/"&\\n"/' > $@

sem.html : sem.markdown
	pandoc -f markdown -t html -s --base-header-level=2 -o $@ $<

sem.1 : sem.markdown
	pandoc -f markdown -t man -s -o $@ $<

dist : clean
	! svn status 2>/dev/null | read foo # fail if unversioned files
	md5sum * > md5sum.txt;
	md5sum -w --quiet -c md5sum.txt;
	tar --exclude-vcs --exclude '$(archfile)' --transform 's ^\. $(archname) g' -c . | bzip2 > "$(archfile)";


ifeq ($(USER), root)
    MODE ?= 755
    GROUP ?= root
    PREFIX ?= /usr/local
else
    MODE ?= 700
    GROUP ?= $(shell groups | sed 's/ .*//g')
    PREFIX ?= $(HOME)/opt
endif

install : all
	install -o $(USER) -g $(GROUP) -m $(MODE) -D sem $(PREFIX)/bin/sem
	install -o $(USER) -g $(GROUP) -m $(MODE) -D sem.1 $(PREFIX)/man/man1/sem.1
