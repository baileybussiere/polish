#include "lex.h"

#define PBC_EXTEN "pbc"
#define DEBUG
#define MAX_LABELS 32

char* global_label_chars = 0;
char** global_label_idens = 0;
size_t* global_label_vals = 0;

int write_num(FILE *out_file, t_lnum num, const t_rnum magic) {
	unsigned size = MAGIC_TO_SIZE(magic);
#ifdef DEBUG
	printf("\tCMPL: pushing num of type %u, size %u.\n", MAGIC_TO_T(magic), size);
#endif
	unsigned pushes_needed = size / BYTES_PER_NUM;
	if (num >= BYTE_SIZE*size)	{
		sprintf(err_extra, "%lu >= 2^%u", num, size);			return ERR_NUMTOOLARGE;
	}
	t_rnum data = magic | (t_cnum) (num & 0xFF);
	fwrite(&data, 1, INSTR_SIZE, out_file);
	num >>= BITS_PER_BYTE*BYTES_PER_NUM;
	while (--pushes_needed) {
		t_rnum data = magic | MAGIC_CONT | (t_cnum) (num & 0xFF);
		fwrite(&data, 1, INSTR_SIZE, out_file);
		num >>= BITS_PER_BYTE*BYTES_PER_NUM;
	}
	return 0;
}

int write_instr(FILE* out_file, const t_instr instr) {
	t_rnum bytes = MAGIC_INSTR | (unsigned char) instr;
	fwrite(&bytes, 1, INSTR_SIZE, out_file);
	return 0;
}

int find_label(const char *iden, const size_t iden_len) {
	size_t i = 0;
	while( i < MAX_LABELS - 1 ) {
		if( *global_label_idens[i] == 0 ) return i;
		if( (size_t) (global_label_idens[i+1] - global_label_idens[i]) != iden_len ) { i++; continue; }
		if( memcmp(global_label_idens[i], iden, iden_len) == 0 ) return i;
	}
	for( size_t j = 0; j < (size_t) (global_label_idens[0] + MAX_LABELS*16 - global_label_idens[i]); j++ ) {
		if( global_label_idens[i][j] == 0 && j == iden_len) return i;
		if( global_label_idens[i][j] != iden[j] ) return MAX_LABELS;
	}
	return MAX_LABELS;
}

int new_label(const char *iden, const size_t iden_len, const size_t new_idx, const size_t prog_p) {
	size_t len_avail = global_label_idens[0] + 16*MAX_LABELS - global_label_idens[new_idx];
	if( iden_len > len_avail ) {
		sprintf(err_extra, "%*.s, %lu avail.", (int) iden_len, iden, len_avail); //TODO
		return ERR_LABELCHAROVERFLOW;
	}
	memcpy(global_label_idens[new_idx], iden, iden_len);
	global_label_idens[new_idx + 1] = global_label_idens[new_idx] + iden_len;
	global_label_vals[new_idx] = prog_p;
	return 0;
}

