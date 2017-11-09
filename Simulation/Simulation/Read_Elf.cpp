#include"Read_Elf.h"

FILE *elf = NULL;
Elf64_Ehdr elf64_hdr;

//Program headers
unsigned int padr = 0;
unsigned int pnum = 0;

//Section Headers
unsigned int sadr = 0;
unsigned int snum = 0;

//Symbol table
unsigned long long symtab_off = 0;
unsigned long long symtab_size = 0;

//String table 字符串表在文件中地址，其内容包括.symtab和.debug节中的符号表e
unsigned long long strtab_off = 0;
unsigned long long strtab_size = 0;
unsigned long long main_size = 0;

extern unsigned long long cadr;
extern unsigned long long csize;
extern unsigned long long dadr;
extern unsigned long long dsize;
extern unsigned long long radr;
extern unsigned long long rsize;
extern unsigned long long main_adr;
extern unsigned long long gp;
extern unsigned int entry;
extern unsigned int endPC;


bool open_file()
{
	elf = fopen("./elf.txt", "w");
	if (!elf)
	{
		perror("Open ./elf.txt");
		return false;
	}
	return true;
}

void read_elf()
{
	if (!open_file())
		return;

	fprintf(elf, "ELF Header:\n");
	read_Elf_header();

	fprintf(elf, "\n\nSection Headers:\n");
	read_elf_sections();

	fprintf(elf, "\n\nProgram Headers:\n");
	read_Phdr();

	fprintf(elf, "\n\nSymbol table:\n");
	read_symtable();

	fclose(elf);

	// entry = main在memory  = main在代码段中的地址
	printf("main_adr: %lx\n", main_adr);
	entry = main_adr;
	endPC = entry + main_size - 4;
}

void read_Elf_header()
{
	//file should be relocated
	fread(&elf64_hdr, 1, sizeof(elf64_hdr), file);

	fprintf(elf, " magic number:  ");
	for (int i = 0; i < EI_NIDENT; i++)
	{
		fprintf(elf, "%.2x ", elf64_hdr.e_ident[i]);
	}
	fprintf(elf, "\n");

	fprintf(elf, " Class:  ");
	switch (elf64_hdr.e_ident[EI_CLASS])
	{
	case ELFCLASS64: fprintf(elf, "ELF64\n"); break;
	default: fprintf(elf, "Unknown file type\n");
	}

	fprintf(elf, " Data:  ");
	switch (elf64_hdr.e_ident[EI_DATA])
	{
	case ELFDATALSB: fprintf(elf, "2' implemntation, little endian\n"); break;
	case ELFDATAMSB: fprintf(elf, "big endian\n"); break;
	default: fprintf(elf, "Invalid\n");
	}

	fprintf(elf, " Version:   ");
	switch (elf64_hdr.e_ident[EI_VERSION])
	{
	case EV_NONE: fprintf(elf, "Invalid\n"); break;
	case EV_CURRENT: fprintf(elf, "1 (current)\n"); break;
	default: fprintf(elf, "Unknown\n");
	}

	fprintf(elf, " OS/ABI:  ");
	switch (elf64_hdr.e_ident[EI_OSABI])
	{
	case 0x00: fprintf(elf, "UNIX System - V \n"); break;
	default: fprintf(elf, "Unknown 0x%x\n", elf64_hdr.e_ident[EI_OSABI]);
	}

	fprintf(elf, " Type:  ");
	switch (elf64_hdr.e_type)
	{
	case ET_NONE: fprintf(elf, "Invalid\n"); break;
	case ET_REL: fprintf(elf, "Relocatable file\n"); break;
	case ET_EXEC: fprintf(elf, "Executable file\n"); break;
	case ET_DYN: fprintf(elf, "Shared object file\n"); break;
	case ET_CORE: fprintf(elf, "Core file\n"); break;
	default: fprintf(elf, "Unknown type 0x%x\n", elf64_hdr.e_type);
	}

	fprintf(elf, " Machine:  ");
	switch (elf64_hdr.e_machine)
	{
	case EM_NONE: fprintf(elf, "Invalid\n"); break;
	case EM_M32: fprintf(elf, "AT&T WE  32100\n"); break;
	default: fprintf(elf, "Unknown 0x%x\n", elf64_hdr.e_machine);
	}

	fprintf(elf, " Version:  ");
	switch (elf64_hdr.e_version)
	{
	case EV_NONE: fprintf(elf, "0x00\n"); break;
	case EV_CURRENT: fprintf(elf, "0x01\n"); break;
	default: fprintf(elf, "Unknown\n");
	}

	fprintf(elf, " Entry point address:  0x%x\n", elf64_hdr.e_entry);

	fprintf(elf, " Start of program headers:  %d (bytes into file)\n", elf64_hdr.e_phoff);
	padr = elf64_hdr.e_phoff;

	fprintf(elf, " Start of section headers:  %d (bytes into file)\n", elf64_hdr.e_shoff);
	sadr = elf64_hdr.e_shoff;

	fprintf(elf, " Flags:  0x%x\n", elf64_hdr.e_flags);

	fprintf(elf, " Size of this header:  %d (Bytes)\n", elf64_hdr.e_ehsize);

	fprintf(elf, " Size of program headers:  %d (Bytes)\n", elf64_hdr.e_phentsize);

	fprintf(elf, " Number of program headers:  %d\n", elf64_hdr.e_phnum);
	pnum = elf64_hdr.e_phnum;

	fprintf(elf, " Size of section headers:  %d Bytes\n", elf64_hdr.e_shentsize);

	fprintf(elf, " Number of section headers:  %d\n", elf64_hdr.e_shnum);
	snum = elf64_hdr.e_shnum;

	fprintf(elf, " Section header string table index:  %d\n", elf64_hdr.e_shstrndx);
}

