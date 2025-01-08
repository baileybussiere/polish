#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#define INSTR_SIZE 2
#define BYTES_PER_NUM (INSTR_SIZE - 1)
#define BITS_PER_BYTE 8
#define BYTE_SIZE (1 << BITS_PER_BYTE)
#define MASK_MAGIC 0xFF00
#define MASK_DATA ~MASK_MAGIC
#define t_cnum unsigned char
#define t_rnum unsigned short
#define t_num  unsigned int
#define t_lnum unsigned long
#define t_instr char

#define MAGIC_CHAR	0x0100
#define MAGIC_RED	0x0200
#define MAGIC_INT	0x0300
#define MAGIC_LONG	0x0400
#define MAGIC_INSTR 0x0000
#define MAGIC_CONT	0x1000
#define T_TO_MAGIC(t) ((t) << BYTES_PER_NUM*BITS_PER_BYTE)
#define MAGIC_TO_T(m) ((m) >> BYTES_PER_NUM*BITS_PER_BYTE)
#define T_TO_SIZE(t) (1 << ((t) - 1))
#define MAGIC_TO_SIZE(m) T_TO_SIZE(MAGIC_TO_T(m))
#define ERR_EXTRA_LEN 48
#define F_TYPEMASK 0x000000FF
#define F_BASEMASK 0x0000FF00
#define isalphanum(c) !( (c) < '0' || ((c) > '9' && (c) < 'A') || ((c) > 'Z' && (c) < 'a') || (c) > 'z' )

#define SIZE_TO_MASK(s) (((unsigned long) 1 << 8*(s)) - 1)

/*
// Prefixes indicate the number of bytes the objects the instruction operates are.
// C(har)			-- 1 byte
// R(educed)		-- 2 bytes
// [none](integer)	-- 4 bytes
// L(ong)			-- 8 bytes
// S(tring)			-- any number of chars, on the stack, terminated by 0
// P(ointer to string)?
//					-- like STRING, but on stack is a LONG pointing to location of
//					   characters
//
// In the following ...->... indicates the change to the stack, everything to
// the left is popped (effectively or really) before the operation and everything
// to the right is pushed (effectively or really) after the operation.
// X indicates a number of bytes matching its prefix, otherwise C R I L S P are
// used
// ->X 				any literal
// ->L				ccp, in, out
// X->				Xdrp
// C->				?
// L->				free, jmp
// X->X				Xinc, Xdec
// C->C				!
// I->L				alloc
// L->X				Xget
// S->S				srev
// X->X X			Xdup
// L X->			Xput, Xputf (X=S)
// X X->X			Xadd, Xsub, Xmul, Xdiv
// X X->C			Xcmp,
// X X->X X			Xswp,
// ... S->... S:	Sfmt,
*/
enum {
	T_IDEN =	  5,
	T_CHAR =	  1,	T_RED =		  2,	T_INT =		  3,	T_LONG =	  4,
	T_EOF =		  0,
	T_CUND = 	 -1,	T_RUND =	 -2,	T_UND =		 -3,	T_LUND =	 -4,
	T_CSWP =	 -5,	T_RSWP =	 -6,	T_SWP =		 -7,	T_LSWP =	 -8,
	T_CDRP =	 -9,	T_RDRP =	-10,	T_DRP =		-11,	T_LDRP =	-12,
	T_CDUP =	-13,	T_RDUP =	-14, 	T_DUP =		-15,	T_LDUP =	-16,
	T_CPUT =	-17,	T_RPUT =	-18,	T_PUT =		-19,	T_LPUT =	-20,
	T_CGET =	-21,	T_RGET =	-22,	T_GET =		-23,	T_LGET =	-24,
	T_CCMP =	-25,	T_RCMP =	-26,	T_CMP =		-27,	T_LCMP =	-28,
	T_CINC =	-29,	T_RINC =	-30,	T_INC =		-31,	T_LINC =	-32,
	T_CDEC =	-33,	T_RDEC =	-34,	T_DEC =		-35,	T_LDEC =	-36,
	T_CADD =	-37,	T_RADD =	-38,	T_ADD =		-39,	T_LADD =	-40,
	T_CSUB =	-41,	T_RSUB =	-42,	T_SUB =		-43,	T_LSUB =	-44,
	T_CMUL =	-45,	T_RMUL =	-46,	T_MUL =		-47,	T_LMUL =	-48,
	T_CDIV =	-49,	T_RDIV =	-50,	T_DIV =		-51,	T_LDIV =	-52,

