.section .text.boot

.globl _start
_start:
    la sp, stack0
    li a0, 1024*4
    csrr a1, mhartid
    addi a1, a1, 1
    mul a0, a0, a1
    add sp, sp, a0
    
    la	t0, bss_begin
    la	t1, bss_end
clear_bss_loop:
    sd  x0, 0(t0)
    addi t0, t0, 8
    blt t0, t1, clear_bss_loop

    call start
spin:
    j spin
