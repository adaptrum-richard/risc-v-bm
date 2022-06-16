.globl kernelvec
.align 4
kernelvec:
        /*sscratch为0，表示中断或异常在内核态触发，否则在用户态*/
        csrrw tp, sscratch, tp //将tp和sscratch的值交换
        //如果来自内核，sscratch则为0, 如果来自用户空间，则sscratch指向用户进程的task
        bnez tp, __save_context
        
        csrr tp, sscratch /*如果是内核态，则恢复tp的值,tp指向当前的进程task*/
        sd sp, 112(tp) //将此时的内核栈指针保存到task_struct的kernel_sp上
__save_context:
        /*TASK_TI_USER_SP*/
        sd sp, 120(tp) //将此时的内核栈指针保存到task_struct的user_sp上
        //TASK_TI_KERNEL_SP
        ld sp, 112(tp) //内核sp

        //保存一个pt_regs
        addi sp, sp, -288
        // save the registers.
        sd ra, 8(sp)
        //sd sp, 16(sp)
        sd gp, 24(sp)
        //sd tp, 32(sp)
        sd t0, 40(sp)
        sd t1, 48(sp)
        sd t2, 56(sp)
        sd s0, 64(sp)
        sd s1, 72(sp)
        sd a0, 80(sp)
        sd a1, 88(sp)
        sd a2, 96(sp)
        sd a3, 104(sp)
        sd a4, 112(sp)
        sd a5, 120(sp)
        sd a6, 128(sp)
        sd a7, 136(sp)
        sd s2, 144(sp)
        sd s3, 152(sp)
        sd s4, 160(sp)
        sd s5, 168(sp)
        sd s6, 176(sp)
        sd s7, 184(sp)
        sd s8, 192(sp)
        sd s9, 200(sp)
        sd s10, 208(sp)
        sd s11, 216(sp)
        sd t3, 224(sp)
        sd t4, 232(sp)
        sd t5, 240(sp)
        sd t6, 248(sp)
        //status
        csrr s1, sstatus
        sd s1, 256(sp)
        //sepc
	csrr s2, sepc
        sd s2, 0(sp)
        //badaddr
        csrr s3, stval
        sd s3, 264(sp)
        //cause
        csrr s4, scause
        sd s4, 272(sp)

        ld s0, 120(tp) // TASK_TI_USER_SP
        sd s0, 16(sp) // save to pt_regs->sp

        csrr s5, sscratch
        sd s5, 32(sp)// save to tp


        csrw sscratch, x0 //scratch清0

        bge s4, zero, 1f //判断是中断还是异常, bge将把s4的二进制补码作为数据和zero比较
        //中断
        la ra, ret_from_exception //设置返回地址
        addi a0, sp, 0 /*a0为pt_regs，作为参数传递*/
        la a1, handler_irq
        jr a1
1:      /*异常*/
        la ra, ret_from_exception
        /*判断是否为系统调用*/
        li t0, 8
        beq s4, t0, handle_syscall
        //其他异常处理
        addi a0, sp, 0
        la a1, hanlder_exception
        jr a1

ret_from_exception:
        ld s0, 256(sp)
        csrc sstatus, (1<<1) //清除IE位,禁止S模式中断
        andi s0, s0, (1<<8) //Previous mode, 1=Supervisor, 0=User
        bnez s0, resume_kernel //从内核返回

resume_userspace:
        //TODO:处理work pending
        ld s0, 136(tp) //检查NEED_RESCHED标志
        andi s0, s0, 1
        bnez s0, user_resched 

        addi s0, sp, 288
        sd s0, 112(tp) //TASK_TI_KERNEL_SP
        csrw sscratch, tp
        

restore_all:
        // restore registers.
        //sstatus
        ld a0, 256(sp)
        csrw sstatus, a0
        //sepc
        ld a2, 0(sp)
        sc.d x0, a2, 0(sp)
        csrw sepc, a2
        
	ld ra, 8(sp)
        ld gp, 24(sp)
        ld tp, 32(sp)
        ld t0, 40(sp)
        ld t1, 48(sp)
        ld t2, 56(sp)
        ld s0, 64(sp)
        ld s1, 72(sp)
        ld a0, 80(sp)
        ld a1, 88(sp)
        ld a2, 96(sp)
        ld a3, 104(sp)
        ld a4, 112(sp)
        ld a5, 120(sp)
        ld a6, 128(sp)
        ld a7, 136(sp)
        ld s2, 144(sp)
        ld s3, 152(sp)
        ld s4, 160(sp)
        ld s5, 168(sp)
        ld s6, 176(sp)
        ld s7, 184(sp)
        ld s8, 192(sp)
        ld s9, 200(sp)
        ld s10, 208(sp)
        ld s11, 216(sp)
        ld t3, 224(sp)
        ld t4, 232(sp)
        ld t5, 240(sp)
        ld t6, 248(sp)

        ld sp, 16(sp) //restore sp
        sret

resume_kernel:
        //TODO: 判断是否需要抢占调度
        ld s0, 128(tp) //检查preempt_count
        bnez s0, restore_all
        ld s0, 136(tp) //检查NEED_RESCHED标志
        andi s0, s0, 1
        beqz s0, restore_all
        call preempt_schedule_irq
        j restore_all

user_resched:
        tail schedule

handle_syscall:
        sd a0, 280(sp) //save 
        /*防止SEPC执行原始的值*/
        addi s2, s2, 0x4 //s2在前面已经被保存为epc了
        sd s2, 0(sp) //保存到pt_regs->epc

        li t0, 24        //TODO:__NR_syscalls 
        /*找到对应的函数地址*/
        la s0, sys_call_table
        slli t0, a7, 3
        add s0, s0, t0
        ld s0, 0(s0)

        jalr s0 //跳转到对应的系统函数，并设置ra
ret_from_syscall:
        sd a0, 80(sp) //返回值
        j ret_from_exception


.globl ret_from_kernel_thread
.align 4
ret_from_kernel_thread:
    call schedule_tail
    la ra, ret_from_exception
    mv a0, s1
    jr s0

.globl ret_from_fork
.align 4
ret_from_fork:
        la ra, ret_from_exception
        tail schedule_tail

.globl timervec
.align 4
timervec:
    # start.c has set up the memory that mscratch points to:
    # scratch[0,8,16] : register save area.
    # scratch[24] : address of CLINT's MTIMECMP register.
    # scratch[32] : desired interval between interrupts.
    
    csrrw a0, mscratch, a0
    sd a1, 0(a0)
    sd a2, 8(a0)
    sd a3, 16(a0)

    # schedule the next timer interrupt
    # by adding interval to mtimecmp.
    ld a1, 24(a0) # CLINT_MTIMECMP(hart)
    ld a2, 32(a0) # interval
    ld a3, 0(a1)
    add a3, a3, a2
    sd a3, 0(a1)

    # raise a supervisor software interrupt.
    li a1, 2
    csrw sip, a1

    ld a3, 16(a0)
    ld a2, 8(a0)
    ld a1, 0(a0)
    csrrw a0, mscratch, a0

    mret
