#include <stdio.h>
#include <stdint.h>
#include "mips32.h"
#include "elf.h"

#define ERROR_INVALID_ARGS "You need 2 arguments.\n1: A config file\n2: An executable elf file\n"
#define ERROR_IO_ERROR "The file could not be read\n"
#define ERROR_CLOSE_FILE "The file could not be closed\n"
#define ERROR_FSCANF "Fscanf failed\n"
#define ERROR_READ_CONFIG "Config could not be read\n"
#define ERROR_J "Error occured when trying to jump\n"
#define ERROR_UNKNOWN_INSTRUCTION -3
#define ERROR_UNKNOWN_OPCODE -2


#define AT regs[1]
#define V0 regs[2]
#define V1 regs[3]
#define SP regs[29]
#define RA regs[31]
/*INSTRUKTOR 0: MEMSZ is perfectly right, but you could have used the hex value 0xA0000 instead */
#define MEMSZ 0xA0000

static uint32_t PC;
static size_t instr_cnt;
static uint32_t regs[32];
static unsigned char mem[MEMSZ];

/* Prints the amount of instructions executed */
void show_status() {

  printf("Executed %zu instruction(s).\n", instr_cnt);
  printf("pc = 0x%x\n", PC);
  printf("at = 0x%x\n", AT);
  printf("v0 = 0x%x\n", V0);
  printf("v1 = 0x%x\n", V1);
  printf("t0 = 0x%x\n", regs[8]);
  printf("t1 = 0x%x\n", regs[9]);
  printf("t2 = 0x%x\n", regs[10]);
  printf("t3 = 0x%x\n", regs[11]);
  printf("t4 = 0x%x\n", regs[12]);
  printf("t5 = 0x%x\n", regs[13]);
  printf("t6 = 0x%x\n", regs[14]);
  printf("t7 = 0x%x\n", regs[15]);
  printf("sp = 0x%x\n", SP);
  printf("ra = 0x%x\n", RA);

}


/* Inserts config data into array */
int read_config_stream(FILE *stream) {

  int i;

    for (i = 8; i < 16; i++){
      uint32_t v;
    /*  INSTRUKTOR  -1:the return value of fscanf is the number of input items
        successfully matched and assigned. Use this to make sure that fscanf return the right
        ourput, and return the function with an error-code, if you do not get
        the right number of outputs. */
      int fScanF = fscanf(stream, "%u", &v);
      if (fScanF == 1) {
        regs[i] = v;
      }
      else {
        printf(ERROR_FSCANF);
        return -1;
      }
  }
  return 0;
}


/* Opens and closes a file */
int read_config(const char *path) {

  FILE *stream = fopen(path, "r");
  int read = -1;
  if (stream != NULL){

  /* INSTRUKTOR -2:  this function have a return value. Use this to test wether or not
      any error occured during the reading of the config */
    read = read_config_stream(stream);
  }
  else{
    printf(ERROR_IO_ERROR);
    return -2;
  }
  if (read != 0) {
    printf(ERROR_READ_CONFIG);
  }
  int f_close = fclose(stream);
  if (f_close != 0) {
    printf(ERROR_CLOSE_FILE);
    return -3;
  }
  return read;

      /* INSTRUKTOR -2: This part of the code is never run
     * this is because you return in both the if-clause and the else-clause.
     * you should not return 0, before you know that everything went well.
     */

  /* INSTRUKTOR 1: Use this style of coding for all you sanity checks  */
  /* INSTRUCTOR If something went wrong, you want to let the caller know,
   * And therby making it possible to halt the program without other instructions get run. */
}


