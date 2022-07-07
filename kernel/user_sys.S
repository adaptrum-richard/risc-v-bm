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
.set SYS_sbrk   ,22

.global printf
printf:
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

.global malloc
malloc:
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
free:
    li a7, SYS_sbrk
    ecall
    ret
