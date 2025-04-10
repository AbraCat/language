.PHONY: all clean prc cmp

all: exe/proc.exe exe/comp.exe

prc: all
	@exe/proc.exe
	
cmp: all
	@exe/comp.exe

clean:
	@rmdir d /s /q
	@rmdir o /s /q
	@rmdir exe /s /q

	@mkdir d
	@mkdir o
	@mkdir exe

	@mkdir d\stack
	@mkdir d\processor
	@mkdir d\assembler
	@mkdir d\disassembler
	@mkdir d\tree
	@mkdir d\compiler

	@mkdir o\stack
	@mkdir o\processor
	@mkdir o\assembler
	@mkdir o\disassembler
	@mkdir o\tree
	@mkdir o\compiler

clean_log:
	@rmdir log /s /q
	@mkdir log
	@mkdir log\dot-src
	@mkdir log\dot-img
	@mkdir log\dump

CC:= gcc

COMMON_O_FILES := $(patsubst %.cpp,o/%.o,$(notdir $(wildcard src/*.cpp)))
STACK_O_FILES  := $(patsubst %.cpp,o/stack/%.o,$(notdir $(wildcard src/stack/*.cpp)))
PROC_O_FILES   := $(patsubst %.cpp,o/processor/%.o,$(notdir $(wildcard src/processor/*.cpp)))
ASM_O_FILES    := $(patsubst %.cpp,o/assembler/%.o,$(notdir $(wildcard src/assembler/*.cpp)))
DISASM_O_FILES := $(patsubst %.cpp,o/disassembler/%.o,$(notdir $(wildcard src/disassembler/*.cpp)))
TREE_O_FILES   := $(patsubst %.cpp,o/tree/%.o,$(notdir $(wildcard src/tree/*.cpp)))
COMP_O_FILES   := $(patsubst %.cpp,o/compiler/%.o,$(notdir $(wildcard src/compiler/*.cpp)))

O_FILES:= $(COMMON_O_FILES) $(STACK_O_FILES) $(PROC_O_FILES) $(ASM_O_FILES) $(DISASM_O_FILES) $(TREE_O_FILES) $(COMP_O_FILES)

DED_FLAGS := -Wshadow -Winit-self -Wredundant-decls -Wcast-align -Wundef -Wfloat-equal -Winline -Wunreachable-code -Wmissing-declarations -Wmissing-include-dirs -Wswitch-enum -Wswitch-default -Wmain -Wextra -Wall -g -pipe -fexceptions -Wcast-qual -Wconversion -Wempty-body -Wformat-security -Wformat=2 -Wignored-qualifiers -Wlogical-op -Wno-missing-field-initializers -Wpointer-arith -Wstack-usage=8192 -Wstrict-aliasing -Wtype-limits -Wwrite-strings -Werror=vla -D_DEBUG -D_EJUDGE_CLIENT_SIDE
INCLUDE_FLAGS:= -I ./h -I ./h/processor -I ./h/stack -I ./h/assembler -I ./h/disassembler -I ./h/tree -I ./h/compiler
CFLAGS:= $(INCLUDE_FLAGS) -Wno-unused-parameter -Wno-unused-function # $(DED_FLAGS)

exe/proc.exe: $(COMMON_O_FILES) $(STACK_O_FILES) $(PROC_O_FILES) $(TREE_O_FILES)
	@$(CC)    $(COMMON_O_FILES) $(STACK_O_FILES) $(PROC_O_FILES) $(TREE_O_FILES) -o exe/proc.exe

exe/comp.exe: $(COMMON_O_FILES) $(COMP_O_FILES) $(ASM_O_FILES) $(DISASM_O_FILES) $(TREE_O_FILES)
	@$(CC)    $(COMMON_O_FILES) $(COMP_O_FILES) $(ASM_O_FILES) $(DISASM_O_FILES) $(TREE_O_FILES) -o exe/comp.exe


include $(wildcard d/*.d)
include $(wildcard d/*/*.d)

o/%.o: src/%.cpp
	@$(CC) $< $(CFLAGS) -c -o $@
	@$(CC) -MM -MT $@ $(INCLUDE_FLAGS) $< -o $(patsubst o/%,d/%,$(patsubst %.o,%.d,$@))









# .PHONY: all clean run clean_log

# all: exe/main.exe

# # docs:
# # 	@doxygen Doxyfile

# run: all
# 	@exe/main.exe

# clean:
# 	@rmdir d /s /q
# 	@rmdir o /s /q
# 	@rmdir exe /s /q
# 	@mkdir d
# 	@mkdir o
# 	@mkdir exe

# clean_log:
# 	@rmdir log /s /q
# 	@mkdir log
# 	@mkdir log\dot-src
# 	@mkdir log\dot-img
# 	@mkdir log\dump
# 	@mkdir log\tex

# CC:= gcc

# O_FILES:= $(patsubst %.cpp,o/%.o,$(notdir $(wildcard src/*.cpp)))

# DED_FLAGS := -Wshadow -Winit-self -Wredundant-decls -Wcast-align -Wundef -Wfloat-equal -Winline -Wunreachable-code -Wmissing-declarations -Wmissing-include-dirs -Wswitch-enum -Wswitch-default -Wmain -Wextra -Wall -g -pipe -fexceptions -Wcast-qual -Wconversion -Wempty-body -Wformat-security -Wformat=2 -Wignored-qualifiers -Wlogical-op -Wno-missing-field-initializers -Wpointer-arith -Wstack-usage=8192 -Wstrict-aliasing -Wtype-limits -Wwrite-strings -Werror=vla -D_DEBUG -D_EJUDGE_CLIENT_SIDE

# CFLAGS:= -I ./h -Wno-unused-parameter -Wno-unused-function

# exe/main.exe: $(O_FILES)
# 	@$(CC) $(O_FILES) -o exe/main.exe

# include $(wildcard d/*.d)

# o/%.o: src/%.cpp
# 	@$(CC) $< $(CFLAGS) -c -o $@
# 	@$(CC) -MM -MT $@ -I ./h $< -o d/$(patsubst %.o,%.d,$(notdir $@))