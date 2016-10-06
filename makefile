LDFLAGS  = -lm
CFLAGS   = -std=c99 -Wall -Wextra -g -O2
RM      := rm
OUTDIR  := out
SOURCES := ${wildcard *.c}
MDS     := ${wildcard *.md}
DBCS    := ${wildcard *.dbc}
DOCS    := ${MDS:%.md=%.htm}
OBJECTS := ${SOURCES:%.c=%.o}
DEPS    := ${SOURCES:%.c=%.d}
XMLS    := ${DBCS:%.dbc=${OUTDIR}/%.xml}
CODECS  := ${DBCS:%.dbc=${OUTDIR}/%.c}
CFLAGS  += -MMD
TARGET  := dbcc

.PHONY: doc all run clean 

all: ${TARGET}

%.o: %.c
	@echo ${CC} $< -c -o $@
	@${CC} ${CFLAGS} ${INCLUDES} $< -c -o $@

${TARGET}: ${OBJECTS}
	@echo ${CC} $< -o $@
	@${CC} ${CFLAGS} $^ ${LDFLAGS} -o $@

${OUTDIR}/%.xml: %.dbc ${TARGET}
	./${TARGET} -x -o ${OUTDIR} $<

${OUTDIR}/%.c: %.dbc ${TARGET}
	./${TARGET} -o ${OUTDIR} $<

run: ${XMLS} ${CODECS}

doc: ${DOCS}

%.htm: %.md
	markdown $^ | tee $@ > /dev/null

-include ${DEPS}

clean:
	${RM} -f *.o *.d *.out ${TARGET} *.htm vgcore.* core
