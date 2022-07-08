#include "syscall.h"

.global kprintf
kprintf:
    li a7, SYS_printf
    ecall
    ret

.global fork
fork:
    li a7, SYS_fork
    ecall
    ret

.global exit
exit:
    li a7, SYS_exit
    ecall
    ret

.global wait
wait:
    li a7, SYS_wait
    ecall
    ret

.global pipe
pipe:
    li a7, SYS_pipe
    ecall
    ret

.global read
read:
    li a7, SYS_read
    ecall
    ret

.global kill
kill:
    li a7, SYS_kill
    ecall
    ret


.global exec
exec:
    li a7, SYS_exec
    ecall
    ret

.global fstat
fstat:
    li a7, SYS_fstat
    ecall
    ret

.global write
write:
    li a7, SYS_write
    ecall
    ret

.global close
close:
    li a7, SYS_close
    ecall
    ret

.global open
open:
    li a7, SYS_open
    ecall
    ret

.global mknod
mknod:
    li a7, SYS_mknod
    ecall
    ret

.global unlink
unlink:
    li a7, SYS_unlink
    ecall
    ret

.global link
link:
    li a7, SYS_link
    ecall
    ret

.global mkdir
mkdir:
    li a7, SYS_mkdir
    ecall
    ret

.global chdir
chdir:
    li a7, SYS_chdir
    ecall
    ret

.global dup
dup:
    li a7, SYS_dup
    ecall
    ret

.global getpid
getpid:
    li a7, SYS_getpid
    ecall
    ret

.global brk
brk:
    li a7, SYS_brk
    ecall
    ret

.global sleep
sleep:
    li a7, SYS_sleep
    ecall
    ret

.global uptime
uptime:
    li a7, SYS_uptime
    ecall
    ret

.global sbrk
sbrk:
    li a7, SYS_sbrk
    ecall
    ret
