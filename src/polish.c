#include "common.h"
#include "fmt-lex.h"

#define STACK_SIZE 256

unsigned long PROG_STACK_SIZE = 0;

int push_num(stack *s, const t_lnum i, const unsigned size) {
	if( s->head + size >= STACK_SIZE ) {
		sprintf(err_extra, "PUSH %lu (size %u)", i, size);				return RERR_SOVERFLOW;
	}
	memcpy(s->data + s->head, &i, size);
	s->head += size;
	return 0;
}

int pop_num(stack *s, t_lnum *i, const unsigned size) {
	if( s->head < size ) {
		sprintf(err_extra, "POP size %u, SP @ %lu", size, s->head);		return RERR_SUNDERFLOW;
	}
	s->head -= size;
	if( i ) memcpy(i, s->data + s->head, size);
	return 0;
}

int peek_num(stack *s, t_lnum *i, const unsigned depth, const unsigned size) {
	if( s->head < depth ) {
		sprintf(err_extra, "peek depth %u, SP @ %lu", depth, s->head);	return RERR_SUNDERFLOW;
	}
	if( depth < size ) {
		sprintf(err_extra, "peek depth %u < %u", depth, size);			return RERR_SUNDERFLOW;
	}
	memcpy(i, s->data + s->head - depth, size);
	return 0;
}

int pop_bargs(stack *s, t_lnum *lhs, t_lnum *rhs, const unsigned size) {
	int RERR;
	if( (RERR = pop_num(s, rhs, size)) )								return RERR;
	if( (RERR = pop_num(s, lhs, size)) )								return RERR;
	return 0;
}

/* Returns depth of the zero character relative to s->head - depth,
i.e., if the zero is top of stack (s->data + s->head - 1) and depth is 0, returns 0;
if the zero is third from the top (s->data + s->head - 3) and depth is 1, returns 1;
Note also that s->data + s->head has nothing (has not been written to),
this is the reason for the leading decrement. */
int find_str(const stack *s, const size_t depth, size_t *count) {
	size_t height = s->head - depth;
	*count = 0;
	while( *(char*) (s->data + --height) != 0 ) {
		if( height == 0 ) {
			sprintf(err_extra, "down from SP %lu", s->head - depth);	return RERR_RUNAWAYSTR;
		}
		(*count)++;
	} return 0;
}

int do_add(stack *s, const unsigned size) {
	t_lnum rhs = 0, lhs = 0;
	int RERR;
	if( (RERR = pop_bargs(s, &lhs, &rhs, size)) )						return RERR;
	push_num(s, rhs + lhs, size);
	return 0;
}
int do_sub(stack *s, const unsigned size) {
	t_lnum rhs = 0, lhs = 0;
	int RERR;
	if( (RERR = pop_bargs(s, &lhs, &rhs, size)) )						return RERR;
	push_num(s, rhs - lhs, size);
	return 0;
}
int do_mul(stack *s, const unsigned size) {
	t_lnum rhs = 0, lhs = 0;
	int RERR;
	if( (RERR = pop_bargs(s, &lhs, &rhs, size)) )						return RERR;
	push_num(s, rhs * lhs, size);
	return 0;
}
int do_div(stack *s, const unsigned size) {
	t_lnum rhs = 0, lhs = 0;
	int RERR;
	if( (RERR = pop_bargs(s, &lhs, &rhs, size)) )						return RERR;
	push_num(s, rhs / lhs, size);
	return 0;
}

int do_swp(stack *s, const unsigned size) {
	t_lnum rhs = 0, lhs = 0;
	int RERR;
	if( (RERR = pop_num(s, &rhs, size)) )								return RERR;
	if( (RERR = pop_num(s, &lhs, size)) )								return RERR;
#ifdef DEBUG
	printf("\t\tswapping %016lX, %016lX, size %u\n", lhs, rhs, size);
	print_stack(*s);
#endif
	push_num(s, rhs, size);
	push_num(s, lhs, size);
	return 0;
}

int do_dup(stack *s, const unsigned size) {
	t_lnum num = 0;
	int RERR;
	if( (RERR = peek_num(s, &num, size, size)) )						return RERR;
#ifdef DEBUG
	printf("\t\tduping %016lX, size %u\n", num, size);
#endif
	if( (RERR = push_num(s, num, size)) )								return RERR;
	return 0;
}

