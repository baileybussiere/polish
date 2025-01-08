#include <ctype.h>
#include "common.h"

int match_instr(char *iden, unsigned len) {
	int i = 0, j = 0;
  	switch( len ) {
  	  case 2:
  		if( !memcmp(iden, instr_names[-T_IN], 2*sizeof(char)) )			return T_IN;
  		return T_IDEN;
	  case 3:
		if( !memcmp(iden, instr_names[-T_END], 3*sizeof(char)) )		return T_END;
		if( !memcmp(iden, instr_names[-T_JMP], 3*sizeof(char)) )		return T_JMP;
		if( !memcmp(iden, instr_names[-T_CPP], 3*sizeof(char)) )		return T_CPP;
		if( !memcmp(iden, instr_names[-T_OUT], 3*sizeof(char)) )		return T_OUT;
		if( !memcmp(iden, instr_names[-T_ERR], 3*sizeof(char)) )		return T_ERR;
		if( !memcmp(iden, instr_names[-T_CLS], 3*sizeof(char)) )		return T_CLS;
		if( !memcmp(iden, instr_names[-T_OPN], 3*sizeof(char)) )		return T_OPN;
		i = T_UND; j = T_DIV;
		for( ; i >= j; i -= 4 ) {
		 	if( !memcmp(iden, instr_names[-i], 3*sizeof(char)) )		return i;
		}
		return T_IDEN;
	  case 4:
		if( !memcmp(iden, instr_names[-T_SREV], 4*sizeof(char)) )		return T_SREV;
		if( !memcmp(iden, instr_names[-T_SSUB], 4*sizeof(char)) )		return T_SSUB;
		if( !memcmp(iden, instr_names[-T_STOK], 4*sizeof(char)) )		return T_STOK;
		if( !memcmp(iden, instr_names[-T_SFMT], 4*sizeof(char)) )		return T_SFMT;
		if( !memcmp(iden, instr_names[-T_SSCN], 4*sizeof(char)) )		return T_SSCN;
		if( !memcmp(iden, instr_names[-T_SCAP], 4*sizeof(char)) )		return T_SCAP;
		if( !memcmp(iden, instr_names[-T_SLOW], 4*sizeof(char)) )		return T_SLOW;
		if( !memcmp(iden, instr_names[-T_OPNF], 4*sizeof(char)) )		return T_OPNF;
		if( !memcmp(iden, instr_names[-T_CLSF], 4*sizeof(char)) )		return T_CLSF;
		switch( iden[0] ) {
			case 'c': i = T_CUND; j = T_CDIV; break;
			case 'r': i = T_RUND; j = T_RDIV; break;
			case 'l': i = T_LUND; j = T_LDIV; break;
			case 's': i = T_SSWP; j = T_SGET; break;
			default: return T_IDEN;
		}
		for( ; i >= j; i -= 4 ) {
			if( !memcmp(iden + 1, instr_names[-i] + 1, 3*sizeof(char)) )return i;
	  	}
	  	return T_IDEN;
	  case 5:
	  	if( !memcmp(iden, instr_names[-T_SPUTF], 5*sizeof(char)) )		return T_SPUTF;
	  	if( !memcmp(iden, instr_names[-T_SGETF], 5*sizeof(char)) )		return T_SGETF;
	  	return T_IDEN;
	  default:
		return T_IDEN;
  	}
}

typedef struct {
	FILE *f;
	unsigned lineno;
	unsigned colno;
	unsigned long val_num;
	unsigned val_iden_count;
	char *val_iden;
	char curr_char;
	char parsing_string;
} lex;

lex make_lex(FILE *f) {
	char *iden = malloc(32*sizeof(char));
	return (lex) {f, 0, 0, 0, 0, iden, fgetc(f), 0};
}

