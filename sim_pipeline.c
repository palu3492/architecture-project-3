#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define NUMMEMORY 65536 /* maximum number of data words in memory */
#define NUMREGS 8 /* number of machine registers */

#define ADD 0
#define NAND 1
#define LW 2
#define SW 3
#define BEQ 4
#define JALR 5 /* JALR - not implemented in this project */
#define HALT 6
#define NOOP 7

#define NOOPINSTRUCTION 0x1c00000

typedef struct IFIDstruct{
	int instr;
	int pcplus1;
} IFIDType;

typedef struct IDEXstruct{
	int instr;
	int pcplus1;
	int readregA;
	int readregB;
	int offset;
} IDEXType;

typedef struct EXMEMstruct{
	int instr;
	int branchtarget;
	int aluresult;
	int readreg;
} EXMEMType;

typedef struct MEMWBstruct{
	int instr;
	int writedata;
} MEMWBType;

typedef struct WBENDstruct{
	int instr;
	int writedata;
} WBENDType;

typedef struct statestruct{
	int pc;
	int instrmem[NUMMEMORY];
	int datamem[NUMMEMORY];
	int reg[NUMREGS];
	int nummemory;
	IFIDType IFID;
	IDEXType IDEX;
	EXMEMType EXMEM;
	MEMWBType MEMWB;
	WBENDType WBEND;
	int cycles; /* Number of cycles run so far */
	int fetched; /* Total number of instructions fetched */
	int retired; /* Total number of completed instructions */
	int branches; /* Total number of branches executed */
	int mispreds; /* Number of branch mispredictions*/
} statetype;

int field0(int instruction){
	return((instruction>>19) & 0x7);
}

int field1(int instruction){
	return((instruction>>16) & 0x7);
}

int field2(int instruction){
	return(instruction & 0xFFFF);
}

int opcode(int instruction){
	return(instruction>>22);
}

int signextend(int num){
	// convert a 16-bit number into a 32-bit integer
	if (num & (1<<15) ){
		num -= (1<<16);
	}
	return num;
}

void printinstruction(int instr){
	char opcodestring[10];
	if (opcode(instr) == ADD){
		strcpy(opcodestring, "add");
	} else if (opcode(instr) == NAND){
		strcpy(opcodestring, "nand");
	} else if (opcode(instr) == LW){
		strcpy(opcodestring, "lw");
	} else if (opcode(instr) == SW){
		strcpy(opcodestring, "sw");
	} else if (opcode(instr) == BEQ){
		strcpy(opcodestring, "beq");
	} else if (opcode(instr) == JALR){
		strcpy(opcodestring, "jalr");
	} else if (opcode(instr) == HALT){
		strcpy(opcodestring, "halt");
	} else if (opcode(instr) == NOOP){
		strcpy(opcodestring, "noop");
	} else{
		strcpy(opcodestring, "data");
	}

	if(opcode(instr) == ADD || opcode(instr) == NAND){
		printf("%s %d %d %d\n", opcodestring, field2(instr), field0(instr), field1(instr));
	}else if(strcmp(opcodestring, "data") == 0){
		printf("%s %d\n", opcodestring, signextend(field2(instr)));
	}else{
		printf("%s %d %d %d\n", opcodestring, field0(instr), field1(instr), signextend(field2(instr)));
	}
}

void printstate(statetype *stateptr){
	int i;
	printf("\n@@@\nstate before cycle %d starts\n", stateptr->cycles);
	printf("\tpc %d\n", stateptr->pc);

	printf("\tdata memory:\n");
	for (i=0; i<stateptr->nummemory; i++){
		printf("\t\tdatamem[ %d ] %d\n", i, stateptr->datamem[i]);
	}

	printf("\tregisters:\n");
	for (i=0; i<NUMREGS; i++){
		printf("\t\treg[ %d ] %d\n", i, stateptr->reg[i]);
	}

	printf("\tIFID:\n");
		printf("\t\tinstruction ");
		printinstruction(stateptr->IFID.instr);
		printf("\t\tpcplus1 %d\n", stateptr->IFID.pcplus1);

	printf("\tIDEX:\n");
		printf("\t\tinstruction ");
		printinstruction(stateptr->IDEX.instr);
		printf("\t\tpcplus1 %d\n", stateptr->IDEX.pcplus1);
		printf("\t\treadregA %d\n", stateptr->IDEX.readregA);
		printf("\t\treadregB %d\n", stateptr->IDEX.readregB);
		printf("\t\toffset %d\n", stateptr->IDEX.offset);

	printf("\tEXMEM:\n");
		printf("\t\tinstruction ");
		printinstruction(stateptr->EXMEM.instr);
		printf("\t\tbranchtarget %d\n", stateptr->EXMEM.branchtarget);
		printf("\t\taluresult %d\n", stateptr->EXMEM.aluresult);
		printf("\t\treadreg %d\n", stateptr->EXMEM.readreg);

	printf("\tMEMWB:\n");
		printf("\t\tinstruction ");
		printinstruction(stateptr->MEMWB.instr);
		printf("\t\twritedata %d\n", stateptr->MEMWB.writedata);

	printf("\tWBEND:\n");
		printf("\t\tinstruction ");
		printinstruction(stateptr->WBEND.instr);
		printf("\t\twritedata %d\n", stateptr->WBEND.writedata);
}

