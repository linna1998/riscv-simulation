#include "Simulation.h"
using namespace std;

extern void read_elf();
extern unsigned int cadr;
extern unsigned int csize;
extern unsigned int vadr;
extern unsigned long long gp;
extern unsigned int madr;
extern unsigned int endPC;
extern unsigned int entry;
extern FILE *file;


//指令运行数
long long inst_num = 0;

//系统调用退出指示
int exit_flag = 0;

//加载代码段
//初始化PC
void load_memory()
{
	fseek(file, cadr, SEEK_SET);
	fread(&memory[vadr >> 2], 1, csize, file);

	vadr = vadr >> 2;
	csize = csize >> 2;
	fclose(file);
}

int main()
{
	//解析elf文件
	read_elf();

	//加载内存
	load_memory();

	//设置入口地址
	PC = entry >> 2;

	//设置全局数据段地址寄存器
	reg[3] = gp;

	reg[2] = MAX / 2;//栈基址 （sp寄存器）

	simulate();

	cout << "simulate over!" << endl;

	return 0;
}

void simulate()
{
	//结束PC的设置
	int end = (int)endPC / 4 - 1;
	while (PC != end)
	{
		//运行
		IF();
		ID();
		EX();
		MEM();
		WB();

		//更新中间寄存器
		IF_ID = IF_ID_old;
		ID_EX = ID_EX_old;
		EX_MEM = EX_MEM_old;
		MEM_WB = MEM_WB_old;

		if (exit_flag == 1)
			break;

		reg[0] = 0;//一直为零

	}
}


//取指令
void IF()
{
	//write IF_ID_old
	IF_ID_old.inst = memory[PC];
	PC = PC + 1;//??? why not add 4
	IF_ID_old.PC = PC;
}

