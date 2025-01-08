#include <ctype.h>

enum {
	FMT_STR = -1,
	FMT_CHAR = -2,
	FMT_RED = -3,
	FMT_INT = -4,
	FMT_LONG = -5,
	FMT_INV = -6,
};

enum {
	SIGN_U = 0,
	SIGN_S = 1,
	SIGN_USHOW = 2,
	SIGN_SSHOW = 3,
};

typedef struct {
	char *c;
	char curr_char;
	char sign;
	char base;
	char padding;
	int width;
} fmt_lex;

fmt_lex make_fmt_lex(char *c) {
	return (fmt_lex) { c, *c, SIGN_U, 10, ' ', 0 };
}

char __advance_fmt(fmt_lex *l) {
	l->c++;
	l->curr_char = *l->c;
	return l->curr_char;
}

int parse_fmt(fmt_lex *l) {
	char c = l->curr_char;
	int ret = 0;
	if( c == '+' ) { l->sign = SIGN_USHOW; c = __advance_fmt(l); }
	l->width = -1;
	if( c == '0' ) { l->padding = '0'; l->width = 0; }
	else if( c > '0' && c <= '9' ) l->width = 0;
	while( c >= '0' && c <= '9' ) {
		l->width *= 10;
		l->width += c - '0';
		c = __advance_fmt(l);
	}
	switch( c ) {
	  case 's': __advance_fmt(l);					return			FMT_STR;
	  case 'c': c = __advance_fmt(l);				ret =			FMT_CHAR;	break;
	  case 'r': c = __advance_fmt(l);				ret =			FMT_RED;	break;
	  case 'i': c = __advance_fmt(l);				ret =			FMT_INT;	break;
	  case 'l': c = __advance_fmt(l);				ret =			FMT_LONG;	break;
	  case 'C': c = __advance_fmt(l); l->sign++;	ret =			FMT_CHAR;	break;
	  case 'R': c = __advance_fmt(l); l->sign++;	ret =			FMT_RED;	break;
	  case 'I': c = __advance_fmt(l); l->sign++;	ret =			FMT_INT;	break;
	  case 'L': c = __advance_fmt(l); l->sign++;	ret =			FMT_LONG;	break;
	  default:	c = __advance_fmt(l);				return			FMT_INV;
	}
	switch( c ) {
	  case 'u': __advance_fmt(l);	l->base = 1;	return			ret;
	  case 'b': __advance_fmt(l);	l->base = 2;	return			ret;
	  case 't': __advance_fmt(l);	l->base = 3;	return			ret;
	  case 'q': __advance_fmt(l);	l->base = 4;	return			ret;
	  case 'p': __advance_fmt(l);	l->base = 5;	return			ret;
	  case 'h': __advance_fmt(l);	l->base = 6;	return			ret;
	  case 's': __advance_fmt(l);	l->base = 7;	return			ret;
	  case 'o': __advance_fmt(l);	l->base = 8;	return			ret;
	  case 'n': __advance_fmt(l);	l->base = 9;	return			ret;
	  case 'd': __advance_fmt(l);	l->base = 10;	return			ret;
	  case 'U': __advance_fmt(l);	l->base = 11;	return			ret;
	  case 'B': __advance_fmt(l);	l->base = 12;	return			ret;
	  case 'T': __advance_fmt(l);	l->base = 13;	return			ret;
	  case 'Q': __advance_fmt(l);	l->base = 14;	return			ret;
	  case 'P': __advance_fmt(l);	l->base = 15;	return			ret;
	  case 'H': __advance_fmt(l);	l->base = 16;	return			ret;
	  case 'S': __advance_fmt(l);	l->base = 17;	return			ret;
	  case 'O': __advance_fmt(l);	l->base = 18;	return			ret;
	  case 'N': __advance_fmt(l);	l->base = 19;	return			ret;
	  case 'v': __advance_fmt(l);	l->base = 20;	return			ret;
	  default:						l->base = 10;	return			ret;
	}
}

