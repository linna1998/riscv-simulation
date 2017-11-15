#pragma once
#pragma once
typedef unsigned long long REG;

struct IFID 
{
	unsigned int inst;
	//inst means the instruction which is going to execute
	int PC;
	//PC means the index of the instruction
}IF_ID, IF_ID_old;


struct IDEX 
{
	int Rd;
	int PC;
	int Imm;
	REG Reg_Rs1,Reg_Rs2;

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
	int PC;
	int Rd;//int值，应该是rd值
	REG ALU_out;
	REG Reg_Rs2;//used in writing to memory

	char Ctrl_M_Branch;
	char Ctrl_M_Zero;
	char Ctrl_M_MemWrite;
	char Ctrl_M_MemRead;

	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}EX_MEM, EX_MEM_old;

struct EXWB
{
	int PC;//用于WB里面写到寄存器，例如JAL指令
	REG ALU_out;
	int Rd;

	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}EX_WB, EX_WB_old;

struct MEMWB 
{
	int PC;//用于WB里面写到寄存器，例如JAL指令
	unsigned long long int Mem_read;//the data from the memory
	REG ALU_out;
	int Rd;

	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}MEM_WB, MEM_WB_old;