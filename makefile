LDFLAGS  = -lm
CFLAGS   = -std=c99 -Wall -Wextra -g -O2 -pedantic
RM      := rm
OUTDIR  := out
SOURCES := ${wildcard *.c}
MDS     := ${wildcard *.md}
HTMLS   := ${MDS:%.md=%.html}
PDFS    := ${MDS:%.md=%.pdf}
MANS    := ${MDS:%.md=%.1}
DBCS    := ${wildcard *.dbc}
OBJECTS := ${SOURCES:%.c=%.o}
DEPS    := ${SOURCES:%.c=%.d}
XMLS    := ${DBCS:%.dbc=${OUTDIR}/%.xml}
XHTMLS  := ${XMLS:%.xml=%.xhtml}
CODECS  := ${DBCS:%.dbc=${OUTDIR}/%.c}
CFLAGS  += -MMD
TARGET  := dbcc

.PHONY: doc all run clean test

all: ${TARGET}

%.o: %.c
	@echo cc $< -c -o $@
	@${CC} ${CFLAGS} ${INCLUDES} $< -c -o $@

%.1: %.md
	pandoc --standalone --to man -o$@ $<

%.html: %.md
	pandoc -o $@ $<

%.pdf: %.md
	pandoc -o $@ $<

${TARGET}: ${OBJECTS}
	@echo ${CC} $< -o $@
	@${CC} ${CFLAGS} $^ ${LDFLAGS} -o $@

${OUTDIR}/%.xml: %.dbc ${TARGET}
	./${TARGET} ${DBCCFLAGS} -x -o ${OUTDIR} $<
	xmllint --noout --schema dbcc.xsd $@

%.xhtml: %.xml dbcc.xslt
	xsltproc --output $@ dbcc.xslt $<

${OUTDIR}/%.c: %.dbc ${TARGET}
	./${TARGET} ${DBCCFLAGS} -o ${OUTDIR} $<

run: ${XMLS} ${CODECS} ${XHTMLS}

test: ${OUTDIR}/ex1.c ${OUTDIR}/ex2.c ${OUTDIR}/ex1.xml ${OUTDIR}/ex2.xml
	make -C ${OUTDIR}

doc: ${HTMLS} ${MANS} ${PDFS}

-include ${DEPS}

clean:
	${RM} -f *.o *.d *.out ${TARGET} *.htm vgcore.* core
