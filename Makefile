.PHONY: all bin-stdlib run prc dis create_dir clean clean_log

all: exe/front.exe exe/middle.exe exe/spu-back.exe exe/asm-back.exe exe/bin-back.exe exe/proc.exe

bin-stdlib:
	@nasm -felf64 src/bin-standard.asm -o bin/stdlib.o
	@ld bin/stdlib.o --strip-all --strip-debug -o bin/stdlib.exe
	@strip --remove-section=shstrtab bin/stdlib.exe

run:
	@bin/prog.exe

prc:
	@exe/proc.exe

# --no-addresses --no-show-raw-insn
dis:
	@objdump -D -b binary -mi386 -Mx86-64 --start-address=0x1000 bin/prog.exe > txt/disasm.txt

create_dir:
	@mkdir d
	@mkdir o
	@mkdir exe
	@mkdir log
	@mkdir log/dump
	@mkdir dot-img

	@mkdir bin
	@mkdir asm
	@mkdir txt

	@mkdir d/stack
	@mkdir d/processor
	@mkdir d/assembler
	@mkdir d/disassembler
	@mkdir d/tree
	@mkdir d/compiler
	@mkdir d/frontend
	@mkdir d/middlend
	@mkdir d/backend
	@mkdir d/main

	@mkdir o/stack
	@mkdir o/processor
	@mkdir o/assembler
	@mkdir o/disassembler
	@mkdir o/tree
	@mkdir o/frontend
	@mkdir o/middlend
	@mkdir o/backend
	@mkdir o/main

clean:
	@rm -rf d o exe log dot-img

	@mkdir d
	@mkdir o
	@mkdir exe
	@mkdir log
	@mkdir log/dump
	@mkdir log/dot-src
	@mkdir log/dot-img

	@mkdir d/stack
	@mkdir d/processor
	@mkdir d/assembler
	@mkdir d/disassembler
	@mkdir d/tree
	@mkdir d/compiler
	@mkdir d/frontend
	@mkdir d/middlend
	@mkdir d/backend
	@mkdir d/main

	@mkdir o/stack
	@mkdir o/processor
	@mkdir o/assembler
	@mkdir o/disassembler
	@mkdir o/tree
	@mkdir o/compiler
	@mkdir o/frontend
	@mkdir o/middlend
	@mkdir o/backend
	@mkdir o/main

clean_log:
	@rmdir log /s /q
	@mkdir log
	@mkdir log/dot-src
	@mkdir log/dot-img
	@mkdir log/dump

CC:= gcc

COMMON_O_FILES := $(patsubst %.cpp,o/%.o,$(notdir $(wildcard src/*.cpp)))
STACK_O_FILES  := $(patsubst %.cpp,o/stack/%.o,$(notdir $(wildcard src/stack/*.cpp)))
PROC_O_FILES   := $(patsubst %.cpp,o/processor/%.o,$(notdir $(wildcard src/processor/*.cpp)))
ASM_O_FILES    := $(patsubst %.cpp,o/assembler/%.o,$(notdir $(wildcard src/assembler/*.cpp)))
DISASM_O_FILES := $(patsubst %.cpp,o/disassembler/%.o,$(notdir $(wildcard src/disassembler/*.cpp)))
TREE_O_FILES   := $(patsubst %.cpp,o/tree/%.o,$(notdir $(wildcard src/tree/*.cpp)))

FRONT_O_FILES   := $(patsubst %.cpp,o/frontend/%.o,$(notdir $(wildcard src/frontend/*.cpp)))
MIDDLE_O_FILES   := $(patsubst %.cpp,o/middlend/%.o,$(notdir $(wildcard src/middlend/*.cpp)))
BACK_O_FILES   := $(patsubst %.cpp,o/backend/%.o,$(notdir $(wildcard src/backend/*.cpp)))

MAIN_O_FILES   := $(patsubst %.cpp,o/main/%.o,$(notdir $(wildcard src/main/*.cpp)))

O_FILES:= $(COMMON_O_FILES) $(STACK_O_FILES) $(PROC_O_FILES) $(ASM_O_FILES) $(DISASM_O_FILES) $(TREE_O_FILES) $(COMP_O_FILES)

DED_FLAGS := -Wshadow -Winit-self -Wredundant-decls -Wcast-align -Wundef -Wfloat-equal -Winline -Wunreachable-code -Wmissing-declarations -Wmissing-include-dirs -Wswitch-enum -Wswitch-default -Wmain -Wextra -Wall -g -pipe -fexceptions -Wcast-qual -Wconversion -Wempty-body -Wformat-security -Wformat=2 -Wignored-qualifiers -Wlogical-op -Wno-missing-field-initializers -Wpointer-arith -Wstack-usage=8192 -Wstrict-aliasing -Wtype-limits -Wwrite-strings -Werror=vla -D_DEBUG -D_EJUDGE_CLIENT_SIDE
INCLUDE_FLAGS:= -I ./h -I ./h/processor -I ./h/stack -I ./h/assembler -I ./h/disassembler -I ./h/tree -I ./h/frontend -I ./h/middlend -I ./h/backend
CFLAGS:= $(INCLUDE_FLAGS) -O3 -Wno-unused-parameter -Wno-unused-function # $(DED_FLAGS)

exe/proc.exe: o/main/main-proc.o $(PROC_O_FILES) $(STACK_O_FILES) $(COMMON_O_FILES)
	@$(CC)    o/main/main-proc.o $(PROC_O_FILES) $(STACK_O_FILES) $(COMMON_O_FILES) -lm -o $@

exe/front.exe: o/main/main-front.o $(COMMON_O_FILES) $(TREE_O_FILES) $(FRONT_O_FILES)
	@$(CC)    o/main/main-front.o $(COMMON_O_FILES) $(TREE_O_FILES) $(FRONT_O_FILES) -o exe/front.exe

exe/middle.exe: o/main/main-middle.o $(COMMON_O_FILES) $(TREE_O_FILES) $(MIDDLE_O_FILES)
	@$(CC)    o/main/main-middle.o $(COMMON_O_FILES) $(TREE_O_FILES) $(MIDDLE_O_FILES) -o exe/middle.exe

exe/spu-back.exe: o/main/main-spu-back.o $(COMMON_O_FILES) $(TREE_O_FILES) $(BACK_O_FILES) $(ASM_O_FILES)
	@$(CC) o/main/main-spu-back.o $(COMMON_O_FILES) $(TREE_O_FILES) $(BACK_O_FILES) $(ASM_O_FILES) -o exe/spu-back.exe

exe/asm-back.exe: o/main/main-asm-back.o $(COMMON_O_FILES) $(TREE_O_FILES) $(BACK_O_FILES)
	@$(CC)    o/main/main-asm-back.o $(COMMON_O_FILES) $(TREE_O_FILES) $(BACK_O_FILES) -o exe/asm-back.exe

exe/bin-back.exe: o/main/main-bin-back.o $(COMMON_O_FILES) $(TREE_O_FILES) $(BACK_O_FILES)
	@$(CC)    o/main/main-bin-back.o $(COMMON_O_FILES) $(TREE_O_FILES) $(BACK_O_FILES) -o exe/bin-back.exe


include $(wildcard d/*.d)
include $(wildcard d/*/*.d)

o/%.o: src/%.cpp
	@$(CC) $< $(CFLAGS) -c -o $@
	@$(CC) -MM -MT $@ $(INCLUDE_FLAGS) $< -o $(patsubst o/%,d/%,$(patsubst %.o,%.d,$@))
