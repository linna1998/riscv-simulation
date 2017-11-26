#include "Simulation.h"
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

// Single step debugging flag.
// If TF == 1: single step debugging.
static bool TF = false;

// ProcessorMode = 0: Single Cycle Processor.
// ProcessorMode = 1: Multiple Cycle Processor.
// ProcessorMode = 2: Pipeline Processor.
static int ProcessorMode = PIPELINE;

// System call return.
#define RET_INST 0x00008067

static char SymbolicName[32][3] = {
	"0", "ra", "sp", "gp", "tp", "t0", "t1", "t2",
	"s0", "s1", "a0", "a1", "a2", "a3", "a4", "a5",
	"a6", "a7", "s2", "s3", "s4", "s5", "s6", "s7",
	"s8", "s9", "sa", "sb", "t3", "t4", "t5", "t6"
};

// Print the registers.
void print_regs()
{
	printf("Register:\n");
	for (int i = 0; i < 32; i++)
	{
		printf("[%2d][%2s]0x%8llx  ", i, SymbolicName[i], reg[i]);
		printf("[%2d][%2s]0x%8llx  ", i, SymbolicName[i], reg[++i]);
		printf("[%2d][%2s]0x%8llx  ", i, SymbolicName[i], reg[++i]);
		printf("[%2d][%2s]0x%8llx\n", i, SymbolicName[i], reg[++i]);
	}
	printf("\n");
}

// Print the stack.
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

// Print the .data section memory.
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

// Print the results. The global variables.
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
	printf("a:        %d\n", memory[a_adr >> 2]);
	printf("b:        %d\n", memory[b_adr >> 2]);
	printf("c:        %d\n", memory[c_adr >> 2]);
	printf("temp:  %d\n", memory[temp_adr >> 2]);
	printf("sum:    %d\n", memory[sum_adr >> 2]);
}

// Print the .text setion memory.
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

// Load the .text section.
void load_text()
{
	if (fseek(file, cadr - 0x10000, 0))
	{
		perror("fseek");
	}
	if (!fread(&memory[cadr >> 2], 1, csize, file))
	{
		perror("fread");
	}
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
	}
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
	}
}

void Init_cache()
{
	CacheConfig_ cc1;
	cc1.s = 6;
	cc1.e = 3;
	cc1.b = 4;
	cc1.write_through = 0;
	cc1.write_allocate = 1;

	CacheConfig_ cc2;
	cc2.s = 9;
	cc2.e = 3;
	cc2.b = 4;
	cc2.write_through = 0;
	cc2.write_allocate = 1;

	CacheConfig_ cc3;
	cc3.s = 14;
	cc3.e = 3;
	cc3.b = 4;
	cc3.write_through = 0;
	cc3.write_allocate = 1;

	l1.SetLower(&l2);
	l2.SetLower(&l3);
	l3.SetLower(&m);
	l1.SetConfig(cc1);
	l2.SetConfig(cc2);
	l3.SetConfig(cc3);
	l1.BuildCache();
	l2.BuildCache();
	l3.BuildCache();

	StorageStats s;
	s.access_counter = 0;
	s.access_time = 0;
	s.fetch_num = 0;
	s.miss_num = 0;
	s.prefetch_num = 0;
	s.replace_num = 0;

	m.SetStats(s);
	l1.SetStats(s);
	l2.SetStats(s);
	l3.SetStats(s);

	StorageLatency ml;
	ml.bus_latency = 0;
	ml.hit_latency = 100;  // 100 cycles
	m.SetLatency(ml);

	StorageLatency ll1, ll2, ll3;
	ll1.bus_latency = 0;
	ll1.hit_latency = 1;  // 1 cycle.
	l1.SetLatency(ll1);
	ll2.bus_latency = 0;
	ll2.hit_latency = 8;
	l2.SetLatency(ll2);
	ll3.bus_latency = 0;
	ll3.hit_latency = 20;
	l3.SetLatency(ll3);
}

