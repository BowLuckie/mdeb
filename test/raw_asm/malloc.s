.intel_syntax noprefix
.global malloc
.global brk
.text

malloc:
    push rbp
    mov rbp, rsp
    sub rsp, 16 

    ;# rdi contains number of bytes requested
    cmp rdi, 0 
    jle .malloc_invalid

    ;# push rdi
    mov [rbp-16], rdi ;# [rbp-16] = byte request

    mov rsi, rdi
    lea rdi, [s_requested_size]
    call printf
    ;#pop rdi
    mov rdi, [rbp-16]

    mov rax, rdi
    add rax, offset ALIGNMENT - 1
    lea rcx, [ALIGNMENT - 1]
    not rcx
    and rax, rcx
    mov rdi, rax
    mov [rbp-16], rdi

    mov rax, [heap_ptr]
    add rax, rdi
    cmp rax, [heap_end]

    jg .malloc_grow_heap

    .malloc_alloc:

        ;# push rdi

        mov rsi, rdi
        lea rdi, [s_allocation]
        mov rdx, [heap_ptr]
        call printf

        ;# pop rdi
        mov rdi, [rbp-16]

        mov rax, [heap_ptr]
        add rdi, rax
        mov [heap_ptr], rdi

        leave
        ret

        .malloc_brk_failed:
            lea rdi, [s_errbrk]
            call printf
            mov rax, -1
            leave
            ret

        .malloc_invalid:
            lea rdi, [s_errinvalid]
            call printf
            mov rax, -1
            leave
            ret


    .malloc_grow_heap: 
        cmp qword ptr [heap_end], 0x0
        je .malloc_init

        .malloc_ready:

            sub rax, [heap_end]

            mov rdx, rax

            mov r8, rax

            add rax, 0xfff ;# page size -1 to calculate overflow
            xor rdx, rdx
            mov rcx, 0x1000
            div rcx

            push rax

            mov rdx, r8
            mov rsi, rdi
            lea rdi, [s_missing_bytes]
            mov rcx, rax
            call printf

            pop rax ;# needed pages

            imul rax, 0x1000
            add rax, [heap_end]
            mov rdi, rax
            push rdi
            call brk
            pop rdi
            cmp rdi, rax
            jl .malloc_brk_failed
            
            mov [heap_end], rax
            mov rsi, rax
            lea rdi, [s_heap_end]
            call printf

            ;# pop rdi
            mov rdi, [rbp-16]
            jmp .malloc_alloc

        .malloc_init:
            xor rdi, rdi 
            call brk

            mov [heap_start], rax
            mov [heap_end], rax
            mov [heap_ptr], rax

            push rax
            lea rdi, [s_init]
            call printf
            pop rax

            push rax
            lea rdi, [s_current_heap] ;# fmt string
            mov rsi, rax
            call printf
            pop rax

            ;#pop rdi
            mov rdi, [rbp-16]
            add rax, rdi

            jmp .malloc_ready

brk: ;# pass new addr or 0 for query through rdi, returns new addr in rax
    mov rax, 12
    ;# rdi is supplied by caller
    syscall
    ret

.data
heap_start: .quad   0x0
heap_end: .quad     0x0
heap_ptr: .quad     0x0

s_heap_start:      .string "\033[36mheap_start:\033[0m\t\033[33m0x%x\033[0m\n"
s_heap_end:        .string "new \033[36mheap_end:\033[0m\t\033[33m0x%x\033[0m\n"
s_current_heap:    .string "new \033[36mheap_current:\033[0m\t\033[33m0x%x\033[0m\n"
s_requested_size:  .string "\033[34mrequested\033[0m \033[35m%d bytes\033[0m ... "
s_allocation:      .string "aligned size to %d \033[32mallocated\033[0m \033[35m\033[0m\t\033[33m0x%x\033[0m\n"
s_missing_bytes:   .string "\033[31mrequested %d bytes, but we're missing %d bytes\033[0m, and will need \033[35m%d pages\033[0m to cover it\n"
s_init:            .string "\033[32mmalloc ready\033[0m\n"
s_errinvalid: .string "\033[31m<ALLOCATION ERROR> invalid bytes requested\033[0m\n"
s_errbrk: .string "\033[31m<ALLOCATION ERROR> the os failed to allocate\033[0m\n"
ALIGNMENT = 16
