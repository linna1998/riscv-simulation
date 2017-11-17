#include "Simulation.h"
#include <string.h>
using namespace std;

extern void read_elf();

extern unsigned long long cadr;
extern unsigned long long csize;
extern unsigned long long dadr;
extern unsigned long long dsize;
extern unsigned long long sdadr;
extern unsigned long long sdsize;
extern unsigned long long radr;
extern unsigned long long rsize;
extern unsigned long long a_adr;
extern unsigned long long b_adr;
extern unsigned long long c_adr;
extern unsigned long long temp_adr;
extern unsigned long long sum_adr;
extern unsigned long long gp;
extern unsigned int entry;
extern unsigned int endPC;
extern FILE *file;

// 单步调试flag，如果TF == 1，进入单步调试模式，每步输出寄存器堆
static bool TF = false;

// ProcessorMode = 0: Single Cycle Processor
// ProcessorMode = 1: Multiple Cycle Processor
// ProcessorMode = 2: Pipeline Processor
static int ProcessorMode = 0;

// 系统调用退出指示
#define RET_INST 0x00008067

static char SymbolicName[32][3] = {
	"0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
	"s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
	"a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
	"s8", "s9", "sa", "sb", "t3", "t4", "t5", "t6"
};

// 打印寄存器堆
void print_regs()
{
	/*
	printf("Register:\n");
	for (int i = 0; i < 32; i++)
	{
		printf("[%2d][%2s]0x%8llx  ", i, SymbolicName[i], reg[i]);
		printf("[%2d][%2s]0x%8llx  ", i, SymbolicName[i], reg[++i]);
		printf("[%2d][%2s]0x%8llx  ", i, SymbolicName[i], reg[++i]);
		printf("[%2d][%2s]0x%8llx\n", i, SymbolicName[i], reg[++i]);
	}
	printf("\n");
	*/
	printf("\n");
	for (int i = 0; i < 32; i++)
	{
		if (reg[i] != 0) printf("[%2d][%2s]0x%8llx\n", i, SymbolicName[i], reg[i]);
	}
}

// 打印栈
void print_stack()
{
	printf("Highest 32 Stack: \n");
	for (int i = 0; i < 32 * 4; i += 4)
	{
		printf("[%08x] %08x ", MAX / 2 - i, memory[(MAX / 2 - i) >> 2]);
		i += 4;
		printf("[%08x] %08x ", MAX / 2 - i, memory[(MAX / 2 - i) >> 2]);
		i += 4;
		printf("[%08x] %08x ", MAX / 2 - i, memory[(MAX / 2 - i) >> 2]);
		i += 4;
		printf("[%08x] %08x\n", MAX / 2 - i, memory[(MAX / 2 - i) >> 2]);
	}
	printf("\n");
}

// 打印代码段
void print_data()
{
	printf(".data Section Memory:\n");
	for (int i = 0; i < dsize; i += 4)
	{
		printf("[%08x] %08x ", (int)(dadr + i), memory[(dadr + i) >> 2]);
		i += 4;
		printf("[%08x] %08x ", (int)(dadr + i), memory[(dadr + i) >> 2]);
		i += 4;
		printf("[%08x] %08x ", (int)(dadr + i), memory[(dadr + i) >> 2]);
		i += 4;
		printf("[%08x] %08x\n", (int)(dadr + i), memory[(dadr + i) >> 2]);
	}
	printf("\n");
}

void print_result()
{
	printf("Result:\n");
	for (int i = 0; i < rsize; i += 4)
	{
		printf("[%08x] %08x ", (int)(radr + i), memory[(radr + i) >> 2]);
		i += 4;
		printf("[%08x] %08x ", (int)(radr + i), memory[(radr + i) >> 2]);
		i += 4;
		printf("[%08x] %08x ", (int)(radr + i), memory[(radr + i) >> 2]);
		i += 4;
		printf("[%08x] %08x\n", (int)(radr + i), memory[(radr + i) >> 2]);
	}
	printf("\n");
	printf("a: %d\n", memory[a_adr >> 2]);
	printf("b: %d\n", memory[b_adr >> 2]);
	printf("c: %d\n", memory[c_adr >> 2]);
	printf("temp: %d\n", memory[temp_adr >> 2]);
	printf("sum: %d\n", memory[sum_adr >> 2]);
}

// 打印代码段
void print_text()
{
	printf(".text Section Memory:\n");
	for (int i = 0; i < csize; i += 4)
	{
		printf("[%08x] %08x ", (int)(cadr + i), memory[(cadr + i) >> 2]);
		i += 4;
		printf("[%08x] %08x ", (int)(cadr + i), memory[(cadr + i) >> 2]);
		i += 4;
		printf("[%08x] %08x ", (int)(cadr + i), memory[(cadr + i) >> 2]);
		i += 4;
		printf("[%08x] %08x\n", (int)(cadr + i), memory[(cadr + i) >> 2]);
	}
	printf("\n");
}

// 加载代码段
void load_text()
{
	if (fseek(file, cadr - 0x10000, 0))
	{
		perror("fseek");
	}
	if (!fread(&memory[cadr >> 2], 1, csize, file))
	{
		perror("fread");
	}  // 注意这里vadr换算到memory数组要除以4
}

void load_data()
{
	if (fseek(file, dadr - 0x10000, 0))
	{
		perror("fseek");
	}
	if (!fread(&memory[dadr >> 2], 1, dsize, file))
	{
		perror("fread");
	}  // 注意这里dadr换算到memory数组要除以4
}

void load_sdata()
{
	if (fseek(file, sdadr - 0x10000, 0))
	{
		perror("fseek");
	}
	if (!fread(&memory[sdadr >> 2], 1, sdsize, file))
	{
		perror("fread");
	}  // 注意这里dadr换算到memory数组要除以4
}

