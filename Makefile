# /makefile


# version information

version = $(shell git describe --dirty --always --tags || cat VERSION)
compiled_date = $(shell date -Iseconds)
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
    PREFIX ?= $(HOME)/.local
endif

export USER MODE GROUP PREFIX 



.PHONY: all clean dist install

all install :
	$(MAKE) -C src $@
	$(MAKE) -C doc $@

clean :
	rm -rf $(package)-* md5sum.txt
	$(MAKE) -C tools $@
	$(MAKE) -C src $@
	$(MAKE) -C doc $@
