.intel_syntax noprefix
.global _start
.text

_start: 
    lea rdi, [s_helloworld]
    call printf

.data
s_helloworld: .asciz "hello world!\n"
