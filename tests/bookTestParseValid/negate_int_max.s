  .globl main
main:
  # Allocate stack
  pushq %rbp
  movq %rsp, %rbp
  subq $8, %rsp

  movl $2147483647, -4(%rbp)
  negl -4(%rbp)
  movl -4(%rbp), %eax

  # Reset stack
  movq %rbp, %rsp
  popq %rbp

  ret
.section .note.GNU-stack,"",@progbits
