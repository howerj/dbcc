#ifndef _2C_H
#define _2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "can.h"
#include <stdbool.h>

typedef struct {
	bool use_id_in_name;
	bool use_time_stamps;
	bool use_doubles_for_encoding;
	bool generate_print, generate_pack, generate_unpack;
	bool generate_asserts;
} dbc2c_options_t;

int dbc2c(dbc_t *dbc, FILE *c, FILE *h, const char *name, dbc2c_options_t *copts);

#ifdef __cplusplus
}
#endif

#endif
