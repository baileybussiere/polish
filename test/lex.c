#include <stdio.h>
#include "lex.h"

int main(void) {
	lex l = make_lex(stdin);
	int tok = next_tok(&l);
	printf("-------------------------------------\n");
	for (;;) {
		printf("TOK: %i\n", tok);
		printf("val_num: %lu\n", l.val_num);
		printf("-------------------------------------\n");
		if (!tok) return 0;
		tok = next_tok(&l);
	}
}
