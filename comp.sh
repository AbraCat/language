#!/bin/bash

input_file="txt/prog.txt"
output_file="bin/prog.exe"
backend="bin"

while [ -n "$1" ] ; do
	case $1 in
		"-i" | "--input")
			shift
			input_file=$1
		;;
		"-o" | "--output")
			shift
			output_file=$1
		;;
        "-b" | "--backend")
			shift
			backend=$1
            case $backend in
                "spu" | "asm" | "bin")
                ;;
                *)
                    echo "Invalid backend"
                    exit 1
                ;;
            esac
        ;;
	esac
	shift
done

exe/front.exe -i $input_file
exe/middle.exe
case $backend in
    "spu")
        exe/spu-back.exe
    ;;
    "asm")
        exe/asm-back.exe
        nasm -felf64 asm/prog.asm -o bin/prog.o
	    ld bin/prog.o -o $output_file
    ;;
    "bin")
        exe/bin-back.exe -o $output_file
        chmod +x $output_file
    ;;
esac
