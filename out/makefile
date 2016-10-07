CC       = gcc
CFLAGS   = -Wall -Wextra -std=c99 -O2
RM      := rm

SOURCES := ${wildcard *.c}
OBJECTS := ${SOURCES:%.c=%.o}

.PHONY: all clean

all: ${OBJECTS}

%.o: %.c
	@echo cc $< -c -o $@
	@${CC} ${CFLAGS} ${INCLUDES} $< -c -o $@

clean:
	${RM} *.c *.h *.xml *.o