void print_stats(int n_instrs){
	printf("INSTRUCTIONS: %d\n", n_instrs);
}

void run(statetype* state){
	statetype* newstate = (statetype*)malloc(sizeof(statetype));

	// Primary loop
	while(1){
		printstate(state);

		/* check for halt */
		if(opcode(state->MEMWB.instr) == HALT){
			state->retired++;
			printf("machine halted\n");
			printf("total of %d cycles executed\n", state->cycles);
			printf("total of %d instructions fetched\n", state->fetched);
			printf("total of %d instructions retired\n", state->retired);
			printf("total of %d branches executed\n", state->branches);
			printf("total of %d branch mispredictions\n", state->mispreds);
			exit(0);
		}

		*newstate = *state;
		newstate->cycles++;

		/* ------------------ IF  stage ------------------ */

		// fetch instruction and store in IF/ID pipeline buffer
		newstate->IFID.instr = state->instrmem[state->pc];

		// increment the PC and store in IF/ID pipeline buffer
		newstate->pc = state->pc+1;
		newstate->IFID.pcplus1 = state->pc+1;
		
		if(opcode(state->IFID.instr) == HALT || opcode(state->IDEX.instr) == HALT || opcode(state->EXMEM.instr) == HALT || opcode(state->MEMWB.instr) == HALT){
			// do nothing
		}else{
			newstate->fetched++;
		}

		/* ------------------ ID  stage ------------------ */

		// fetch instruction from IF/ID and store in ID/EX pipeline buffer
		newstate->IDEX.instr = state->IFID.instr;

		// fetch PC from IF/ID and store in ID/EX pipeline buffer
		newstate->IDEX.pcplus1 = state->IFID.pcplus1;

		// decode instruction and store in ID/EX pipeline buffer
		newstate->IDEX.readregA = state->reg[field0(state->IFID.instr)];
		newstate->IDEX.readregB = state->reg[field1(state->IFID.instr)];
		newstate->IDEX.offset = signextend(field2(state->IFID.instr));

		/* ------------------ EX  stage ------------------ */

		// fetch instruction from ID/EX and store in EX/MEM pipeline buffer
		newstate->EXMEM.instr = state->IDEX.instr;

		// calculate branch target and store in EX/MEM pipeline buffer
		newstate->EXMEM.branchtarget = state->IDEX.pcplus1 + state->IDEX.offset;

		// calculate ALU result and store in EX/MEM pipeline buffer
		// ADD
		if(opcode(state->IDEX.instr) == ADD){
			newstate->EXMEM.aluresult = state->IDEX.readregA + state->IDEX.readregB;
		}
		// NAND
		else if(opcode(state->IDEX.instr) == NAND){
			newstate->EXMEM.aluresult = ~(state->IDEX.readregA & state->IDEX.readregB);
		}
		// LW or SW
		else if(opcode(state->IDEX.instr) == LW || opcode(state->IDEX.instr) == SW){
			newstate->EXMEM.aluresult = state->IDEX.readregB + state->IDEX.offset;
		}
		// BEQ
		else if(opcode(state->IDEX.instr) == BEQ){
			newstate->EXMEM.aluresult = (state->IDEX.readregA == state->IDEX.readregB);
		}else{
			newstate->EXMEM.aluresult = 0;
		}

		// store data read from register A in EX/MEM pipeline buffer
		newstate->EXMEM.readreg = state->IDEX.readregA;

		/* ------------------ MEM stage ------------------ */

		// fetch instruction from EX/MEM and store in MEM/WB pipeline buffer
		newstate->MEMWB.instr = state->EXMEM.instr;

		// ADD or NAND
		if(opcode(state->EXMEM.instr) == ADD || opcode(state->EXMEM.instr) == NAND){
			newstate->MEMWB.writedata = state->EXMEM.aluresult;
		}
		// LW
		else if(opcode(state->EXMEM.instr) == LW){
			newstate->MEMWB.writedata = state->datamem[state->EXMEM.aluresult];
		}
		// SW
		else if(opcode(state->EXMEM.instr) == SW){
			newstate->datamem[state->EXMEM.aluresult] = state->EXMEM.readreg;
		}
		// BEQ
		else if(opcode(state->EXMEM.instr) == BEQ){
			if(state->EXMEM.aluresult){
				newstate->pc = state->EXMEM.branchtarget;
				newstate->branches++;
			}
		}else{
			newstate->MEMWB.writedata = 0;
		}

		/* ------------------ WB  stage ------------------ */

		// fetch instruction from MEM/WB and store in WBEND pipeline buffer
		newstate->WBEND.instr = state->MEMWB.instr;

		// fetch write data from MEM/WB and store in WBEND pipeline buffer
		newstate->WBEND.writedata = state->MEMWB.writedata;

		// AND or NAND
		if(opcode(state->MEMWB.instr) == ADD || opcode(state->MEMWB.instr) == NAND){
			newstate->reg[field2(state->MEMWB.instr)] = state->MEMWB.writedata;
			newstate->retired++;
		}
		// LW
		else if(opcode(state->MEMWB.instr) == LW){
			newstate->reg[field0(state->MEMWB.instr)] = state->MEMWB.writedata;
			newstate->retired++;
		}else if(opcode(state->MEMWB.instr) == SW || opcode(state->MEMWB.instr) == BEQ){
			newstate->retired++;
		}

		/* this is the last statement before the end of the loop. It marks the end of
		the cycle and updates the current state with the values calculated in this
		cycle - AKA "Clock Tick" */
		*state = *newstate;
	}
}