int main(int argc, char* argv[])
{
	char filename[15] = "./test";
	// Parse the arguments.
	if (argc != 1)
	{
		for (int i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "--help") == 0)
			{
				cout << "HELP   :" << endl;
				cout << "  --TF   : Enter step through mode.  Not TF Mode by default." << endl;
				cout << "  --name : The excutable file's name. './test' by default." << endl;
				cout << "           For example, --name=./QuickSort" << endl;
				cout << "  --SF, --MF, --TF: The simulator's modules." << endl;
				cout << "           --SF means single cycle processor." << endl;
				cout << "           --MF means multiple cycle processor." << endl;
				cout << "           --PF means pipelin processor." << endl;
				cout << "           Single cycle processor by default." << endl;
				return 0;
			}
			else if (strstr(argv[i], "--name=") != NULL)
			{
				strcpy(filename, argv[i] + 7);
			}
			else if (strcmp(argv[i], "--TF") == 0)
			{
				cout << "TF Mode." << endl;
				cout << endl;
				TF = true;
			}
			else if (strcmp(argv[i], "--SF") == 0)
			{
				cout << "Single cycle processor." << endl;
				cout << endl;
				ProcessorMode = 0;
			}
			else if (strcmp(argv[i], "--MF") == 0)
			{
				cout << "Multiple cycle processor." << endl;
				cout << endl;
				ProcessorMode = 1;
			}
			else if (strcmp(argv[i], "--PF") == 0)
			{
				cout << "Pipeline processor." << endl;
				cout << endl;
				ProcessorMode = 2;
			}
			else
			{
				cout << "ERROR:  Unknown flags. See --help." << endl;
				return 0;
			}
		}
	}

	// 解析elf文件
	cout << "OPEN FILE: " << filename << endl << endl;
	file = fopen(filename, "r+");
	if (!file)
	{
		perror("Open executable file");
		return false;
	}
	read_elf();

	// 加载内存
	load_text();
	load_data();
	load_sdata();
	fclose(file);

	// 设置入口地址
	global_PC = entry >> 2;

	// 设置全局数据段地址寄存器
	reg[3] = gp;

	reg[2] = MAX / 2;  // 栈基址 （sp寄存器）

	cout << "[!] Simulation starts!" << endl;
	printf("===============================================================================\n");

	if (ProcessorMode == 0)
		SingleCycleProcessor();
	else if (ProcessorMode == 1)
		MultiCycleProcessor();
	else if (ProcessorMode == 2)
		PipelineProcessor();

	printf("===============================================================================\n");
	printf("[!] Simulation is over!\n");
	printf(" Instruction Number : %d\n", num_inst);
	printf(" Cycle Number       : %d\n", num_cycle);
	printf(" CPI                : %f\n", num_cycle / (float)(num_inst));
	printf("Please press Enter to continue...\n");

	getchar();

	return 0;
}

void SingleCycleProcessor()
{
	num_cycle = 0;
	num_inst = 0;
	while ((global_PC * 4) != endPC)
	{
		if (TF)
		{
			printf("[TF] Instruction:  %08x\n", memory[global_PC]);
			printf("PC              :  %08X\n", global_PC * 4);
		}
		// Addd instruction number.
		num_inst++;
		num_cycle++;
		IF();
		// 更新中间寄存器
		IF_ID = IF_ID_old;

		ID();
		ID_EX = ID_EX_old;

		bool jump= EX();
		EX_MEM = EX_MEM_old;		
						
		MEM();
		MEM_WB = MEM_WB_old;

		WB();
		
		if (jump)
		{
			global_PC = EX_MEM.Jump_PC;//跳转，设置新地址
		}
		
		reg[0] = 0;  // 一直为零

		if (TF)
		{
			print_regs();
			print_stack();
			print_result();
			getchar();
		}
	}

	printf("===============================================================================\n");
	printf("[!] Return from main function.\n");
	print_regs();
	print_stack();
	print_result();
}

void MultiCycleProcessor()
{
	state = STATE_IF;
	num_cycle = 0;
	num_inst = 0;
	int temp_state = state;
	while ((global_PC * 4) != endPC)
	{
		if (TF)
		{
			printf("[TF] Instruction:  %08x\n", memory[global_PC]);
			printf("PC              :  %08X\n", global_PC * 4);
			cout << "state           :" << state << endl;
		}
		temp_state = state;
		// 运行
		switch (state)
		{
			// If state = 0, then come to this state 
		case (STATE_IF):
		{
			IF();
			IF_ID = IF_ID_old;
			break;
		}
		case (STATE_ID):
		{
			ID();
			ID_EX = ID_EX_old;
			break;
		}
		case (STATE_EX_R):
		{
			bool jump = EX();
			// Jump for JAL JALR. 
			EX_WB= EX_WB_old;
			if (jump)
			{
				printf("Jump_PC to %d.\n", EX_WB.Jump_PC*4);
				global_PC = EX_WB.Jump_PC;//跳转，设置新地址
			}
			break;
		}
		case (STATE_EX_MUL):
		{
			EX();
			EX_WB = EX_WB_old;
			break;
		}
		case (STATE_EX_DIV):
		{
			EX();
			EX_WB = EX_WB_old;
			break;
		}
		case (STATE_EX_LB):
		{
			EX();
			EX_MEM = EX_MEM_old;
			break;
		}
		case (STATE_EX_S):
		{
			EX();
			EX_MEM = EX_MEM_old;
			break;
		}
		case (STATE_EX_SB):
		{		
			//如果返回true说明跳转
			//如果返回false说明不跳转
			bool jump = EX();  
			EX_MEM = EX_MEM_old;
			if (jump)
			{
				printf("Jump_PC to %d.\n", EX_MEM.Jump_PC*4);
				global_PC = EX_MEM.Jump_PC;//跳转，设置新地址
			}
			break;
		}
		case (STATE_MEM_LB):
		{
			MEM();
			MEM_WB = MEM_WB_old;
			break;
		}
		case (STATE_MEM_S):
		{
			MEM();
			break;
		}
		case (STATE_WB_R):
		{
			WB();
			break;
		}
		case (STATE_WB_LB):
		{
			WB();
			break;
		}
		}

		// Set new state.
		// ID set new state in ID() function.
		if (temp_state != STATE_ID)
		{
			state = state_change[temp_state];
		}
		// Addd instruction number.
		if (temp_state == STATE_IF)
		{
			num_inst++;
		}
		//// Add cycle number;
		//num_cycle += cycle_count[temp_state];
		//if (cycle_count[temp_state] == 40)
		//{
		//	printf("DIV REM here. num_cycle += 40\n");
		//}
		//if (cycle_count[temp_state] == 2)
		//{
		//	printf("MUL here.     num_cycle += 2\n");
		//}

		//有了流水线之后，num_cycle在EX里面加了
		num_cycle += 1;	

		reg[0] = 0;  // 一直为零

		if (TF)
		{
			print_regs();
			print_stack();
			//print_result();
			getchar();
		}
	}

	printf("===============================================================================\n");
	printf("[!] Return from main function.\n");
	print_regs();
	print_stack();
	print_result();
}