int do_jmp(stack *s, const size_t prog_size, size_t *prog_p) {
	t_lnum addr = 0;
	int RERR;
	if( (RERR = pop_num(s, &addr, 8)) )									return RERR;
	if( addr > PROG_STACK_SIZE ) {
		sprintf(err_extra, "JMP to %lu, prog size %lu", addr, prog_size); return RERR_INV_JMP;
	}
	*prog_p = addr;
	return 0;
}

int do_cond(stack *s, const size_t prog_size, size_t *prog_p) {
	t_lnum cond = 0;
	int RERR;
	if( (RERR = pop_num(s, &cond, 1)) )									return RERR;
	if( cond ) { *prog_p = *prog_p + 1; return 0; }
	if( *prog_p + 2 >= prog_size ) {
		sprintf(err_extra, "end of program on conditional");			return RERR_INV_JMP;
	}
	*prog_p = *prog_p + 2;
	return 0;
}

int do_dec(stack *s, const unsigned size) {
	if( s->head < size ) {
		sprintf(err_extra, "DEC (size %u), SP @ %lu", size, s->head);	return RERR_SUNDERFLOW;
	}
	switch( size ) {
	  case 1: --*(t_cnum*) (s->data + s->head - size); return 0;
	  case 2: --*(t_rnum*) (s->data + s->head - size); return 0;
	  case 4: --*(t_num*)  (s->data + s->head - size); return 0;
	  case 8: --*(t_lnum*) (s->data + s->head - size); return 0;
	}
	return 0;
}
int do_inc(stack *s, const unsigned size) {
	if( s->head < size ) {
		sprintf(err_extra, "INC (size %u), SP @ %lu", size, s->head);	return RERR_SUNDERFLOW;
	}
	switch( size ) {
	  case 1: ++*(t_cnum*) (s->data + s->head - size); return 0;
	  case 2: ++*(t_rnum*) (s->data + s->head - size); return 0;
	  case 4: ++*(t_num*)  (s->data + s->head - size); return 0;
	  case 8: ++*(t_lnum*) (s->data + s->head - size); return 0;
	}
	return 0;
}

int do_cmp(stack *s, const unsigned size) {
	t_lnum lhs = 0, rhs = 0;
#ifdef DEBUG
	printf("comparing...\n");
#endif
	int RERR;
	if( (RERR = pop_num(s, &rhs, size)) )		return RERR;
	if( (RERR = peek_num(s, &lhs, size, size)) )return RERR;
#ifdef DEBUG
	printf("\t\tComparing %lu and %lu.\n", lhs, rhs);
#endif
	if( rhs > lhs )		push_num(s, 1, 1);
	else if( rhs < lhs )push_num(s, (t_lnum) 0xFF, 1);
	else				push_num(s, 0, 1);
	return 0;
}
int do_not(stack *s) {
	t_lnum cond = 0;
	int RERR;
	if( (RERR = pop_num(s, &cond, 1)) )			return RERR;
	push_num(s, !cond, 1);
	return 0;
}

