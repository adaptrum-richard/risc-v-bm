.section ".text.user"

.set SYS_printf , 0
.set SYS_fork   , 1
.set SYS_exit   , 2
.set SYS_wait   , 3
.set SYS_pipe   , 4
.set SYS_read   , 5
.set SYS_kill   , 6
.set SYS_exec   , 7
.set SYS_fstat  , 8
.set SYS_chdir  , 9
.set SYS_dup    ,10
.set SYS_getpid ,11
.set SYS_brk    ,12
.set SYS_sleep  ,13
.set SYS_uptime ,14
.set SYS_open   ,15
.set SYS_write  ,16
.set SYS_mknod  ,17
.set SYS_unlink ,18
.set SYS_link   ,19
.set SYS_mkdir  ,20
.set SYS_close  ,21
.set SYS_free   ,22
.globl call_sys_printf
call_sys_printf:
    li a7, SYS_printf
    ecall
    ret

.globl call_sys_sleep
call_sys_sleep:
    li a7, SYS_sleep
    ecall
    ret

.globl call_sys_malloc
call_sys_malloc:
    li a7, SYS_brk
    ecall
    ret

.globl call_sys_free
call_sys_free:
    li a7, SYS_free
    ecall
    ret