void PipelineProcessor()
{
	num_cycle = 0;
	num_inst = 0;
	bool conflict = false;
	bool jump = false;
	int temp_IF_PC = 0;
	// Initialize.
	for (int i = 0; i < 3; i++)
	{
		RdQueue.push_back(-1);
	}
	ID_EX.havePushedRd = 1;
	EX_MEM.havePushedRd = 1;
	EX_WB.havePushedRd = 1;
	MEM_WB.havePushedRd = 1;

	//ID_EX.isNop = 1;
	//EX_MEM.isNop = 1;
	//EX_WB.isNop = 1;
	//MEM_WB.isNop = 1;

	while ((MEM_WB.PC * 4) != endPC)
	{
		num_cycle++;
		if (MEM_WB.isNop == 0) num_inst++;
		temp_IF_PC = global_PC;
		if (TF)
		{
			printf("[TF] Instruction:  %08x\n", memory[global_PC]);
			printf("Before doing things.\n");
			printf("PC_IF             :  %08X\n", global_PC * 4);
			printf("PC_ID             :  %08X, isNop  %d\n", IF_ID.PC * 4, IF_ID.isNop);
			printf("PC_EX             :  %08X, isNop  %d, havePushedRd  %d\n",
				ID_EX.PC * 4, ID_EX.isNop, ID_EX.havePushedRd);
			printf("PC_MEM         :  %08X, isNop  %d, havePushedRd  %d\n",
				EX_MEM.PC * 4, EX_MEM.isNop, EX_MEM.havePushedRd);
			printf("PC_WB           :  %08X, isNop  %d, havePushedRd  %d\n",
				MEM_WB.PC * 4, MEM_WB.isNop, MEM_WB.havePushedRd);
			for (int i = 0; i < RdQueue.size(); i++)
			{
				printf("RdQueueu[%d]: %d   ", i, RdQueue[i]);
			}
		}

		IF();
		conflict = ID();
		//如果返回true说明跳转
		//如果返回false说明不跳转
		jump= EX();  // EX中跳转可能再改PC，这个改的优先级高
		MEM();
		WB();
		
		// PC 跳转逻辑		
		if (jump)
		{
			global_PC = EX_MEM_old.Jump_PC;//跳转，设置新地址
			IF_ID_old.isNop = 1;
			ID_EX_old.isNop = 1;
			if (ID_EX_old.havePushedRd == 1)
			{
				RdQueue.pop_back();
				ID_EX_old.havePushedRd = 0;
			}
		}
		else if (conflict)
		{
			// ???
			global_PC = temp_IF_PC;  // 取指取原址

			//printf("global_PC become %08X\n.", global_PC * 4);
			IF_ID_old.isNop = 1;
			ID_EX_old.isNop = 1;
			if (ID_EX_old.havePushedRd == 1)
			{
				RdQueue.pop_back();
				ID_EX_old.havePushedRd = 0;
			}
		}
		////complete Branch instruction PC change
		//if ((EX_MEM_old.Ctrl_M_Branch == 2 || EX_MEM_old.Ctrl_M_Branch == 3 || EX_MEM_old.Ctrl_M_Branch == 4 || EX_MEM_old.Ctrl_M_Branch == 5)
		//	&& EX_MEM_old.Ctrl_M_Zero == 0)
		//{
		//	global_PC = EX_MEM_old.Jump_PC;//跳转，设置新地址, SB type
		//	IF_ID_old.isNop = 1;
		//	ID_EX_old.Ctrl_M_MemWrite = 0;
		//	ID_EX_old.Ctrl_WB_RegWrite = 0;
		//}
		//else if (EX_MEM_old.Ctrl_M_Branch == 1 || EX_MEM_old.Ctrl_M_Branch == 6)
		//{
		//	global_PC = EX_MEM_old.Jump_PC;// JAL&JALR
		//	IF_ID_old.isNop = 1;
		//	//ID_EX_old.Rd = -1;
		//	ID_EX_old.Ctrl_M_MemWrite = 0;
		//	ID_EX_old.Ctrl_WB_RegWrite = 0;
		//}


		// 更新中间寄存器		
		if (!(conflict && !jump)) IF_ID = IF_ID_old;
		//IF_ID = IF_ID_old;
		ID_EX = ID_EX_old;
		EX_MEM = EX_MEM_old;
		MEM_WB = MEM_WB_old;

		reg[0] = 0;  // 一直为零

		if (TF)
		{

			printf("After doing things.\n");
			printf("PC_IF             :  %08X\n", global_PC * 4);
			printf("PC_ID             :  %08X, isNop  %d\n", IF_ID.PC * 4, IF_ID.isNop);
			printf("PC_EX             :  %08X, isNop  %d, havePushedRd  %d\n",
				ID_EX.PC * 4, ID_EX.isNop, ID_EX.havePushedRd);
			printf("PC_MEM         :  %08X, isNop  %d, havePushedRd  %d\n",
				EX_MEM.PC * 4, EX_MEM.isNop, EX_MEM.havePushedRd);
			printf("PC_WB           :  %08X, isNop  %d, havePushedRd  %d\n",
				MEM_WB.PC * 4, MEM_WB.isNop, MEM_WB.havePushedRd);
			for (int i = 0; i < RdQueue.size(); i++)
			{
				printf("RdQueueu[%d]: %d   ", i, RdQueue[i]);
			}
			// Program 1&2 debug.
			//printf("b: M[11760] 0x%8llx.\n", memory[0x11760>>2]);
			//printf("c: M[11764] 0x%8llx.\n", memory[0x11764 >> 2]);

			// Program 3&4 debug.
			//printf("-20(s0) 0x%8llx.\n", memory[( 0x8000000-0x20)>> 2]);

			print_regs();
			//print_stack();
			print_result();
			getchar();
		}
	}
	num_inst = num_inst - 3;//// 除去开始前三条划水指令
	printf("===============================================================================\n");
	printf("[!] Return from main function.\n");
	print_regs();
	print_stack();
	print_result();
}


