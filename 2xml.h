#ifndef _2XML_H
#define _2XML_H

#ifdef __cplusplus
extern "C" {
#endif

#include "can.h"

int dbc2xml(dbc_t *dbc, FILE *output, bool use_time_stamps);

#ifdef __cplusplus
}
#endif

#endif
