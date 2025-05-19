global _start
section .text

input:
jmp _input
output:
jmp _output
exit:
jmp _exit

_input:
    push rbx
    push rcx
    push rdx
    push r8
    push rdi
    push rsi

    mov rax, 0
    mov rdi, 0
    sub rsp, 32
    mov rsi, rsp
    mov rdx, 32
    syscall

    xor rbx, rbx
    mov bl, [rsp]
    mov rdx, '-'
    cmp rbx, rdx
    jne input_positive_1

    ; negative (1)
    add rsp, 1
    sub rax, 1

    input_positive_1:
    sub rax, 1
    mov r8, rax  ; r8  - number of chars read (persists)
    mov rcx, rax ; rcx - number of chars read (used as counter)
    xor rax, rax

    input_loop:
    imul rax, 10
    xor rdx, rdx
    mov dl, [rsp]
    add rax, rdx
    sub rax, '0'
    add rsp, 1
    sub rcx, 1
    test rcx, rcx
    jne input_loop

    mov rdx, '-'
    cmp rbx, rdx
    jne input_positive_2

    ; negative (2)
    imul rax, -1
    sub rsp, 1

    input_positive_2:
    add rsp, 32
    sub rsp, r8

    pop rsi
    pop rdi
    pop r8
    pop rdx
    pop rcx
    pop rbx
    ret

_output:
    mov rax, [rsp + 8]
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi

    xor rbx, rbx
    cmp rax, rbx
    je output_zero
    jg output_positive

    ; negative
    push rax
    sub rsp, 8
    mov BYTE [rsp], '-'
    mov rax, 1
    mov rdi, 1
    mov rsi, rsp
    mov rdx, 1
    syscall
    add rsp, 8
    pop rax
    imul rax, -1

    output_positive:
    xor rcx, rcx
    mov rsi, rsp
    sub rsp, 32

    output_loop:
    test rax, rax
    je output_print

    xor rdx, rdx
    mov rbx, 10
    idiv rbx

    add rdx, '0'
    sub rsi, 1
    mov BYTE [rsi], dl
    inc rcx
    jmp output_loop

    output_zero:
    sub rsp, 8
    mov BYTE [rsp], '0'
    mov rax, 1
    mov rdi, 1
    mov rsi, rsp
    mov rdx, 1
    syscall
    add rsp, 8
    jmp output_end

    output_print:
    mov rax, 1
    mov rdi, 1
    mov rdx, rcx
    syscall
    add rsp, 32

    output_end:

    sub rsp, 8
    mov BYTE [rsp], 10
    mov rax, 1
    mov rdi, 1
    mov rsi, rsp
    mov rdx, 1
    syscall
    add rsp, 8

    xor rax, rax
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    ret

_exit:
    mov rdi, [rsp + 8]
    mov rax, 60
    syscall

_start:
    push 0
    call exit