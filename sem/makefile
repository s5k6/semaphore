# /makefile


# version information

version = $(shell cat VERSION)r$(shell cat REVISION 2>/dev/null || svnversion -n || echo modified)
compiled_date = $(shell date)
compiled_by = $(USER)@$(shell uname -n -o -m)

export version compiled_by compiled_date


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
archname = $(package)-$(shell cat VERSION)


# phony rules

.PHONY: all clean dist install

all install :
	$(MAKE) -C src $@
	$(MAKE) -C doc $@

clean :
	rm -rf $(package)-* md5sum.txt
	$(MAKE) -C tools $@
	$(MAKE) -C src $@
	$(MAKE) -C doc $@

dist :
	svn export . '$(archname)'
	svnversion > '$(archname)/REVISION'
	find '$(archname)' -name md5sum.txt -prune -o -type f -exec md5sum {} \; | sed 's#$(archname)/##g' > '$(archname)/md5sum.txt'
	cd '$(archname)' && md5sum --quiet -c md5sum.txt;
	tar -c '$(archname)' | bzip2 > '$(archname).tar.bz2'
