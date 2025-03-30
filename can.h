#ifndef CAN_H
#define CAN_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "mpc.h"

typedef enum {
	endianess_motorola_e = 0,
	endianess_intel_e = 1,
} endianess_e;

typedef enum {
	numeric_unsigned_e,
	numeric_signed_e,
	numeric_floating_e,
} numeric_e;

typedef struct {
	char *name;
	unsigned value;
} val_list_item_t;

typedef struct {
	size_t val_list_item_count;
	val_list_item_t **val_list_items;
	unsigned id;   /**< identifier, 11 or 29 bit */
	char *name;
} val_list_t;

typedef struct {
	char *multiplexed;
	char *multiplexor;
	unsigned min_value;
	unsigned max_value;
	unsigned id;   /**< identifier, 11 or 29 bit */
} mul_val_list_t;

typedef struct signal_t signal_t;

struct signal_t {
	size_t ecu_count;    /**< ECU count */
	char *units;         /**< units used */
	char **ecus;         /**< ECUs sending/receiving */
	char *name;          /**< name of the signal */
	double scaling;      /**< scaling */
	double offset;       /**< offset */
	double minimum;      /**< minimum value */
	double maximum;      /**< maximum value */
	unsigned bit_length; /**< bit length in message buffer */
	unsigned start_bit;  /**< starting bit position in message */
	endianess_e endianess; /**< endianess of message */
	bool is_signed;      /**< if true, value is signed */
	bool is_floating;    /**< if true, value is a floating point number*/
	unsigned sigval;     /**< 1 == float, 2 == double. is_floating implies sigval == 1 || sigval == 2 */
	bool is_multiplexor; /**< true if this is a multiplexor */
	bool is_multiplexed; /**< true if this is a multiplexed signal */
	unsigned switchval;  /**< if is_multiplexed, this will contain the
				   value that decodes this signal for the multiplexor */
	val_list_t *val_list;
	size_t mul_num;      /**< number of multiplexed signals */
	signal_t **muxed;    /**< list of multiplexed signals */
	mul_val_list_t **mux_vals; /**< list of mux_vals relative to signals */
	char *comment;
};

typedef struct {
	char *name;          /**< can message name */
	char *ecu;           /**< name of ECU */
	signal_t **sigs;     /**< signals that can decode/encode this message*/
	uint64_t data;       /**< data, up to eight bytes, not used for generation */
	size_t signal_count; /**< number of signals */
	unsigned dlc;        /**< length of CAN message 0-8 bytes */
	unsigned long id;    /**< identifier, 11 or 29 bit */
	bool is_extended;    /**< is extended mode message (29bit) */
	char *comment;
} can_msg_t;

typedef struct {
	bool use_float;       /**< true if floating point conversion routines are needed */
	size_t message_count; /**< count of messages */
	can_msg_t **messages; /**< list of messages */
	size_t val_count;     /**< count of vals */
	val_list_t **vals;    /**< value list; used for enumerations in DBC file */
	size_t mul_val_count; /**< count of mul_vals*/
	mul_val_list_t **mul_vals; /**< multiplexed value list; used for multiplexed signals in DBC file */
	int version;          /**< version information used for generating files (not just C) */
	char *dbc_version;    /**< Raw version string from DBC file's "VERSION" line (e.g. "1.0"), may be NULL if undefined */
} dbc_t;

dbc_t *ast2dbc(mpc_ast_t *ast);
void dbc_delete(dbc_t *dbc);

#ifdef __cplusplus
}
#endif

#endif
