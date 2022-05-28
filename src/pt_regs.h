#ifndef __PT_REGS_H
#define __PT_REGS_H

struct pt_regs {
	unsigned long epc;  //0
	unsigned long ra;   //8
	unsigned long sp;   //16
	unsigned long gp;   //24
	unsigned long tp;   //32
	unsigned long t0;   //40
	unsigned long t1;   //48
	unsigned long t2;   //56
	unsigned long s0;	//64
	unsigned long s1;	//72
	unsigned long a0;	//80
	unsigned long a1;	//88
	unsigned long a2;	//96
	unsigned long a3;	//104
	unsigned long a4;	//112
	unsigned long a5;	//120
	unsigned long a6;	//128
	unsigned long a7;	//136
	unsigned long s2;	//144
	unsigned long s3;	//152
	unsigned long s4;	//160
	unsigned long s5;	//168
	unsigned long s6;	//176
	unsigned long s7;	//184
	unsigned long s8;	//192
	unsigned long s9;	//200
	unsigned long s10;	//208
	unsigned long s11;	//216
	unsigned long t3;	//224
	unsigned long t4;	//232
	unsigned long t5;	//240
	unsigned long t6;	//248
	/* Supervisor/Machine CSRs */
	unsigned long status;	//256
	unsigned long badaddr;	//264
	unsigned long cause;	//272
	/* a0 value before the syscall */
	unsigned long orig_a0;	//280
};
#endif
