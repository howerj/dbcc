#ifndef CAN_H
#define CAN_H

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
	/**@todo fill in is_floating*/
	bool is_floating;    /**< if true, value is a floating point number*/
	bool is_multiplexor; /**< true if this is a multiplexor */
	bool is_multiplexed; /**< true if this is a multiplexed signal */
	unsigned switchval;  /**< if is_multiplexed, this will contain the
			       value that decodes this signal for the multiplexor */
} signal_t;

typedef struct {
	char *name;    /**< can message name */
	char *ecu;     /**< name of ECU @todo check this makes sense */
	signal_t **signals; /**< signals that can decode/encode this message*/
	uint64_t data; /**< data, up to eight bytes, not used for generation */
	size_t signal_count; /**< number of signals */
	unsigned dlc;  /**< length of CAN message 0-8 bytes */
	unsigned id;   /**< identifier, 11 or 29 bit */
} can_msg_t;

typedef struct {
	int message_count;
	can_msg_t **messages;
} dbc_t;

signal_t *signal_new(void);
void signal_delete(signal_t *signal);
can_msg_t *can_msg_new(void);
void can_msg_delete(can_msg_t *msg);
signal_t *ast2signal(mpc_ast_t *ast);
can_msg_t *ast2msg(mpc_ast_t *ast);
dbc_t *ast2dbc(mpc_ast_t *ast);
void dbc_delete(dbc_t *dbc);

#endif