int do_alloc(stack *s) {
	t_lnum size = 0, ptr = 0;
	int RERR;
	if( (RERR = pop_num(s, &size, 4)) )			return RERR;
	ptr = (t_lnum) malloc(size);
	push_num(s, ptr, 8);
	return 0;
}
int do_free(stack *s) {
	t_lnum addr = 0;
	int RERR;
	if( (RERR = pop_num(s, &addr, 8)) )			return RERR;
	free((void*) addr);
	return 0;
}
int do_open_file(stack *s) {
	t_lnum mode = 0;
	t_lnum fp = 0;
	size_t strlen = 0;
	int RERR;
	if( (RERR = pop_num(s, &mode, 1)) )			return RERR;
	if( (RERR = find_str(s, 0, &strlen)) )		return RERR;
  	*(char*) (s->data + s->head) = 0; //TODO make this safe??
	switch( mode ) {
	  case 1:
		fp = (t_lnum) fopen((char*) (s->data + s->head - strlen), "r");
		if( (RERR = push_num(s, fp, 8)) )		return RERR;
		return 0;
	  case 2:
		fp = (t_lnum) fopen((char*) (s->data + s->head - strlen), "w");
		if( (RERR = push_num(s, fp, 8)) )		return RERR;
		return 0;
	  case 3:
		fp = (t_lnum) fopen((char*) (s->data + s->head - strlen), "a");
		if( (RERR = push_num(s, fp, 8)) )		return RERR;
		return 0;
	  case 9:
		fp = (t_lnum) fopen((char*) (s->data + s->head - strlen), "r+");
		if( (RERR = push_num(s, fp, 8)) )		return RERR;
		return 0;
	  case 10:
		fp = (t_lnum) fopen((char*) (s->data + s->head - strlen), "w+");
		if( (RERR = push_num(s, fp, 8)) )		return RERR;
		return 0;
	  case 11:
		fp = (t_lnum) fopen((char*) (s->data + s->head - strlen), "a+");
		if( (RERR = push_num(s, fp, 8)) )		return RERR;
		return 0;
	  default:
		if( (RERR = push_num(s, 0, 8)) )		return RERR;
		return 0;
	}
}
int do_close_file(stack *s) {
	t_lnum fp = 0;
	int RERR;
	if( (RERR = pop_num(s, &fp, 8)) )			return RERR;
	fclose((FILE*) fp);
	return 0;
}
int do_put(stack *s, const unsigned size) {
	t_lnum num = 0, addr = 0;
	int RERR;
	if( (RERR = pop_num(s, &num, size)) )		return RERR;
	if( (RERR = pop_num(s, &addr, 8)) )			return RERR;
	switch( size ) {
	  case 1: *(t_cnum*) addr = num;			return 0;
	  case 2: *(t_rnum*) addr = num;			return 0;
	  case 4: *(t_num*)  addr = num;			return 0;
	  case 8: *(t_lnum*) addr = num;			return 0;
	}
	return 0;
}

int do_get(stack *s, const unsigned size) {
	t_lnum addr = 0;
	int RERR;
	if( (RERR = pop_num(s, &addr, 8)) )			return RERR;
	switch( size ) {
	  case 1: push_num(s, *(t_cnum*) addr, size);	return 0;
	  case 2: push_num(s, *(t_rnum*) addr, size);	return 0;
	  case 4: push_num(s, *(t_num*)  addr, size);	return 0;
	  case 8: push_num(s, *(t_lnum*) addr, size);	return 0;
	}
	return 0;
}

int do_sgetf(stack *s) {
	t_lnum fp = 0;
	int RERR;
	if( (RERR = pop_num(s, &fp, 8)) )				return RERR;
	if( (RERR = push_num(s, 0, 1)) )				return RERR;
	int maxcnt = STACK_SIZE - s->head--;
	*(char*) (s->data + s->head) = 0; // TODO: write fgets equivalent by hand that gives num bytes gotten and doesn't append 0
	RERR = !fgets((char*) (s->data + s->head + 1), maxcnt, (FILE*) fp);
	s->head += strlen((char*) (s->data + s->head + 1));
	if( RERR ) return RERR_STRGET;
	return 0;
}

int do_sputf(stack *s) {
	t_lnum fp = 0;
	size_t strlen = 0;
	int RERR;
	if( (RERR = pop_num(s, &fp, 8)) )				return RERR;
	if( (RERR = find_str(s, 0, &strlen)) )			return RERR;
#ifdef DEBUG
	printf("\t\tStr length: %lu\n", strlen);
#endif
	s->head -= strlen + 1;
	for( unsigned i = 1; i <= strlen; i++ )
		fputc(*(char*) (s->data + s->head + i), (FILE*) fp);
	fputc(0, (FILE*) fp);
	return 0;
}

int do_sdrp(stack *s) {
	int RERR;
	size_t strlen;
	if( (RERR = find_str(s, 0, &strlen)) ) 				return RERR;
	s->head -= strlen + 1;
	return 0;
}

