
.PHONY: all clean install

all install clean :
	$(MAKE) -C src $@
	$(MAKE) -C doc $@