char __advance(lex *l) {
	int c = fgetc(l->f);
	if( c == EOF )	l->curr_char = T_EOF;
	else			l->curr_char = (char) c;
	if( l->curr_char == '\n' )	{ l->lineno++;		l->colno = 0; }
	else						  l->colno++;
	return l->curr_char;
}

int __append_digit(lex *l, unsigned int c, int flags) {
	if( flags < T_NOT_LEXED_YET ) return flags;
	unsigned base = (flags & F_BASEMASK) >> 8;
	l->val_num *= base;
	if( base == 1 && c == '1' )					{ l->val_num++;								return flags; }
	if( c >= '0' && c < '0' + base && c <= '9' ){ l->val_num += c - (unsigned) '0';			return flags; }
	if( c >= 'A' && c < 'A' + base - 10 )		{ l->val_num += c + 10 - (unsigned) 'A';	return flags; }
	if( c >= 'a' && c < 'a' + base - 10 )		{ l->val_num += c + 10 - (unsigned) 'a';	return flags; }
	return T_INV_NUM;
}

int parse_num_pref(lex *l) {
	char c = __advance(l);
	int ret = 0;
	switch( c ) {
	  case 'u':	__advance(l);				return			0x00000103;
	  case 'b':	__advance(l);				return			0x00000203;
	  case 't':	__advance(l);				return			0x00000303;
	  case 'q':	__advance(l);				return			0x00000403;
	  case 'p':	__advance(l);				return			0x00000503;
	  case 'h':	__advance(l);				return			0x00000603;
	  case 's':	__advance(l);				return			0x00000703;
	  case 'o':	__advance(l);				return			0x00000803;
	  case 'n':	__advance(l);				return			0x00000903;
	  case 'd':	__advance(l);				return			0x00000A03;
	  case 'U':	__advance(l);				return			0x00000B03;
	  case 'B':	__advance(l);				return			0x00000C03;
	  case 'T':	__advance(l);				return			0x00000D03;
	  case 'Q':	__advance(l);				return			0x00000E03;
	  case 'P':	__advance(l);				return			0x00000F03;
	  case 'H':	__advance(l);				return			0x00001003;
	  case 'S':	__advance(l);				return			0x00001103;
	  case 'O':	__advance(l);				return			0x00001203;
	  case 'N':	__advance(l);				return			0x00001303;
	  case 'v':	__advance(l);				return			0x00001403;
	  case 'c': case 'C': c = __advance(l);	ret =			0x00000001; break;
	  case 'r': case 'R': c = __advance(l); ret =			0x00000002; break;
	  case 'i': case 'I': c = __advance(l); ret =			0x00000003; break;
	  case 'l': case 'L': c = __advance(l); ret =			0x00000004; break;
	  case '0'...'9':	 	 				return			0x00000A00;
	  default:								return			T_INV_NUMPREF;
	}
	switch( c ) {
	  case 'u':	__advance(l);			return	  ret | 0x00000100;
	  case 'b':	__advance(l);			return	  ret | 0x00000200;
	  case 't': __advance(l);			return	  ret | 0x00000300;
	  case 'q': __advance(l);			return	  ret | 0x00000400;
	  case 'p': __advance(l);			return	  ret | 0x00000500;
	  case 'h': __advance(l);			return	  ret | 0x00000600;
	  case 's': __advance(l);			return	  ret | 0x00000700;
	  case 'o': __advance(l);			return	  ret | 0x00000800;
	  case 'n': __advance(l);			return	  ret | 0x00000900;
	  case 'd': __advance(l);			return	  ret | 0x00000A00;
	  case 'U': __advance(l);			return	  ret | 0x00000B00;
	  case 'B': __advance(l);			return	  ret | 0x00000C00;
	  case 'T': __advance(l);			return	  ret | 0x00000D00;
	  case 'Q': __advance(l);			return	  ret | 0x00000E00;
	  case 'P': __advance(l);			return	  ret | 0x00000F00;
	  case 'H': __advance(l);			return	  ret | 0x00001000;
	  case 'S': __advance(l);			return	  ret | 0x00001100;
	  case 'O': __advance(l);			return	  ret | 0x00001200;
	  case 'N': __advance(l);			return	  ret | 0x00001300;
	  case 'v': __advance(l);			return	  ret | 0x00001400;
	  case '0'...'9': 					return    ret | 0x00000A00;
	  default:							return			T_INV_NUMPREF;
	}
}

