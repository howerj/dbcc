LDFLAGS  = -lm
CFLAGS   = -std=c99 -Wall -Wextra -g -O2 -pedantic -fwrapv -DDBCC_VERSION="\"v1.2.2\""
RM      := rm
OUTDIR  := out
SOURCES := ${wildcard *.c}
MDS     := ${wildcard *.md}
HTMLS   := ${MDS:%.md=%.html}
PDFS    := ${MDS:%.md=%.pdf}
MANS    := ${MDS:%.md=%.1}
DBCS    := ${wildcard *.dbc}
OBJECTS := ${SOURCES:%.c=%.o}
LIBOBJS := $(filter-out main.o, $(OBJECTS))
DEPS    := ${SOURCES:%.c=%.d}
XMLS    := ${DBCS:%.dbc=${OUTDIR}/%.xml}
XHTMLS  := ${XMLS:%.xml=%.xhtml}
CODECS  := ${DBCS:%.dbc=${OUTDIR}/%.c}
CFLAGS  += -MMD
TARGET  := dbcc

.PHONY: doc all run clean test

all: ${TARGET}

%.o: %.c
	${CC} ${CFLAGS} ${INCLUDES} $< -c -o $@

%.1: %.md
	pandoc --standalone --to man -o$@ $<

%.html: %.md
	pandoc -o $@ $<

%.pdf: %.md
	pandoc -o $@ $<

lib${TARGET}.a: ${OBJECTS}
	ar rcs $@ ${OBJECTS}
	ranlib $@

${TARGET}: ${OBJECTS}
	${CC} ${CFLAGS} $^ ${LDFLAGS} -o $@

${OUTDIR}/%.c: %.dbc ${TARGET}
	./${TARGET} ${DBCCFLAGS} -o ${OUTDIR} $<

${OUTDIR}/%.xml: %.dbc ${TARGET}
	./${TARGET} ${DBCCFLAGS} -x -o ${OUTDIR} $<
	xmllint --noout --schema dbcc.xsd $@

${OUTDIR}/%.csv: %.dbc ${TARGET}
	./${TARGET} ${DBCCFLAGS} -C -o ${OUTDIR} $<

${OUTDIR}/%.json: %.dbc ${TARGET}
	./${TARGET} ${DBCCFLAGS} -j -o ${OUTDIR} $<

%.xhtml: %.xml dbcc.xslt
	xsltproc --output $@ dbcc.xslt $<


run: ${XMLS} ${CODECS} ${XHTMLS}

TESTS=${OUTDIR}/ex1.c \
      ${OUTDIR}/ex2.c \
      ${OUTDIR}/double_signal.c \
      ${OUTDIR}/float_signal.c \
      ${OUTDIR}/ex1.xml \
      ${OUTDIR}/ex2.xml \
      ${OUTDIR}/ex1.csv \
      ${OUTDIR}/ex2.csv \
      ${OUTDIR}/ex1.json \
      ${OUTDIR}/ex2.json \
      ${OUTDIR}/enum.c

test: ${TESTS}
	make -C ${OUTDIR}

doc: ${HTMLS} ${MANS} ${PDFS}

-include ${DEPS}

clean:
	${RM} -f *.o *.d *.out ${TARGET} *.htm vgcore.* core