int main(int argc, char** argv){
	/* get command line arguments */
	char* fname;

	if(argc == 1){
		fname = (char*)malloc(sizeof(char)*100);
		printf("Enter the name of the machine code file to simulate: ");
		fgets(fname, 100, stdin);
		fname[strlen(fname)-1] = '\0'; // gobble up the \n with a \0
	}else if (argc == 2){
		int strsize = strlen(argv[1]);

		fname = (char*)malloc(strsize);
		fname[0] = '\0';

		strcat(fname, argv[1]);
	}else{
		printf("Please run this program correctly\n");
		exit(-1);
	}

	FILE *fp = fopen(fname, "r");
	if (fp == NULL){
		printf("Cannot open file '%s' : %s\n", fname, strerror(errno));
		return -1;
	}

	/* count the number of lines by counting newline characters */
	int line_count = 0;
	int c;
	while (EOF != (c=getc(fp))){
		if (c == '\n'){
			line_count++;
		}
	}

	/* reset fp to the beginning of the file */
	rewind(fp);

	/* initialize state */
	statetype* state = (statetype*)malloc(sizeof(statetype));

	state->pc = 0;

	memset(state->instrmem, 0, NUMMEMORY*sizeof(int));
	memset(state->datamem, 0, NUMMEMORY*sizeof(int));
	memset(state->reg, 0, NUMREGS*sizeof(int));

	state->nummemory = line_count;

	/* initialize IFID */
	state->IFID.instr = NOOPINSTRUCTION;
	state->IFID.pcplus1 = 0;

	/* initialize IDEX */
	state->IDEX.instr = NOOPINSTRUCTION;
	state->IDEX.pcplus1 = 0;
	state->IDEX.readregA = 0;
	state->IDEX.readregB = 0;
	state->IDEX.offset = 0;

	/* initialize EXMEM */
	state->EXMEM.instr = NOOPINSTRUCTION;
	state->EXMEM.branchtarget = 0;
	state->EXMEM.aluresult = 0;
	state->EXMEM.readreg = 0;

	/* initialize MEMWB */
	state->MEMWB.instr = NOOPINSTRUCTION;
	state->MEMWB.writedata = 0;

	/* initialize WBEND */
	state->WBEND.instr = NOOPINSTRUCTION;
	state->WBEND.writedata = 0;

	/* initialize stat trackers */
	state->cycles = 0;
	state->fetched = 0;
	state->retired = 0;
	state->branches = 0;
	state->mispreds = 0;

	char line[256];

	int i = 0;
	while (fgets(line, sizeof(line), fp)){
		/* note that fgets doesn't strip the terminating \n, checking its
		   presence would allow to handle lines longer that sizeof(line) */
		state->instrmem[i] = atoi(line);
		state->datamem[i] = atoi(line);
		i++;
	}
	fclose(fp);

	/* run the simulation */
	run(state);

	free(state);
	free(fname);

}