int do_sformat(stack *s) {
	int RERR, tok;
	t_lnum numval = 0;
	unsigned char width = 0, fmt_width; unsigned long significand;
	char prefix = 0;
	size_t strptr = s->head, baseptr, count = 0;
	if( (RERR = find_str(s, 0, &count)) )			return RERR;
#ifdef DEBUG
	printf("\t\tFormat str length: %lu\n", count);
#endif
	strptr -= count;
	baseptr = strptr - 1;
	fmt_lex l = make_fmt_lex((char*) (s->data + strptr));
	while( strptr < s->head ) {
#ifdef DEBUG
		printf("\t\t\tFormatting %c; strptr: %lu; baseptr: %lu\n", l.curr_char, strptr, baseptr);
#endif
		tok = next_token_fmt(&l);
		fmt_width = (l.c - (char*) s->data) - strptr;
		width = 0; significand = 0;
		switch( tok ) {
		  case FMT_CHAR:
#ifdef DEBUG
			printf("\t\tFound %%c\n");
#endif
			if( (RERR = peek_num(s, &numval, s->head - baseptr + 1, 1)) )	return RERR;
			baseptr -= 1;
			width = fmt_num_width_prep(&l, &numval, 1, &significand, &prefix);
			if( width > (unsigned) l.width )	{ stack_move(s, strptr + fmt_width, strptr + width); }
			else								{ stack_move(s, strptr + fmt_width, strptr + l.width); }
			fmt_num(&l, (char*) (s->data + strptr), numval, width, significand, prefix);
			l.c = s->data + strptr; l.curr_char = *l.c;
			break;
		  case FMT_RED:
#ifdef DEBUG
			printf("\t\tFound %%r\n");
#endif
			if( (RERR = peek_num(s, &numval, s->head - baseptr + 2, 2)) )	return RERR;
			baseptr -= 2;
			width = fmt_num_width_prep(&l, &numval, 2, &significand, &prefix);
			if( width > (unsigned) l.width )	{ stack_move(s, strptr + fmt_width, strptr + width); }
			else								{ stack_move(s, strptr + fmt_width, strptr + l.width); }
			fmt_num(&l, (char*) (s->data + strptr), numval, width, significand, prefix);
			l.c = s->data + strptr; l.curr_char = *l.c;
			break;
		  case FMT_INT:
#ifdef DEBUG
			printf("\t\tFound %%i\n");
#endif
			if( (RERR = peek_num(s, &numval, s->head - baseptr + 4, 4)) )	return RERR;
			baseptr -= 4;
			width = fmt_num_width_prep(&l, &numval, 4, &significand, &prefix);
			if( width > (unsigned) l.width )	{ stack_move(s, strptr + fmt_width, strptr + width); }
			else								{ stack_move(s, strptr + fmt_width, strptr + l.width); }
			fmt_num(&l, (char*) (s->data + strptr), numval, width, significand, prefix);
			l.c = s->data + strptr; l.curr_char = *l.c;
			break;
		  case FMT_LONG:
#ifdef DEBUG
			printf("\t\tFound %%l\n");
#endif
			if( (RERR = peek_num(s, &numval, s->head - baseptr + 8, 8)) )	return RERR;
			baseptr -= 8;
			width = fmt_num_width_prep(&l, &numval, 8, &significand, &prefix);
			if( width > (unsigned) l.width )	{ stack_move(s, strptr + fmt_width, strptr + width); }
			else								{ stack_move(s, strptr + fmt_width, strptr + l.width); }
			fmt_num(&l, (char*) (s->data + strptr), numval, width, significand, prefix);
			l.c = s->data + strptr; l.curr_char = *l.c;
			break;
		  case FMT_STR:
#ifdef DEBUG
			printf("\t\tFound %%s\n");
#endif
			if( (RERR = find_str(s, s->head - baseptr, &count)) )			return RERR;
			baseptr -= count + 1;
			if( l.width == -1 || count > (unsigned) l.width ) {
				stack_move(s, strptr + fmt_width, strptr + count);
				memcpy(s->data + strptr, s->data + baseptr + 1, count);
			}
			else {
				stack_move(s, strptr + fmt_width, strptr + l.width);
				memset(s->data + strptr, ' ', l.width - count);
				memcpy(s->data + strptr + l.width - count, s->data + baseptr + 1, count);
			}
			l.c = s->data + strptr; l.curr_char = *l.c;
			break;
		  case FMT_INV:														return RERR_INVFMT;
		  default:
			strptr++;
		}
	}
	return 0;
}

