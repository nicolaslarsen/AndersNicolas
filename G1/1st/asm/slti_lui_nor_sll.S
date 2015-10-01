.globl _start
_start:
.set noreorder

slti $t0, $t1, 1
lui $t0, 1023
addiu $t1, $t1, 31
sll $t1, $t1, 4
nor $v0, $t1, $t0
syscall
