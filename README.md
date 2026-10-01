heres something fun you can do:

here we have an elf binary written in intel sytle gas

```gas
.intel_syntax noprefix

_start: 
    lea rdi, [s_helloworld]
    call printf
```

simply, its going to print this string

```gas
.data
s_helloworld: .asciz "hello world!\n"
```

which it does!

```sh
> ./build/mdeb.bin test/raw_asm/build/main.bin 
Started debugging process 27509
mdeb> c
hello world!
[mdeb] exited with 0
mdeb>
```

if we inspect the binary further, we can see whats going on

```sh
> readelf -x .data test/raw_asm/build/main.bin

Hex dump of section '.data':
  0x00403030 68656c6c 6f20776f 726c6421 0a006865 hello world!..he
  0x00403040 6c6c6f2c 20256400 6d617465 79000000 llo, %d.matey... # "hello, %d\0matey" is a test string from printf.s
  0x00403050 00000000 00000000 00000000 00000000 ................
  0x00403060 00000000 00000000 00000000 00000000 ................
  0x00403070 00000000 00000000 00000000 00000000 ................
  0x00403080 00000000 00000000 00000000 00000000 ................
  0x00403090 00000000 00000000 00000000 00000000 ................
  0x004030a0 00000000 00000000 00000000 00000000 ................
  0x004030b0 00000000 00000000 00000000 00000000 ................
  0x004030c0 00000000 00000000 00000000 0000     ..............

> objdump -dw test/raw_asm/build/main.bin | head

test/raw_asm/build/main.bin:     file format elf64-x86-64


Disassembly of section .text:

0000000000401000 <_start>:
  401000:       48 8d 3c 25 30 30 40 00         lea    0x403030,%rdi
  401008:       e8 0c 00 00 00          call   401019 <printf>
```

so, if we wanted to print a new string, we could overwrite the hello world string, or we could write into the empty space a new string and `lea` its address into `rdi` to print.

```sh
> ./build/mdeb.bin test/raw_asm/build/main.bin
Started debugging process 27067
mdeb> mem w 0x00403050 0x6D6172666E69616D
mdeb> mem w 0x00403058 0x657262202E2E2E65
mdeb> mem w 0x00403060 0x0000216465686361
mdeb> mem w 0x401000 0x00403050253c8d48
```

since we cant write bytes only at a time, we have to overwrite the whole `lea ...` instruction with a new address. basically it looks like this when decoded

```at&t
lea    0x403050,%rdi
```

so its pointing at 0x403050, which we overwrote with the 3 previous commands with a new string. when can then see running it calls printf with the new address and prints our secret string! 

```sh
mdeb> c
mainframe... breached!% # we did not add a newline
[mdeb] exited with 0
```
