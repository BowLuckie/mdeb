.intel_syntax noprefix
.global strlen
.text

strlen:
    xor rcx, rcx

    slen.loop:
        mov al, [rdi + rcx]
        cmp al, 0
        je slen.ret
        inc rcx
        jmp slen.loop

      slen.ret:
        mov rax, rcx
        ret
