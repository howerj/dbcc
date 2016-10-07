/**@todo Multiplexed signals are not handled
 * @todo DLC processing
 * @todo Validation functions with configurable behavior (range checks, etc)
 * @todo Tidy up this module, make things configurable*/

#include "2c.h"
#include "util.h"
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <string.h>
#include <time.h>

#define MAX_NAME_LENGTH (512u)

static const bool swap_motorola = true;

static unsigned fix_start_bit(bool motorola, unsigned start)
{
	if(motorola)
		start = (8 * (7 - (start / 8))) + (start % 8);
	return start;
}

static const char *determine_unsigned_type(unsigned length)
{
	const char *type = "uint64_t";
	if(length <= 32)
		type = "uint32_t";
	if(length <= 16)
		type = "uint16_t";
	if(length <= 8)
		type = "uint8_t";
	return type;
}

static const char *determine_signed_type(unsigned length)
{
	const char *type = "int64_t";
	if(length <= 32)
		type = "int32_t";
	if(length <= 16)
		type = "int16_t";
	if(length <= 8)
		type = "int8_t";
	return type;
}

static const char *determine_type(unsigned length, bool is_signed)
{
	return is_signed ?
		determine_signed_type(length) :
		determine_unsigned_type(length);
}

static int comment(signal_t *sig, FILE *o)
{
	assert(sig && o);
	return fprintf(o, "\t/* %s: start-bit %u, length %u, endianess %s, scaling %lf, offset %lf */\n",
			sig->name,
			sig->start_bit,
			sig->bit_length,
			sig->endianess == endianess_motorola_e ? "motorola" : "intel",
	      		sig->scaling,
			sig->offset);
}

static int signal2deserializer(signal_t *sig, FILE *o)
{
	assert(sig && o);
	bool motorola   = sig->endianess == endianess_motorola_e;
	unsigned start  = fix_start_bit(motorola, sig->start_bit);
	unsigned length = sig->bit_length;
	uint64_t mask = length == 64 ?
		0xFFFFFFFFFFFFFFFFuLL :
		(1uLL << length) - 1uLL;

	if(comment(sig, o) < 0)
		return -1;

	if(start)
		fprintf(o, "\tx = (%c >> %d) & 0x%"PRIx64";\n", motorola ? 'm' : 'i', start, mask);
	else
		fprintf(o, "\tx = %c & 0x%"PRIx64";\n", motorola ? 'm' : 'i',  mask);

	if(sig->is_floating) {
		assert(length == 32 || length == 64);
		if(fprintf(o, "\tunpack->%s = *((%s*)&x);\n", sig->name, length == 32 ? "float" : "double") < 0)
			return -1;
		return 0;
	}

	if(sig->is_signed) {
		uint64_t top = (1uL << (length - 1));
		uint64_t negative = ~mask;
		if(length <= 32)
			negative &= 0xFFFFFFFF;
		if(length <= 16)
			negative &= 0xFFFF;
		if(length <= 8)
			negative &= 0xFF;
		if(negative)
			fprintf(o, "\tx = x & 0x%"PRIx64" ? x | 0x%"PRIx64" : x; \n", top, negative);
	} 

	fprintf(o, "\tunpack->%s = x;\n", sig->name);
	return 0;
}

static int signal2serializer(signal_t *sig, FILE *o)
{
	assert(sig && o);
	bool motorola = sig->endianess == endianess_motorola_e;
	int start = fix_start_bit(motorola, sig->start_bit);

	uint64_t mask = sig->bit_length == 64 ?
		0xFFFFFFFFFFFFFFFFuLL :
		(1uLL << sig->bit_length) - 1uLL;

	if(comment(sig, o) < 0)
		return -1;

	/**@todo fix this cast and deference to be something more sensible */
	fprintf(o, "\tx = (*(%s*)(&pack->%s)) & 0x%"PRIx64";\n", determine_unsigned_type(sig->bit_length), sig->name, mask);
	if(start)
		fprintf(o, "\tx <<= %u; \n", start);
	fprintf(o, "\t%c |= x;\n", motorola ? 'm' : 'i');
	return 0;
}

