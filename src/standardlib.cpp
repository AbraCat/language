#include <standardlib.h>
#include <utils.h>

#include <stdlib.h>
#include <elf.h>

static const int phnum = 1, virtual_adr = 0x400000, code_offset = 0x1000, ph_align = 0x1000;
static const int input_offset = 0, output_offset = 2, exit_offset = 7;
static const int stdlib_buflen = 10000, max_bin_stdlib_len = 10000;

static const char asm_std_lib[] = "src/asm-standard.asm", stdlib_full[] = "bin/stdlib.exe",
                  stdlib_stripped[] = "bin/stdlib-stripped.exe";

ErrEnum stripStdlib()
{
    int file_size = 0;
    char* buf = NULL;
    returnErr(readFile(stdlib_full, (void**)&buf, &file_size));

    Elf64_Ehdr* hdr = (Elf64_Ehdr*)buf;

    int phdr_adr = hdr->e_phoff + hdr->e_phentsize; // second entry
    Elf64_Phdr* prog_header = (Elf64_Phdr*)(buf + phdr_adr);

    int p_vaddr = prog_header->p_vaddr;
    int p_offset = prog_header->p_offset;

    int entrypoint = hdr->e_entry - (p_vaddr - p_offset);

    FILE *fout = fopen(stdlib_stripped, "w");
    if (fout == NULL) return ERR_OPEN_FILE;
    fwrite(buf + p_offset, sizeof(char), entrypoint - p_offset, fout);
    fclose(fout);

    return ERR_OK;
}

void writeHeader(char* buf, int entrypoint)
{
    Elf64_Ehdr* hdr = (Elf64_Ehdr*)buf;
    hdr->e_ident[EI_MAG0] = ELFMAG0;
    hdr->e_ident[EI_MAG1] = ELFMAG1;
    hdr->e_ident[EI_MAG2] = ELFMAG2;
    hdr->e_ident[EI_MAG3] = ELFMAG3;

    hdr->e_ident[EI_CLASS] = ELFCLASS64;
    hdr->e_ident[EI_DATA] = ELFDATA2LSB;
    hdr->e_ident[EI_VERSION] = EV_CURRENT;
    hdr->e_ident[EI_OSABI] = ELFOSABI_NONE;
    
    hdr->e_type = ET_EXEC;
    hdr->e_machine = EM_X86_64;
    hdr->e_version = EV_CURRENT;
    hdr->e_entry = virtual_adr + code_offset + entrypoint;

    hdr->e_phoff = hdr->e_ehsize = sizeof(Elf64_Ehdr);
    hdr->e_phentsize = sizeof(Elf64_Phdr);
    hdr->e_phnum = phnum;
    writeProgramHeader(buf + hdr->e_phoff, PF_R | PF_X, 0);
}

void writeProgramHeader(char* buf, int flags, int offset)
{
    Elf64_Phdr* ph = (Elf64_Phdr*)buf;

    ph->p_type = PT_LOAD;
    ph->p_flags = flags;
    ph->p_offset = offset;
    ph->p_vaddr = virtual_adr + offset;
    ph->p_paddr = virtual_adr + offset;
    ph->p_align = ph_align;
}

ErrEnum getHeaderAndStdlib(char* buf)
{
    FILE* stdlib_file = fopen(stdlib_stripped, "r");
    if (stdlib_file == NULL) return ERR_OPEN_FILE;
    int stdlib_size = fread(buf + code_offset, 1, max_bin_stdlib_len, stdlib_file);

    writeHeader(buf, stdlib_size);
    fclose(stdlib_file);
    return ERR_OK;
}

void patchCodeSize(char* buf, int size)
{
    Elf64_Ehdr* hdr = (Elf64_Ehdr*)buf;
    int ph_adr = hdr->e_phoff;
    Elf64_Phdr* ph = (Elf64_Phdr*)(buf + ph_adr);
    ph->p_filesz = ph->p_memsz = size;
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
