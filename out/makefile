CC       = gcc
CFLAGS   = -Wall -Wextra -std=c99 -O2 -pedantic -fwrapv
RM      := rm -f

SOURCES := ${wildcard *.c}
OBJECTS := ${SOURCES:%.c=%.o}

.PHONY: all clean

all: ${OBJECTS}

%.o: %.c
	@echo cc $< -c -o $@
	@${CC} ${CFLAGS} ${INCLUDES} $< -c -o $@

clean:
	${RM} *.c *.h *.xml *.o *.xhtml *.csv *.bsm *.json