static int signal2print(signal_t *sig, unsigned id, FILE *o)
{
	/*super lazy*/
	fprintf(o, "\tscaled = decode_can_0x%03x_%s(print);\n", id, sig->name);

	if(sig->is_floating)
		return fprintf(o, "\tfprintf(data, \"%s = %%.3lf (wire: %%lf)\\n\", scaled, (double)(print->%s));\n", 
				sig->name, sig->name);
	return fprintf(o, "\tfprintf(data, \"%s = %%.3lf (wire: %%.0lf)\\n\", scaled, (double)(print->%s));\n", 
			sig->name, sig->name);
}

static int signal2type(signal_t *sig, FILE *o)
{
	assert(sig && o);
	unsigned length = sig->bit_length;
	const char *type = determine_type(length, sig->is_signed);

	if(length == 0) {
		warning("signal %s has bit length of 0 (fix the dbc file)");
		return -1;
	}

	if(sig->is_floating) {
		if(length != 32 || length != 64) {
			warning("signal %s is floating point number but has length %u (fix the dbc file)", sig->name, length);
			return -1;
		}
		type = length == 64 ? "double" : "float";
	}

	int r = fprintf(o, "\t%s %s; /*scaling %.1lf, offset %.1lf, units %s*/\n", 
			type, sig->name, sig->scaling, sig->offset, sig->units[0] ? sig->units : "none");
	return r;
}

static int signal2macros(const char *msgname, unsigned id, signal_t *sig, FILE *o, bool decode, bool header)
{
	const char *type = determine_type(sig->bit_length, sig->is_signed);
	const char *method = decode ? "decode" : "encode";
	const char *rtype = type;
	/**@todo more advanced type conversion could be done here */
	if(sig->scaling != 1.0 || sig->offset != 0.0)
		rtype = "double";
	fprintf(o, "%s %s_can_0x%03x_%s(%s_t *record)", rtype, method, id, sig->name, msgname);
	if(header)
		return fputs(";\n", o);
	
	fputs("\n{\n", o);
	fprintf(o, "\t%s rval = (%s)(record->%s);\n", rtype, rtype, sig->name);
	if(sig->scaling == 0.0)
		error("invalid scaling factor (fix your DBC file)");
	if(sig->scaling != 1.0)
		fprintf(o, "\trval *= %lf;\n", decode ? sig->scaling : 1.0 / sig->scaling);
	if(sig->offset != 0.0)
		fprintf(o, "\trval += %lf;\n", decode ? sig->offset  : -1.0 * sig->offset);
	fputs("\treturn rval;\n", o);
	return fputs("}\n\n", o);
}

static int print_function_name(FILE *out, const char *prefix, const char *name, const char *postfix, bool in, char *datatype)
{
	assert(out && prefix && name && postfix);
	return fprintf(out, "int %s_%s(%s_t *%s, %s %sdata)%s", prefix, name, name, prefix, datatype, in ? "" : "*", postfix);
}

static void make_name(char *newname, size_t maxlen, const char *name, unsigned id)
{
	snprintf(newname, maxlen-1, "can_0x%03x_%s", id, name);
}