	T_SSWP =	-53,	T_SREV =	-54,	T_SSUB =	-55,
	T_SDRP =	-57,	T_CLSF =	-58,	T_CLS =		-59,
	T_SDUP =	-61,	T_OPNF =	-62,	T_OPN =		-63,	T_STOK =	-63,
	T_SPUT =	-65,	T_SPUTF =	-66,	T_OUT =		-67,	T_SFMT =	-68,
	T_SGET =	-69,	T_SGETF =	-70,	T_IN =		-71,	T_SSCN =	-72,
	T_SCMP =	-73,	T_SCAP =	-74,	T_ERR =		-75,	T_SLOW =	-76,

	T_JMP =		-77,	T_CPP =		-78,	T_END =		-79,
	T_NEW_LABEL = -81,	T_JMP_LABEL = -82,

	T_NOT_LEXED_YET = -500,
	T_INV_NUMPREF	= -501,
	T_INV_NUM 		= -502,
	T_INV_LABEL		= -503,
};

char *instr_names[] = {
	"",
	"cund",		"rund",		"und",		"lund",
	"cswp",		"rswp",		"swp",		"lswp",
	"cdrp",		"rdrp",		"drp",		"ldrp",
	"cdup",		"rdup",		"dup",		"ldup",
	"cput",		"rput",		"put",		"lput",
	"cget",		"rget",		"get",		"lget",
	"ccmp",		"rcmp",		"cmp",		"lcmp",
	"cinc",		"rinc",		"inc",		"linc",
	"cdec",		"rdec",		"dec",		"ldec",
	"cadd",		"radd",		"add",		"ladd",
	"csub",		"rsub",		"sub",		"lsub",
	"cmul",		"rmul",		"mul",		"lmul",
	"cdiv",		"rdiv",		"div",		"ldiv",
	"sswp",		"srev",		"ssub",		"",
	"sdrp",		"clsf",		"cls",		"",
	"sdup",		"opnf",		"opn",		"stok",
	"sput",		"sputf",	"out",		"sfmt",
	"sget",		"sgetf",	"in",		"sscn",
	"scmp",		"scap",		"err",		"slow",
	"jmp",  	"cpp",  	"end",		"",
	"new label","jmp label",
};

enum { /* COMPILATION ERRORS */
	ERR_POVERFLOW = 1,
	ERR_NUMTOOLARGE = 2,
	ERR_LEX = 3,
	ERR_INVINSTR = 4,
	ERR_EOF = 5,
	ERR_LABELREDEF = 6,
	ERR_LABELUNDEF = 7,
	ERR_LABELCHAROVERFLOW = 8,
};

const char *err_notify = "CMPL ERR: ";
const char *err_strs[] = {
	"Program stack overflow; ",
	"Overlarge number for type; ",
	"Invalid token; ",
	"Unknown instruction %s; ",
	"Program leaves valid memory before END; ",
	"Label redefinition; ",
	"No label matching ",
	"Total label length exceeds buffer; ",
};

enum { /* RUNTIME ERRORS */
	RERR_SOVERFLOW = 1,
	RERR_SUNDERFLOW = 2,
	RERR_INV_JMP = 3,
	RERR_MALF_NUM = 4,
	RERR_UNEXP_CONT = 5,
	RERR_RUNAWAYSTR = 6,
	RERR_STRGET = 7,
	RERR_INVFMT = 8,
};

const char *rerr_notify = "RUN ERR: ";
const char *rerr_strs[] = {
	"Stack overflow; ",
	"Stack underflow; ",
	"Invalid jump; ",
	"Malformed number; ",
	"Unexpected continuation bit; ",
	"Runaway string; ",
	"Error reading string; ",
	"Invalid format string; ",
};