//32位指令，64位寄存器
//取指令
void IF()
{
	//write IF_ID_old
	IF_ID_old.inst = memory[global_PC];
	IF_ID_old.PC = global_PC;//存的是本指令的PC
	global_PC = global_PC + 1;//int memory[], so PC+1 means add 4 bytes	
	IF_ID_old.isNop = 0;
}

//译码
//返回是否发生数据冲突
bool ID()
{
	//Read IF_ID
	unsigned int inst = IF_ID.inst;
	unsigned int temp_PC = IF_ID.PC;  // 本指令PC
	int isNop = IF_ID.isNop;

	//EXTop=0: Zero extend
	//EXTop=1: sign extend
	int EXTop = 0;
	//the number needed to be extended
	unsigned int EXTsrc = 0;

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
	//ALUSrcA=0: A from register rs1
	//ALUSrcA=1: A from PC 
	char ALUSrcA;
	//ALUSrcB=0: B from register rs2
	//ALUSrcB=1: B from Extend part 
	char ALUSrcB;

	//Branch=0: new_PC=PC+1
	//Branch=1: (old_PC*4+IMM*2)/4->PC, choose to branch in JAL
	//Branch=2: branch if R[rs1]==R[rs2]
	//Branch=3: branch if R[rs1]!=R[rs2]
	//Branch=4: branch if R[rs1]<R[rs2]
	//Branch=5: branch if R[rs1]>=R[rs2]
	//Branch = 6: (ALU*2)/4->PC in JALR
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
	//MemtoReg=2: write old_PC*4+4 to R[rd] in JALR&JALto register
	//MemtoReg=3: send ALU's result to register, and signed extend it to 64 bits
	char MemtoReg;

	OP = getbit(inst, 0, 6);
	fuc3 = getbit(inst, 12, 14);
	fuc7 = getbit(inst, 25, 31);
	////shamt: same direction with rs2
	//in v1.3 change shamt into 6 bits
	shamt = getbit(inst, 20, 25);
	rs1 = getbit(inst, 15, 19);
	rs2 = getbit(inst, 20, 24);
	rd = getbit(inst, 7, 11);

	//immediate numbers are in different kinds and be put in different areas
	imm_I = getbit(inst, 20, 31);//I
	int temp_S1 = getbit(inst, 7, 11);
	int temp_S2 = getbit(inst, 25, 31)*(1 << 5);
	imm_S = temp_S1 + temp_S2;  // S

	int temp_SB1 = getbit(inst, 8, 11)*(1 << 1);
	int temp_SB2 = getbit(inst, 25, 30)*(1 << 5);
	int temp_SB3 = getbit(inst, 7, 7)*(1 << 11);
	int temp_SB4 = getbit(inst, 31, 31)*(1 << 12);
	imm_SB = temp_SB1 + temp_SB2 + temp_SB3 + temp_SB4;//SB

	imm_U = getbit(inst, 12, 31);//U

	int temp_1 = getbit(inst, 21, 30)*(1 << 1);
	int temp_2 = getbit(inst, 20, 20)*(1 << 11);
	int temp_3 = getbit(inst, 12, 19)*(1 << 12);
	int temp_4 = getbit(inst, 31, 31)*(1 << 20);//UJ
	imm_UJ = temp_1 + temp_2 + temp_3 + temp_4;

	int next_state = STATE_EX_R;  // 先设一个默认的缺失值，后面会改的

	// NOP instruction
	if (isNop)
	{
		next_state = STATE_IF;
		rd = -1;
		EXTop = 0;
		ALUSrcA = 0;
		ALUSrcB = 0;
		ALUop = 0;
		Branch = 0;
		MemRead = 0;
		MemWrite = 0;
		RegWrite = 0;
		MemtoReg = 0;
	}
	//R type
	else if (OP == OP_R)
	{
		next_state = STATE_EX_R;  // Except 64-bit multiple and div,rem
		//same control parts
		EXTop = 0;
		ALUSrcA = 0;
		ALUSrcB = 0;
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
			next_state = STATE_EX_MUL;
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
			next_state = STATE_EX_MUL;
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
			next_state = STATE_EX_DIV;
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
			next_state = STATE_EX_DIV;
		}
		else if (fuc3 == F3_AND&& fuc7 == F7_AND)
		{
			ALUop = 12;//and & 
		}
	}
	//RW
	else if (OP == OP_ADDW)
	{
		next_state = STATE_EX_R;
		// EXTop no need
		// EXTsrc no need
		ALUSrcA = 0;
		ALUSrcB = 0;
		Branch = 0;
		MemRead = 0;
		MemWrite = 0;
		RegWrite = 1;
		MemtoReg = 3;
		if (fuc3 == F3_ADDW && fuc7 == F7_ADDW)
		{
			ALUop = 0;
		}
		if (fuc3 == F3_ADDW && fuc7 == F7_MULW)
		{
			ALUop = 1;
		}
		if (fuc3 == F3_ADDW && fuc7 == F7_SUBW)
		{
			ALUop = 2;
		}
		if (fuc3 == F3_DIVW && fuc7 == F7_DIVW)
		{
			next_state = STATE_EX_DIV;
			ALUop = 7;
		}
	}
	//I type 1
	else if (OP == OP_LB)
	{
		next_state = STATE_EX_LB;
		//same control parts
		EXTop = 1;//sign extend
		EXTsrc = imm_I;
		ALUop = 0;//add
		ALUSrcA = 0;
		ALUSrcB = 1;
		Branch = 0;
		MemWrite = 0;
		RegWrite = 1;
		MemtoReg = 1;
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
		next_state = STATE_EX_R;
		//same control parts
		EXTop = 1;//sign extend
		EXTsrc = imm_I;//except the shifts, they use shamt
		ALUSrcA = 0;
		ALUSrcB = 1;
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
			EXTsrc = shamt;
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
			EXTsrc = shamt;
			ALUop = 8;//srl
		}
		else if (fuc3 == F3_SRLI && fuc7 == F7_SRAI)
		{
			EXTsrc = shamt;
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
		next_state = STATE_EX_R;
		//R[rd] ← SignExt( (R[rs1](63:0) + SignExt(imm))[31:0] )
		if (fuc3 == F3_ADDIW)
		{
			EXTop = 1;//sign extend
			EXTsrc = imm_I;
			ALUop = 0;
			ALUSrcA = 0;
			ALUSrcB = 1;
			Branch = 0;
			MemRead = 0;
			MemWrite = 0;
			RegWrite = 1;
			MemtoReg = 0;
		}
		//SLLIW
		else if (fuc3 == F3_SLLIW)
		{
			EXTop = 1;//sign extend
			EXTsrc = shamt;
			ALUop = 3;
			ALUSrcA = 0;
			ALUSrcB = 1;
			Branch = 0;
			MemRead = 0;
			MemWrite = 0;
			RegWrite = 1;
			MemtoReg = 0;
		}
		//SRLIW
		else if (fuc3 == F3_SRLIW && fuc7 == F7_SRLIW)
		{
			EXTop = 1;//sign extend
			EXTsrc = shamt;
			ALUop = 8;
			ALUSrcA = 0;
			ALUSrcB = 1;
			Branch = 0;
			MemRead = 0;
			MemWrite = 0;
			RegWrite = 1;
			MemtoReg = 0;
		}
		//SRAIW
		else if (fuc3 == F3_SRLIW && fuc7 == F7_SRAIW)
		{
			EXTop = 1;//sign extend
			EXTsrc = shamt;
			ALUop = 9;
			ALUSrcA = 0;
			ALUSrcB = 1;
			Branch = 0;
			MemRead = 0;
			MemWrite = 0;
			RegWrite = 1;
			MemtoReg = 0;
		}
	}
	//JALR
	//ID:
	//EX: ALU calculate { (R[rs1]+imm) ,1b'0}
	//MEM: 	
	//WB: old_PC*4+4 -> R[rd], (ALU result*2)/4->PC
	else if (OP == OP_JALR)
	{

		next_state = STATE_EX_R;
		//???
		//R[rd] ←old_PC*4+4
		//PC ← { (R[rs1] + imm), 1b'0} /4
		if (fuc3 == F3_JALR)
		{
			EXTop = 1;//sign extend
			EXTsrc = imm_I;
			//printf("IMM : %llx\n", imm_I);
			ALUop = 0;
			ALUSrcA = 0;
			ALUSrcB = 1;
			Branch = 6;//(ALUresult*2)/4->PC in JALR
			MemRead = 0;
			MemWrite = 0;
			RegWrite = 1;
			MemtoReg = 2;
		}
	}
	else if (OP == OP_ECALL)
	{
		next_state = STATE_EX_R;
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
		next_state = STATE_EX_S;
		//same control parts
		EXTop = 1;//sign extend
		EXTsrc = imm_S;
		ALUop = 0;
		ALUSrcA = 0;
		ALUSrcB = 1;
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
		next_state = STATE_EX_SB;
		//???
		//same control parts
		EXTop = 1;//signed extern, but not used in this ALU			
		EXTsrc = imm_SB;
		//send to PC's Adder and add with old PC
		ALUop = 2;//sub, R[rs1]-R[rs2] to describe whether they are same		
				  //maybe send the result to PC selecter
		ALUSrcA = 0;
		ALUSrcB = 0;
		MemRead = 0;
		MemWrite = 0;
		RegWrite = 0;
		MemtoReg = 0;
		//need another ALU to add PC and imm!!!

		//if(R[rs1] == R[rs2])
		//PC ← ( PC*4 + {offset, 1b'0} ) / 4
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
		next_state = STATE_EX_R;
		//???
		//get PC value as rs1 to ALU
		EXTop = 1;//sign extend
		EXTsrc = imm_U*(1 << 12);
		ALUop = 0;
		ALUSrcA = 1;  // get rs1 from PC
		ALUSrcB = 1;
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
		next_state = STATE_EX_R;
		EXTop = 1;//sign extend
		EXTsrc = imm_U*(1 << 12);
		rs1 = 0;//reg[0]=0
		ALUop = 0;
		ALUSrcA = 0;
		ALUSrcB = 1;
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
		//R[rd] ← old_PC*4 + 4
		//PC ← ( old_PC *4 + {imm, 1b'0} )/4 //finished
		next_state = STATE_EX_R;
		EXTop = 1;//sign extend
		EXTsrc = ext_signed(imm_UJ, 1);
		rs1 = 0;//reg[0]=0
		ALUop = 0;
		ALUSrcA = 0;
		ALUSrcB = 1;
		Branch = 1;//(old_PC*4+EXTsrc)/4->PC
		MemRead = 0;
		MemWrite = 0;
		RegWrite = 1;
		MemtoReg = 2;

		printf("JAL ID");
	}

	//多周期
	state = next_state;

	//write ID_EX_old
	ID_EX_old.Rd = rd;
	ID_EX_old.PC = temp_PC;  // 本指令PC
	ID_EX_old.Imm = ext_signed(EXTsrc, EXTop);
	ID_EX_old.Reg_Rs1 = reg[rs1];  // 译码阶段取操作数
	ID_EX_old.Reg_Rs2 = reg[rs2];
	ID_EX_old.isNop = isNop;
	ID_EX_old.havePushedRd = 0;

	ID_EX_old.Ctrl_EX_ALUSrcA = ALUSrcA;
	ID_EX_old.Ctrl_EX_ALUSrcB = ALUSrcB;
	ID_EX_old.Ctrl_EX_ALUOp = ALUop;

	ID_EX_old.Ctrl_M_Branch = Branch;
	ID_EX_old.Ctrl_M_MemWrite = MemWrite;
	ID_EX_old.Ctrl_M_MemRead = MemRead;

	ID_EX_old.Ctrl_WB_RegWrite = RegWrite;
	ID_EX_old.Ctrl_WB_MemtoReg = MemtoReg;

	// Debug.
	/*
	if (OP == OP_S)
	{
		printf("OP_SB. Rs1 = %d. Rs2 = %d \n", rs1, rs2);
	}
	if (temp_PC * 4 == 0x101ec)
	{
		printf("0x101ec. OP = %d. Rs1 = %d. Rs2 = %d \n", OP, rs1, rs2);
	}
	*/

	// 流水线下，针对非空指令
	if (isNop == 0 && ProcessorMode == 2)
	{
		//判断冲突
		vector<int>::iterator itRs1, itRs2;
		itRs1 = find(RdQueue.begin(), RdQueue.end(), rs1);
		itRs2 = find(RdQueue.begin(), RdQueue.end(), rs2);
		if (ALUSrcA == 0 && itRs1 != RdQueue.end())
		{
			//printf("Rs1 hazard. Rs1 = %d.\n", rs1);
			// 可以把冲突时候的push去了么？
			//RdQueue.push_back(-1);
			//ID_EX_old.havePushedRd = 1;
			return true;
		}
		else if ((ALUSrcB == 0 || OP == OP_S) && itRs2 != RdQueue.end())
		{
			//printf("Rs2 hazard. Rs2 = %d.\n", rs2);
			//RdQueue.push_back(-1);
			//ID_EX_old.havePushedRd = 1;
			return true;
		}

		//流水线没有数据冒险，rd入队
		if (RegWrite == 1)
		{
			RdQueue.push_back(rd);
			ID_EX_old.havePushedRd = 1;
		}
		// 否则，非空指令入队-1
		else
		{
			RdQueue.push_back(-1);
			ID_EX_old.havePushedRd = 1;
		}
	}
	return false;
}

