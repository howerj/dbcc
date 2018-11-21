#ifndef UTIL_H
#define UTIL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#define UNUSED(X) ((void)(X))

typedef enum {
	LOG_NO_MESSAGES,
	LOG_ERRORS,
	LOG_WARNINGS,
	LOG_NOTES,
	LOG_DEBUG,
	LOG_ALL_MESSAGES,
} log_level_e;

bool is_integer(double i);
double fractional(double x);
bool is_power_of_two(uint64_t n);
bool verbose(log_level_e level);
void set_log_level(log_level_e level);
log_level_e get_log_level(void);
const char *emsg(void);
void error(const char *fmt, ...);
void warning(const char *fmt, ...);
void note(const char *fmt, ...);
void debug(const char *fmt, ...);
FILE *fopen_or_die(const char *name, const char *mode);
void *allocate(size_t sz);
char *duplicate(const char *s);
void *reallocator(void *p, size_t n);
char *slurp(FILE *f);
char *dbcc_basename(char *s);

#ifdef __cplusplus
}
#endif

#endif
