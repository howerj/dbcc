#include "2xml.h"
#include "util.h"
/**@todo neaten up producing the XML, perhaps with a make tag vprintf wrapper */
/**@todo units needs escaping*/

static int signal2xml(signal_t *sig, FILE *o)
{
	fprintf(o, "\t\t<signal>\n");
	fprintf(o, "\t\t\t<name>%s</name>\n", sig->name);
	fprintf(o, "\t\t\t<startbit>%u</startbit>\n", sig->start_bit);
	fprintf(o, "\t\t\t<bitlength>%u</bitlength>\n", sig->bit_length);
	fprintf(o, "\t\t\t<endianess>%s</endianess>\n", sig->endianess == endianess_motorola_e ? "motorola" : "intel");
	fprintf(o, "\t\t\t<scaling>%lf</scaling>\n", sig->scaling);
	fprintf(o, "\t\t\t<offset>%lf</offset>\n", sig->offset);
	fprintf(o, "\t\t\t<minimum>%lf</minimum>\n", sig->minimum);
	fprintf(o, "\t\t\t<maximum>%lf</maximum>\n", sig->maximum);
	fprintf(o, "\t\t\t<signed>%s</signed>\n", sig->is_signed ? "true" : "false");
	fprintf(o, "\t\t\t<units>%s</units>\n", sig->units);
	if(fprintf(o, "\t\t</signal>\n") < 0)
		return -1;
	return 0;
}


static int msg2xml(can_msg_t *msg, FILE *o)
{
	fprintf(o, "\t<message>\n");
	fprintf(o, "\t\t<name>%s</name>\n", msg->name);
	fprintf(o, "\t\t<id>0x%x</id>\n", msg->id);
	fprintf(o, "\t\t<dlc>%u</dlc>\n", msg->dlc);

	for(size_t i = 0; i < msg->signal_count; i++)
		if(signal2xml(msg->signals[i], o) < 0)
			return -1;
	if(fprintf(o, "\t</message>\n") < 0)
		return -1;
	return 0;
}


int dbc2xml(dbc_t *dbc, FILE *output)
{
	/**@todo print out ECU node information, and the standard XML header */
	fprintf(output, "<candb>\n");
	for(int i = 0; i < dbc->message_count; i++)
		if(msg2xml(dbc->messages[i], output) < 0)
			return -1;
	if(fprintf(output, "<candb>\n") < 0)
		return -1;
	return 0;
}