void read_elf_sections()
{

	Elf64_Shdr elf64_shdr;

	// 先找到放节名字的table的节头条目
	fseek(file, sadr + elf64_hdr.e_shstrndx * sizeof(Elf64_Shdr), 0);
	fread(&elf64_shdr, 1, sizeof(elf64_shdr), file);
	// 找到符号表节
	fseek(file, elf64_shdr.sh_offset, 0);
	unsigned char name[10000];
	// 读入整个符号表
	fread(name, 1, elf64_shdr.sh_size, file);

	// 回到节头部表
	fseek(file, sadr, 0);

	for (int c = 0; c < snum; c++)
	{
		fprintf(elf, " [%3d]\n", c);

		fread(&elf64_shdr, 1, sizeof(elf64_shdr), file);

		fprintf(elf, " Name: ");
		fprintf(elf, "%-15s", name + elf64_shdr.sh_name);
		if (!strcmp((const char*)(name + elf64_shdr.sh_name), ".strtab"))
		{
			strtab_off = elf64_shdr.sh_offset;
			strtab_size = elf64_shdr.sh_size;
		}
		if (!strcmp((const char*)(name + elf64_shdr.sh_name), ".symtab"))
		{
			symtab_off = elf64_shdr.sh_offset;
			symtab_size = elf64_shdr.sh_size;
		}
		if (!strcmp((const char*)(name + elf64_shdr.sh_name), ".text"))
		{
			cadr = elf64_shdr.sh_addr;
			csize = elf64_shdr.sh_size;
		}
		if (!strcmp((const char*)(name + elf64_shdr.sh_name), ".data"))
		{
			dadr = elf64_shdr.sh_addr;
			dsize = elf64_shdr.sh_size;
		}


		fprintf(elf, " Type: ");
		switch (elf64_shdr.sh_type)
		{
		case SHT_NULL: fprintf(elf, "%-9s", "NULL"); break;
		case SHT_PROGBITS: fprintf(elf, "%-9s", "PROGBITS"); break;
		case SHT_SYMTAB: fprintf(elf, "%-9s", "SYMTAB"); break;
		case SHT_STRTAB: fprintf(elf, "%-9s", "STRTAB"); break;
		case SHT_RELA: fprintf(elf, "%-9s", "RELA"); break;
		case SHT_HASH: fprintf(elf, "%-9s", "HASH"); break;
		case SHT_DYNAMIC: fprintf(elf, "%-9s", "DYNAMIC"); break;
		case SHT_NOTE: fprintf(elf, "%-9s", "NOTE"); break;
		case SHT_NOBITS: fprintf(elf, "%-9s", "NOBITS"); break;
		case SHT_REL: fprintf(elf, "%-9s", "REL"); break;
		case SHT_SHLIB: fprintf(elf, "%-9s", "SHLIB"); break;
		case SHT_DYNSYM: fprintf(elf, "%-9s", "DYNSYM"); break;
		case SHT_LOPROC: fprintf(elf, "%-9s", "LOPROC"); break;
		case SHT_HIPROC: fprintf(elf, "%-9s", "HIPROC"); break;
		case SHT_LOUSER: fprintf(elf, "%-9s", "LOUSER"); break;
		case SHT_HIUSER: fprintf(elf, "%-9s", "HIUSER"); break;
		default: fprintf(elf, "%-9s", "Unknown");
		}
		fprintf(elf, " Address:  %08x", elf64_shdr.sh_addr);

		fprintf(elf, " Offest:  %08x", elf64_shdr.sh_offset);

		fprintf(elf, " Size:  %08x", elf64_shdr.sh_size);

		fprintf(elf, " Entsize:  %08x", elf64_shdr.sh_entsize);

		fprintf(elf, " Flags:   ");
		switch (elf64_shdr.sh_flags)
		{
		case SHF_WRITE: fprintf(elf, "%-5s", " W "); break;
		case SHF_ALLOC: fprintf(elf, "%-5s", " A "); break;
		case SHF_EXECINSTR: fprintf(elf, "%-5s", " E "); break;
		case SHF_MASKPROC: fprintf(elf, "%-5s", " M "); break;
		case SHF_WRITE | SHF_ALLOC: fprintf(elf, "%-5s", " WA "); break;
		case SHF_WRITE | SHF_EXECINSTR: fprintf(elf, "%-5s", " WE"); break;
		case SHF_ALLOC | SHF_EXECINSTR: fprintf(elf, "%-5s", " AE"); break;
		case SHF_WRITE | SHF_ALLOC | SHF_EXECINSTR: fprintf(elf, "%-5s", " WAE"); break;
		default: fprintf(elf, "%-5s", " U ");
		}

		fprintf(elf, " Link:  %2d", elf64_shdr.sh_link);

		fprintf(elf, " Info:  %3d", elf64_shdr.sh_info);

		fprintf(elf, " Align: %3x\n", elf64_shdr.sh_addralign);

	}
}