void Ana_cache()
{
	StorageStats_ ss;
	l1.GetStats(ss);
	printf(" Analyze on cache l1:\n");
	printf(" Total hit = %d\n", ss.access_counter - ss.miss_num);
	printf(" Total num = %d\n", ss.access_counter);
	printf(" Miss Rate= %f\n", (double)(ss.miss_num) / (double)(ss.access_counter));
	printf(" Total time= %d cycles\n", ss.access_time);
	printf(" Total replacement = %d\n", ss.replace_num);
	printf("\n");

	l2.GetStats(ss);
	printf(" Analyze on cache l2:\n");
	printf(" Total hit = %d\n", ss.access_counter - ss.miss_num);
	printf(" Total num = %d\n", ss.access_counter);
	printf(" Miss Rate= %f\n", (double)(ss.miss_num) / (double)(ss.access_counter));
	printf(" Total time= %d cycles\n", ss.access_time);
	printf(" Total replacement = %d\n", ss.replace_num);
	printf("\n");

	l3.GetStats(ss);
	printf(" Analyze on cache llc:\n");
	printf(" Total hit = %d\n", ss.access_counter - ss.miss_num);
	printf(" Total num = %d\n", ss.access_counter);
	printf(" Miss Rate= %f\n", (double)(ss.miss_num) / (double)(ss.access_counter));
	printf(" Total time= %d cycles\n", ss.access_time);
	printf(" Total replacement = %d\n", ss.replace_num);
	printf("\n");

	m.GetStats(ss);
	printf(" Analyze on memory:\n");
	printf(" Total hit = %d\n", ss.access_counter - ss.miss_num);
	printf(" Total num = %d\n", ss.access_counter);
	printf(" Miss Rate= %f\n", (double)(ss.miss_num) / (double)(ss.access_counter));
	printf(" Total time= %d cycles\n", ss.access_time);
	printf(" Total replacement = %d\n", ss.replace_num);
	printf("\n");
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
				cout << "  --SF, --MF, --TF : The simulator's modules." << endl;
				cout << "           --SF means single cycle processor." << endl;
				cout << "           --MF means multiple cycle processor." << endl;
				cout << "           --PF means pipelin processor." << endl;
				cout << "           Pipeline processor by default." << endl;
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
				ProcessorMode = SINGLE;
			}
			else if (strcmp(argv[i], "--MF") == 0)
			{
				cout << "Multiple cycle processor." << endl;
				cout << endl;
				ProcessorMode = MULTI;
			}
			else if (strcmp(argv[i], "--PF") == 0)
			{
				cout << "Pipeline processor." << endl;
				cout << endl;
				ProcessorMode = PIPELINE;
			}
			else
			{
				cout << "ERROR:  Unknown flags. See --help." << endl;
				return 0;
			}
		}
	}
	// Initialize the cache.
	Init_cache();

	// Read the .elf file.
	cout << "OPEN FILE: " << filename << endl << endl;
	file = fopen(filename, "r+");
	if (!file)
	{
		perror("Open executable file");
		return false;
	}
	read_elf();

	// Load the memory.
	load_text();
	load_data();
	load_sdata();
	fclose(file);

	// Set the PC entry.
	global_PC = entry >> 2;

	// Set gp register.
	reg[3] = gp;

	reg[2] = MAX / 2;  // Set sp register for stack.

	cout << "[!] Simulation starts!" << endl;
	printf("===============================================================================\n");

	if (ProcessorMode == SINGLE)
		SingleCycleProcessor();
	else if (ProcessorMode == MULTI)
		MultiCycleProcessor();
	else if (ProcessorMode == PIPELINE)
		PipelineProcessor();

	printf("===============================================================================\n");
	printf("[!] Simulation is over!\n");
	printf(" Instruction Number : %d\n", num_inst);
	printf(" Cycle Number       : %d\n", num_cycle);
	if (ProcessorMode == PIPELINE)
	{
		printf(" Branch_nop_cycle   : %d\n", num_branch_nop);
		printf(" Data_nop_cycle     : %d\n", num_data_nop);
	}
	printf(" CPI                : %f\n", num_cycle / (float)(num_inst));
	printf("\n");
	Ana_cache();	
	printf("Please press Enter to continue...\n");

	getchar();

	return 0;
}

