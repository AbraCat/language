#include <standartlib.h>

#include <stdlib.h>

static const int stdlib_buflen = 10000, max_bin_stdlib_len = 10000;
static const char asm_std_lib[] = "src/standard.asm", bin_std_lib[] = "bin/stdlib.exe";

ErrEnum getBinStdLib(FILE* file, char* buf)
{
    FILE* stdlib_file = fopen(bin_std_lib, "r");
    if (stdlib_file == NULL) return ERR_OPEN_FILE;
    int n_read = fread(buf, 1, max_bin_stdlib_len, stdlib_file);
    fclose(stdlib_file);

    for (int i = 0x28; i < 0x28 + 8; ++i) buf[i] = 0; // remove section header table
    for (int i = 0x3a; i < 0x3a + 6; ++i) buf[i] = 0; // remove section header table

    return ERR_OK;
}

void insertStdNames(NameArr* name_arr)
{
    int name_id = 0;
    insertName("input", name_arr, &name_id);
    name_arr->names[name_id].type = NAME_FUNC;
    name_arr->names[name_id].n_args = 0;
    name_arr->names[name_id].adr = 0x1000;

    insertName("output", name_arr, &name_id);
    name_arr->names[name_id].type = NAME_FUNC;
    name_arr->names[name_id].n_args = 1;
    name_arr->names[name_id].adr = 0x1000 + 2;

    insertName("exit", name_arr, &name_id);
    name_arr->names[name_id].type = NAME_FUNC;
    name_arr->names[name_id].n_args = 1;
    name_arr->names[name_id].adr = 0x1000 + 7;
}

void printStdLib(FILE* fout, const char* trash_reg, const char* ret_val_reg, const char* frame_adr_reg)
{
    fprintf(fout, "PUSH 0\nCALL main:\nHLT\n"
    "input:\nPOP %s\nIN\nRET\n"
    "output:\nOUT\nPOP %s\nPUSH 0\nRET\n"
    "sqrt:\nSQRT\nPOP %s\nPOP %s\nPUSH %s\nRET\n",
    frame_adr_reg, frame_adr_reg, ret_val_reg, frame_adr_reg, ret_val_reg);
}

void asmPrintStdLib(FILE* file)
{
    FILE* stdlib_file = fopen(asm_std_lib, "r");
    char* stdlib_buf = (char*)calloc(stdlib_buflen, sizeof(char));

    int n_read = fread(stdlib_buf, sizeof(char), stdlib_buflen, stdlib_file);
    fwrite(stdlib_buf, sizeof(char), n_read, file);

    fclose(stdlib_file);
    free(stdlib_buf);
}
