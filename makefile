LDFLAGS  = 
CFLAGS   = -std=c99 -Wall -Wextra -g -O2
RM      := rm
SOURCES := ${wildcard *.c}
OBJECTS := ${SOURCES:%.c=%.o}
DEPS    := ${SOURCES:%.c=%.d}
CFLAGS  += -MMD
TARGET  := dbcc

.PHONY: doc all clean 

all: mpc.h ${TARGET}

%.o: %.c
	@echo ${CC} $< -c -o $@
	@${CC} ${CFLAGS} ${INCLUDES} $< -c -o $@

${TARGET}: ${OBJECTS}
	@echo ${CC} $< -o $@
	@${CC} ${CFLAGS} $^ ${LDFLAGS} -o $@

run: ${TARGET}
	./${TARGET} ex1.dbc

doc: dbcc.htm

dbcc.htm: readme.md
	markdown $^ | tee $@

-include ${DEPS}

clean:
	${RM} -f *.o *.d *.out ${TARGET} *.htm vgcore.* core
