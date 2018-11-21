/**@note Error checking is not done in this file, or if it is done it should be
 * done with assertions, the parser does the validation and processing of the
 * input, if a returned object is not checked it is because it *must* exist (or
 * there is a bug in the grammar).
 * @note most values will be converted to doubles and then to integers, which
 * is fine for 32 bit values, but fails for large integers. */
#include "can.h"
#include "util.h"
#include <assert.h>
#include <stdlib.h>
#include <inttypes.h>
#include <math.h>

static signal_t *signal_new(void)
{
	return allocate(sizeof(signal_t));
}

static void signal_delete(signal_t *signal)
{
	if(!signal)
		return;
	for(size_t i = 0; i < signal->ecu_count; i++)
		free(signal->ecus[i]);
	free(signal->name);
	free(signal->ecus);
	free(signal->units);
	free(signal);
}

static can_msg_t *can_msg_new(void)
{
	return allocate(sizeof(can_msg_t));
}

static void can_msg_delete(can_msg_t *msg)
{
	if(!msg)
		return;
	for(size_t i = 0; i < msg->signal_count; i++)
		signal_delete(msg->signal_s[i]);
	free(msg->signal_s);
	free(msg->name);
	free(msg->ecu);
	free(msg);
}

static void y_mx_c(mpc_ast_t *ast, signal_t *sig)
{
	assert(ast && sig);
	mpc_ast_t *scalar = ast->children[1];
	mpc_ast_t *offset = ast->children[3];
	int r = sscanf(scalar->contents, "%lf", &sig->scaling);
	assert(r == 1);
	r = sscanf(offset->contents, "%lf", &sig->offset);
	assert(r == 1);
}

static void range(mpc_ast_t *ast, signal_t *sig)
{
	assert(ast && sig);
	mpc_ast_t *min = ast->children[1];
	mpc_ast_t *max = ast->children[3];
	int r = sscanf(min->contents, "%lf", &sig->minimum);
	assert(r == 1);
	r = sscanf(max->contents, "%lf", &sig->maximum);
	assert(r == 1);
}

/**@todo implement nodes function
static void nodes(mpc_ast_t *ast, signal_t *sig)
{
	assert(ast && sig);
}
*/

static void units(mpc_ast_t *ast, signal_t *sig)
{
	assert(ast && sig);
	mpc_ast_t *unit = mpc_ast_get_child(ast, "regex");
	sig->units = duplicate(unit->contents);
}

static int sigval(mpc_ast_t *top, unsigned id, const char *signal)
{
	assert(top);
	assert(signal);
	for(int i = 0; i >= 0;) {
		i = mpc_ast_get_index_lb(top, "sigval|>", i);
		if(i >= 0) {
			mpc_ast_t *sv = mpc_ast_get_child_lb(top, "sigval|>", i);
			mpc_ast_t *name   = mpc_ast_get_child(sv, "name|ident|regex");
			mpc_ast_t *svid = mpc_ast_get_child(sv,   "id|integer|regex");
			assert(name);
			assert(svid);
			unsigned svidd = 0;
			sscanf(svid->contents, "%u", &svidd);
			if(id == svidd && !strcmp(signal, name->contents)) {
				unsigned typed = 0;
				mpc_ast_t *type = mpc_ast_get_child(sv, "sigtype|integer|regex");
				sscanf(type->contents, "%u", &typed);
				debug("floating -> %s:%u:%u\n", name->contents, id, typed);
				return typed;
			}
			i++;
		}
	}
	return -1;
}

static signal_t *ast2signal(mpc_ast_t *top, mpc_ast_t *ast, unsigned can_id)
{
	int r;
	assert(ast);
	signal_t *sig = signal_new();
	mpc_ast_t *name   = mpc_ast_get_child(ast, "name|ident|regex");
	mpc_ast_t *start  = mpc_ast_get_child(ast, "startbit|integer|regex");
	mpc_ast_t *length = mpc_ast_get_child(ast, "length|regex");
	mpc_ast_t *endianess = mpc_ast_get_child(ast, "endianess|char");
	mpc_ast_t *sign   = mpc_ast_get_child(ast, "sign|char");
	sig->name = duplicate(name->contents);
	r = sscanf(start->contents, "%u", &sig->start_bit);
	assert(r == 1 && sig->start_bit <= 64);
	r = sscanf(length->contents, "%u", &sig->bit_length);
	assert(r == 1 && sig->bit_length <= 64);
	char endchar = endianess->contents[0];
	assert(endchar == '0' || endchar == '1');
	sig->endianess = endchar == '0' ?
		endianess_motorola_e :
		endianess_intel_e ;
	char signchar = sign->contents[0];
	assert(signchar == '+' || signchar == '-');
	sig->is_signed = signchar == '-';

	y_mx_c(mpc_ast_get_child(ast, "y_mx_c|>"), sig);
	range(mpc_ast_get_child(ast, "range|>"), sig);
	units(mpc_ast_get_child(ast, "unit|string|>"), sig);
	/*nodes(mpc_ast_get_child(ast, "nodes|node|ident|regex|>"), sig);*/

	/* process multiplexed values, if present */
	mpc_ast_t *multiplex = mpc_ast_get_child(ast, "multiplexor|>");
	if(multiplex) {
		sig->is_multiplexed = true;
		sig->switchval = atol(multiplex->children[1]->contents);
	}

	if(mpc_ast_get_child(ast, "multiplexor|char")) {
		assert(!sig->is_multiplexed);
		sig->is_multiplexor = true;
	}

	sig->sigval = sigval(top, can_id, sig->name);
	if(sig->sigval == 1 || sig->sigval == 2)
		sig->is_floating = true;

	debug("\tname => %s; start %u length %u %s %s %s",
			sig->name, sig->start_bit, sig->bit_length, sig->units,
			sig->endianess ? "intel" : "motorola",
			sig->is_signed ? "signed " : "unsigned");
	return sig;
}