//译码
void ID()
{
	//Read IF_ID
	unsigned int inst = IF_ID.inst;
	//EXTop=0: Zero extend
	//EXTop=1: sign extend
	int EXTop = 0;
	//the number needed to be extended
	unsigned int EXTsrc = 0;
	//RegDst=0: write into rd
	//RegDst=1: write into PC
	//RegDst=2: don't write to register
	char RegDst;
	//ALUop=0: add
	//ALUop=1: mul
	//ALUop=2: sub	
	//ALUop=3: sll, shift left
	//ALUop=4: get the high 32 bits of multiple result
	//ALUop=5: slt, set less than
	//ALUop=6: xor
	//ALUop=7: div
	//ALUop=8: srl, shift right logical, fill with 0
	//ALUop=9: sra: shift right arithmetic, fill with 1
	//ALUop=10: or
	//ALUop=11: rem, R[rd] ← R[rs1] % R[rs2]
	//ALUop=12: and. &
	char ALUop;
	//ALUSrc=0: B from register rs2
	//ALUSrc=1: B from Extend part 
	char ALUSrc;

	//Branch=0: new_PC=PC+4
	//Branch=1: new_PC=a new PC, choose to branch;
	//Branch=2: branch if R[rs1]==R[rs2]
	//Branch=3: branch if R[rs1]!=R[rs2]
	//Branch=4: branch if R[rs1]<R[rs2]
	//Branch=5: branch if R[rs1]>=R[rs2]
	char Branch;
	//MemRead=0: don't read from memory
	//MemRead=1: read from memory, byte
	//MemRead=2: read from memory, half word
	//MemRead=3: read from memory, word
	//MemRead=4: read from memory, double word
	char MemRead;
	//MemWrite=0: don't write to memory
	//MemWrite=1: write to memory, byte, [7:0]
	//MemWrite=2: write to memory, half word[15:0]
	//MemWrite=3: write to memory, word[31:0]
	//MemWrite=4: write to memory, double word[63:0]
	char MemWrite;

	//RegWrite=0: don't write to register
	//RegWrite=1: write to register
	char RegWrite;
	//MemtoReg=0: send ALU's result to register
	//MemtoReg=1: send Memory's result to register
	//MemtoReg=2: send PC+4 to register
	char MemtoReg;

	OP = getbit(inst, 0, 6);
	fuc3 = getbit(inst, 12, 14);
	fuc7 = getbit(inst, 25, 31);
	//shamt: same direction with rs2
	shamt = getbit(inst, 20, 24);
	rs1 = getbit(inst, 15, 19);
	rs2 = getbit(inst, 20, 24);
	rd = getbit(inst, 7, 11);

	//immediate numbers are in different kinds and be put in different areas
	imm_I = getbit(inst, 20, 31);//I
	imm_S = getbit(inst, 7, 11) + getbit(inst, 25, 31)*(1 << 5);//S
	imm_SB = (1 << 1)*(getbit(inst, 8, 11) + getbit(inst, 25, 30)*(1 << 5) + getbit(inst, 7, 7)*(1 << 11) + getbit(inst, 31, 31)*(1 << 12));//SB
	imm_U = getbit(inst, 12, 31);//U
	imm_UJ = (1 << 1)*(getbit(inst, 21, 30) + getbit(inst, 20, 20)*(1 << 11) + getbit(inst, 12, 19)*(1 << 12) + getbit(inst, 31, 31)*(1 << 20));//UJ

	//R type
	if (OP == OP_R)
	{
		//same control parts
		EXTop = 0;
		RegDst = 0;
		ALUSrc = 0;
		Branch = 0;
		MemRead = 0;
		MemWrite = 0;
		RegWrite = 1;
		MemtoReg = 0;
		//different control part: ALUop
		//add
		if (fuc3 == F3_ADD&&fuc7 == F7_ADD)
		{
			ALUop = 0;
		}
		//mul
		else if (fuc3 == F3_ADD && fuc7 == F7_MUL)
		{
			ALUop = 1;
		}
		//sub
		else if (fuc3 == F3_ADD&& fuc7 == F7_SUB)
		{
			ALUop = 2;
		}
		//sll
		else if (fuc3 == F3_SLL&& fuc7 == F7_SLL)
		{
			ALUop = 3;
		}
		//mulh
		else if (fuc3 == F3_SLL&& fuc7 == F7_MULH)
		{
			//get the high 32 bits of multiple result
			ALUop = 4;
		}
		//slt
		else if (fuc3 == F3_SLT&& fuc7 == F7_SLT)
		{
			//set less than op
			ALUop = 5;
		}
		else if (fuc3 == F3_XOR&& fuc7 == F7_XOR)
		{
			ALUop = 6;//xor ^
		}
		else if (fuc3 == F3_XOR&& fuc7 == F7_DIV)
		{
			ALUop = 7;//div, /
		}
		else if (fuc3 == F3_SRL&& fuc7 == F7_SRL)
		{
			ALUop = 8;//shift right logically
		}
		else if (fuc3 == F3_SRL&& fuc7 == F7_SRA)
		{
			ALUop = 9;//shift right arithmetic
		}
		else if (fuc3 == F3_OR&& fuc7 == F7_OR)
		{
			ALUop = 10;//or |
		}
		else if (fuc3 == F3_OR&& fuc7 == F7_REM)
		{
			//R[rd] ← R[rs1] % R[rs2], the remaining part
			ALUop = 11;
		}
		else if (fuc3 == F3_AND&& fuc7 == F7_AND)
		{
			ALUop = 12;//and & 
		}
	}

	//I type 1
	else if (OP == OP_LB)
	{
		//same control parts
		EXTop = 1;//sign extend
		EXTsrc = imm_I;
		RegDst = 0;
		ALUop = 0;//add
		ALUSrc = 1;
		Branch = 0;
		MemWrite = 0;
		RegWrite = 1;
		MemtoReg = 0;
		if (fuc3 == F3_LB)
			//R[rd] ← SignExt(Mem(R[rs1] + offset, byte))
		{
			MemRead = 1;
		}
		else if (fuc3 == F3_LH)
		{
			MemRead = 2;
		}
		else if (fuc3 == F3_LW)
		{
			MemRead = 3;
		}
		else if (fuc3 == F3_LD)
		{
			MemRead = 4;
		}
	}

	//I type 2
	else if (OP == OP_ADDI)
	{
		//same control parts
		EXTop = 1;//sign extend
		EXTsrc = imm_I;
		RegDst = 0;
		ALUSrc = 1;
		Branch = 0;
		MemRead = 0;
		MemWrite = 0;
		RegWrite = 1;
		MemtoReg = 0;
		if (fuc3 == F3_ADDI)
		{
			ALUop = 0;//add
		}
		else if (fuc3 == F3_SLLI && fuc7 == F7_SLLI)
		{
			ALUop = 3;//sll
		}
		else if (fuc3 == F3_SLTI)
		{
			ALUop = 5;//slt
		}
		else if (fuc3 == F3_XORI)
		{
			ALUop = 6;//xor
		}
		else if (fuc3 == F3_SRLI && fuc7 == F7_SRLI)
		{
			ALUop = 8;//srl
		}
		else if (fuc3 == F3_SRLI && fuc7 == F7_SRAI)
		{
			ALUop = 9;//sra
		}
		else if (fuc3 == F3_ORI)
		{
			ALUop = 10;//or
		}
		else if (fuc3 == F3_ANDI)
		{
			ALUop = 12;//and
		}
	}

	//I type 3
	else if (OP == OP_ADDIW)
	{
		//R[rd] ← SignExt(R[rs1](31:0) + imm)
		if (fuc3 == F3_ADDIW)
		{
			EXTop = 1;//sign extend
			EXTsrc = imm_I;
			RegDst = 0;
			ALUop = 0;
			ALUSrc = 1;
			Branch = 0;
			MemRead = 0;
			MemWrite = 0;
			RegWrite = 1;
			MemtoReg = 0;
		}
	}
	//JALR
	//ID:
	//EX: PC_adder calculate PC+4, ALU calculate R[rs1]+{imm,1b'0}
	//MEM: 
	//WB: PC+4-> R[rd], ALU result->PC
	else if (OP == OP_JALR)
	{
		//???
		//R[rd] ← PC + 4
		//PC ← R[rs1] + {imm, 1b'0}
		if (fuc3 == F3_JALR)
		{
			EXTop = 1;//sign extend
			EXTsrc = imm_I * 2;
			RegDst = 0;
			ALUop = 0;
			ALUSrc = 1;
			Branch = 1;//set a new PC value
			MemRead = 0;
			MemWrite = 0;
			RegWrite = 1;
			MemtoReg = 2;
		}
	}
	else if (OP == OP_ECALL)
	{
		//Transfers control to operating system)
		//a0 = 1 is print value of a1 as an integer.
		//a0 = 10 is exit or end of code indicator
		if (fuc3 == F3_ECALL && fuc7 == F7_ECALL)
		{
			//??? 
			//what's a0 && a1 ??
		}
	}

	//S type
	else if (OP == OP_S)
	{
		//same control parts
		EXTop = 1;//sign extend
		EXTsrc = imm_S;
		RegDst = 2;//don't write to register
		ALUop = 0;
		ALUSrc = 1;
		Branch = 0;
		MemRead = 0;
		RegWrite = 0;
		MemtoReg = 0;

		//Mem(R[rs1] + offset) ← R[rs2][7:0]
		if (fuc3 == F3_SB)
		{
			MemWrite = 1;//write [7:0]
		}
		else if (fuc3 == F3_SH)
		{
			MemWrite = 2;//write[15:0]
		}
		else if (fuc3 == F3_SW)
		{
			MemWrite = 3;
		}
		else if (fuc3 == F3_SD)
		{
			MemWrite = 4;
		}
	}

	//SB type
	else if (OP == OP_SB)
	{
		//???
		//same control parts
		EXTop = 1;//signed extern, but not used in this ALU			
		EXTsrc = imm_SB * 2;
		//send to PC's Adder and add with old PC
		RegDst = 2;//don't write
		ALUop = 0;//sub, R[rs1]-R[rs2] to describe whether they are same		
				  //maybe send the result to PC selecter
		ALUSrc = 0;
		MemRead = 0;
		MemWrite = 0;
		RegWrite = 0;
		MemtoReg = 0;
		//need another ALU to add PC and imm!!!

		//if(R[rs1] == R[rs2])
		//PC ← PC + {offset, 1b'0}
		if (fuc3 == F3_BEQ)
		{
			Branch = 2;//branch if R[rs1]==R[rs2]
		}
		else if (fuc3 == F3_BNE)
		{
			Branch = 3;
		}
		else if (fuc3 == F3_BLT)
		{
			Branch = 4;
		}
		else if (fuc3 == F3_BGE)
		{
			Branch = 5;
		}
	}

	//U type 1
	//R[rd] ← PC + {offset, 12'b0}
	else if (OP == OP_AUIPC)
	{
		//???
		//get PC value as rs1 to ALU
		EXTop = 1;//sign extend
		EXTsrc = imm_U*(1<<12);
		RegDst = 0;//write to R[rd]
		ALUop = 0;
		ALUSrc = 1;
		Branch = 0;
		MemRead = 0;
		MemWrite = 0;
		RegWrite = 1;
		MemtoReg = 0;
	}
	//U type 2
	//R[rd] ← {offset, 12'b0}
	else if (OP == OP_LUI)
	{
		EXTop = 1;//sign extend
		EXTsrc = imm_U*(1 << 12);
		rs1 = 0;//reg[0]=0
		RegDst = 0;//write to R[rd]
		ALUop = 0;
		ALUSrc = 1;
		Branch = 0;
		MemRead = 0;
		MemWrite = 0;
		RegWrite = 1;
		MemtoReg = 0;
	}
	//UI type
	else if (OP == OP_JAL)
	{		
		//???
	}

	//write ID_EX_old
	ID_EX_old.Rd = rd;
	ID_EX_old.Rs1 = rs1;
	ID_EX_old.Rs2 = rs2;
	ID_EX_old.PC = PC;
	ID_EX_old.Imm = ext_signed(EXTsrc, EXTop);
	ID_EX_old.Reg_Rd = reg[rd];
	ID_EX_old.Reg_Rs1 = reg[rs1];
	ID_EX_old.Reg_Rs2 = reg[rs2];

	ID_EX_old.Ctrl_EX_ALUSrc = ALUSrc;
	ID_EX_old.Ctrl_EX_ALUOp = ALUop;
	ID_EX_old.Ctrl_EX_RegDst = RegDst;

	ID_EX_old.Ctrl_M_Branch = Branch;
	ID_EX_old.Ctrl_M_MemWrite = MemWrite;
	ID_EX_old.Ctrl_M_MemRead = MemRead;

	ID_EX_old.Ctrl_WB_RegWrite = RegWrite;
	ID_EX_old.Ctrl_WB_MemtoReg = MemtoReg;
}

//执行
void EX()
{
	//read ID_EX
	int temp_PC = ID_EX.PC;
	char RegDst = ID_EX.Ctrl_EX_RegDst;
	char ALUOp = ID_EX.Ctrl_EX_ALUOp;

	//Branch PC calulate
	//...

	//choose ALU input number
	//...

	//alu calculate
	int Zero;
	REG ALUout;
	switch (ALUOp) {
	default:;
	}

	//choose reg dst address
	int Reg_Dst;
	if (RegDst)
	{

	}
	else
	{

	}

	//write EX_MEM_old
	EX_MEM_old.ALU_out = ALUout;
	EX_MEM_old.PC = temp_PC;
	//.....
}

//访问存储器
void MEM()
{
	//read EX_MEM

	//complete Branch instruction PC change

	//read / write memory

	//write MEM_WB_old
}


//写回
void WB()
{
	//read MEM_WB

	//write reg
}