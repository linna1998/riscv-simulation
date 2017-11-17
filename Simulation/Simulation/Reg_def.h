#pragma once
#pragma once
typedef unsigned long long REG;

struct IFID 
{
	// inst means the instruction which is going to execute.
	unsigned int inst;
	// PC means the index of the instruction.
	int PC;
	// isNop = 1 means is a Nop instruction.
	int isNop;

}IF_ID, IF_ID_old;

struct IDEX 
{
	int Rd;
	int PC;  // The PC of this instruction.
	int Imm;
	REG Reg_Rs1,Reg_Rs2;	
	int isNop;
	// Whether this instruction has pushe Rd into RdQueue or not.
	// havePushedRd = 1 means have pushed.
	int havePushedRd;  
	char Ctrl_EX_ALUSrcA;
	char Ctrl_EX_ALUSrcB;
	char Ctrl_EX_ALUOp;	

	char Ctrl_M_Branch;
	char Ctrl_M_MemWrite;
	char Ctrl_M_MemRead;

	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}ID_EX, ID_EX_old;

struct EXMEM 
{
	int PC;  // The PC of this instruction.
	int Jump_PC;
	int Rd;
	REG ALU_out;
	REG Reg_Rs2;  // In S type, write it into memory.
	int isNop;
	int havePushedRd;  

	char Ctrl_M_Branch;
	char Ctrl_M_Zero;
	char Ctrl_M_MemWrite;
	char Ctrl_M_MemRead;

	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}EX_MEM, EX_MEM_old;

struct EXWB
{
	int PC;  // The PC of this instruction.
	int Jump_PC;
	REG ALU_out;
	int Rd;
	int isNop;
	int havePushedRd;  

	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}EX_WB, EX_WB_old;

struct MEMWB 
{
	int PC;  // The PC of this instruction.
	unsigned long long int Mem_read;  // The data from the memory.
	REG ALU_out;
	int Rd;
	int isNop;
	int havePushedRd;  

	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}MEM_WB, MEM_WB_old;