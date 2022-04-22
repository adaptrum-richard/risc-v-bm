

.globl ret_from_kernel_thread
.align 4
ret_from_kernel_thread:
    call schedule_tail
    mv a0, s1
    jr s0
    