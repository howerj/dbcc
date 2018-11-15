#ifndef _2XML_H
#define _2XML_H

#include "can.h"


#define BSM_PREFIX "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n\
<beSTORM Version=\"1.2\">\n\
	<GeneratorOptSettings >\n\
		<BT FactoryDefined=\"1\" MaxBytesToGenerate=\"8\" FactoryType=\"Binary\" />\n\
	</GeneratorOptSettings>\n\
	<ModuleSettings>\n\
		<M Name=\"CAN\">\n\
			<P Name=\"CAN Protocol\">\n\
				<SC Name=\"CAN Sequence\">\n\
					<SP Name=\"CAN Open\" Library=\"CAN Interface.dll\" Procedure=\"OpenDevice\">\n\
						<S Name=\"IPAddress\">\n\
							<EV Name=\"IPAddress\" Description=\"CAN IP Address\" ASCIIValue=\"&lt;CAN Device&gt;\" Required=\"1\" />\n\
						</S>\n\
						<S Name=\"Port\">\n\
							<EV Name=\"Port\" Description=\"CAN Port\" ASCIIValue=\"0\" Required=\"1\" Comment=\"Should be either 0, 1, 2, or 3\"/>\n\
						</S>\n\
					</SP>\n\
					<SP Name=\"CAN SetGlobals\" Library=\"CAN Interface.dll\" Procedure=\"SetGlobals\">\n\
						<S Name=\"HANDLE\">\n\
							<PC Name=\"CAN\" ConditionedName=\"CAN Open\" Parameter=\"HANDLE\" />\n\
						</S>\n\
						<S Name=\"Baudrate\">\n\
							<EV Name=\"Baudrate\" Description=\"Baudrate\" ASCIIValue=\"250000\" Required=\"1\" Comment=\"Should be either '10000', '20000', '50000', '62500', '100000', '125000', '250000', '500000', '800000', or '1000000'\"/>\n\
						</S>\n\
					</SP>\n\
\n\
					<SE Name=\"Messages\">\n\
"

#define BSM_MESSAGE_PREFIX "\n\
						<SP Name=\"CAN Send (%s - %d)\" Library=\"CAN Interface.dll\" Procedure=\"Write\">\n\
							<S Name=\"HANDLE\">\n\
								<PC Name=\"HANDLE\" ConditionedName=\"CAN Open\" Parameter=\"HANDLE\" />\n\
							</S>\n\
							<S Name=\"Identifier\">\n\
								<C Name=\"Identifier\">%d</C>\n\
							</S>\n\
							<S ParamName=\"Data\" Name=\"Message\">\n\
								<BC Name=\"Message Bits\" PaddingSize=\"%d\" PaddingBit=\"0\">\n\
"

#define BSM_MESSAGE_SUFFIX "								</BC>\n\
							</S>\n\
						</SP>\n\
"

#define BSM_SUFFIX "\n\
					</SE>\n\
\n\
					<SP Name=\"CAN Close\" Library=\"CAN Interface.dll\" Procedure=\"CloseDevice\">\n\
						<S Name=\"HANDLE\">\n\
							<PC Name=\"CAN\" ConditionedName=\"CAN Open\" Parameter=\"HANDLE\" />\n\
						</S>\n\
					</SP>\n\
				</SC>\n\
			</P>\n\
		</M>\n\
	</ModuleSettings>\n\
</beSTORM>\n\
"

int dbc2xml(dbc_t *dbc, FILE *output, bool use_time_stamps);
int dbc2bsm(dbc_t *dbc, FILE *output, bool use_time_stamps);

#endif
