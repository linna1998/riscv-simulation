#pragma once
#pragma once
typedef unsigned long long REG;

struct IFID {
	unsigned int inst;
	//inst means the instruction which is going to execute
	int PC;
	//PC means the index of the instruction
}IF_ID, IF_ID_old;


struct IDEX {
	int Rd, Rs1, Rs2;
	int PC;
	int Imm;
	REG Reg_Rd, Reg_Rs1,Reg_Rs2;

	char Ctrl_EX_ALUSrc;
	char Ctrl_EX_ALUOp;
	char Ctrl_EX_RegDst;

	char Ctrl_M_Branch;
	char Ctrl_M_MemWrite;
	char Ctrl_M_MemRead;

	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}ID_EX, ID_EX_old;

struct EXMEM {
	int PC;
	int Reg_dst;
	REG ALU_out;
	int Zero;
	REG Reg_Rt;

	char Ctrl_M_Branch;
	char Ctrl_M_MemWrite;
	char Ctrl_M_MemRead;

	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}EX_MEM, EX_MEM_old;

struct MEMWB {
	unsigned int Mem_read;
	REG ALU_out;
	int Reg_dst;

	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}MEM_WB, MEM_WB_old;