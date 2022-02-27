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
    
.section .data
msg:
  .string "Hello, world!\n"