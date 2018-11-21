#ifndef _2C_H
#define _2C_H

#ifdef __cplusplus
 extern "C" {
#endif

#include "can.h"
#include <stdbool.h>

int dbc2c(dbc_t *dbc, FILE *c, FILE *h, const char *name, bool use_time_stamps,
          bool generate_print, bool generate_pack, bool generate_unpack);

#ifdef __cplusplus
}
#endif

#endif
