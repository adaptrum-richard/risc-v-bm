.section .text
#
# 学习la和lb指令用法：
# 函数char load_isa_la_lb(int index)
# 函数含义：
#   la加载msg的地址到s2寄存器,参数index在汇编中用a0表示
#   然后使用lb指令读取msg字符串的第index个字节，返回值存放在a0中。
# 总结：
#    la 地址加载指令，伪指令，用法 la rd, symbol, 将symbol加载到x[rd]寄存器中
#    lb 字节加载指令，lb rd, offset(rs1)，从地址x[rs1]+sign-extend(offset) 读取一个字节
.globl load_isa_la_lb
load_isa_la_lb:
    la s2, msg
    add s3, s2, a0
    lb a0, 0(s3)
    ret

#
# 函数unsigned long load_isa_ld(unsigned long *array, int index)
# 函数含义：
#   返回array[index]的值。
#   array的值存放在寄存器a0中，index的值存放在寄存器a1中
#   slli 逻辑左移指令，将index的值左移3位，相当于乘以8
# 总结：
#   1. 在汇编中，index只是一个数值，不会智能的将其转换为array[index]，
#   必须转化为array+8*index 才能得到array[index]的值
#   2. ld指令为双字加载指令，格式 ld rd, offset(rs1) 
#        从地址x[rs1]+sign-extend(offset)读取8个字节，写入x[rd]

.globl load_isa_ld
load_isa_ld:
    slli a1, a1, 3 # a1*8
    add s1, a0, a1
    ld a0, 0(s1)
    ret
#
# 函数unsigned int load_isa_lh(unsigned int *array, int index)
#  load_isa_lh和load_isa_lhu函数是为了对比lh和lhu指令
#   lh:  lh  rd offset(rs1)  半字加载指令，从地址x[rs1]+sign-extend(offset)读取2个字节，经过符号位扩展后写入x[rd]
#   lhu: lhu rd offset(rs1)  无符号半字加载指令，从地址x[rs1]+sign-extend(offset)读取2个字节，经过符号位扩展后写入x[rd]
.globl load_isa_lh
load_isa_lh:
    slli a1, a1, 2 # a1*4
    add s1, a0, a1
    lh a0, 0(s1)
    ret

#
# 函数unsigned int load_isa_lhu(unsigned int *array, int index)
#
.globl load_isa_lhu
load_isa_lhu:
    slli a1, a1, 2 # a1*4
    add s1, a0, a1
    lhu a0, 0(s1)
    ret

#
#  unsigned long store_isa_sd(unsigned long *addr, unsigned long v)
#  函数含义:
#       将v存储到地址addr中，然后在读取addr内存中数据的值，返回
#
.globl store_isa_sd
store_isa_sd:
    sd a1, 0(a0)
    ld a0, 0(a0)
    ret

#  unsigned int store_isa_sd(unsigned int *addr, unsigned int v)
.globl store_isa_sw
store_isa_sw:
    sw a1, 0(a0)
    lw a0, 0(a0)
    ret

#  unsigned short store_isa_sd(unsigned short *addr, unsigned short v)
.globl store_isa_sh
store_isa_sh:
    sh a1, 0(a0)
    lh a0, 0(a0)    
    ret

#  unsigned char store_isa_sd(unsigned short *char, unsigned char v)
.globl store_isa_sb
store_isa_sb:
    sb a1, 0(a0)
    lb a0, 0(a0)   
    ret

.section .data
msg:
  .string "Hello, world!\n"