static can_msg_t *ast2msg(mpc_ast_t *top, mpc_ast_t *ast)
{
	assert(top);
	assert(ast);
	can_msg_t *c = can_msg_new();
	mpc_ast_t *name = mpc_ast_get_child(ast, "name|ident|regex");
	mpc_ast_t *ecu  = mpc_ast_get_child(ast, "ecu|ident|regex");
	mpc_ast_t *dlc  = mpc_ast_get_child(ast, "dlc|integer|regex");
	mpc_ast_t *id   = mpc_ast_get_child(ast, "id|integer|regex");
	c->name = duplicate(name->contents);
	c->ecu  = duplicate(ecu->contents);
	int r = sscanf(dlc->contents, "%u", &c->dlc);
	assert(r == 1);
	r = sscanf(id->contents,  "%u", &c->id);
	assert(r == 1);

	/**@todo make test cases with no signals, and the like*/
    signal_t **signal_s = allocate(sizeof(*signal_s));
	size_t len = 1, j = 0;
	for(int i = 0; i >= 0;) {
		i = mpc_ast_get_index_lb(ast, "signal|>", i);
		if(i >= 0) {
			mpc_ast_t *sig_ast = mpc_ast_get_child_lb(ast, "signal|>", i);
			signal_s = reallocator(signal_s, sizeof(*signal_s)*++len);
			signal_s[j++] = ast2signal(top, sig_ast, c->id);
			i++;
		}
	}

	c->signal_s = signal_s;
	c->signal_count = j;

	if (c->signal_count > 1) { // Lets sort the signals so that their start_bit is asc (lowest number first)
		bool bFlip = false;
		do {
			bFlip = false;
			for (size_t i = 0; i < c->signal_count - 1; i++) {
				if (c->signal_s[i]->start_bit > c->signal_s[i + 1]->start_bit) {
					signal_t *tmp = c->signal_s[i];
					c->signal_s[i] = c->signal_s[i + 1];
					c->signal_s[i + 1] = tmp;
					bFlip = true;
				}
			}
		} while (bFlip);
	}

	debug("%s id:%u dlc:%u signals:%zu ecu:%s", c->name, c->id, c->dlc, c->signal_count, c->ecu);
	return c;
}

dbc_t *dbc_new(void)
{
	return allocate(sizeof(dbc_t));
}

void dbc_delete(dbc_t *dbc)
{
	if(!dbc)
		return;
	for(int i = 0; i < dbc->message_count; i++)
		can_msg_delete(dbc->messages[i]);
	free(dbc);
}

dbc_t *ast2dbc(mpc_ast_t *ast)
{
	const int index     = mpc_ast_get_index_lb(ast, "messages|>", 0);
	mpc_ast_t *msgs_ast = mpc_ast_get_child_lb(ast, "messages|>", 0);
	if(index < 0) {
		warning("no messages found");
		return NULL;
	}

	int n = msgs_ast->children_num;
	if(n <= 0) {
		warning("messages has no children");
		return NULL;
	}

	dbc_t *d = dbc_new();
	can_msg_t **r = allocate(sizeof(*r) * (n+1));
	int j = 0;
	for(int i = 0; i >= 0;) {
		i = mpc_ast_get_index_lb(msgs_ast, "message|>", i);
		if(i >= 0) {
			mpc_ast_t *msg_ast = mpc_ast_get_child_lb(msgs_ast, "message|>", i);
			r[j++] = ast2msg(ast, msg_ast);
			i++;
		}
	}
	d->message_count = j;
	d->messages = r;

	int i = mpc_ast_get_index_lb(ast, "sigval|>", 0);
	if (i >= 0)
		d->use_float = true;

	return d;
}


