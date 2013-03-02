
# the c compiler

vpcc = gcc -std=c99 -Wall -Werror -O2


# phony targets

.PHONY: all clean install

all : killme

clean :
	rm -rf killme


# production targets

killme : killme.c
	$(vpcc) -o killme killme.c
