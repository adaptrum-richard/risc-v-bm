.globl set_pgd
set_pgd:
    beqz a0, 1f
    csrw satp, a0
    sfence.vma zero, zero
1:
    ret

.globl cpu_switch_to
cpu_switch_to:
    /* Save context into prev->thread */
    sd ra, 0(a0)
    sd sp, 8(a0)
    sd s0, 16(a0)
    sd s1, 24(a0)
    sd s2, 32(a0)
    sd s3, 40(a0)
    sd s4, 48(a0)
    sd s5, 56(a0)
    sd s6, 64(a0)
    sd s7, 72(a0)
    sd s8, 80(a0)
    sd s9, 88(a0)
    sd s10, 96(a0)
    sd s11, 104(a0)

    /* Restore context from next->thread */
    ld ra, 0(a1)
    ld sp, 8(a1)
    ld s0, 16(a1)
    ld s1, 24(a1)
    ld s2, 32(a1)
    ld s3, 40(a1)
    ld s4, 48(a1)
    ld s5, 56(a1)
    ld s6, 64(a1)
    ld s7, 72(a1)
    ld s8, 80(a1)
    ld s9, 88(a1)
    ld s10, 96(a1)
    ld s11, 104(a1)
    andi tp, tp, 0xf /*此时tp低4位保留cpu id的值*/
    add tp, a1, tp/*tp高60为0，tp+= a1，保存task_struct到tp中*/
    ret
