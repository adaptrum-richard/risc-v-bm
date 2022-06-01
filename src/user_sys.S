.section ".text.user"

.set SYS_PRINTF_NUMBER, 0 
.set SYS_SLEEP_NUMBER, 1
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