void SingleCycleProcessor()
{
	bool jump = false;
	num_cycle = 0;
	num_inst = 0;
	while ((global_PC * 4) != endPC)
	{
		if (TF)
		{
			printf("[TF] Instruction:  %08x\n", memory[global_PC]);
			printf("PC              :  %08X\n", global_PC * 4);
		}
		// Add instruction number.
		num_inst++;
		num_cycle++;

		IF();
		// Refresh the registers.
		IF_ID = IF_ID_old;
		ID();
		ID_EX = ID_EX_old;
		jump = EX();
		EX_MEM = EX_MEM_old;
		MEM();
		MEM_WB = MEM_WB_old;
		WB();

		if (jump)
		{
			global_PC = EX_MEM.Jump_PC;  // Jump. Set the new PC.
		}

		reg[0] = 0;

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
	bool jump = false;
	state = STATE_IF;  // Initialize the state.
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
		// Run.
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
			jump = EX();
			// Jump for JAL JALR. 
			EX_WB = EX_WB_old;
			if (jump)
			{
				global_PC = EX_WB.Jump_PC;  // Jump. Set new PC.
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
			jump = EX();
			EX_MEM = EX_MEM_old;
			if (jump)
			{
				global_PC = EX_MEM.Jump_PC;  // Jump. Set new PC.
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
		// Add instruction number.
		if (temp_state == STATE_IF)
		{
			num_inst++;
		}
		// Add cycle number.
		num_cycle += 1;

		reg[0] = 0;

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

void PipelineProcessor()
{
	// Initialize.
	num_cycle = 0;
	num_inst = 0;
	num_branch_nop = 0;
	num_data_nop = 0;

	bool conflict = false;
	bool jump = false;
	int temp_IF_PC = 0;

	RdNode new_RdNode;
	new_RdNode.valid = false;
	new_RdNode.Rd = -1;
	new_RdNode.value = -1;

	for (int i = 0; i < 3; i++)
	{
		RdQueue.push_back(new_RdNode);
	}
	ID_EX.havePushedRd = 1;
	EX_MEM.havePushedRd = 1;
	EX_WB.havePushedRd = 1;
	MEM_WB.havePushedRd = 1;

	while ((MEM_WB.PC * 4) != endPC)
	{
		num_cycle++;
		if (MEM_WB.isNop == 0) num_inst++;
		temp_IF_PC = global_PC;
		if (TF)
		{
			printf("[TF] Instruction:  %08x.\n", memory[global_PC]);
			printf("PC_IF             :  %08X.\n", global_PC * 4);
			printf("PC_ID             :  %08X.\n", IF_ID.PC * 4);
			printf("PC_EX             :  %08X.\n", ID_EX.PC * 4);
			printf("PC_MEM         :  %08X.\n", EX_MEM.PC * 4);
			printf("PC_WB           :  %08X.\n", MEM_WB.PC * 4);

			//// Debug
			//printf("[TF] Instruction:  %08x\n", memory[global_PC]);		
			//printf("PC_IF             :  %08X.\n", global_PC * 4);
			//printf("PC_ID             :  %08X, isNop  %d\n", IF_ID.PC * 4, IF_ID.isNop);
			//printf("PC_EX             :  %08X, isNop  %d, havePushedRd  %d\n",
			//	ID_EX.PC * 4, ID_EX.isNop, ID_EX.havePushedRd);
			//printf("PC_MEM         :  %08X, isNop  %d, havePushedRd  %d\n",
			//	EX_MEM.PC * 4, EX_MEM.isNop, EX_MEM.havePushedRd);
			//printf("PC_WB           :  %08X, isNop  %d, havePushedRd  %d\n",
			//	MEM_WB.PC * 4, MEM_WB.isNop, MEM_WB.havePushedRd);
		}

		IF();
		conflict = ID();
		// If EX() returns true, then have t jump.
		jump = EX();
		MEM();
		WB();

		// PC jump		
		if (jump)
		{
			global_PC = EX_MEM_old.Jump_PC;  // Set new address.
			IF_ID_old.isNop = 1;
			ID_EX_old.isNop = 1;
			num_branch_nop += 2;
			if (ID_EX_old.havePushedRd == 1)
			{
				RdQueue.pop_back();
				ID_EX_old.havePushedRd = 0;
			}
		}
		else if (conflict)
		{
			global_PC = temp_IF_PC;  // Get the old PC address.
			IF_ID_old.isNop = 1;
			ID_EX_old.isNop = 1;
			num_data_nop++;
			if (ID_EX_old.havePushedRd == 1)
			{
				RdQueue.pop_back();
				ID_EX_old.havePushedRd = 0;
			}
		}

		// refresh the registers.
		if (!(conflict && !jump)) IF_ID = IF_ID_old;
		ID_EX = ID_EX_old;
		EX_MEM = EX_MEM_old;
		MEM_WB = MEM_WB_old;

		reg[0] = 0;

		if (TF)
		{
			print_regs();
			print_stack();
			print_result();
			getchar();
		}
	}
	num_inst = num_inst - 3;  // The first three nop instructions.
	printf("===============================================================================\n");
	printf("[!] Return from main function.\n");
	print_regs();
	print_stack();
	print_result();
}


// 32 bits instruction.
// 64 bits register.

// Instruction fetch.
void IF()
{
	//write IF_ID_old
	IF_ID_old.inst = memory[global_PC];
	IF_ID_old.PC = global_PC;  // Store the PC of this instruction.
	global_PC = global_PC + 1;  //int memory[], so PC+1 means add 4 bytes	
	IF_ID_old.isNop = 0;
}

// INstruction decode.
// return true means data conflict in pipeline.
bool ID()
{
	//Read IF_ID
	unsigned int inst = IF_ID.inst;
	unsigned int inst_PC = IF_ID.PC;  // The PC of this instruction.
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
	//Branch=1: (inst_PC*4+IMM*2)/4->PC, choose to branch in JAL
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
	//MemtoReg=2: write inst_PC*4+4 to R[rd] in JALR&JALto register
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

	int next_state = STATE_EX_R;  // Default setting.

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
	//WB: inst_PC*4+4 -> R[rd], (ALU result*2)/4->PC
	else if (OP == OP_JALR)
	{
		next_state = STATE_EX_R;
		//R[rd] ←inst_PC*4+4
		//PC ← { (R[rs1] + imm), 1b'0} /4
		if (fuc3 == F3_JALR)
		{
			EXTop = 1;//sign extend
			EXTsrc = imm_I;
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
		//Transfers control to operating system.
		//a0 = 1 is print value of a1 as an integer.
		//a0 = 10 is exit or end of code indicator
		if (fuc3 == F3_ECALL && fuc7 == F7_ECALL)
		{
			printf("ECALL. Illegal instruction.\n");
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
		//R[rd] ← inst_PC*4 + 4
		//PC ← (inst_PC *4 + {imm, 1b'0} )/4 //finished
		next_state = STATE_EX_R;
		EXTop = 1;//sign extend
		EXTsrc = ext_signed(imm_UJ, 1);
		rs1 = 0;//reg[0]=0
		ALUop = 0;
		ALUSrcA = 0;
		ALUSrcB = 1;
		Branch = 1;//(inst_PC*4+EXTsrc)/4->PC
		MemRead = 0;
		MemWrite = 0;
		RegWrite = 1;
		MemtoReg = 2;
	}

	// Set the state in multiple cycle processor.
	if (ProcessorMode == MULTI) state = next_state;

	//write ID_EX_old
	ID_EX_old.Rd = rd;
	ID_EX_old.PC = inst_PC;  // The PC of this instruction.
	ID_EX_old.Imm = ext_signed(EXTsrc, EXTop);
	ID_EX_old.Reg_Rs1 = reg[rs1];  // Get the number in register in decode step.
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

	if (isNop == 0 && ProcessorMode == PIPELINE)
	{
		// Judge hazard.
		// The last element in RdQueue with index rs1.
		int index_Rs1 = RdQueue.size() - 1, index_Rs2 = RdQueue.size() - 1;
		while (index_Rs1 > 0 && RdQueue[index_Rs1].Rd != rs1)
			index_Rs1--;
		while (index_Rs2 > 0 && RdQueue[index_Rs2].Rd != rs2)
			index_Rs2--;
		if (ALUSrcA == 0 && RdQueue[index_Rs1].Rd == rs1)
		{
			if (RdQueue[index_Rs1].valid == true)
			{
				ID_EX_old.Reg_Rs1 = RdQueue[index_Rs1].value;
			}
			else
			{
				return true;
			}
		}
		// ALU_A ALU_B all supports data forwarding. !!! Debug for hours !!!!
		if ((ALUSrcB == 0 || OP == OP_S) && RdQueue[index_Rs2].Rd == rs2)
		{
			if (RdQueue[index_Rs2].valid == true)
			{
				ID_EX_old.Reg_Rs2 = RdQueue[index_Rs2].value;
			}
			else
			{
				return true;
			}
		}

		// No hazard. 		
		// Push Rd into RdQueue if it writes Rd.
		RdNode new_RdNode;
		if (RegWrite == 1)
		{
			new_RdNode.Rd = rd;
			new_RdNode.value = -1;
			new_RdNode.valid = false;
			RdQueue.push_back(new_RdNode);
			for (int j = 0; j < RdQueue.size(); j++)
			{
				if (rd == RdQueue[j].Rd)
				{
					RdQueue[j].valid = false;
				}
			}
			ID_EX_old.havePushedRd = 1;
		}
		// Else, push back -1.
		else
		{
			new_RdNode.Rd = -1;
			new_RdNode.value = -1;
			new_RdNode.valid = false;
			RdQueue.push_back(new_RdNode);
			ID_EX_old.havePushedRd = 1;
		}
	}
	return false;
}

// Execute.
// If it returns true, then have to jump.
bool EX()
{
	//read ID_EX	
	int Rd = ID_EX.Rd;
	int temp_PC = ID_EX.PC;   // The PC of this instruction.
	int inst_PC = ID_EX.PC;  // The PC of this instruction.
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

	//choose ALU input number
	REG ALU_A = 0;
	REG ALU_B = 0;

	if (ALUSrcA == 0) ALU_A = Reg_Rs1;
	else ALU_A = inst_PC * 4;  //used in AUIPC, ALU_A+ALU_B
	if (ALUSrcB == 0) ALU_B = Reg_Rs2;
	else ALU_B = Imm;


	// Alu calculation.
	char Zero = 1;  // The branch flag.
	 // Jump to new PC if (Branch== (2||3||4||5) ) && (Zero==0).
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
		ALUout = ALU_A*ALU_B;  // The lowest 64 bits of multiply.
		if ((ProcessorMode == MULTI) || (isNop == 0 && ProcessorMode == PIPELINE))
			num_cycle += 39;  // In Multiple processor && pipeline
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
		if ((ALUout & (0x8000000000000000)) && Branch == 4) Zero = 0;  // The highest bit of ALUout is 1.
		if ((!(ALUout & (0x8000000000000000))) && Branch == 5) Zero = 0;

		break;
	}
	case 3:
	{
		ALUout = ALU_A << ALU_B;
		break;
	}
	case 4:
	{
		// The highest 64 bits of multiply.
		long long A_High = ((ALU_A & 0xFFFF0000) >> 32) & 0xFFFF;
		long long A_Low = ALU_A & 0x0000FFFF;
		long long B_High = ((ALU_B & 0xFFFF0000) >> 32) & 0xFFFF;
		long long B_Low = ALU_B & 0x0000FFFF;

		long long AHBH = A_High*B_High;
		long long AHBL = ((A_High*B_Low) >> 32) & 0xFFFF;
		long long ALBH = ((A_Low*B_High) >> 32) & 0xFFFF;

		ALUout = AHBH + AHBL + ALBH;
		if ((ProcessorMode == MULTI) || (isNop == 0 && ProcessorMode == PIPELINE))
			num_cycle += 39;  // In Multiple processor && pipeline
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
		if ((ProcessorMode == MULTI) || (isNop == 0 && ProcessorMode == PIPELINE))
			num_cycle += 39;  // In Multiple processor && pipeline
		break;
	}
	case 8:
	{
		ALUout = (unsigned int)ALU_A >> ALU_B;  // Logical shift right.
		break;
	}
	case 9:
	{
		ALUout = (int)ALU_A >> ALU_B;  // Arithmetic shift right.
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
		if ((ProcessorMode == MULTI) || (isNop == 0 && ProcessorMode == PIPELINE))
			num_cycle += 39;  // In Multiple processor && pipeline
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
	if (Branch == 6) temp_PC = (ALUout &(~1)) / 4;//JALR fresh tmep_ PC number

	bool jump = false;
	jump = ((Branch == 2 || Branch == 3 || Branch == 4 || Branch == 5) && Zero == 0);
	jump = jump || (Branch == 1 || Branch == 6);
	if (ProcessorMode == PIPELINE) jump = jump && (isNop == 0);

	// ALU Calculation's data forwarding.
	if (ProcessorMode == PIPELINE && isNop == 0 && RegWrite == 1)
	{
		for (int i = 0; i < RdQueue.size(); i++)
		{
			if (RdQueue[i].Rd == Rd && RdQueue[i].valid == false)
			{
				if (MemtoReg == 0) RdQueue[i].value = ALUout;
				else if (MemtoReg == 2)  RdQueue[i].value = inst_PC * 4 + 4;
				else if (MemtoReg == 3)  RdQueue[i].value = ext_signed(ALUout, 1);
				if (MemtoReg == 0 || MemtoReg == 2 || MemtoReg == 3)
				{
					RdQueue[i].valid = true;
					break;  // Write into the oldest value;
				}
			}
		}
	}
	//write EX_MEM_old
	EX_MEM_old.PC = inst_PC;  // The PC of this instruction.
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
	// Used in multiple cycle processor.
	EX_WB_old.PC = inst_PC;  // The PC of this instruction.
	EX_WB_old.Jump_PC = temp_PC;
	EX_WB_old.ALU_out = ALUout;
	EX_WB_old.Rd = Rd;
	EX_WB_old.isNop = isNop;
	EX_WB_old.havePushedRd = havePushedRd;
	EX_WB_old.Ctrl_WB_RegWrite = RegWrite;
	EX_WB_old.Ctrl_WB_MemtoReg = MemtoReg;

	return jump;
}

// Memory.
void MEM()
{
	//read EX_MEM
	int inst_PC = EX_MEM.PC;   // The PC of this instruction.
	int Rd = EX_MEM.Rd;
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

	int read;
	int addr = 0;
	char content[32];
	int hit = 0, time = 0;

	if ((ProcessorMode == SINGLE) || (ProcessorMode == MULTI) ||
		(ProcessorMode == PIPELINE && isNop == 0))
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
			if (ALUout % 4 == 0 || ALUout % 4 == 1)
				Mem_read = ext_signed(memory[ALUout >> 2] & 0xFFFF, 1);
			if (ALUout % 4 == 2 || ALUout % 4 == 3)
				Mem_read = ext_signed(((memory[ALUout >> 2] & 0xFFFF0000) >> 16) & 0xFFFF, 1);
		}
		else if (MemRead == 3)
		{
			Mem_read = ext_signed(memory[ALUout >> 2] & 0xFFFFFFFF, 1);
		}
		else if (MemRead == 4)
		{
			// Small end.
			// Read in 8 bytes from memory.
			unsigned long long temp_mem = memory[(ALUout >> 2) + 1];
			Mem_read = memory[ALUout >> 2] + (temp_mem << 32);
		}

		// Simulate Cache.
		if (MemRead == 1 || MemRead == 2 || MemRead == 3)
		{
			read = 1;
			addr = ALUout >> 2;
			l1.HandleRequest(addr, 4, read, content, hit, time);
		}
		else if (MemRead == 4)
		{
			read = 1;
			addr = ALUout >> 2;
			l1.HandleRequest(addr, 4, read, content, hit, time);
			addr = ALUout >> 2 + 1;
			l1.HandleRequest(addr, 4, read, content, hit, time);
		}

		//write to memory
		//MemWrite=0: don't write to memory
		//MemWrite=1: write to memory, byte, [7:0]
		//MemWrite=2: write to memory, half word[15:0]
		//MemWrite=3: write to memory, word[31:0]
		//MemWrite=4: write to memory, double word[63:0]	
		if (MemWrite == 1)
		{
			unsigned int temp_read = memory[(ALUout >> 2)&(~0x3)];  // Read in 4 bytes.
			unsigned int temp_write = Reg_Rs2 & 0xFF;
			if (ALUout % 4 == 0) memory[(ALUout >> 2)] =
				(temp_read&(~0xFF)) + temp_write;
			if (ALUout % 4 == 1) memory[(ALUout >> 2)] =
				(temp_read&(~0xFF00)) + ((temp_write << 8) & 0xFF00);
			if (ALUout % 4 == 2) memory[(ALUout >> 2)] =
				(temp_read&(~0xFF0000)) + ((temp_write << 16) & 0xFF0000);
			if (ALUout % 4 == 3) memory[(ALUout >> 2)] =
				(temp_read&(~0xFF000000)) + ((temp_write << 24) & 0xFF000000);
		}
		else if (MemWrite == 2)
		{
			unsigned int temp_read = memory[(ALUout >> 2)];
			unsigned int temp_write = Reg_Rs2 & 0xFFFF;
			if (ALUout % 4 == 0 || ALUout % 4 == 1) memory[(ALUout >> 2)] =
				(temp_read&(~0xFFFF)) + temp_write;
			if (ALUout % 4 == 2 || ALUout % 4 == 3) memory[(ALUout >> 2)] =
				(temp_read & 0xFFFF) + ((temp_write << 16) & 0xFFFF0000);
		}
		else if (MemWrite == 3)
		{
			memory[(ALUout >> 2)] = Reg_Rs2 & 0xFFFFFFFF;
		}
		else if (MemWrite == 4)
		{
			memory[(ALUout >> 2)] = Reg_Rs2 & 0xFFFFFFFF;
			unsigned long long temp_mask = (0xFFFFFFFF00000000);
			memory[(ALUout >> 2) + 1] = (int)(Reg_Rs2&temp_mask);
		}

		// Simulate Cache.
		if (MemWrite == 1 || MemWrite == 2 || MemWrite == 3)
		{
			read = 0;
			addr = ALUout >> 2;
			l1.HandleRequest(addr, 4, read, content, hit, time);
		}
		else if (MemWrite == 4)
		{
			read = 0;
			addr = ALUout >> 2;
			l1.HandleRequest(addr, 4, read, content, hit, time);
			addr = ALUout >> 2 + 1;
			l1.HandleRequest(addr, 4, read, content, hit, time);
		}

		// Pipeline, update clock cycle.
		if (ProcessorMode == PIPELINE && isNop == 0)
		{
			if (MemRead != 0 || MemWrite != 0)
				num_cycle += time - 1;
		}
	}
	// Load's data forwarding.
	if (ProcessorMode == PIPELINE && isNop == 0 && RegWrite == 1)
	{
		for (int i = 0; i < RdQueue.size(); i++)
		{
			if (RdQueue[i].Rd == Rd && RdQueue[i].valid == false)
			{
				if (MemtoReg == 1)
				{
					RdQueue[i].value = Mem_read;
					RdQueue[i].valid = true;
					break;  // Write into the oldest value.
				}
			}
		}
	}

	//write MEM_WB_old
	MEM_WB_old.PC = inst_PC;
	MEM_WB_old.Mem_read = Mem_read;
	MEM_WB_old.ALU_out = ALUout;
	MEM_WB_old.Rd = Rd;
	MEM_WB_old.isNop = isNop;
	MEM_WB_old.havePushedRd = havePushedRd;

	MEM_WB_old.Ctrl_WB_RegWrite = RegWrite;
	MEM_WB_old.Ctrl_WB_MemtoReg = MemtoReg;
}

// Write back.
void WB()
{
	int inst_PC;
	unsigned long long int Mem_read;
	REG ALUout;
	int Rd;
	int isNop;
	int havePushedRd;

	char  RegWrite;
	char MemtoReg;

	if (state == STATE_WB_R && ProcessorMode == MULTI)
	{
		//read from EX_WB
		inst_PC = EX_WB.PC;
		ALUout = EX_WB.ALU_out;
		Rd = EX_WB.Rd;
		isNop = EX_WB.isNop;
		havePushedRd = EX_WB.havePushedRd;
		//RegWrite=0: don't write to register
		//RegWrite=1: write to register	 
		RegWrite = EX_WB.Ctrl_WB_RegWrite;

		//MemtoReg=0: send ALU's result to register
		//MemtoReg=1: send Memory's result to register
		//MemtoReg=2: write inst_PC*4+4 to R[rd] in JALR&JALto register
		MemtoReg = EX_WB.Ctrl_WB_MemtoReg;
	}
	else // Suitable for Pipeline
	{
		//read from MEM_WB
		inst_PC = MEM_WB.PC;
		Mem_read = MEM_WB.Mem_read;
		ALUout = MEM_WB.ALU_out;
		Rd = MEM_WB.Rd;
		isNop = MEM_WB.isNop;
		havePushedRd = MEM_WB.havePushedRd;
		//RegWrite=0: don't write to register
		//RegWrite=1: write to register	 
		RegWrite = MEM_WB.Ctrl_WB_RegWrite;

		//MemtoReg=0: send ALU's result to register
		//MemtoReg=1: send Memory's result to register
		//MemtoReg=2: write inst_PC*4+4 to R[rd] in JALR&JALto register
		MemtoReg = MEM_WB.Ctrl_WB_MemtoReg;
	}

	if ((ProcessorMode == SINGLE) || (ProcessorMode == MULTI) ||
		(ProcessorMode == PIPELINE && isNop == 0))
	{
		// Write to register.      
		if (RegWrite == 1)
		{
			if (MemtoReg == 0) reg[Rd] = ALUout;
			else if (MemtoReg == 1) reg[Rd] = Mem_read;
			else if (MemtoReg == 2) reg[Rd] = inst_PC * 4 + 4;
			else if (MemtoReg == 3) reg[Rd] = ext_signed(ALUout, 1);
		}
		if (havePushedRd && ProcessorMode == PIPELINE)
		{
			vector<RdNode>::iterator k = RdQueue.begin();
			RdQueue.erase(k);// Delete the first element in RdQueue.
		}
	}
}
