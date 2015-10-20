#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "mips32.h"
#include "elf.h"


#define ERROR_INVALID_ARGS "You need 2 arguments.\n1: A config file\n2: An executable elf file\n"
#define ERROR_IO_ERROR "The file could not be read\n"
#define ERROR_CLOSE_FILE "The file could not be closed\n"
#define ERROR_FSCANF "Fscanf failed\n"
#define ERROR_READ_CONFIG "Config could not be read\n"
#define ERROR_J "Error occured when trying to jump\n"
#define ERROR_ELF_DUMP "Error occured in elf_dump()\n"
#define ERROR_INTERP "Error occured in interp()\n"
#define ERROR_UNKNOWN_INSTRUCTION -3
#define ERROR_UNKNOWN_OPCODE -2
#define ERROR_UNKNOWN_FUNCT -4
#define ERROR_INTERP_CONTROL -5
#define ERROR_INTERP_ID -6
#define ERROR_INTERP_EX -7
#define ERROR_INTERP_MEM -8
#define SAW_SYSCALL -1

#define AT regs[1]
#define V0 regs[2]
#define V1 regs[3]
#define SP regs[29]
#define RA regs[31]
#define MEMSZ 0xA0000

struct preg_if_id {
  uint32_t inst;
  uint32_t next_pc;
};
static struct preg_if_id if_id;

struct preg_id_ex {
  bool mem_read;
  bool mem_write;
  bool reg_write;
  bool alu_src;
  bool mem_to_reg;
  bool branch;
  bool beq;
  bool jump;

  uint32_t reg_dst;
  uint32_t rt;
  uint32_t rs;
  uint32_t rs_value;
  uint32_t rt_value;
  uint32_t ext_imm;
  uint32_t funct;
  uint32_t shamt;
  uint32_t next_pc;
  uint32_t jump_target;
};
static struct preg_id_ex id_ex;

struct preg_ex_mem {
  bool mem_read;
  bool mem_write;
  bool reg_write;
  bool mem_to_reg;
  bool branch;
  bool beq;
  
  uint32_t reg_dst;
  uint32_t rt;
  uint32_t rt_value;
  uint32_t alu_res;
  uint32_t branch_target;
};
static struct preg_ex_mem ex_mem;

struct preg_mem_wb {
  bool reg_write;
  bool mem_to_reg;
  
  uint32_t reg_dst;
  uint32_t rt;
  uint32_t read_data;
  uint32_t alu_res;
};
static struct preg_mem_wb mem_wb;

static uint32_t PC;
static size_t instr_cnt;
static size_t cycles;
static uint32_t regs[32];
static unsigned char mem[MEMSZ];

/* Prints the amount of instructions executed */
void show_status() {

  printf("Executed %zu instruction(s).\n", instr_cnt);
  printf("%zu cycle(s) elapsed.\n", cycles);
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
}