int compile(FILE *in_file, FILE *out_file) {
	global_label_idens = malloc(MAX_LABELS);
	global_label_chars = calloc(MAX_LABELS*16, 1);
	global_label_vals = malloc(MAX_LABELS*sizeof(size_t));
	global_label_idens[0] = global_label_chars;

	lex l = make_lex(in_file);
	int tok, err, cond = 0;
	size_t label_idx = 0, prog_p = 0;
	while (tok = next_tok(&l), tok) {
		switch (tok) {
		  case T_EOF: return 0;
		  case T_IDEN:
		  	printf(err_strs[ERR_INVINSTR - 1], l.val_iden); return ERR_INVINSTR;
		  	break;
		  case T_CHAR...T_LONG:
			if( cond ) {
				if( (err = write_instr(out_file, '?')) ) return err;
				prog_p++; cond = 0;
			}
			prog_p += T_TO_SIZE(tok);
			if( (err = write_num(out_file, l.val_num, T_TO_MAGIC(tok))) ) return err;
			break;
		  case (-600)...T_NOT_LEXED_YET:
			printf("LEX ERR: %s\n", err_strs[T_NOT_LEXED_YET - err]);
			return ERR_LEX;
		  case T_NEW_LABEL:
#ifdef DEBUG
			printf("Trying to add a new label at %lu...\n", prog_p);
#endif
			if( (label_idx = find_label(l.val_iden, l.val_iden_count)) != MAX_LABELS) {
				if( *global_label_idens[label_idx] ) {
					sprintf(err_extra, "%*.s, def: %lu", (int) l.val_iden_count, l.val_iden, prog_p);
					return ERR_LABELREDEF;
				}
			}
#ifdef DEBUG
			printf("...Label not found, good.\n");
#endif
			if( (err = new_label(l.val_iden, l.val_iden_count, label_idx, prog_p)) ) return err;
			break;
		  case T_JMP_LABEL:
#ifdef DEBUG
			printf("Trying to add jmp to label...\n");
#endif
			if( (label_idx = find_label(l.val_iden, l.val_iden_count)) == MAX_LABELS) return ERR_LABELUNDEF;
			if( *global_label_idens[label_idx] == 0 ) return ERR_LABELUNDEF;
			if( (err = write_num(out_file, global_label_vals[label_idx], MAGIC_LONG)) ) return err;
			prog_p += 8;
			if( cond ) {
				if( (err = write_instr(out_file, T_LUND)) ) return err;
				if( (err = write_instr(out_file, '?')) ) return err;
				if( (err = write_instr(out_file, T_JMP)) ) return err;
				if( (err = write_instr(out_file, T_LDRP)) ) return err;
				prog_p += 4; cond = 0;
			}
			else {
				if( (err = write_instr(out_file, T_JMP)) ) return err;
				prog_p++;
			}
			break;
		  case '?':
			cond = 1;
			break;
		  default:
			if( cond ) {
				if( (err = write_instr(out_file, '?')) ) return err;
				prog_p++;
				cond = 0;
			}
			if( (err = write_instr(out_file, tok)) ) { printf("%s%s%s\n", err_notify, err_strs[err - 1], err_extra); return err; }
			prog_p++;
			break;
		}
	}
	return 0;
}

char *extension_to_pbc(char *file_path) {
	char *c = file_path;
	char *last_dot = c;
	while (*c) {
		if (*c == '.') last_dot = c;
		c++;
	}
	char *out = malloc(last_dot - file_path + 1 + sizeof(PBC_EXTEN));
	strcpy(out, file_path);
	strcpy(out + (last_dot - file_path) + 1, PBC_EXTEN);
	return out;
}

int main(int argc, char *argv[]) {
	FILE *input_file = 0;
	FILE *output_file = 0;
	switch (argc) {
	  case 2:
	  	if (strcmp(argv[1], "-") == 0) {
	  		input_file = stdin;
	  		output_file = stdout;
	  	} else {
	  		input_file = fopen(argv[1], "r");
	  		char *out_filename = extension_to_pbc(argv[1]);
	  		output_file = fopen(out_filename, "w");
	  		free(out_filename);
	  	} break;
	  case 3:
		if (strcmp(argv[1], "-") == 0) input_file = stdin;
		else input_file = fopen(argv[1], "r");
		output_file = fopen(argv[2], "w");
		break;
	  default:
		if (argc <= 1) { printf("Please provide an input file or '-' for standard input.\n"); return 1; }
		else { printf("Too many arguments.\n"); return 1; }
	}
	if (input_file == 0) { printf("File %s not found.\n", argv[1]); return 1; }
	if (output_file == 0) { printf("Couldn't open file %s.\n", argv[2]); return 1; }
	int err = compile(input_file, output_file);
	if (err) {
		printf("%s%s%s\n", err_notify, err_strs[err], err_extra);
		printf("Program read error.\nExiting...\n");
		return 1;
	}
}