//执行
//如果返回true说明跳转
//如果返回false说明不跳转
bool EX()
{
	//read ID_EX	
	int Rd = ID_EX.Rd;
	int temp_PC = ID_EX.PC;  // 本指令PC
	int old_PC = ID_EX.PC;  // 本指令PC
	int Imm = ID_EX.Imm;
	REG Reg_Rs1 = ID_EX.Reg_Rs1;
	REG Reg_Rs2 = ID_EX.Reg_Rs2;
	int isNop = ID_EX.isNop;
	int havePushedRd = ID_EX.havePushedRd;

	char ALUSrcA = ID_EX.Ctrl_EX_ALUSrcA;
	char ALUSrcB = ID_EX.Ctrl_EX_ALUSrcB;
	char ALUOp = ID_EX.Ctrl_EX_ALUOp;

	char Branch = ID_EX.Ctrl_M_Branch;
	char MemWrite = ID_EX.Ctrl_M_MemWrite;
	char MemRead = ID_EX.Ctrl_M_MemRead;

	char RegWrite = ID_EX.Ctrl_WB_RegWrite;
	char MemtoReg = ID_EX.Ctrl_WB_MemtoReg;

	//Branch PC calulate
	if (Branch == 1 || Branch == 2 || Branch == 3 || Branch == 4 || Branch == 5)
		temp_PC = (temp_PC * 4 + Imm) / 4;//SB & JAL	
	else temp_PC = temp_PC;
	//Branch=6 JALR 在后面用到ALU结果更新

	//JAL
	if (Branch == 1) printf("JAL temp_PC : %08llx\n", temp_PC * 4);

	//choose ALU input number
	REG ALU_A = 0;
	REG ALU_B = 0;

	if (ALUSrcA == 0) ALU_A = Reg_Rs1;
	else ALU_A = temp_PC * 4;//used in AUIPC, ALU_A+ALU_B
	if (ALUSrcB == 0) ALU_B = Reg_Rs2;
	else ALU_B = Imm;

	if (Branch == 6)
	{
		//printf("Imm in EX: %08llx\n", Imm);
		//printf("ALU_B in EX: %08llx\n", ALU_B);
	}

	//alu calculate
	char Zero = 1;//the branch flag
	 //if (Branch== (2||3||4||5) ) && (Zero==0) then jump to new PC
	REG ALUout;
	switch (ALUOp)
	{
	case 0:
	{
		ALUout = ALU_A + ALU_B;
		if (Branch == 6)
		{
			//printf("ALU_A : %08llx\n", ALU_A);
			//printf("ALU_B : %08llx\n", ALU_B);
			//printf("ALUout : %08llx\n", ALUout);
		}
		break;
	}
	case 1:
	{
		ALUout = ALU_A*ALU_B;//乘法的低64位
		if (isNop == 0 && (ProcessorMode != 0)) num_cycle += 1;  // In Multiple processor && pipeline
		break;
	}
	case 2:
	{
		//Branch=2: branch if R[rs1]==R[rs2]
		//Branch=3: branch if R[rs1]!=R[rs2]
		//Branch=4: branch if R[rs1]<R[rs2]
		//Branch=5: branch if R[rs1]>=R[rs2]
		Zero = 1;
		ALUout = ext_signed((ALU_A - ALU_B), 1);
		if (ALUout == 0 && Branch == 2) Zero = 0;
		if (ALUout != 0 && Branch == 3) Zero = 0;
		if ((ALUout & (0x8000000000000000)) && Branch == 4) Zero = 0;//the highest bit of ALUout is 1
		if ((!(ALUout & (0x8000000000000000))) && Branch == 5) Zero = 0;

		//printf("ALU_A : %llx\n", ALU_A);
		//printf("ALU_B : %llx\n", ALU_B);
		//printf("ALUout : %llx\n", ALUout);
		//printf("Branch : %llx\n", Branch);
		//printf("Zero : %llx\n", Zero);
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
		if (isNop == 0 && (ProcessorMode != 0)) num_cycle += 1;  // In Multiple processor && pipeline
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
		if (isNop == 0 && (ProcessorMode != 0)) num_cycle += 39;  // In Multiple processor && pipeline
		break;
	}
	case 8:
	{
		ALUout = (unsigned int)ALU_A >> ALU_B;//逻辑右移
		break;
	}
	case 9:
	{
		ALUout = (int)ALU_A >> ALU_B;//算术右移
		break;
	}
	case 10:
	{
		ALUout = ALU_A | ALU_B;
		break;
	}
	case 11:
	{
		ALUout = ALU_A % ALU_B;
		if (isNop == 0 && (ProcessorMode != 0)) num_cycle += 39;  // In Multiple processor && pipeline
		break;
	}
	case 12:
	{
		ALUout = ALU_A & ALU_B;
		break;
	}
	default: ALUout = 0;
	}

	//(ALU result &(~1)) / 4->PC
	if (Branch == 6) temp_PC = (ALUout &(~1)) / 4;//JALR fresh PC number
	//JALR
	//if (Branch==6) printf("JALR ALUout : %llx\n", ALUout);
	//if (Branch==6) printf("JALR temp_PC : %llx\n", temp_PC * 4);	

	bool jump = false;
	jump = ((Branch == 2 || Branch == 3 || Branch == 4 || Branch == 5) && Zero == 0);
	jump = jump || (Branch == 1 || Branch == 6);
	// 流水线下，非Nop指令才有跳转的权利！
	if (ProcessorMode == 2) jump = jump && (isNop == 0);

	//write EX_MEM_old
	EX_MEM_old.PC = old_PC;//保留的是本指令的PC值，不是现在的PC或者temp_PC 
	EX_MEM_old.Jump_PC = temp_PC;
	EX_MEM_old.Rd = Rd;
	EX_MEM_old.ALU_out = ALUout;

	EX_MEM_old.Reg_Rs2 = Reg_Rs2;
	EX_MEM_old.isNop = isNop;
	EX_MEM_old.havePushedRd = havePushedRd;

	EX_MEM_old.Ctrl_M_Branch = Branch;
	EX_MEM_old.Ctrl_M_Zero = Zero;
	EX_MEM_old.Ctrl_M_MemWrite = MemWrite;
	EX_MEM_old.Ctrl_M_MemRead = MemRead;

	EX_MEM_old.Ctrl_WB_RegWrite = RegWrite;
	EX_MEM_old.Ctrl_WB_MemtoReg = MemtoReg;

	//write EX_WB_old
	EX_WB_old.PC = old_PC;//保留的是本指令的PC值，不是现在的PC或者temp_PC 
	EX_WB_old.Jump_PC = temp_PC;
	EX_WB_old.ALU_out = ALUout;
	EX_WB_old.Rd = Rd;
	EX_WB_old.isNop = isNop;
	EX_WB_old.havePushedRd = havePushedRd;
	EX_WB_old.Ctrl_WB_RegWrite = RegWrite;
	EX_WB_old.Ctrl_WB_MemtoReg = MemtoReg;
	
	return jump;
}

