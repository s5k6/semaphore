
vpcc = gcc -std=c99 -Wall -Werror -O2
doc = sem.html sem.1

.PHONY: all clean

all : sem $(doc)

clean :
	rm -rf sem *.inc $(doc)

sem : main.c short.inc license.inc
	$(vpcc) -o sem -DVERSION='"$(shell cat VERSION)"' -DSVNVERSION='"$(shell svnversion)"' -DCOMPILED_DATE='"$(shell date)"' -DCOMPILED_BY='"$(USER)@$(shell uname -n -o -m)"' main.c -lrt
	strip sem

%.inc : %.txt
	cat $< | sed 's/"/\\"/g' | sed 's/.*/"&\\n"/' > $@

sem.html : sem.markdown
	pandoc -f markdown -t html -s --base-header-level=2 -o $@ $<

sem.1 : sem.markdown
	pandoc -f markdown -t man -s -o $@ $<
