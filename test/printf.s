.intel_syntax noprefix
.global printf
.global exit
.global htoa
.global itoa
.text

exit:
    mov rax, 60
    xor rdi, rdi
    syscall ;# exit(0)

printf:
    push rbp
    mov rbp, rsp

    push r15
    push r14
    push r13

    ;# allocate and store the current state of the registers, so we can interate over them with r15
    sub rsp, 88 ;# allocate an extra 8 for padding
    mov [rbp - 72], rdi ;# fmt string
    mov [rbp - 64], rsi
    mov [rbp - 56], rdx
    mov [rbp - 48], rcx
    mov [rbp - 40], r8
    mov [rbp - 32], r9

    mov r13, [rbp - 72] ;# fmt string ptr
    lea r14, [buf]      ;# buf ptr
    lea r15, [rbp - 64] ;# arg (registers) ptr

    .pf_loop:
        mov al, [r13] ;# current char of the fmt string

        cmp al, 0
        je .pf_done

        cmp al, '%'
        je .pf_handlearg ;# a single byte

        ;# if its nothing special, copy it to buf and move on
        mov [r14], al ;# *r14 is the top of buf
        inc r13 ;# next letter
        inc r14
        jmp .pf_loop

    .pf_handlearg:
        inc r13 ;# the verb specifier letter ptr
        mov al, [r13] ;# now hold the specifier

        cmp al, 'c'
        je .pf_handle_char

        cmp al, '%'
        je .pf_handle_percent

        cmp al, 's'
        je .pf_handle_string

        cmp al, 'u'
        je .pf_handle_unsigned

        cmp al, 'd'
        je .pf_handle_signed

        cmp al, 'x'
        je .pf_handle_hex

        jmp .pf_unsupported

    .pf_handle_char: ;# a single byte
        mov al, [r15] ;# the first byte of the arg
        mov [r14], al ;# write the byte from the arg into the buf

        ;# proceed to next letter
        inc r13
        inc r14
        add r15, 8 ;# we now are looking for a format for the next arg in line
        jmp .pf_loop

    .pf_handle_percent:
        mov byte ptr [r14], '%'
        inc r13 ;# next char
        inc r14
        jmp .pf_loop

    .pf_handle_string:
        ;# this arg is now treated as a (char *), so r15 is a char **
        mov rsi, [r15] ;# rsi is our reading char pointer
        jmp .pf_append_string

    .pf_append_string:
        mov al, [rsi] ;# char of char *rsi

        cmp al, 0 ;# write it until we hit a null byte
        je .pf_append_string_done

        mov [r14], al ;# write to the end of our string
        inc rsi ;# move to the next char in *r15
        inc r14 ;# increment buf writing head
        jmp .pf_append_string

        .pf_append_string_done:
            add r15, 8 ;# because we have finished writing this string, move focus to the next arg
            inc r13 ;# also focus the next letter in the fmt string, because the `s` is done
            jmp .pf_loop ;# becuase we hit a null byte, we dont want to write anything so our buf writer wont increment

    .pf_handle_unsigned:
        mov rdi, [r15] ;# convert the arg into a ascii printable char
        lea rsi, [rbp - 104] ;# the buffer to write to
        mov rdx, 32 ;# the buffer size
        call itoa ;# our ascii (char *) is now in *(rbp - 80 + len)
        mov rsi, rax
        jmp .pf_append_string
        jmp .pf_loop

    .pf_handle_signed:
        mov rdi, [r15] ;# int fmt_arg
        cmp rdi, 0 
        jl .pf_handle_negative
        jmp .pf_handle_unsigned

        .pf_handle_negative:
            mov byte ptr [r14], '-' 
            inc r14
            neg rdi ;# rdi is negative but itoa wants positive and we have the negative sign already
            lea rsi, [rsp-104]
            mov rdx, 32
            call itoa
            mov rsi, rax
            jmp .pf_append_string

    .pf_handle_hex:
        mov rdi, [r15] ;# rdi is our char *fmt_arg
        lea rsi, [rbp - 104]
        mov rdx, 32 
        call htoa
        mov rsi, rax
        jmp .pf_append_string

    .pf_unsupported:
        jmp .pf_loop

    .pf_done:
    mov byte ptr [r14], 0 ;# *r14 will always be a \0 anyway

    ;# print the final buf
    mov rax, 1 
    mov rdi, 1
    lea rsi, [buf]
    sub r14, rsi ;# start - end = size
    mov rdx, r14
    syscall ;# write(STDOUT, &buf, r14 - &buf) r14 is a pointer to the end of the buf

    add rsp, 88
    pop r13
    pop r14
    pop r15
    leave
    ret

itoa: 
    mov rax, rdi ;# move the input number into the division register
    dec rdx ;# becuase we are using the size to seek, we want to go to the last available address, not one over the end
    add rsi, rdx ;# add the supplied length to seek to the end of the buffer
    lea rdi, [rsi] ;# rdi is now our itoa-buffer writer head (ibuf)
    mov byte ptr [rdi], 0  ;# put a null byte at the end
    dec rdi

    .itoa_loop:
        xor rdx, rdx
        mov rcx, 10
        div rcx ;# rax /= rcx, rdx = rax % rcx
        add rdx, '0' ;# an ascii 0, 0x30
        mov byte ptr [rdi], dl ;# write a single byte (char) from into ibuf
        cmp rax, 0 ;# no quotient, only remainder
        je .itoa_done
        dec rdi ;# we are writing backwards
        jmp .itoa_loop

    .itoa_done:
        mov rax, rdi
        RET

htoa:
    mov rax, rdi ;# move the input number into the division register
    dec rdx ;# becuase we are using the size to seek, we want to go to the last available address, not one over the end
    add rsi, rdx ;# add the supplied length to seek to the end of the buffer
    lea rdi, [rsi] ;# rdi is now our itoa-buffer writer head (ibuf)
    mov byte ptr [rdi], 0  ;# put a null byte at the end
    dec rdi

    .htoa_loop:
        xor rdx, rdx 
        mov rcx, 16 ;# dividend
        div rcx ;# rax /= rcx, rdx = rax % rcx
        cmp rdx, 10
        jl .htoa_num
        jmp .htoa_letter

        .htoa_num:
            add rdx, '0' ;# an ascii 0, 0x30
            mov byte ptr [rdi], dl ;# write a single byte (char) from into ibuf
            cmp rax, 0 ;# no quotient, only remainder
            je .htoa_done
            dec rdi ;# we are writing backwards
            jmp .htoa_loop

        .htoa_letter:
            add rdx, 'a'
            sub rdx, 10
            mov [rdi], dl
            cmp rax, 0
            je .htoa_done
            dec rdi
            jmp .htoa_loop

    .htoa_done:
        mov rax, rdi
        RET

.data
fmt: .asciz "hello, %d"
name: .asciz "matey"
buf: .skip 128