//访问存储器
void MEM()
{
	//read EX_MEM
	int old_PC = EX_MEM.PC;//保留跳转指令前的PC值，本指令PC，PC/4，用于之后进行变形后塞到寄存器里
	int Rd = EX_MEM.Rd;//rd值
	REG ALUout = EX_MEM.ALU_out;
	REG Reg_Rs2 = EX_MEM.Reg_Rs2;
	int isNop = EX_MEM.isNop;
	int havePushedRd = EX_MEM.havePushedRd;

	char Branch = EX_MEM.Ctrl_M_Branch;
	char Zero = EX_MEM.Ctrl_M_Zero;
	char MemWrite = EX_MEM.Ctrl_M_MemWrite;
	char MemRead = EX_MEM.Ctrl_M_MemRead;

	char RegWrite = EX_MEM.Ctrl_WB_RegWrite;
	char MemtoReg = EX_MEM.Ctrl_WB_MemtoReg;

	//read / write memory
	unsigned long long int Mem_read;

	if ((ProcessorMode == 0) || (ProcessorMode == 1) || (ProcessorMode == 2 && isNop == 0))
	{
		//MemRead=0: don't read from memory
		//MemRead=1: read from memory, byte
		//MemRead=2: read from memory, half word, 2 byte
		//MemRead=3: read from memory, word, 4 byte
		//MemRead=4: read from memory, double word, 8 byte	
		if (MemRead == 1)
		{
			if (ALUout % 4 == 0) Mem_read = ext_signed(memory[ALUout >> 2] & 0xFF, 1);
			if (ALUout % 4 == 1) Mem_read = ext_signed(memory[ALUout >> 2] & 0xFF00, 1);
			if (ALUout % 4 == 2) Mem_read = ext_signed(memory[ALUout >> 2] & 0xFF0000, 1);
			if (ALUout % 4 == 3) Mem_read = ext_signed(memory[ALUout >> 2] & 0xFF000000, 1);
		}
		else if (MemRead == 2)
		{
			if (ALUout % 4 == 0 || ALUout % 4 == 1) Mem_read = ext_signed(memory[ALUout >> 2] & 0xFFFF, 1);
			if (ALUout % 4 == 2 || ALUout % 4 == 3)  Mem_read = ext_signed(((memory[ALUout >> 2] & 0xFFFF0000) >> 16) & 0xFFFF, 1);
		}
		else if (MemRead == 3)
		{
			Mem_read = ext_signed(memory[ALUout >> 2] & 0xFFFFFFFF, 1);
		}
		else if (MemRead == 4)
		{
			//注意小端法
			//读两个memory
			unsigned long long temp_mem = memory[(ALUout >> 2) + 1];
			Mem_read = memory[ALUout >> 2] + (temp_mem << 32);
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
			unsigned int temp_read = memory[(ALUout >> 2)&(~0x3)];//把ALUout搞成4的倍数，读出4个byte
			unsigned int temp_write = Reg_Rs2 & 0xFF;
			if (ALUout % 4 == 0) memory[(ALUout >> 2)] = (temp_read&(~0xFF)) + temp_write;
			if (ALUout % 4 == 1) memory[(ALUout >> 2)] = (temp_read&(~0xFF00)) + ((temp_write << 8) & 0xFF00);
			if (ALUout % 4 == 2) memory[(ALUout >> 2)] = (temp_read&(~0xFF0000)) + ((temp_write << 16) & 0xFF0000);
			if (ALUout % 4 == 3) memory[(ALUout >> 2)] = (temp_read&(~0xFF000000)) + ((temp_write << 24) & 0xFF000000);
		}
		else if (MemWrite == 2)//
		{
			unsigned int temp_read = memory[(ALUout >> 2)];
			unsigned int temp_write = Reg_Rs2 & 0xFFFF;
			if (ALUout % 4 == 0 || ALUout % 4 == 1) memory[(ALUout >> 2)] = (temp_read&(~0xFFFF)) + temp_write;
			if (ALUout % 4 == 2 || ALUout % 4 == 3) memory[(ALUout >> 2)] =
				(temp_read & 0xFFFF) + ((temp_write << 16) & 0xFFFF0000);
		}
		else if (MemWrite == 3)
		{
			memory[(ALUout >> 2)] = Reg_Rs2 & 0xFFFFFFFF;
		}
		else if (MemWrite == 4)
		{
			//我感觉这是小端的放法，也比划了一下，然而还是可能出bugQAQ
			memory[(ALUout >> 2)] = Reg_Rs2 & 0xFFFFFFFF;
			unsigned long long temp_mask = (0xFFFFFFFF00000000);
			memory[(ALUout >> 2) + 1] = (int)(Reg_Rs2&temp_mask);
		}
	}
	//write MEM_WB_old
	MEM_WB_old.PC = old_PC;//保留的是本指令的PC值，不是现在的PC或者temp_PC 
	MEM_WB_old.Mem_read = Mem_read;
	MEM_WB_old.ALU_out = ALUout;
	MEM_WB_old.Rd = Rd;
	MEM_WB_old.isNop = isNop;
	MEM_WB_old.havePushedRd = havePushedRd;

	MEM_WB_old.Ctrl_WB_RegWrite = RegWrite;
	MEM_WB_old.Ctrl_WB_MemtoReg = MemtoReg;
}