void read_Phdr()
{
	Elf64_Phdr elf64_phdr;

	fseek(file, padr, 0);

	for (int c = 0; c < pnum; c++)
	{
		fprintf(elf, " [%3d]\n", c);

		fread(&elf64_phdr, 1, sizeof(elf64_phdr), file);

		fprintf(elf, " Type:   ");
		switch (elf64_phdr.p_type)
		{
		case PT_NULL: fprintf(elf, "%-9s", "Invalid"); break;
		case PT_LOAD: fprintf(elf, "%-9s", "LOAD"); break;
		default: fprintf(elf, "%-9s", "Unknown");
		}

		fprintf(elf, " Offset:   %08x", elf64_phdr.p_offset);

		fprintf(elf, " VirtAddr:  %08x", elf64_phdr.p_vaddr);

		fprintf(elf, " PhysAddr:   %08x", elf64_phdr.p_paddr);

		fprintf(elf, " FileSiz:   %08x", elf64_phdr.p_filesz);

		fprintf(elf, " MemSiz:   %08x", elf64_phdr.p_memsz);

		fprintf(elf, " Align:   %04x\n", elf64_phdr.p_align);
	}
}


void read_symtable()
{
	Elf64_Sym elf64_sym;
	Elf64_Shdr elf64_shdr;
	unsigned char sysname[10000];

	// 读.strtab
	fseek(file, strtab_off, 0);
	fread(sysname, 1, strtab_size, file);

	// 找到.symtab
	fseek(file, symtab_off, 0);
	int len = symtab_size / sizeof(Elf64_Sym);

	for (int c = 0; c < len; c++)
	{
		fprintf(elf, " [%3d]   ", c);

		fread(&elf64_sym, 1, sizeof(elf64_sym), file);

		fprintf(elf, " Name:  %25s\n", sysname + elf64_sym.st_name);
		if (!strcmp((const char*)(sysname + elf64_sym.st_name), "main"))
		{
			main_adr = elf64_sym.st_value;
			main_size = elf64_sym.st_size;

		}
		if (!strcmp((const char*)(sysname + elf64_sym.st_name), "result"))
		{
			radr = elf64_sym.st_value;
			rsize = elf64_sym.st_size;

		}
		if (!strcmp((const char*)(sysname + elf64_sym.st_name), "_gp"))
		{
			gp = elf64_sym.st_value;
		}


		fprintf(elf, " Bind:   ");
		switch (ELF32_ST_BIND(elf64_sym.st_info))
		{
		case STB_LOCAL: fprintf(elf, "%-9s", "LOCAL"); break;
		case STB_GLOBAL: fprintf(elf, "%-9s", "GLOBAL"); break;
		case STB_WEAK: fprintf(elf, "%-9s", "WEAK"); break;
		case STB_LOPROC: fprintf(elf, "%-9s", "OBJECT"); break;
		case STB_HIPROC: fprintf(elf, "%-9s", "HIPROC"); break;
		default: fprintf(elf, "%-9s", "Unknown");
		}

		fprintf(elf, " Type:   ");
		switch (ELF32_ST_TYPE(elf64_sym.st_info))
		{
		case STT_NOTYPE: fprintf(elf, "%-9s", "NOTYPE");  break;
		case STT_OBJECT: fprintf(elf, "%-9s", "OBJECT");  break;
		case STT_FUNC: fprintf(elf, "%-9s", "FUNC");  break;
		case STT_SECTION: fprintf(elf, "%-9s", "SECTION");  break;
		case STT_FILE: fprintf(elf, "%-9s", "FILE");  break;
		case STT_LOPROC: fprintf(elf, "%-9s", "OBJECT");  break;
		case STT_HIPROC: fprintf(elf, "%-9s", "HIPROC");  break;
		default: fprintf(elf, "%-9s", "Unknown");
		}

		fprintf(elf, " Size:   %4d", elf64_sym.st_size);

		fprintf(elf, " Value:   %08x\n", elf64_sym.st_value);

	}

}