int interp_control(){
  uint32_t opcode = GET_OPCODE(if_id.inst);
  uint32_t address;
  switch (opcode){
    
    case OPCODE_R :
      id_ex.reg_write     = true;
      id_ex.reg_dst       = GET_RD(if_id.inst);
      id_ex.rs            = GET_RS(if_id.inst);
      id_ex.rt            = GET_RT(if_id.inst);
      id_ex.rs_value      = regs[id_ex.rs];
      id_ex.rt_value      = regs[id_ex.rt];
      id_ex.funct         = GET_FUNCT(if_id.inst);
      id_ex.shamt         = GET_SHAMT(if_id.inst); 
      if (id_ex.funct == FUNCT_JR) {
        
        id_ex.jump        = true;

        if (mem_wb.reg_dst == id_ex.rs && mem_wb.reg_write){
          id_ex.jump_target = mem_wb.alu_res;
        }
        else {
          id_ex.jump_target = id_ex.rs_value;
        }
        
      } 
      break;

    case OPCODE_BEQ :
      id_ex.branch        = true;
      id_ex.beq           = true;
      id_ex.rs            = GET_RS(if_id.inst);
      id_ex.rs_value      = regs[id_ex.rs];
      id_ex.rt_value      = regs[id_ex.rt];
      id_ex.ext_imm       = SIGN_EXTEND(GET_IMM(if_id.inst));  
      id_ex.funct         = FUNCT_SUB;
      break;
    
    case OPCODE_BNE :
      id_ex.branch        = true;
      id_ex.beq           = false; 
      id_ex.rs_value      = regs[GET_RS(if_id.inst)];
      id_ex.rt_value      = regs[id_ex.rt];
      id_ex.ext_imm       = SIGN_EXTEND(GET_IMM(if_id.inst));   
      id_ex.funct         = FUNCT_SUB;
      break;

    case OPCODE_LW :
      id_ex.mem_read      = true;
      id_ex.reg_write     = true;
      id_ex.alu_src       = true;
      id_ex.mem_to_reg    = true;
      id_ex.reg_dst       = GET_RT(if_id.inst);
      id_ex.rt            = GET_RT(if_id.inst);
      id_ex.rs_value      = regs[GET_RS(if_id.inst)];
      id_ex.rt_value      = regs[id_ex.rt];
      id_ex.ext_imm       = SIGN_EXTEND(GET_IMM(if_id.inst));
      id_ex.funct         = FUNCT_ADD;
      break;
  
    case OPCODE_SW :
      id_ex.mem_write     = true;
      id_ex.alu_src       = true;
      id_ex.rt            = GET_RT(if_id.inst);
      id_ex.rs_value      = regs[GET_RS(if_id.inst)];
      id_ex.rt_value      = regs[id_ex.rt];
      id_ex.ext_imm       = SIGN_EXTEND(GET_IMM(if_id.inst));
      id_ex.funct         = FUNCT_ADD;
      break;

    case OPCODE_J :
      id_ex.jump          = true;
      address             = GET_ADDRESS(if_id.inst);
      id_ex.jump_target   = (if_id.next_pc & MS_4B) | (address << 2);
      break;

    case OPCODE_JAL :
      id_ex.reg_write     = true;
      id_ex.jump          = true;
      address             = GET_ADDRESS(if_id.inst);
      id_ex.jump_target   = (if_id.next_pc & MS_4B) | (address << 2);
      id_ex.rs_value      = 0;
      id_ex.rt_value      = if_id.next_pc;
      id_ex.reg_dst       = 31;
      id_ex.funct         = FUNCT_ADD;
      break;

    case OPCODE_ADDI :
      id_ex.reg_write     = true;
      id_ex.alu_src       = true;
      id_ex.rs            = GET_RS(if_id.inst);
      id_ex.rs_value      = regs[id_ex.rs];
      id_ex.reg_dst       = GET_RT(if_id.inst);
      id_ex.ext_imm       = SIGN_EXTEND(GET_IMM(if_id.inst));
      id_ex.funct         = FUNCT_ADD;
      break;      
    
    case OPCODE_ADDIU :
      id_ex.reg_write     = true;
      id_ex.alu_src       = true;
      id_ex.rs            = GET_RS(if_id.inst);
      id_ex.rs_value      = regs[id_ex.rs];
      id_ex.reg_dst       = GET_RT(if_id.inst);
      id_ex.ext_imm       = SIGN_EXTEND(GET_IMM(if_id.inst));
      id_ex.funct         = FUNCT_ADD;
      break;      

    case OPCODE_ANDI :
      id_ex.reg_write     = true;
      id_ex.rs            = GET_RS(if_id.inst);
      id_ex.rs_value      = regs[id_ex.rs];
      id_ex.reg_dst       = GET_RT(if_id.inst);
      id_ex.ext_imm       = ZERO_EXTEND(GET_IMM(if_id.inst));
      id_ex.rt_value      = id_ex.ext_imm; //hack
      id_ex.funct         = FUNCT_AND;
      break;

     case OPCODE_ORI :
      id_ex.reg_write     = true;
      id_ex.rs            = GET_RS(if_id.inst);
      id_ex.rs_value      = regs[id_ex.rs];
      id_ex.reg_dst       = GET_RT(if_id.inst);
      id_ex.ext_imm       = ZERO_EXTEND(GET_IMM(if_id.inst));
      id_ex.rt_value      = id_ex.ext_imm; //hack
      id_ex.funct         = FUNCT_OR;
      break;

    case OPCODE_LUI :
      id_ex.reg_write     = true;
      id_ex.ext_imm       = GET_IMM(if_id.inst) << 16;
      id_ex.rt_value      = id_ex.ext_imm; //hack
      id_ex.reg_dst       = GET_RT(if_id.inst);
      id_ex.funct         = FUNCT_ADD;
      break;

    case OPCODE_SLTI :
      id_ex.reg_write     = true;
      id_ex.ext_imm       = SIGN_EXTEND(GET_IMM(if_id.inst));
      int signed_ext_imm  = id_ex.ext_imm;
      id_ex.rs            = GET_RS(if_id.inst);
      int signed_rs_value = regs[id_ex.rs];
      if (signed_rs_value < signed_ext_imm) {
        id_ex.rs_value = 1;
      }
      else {
        id_ex.rs_value = 0;
      }
      id_ex.rt_value      = 0; //hack
      id_ex.reg_dst       = GET_RT(if_id.inst);
      id_ex.funct         = FUNCT_ADD;
      break;

    case OPCODE_SLTIU :
      id_ex.reg_write     = true;
      id_ex.ext_imm       = SIGN_EXTEND(GET_IMM(if_id.inst));
      id_ex.rs            = GET_RS(if_id.inst);
      id_ex.rs_value      = regs[id_ex.rs];
      if (id_ex.rs_value < id_ex.ext_imm) {
        id_ex.rs_value = 1;
      }
      else {
        id_ex.rs_value = 0;
      }
      id_ex.rt_value      = 0; //hack
      id_ex.reg_dst       = GET_RT(if_id.inst);
      id_ex.funct         = FUNCT_ADD;
      break;
    
    default:
      printf("ERROR: Unknown opcode: 0x%x in interp_control()\n", opcode); 
      return ERROR_UNKNOWN_OPCODE; 
  } 
  return 0;   
}


