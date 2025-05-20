#include <standardlib.h>

#include <stdlib.h>

static const int code_offset = 0x1000;
static const int p_offset_adr = 0x80;
static const int shoff_adr = 0x28, shoff_len = 8, shentsize_adr = 0x3a, shentsize_len = 6;

static const int input_offset = 0, output_offset = 2, exit_offset = 7;

static const int stdlib_buflen = 10000, max_bin_stdlib_len = 10000;
static const char asm_std_lib[] = "src/asm-standard.asm", bin_std_lib[] = "bin/stdlib.exe";

void removeSectionHeader(char* buf)
{
    for (int i = shoff_adr; i < shoff_adr + shoff_len; ++i) buf[i] = 0;
    for (int i = shentsize_adr; i < shentsize_adr + shentsize_len; ++i) buf[i] = 0;
}

ErrEnum getBinStdLib(FILE* file, char* buf)
{
    FILE* stdlib_file = fopen(bin_std_lib, "r");
    if (stdlib_file == NULL) return ERR_OPEN_FILE;
    int n_read = fread(buf, 1, max_bin_stdlib_len, stdlib_file);
    fclose(stdlib_file);

    myAssert(*(int*)(buf + p_offset_adr) == code_offset);
    removeSectionHeader(buf);

    return ERR_OK;
}

ErrEnum insertStdFunction(NameArr* name_arr, const char* name, int n_args, int offset)
{
    int name_id = 0;
    returnErr(insertName(name, name_arr, &name_id));
    name_arr->names[name_id].type = NAME_FUNC;
    name_arr->names[name_id].n_args = n_args;
    name_arr->names[name_id].adr = code_offset + offset;

    return ERR_OK;
}

ErrEnum insertStdNames(NameArr* name_arr)
{
    returnErr(insertStdFunction(name_arr, "input", 0, input_offset));
    returnErr(insertStdFunction(name_arr, "output", 1, output_offset));
    returnErr(insertStdFunction(name_arr, "exit", 1, exit_offset));
    return ERR_OK;
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
