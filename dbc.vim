" Vim syntax file
" Language: 	DBC File
" 		"*.dbc"
" Maintainer: Richard Howe
" Latest Revision: 10 October 2016

if exists("b:current_syntax")
	finish
endif

syn keyword dbcSymbols CM_ BA_DEF_ BA_ VAL_ CAT_DEF_ CAT_ FILTER BA_DEF_DEF_
syn keyword dbcSymbols EV_DATA_ ENVVAR_DATA_ SGTYPE_ SGTYPE_VAL_ BA_DEF_SGTYPE_
syn keyword dbcSymbols BA_SGTYPE_ SIG_TYPE_REF_ VAL_TABLE_ SIG_GROUP_ SIG_VALTYPE_
syn keyword dbcSymbols SIGTYPE_VALTYPE_ BO_TX_BU_ BA_DEF_REL_ BA_REL_ BA_DEF_DEF_REL_
syn keyword dbcSymbols BU_SG_REL_ BU_EV_REL_ BU_BO_REL_ NS_DESC_ 

syn keyword dbcSection VERSION NS_ BS_ BO_ SG_ BU_ Vector__XXX
syn region dbcString start=/\v"/ skip=/\v\\./ end=/\v"/ contains=@Spell

syn match dbcNumber '\d\+' 
syn match dbcNumber '[-+]\d\+' 

" Floating point number with decimal no E or e (+,-)
syn match dbcNumber '\d\+\.\d*'
syn match dbcNumber '[-+]\d\+\.\d*'

" Floating point like number with E and no decimal point (+,-)
syn match dbcNumber '[-+]\=\d[[:digit:]]*[eE][\-+]\=\d\+' 
syn match dbcNumber '\d[[:digit:]]*[eE][\-+]\=\d\+'

" Floating point like number with E and decimal point (+,-)
syn match dbcNumber '[-+]\=\d[[:digit:]]*\.\d*[eE][\-+]\=\d\+'
syn match dbcNumber '\d[[:digit:]]*\.\d*[eE][\-+]\=\d\+' 

syn match dbcIdentifier '[a-zA-Z_][0-9a-zA-Z_]*'

let b:current_syntax = "dbc"

highlight link dbcSymbols Type
highlight link dbcSection Keyword
highlight link dbcString String
highlight link dbcNumber Number
highlight link dbcIdentifier Identifier