int interp_id() {

  // Set all values to 0 or false
  id_ex.mem_read    = false;
  id_ex.mem_write   = false;
  id_ex.reg_write   = false;
  id_ex.alu_src     = false;
  id_ex.mem_to_reg  = false;
  id_ex.branch      = false;
  id_ex.jump        = false;

  id_ex.rs            = 0;
  id_ex.rt            = 0;
  id_ex.rs_value      = 0;
  id_ex.rt_value      = 0;
  id_ex.ext_imm       = 0;
  id_ex.funct         = 0;
  id_ex.shamt         = 0;
  id_ex.jump_target   = 0;
  id_ex.next_pc       = if_id.next_pc; 
  int retControl = interp_control();
  if (retControl != 0){
    printf("ERROR: interp_control() failed\n");
    return ERROR_INTERP_CONTROL;
  }
  return 0;
}


int alu() {
  // INSTRUKTOR 0: instead of the if-else case, then simply
  // add a variable and use this as second operand in all operations.
  // Then you can make a simple test here, to see if
  // it should hold the imm. value or the rt_value.
  if (id_ex.alu_src) {
    switch (id_ex.funct) {
      case (FUNCT_ADD):
        ex_mem.alu_res = id_ex.rs_value + id_ex.ext_imm;
        break;
 
      default:
        printf("ERROR: Unknown funct: 0x%x in alu()\n", id_ex.funct);
        return ERROR_UNKNOWN_FUNCT;
    }
  }
  else {
    switch (id_ex.funct) {
      case (FUNCT_ADD):
      case (FUNCT_ADDU):
        ex_mem.alu_res = id_ex.rs_value + id_ex.rt_value;
        break;

      case (FUNCT_AND):
        ex_mem.alu_res = id_ex.rs_value & id_ex.rt_value;
        break;

      case (FUNCT_NOR):
        ex_mem.alu_res = ~(id_ex.rs_value | id_ex.rt_value);
        break;

      case (FUNCT_OR):
        ex_mem.alu_res = id_ex.rs_value | id_ex.rt_value;
        break;

      case (FUNCT_SLL):
        ex_mem.alu_res = id_ex.rt_value << id_ex.shamt;
        break;
    
      case (FUNCT_SLT):
        ; // Empty statement
        int signed_rs_value = id_ex.rs_value;
        int signed_rt_value = id_ex.rt_value;
        if (signed_rs_value < signed_rt_value) {
          ex_mem.alu_res = 1;
        }
        else {
          ex_mem.alu_res = 0;
        }
        break;
 
      case (FUNCT_SLTU):
        if (id_ex.rs_value < id_ex.rt_value) {
          ex_mem.alu_res = 1;
        }
        else {
          ex_mem.alu_res = 0;
        }
        break;

     case (FUNCT_SRL):
        ex_mem.alu_res = id_ex.rt_value >> id_ex.shamt;
        break;

      case (FUNCT_SUB):
        ex_mem.alu_res = id_ex.rs_value - id_ex.rt_value;
        break;

      case (FUNCT_SUBU):
        ex_mem.alu_res = id_ex.rs_value - id_ex.rt_value;
        break;
  
      case (FUNCT_JR):
        // Nothing
        break;
      
      case (FUNCT_SYSCALL):
        return SAW_SYSCALL;
      
      default:
        printf("ERROR: Unknown funct: 0x%x in alu()\n", id_ex.funct);
        return ERROR_UNKNOWN_FUNCT;
    }
  } 
  return 0;
}

