//todo:
//Greencard changes 1.3 & 1.4

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
		//更新中间寄存器
		IF_ID = IF_ID_old;
		ID();
		ID_EX = ID_EX_old;
		EX();
		EX_MEM = EX_MEM_old;
		MEM();
		MEM_WB = MEM_WB_old;
		WB();

		if (exit_flag == 1)
			break;

		reg[0] = 0;//一直为零

	}
}

//32位指令，64位寄存器
//取指令
void IF()
{
	//write IF_ID_old
	IF_ID_old.inst = memory[PC];
	PC = PC + 1;
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
	////RegDst=0: send ALUout into register rd
	////RegDst=1: send ALUout into PC
	////RegDst=2: don't write to register
	//The old version from RegDst
	//RegDst=0: write into rd
	//RegDst=1: write into PC
	//RegDst=2: don't write to register
	char RegDst;
	//ALUop=0: add
	//ALUop=1: mul, get the low 64 bits of multiple result
	//ALUop=2: sub	
	//ALUop=3: sll, shift left
	//ALUop=4: mul, get the high 64 bits of multiple result
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
	////RegWrite=1: write ALUout to register rd
	////RegWrite=2: write PC to register rd
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
			//get the low 64 bits of multiplying
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
			//get the high 64 bits of multiple result
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
			//R[rd] ← SignExt(Mem(R[rs1] + offset, word))
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
		//R[rd] ← SignExt( (R[rs1](63:0) + SignExt(imm))[31:0] )
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
	//EX: PC_adder calculate PC+4, ALU calculate { (R[rs1]+imm) ,1b'0}
	//MEM: 
	//WB: PC+4-> R[rd], ALU result->PC
	else if (OP == OP_JALR)
	{
		//???
		//R[rd] ← PC + 4
		//PC ← { (R[rs1] + imm), 1b'0}
		if (fuc3 == F3_JALR)
		{
			EXTop = 1;//sign extend
			EXTsrc = imm_I;
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
		EXTsrc = imm_U*(1 << 12);
		rs1 = -1;//special: rs1=-1 get rs1 from PC
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
		//R[rd] ← PC + 4
		//PC ← PC + {imm, 1b'0}

		//changed from JALR
		//JALR's control part

		rs1 = -1;//PC->PC+…


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
	int Rd = ID_EX.Rd;
	int Rs1 = ID_EX.Rs1;
	int Rs2 = ID_EX.Rs2;
	int temp_PC = ID_EX.PC;
	int Imm = ID_EX.Imm;
	REG Reg_Rd = ID_EX.Reg_Rd;
	REG Reg_Rs1 = ID_EX.Reg_Rs1;
	REG Reg_Rs2 = ID_EX.Reg_Rs2;

	char ALUSrc = ID_EX.Ctrl_EX_ALUSrc;
	char ALUOp = ID_EX.Ctrl_EX_ALUOp;
	char RegDst = ID_EX.Ctrl_EX_RegDst;

	char Branch = ID_EX.Ctrl_M_Branch;
	char MemWrite = ID_EX.Ctrl_M_MemWrite;
	char MemRead = ID_EX.Ctrl_M_MemRead;

	char RegWrite = ID_EX.Ctrl_WB_RegWrite;
	char MemtoReg = ID_EX.Ctrl_WB_MemtoReg;

	//Branch PC calulate
	if (Branch != 0) temp_PC = temp_PC + Imm/4;//SB & JAR
	//temp_PC+Imm/4-1 ?? 
	//
	else temp_PC = temp_PC;

	//choose ALU input number
	REG ALU_A = Reg_Rs1;
	REG ALU_B = 0;
	if (ALUSrc == 0) ALU_B = Reg_Rs2;
	else ALU_B = Imm;

	//alu calculate
	int Zero;//the branch flag
	//if (Branch==1) && (Zero==0) then jump to new PC
	REG ALUout;
	switch (ALUOp)
	{
	case 0:
	{
		ALUout = ALU_A + ALU_B;
		break;
	}
	case 1:
	{
		ALUout = ALU_A*ALU_B;//乘法的低64位
		break;
	}
	case 2:
	{
		//Branch=2: branch if R[rs1]==R[rs2]
		//Branch=3: branch if R[rs1]!=R[rs2]
		//Branch=4: branch if R[rs1]<R[rs2]
		//Branch=5: branch if R[rs1]>=R[rs2]

		ALUout = ALU_A - ALU_B;
		if (ALUout == 0 && Branch == 2) Zero = 0;
		if (ALUout != 0 && Branch == 3) Zero = 0;
		if (ALUout < 0 && Branch == 4) Zero = 0;
		if (ALUout >= 0 && Branch == 5) Zero = 0;
		break;
	}
	case 3:
	{
		ALUout = ALU_A << ALU_B;
		break;
	}
	case 4:
	{
		//乘法的高64位
		long long A_High = ((ALU_A & 0xFFFF0000) >> 32) & 0xFFFF;
		long long A_Low = ALU_A & 0x0000FFFF;
		long long B_High = ((ALU_B & 0xFFFF0000) >> 32) & 0xFFFF;
		long long B_Low = ALU_B & 0x0000FFFF;

		long long AHBH = A_High*B_High;
		long long AHBL = ((A_High*B_Low) >> 32) & 0xFFFF;
		long long ALBH = ((A_Low*B_High) >> 32) & 0xFFFF;

		ALUout = AHBH + AHBL + ALBH;
		break;
	}
	case 5:
	{
		ALUout = (ALU_A < ALU_B) ? 1 : 0;
		break;
	}
	case 6:
	{
		ALUout = ALU_A ^ ALU_B;
		break;
	}
	case 7:
	{
		ALUout = ALU_A / ALU_B;
		break;
	}
	case 8:
	{
		ALUout = (unsigned)ALU_A >> ALU_B;//逻辑右移
		break;
	}
	case 9:
	{
		ALUout = (signed)ALU_A >> ALU_B;//算术右移
		break;
	}
	case 10:
	{
		ALUout = ALU_A / ALU_B;
		break;
	}
	case 11:
	{
		ALUout = ALU_A % ALU_B;
		break;
	}
	case 12:
	{
		ALUout = ALU_A & ALU_B;
		break;
	}
	default: ALUout = 0;
	}

	//choose reg dst address
	//RegDst=0: write into rd
	//RegDst=1: write into PC
	//RegDst=2: don't write to register
	//???前面Reg_Dst还是char的…
	//越写越乱了QAQ
	int Reg_Dst;
	if (RegDst == 0)
	{

	}
	else
	{

	}

	//write EX_MEM_old
	EX_MEM_old.PC = temp_PC;
	EX_MEM_old.Reg_Dst = Reg_Dst;//???
	EX_MEM_old.ALU_out = ALUout;
	EX_MEM_old.Zero = Zero;
	EX_MEM_old.Reg_Rd = Reg_Rd;//???

	EX_MEM_old.Ctrl_M_Branch = Branch;
	EX_MEM_old.Ctrl_M_MemWrite = MemWrite;
	EX_MEM_old.Ctrl_M_MemRead = MemRead;

	EX_MEM_old.Ctrl_WB_RegWrite = RegWrite;
	EX_MEM_old.Ctrl_WB_MemtoReg = MemtoReg;

}

//访问存储器
void MEM()
{
	//read EX_MEM
	int temp_PC = EX_MEM.PC;
	int Reg_Dst = EX_MEM.Reg_Dst;//???
	REG ALUout = EX_MEM.ALU_out;
	int Zero = EX_MEM.Zero;
	REG Reg_Rs2 = EX_MEM.Reg_Rs2;

	char Branch = EX_MEM.Ctrl_M_Branch;
	char MemWrite = EX_MEM.Ctrl_M_MemWrite;
	char MemRead = EX_MEM.Ctrl_M_MemRead;

	char RegWrite = EX_MEM.Ctrl_WB_RegWrite;
	char MemtoReg = EX_MEM.Ctrl_WB_MemtoReg;

	//complete Branch instruction PC change
	if (Branch != 0 && Zero == 0) PC = temp_PC;//跳转，设置新地址

	//read / write memory
	unsigned long long int Mem_read;
	//MemRead=0: don't read from memory
	//MemRead=1: read from memory, byte
	//MemRead=2: read from memory, half word, 2 byte
	//MemRead=3: read from memory, word, 4 byte
	//MemRead=4: read from memory, double word, 8 byte	
	if (MemRead == 1)
	{
		Mem_read = ext_signed(memory[ALUout]&0xFF, 1);
	}
	else if (MemRead == 2)
	{
		Mem_read = ext_signed(memory[ALUout] & 0xFFFF, 1);
	}
	else if (MemRead == 3)
	{
		Mem_read = ext_signed(memory[ALUout] & 0xFFFFFFFF, 1);
	}
	else if (MemRead == 4)
	{
		//注意小端法
		//读两个memory
		Mem_read = memory[ALUout]+(1<<31)*memory[ALUout+1];
		//我觉得这个是小端法了，但不确定…

	}

	//write to memory
	//MemWrite=0: don't write to memory
	//MemWrite=1: write to memory, byte, [7:0]
	//MemWrite=2: write to memory, half word[15:0]
	//MemWrite=3: write to memory, word[31:0]
	//MemWrite=4: write to memory, double word[63:0]	
	if (MemWrite == 1)
	{
		memory[ALUout] = Reg_Rs2& 0xFF;
	}
	else if (MemWrite == 2)
	{
		memory[ALUout] = Reg_Rs2 & 0xFFFF;
	}
	else if (MemWrite == 3)
	{
		memory[ALUout] = Reg_Rs2 & 0xFFFFFFFF;
	}
	else if (MemWrite == 4)
	{
		//我感觉这是小端的放法，也比划了一下，然而还是可能出bugQAQ
		memory[ALUout] = Reg_Rs2 & 0xFFFFFFFF;
		memory[ALUout + 1] = Reg_Rs2& (0xFFFFFFFF << 32);
	}

	//write MEM_WB_old
	MEM_WB_old.Mem_read = Mem_read;
	MEM_WB_old.ALU_out = ALUout;
	MEM_WB_old.Reg_Dst = Reg_Dst;
	
	MEM_WB_old.Ctrl_WB_RegWrite = RegWrite;
	MEM_WB_old.Ctrl_WB_MemtoReg = MemtoReg;
}


//写回
void WB()
{
	//read MEM_WB

	//write reg
}