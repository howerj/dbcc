LDFLAGS  = -lm
CFLAGS   = -std=gnu99 -Wall -Wextra -g -O2
RM      := rm
OUTDIR  := out
SOURCES := ${wildcard *.c}
MDS     := ${wildcard *.md}
DOCS    := ${MDS:%.md=%.htm}
OBJECTS := ${SOURCES:%.c=%.o}
DEPS    := ${SOURCES:%.c=%.d}
CFLAGS  += -MMD
TARGET  := can

.PHONY: doc all clean 

all: ${TARGET}

%.o: %.c
	@echo cc $< -c -o $@
	@${CC} ${CFLAGS} ${INCLUDES} $< -c -o $@

${TARGET}: ${OBJECTS}
	@echo ${CC} $< -o $@
	@${CC} ${CFLAGS} $^ ${LDFLAGS} -o $@

doc: ${DOCS}

%.htm: %.md
	markdown $^ | tee $@ > /dev/null

-include ${DEPS}

clean:
	${RM} -f *.o *.d *.out ${TARGET} *.htm vgcore.* core