int next_tok(lex *l) {
	int flags = 0;
	char curr_char = l->curr_char;
	l->val_num = 0;
	l->val_iden_count = 0;
	l->val_iden[l->val_iden_count] = 0;
	if( l->parsing_string ) {
		if( curr_char == '\\' ) {
			curr_char = __advance(l);
			switch( curr_char ) {
			  case 'n': l->val_num = '\n';	__advance(l); return T_CHAR;
			  case 't': l->val_num = '\t';	__advance(l); return T_CHAR;
			  case '"': l->val_num = '"'; 	__advance(l); return T_CHAR;
			  case '\\': l->val_num = '\\'; __advance(l); return T_CHAR;
			  case '0': l->val_num = 0;		__advance(l); return T_CHAR;
			  case 'a': l->val_num = '\a';	__advance(l); return T_CHAR;
			  case 'b': l->val_num = '\b';	__advance(l); return T_CHAR;
			  case 'r': l->val_num = '\r'; __advance(l); return T_CHAR;
			  case 'v': l->val_num = '\v'; __advance(l); return T_CHAR;
			}
		}
		if( curr_char == '"' ) { l->parsing_string = 0; curr_char = __advance(l); }
		else { l->val_num = curr_char; __advance(l); return T_CHAR; }
	}
	while( isspace(curr_char) ) {
		curr_char = __advance(l);
	}
	switch( curr_char ) {
	  case 0: return T_EOF;
	  case '+': __advance(l); return T_ADD;
	  case '-': __advance(l); return T_SUB;
	  case '*': __advance(l); return T_MUL;
	  case '/': __advance(l); return T_DIV;
  	  case '~': __advance(l); return T_UND;
	  case '.': __advance(l); return T_DRP;
	  case '0'...'9':
	  	flags = T_INT | 0x00000A00;
	  	while( isalphanum(curr_char) ) {
			flags = __append_digit(l, curr_char, flags);
			curr_char = __advance(l);
	  	}
	  	if( flags < T_NOT_LEXED_YET ) return flags;
	  	return flags & F_TYPEMASK;
	  case '#':
	  	flags = parse_num_pref(l);
		curr_char = l->curr_char;
	  	while( isalphanum(curr_char) ) {
			flags = __append_digit(l, curr_char, flags);
			curr_char = __advance(l);
	  	}
	  	if( flags < T_NOT_LEXED_YET ) return flags;
		return flags & F_TYPEMASK;
	  case '"':
		l->parsing_string = 1;
		__advance(l);
		return T_CHAR;
	  case 'a'...'z': case 'A'...'Z':
		while( isalphanum(curr_char) ) {
			l->val_iden[l->val_iden_count++] = curr_char;
			curr_char = __advance(l);
		}
		return match_instr(l->val_iden, l->val_iden_count);
	  case ':':
		curr_char = __advance(l);
		while( isalphanum(curr_char) || curr_char == '_' ) {
			l->val_iden[l->val_iden_count++] = curr_char;
			curr_char = __advance(l);
		}
		if( l->val_iden_count == 0 ) return T_INV_LABEL;
		return T_NEW_LABEL;
	  case '@':
		curr_char = __advance(l);
		while( isalphanum(curr_char) || curr_char == '_' ) {
			l->val_iden[l->val_iden_count++] = curr_char;
			curr_char = __advance(l);
		}
		if( l->val_iden_count == 0 ) return T_CPP;
		return T_JMP_LABEL;
	  default:
		__advance(l);
		return curr_char;
	}
}
