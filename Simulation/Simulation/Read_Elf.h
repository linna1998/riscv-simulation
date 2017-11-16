#include<stdio.h>
#include<string.h>
#include<iostream>
typedef struct {
	unsigned char b[8];
}int64;

typedef struct {
	unsigned char b[4];
}int32;

typedef struct {
	unsigned char b[2];
}int16;

typedef struct {
	unsigned char b;
}int8;

typedef unsigned long long Elf64_Addr;
typedef unsigned long long Elf64_Off;
typedef unsigned long long Elf64_Xword;
typedef unsigned long long Elf64_Sxword;
typedef unsigned int Elf64_Word;
typedef unsigned int Elf64_Sword;
typedef unsigned short Elf64_Half;


#define	EI_CLASS 4
#define	EI_DATA 5
#define	EI_VERSION 6
#define	EI_OSABI 7
#define	EI_ABIVERSION 8
#define	EI_PAD 9
#define	EI_NIDENT 16


/** EI_CLASS */
#define ELFCLASSNONE 0  // Invalid class
#define ELFCLASS32 1  // 32-bit objects
#define ELFCLASS64 2  // 64-bit objects

/** EI_DATA */
#define ELFDATANONE 0  // Invalid data encoding
#define ELFDATALSB 1  // Little endian, the high bit is in low address
#define ELFDATAMSB 2  // Big endian, the high bit is in high address

/** e_type */
#define ET_NONE 0  // No file type
#define ET_REL 1  // Relocatable file
#define ET_EXEC 2  // Executable file
#define ET_DYN 3  // Shared object file
#define ET_CORE 4  // Core file

/** EI_VERSION */
#define EV_NONE 0  // Invalid version
#define EV_CURRENT 1  // Current version

/** e_machine */
#define EM_NONE 0  // No machine
#define EM_M32 1  // AT&T WE 32100

/** sh_type */
#define SHT_NULL 0  // Marks the section header as inactive
#define SHT_PROGBITS 1
#define SHT_SYMTAB 2  // Hold a symbol table. Provides symblos for link editing
#define SHT_STRTAB 3
#define SHT_RELA 4
#define SHT_HASH 5
#define SHT_DYNAMIC 6
#define SHT_NOTE 7
#define SHT_NOBITS 8
#define SHT_REL 9
#define SHT_SHLIB 10
#define SHT_DYNSYM 11
#define SHT_LOPROC 0x70000000
#define SHT_HIPROC 0x7fffffff
#define SHT_LOUSER 0x80000000
#define SHT_HIUSER 0xffffffff

/** Segment types */
#define PT_NULL 0
#define PT_LOAD	1

/** sh_flags */
#define SHF_WRITE 0x1
#define SHF_ALLOC 0x2
#define SHF_EXECINSTR 0x4
#define SHF_RELA_LIVEPATCH 0x00100000
#define SHF_RO_AFTER_INIT 0x00200000
#define SHF_MASKPROC 0xf0000000

/** st_info */
#define ELF32_ST_BIND(i) ((i)>>4)
#define ELF32_ST_TYPE(i) ((i)&0xf)

/** stb bind type */
#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_LOPROC 13
#define STB_HIPROC 15

/** ELF32_ST_TYPE */
#define STT_NOTYPE 0
#define STT_OBJECT 1
#define STT_FUNC 2
#define STT_SECTION 3
#define STT_FILE 4
#define STT_LOPROC 13
#define STT_HIPROC 15

#define	SHN_UNDEF 0
#define	SHN_LOPROC 0xFF00
#define	SHN_HIPROC 0xFF1F
#define	SHN_LOOS 0xFF20
#define	SHN_HIOS 0xFF3F
#define	SHN_ABS 0xFFF1
#define	SHN_COMMON 0xFFF2


typedef struct
{
	unsigned char e_ident[16]; /* ELF identification */
	Elf64_Half e_type; /* Object file type */
	Elf64_Half e_machine; /* Machine type */
	Elf64_Word e_version; /* Object file version */
	Elf64_Addr e_entry; /* Entry point address */
	Elf64_Off e_phoff; /* Program header offset */
	Elf64_Off e_shoff; /* Section header offset */
	Elf64_Word e_flags; /* Processor-specific flags */
	Elf64_Half e_ehsize; /* ELF header size */
	Elf64_Half e_phentsize; /* Size of program header entry */
	Elf64_Half e_phnum; /* Number of program header entries */
	Elf64_Half e_shentsize; /* Size of section header entry */
	Elf64_Half e_shnum; /* Number of section header entries */
	Elf64_Half e_shstrndx; /* Section name string table index */
} Elf64_Ehdr;

typedef struct
{
	Elf64_Word sh_name; /* Section name */
	Elf64_Word sh_type; /* Section type */
	Elf64_Xword sh_flags; /* Section attributes */
	Elf64_Addr sh_addr; /* Virtual address in memory */
	Elf64_Off sh_offset; /* Offset in file */
	Elf64_Xword sh_size; /* Size of section */
	Elf64_Word sh_link; /* Link to other section */
	Elf64_Word sh_info; /* Miscellaneous information */
	Elf64_Xword sh_addralign; /* Address alignment boundary */
	Elf64_Xword sh_entsize; /* Size of entries, if section has table */
} Elf64_Shdr;

typedef struct
{
	Elf64_Word st_name; /* Symbol name */
	unsigned char st_info; /* Type and Binding attributes */
	unsigned char st_other; /* Reserved */
	Elf64_Half st_shndx; /* Section table index */
	Elf64_Addr st_value; /* Symbol value */
	Elf64_Xword st_size; /* Size of object (e.g., common) */
} Elf64_Sym;


typedef struct
{
	Elf64_Word p_type; /* Type of segment */
	Elf64_Word p_flags; /* Segment attributes */
	Elf64_Off p_offset; /* Offset in file */
	Elf64_Addr p_vaddr; /* Virtual address in memory */
	Elf64_Addr p_paddr; /* Reserved */
	Elf64_Xword p_filesz; /* Size of segment in file */
	Elf64_Xword p_memsz; /* Size of segment in memory */
	Elf64_Xword p_align; /* Alignment of segment */
} Elf64_Phdr;


void read_elf();
void read_Elf_header();
void read_elf_sections();
void read_symtable();
void read_Phdr();


//代码段在解释文件中的偏移地址
unsigned long long cadr = 0;  // 在Read_Elf.cpp中设置，在Simulation.cpp中加载代码段使用
							  //代码段的长度
unsigned long long csize = 0;  // 在Read_Elf.cpp中设置，在Simulation.cpp中加载代码段使用

							   //全局数据段在内存的地址
unsigned long long dadr = 0;
unsigned long long dsize = 0;
unsigned long long sdadr = 0;
unsigned long long sdsize = 0;

unsigned long long radr = 0;
unsigned long long rsize = 0;

//代码段在内存中的虚拟地址
//因为决定在memory数组中代码地址和elf中相同，所以vadr = cadr
// unsigned int vadr = 0x1000;

//main函数在虚存中的地址，不是ELF文件中的地址，多了4kalign
unsigned long long main_adr = 0;  // 在Read_Elf.cpp中设置

unsigned long long gp = 0;

//程序结束时的PC
unsigned int endPC = 0;  // 不知道该怎么用

						 //程序的入口地址(main)
unsigned int entry = 0;   // 在Read_Elf.cpp中设置，在Simulation.cpp中设置初始PC

FILE *file = NULL;