int do_sscan(stack *s) {
	int RERR, tok;
	t_lnum numval = 0;
	unsigned char width = 0;
	size_t fmtptr = s->head, strptr = s->head, fmtcount = 0, strcount = 0, baseptr = s->head, numptr;
	if( (RERR = find_str(s, 0, &fmtcount)) )								return RERR;
#ifdef DEBUG
	printf("\t\tFormat str length: %lu\n", fmtcount);
#endif
	fmtptr -= fmtcount;
	strptr -= fmtcount;
	if( (RERR = find_str(s, fmtcount+1, &strcount)) )						return RERR;
	strptr -= strcount;
	baseptr -= strcount + 1;
	numptr = baseptr;
	size_t fmtstart = fmtptr;

	*(char*) (s->data + s->head) = 0;
	fmt_lex l = make_fmt_lex((char*) (s->data + strptr));
	while( fmtptr < s->head ) {
#ifdef DEBUG
		printf("\t\t\tScanning %c; fmtptr: %lu; strptr: %lu\n", l.curr_char, fmtptr, strptr);
#endif
		tok = next_token_fmt(&l);
		fmtptr = (l.c - (char*) s->data);
		width = 0;
		switch( tok ) {
		  case FMT_CHAR:
#ifdef DEBUG
			printf("\t\tFound %%c\n");
#endif
			if( fparse_num(&l, s->data, &strptr, &numval) ) {
				s->head = baseptr + 1;
				*(char*) (s->data + s->head - 2) = 0;
				return 0;
			}
			if( baseptr + 1 >= strptr )
				memmove(s->data + strptr + 1, s->data + strptr, fmtptr - strptr);
			memcpy(s->data + numptr, &numval, 1);
			numptr += 1;
			break;
		  case FMT_RED:
#ifdef DEBUG
			printf("\t\tFound %%r\n");
#endif
			if( fparse_num(&l, s->data, &strptr, &numval) ) {
				s->head = baseptr + 1;
				*(char*) (s->data + s->head - 2) = 0;
				return 0;
			}
			if( baseptr + 2 >= strptr ) {
				int off = fmtstart + 2 - fmtptr;
				if( off > 0 ) {
					memmove(s->data + fmtptr + off, s->data + fmtptr, s->head - fmtptr);
					s->head += off; fmtstart += off; l.c += off;
					*(char*) (s->data + s->head) = 0;
				}
				memmove(s->data + baseptr + 2, s->data + strptr, fmtstart - strptr);
			}
			memcpy(s->data + numptr, &numval, 2);
			numptr += 2;
			break;
		  case FMT_INT:
#ifdef DEBUG
			printf("\t\tFound %%i\n");
#endif
			if( fparse_num(&l, s->data, &strptr, &numval) ) {
				s->head = baseptr + 1;
				*(char*) (s->data + s->head - 2) = 0;
				return 0;
			}
			if( baseptr + 4 >= strptr ) {
				int off = fmtstart + 4 - fmtptr;
				if( off > 0 ) {
					memmove(s->data + fmtptr + off, s->data + fmtptr, s->head - fmtptr);
					s->head += off; fmtstart += off; l.c += off;
					*(char*) (s->data + s->head) = 0;
				}
				memmove(s->data + baseptr + 4, s->data + strptr, fmtstart - strptr);
			}
			memcpy(s->data + numptr, &numval, 4);
			numptr += 4;
			break;
		  case FMT_LONG:
#ifdef DEBUG
			printf("\t\tFound %%l\n");
#endif
			if( fparse_num(&l, s->data, &strptr, &numval) ) {
				s->head = baseptr + 1;
				*(char*) (s->data + s->head - 2) = 0;
				return 0;
			}
			if( baseptr + 8 >= strptr ) {
				int off = fmtstart + 8 - fmtptr;
				if( off > 0 ) {
					memmove(s->data + fmtptr + off, s->data + fmtptr, s->head - fmtptr);
					s->head += off; fmtstart += off; l.c += off;
					*(char*) (s->data + s->head) = 0;
				}
				memmove(s->data + baseptr + 8, s->data + strptr, fmtstart - strptr);
			}
			memcpy(s->data + numptr, &numval, 8);
			numptr += 8;
			break;
		  case FMT_STR:
#ifdef DEBUG
			printf("\t\tFound %%s\n");
#endif
			while( *(char*) (s->data + strptr) && (width < (unsigned long) l.width) ) {
				strptr++; width++;
			}
			if( l.width == -1 ) break;
			else if( width < l.width ) {
				s->head = baseptr + 1;
				*(char*) (s->data + s->head - 2) = 0;
				return 0;
			}
			memmove(s->data + numptr, s->data + strptr - width, width);
			numptr += width;
			break;
		  case FMT_INV:														return RERR_INVFMT;
		  case ' ':
			while( isspace(*(char*) (s->data + strptr)) ) {
				strptr++;
			}
		  default:
			if( *(char*) (s->data + strptr) != *(char*) (s->data + fmtptr) ) {
				s->head = baseptr + 1;
				return -1; // TODO
			}
			strptr++;
		}
	}
	return 0;
}

