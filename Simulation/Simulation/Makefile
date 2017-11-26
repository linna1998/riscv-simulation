CC = g++

all: sim

sim : cache.o memory.o Read_Elf.o Simulation.o

	$(CC) -o $@ $^

cache.o : cache.h def.h

memory.o : memory.h

Read_Elf.o : Read_Elf.h

Simulation.o : Simulation.h
 
.PHONY : clean

clean :
	rm -rf sim *.o