/* Executes the R instructions */
int interp_r (uint32_t inst){

  uint32_t funct = GET_FUNCT(inst);
  uint32_t rs, rt, rd, shamt;
  rs    = GET_RS(inst);
  rt    = GET_RT(inst);
  rd    = GET_RD(inst);
  shamt = GET_SHAMT(inst);

  switch(funct)
  {
    case FUNCT_JR :
      /* INSTRUKTOR -2: Because you are incrementing PC
       * in interp(), then you will be off-by-one/four */

       /* We get the instruction before incrementing PC and should
        * not be off-by-anything. However, when the program has
        * terminated, the PC is still set as if there were more
        * instructions
        */
      PC = regs[rs];
      break;

    case FUNCT_SYSCALL :
    /* INSTRUKTOR 0: no code after a return-statement will be run,
     * if you return here, you dont need a break-statement.
     * It would have been nicer for you to define the syscall-code, so you dont have any
     * 'magic numbers' */
      return -1;

    case FUNCT_ADDU :
      regs[rd] = regs[rs] + regs[rt];
      break;

    case FUNCT_SUBU :
      regs[rd] = regs[rs] - regs[rt];
      break;

    case FUNCT_AND :
      regs[rd] = regs[rs] & regs[rt];
      break;

    case FUNCT_OR :
      regs[rd] = regs[rs] | regs[rt];
      break;

    case FUNCT_NOR :
      regs[rd] = ~(regs[rs] | regs[rt]);
      break;

    case FUNCT_SLT :
      if (regs[rs] < regs[rt])
        regs[rd] = 1;
      else 
        regs[rd] = 0;
      break;

    case FUNCT_SLL :
      regs[rd] = regs[rt] << shamt;
      break;

    case FUNCT_SRL :
      regs[rd] = regs[rt] >> shamt;
      break;

    default:
      return ERROR_UNKNOWN_INSTRUCTION;
  /* INSTRUKTOR -1: You need a default case, look at interp for inspiration */
  }
  
  return 0;
}

// Jumps the PC to an address contained in the instruction
int j(uint32_t inst){

  // The top 4 bits of the PC 

  /* INSTRUKTOR -2: By shifting left, you are removing the 26 most significant bits
   * You want to keep the 4 most significant bits instead.
   * recall that MS_4B gives you a mask looking like 1111000[...]0. 
   * what happens when you use the or-operator on something that have 1's?
   * think of how you can use the and-operator to fix this instead.
   * Think of the difference between
   * {(PC+4[31:28], address, 2'b0)} (the right one) and
   * {(PC[31:28], address, 2'b0) + 4} (almost your version)
   * this might cause you to change your interp()
   */
  uint32_t upperPC  = PC & MS_4B;
  
  // The address provided in the MIPS j instruction
  uint32_t address  = GET_ADDRESS(inst);

  // The top 4 bits of the pc concatenated with the address,
  // concatenated with 00, thus creating the Pseudo-direct address
  uint32_t PD_address = upperPC | (address << 2);
  printf("PD_ADDRESS: %x\n",PD_address); 
  PC = PD_address;

  return 0;
}

// Jumps the pc to an address contained in the instruction,
// and sets the $ra to the next instruction
int jal(uint32_t inst){
  // Sets the return address to the following instruction.
  // PC is incremented immediately after reading the instruction in interp()
  // and thus we do not need to increment it now to get the next instruction
  RA = PC;

  j(inst);

  return 0;
}

