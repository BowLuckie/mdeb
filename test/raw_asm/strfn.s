.intel_syntax noprefix
.global _start
.text

match:
    lea rdi, [s_match]
    call print
    call exit
    
nomatch:
    lea rdi, [s_nomatch]
    call print
    call exit
strcmp: ;# arg1: rdi, arg2: rsi
    mov al, [rdi]
    mov dl, [rsi]
    cmp al, dl
    jne sc.done
    
    cmp al, 0
    je sc.done
    
    inc rdi
    inc rsi
    jmp strcmp
    
    sc.done:
        sub al, dl
        ret    
        
strncmp: ;# arg1: rdi, arg2: rsi, arg3: rdx
    mov rcx, rdx
    snc.loop:
        dec rcx
        mov al, [rdi]
        mov dl, [rsi]
        cmp al, dl
        jne snc.done
    
        cmp al, 0
        je snc.done
    
        cmp rcx, 0
        je snc.done
    
        inc rdi
        inc rsi
    jmp snc.loop
    
    snc.done:
        sub al, dl
        ret    
        
print:
        push rdi
        call strlen
        pop rsi
        mov rdx, rax
        mov rax, 1
        mov rdi, 1
        syscall
        ret

.data
s1: .asciz "Abc"
s2: .asciz "Ab"
s_match: .asciz "The strings match!\n"
s_nomatch: .asciz "The strings DO NOT match!\n"