int interp_ex(){
  ex_mem.mem_read       = id_ex.mem_read;
  ex_mem.mem_write      = id_ex.mem_write;
  ex_mem.reg_write      = id_ex.reg_write;
  ex_mem.branch         = id_ex.branch;
  ex_mem.beq            = id_ex.beq;
  ex_mem.rt             = id_ex.rt; 
  ex_mem.rt_value       = id_ex.rt_value;
  ex_mem.mem_to_reg     = id_ex.mem_to_reg;
  ex_mem.reg_dst        = id_ex.reg_dst;
  ex_mem.branch_target  = id_ex.next_pc + (id_ex.ext_imm << 2); 

  int retAlu = alu();
  
  if (retAlu != SAW_SYSCALL && retAlu != 0) {
    printf("ERROR: alu() failed\n");
  }
  
  return retAlu;
}

void interp_mem(){
  mem_wb.reg_write  = ex_mem.reg_write;  
  mem_wb.rt         = ex_mem.rt;
  mem_wb.alu_res    = ex_mem.alu_res;
  mem_wb.mem_to_reg = ex_mem.mem_to_reg;
  mem_wb.reg_dst    = ex_mem.reg_dst;
  
  if (ex_mem.mem_read) {
    mem_wb.read_data = GET_BIGWORD(mem, ex_mem.alu_res);
  }
  if (ex_mem.mem_write) {
    SET_BIGWORD(mem, ex_mem.alu_res, ex_mem.rt_value);
  }
}

void interp_wb(){
  if (mem_wb.reg_dst == 0 || !mem_wb.reg_write){
    // Do nothing
    return;
  }
  
  if (mem_wb.mem_to_reg) {
    regs[mem_wb.reg_dst] = mem_wb.read_data;
  } 
  else {
    regs[mem_wb.reg_dst] = mem_wb.alu_res;
  }
}

