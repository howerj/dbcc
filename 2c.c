/**@todo Multiplexed signals are not handled
 * @todo DLC processing
 * @todo Validation functions with configurable behavior (range checks, etc) */
#include "2c.h"
#include "util.h"
#include <assert.h>
#include <time.h>
#include <inttypes.h>
#include <string.h>

#define MAX_NAME_LENGTH (512u)

static unsigned fix_start_bit(bool motorola, unsigned start)
{
	if(motorola)
		start = (8 * (7 - (start / 8))) + (start % 8);
	return start;
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
	fprintf(o, "\tx = (*(uint64_t*)(&pack->%s)) & 0x%"PRIx64";\n", sig->name, mask);
	if(start)
		fprintf(o, "\tx <<= %u; \n", start);
	fprintf(o, "\t%c |= x;\n", motorola ? 'm' : 'i');
	return 0;
}

static const char *determine_type(unsigned length, bool is_signed)
{
	const char *type;
	type = is_signed ? "int64_t" : "uint64_t"; 
	if(length <= 32)
		type = is_signed ? "int32_t" : "uint32_t"; 
	if(length <= 16)
		type = is_signed ? "int16_t" : "uint16_t"; 
	if(length <= 8)
		type = is_signed ? "int8_t" : "uint8_t"; 
	return type;
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

static int print_function_name(FILE *out, const char *prefix, const char *name, const char *postfix, bool in)
{
	assert(out && prefix && name && postfix);
	return fprintf(out, "int %s_%s(%s_t *restrict %s, uint64_t %sdata)%s", 
			prefix, name, name, prefix, in ? "" : "*restrict ", postfix);
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

	print_function_name(c, "pack", name, "\n{\n", false);
	fprintf(c, "\tregister uint64_t x = 0, m = 0, i = 0;\n");
	for(size_t i = 0; i < msg->signal_count; i++)
		if(signal2serializer(msg->signals[i], c) < 0)
			return -1;
	fprintf(c, "\t*data = reverse_byte_order(m) | i;\n");
	fprintf(c, "\treturn 0;\n}\n\n");

	print_function_name(c, "unpack", name, "\n{\n", true);
	fprintf(c, "\tregister uint64_t x, m = reverse_byte_order(data), i = 0;\n");
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
	return 0;
}

static int msg2h(can_msg_t *msg, FILE *h)
{
	assert(msg && h);
	char name[MAX_NAME_LENGTH] = {0};
	make_name(name, MAX_NAME_LENGTH, msg->name, msg->id);

	/**@todo print structure in optimal order (sort for size of signal
	 * before printing structure out) */
	fprintf(h, "typedef struct {\n" );
	for(size_t i = 0; i < msg->signal_count; i++)
		if(signal2type(msg->signals[i], h) < 0)
			return -1;
	fprintf(h, "} %s_t;\n\n", name);

	print_function_name(h, "pack", name, ";\n", false);
	print_function_name(h, "unpack", name, ";\n\n", true);

	for(size_t i = 0; i < msg->signal_count; i++) {
		if(signal2macros(name, msg->id, msg->signals[i], h, true, true) < 0)
			return -1;
		if(signal2macros(name, msg->id, msg->signals[i], h, false, true) < 0)
			return -1;
	}
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

static int signal_compare_function(const void *a, const void *b)
{
	signal_t *ap = *((signal_t**)a);
	signal_t *bp = *((signal_t**)b);
	if(ap->bit_length <  bp->bit_length) return  1;
	if(ap->bit_length == bp->bit_length) return  0;
	if(ap->bit_length >  bp->bit_length) return -1;
	return 0;
}

int dbc2c(dbc_t *dbc, FILE *c, FILE *h, const char *name)
{
	/**@todo print out ECU node information */
	assert(dbc && c && h);
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	/**@todo make header guard based on fail name*/
	static const char *file_guard = "CAN_CODEC";
	
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
		"#ifndef %s_H\n"
		"#define %s_H\n\n"
		"#include <stdint.h>\n\n"
		"#ifdef __cplusplus\n"
		"#define restrict\n"
		"extern \"C\" { \n"
		"#endif\n\n",
		asctime(timeinfo),
		file_guard, file_guard);

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
	fprintf(c, "#include \"%s\"\n\n", name);
	fprintf(c, "%s\n", cfunctions);
	for(int i = 0; i < dbc->message_count; i++)
		if(msg2c(dbc->messages[i], c) < 0)
			return -1;
	return 0;
}

