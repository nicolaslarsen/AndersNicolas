addi $sp, $sp, -12
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
