.section ".text.user"

.set SYS_PRINTF_NUMBER, 0 
.set SYS_SLEEP_NUMBER, 1
.set SYS_MALLOC_NUMBER, 2
.set SYS_FREE_NUMBER, 3
.globl call_sys_printf
call_sys_printf:
    li a7, SYS_PRINTF_NUMBER
    ecall
    ret

.globl call_sys_sleep
call_sys_sleep:
    li a7, SYS_SLEEP_NUMBER
    ecall
    ret

.globl call_sys_malloc
call_sys_malloc:
    li a7, SYS_MALLOC_NUMBER
    ecall
    ret

.globl call_sys_free
call_sys_free:
    li a7, SYS_FREE_NUMBER
    ecall
    ret