int exec(stack *prog_stack, stack *data_stack) {
	size_t prog_p		= 0;
	int err				= 0;
	short magic			= 0;
	t_instr instr		= 0;
	t_lnum save			= 0;
	t_lnum val			= 0;
	unsigned char under = 0;
	t_rnum bytes		= *(t_rnum*) prog_stack->data;
	magic		= (t_rnum)		bytes & MASK_MAGIC;
	instr		= (t_instr) 	bytes & MASK_DATA;
#ifdef DEBUG
	printf("--------------------------------\n");
	printf("\t\tEXECUTING\t\t\n");
#endif
	for(;;) {
#ifdef SHOWSTACK
		printf("Program pointer at %lu ", prog_p);
		char buff[32] = {0};
		sprint_instr(buff, prog_stack->data + prog_p*INSTR_SIZE);
		printf("%s\n", buff);
#endif
		if( magic & MAGIC_CONT ) {
			sprintf(err_extra, "%04X @ PP %lu", magic, prog_p);					return RERR_UNEXP_CONT;
		} else if ( magic ) {
			val = 0;
			err = get_num(prog_stack->data + prog_p*INSTR_SIZE, magic, &val);
			if( err ) return err;
			prog_p += MAGIC_TO_SIZE(magic);
			err = push_num(data_stack, val, MAGIC_TO_SIZE(magic));
			if( err ) return err;
		} else {
			switch( instr ) {
			  case 0:
				sprintf(err_extra, "EOF @ PP %lu", prog_p);						return ERR_EOF;
			  case T_CADD:
				if( (err = do_add(data_stack, 1)) ) 	{ return err; }			prog_p++; break;
			  case T_RADD:
				if( (err = do_add(data_stack, 2)) ) 	{ return err; }			prog_p++; break;
			  case T_ADD:
				if( (err = do_add(data_stack, 4)) ) 	{ return err; }			prog_p++; break;
			  case T_LADD:
				if( (err = do_add(data_stack, 8)) ) 	{ return err; }			prog_p++; break;
			  case T_CSUB:
				if( (err = do_sub(data_stack, 1)) ) 	{ return err; }			prog_p++; break;
			  case T_RSUB:
				if( (err = do_sub(data_stack, 2)) ) 	{ return err; }			prog_p++; break;
			  case T_SUB:
				if( (err = do_sub(data_stack, 4)) ) 	{ return err; }			prog_p++; break;
			  case T_LSUB:
				if( (err = do_sub(data_stack, 8)) ) 	{ return err; }			prog_p++; break;
			  case T_CMUL:
				if( (err = do_mul(data_stack, 1)) ) 	{ return err; }			prog_p++; break;
			  case T_RMUL:
				if( (err = do_mul(data_stack, 2)) ) 	{ return err; }			prog_p++; break;
			  case T_MUL:
				if( (err = do_mul(data_stack, 4)) ) 	{ return err; }			prog_p++; break;
			  case T_LMUL:
				if( (err = do_mul(data_stack, 8)) ) 	{ return err; }			prog_p++; break;
			  case T_CDIV:
				if( (err = do_div(data_stack, 1)) ) 	{ return err; }			prog_p++; break;
			  case T_RDIV:
				if( (err = do_div(data_stack, 2)) ) 	{ return err; }			prog_p++; break;
			  case T_DIV:
				if( (err = do_div(data_stack, 4)) ) 	{ return err; }			prog_p++; break;
			  case T_LDIV:
				if( (err = do_div(data_stack, 8)) ) 	{ return err; }			prog_p++; break;
			  case T_CSWP:
				if( (err = do_swp(data_stack, 1)) ) 	{ return err; }			prog_p++; break;
			  case T_RSWP:
				if( (err = do_swp(data_stack, 2)) ) 	{ return err; }			prog_p++; break;
			  case T_SWP:
				if( (err = do_swp(data_stack, 4)) ) 	{ return err; }			prog_p++; break;
			  case T_LSWP:
				if( (err = do_swp(data_stack, 8)) ) 	{ return err; }			prog_p++; break;
			  case T_CDUP:
				if( (err = do_dup(data_stack, 1)) ) 	{ return err; }			prog_p++; break;
			  case T_RDUP:
				if( (err = do_dup(data_stack, 2)) ) 	{ return err; }			prog_p++; break;
			  case T_DUP:
				if( (err = do_dup(data_stack, 4)) ) 	{ return err; }			prog_p++; break;
			  case T_LDUP:
				if( (err = do_dup(data_stack, 8)) ) 	{ return err; }			prog_p++; break;
			  case T_JMP:
				if( (err = do_jmp(data_stack, prog_stack->head, &prog_p)) )  { return err; } break;
			  case '?':
			 	if( (err = do_cond(data_stack, prog_stack->head, &prog_p)) ) { return err; } break;
			  case T_CDEC:
				if( (err = do_dec(data_stack, 1)) )		{ return err; }			prog_p++; break;
			  case T_RDEC:
				if( (err = do_dec(data_stack, 2)) )		{ return err; }			prog_p++; break;
			  case T_DEC:
				if( (err = do_dec(data_stack, 4)) )		{ return err; }			prog_p++; break;
			  case T_LDEC:
				if( (err = do_dec(data_stack, 8)) )		{ return err; }			prog_p++; break;
			  case T_CINC:
				if( (err = do_inc(data_stack, 1)) )		{ return err; }			prog_p++; break;
			  case T_RINC:
				if( (err = do_inc(data_stack, 2)) )		{ return err; }			prog_p++; break;
			  case T_INC:
				if( (err = do_inc(data_stack, 4)) )		{ return err; }			prog_p++; break;
			  case T_LINC:
				if( (err = do_inc(data_stack, 8)) )		{ return err; }			prog_p++; break;
			  case T_CUND:
				if( (err = pop_num(data_stack, &save, 1)) ) { return err; } under = 1; prog_p++; goto next;
			  case T_RUND:
				if( (err = pop_num(data_stack, &save, 2)) ) { return err; } under = 2; prog_p++; goto next;
			  case T_UND:
				if( (err = pop_num(data_stack, &save, 4)) ) { return err; } under = 4; prog_p++; goto next;
			  case T_LUND:
				if( (err = pop_num(data_stack, &save, 8)) ) { return err; } under = 8; prog_p++; goto next;
			  case T_CCMP:
				if( (err = do_cmp(data_stack, 1)) )		{ return err; }			prog_p++; break;
			  case T_RCMP:
				if( (err = do_cmp(data_stack, 2)) )		{ return err; }			prog_p++; break;
			  case T_CMP:
				if( (err = do_cmp(data_stack, 4)) )		{ return err; }			prog_p++; break;
			  case T_LCMP:
				if( (err = do_cmp(data_stack, 8)) )		{ return err; }			prog_p++; break;
			  case '!':
				if( (err = do_not(data_stack)) )		{ return err; }			prog_p++; break;
			  case T_CDRP:
				if( (err = pop_num(data_stack, 0, 1)) ) { return err; }			prog_p++; break;
			  case T_RDRP:
				if( (err = pop_num(data_stack, 0, 2)) ) { return err; }			prog_p++; break;
			  case T_DRP:
				if( (err = pop_num(data_stack, 0, 4)) ) { return err; }			prog_p++; break;
			  case T_LDRP:
				if( (err = pop_num(data_stack, 0, 8)) ) { return err; }			prog_p++; break;
			  case T_OPN:
			 	if( (err = do_alloc(data_stack)) )		{ return err; }			prog_p++; break;
			  case T_CLS:
				if( (err = do_free(data_stack)) )		{ return err; }			prog_p++; break;
			  case T_OPNF:
			 	if( (err = do_open_file(data_stack)) )	{ return err; }			prog_p++; break;
			  case T_CLSF:
				if( (err = do_close_file(data_stack)) ) { return err; }			prog_p++; break;
			  case T_CPUT:
				if( (err = do_put(data_stack, 1)) )		{ return err; }			prog_p++; break;
			  case T_RPUT:
				if( (err = do_put(data_stack, 2)) ) 	{ return err; }			prog_p++; break;
			  case T_PUT:
				if( (err = do_put(data_stack, 4)) ) 	{ return err; }			prog_p++; break;
			  case T_LPUT:
				if( (err = do_put(data_stack, 8)) ) 	{ return err; }			prog_p++; break;
			  case T_CGET:
				if( (err = do_get(data_stack, 1)) ) 	{ return err; }			prog_p++; break;
			  case T_RGET:
				if( (err = do_get(data_stack, 2)) ) 	{ return err; }			prog_p++; break;
			  case T_GET:
				if( (err = do_get(data_stack, 4)) ) 	{ return err; }			prog_p++; break;
			  case T_LGET:
				if( (err = do_get(data_stack, 8)) ) 	{ return err; }			prog_p++; break;
			  case T_IN:
				if( (err = push_num(data_stack, (t_lnum) stdin, 8)) )  { return err; } prog_p++; break;
			  case T_OUT:
				if( (err = push_num(data_stack, (t_lnum) stdout, 8)) ) { return err; } prog_p++; break;
			  case T_SPUTF:
				if( (err = do_sputf(data_stack)) )		{ return err; }			prog_p++; break;
			  case T_SGETF:
				if( (err = do_sgetf(data_stack)) )		{ return err; }			prog_p++; break;
			  case T_SFMT:
				if( (err = do_sformat(data_stack)) ) 	{ return err; }			prog_p++; break;
			  case T_SSCN:
				if( (err = do_sscan(data_stack)) )		{ return err; }			prog_p++; break;
			  case T_SDRP:
				if( (err = do_sdrp(data_stack)) ) 		{ return err; } 		prog_p++; break;
			  case T_END:
			  	if( under ) {
			  		err = push_num(data_stack, save, under);
			  		if( err ) return err;
			  	} return 0;
			}
		}
		if( under ) {
			err = push_num(data_stack, save, under);
			under = 0;
			if( err ) return err;
		}
#ifdef SHOWSTACK
		printf("Stack state: ");
		print_stack(*data_stack);
		printf("----------------------------\n");
#endif
	  next:
		bytes = *(t_rnum*) (prog_stack->data + INSTR_SIZE*prog_p);
		magic	= (short)	bytes & MASK_MAGIC;
		val		= (t_num)	bytes & MASK_DATA;
		instr	= (t_instr) bytes & MASK_DATA;
	}
}