char err_extra[ERR_EXTRA_LEN] = {0};

typedef struct {
	void *data;
	size_t head;
} stack;

stack make_stack(size_t size) {
	void *data = calloc(size, 1);
	size_t head = 0;
	return (stack) { data, head };
}

void stack_move(stack *s, size_t source, size_t dest) {
	size_t numbytes = s->head - source;
	memmove(s->data + dest, s->data + source, numbytes);
	s->head += (signed long) dest - (signed long) source;
}

void print_stack(stack s) {
	for (size_t i = 0; i < s.head; i++)
		printf("%02X ", *(t_cnum*) (s.data + i));
	printf("\n");
	return;
}

int get_num(const void *bytes, const t_rnum magic, t_lnum *val) {
	unsigned size = MAGIC_TO_SIZE(magic);
	t_cnum data = (*(t_rnum*) bytes) & MASK_DATA;
	*val = data;
	for (unsigned i = 1; i < size/BYTES_PER_NUM; i++) {
		t_rnum cmagic = (*(t_rnum*) (bytes + INSTR_SIZE*i)) & MASK_MAGIC;
		if ((cmagic ^ MAGIC_CONT) != magic) { 
			sprintf(err_extra, "%04X @ +%u (%04X)", cmagic, i, magic | MAGIC_CONT);
			return RERR_MALF_NUM;
		}
		data = (*(t_rnum*) (bytes + INSTR_SIZE*i)) & MASK_DATA;
		*val += (t_lnum) data << i*BITS_PER_BYTE*BYTES_PER_NUM;
	}
	return 0;
}

unsigned sprint_instr(char *buff, void *bytes) {
	short magic = (*(short*) bytes) & MASK_MAGIC;
	if (magic && !(magic & MAGIC_CONT)) {
		t_lnum val = 0;
		int err = get_num(bytes, magic, &val);
		if (err) sprintf(buff, "(??, INVALID)");
		switch (magic) {
			case MAGIC_CHAR:
				sprintf(buff, "(%04X, %c)", *(t_rnum*) bytes, (t_cnum) val);
				return 1;
			case MAGIC_RED:
				sprintf(
					buff, "(%04X%04X, %u)", *(t_rnum*) bytes, *(t_rnum*) (bytes + INSTR_SIZE),
					(t_rnum) val
				);
				return 2;
			case MAGIC_INT:
				sprintf(
					buff, "(%04X%04X%04X%04X, %u)", *(t_rnum*) bytes, *(t_rnum*) (bytes + INSTR_SIZE),
					*(t_rnum*) (bytes + INSTR_SIZE*2), *(t_rnum*) (bytes + INSTR_SIZE*3), (t_num) val
				);
				return 4;
			case MAGIC_LONG:
				sprintf(
					buff, "(%04X%04X%04X%04X%04X%04X%04X%04X, %lu)", *(t_rnum*) bytes, *(t_rnum*) (bytes + INSTR_SIZE),
					*(t_rnum*) (bytes + INSTR_SIZE*2), *(t_rnum*) (bytes + INSTR_SIZE*3),
					*(t_rnum*) (bytes + INSTR_SIZE*4), *(t_rnum*) (bytes + INSTR_SIZE*5),
					*(t_rnum*) (bytes + INSTR_SIZE*6), *(t_rnum*) (bytes + INSTR_SIZE*7),
					(t_lnum) val
				);
				return 8;
			default:
				sprintf(buff, "(%04X, INVALID)", *(t_rnum*) bytes);
				return 1;
		}
	}
	else if (magic == MAGIC_INSTR) {
		t_instr val_instr	= (t_cnum) (*(t_instr*) bytes & MASK_DATA);
		if (val_instr <= T_EOF) sprintf(buff, "(%04X, %s)", 	 *(t_rnum*) bytes, instr_names[-val_instr]);
		else					sprintf(buff, "(%04X, %c)", 	 *(t_rnum*) bytes, val_instr);
	}
	else sprintf(buff, "(%04X, INVALID)", *(t_rnum*) bytes);
	return 1;
}