int next_token_fmt(fmt_lex *l) {
	char c = l->curr_char;
	l->base = 10;
	l->width = -1;
	l->sign = 0;
	if( c != '%' ) { __advance_fmt(l); return c; }
//	printf("FMTLEXER: Formatting mark found: %c!\n", c);
	c = __advance_fmt(l);
	switch( c ) {
	  case '%':		return '%';
	  default:	return parse_fmt(l);
	}
}

char digit2char(unsigned char digit) {
	if( digit < 10 )return '0' + digit;
	else 			return 'A' + digit - 10;
}

unsigned char fmt_num_width_prep(fmt_lex *l, unsigned long *num, const unsigned size, unsigned long *significand, char *prefix) {
	unsigned long abs = SIZE_TO_MASK(size) & *num;
	unsigned char width = 1;
	*prefix = 0;
	switch( l->sign ) {
	  case SIGN_S:
	  	if( abs & (1 << (8*size - 1)) ) {
			abs = ~(SIZE_TO_MASK(size) & (abs - 1));
			*prefix = '-'; width = 2;
	  	} break;
	  case SIGN_SSHOW:
	  	width = 2;
		if( abs & (1 << (8*size - 1)) ) {
			abs = ~(SIZE_TO_MASK(size) & (abs - 1));
			*prefix = '-';
		} else *prefix = '+';
		break;
	  case SIGN_USHOW: width = 2; *prefix = '+';
	}
	*significand = l->base;
	while( *significand <= abs ) { *significand *= l->base; width++; }
	if( l->width == -1 ) l->width = width;
	*significand /= l->base;
	*num = abs;
	return width;
}

void fmt_num(const fmt_lex *l, char *out, unsigned long abs, unsigned char width, unsigned long significand, const char prefix) {
	unsigned char digit = 0;
	while( width < l->width ) { *out = l->padding; out++; width++; }
	if( prefix ) { *out = prefix; out++; }
	for( ;; ) {
		digit = abs / significand;
		*out = digit2char(digit); out++;
		abs -= digit * significand;
		if( significand == 1 ) break;
		significand /= l->base;
	}
}

int fparse_num(const fmt_lex *l, char *in, size_t *ptr, t_lnum *numval) {
	char negative = 0;
	size_t delta = 0;
	switch( l->sign ) {
	  case SIGN_S:
		if( *(in + *ptr + delta) == '-' ) { negative = 1; delta++; }
		break;
	  case SIGN_SSHOW:
		if( *(in + *ptr + delta) == '-' ) { negative = 1; delta++; }
		else if( *(in + *ptr + delta) == '+' ) delta++;
		else return 1;
		break;
	  case SIGN_USHOW:
		if( *(in + delta) != '+' ) { return 1; }
	}
	*numval = 0;
	if( l->base == 1 ) {
		while( delta < (unsigned long) l->width && *(in + *ptr + delta) == l->padding ) { delta++; };
		while( delta < (unsigned long) l->width && *(in + *ptr + delta) == '1' ) { (*numval)++; delta++; }
		if( delta < (unsigned long) l->width && l->width != -1 ) return 1;
		if( negative ) { *numval = (*numval - 1) ^ 0xFFFFFFFFFFFFFFFF; }
		*ptr += delta;
		return 0;
	}
	char curr_char = 0;
	while( delta < (unsigned long) l->width && *(in + *ptr + delta) == l->padding ) { delta++; }
	while( delta < (unsigned long) l->width ) {
		curr_char = *(in + *ptr + delta++);
		if( curr_char >= '0' && curr_char <= '9' && curr_char < l->base ) {
			curr_char *= l->base;
			curr_char += curr_char - '0';
		} else if( curr_char >= 'a' && curr_char < 'a' + l->base - 10 ) {
			curr_char *= l->base;
			curr_char += curr_char - 'a' + 10;
		} else if( curr_char >= 'A' && curr_char < 'A' + l->base - 10 ) {
			curr_char *= l->base;
			curr_char += curr_char - 'A' + 10;
		} else if( l->width == -1 ) break;
		else return 1;
	}
	if( negative ) { *numval = (*numval - 1) ^ 0xFFFFFFFFFFFFFFFF; }
	*ptr += delta;
	return 0;
}