//写回
void WB()
{
	int temp_PC;
	unsigned long long int Mem_read;
	REG ALUout;
	int Rd;
	int isNop;
	int havePushedRd;

	char  RegWrite;
	char MemtoReg;

	if (state == STATE_WB_R)
	{
		//read from EX_WB
		temp_PC = EX_WB.PC;//用于送到rd里面去，是本指令的old_PC值，需要变换之后再放到寄存器里面		
		ALUout = EX_WB.ALU_out;
		Rd = EX_WB.Rd;//rd的值
		isNop = EX_WB.isNop;
		havePushedRd = EX_WB.havePushedRd;
		//RegWrite=0: don't write to register
		//RegWrite=1: write to register	 
		RegWrite = EX_WB.Ctrl_WB_RegWrite;

		//MemtoReg=0: send ALU's result to register
		//MemtoReg=1: send Memory's result to register
		//MemtoReg=2: write old_PC*4+4 to R[rd] in JALR&JALto register
		MemtoReg = EX_WB.Ctrl_WB_MemtoReg;
	}
	//else if (state == STATE_WB_LB)
	else // Suitable for Pipeline
	{
		//read from MEM_WB
		temp_PC = MEM_WB.PC;//用于送到rd里面去，是本指令的old_PC值，需要变换之后再放到寄存器里面
		Mem_read = MEM_WB.Mem_read;
		ALUout = MEM_WB.ALU_out;
		Rd = MEM_WB.Rd;//rd的值
		isNop = MEM_WB.isNop;
		havePushedRd = MEM_WB.havePushedRd;
		//RegWrite=0: don't write to register
		//RegWrite=1: write to register	 
		RegWrite = MEM_WB.Ctrl_WB_RegWrite;

		//MemtoReg=0: send ALU's result to register
		//MemtoReg=1: send Memory's result to register
		//MemtoReg=2: write old_PC*4+4 to R[rd] in JALR&JALto register
		MemtoReg = MEM_WB.Ctrl_WB_MemtoReg;
	}

	//在主模拟器(之前是在EX)里面把PC就写回去啦~

	if ((ProcessorMode == 0) || (ProcessorMode == 1) || (ProcessorMode == 2 && isNop == 0))
	{
		//write reg      
		//啊Rd这里需要个什么东西把rd传过来…
		//前面好像都错了QAQ要么就再加个变量把rd也传下来
		//好吧我试试把rd一直传下来
		if (RegWrite == 1)
		{
			if (MemtoReg == 0) reg[Rd] = ALUout;
			else if (MemtoReg == 1) reg[Rd] = Mem_read;
			else if (MemtoReg == 2) reg[Rd] = temp_PC * 4 + 4;//送到reg里面，所以最后不用/4
			else if (MemtoReg == 3) reg[Rd] = ext_signed(ALUout, 1);
		}
		if (havePushedRd)
		{
			//流水线
			vector<int>::iterator k = RdQueue.begin();
			RdQueue.erase(k);//删除第一个元素
		}
	}
}
