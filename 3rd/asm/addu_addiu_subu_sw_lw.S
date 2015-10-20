addiu $a0, $a0, 2
addiu $a1, $a1, 4
addiu $a2, $a2, 1
addiu $a3, $a3, 3
addiu $sp, $sp, -12
addiu $t1, $t1, 42
addiu $t0, $t0, 43
addiu $s0, $s0, 44
sw $t1, 8($sp)
sw $t0, 4($sp)
sw $s0, 0($sp)
addu $t0, $a0, $a1
addu $t1, $a2, $a3
subu $s0, $t0, $t1
addu $v0, $s0, $zero
lw $s0, 0($sp)
lw $t0, 4($sp)
lw $t1, 8($sp)
addu $v1, $v1, $s0
addiu $sp, $sp, 12
nop
syscall
nop
nop
