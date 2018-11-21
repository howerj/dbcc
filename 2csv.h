#ifndef _2CSV_H
#define _2CSV_H

#ifdef __cplusplus
extern "C" {
#endif

#include "can.h"

int dbc2csv(dbc_t *dbc, FILE *output);

#ifdef __cplusplus
}
#endif

#endif
