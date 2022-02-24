.section .text.boot

.globl _start
_start:
    li s1, 0x10000000 # s1 := 0x1000_0000
    la s2, message    # s2 := <message>
    addi s3, s2, 14   # s3 := s2 + 14
1:
    lb s4, 0(s2)      # s4 := (s2)
    sb s4, 0(s1)      # (s1) := s4
    addi s2, s2, 1    # s2 := s2 + 1
    blt s2, s3, 1b    # if s2 < s3, branch back to 1

    la sp, kernel_sp
    call start

.section .data
message:
  .string "Hello, world!\n"