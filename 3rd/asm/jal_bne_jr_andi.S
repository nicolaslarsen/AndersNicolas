.globl _start
_start:
.set noreorder

addiu $t5, $t5, 1
addiu $t3, $t3, 31
bne $v0, $t5, lel

test: 
  andi $t2, $t3, 15 
  jr $ra
lel:
  addu $v0, $t5, $zero
  jal test
  syscall