static int msg2c(can_msg_t *msg, FILE *c)
{
	assert(msg && c);
	char name[MAX_NAME_LENGTH] = {0};
	make_name(name, MAX_NAME_LENGTH, msg->name, msg->id);
	bool motorola_used = false;
	bool intel_used = false;

	for(size_t i = 0; i < msg->signal_count; i++)
		if(msg->signals[i]->endianess == endianess_motorola_e)
			motorola_used = true;
		else
			intel_used = true;

	fprintf(c, "%s_t %s_data;\n\n", name, name);

	print_function_name(c, "pack", name, "\n{\n", false, "uint64_t");
	fprintf(c, "\tregister uint64_t x;\n");
	if(motorola_used)
		fprintf(c, "\tregister uint64_t m = 0;\n");
	if(intel_used)
		fprintf(c, "\tregister uint64_t i = 0;\n");

	for(size_t i = 0; i < msg->signal_count; i++)
		if(signal2serializer(msg->signals[i], c) < 0)
			return -1;

	fprintf(c, "\t*data = %s%s%s%s%s;\n",
			swap_motorola && motorola_used ? "reverse_byte_order" : "",
			motorola_used ? "(m)" : "",
			motorola_used && intel_used ? "|" : "",
			swap_motorola || !intel_used ? "" : "reverse_byte_order",
			motorola_used ? "" : "(i)");
	fprintf(c, "\treturn 0;\n}\n\n");

	print_function_name(c, "unpack", name, "\n{\n", true, "uint64_t");
	fprintf(c, "\tregister uint64_t x;\n");
	if(motorola_used)
		fprintf(c, "\tregister uint64_t m = %s(data);\n", swap_motorola ? "reverse_byte_order" : "");
	if(intel_used)
		fprintf(c, "\tregister uint64_t i = %s(data);\n", swap_motorola ? "" : "reverse_byte_order");

	for(size_t i = 0; i < msg->signal_count; i++)
		if(signal2deserializer(msg->signals[i], c) < 0)
			return -1;
	fprintf(c, "\treturn 0;\n}\n\n");

	for(size_t i = 0; i < msg->signal_count; i++) {
		if(signal2macros(name, msg->id, msg->signals[i], c, true, false) < 0)
			return -1;
		if(signal2macros(name, msg->id, msg->signals[i], c, false, false) < 0)
			return -1;
	}

	
	print_function_name(c, "print", name, "\n{\n", false, "FILE");
	fprintf(c, "\tdouble scaled;\n");
	for(size_t i = 0; i < msg->signal_count; i++) {
		if(signal2print(msg->signals[i], msg->id, c) < 0)
			return -1;
	}
	fprintf(c, "\treturn 0;\n}\n\n");

	return 0;
}

static int msg2h(can_msg_t *msg, FILE *h)
{
	assert(msg && h);
	char name[MAX_NAME_LENGTH] = {0};
	make_name(name, MAX_NAME_LENGTH, msg->name, msg->id);

	/**@todo add time stamp information to struct */
	fprintf(h, "typedef struct {\n" );
	for(size_t i = 0; i < msg->signal_count; i++)
		if(signal2type(msg->signals[i], h) < 0)
			return -1;
	fprintf(h, "} %s_t;\n\n", name);
	fprintf(h, "extern %s_t %s_data;\n", name, name);

	print_function_name(h, "pack", name, ";\n", false, "uint64_t");
	print_function_name(h, "unpack", name, ";\n\n", true, "uint64_t");

	for(size_t i = 0; i < msg->signal_count; i++) {
		if(signal2macros(name, msg->id, msg->signals[i], h, true, true) < 0)
			return -1;
		if(signal2macros(name, msg->id, msg->signals[i], h, false, true) < 0)
			return -1;
	}

	print_function_name(h, "print", name, ";\n", false, "FILE");

	fputs("\n\n", h);

	return 0;
}

static const char *cfunctions = 
"static inline uint64_t reverse_byte_order(uint64_t x)\n" 
"{\n"
"\tx = (x & 0x00000000FFFFFFFF) << 32 | (x & 0xFFFFFFFF00000000) >> 32;\n"
"\tx = (x & 0x0000FFFF0000FFFF) << 16 | (x & 0xFFFF0000FFFF0000) >> 16;\n"
"\tx = (x & 0x00FF00FF00FF00FF) << 8  | (x & 0xFF00FF00FF00FF00) >> 8;\n"
"\treturn x;\n"
"}\n\n";

static int message_compare_function(const void *a, const void *b)
{
	can_msg_t *ap = *((can_msg_t**)a);
	can_msg_t *bp = *((can_msg_t**)b);
	if(ap->id <  bp->id) return -1;
	if(ap->id == bp->id) return  0;
	if(ap->id >  bp->id) return  1;
	return 0;
}