void interp_if(){
  if_id.inst = GET_BIGWORD(mem, PC);
  uint32_t if_id_rs = GET_RS(if_id.inst);
  uint32_t if_id_rt = GET_RT(if_id.inst);
  if (id_ex.mem_read && ((id_ex.rt = if_id_rs) || (id_ex.rt = if_id_rt))) {
    return; // Stall the pipeline
  }
  PC = PC + 4;
  if_id.next_pc = PC;
  instr_cnt++;
}

// Stub function
int forward(){

bool ex_hazard_rs   = ex_mem.reg_write && ex_mem.reg_dst != 0 && ex_mem.reg_dst == id_ex.rs;
bool ex_hazard_rt   = ex_mem.reg_write && ex_mem.reg_dst != 0 && ex_mem.reg_dst == id_ex.rt; 
bool mem_hazard_rs  = mem_wb.reg_write && mem_wb.reg_dst != 0 && 
                      !ex_hazard_rs && mem_wb.reg_dst == id_ex.rs;
bool mem_hazard_rt  = mem_wb.reg_write && mem_wb.reg_dst != 0 && 
                      !(ex_hazard_rt)  && mem_wb.reg_dst == id_ex.rt;
  
  if (mem_hazard_rs){
    
    if(mem_wb.mem_to_reg){
      id_ex.rs_value = mem_wb.read_data; 
    }
    
    else{
      id_ex.rs_value = mem_wb.alu_res;
    }
  }

  if (mem_hazard_rt){

    if(mem_wb.mem_to_reg){
      id_ex.rt_value = mem_wb.read_data;
    }
    
    else{
      id_ex.rt_value = mem_wb.alu_res;
    }
  }

  if (ex_hazard_rs) {
    id_ex.rs_value = ex_mem.alu_res; 
  }

  if (ex_hazard_rt) {
    id_ex.rt_value = ex_mem.alu_res;
  }
  return 0;
}
// Stub function
int cycle(){
  int retval = 0;
  interp_wb();
  interp_mem();
  int retEx = interp_ex();
  
  if (retEx == SAW_SYSCALL){
    retval = SAW_SYSCALL;
  }
  else if (retEx != 0) {
    printf("ERROR: interp_ex() failed\n");
    return ERROR_INTERP_EX;
  }
  int retId = interp_id();
  if (retId != 0) {
    printf("ERROR: interp_id() failed\n");
    return ERROR_INTERP_ID;
  }
  interp_if();
  if (ex_mem.branch) {
    if ((ex_mem.beq && ex_mem.alu_res == 0) || (!ex_mem.beq && ex_mem.alu_res != 0)){ 
      PC = ex_mem.branch_target;
      if_id.inst = 0;
      instr_cnt--;
    } 
  }
  if (id_ex.jump) {
    PC = id_ex.jump_target;
  }
  forward();
    
  return retval;
}

// Runs an infinite loop and increments the PC
// after dealing with an instruction.
// The loop only ends on a syscall or an error
int interp(){
  int retCycle = 0;
  int stop = 0;

  while(!stop){
  
    cycles++;
    retCycle = cycle();
    if (retCycle != 0)
      stop = 1;
  }
  return retCycle;
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
  }
  else {
    printf(ERROR_INVALID_ARGS);
    return -1;
  }

  if (read == 0) {
    int retElfDump = elf_dump(argv[2], &PC, &mem[0], MEMSZ);
    
    if(retElfDump != 0) {
      printf(ERROR_ELF_DUMP);
      return -2;
    }
    
    SP = MIPS_RESERVE + MEMSZ;
    
    int retInterp = interp();
    if (retInterp != SAW_SYSCALL && retInterp != 0) {
      printf(ERROR_INTERP);
      return -3;
    }  
    show_status();
  }
  else {
    printf(ERROR_READ_CONFIG);
    return -4;
  }
  return 0;
}
