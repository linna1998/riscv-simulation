#pragma once
#include<iostream>
#include<stdio.h>
#include<math.h>

#include<time.h>
#include<stdlib.h>
#include"Reg_def.h"

//R type
#define OP_R 51

#define F3_ADD 0
#define F7_ADD 0
#define F7_MUL 0x01
#define F7_SUB 0x20

#define F3_SLL 0x1
#define F7_SLL 0x01
#define F7_MULH 0x01
#define F3_SLT 0x2
#define F7_SLT 0

#define F3_XOR 0x4
#define F7_XOR 0
#define F7_DIV 0x1
#define F3_SRL 0x5
#define F7_SRL 0
#define F7_SRA 0x20

#define F3_OR 0x6
#define F7_OR 0
#define F7_REM 0x1
#define F3_AND 0x7
#define F7_AND 0

// RW Type
#define OP_ADDW 0x3B
#define F3_ADDW 0
#define F7_ADDW 0
#define F7_MULW 1

//I type 1
#define OP_LB 3
#define F3_LB 0
#define F3_LH 1
#define F3_LW 2
#define F3_LD 3

//I type 2
#define OP_ADDI 0x13
#define F3_ADDI 0
#define F3_SLLI 1
#define F7_SLLI 0
#define F3_SLTI 0x2
#define F3_XORI 0x4
#define F3_SRLI 0x5
#define F7_SRLI 0
#define F7_SRAI 0x20
#define F3_ORI 0x6
#define F3_ANDI 0x7

//I type 3
#define OP_ADDIW 0x1B
#define F3_ADDIW 0
#define OP_JALR 0x67
#define F3_JALR 0
#define OP_ECALL 0x73
#define F3_ECALL 0
#define F7_ECALL 0

//S type
#define OP_S 0x23
#define F3_SB 0
#define F3_SH 1
#define F3_SW 2
#define F3_SD 3

//SB type
#define OP_SB 0x63
#define F3_BEQ 0
#define F3_BNE 1
#define F3_BLT 4
#define F3_BGE 5

//U type
#define OP_AUIPC 0x17
#define OP_LUI 0x37

//UJ type
#define OP_JAL 0x6f

#define MAX 0x10000000

//主存
unsigned int memory[MAX] = { 0 };
//寄存器堆
REG reg[32] = { 0 };
//PC
int PC = 0;


//各个指令解析段
unsigned int OP = 0;
unsigned int fuc3 = 0, fuc7 = 0;
int shamt = 0;
int rs1 = 0, rs2 = 0, rd = 0;
unsigned int imm_I = 0;//I
unsigned int imm_S = 0;//S
unsigned int imm_SB = 0;//SB
unsigned int imm_U = 0;//U
unsigned int imm_UJ = 0;//UJ

void load_memory();//加载内存

void simulate();

void IF();

void ID();

void EX();

void MEM();

void WB();

//符号扩展
long long int ext_signed(unsigned int src, int bit);

//获取指定位
unsigned int getbit(unsigned inst, int s, int e);

unsigned int getbit(unsigned inst, int s, int e)
//get from s bit to e bit in inst
//change it into a number
//the lowest bit is 0
//function being checked, correct!
{
	int mask = 0;
	for (int i = s; i <= e; i++)
		mask += (1 << i);
	inst = inst&mask;
	inst = (((int)inst) >> s);
	return inst;
}

long long int ext_signed(unsigned int src, int bit)
//是扩到64位吗？应该是的23333
//看了下，都是32位扩到64位呀~
{
	//bit=0: Zero extend
	//bit=1: sign extend
	long long int result = src;
	long long int mask = (1 << 31);
	if ((result&mask) && (bit == 0)) result = result & 0xFFFFFFFF;

	if (result&mask)
	{
		for (int i = 63; i >= 32; i--) result |= ((long long int)1 << i);
	}

	return result;
}
#pragma once
