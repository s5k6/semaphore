
vpcc = gcc -std=c99 -Wall -Werror -O2

.PHONY: all clean

all : sem

clean :
	rm -rf sem *.inc

sem : main.c short.inc long.inc license.inc version.inc
	$(vpcc) -o sem main.c -lrt
	strip sem

%.inc : %.txt
	cat $< | sed 's/"/\\"/g' | sed 's/.*/"&\\n"/' > $@
