#pragma once
#pragma once
typedef unsigned long long REG;

struct IFID 
{
	//inst means the instruction which is going to execute
	unsigned int inst;
	//PC means the index of the instruction
	int PC;
	// isNop==1 means is a Nop instruction
	int isNop;

}IF_ID, IF_ID_old;


struct IDEX 
{
	int Rd;
	int PC;
	int Imm;
	REG Reg_Rs1,Reg_Rs2;	
	int isNop;
	int havePushedRd;  // ��ʾ��ָ��ID�׶��Ƿ�push��Rd, =1��push����

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
	int Jump_PC;
	int Rd;//intֵ��Ӧ����rdֵ
	REG ALU_out;
	REG Reg_Rs2;//used in writing to memory
	int isNop;
	int havePushedRd;  // ��ʾ��ָ��ID�׶��Ƿ�push��Rd, =1��push����

	char Ctrl_M_Branch;
	char Ctrl_M_Zero;
	char Ctrl_M_MemWrite;
	char Ctrl_M_MemRead;

	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}EX_MEM, EX_MEM_old;

struct EXWB
{
	int PC;//����WB����д���Ĵ���������JALָ��
	REG ALU_out;
	int Rd;
	int isNop;
	int havePushedRd;  // ��ʾ��ָ��ID�׶��Ƿ�push��Rd, =1��push����

	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}EX_WB, EX_WB_old;

struct MEMWB 
{
	int PC;//����WB����д���Ĵ���������JALָ��
	unsigned long long int Mem_read;//the data from the memory
	REG ALU_out;
	int Rd;
	int isNop;
	int havePushedRd;  // ��ʾ��ָ��ID�׶��Ƿ�push��Rd, =1��push����

	char Ctrl_WB_RegWrite;
	char Ctrl_WB_MemtoReg;

}MEM_WB, MEM_WB_old;