// Identifies the instruction by looking at the opcode,
// then performs the instruction
int interp_inst (uint32_t inst){

  uint32_t opcode = GET_OPCODE(inst);
  uint32_t rs, rt, imm;
  int retval = 0;
  switch(opcode)
  {
    case OPCODE_R :
      retval = interp_r(inst);
      break;
    
    case OPCODE_J :
      ; // empty statement hack
      /* INSTRUKTOR -1: use the return value */
      int jump = j(inst);
      if (jump != 0) {
        printf(ERROR_J);
      }
      break;
    
    case OPCODE_JAL :
      jal(inst);
      break;
     
    case OPCODE_BEQ :
      rs  = GET_RS(inst);
      rt  = GET_RT(inst);
      imm = SIGN_EXTEND(GET_IMM(inst));
      if (regs[rs] == regs[rt]) {
        /* INSTRUKTOR -2: remember to signextend */
        PC = PC + 4* imm;
      }
      break;

    case OPCODE_BNE :
      rs  = GET_RS(inst);
      rt  = GET_RT(inst);
      imm = SIGN_EXTEND(GET_IMM(inst));
      if (regs[rs] != regs[rt]) {
        PC = PC + 4*imm;
      }
      break;

    case OPCODE_ADDIU :
      rs  = GET_RS(inst);
      rt  = GET_RT(inst);
      // gets imm and sign-extends
      imm = SIGN_EXTEND(GET_IMM(inst));
      regs[rt] = regs[rs] + imm; 
      break;

    case OPCODE_SLTI :
      rs  = GET_RS(inst);
      rt  = GET_RT(inst);
      // gets imm and sign-extends
      imm = SIGN_EXTEND(GET_IMM(inst));
      if (regs[rs] < imm) {
        regs[rt] = 1;
      }
      else {
        regs[rt] = 0;
      }
      break;

    case OPCODE_ANDI :
      rs  = GET_RS(inst);
      rt  = GET_RT(inst);
      // gets imm and zero-extends
      imm = ZERO_EXTEND(GET_IMM(inst));
      regs[rt] = regs[rs] & imm;
      break;

    case OPCODE_ORI :
      rs  = GET_RS(inst);
      rt  = GET_RT(inst);
      // gets imm and zero-extends
      imm = ZERO_EXTEND(GET_IMM(inst));
      regs[rt] = regs[rs] | imm;
      break;
      
    case OPCODE_LUI :
      rt  = GET_RT(inst);
      // gets imm and left-shifts by 16 bits
      imm = GET_IMM(inst) << 16;
      regs[rt] = imm;
      break;

    case OPCODE_LW :
      rs  = GET_RS(inst);
      rt  = GET_RT(inst);
      // gets imm and sign-extends
      imm     = SIGN_EXTEND(GET_IMM(inst));
      uint32_t loadAddress = regs[rs] + imm;
      regs[rt] = GET_BIGWORD(mem, loadAddress);
      break;

    case OPCODE_SW :
      rs  = GET_RS(inst);
      rt  = GET_RT(inst);
      // gets imm and sign-extends
      imm = SIGN_EXTEND(GET_IMM(inst));
      uint32_t storeAddress  = regs[rs] + imm; 
      SET_BIGWORD(mem, storeAddress, regs[rt]);
      break;

    default :
      retval = ERROR_UNKNOWN_OPCODE; 
  }
  return retval;
}

// Runs an infinite loop and increments the PC
// after dealing with an instruction.
// The loop only ends on a syscall
int interp(){

  int stop = 0;

  while(!stop){
  
    // Gets the MIPS instruction from memory
    uint32_t inst = GET_BIGWORD(mem, PC);
    // Increments the program counter
    PC += 4;
    // Increments the instruction count
    instr_cnt++;
    int retval = interp_inst(inst);
    if (retval == -1)
      stop = 1;

    if (retval == ERROR_UNKNOWN_OPCODE) {
      stop = 1;
      printf("ERROR: UNKNOWN OPCODE\n");  
    }
    if (retval == ERROR_UNKNOWN_INSTRUCTION) {
      stop = 1;
      printf("ERROR: UNKNOWN INSTRUCTION\n");
    }

  }
  return 0;
}

// Gets the cfg from the command line, reads it, updates the registers,
// reads the MIPS instructions from the elf file and inserts them into
// memory. Lastly, main initializes the stackpointer,
// interprets the instructions and shows the status of the registers
// after completing them.
int main(int argc, char *argv[]) {
int read;
  if (argc == 3) {
    read = read_config(argv[1]);
    // ^^^^^^^^^^^^^^^^^
    // INSTRUKTOR -: Remember to _always_ check the return value of any function to
    // check that no error occurred.  (that is why you have a return value in c)
  }
  else {
    /* INSTRUKTOR 0:  Your message should help the user, letting them know
     * What kind of first- and second argument should be given to this program */
    printf(ERROR_INVALID_ARGS);
    return -1;
  }
  /* INSTRUKTOR -1: this part of the code should only be run, it the right
   * arguments are given. */
  if (read == 0) {
    elf_dump(argv[2], &PC, &mem[0], MEMSZ);

  /* INSTRUKTOR 0: change the initial value of sp as explained in the 
   * feedback */
    SP = MIPS_RESERVE + MEMSZ;
    interp();  
    show_status();
  }
  else {
    printf(ERROR_READ_CONFIG);
    return -2;
  }
  return 0;
}
