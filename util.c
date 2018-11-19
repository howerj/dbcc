#include "util.h"
#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

static log_level_e log_level = LOG_NOTES;

bool is_integer(double i)
{
	double integral = 0, fractional = 0;
	fractional = modf(i, &integral);
	return fabs(fractional) < 0.000001; /*is the fractional bit zero?*/
}

bool is_power_of_two(uint64_t n)
{
	return (n > 0 && ((n & (n - 1)) == 0));
}

double fractional(double x)
{
	double i = 0;
	return modf(x, &i);
}

bool verbose(log_level_e level)
{
	return level <= log_level && log_level != LOG_NO_MESSAGES;
}

void set_log_level(log_level_e level)
{
	log_level = level;
}

log_level_e get_log_level(void)
{
	return log_level;
}

const char *emsg(void)
{
	return errno ? strerror(errno) : "unknown reason";
}

static void logmsg(log_level_e ll, const char *prefix, const char *fmt, va_list ap)
{
	assert(prefix && fmt && ll < LOG_ALL_MESSAGES);
	if(!verbose(ll))
		return;
	fputs(prefix , stderr);
	vfprintf(stderr, fmt, ap);
	fputc('\n', stderr);
}

#define LOG_INTERAL(LEVEL, PREFIX, FMT)\
	do {\
		va_list args;\
		assert(fmt);\
		va_start(args, fmt);\
		logmsg((LEVEL), (PREFIX), (FMT), args);\
		va_end(args);\
	} while(0);


void error(const char *fmt, ...)
{
	assert(fmt);
	LOG_INTERAL(LOG_ERRORS, "error: ", fmt);
	exit(EXIT_FAILURE);
}

void warning(const char *fmt, ...)
{
	assert(fmt);
	LOG_INTERAL(LOG_WARNINGS, "warning: ", fmt);
}

void note(const char *fmt, ...)
{
	assert(fmt);
	LOG_INTERAL(LOG_NOTES, "note: ", fmt);
}

void debug(const char *fmt, ...)
{
	assert(fmt);
	LOG_INTERAL(LOG_DEBUG, "debug: ", fmt);
}

FILE *fopen_or_die(const char *name, const char *mode)
{
	assert(name && mode);
	errno = 0;
	FILE *r = fopen(name, mode);
	if(!r)
		error("open '%s' (mode '%s'): %s", name, mode, emsg());
	return r;
}

void *allocate(size_t sz)
{
	errno = 0;
	void *r = calloc(sz, 1);
	if(!r)
		error("allocate failed: %s", emsg());
	return r;
}

void *reallocator(void *p, size_t n)
{
	void *r = realloc(p, n);
	if(!r)
		error("reallocator failed: %s", emsg());
	return r;
}

char *duplicate(const char *s)
{
	assert(s);
	size_t length = strlen(s) + 1;
	char *r = allocate(length);
	memcpy(r, s, length);
	return r;
}

/**@warning does not work for large file >4GB */
char *slurp(FILE *f)
{
	assert(f);
	long length = 0, r = 0;
	char *b = NULL;
	errno = 0;
	if((r = fseek(f, 0, SEEK_END)) < 0)
		goto fail;
	errno = 0;
	if((r = length = ftell(f)) < 0)
		goto fail;
	errno = 0;
	if((r = fseek(f, 0, SEEK_SET)) < 0)
		goto fail;
	b = allocate(length + 1);
	errno = 0;
	if((unsigned long)length != fread(b, 1, length, f))
		goto fail;
	return b;
fail:
	free(b);
	fprintf(stderr, "slurp failed: %s", emsg());
	return NULL;
}

/* Stolen from musl-libc!
 * <https://www.musl-libc.org/download.html>
 *
----------------------------------------------------------------------
Copyright © 2005-2014 Rich Felker, et al.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
----------------------------------------------------------------------
*/
char *dbcc_basename(char *s)
{
	assert(s);
	size_t i;
	if (!s || !*s) return ".";
	i = strlen(s)-1;
	for (; i&&s[i]=='/'; i--) s[i] = 0;
	for (; i&&s[i-1]!='/'; i--);
	return s+i;
}


