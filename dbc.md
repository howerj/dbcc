# dbc.md
## DBC file specification

## Parser Grammar

The current grammar for the DBC parser is as follows:

	 s         : /[ \t]/ ;
	 n         : /\r?\n/ ;
	 sign      : '+' | '-' ;
	 float     : /[-+]?[0-9]+(\.[0-9]+)?([eE][-+]?[0-9]+)?/ ;
	 ident     : /[a-zA-Z_][a-zA-Z0-9_]*/ ;
	 integer   : <sign>? /[0-9]+/ ;
	 factor    : <float> | <integer> ;
	 offset    : <float> | <integer> ;
	 length    : /[0-9]+/ ;
	 range     : '[' ( <float> | <integer> ) '|' ( <float> | <integer> ) ']' ;
	 node      : <ident> ;
	 nodes     : <node> <s>* ( ',' <s>* <node>)* ;
	 string    : '"' /[^"]*/ '"' ;  
	 unit      : <string> ;
	 startbit  : <integer> ;
	 endianess : '0' | '1' ;
	 y_mx_c    : '(' <factor> ',' <offset> ')' ;
	 name      : <ident> ;
	 ecu       : <ident> ;
	 dlc       : <integer> ;
	 id        : <integer> ;
	 multiplexor : 'M' | 'm' <s>* <integer> ;
	 signal    : <s>* "SG_" <s>+ <name> <s>* <multiplexor>? <s>* ':' <s>* <startbit> <s>* '|' <s>*
		     <length> <s>* '@' <s>* <endianess> <s>* <sign> <s>* <y_mx_c> <s>*
		     <range> <s>* <unit> <s>* <nodes> <s>* <n> ;
	 message   : "BO_" <s>+ <id> <s>+ <name>  <s>* ':' <s>* <dlc> <s>+ <ecu> <s>* <n> <signal>* ;
	 messages  : (<message> <n>+)* ;
	 version   : "VERSION" <s> <string> <n>+ ;
	 ecus      : "BU_" <s>* ':' (<ident>|<s>)* <n> ;
	 symbols   : "NS_" <s>* ':' <s>* <n> (' ' <ident> <n>)* <n> ;
	 sigtype   : <integer>  ;
	 sigval    : <s>* "SIG_VALTYPE_" <s>+ <id> <s>+ <name> <s>* ":" <s>* <sigtype> <s>* ';' <n>* ;
	 whatever  : (<ident>|<string>|<integer>|<float>) ;
	 bs        : "BS_" <s>* ':' <n>+ ;  types     : <s>* <ident> (<whatever>|<s>)+ ';' (<n>*|/$/) ;
	 values    : "VAL_TABLE_" (<whatever>|<s>)* ';' <n> ;
	 dbc       : <version> <symbols> <bs> <ecus> <values>* <n>* <messages> (<sigval>|<types>)*  ;

The file format contains significant whitespace, so the parsers grammar has to
be made to be more complex than it should be, alternatively a parser could be
made that accepts **ident** tags only if they are not keywords (which would be
*much* better).

The two most important things in the [DBC][] file are messages and signals. You
can resolve

## Maximum Character Length

The identifiers are limited to 128 characters in length.

## Keywords

* "BO\_" Message Object
* "EV\_" Environment Variable
* "SG\_" Signal 
* "BU\_" Network Node

## Messages


### Signals


[DBC]: http://vector.com/vi_candb_en.html
 
