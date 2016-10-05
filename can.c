#include "can.h"
#include "util.h"
#include <stdlib.h>

signal_t *signal_new(void)
{
	return allocate(sizeof(signal_t));
}

void signal_delete(signal_t *signal)
{
	if(signal)
		free(signal->name);
	for(size_t i = 0; i < signal->ecu_count; i++)
		free(signal->ecus[i]);
	free(signal->ecus);
	free(signal);
}

can_msg_t *can_msg_new(void)
{
	return allocate(sizeof(can_msg_t));
}

void can_msg_delete(can_msg_t *msg)
{
	if(!msg)
		return;
	for(size_t i = 0; i < msg->signal_count; i++)
		signal_delete(msg->signals[i]);
	free(msg->signals);
	free(msg->name);
	free(msg->ecu);
	free(msg);
}

signal_t *ast2signal(mpc_ast_t *ast)
{
	signal_t *sig = signal_new();
	note("signal %s", ast->contents);
	return sig;
}

can_msg_t *ast2msg(mpc_ast_t *ast)
{
	/**@todo clean this up so magic numbers are not used */
	can_msg_t *c = can_msg_new();
	mpc_ast_t *name  = mpc_ast_get_child_lb(ast, "name|ident|regex", 0);
	mpc_ast_t *ecu  = mpc_ast_get_child_lb(ast, "ecu|ident|regex", 0);
	mpc_ast_t *dlc = mpc_ast_get_child_lb(ast, "dlc|integer|regex", 0);
	mpc_ast_t *id  = mpc_ast_get_child_lb(ast, "id|integer|regex", 0);
	c->name = duplicate(name->contents);
	c->ecu  = duplicate(ecu->contents);
	sscanf(dlc->contents, "%u", &c->dlc);
	sscanf(id->contents,  "%u", &c->id);

	signal_t **signals = allocate(sizeof(*signals[0]));
	size_t len = 1;
	for(int i = 0; i >= 0;) {
		i = mpc_ast_get_index_lb(ast, "signal|>", i);
		if(i >= 0) {
			mpc_ast_t *sig_ast = mpc_ast_get_child_lb(ast, "signal|>", i);
			len++;
			signals = realloc(signals, sizeof(*signals[0])*len);
			signals[i] = ast2signal(sig_ast);
			i++;
		}
	}
	c->signals = signals;
	c->signal_count = len;

	note("%s id:%u dlc:%u signals:%zu ecu:%s", c->name, c->id, c->dlc, c->signal_count, c->ecu);

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
	int index          = mpc_ast_get_index_lb(ast, "messages|>", 0);
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
	for(int i = 0; i >= 0;) {
		i = mpc_ast_get_index_lb(msgs_ast, "message|>", i);
		if(i >= 0) {
			mpc_ast_t *msg_ast = mpc_ast_get_child_lb(msgs_ast, "message|>", i);
			r[i] = ast2msg(msg_ast);
			i++;
		}
	}
	d->message_count = n;
	d->messages = r;
	return d;
}