static int signal_compare_function(const void *a, const void *b)
{
	signal_t *ap = *((signal_t**)a);
	signal_t *bp = *((signal_t**)b);
	if(ap->bit_length <  bp->bit_length) return  1;
	if(ap->bit_length == bp->bit_length) return  0;
	if(ap->bit_length >  bp->bit_length) return -1;
	return 0;
}

static int switch_function(FILE *c, dbc_t *dbc, char *function, bool unpack, bool prototype, char *datatype)
{
	fprintf(c, "int %s_message(unsigned id, %s %sdata)", function, datatype, unpack ? "" : "*");
	if(prototype)
		return fprintf(c, ";\n");
	fprintf(c, " {\n");
	fprintf(c, "\tswitch(id) {\n");
	for(int i = 0; i < dbc->message_count; i++) {
		can_msg_t *msg = dbc->messages[i];
		char name[MAX_NAME_LENGTH] = {0};
		make_name(name, MAX_NAME_LENGTH, msg->name, msg->id);
		fprintf(c, "\tcase 0x%03x: return %s_%s(&%s_data, data); break;\n", 
				msg->id,
				function,
				name,
				name);
	}
	fprintf(c, "\tdefault: break; \n\t}\n");
	return fprintf(c, "\treturn -1; \n}\n\n");
}

int dbc2c(dbc_t *dbc, FILE *c, FILE *h, const char *name)
{
	/**@todo print out ECU node information */
	assert(dbc && c && h);
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	char *file_guard = duplicate(name);
	size_t file_guard_len = strlen(file_guard);

	/* make file guard all upper case alphanumeric only, first character
	 * alpha only*/
	if(!isalpha(file_guard[0]))
		file_guard[0] = '_';
	for(size_t i = 0; i < file_guard_len; i++)
		file_guard[i] = (isalnum(file_guard[i])) ?
			toupper(file_guard[i]) : '_';

	/* sort signals by id */
	qsort(dbc->messages, dbc->message_count, sizeof(dbc->messages[0]), message_compare_function);

	/* sort by size for better struct packing */
	for(int i = 0; i < dbc->message_count; i++) {
		can_msg_t *msg = dbc->messages[i];
		qsort(msg->signals, msg->signal_count, sizeof(msg->signals[0]), signal_compare_function);
	}

	/* header file (begin) */
	fprintf(h,
		"/** @brief CAN message encoder/decoder: automatically generated - do not edit\n"
		"  * @note  Generated on %s"
		"  * @note  Generated by dbcc: See https://github.com/howerj/dbcc\n"
		"  */\n\n"
		"#ifndef %s\n"
		"#define %s\n\n"
		"#include <stdint.h>\n"
		"#include <stdio.h>\n\n"
		"#ifdef __cplusplus\n"
		"extern \"C\" { \n"
		"#endif\n\n",
		asctime(timeinfo),
		file_guard, file_guard);

	switch_function(h, dbc, "unpack", true, true, "uint64_t");
	switch_function(h, dbc, "pack", false, true, "uint64_t");
	switch_function(h, dbc, "print", true, true, "FILE*");
	fputs("\n", h);

	for(int i = 0; i < dbc->message_count; i++)
		if(msg2h(dbc->messages[i], h) < 0)
			return -1;
	fputs(
		"#ifdef __cplusplus\n"
		"} \n"
		"#endif\n\n"
		"#endif\n",
		h);
	/* header file (end) */

	/* C FILE */
	fprintf(c, "#include \"%s\"\n", name);
	fprintf(c, "#include <inttypes.h>\n\n");
	fprintf(c, "%s\n", cfunctions);

	switch_function(c, dbc, "unpack", true, false, "uint64_t");
	switch_function(c, dbc, "pack", false, false, "uint64_t");
	switch_function(c, dbc, "print", true, false, "FILE*");

	for(int i = 0; i < dbc->message_count; i++)
		if(msg2c(dbc->messages[i], c) < 0)
			return -1;

	free(file_guard);
	return 0;
}

