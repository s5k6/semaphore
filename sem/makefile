# /makefile


# version information

version = $(shell cat VERSION)
svnversion = $(shell svnversion)
compiled_date = $(shell date)
compiled_by = $(USER)@$(shell uname -n -o -m)

export version svnversion compiled_by compiled_date


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

export USER MODE GROUP PREFIX 


# variables for creating a package

package = sem
archname = $(package)-$(shell cat VERSION)r$(shell svnversion)
archfile = $(archname).tar.bz2


# phony rules

.PHONY: all clean dist install

all install :
	$(MAKE) -C src $@
	$(MAKE) -C doc $@

clean :
	rm -f $(package)-* md5sum.txt
	$(MAKE) -C src $@
	$(MAKE) -C doc $@

dist : clean
	! svn status 2>/dev/null | read foo # fail with unversioned files
	md5sum * > md5sum.txt;
	md5sum -w --quiet -c md5sum.txt;
	tar --exclude-vcs --exclude '$(archfile)' --transform 's ^\. $(archname) g' -c . | bzip2 > "$(archfile)";