void print_prog(stack prog_stack) {
	char *buff = malloc(32);
	printf("Program: \n");
	for( size_t i = 0; i < prog_stack.head; ) {
		i += INSTR_SIZE*sprint_instr(buff, prog_stack.data + i);
		printf("%s", buff);
	}
	free(buff);
	printf("\n");
}

int main(int argc, char *argv[]) {
	FILE *pbc_file = 0;
	if( argc <= 1 ) { printf("Please provide a Polish bytecode file.\n"); return 1; }
	pbc_file = fopen(argv[1], "r");
	if( pbc_file == 0 ) { printf("File %s not found.\n", argv[1]); return 1; }
	fseek(pbc_file, 0, SEEK_END);
	PROG_STACK_SIZE = ftell(pbc_file);
	rewind(pbc_file);
	stack data_stack = make_stack(STACK_SIZE);
	stack prog_stack = make_stack(PROG_STACK_SIZE);
	fread(prog_stack.data, PROG_STACK_SIZE, 1, pbc_file);
	prog_stack.head += PROG_STACK_SIZE;
	fclose(pbc_file);
	int err = exec(&prog_stack, &data_stack);
	if( err ) { printf("%s%s%s\n", rerr_notify, rerr_strs[err - 1], err_extra); return 1; }
	//print_stack(data_stack);
	return 0;
}

