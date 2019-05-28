#include "../kernel.h"

#define PROGVER 1
#define MAXPROGSIZE 0xFFFF
#define MAXSTACKSIZE 0xFF
#define MSSECONDS 1
#define MAXINSTRUCTIONS 0xFF

#define DEBUG 0

unsigned char register_a 	= 0x00;
unsigned char register_x 	= 0x00;
unsigned char register_y 	= 0x00;
unsigned char register_sp	= 0x00;
unsigned short register_pc	= 0x00;
unsigned char register_flags	= 0x00;

unsigned char memory[MAXPROGSIZE];
unsigned short stack[MAXSTACKSIZE];
unsigned long instructionset[MAXINSTRUCTIONS];

//
// Debug facilities
//

void regdump(){
	printf("REGDUMP\n");
	printf("A  = %x \n",register_a);
	printf("X  = %x \n",register_x);
	printf("Y  = %x \n",register_y);
	printf("SP = %x \n",register_sp);
	printf("PC = %x \n",register_pc);
	printf("FLG= %x \n",register_flags);
	printf("\nSTACK\n");
	for(unsigned char i = 0 ; i < register_sp ; i++){
		printf("%i -> %x \n",i,(unsigned int)stack[(register_sp-1)-i]);
	}
}

void panic(const char* message){
	printf("[PANIC] %s \n",message);
	for(;;);
}

//
// Stack facilities
//

void pushCallstack(unsigned short address){
	if(register_sp>=MAXSTACKSIZE){
		panic("Stack went to deep!");
	}else{
		stack[register_sp++] = address;
	}
}

unsigned short popCallstack(){
	if(register_sp==0){
		panic("Callstack error");
	}else{
		register_sp--;
		unsigned short res = stack[register_sp];
		return res;
	}
}

//
// Memory facilities
//

unsigned char getMemory(unsigned short address){
	if(address<MAXPROGSIZE&&address>-1){
		return memory[address & 0xFFFF ] & 0xFF;
	}else{
		panic("Unable to allocate memory\n");
	}
}

unsigned short getWMemory(unsigned short address){
	unsigned short A = getMemory(address);
	unsigned short B = getMemory(address+1);
	return (B << 8) | A;
}

#define gotoxy(x,y) printf("\033[%d;%dH", (x), (y))
#define scrwidth 0x1F

void setMemory(unsigned short address,unsigned char value){
	if(address<MAXPROGSIZE&&address>-1){
		memory[address & 0xFFFF ] = value & 0xFF;
		if((address & 0xFFFF)>=0x0200&&(address & 0xFFFF)<=0x05DD){
			unsigned short trsalt = address - 0x0200;
			unsigned short gamma = trsalt/scrwidth;
			unsigned short delta = trsalt-gamma;
			putpixel(gamma,delta,value);
		}
	}else{
		panic("Unable to allocate memory\n");
	}
}

void loadToMemory(unsigned short address, unsigned char* data, unsigned short count){
	if(address<MAXPROGSIZE&&address>-1){
		if((address+count)<MAXPROGSIZE&&(address+count)>-1){
			for(unsigned short i = 0 ; i < count ; i++){
				setMemory(address+i,data[i]);
			}
		}else{
			panic("Unable to allocate memory\n");
		}
	}else{
		panic("Unable to allocate memory\n");
	}
}

void loadDemo(){
	unsigned char dataset[309] = {
		 0x20 ,0x06 ,0x06 ,0x20 ,0x38 ,0x06 ,0x20 ,0x0d ,0x06 ,0x20 ,0x2a ,0x06 ,0x60 ,0xa9 ,0x02 ,0x85 
		,0x02 ,0xa9 ,0x04 ,0x85 ,0x03 ,0xa9 ,0x11 ,0x85 ,0x10 ,0xa9 ,0x10 ,0x85 ,0x12 ,0xa9 ,0x0f ,0x85 
		,0x14 ,0xa9 ,0x04 ,0x85 ,0x11 ,0x85 ,0x13 ,0x85 ,0x15 ,0x60 ,0xa5 ,0xfe ,0x85 ,0x00 ,0xa5 ,0xfe 
		,0x29 ,0x03 ,0x18 ,0x69 ,0x02 ,0x85 ,0x01 ,0x60 ,0x20 ,0x4d ,0x06 ,0x20 ,0x8d ,0x06 ,0x20 ,0xc3 
		,0x06 ,0x20 ,0x19 ,0x07 ,0x20 ,0x20 ,0x07 ,0x20 ,0x2d ,0x07 ,0x4c ,0x38 ,0x06 ,0xa5 ,0xff ,0xc9 
		,0x77 ,0xf0 ,0x0d ,0xc9 ,0x64 ,0xf0 ,0x14 ,0xc9 ,0x73 ,0xf0 ,0x1b ,0xc9 ,0x61 ,0xf0 ,0x22 ,0x60 
		,0xa9 ,0x04 ,0x24 ,0x02 ,0xd0 ,0x26 ,0xa9 ,0x01 ,0x85 ,0x02 ,0x60 ,0xa9 ,0x08 ,0x24 ,0x02 ,0xd0 
		,0x1b ,0xa9 ,0x02 ,0x85 ,0x02 ,0x60 ,0xa9 ,0x01 ,0x24 ,0x02 ,0xd0 ,0x10 ,0xa9 ,0x04 ,0x85 ,0x02 
		,0x60 ,0xa9 ,0x02 ,0x24 ,0x02 ,0xd0 ,0x05 ,0xa9 ,0x08 ,0x85 ,0x02 ,0x60 ,0x60 ,0x20 ,0x94 ,0x06 
		,0x20 ,0xa8 ,0x06 ,0x60 ,0xa5 ,0x00 ,0xc5 ,0x10 ,0xd0 ,0x0d ,0xa5 ,0x01 ,0xc5 ,0x11 ,0xd0 ,0x07 
		,0xe6 ,0x03 ,0xe6 ,0x03 ,0x20 ,0x2a ,0x06 ,0x60 ,0xa2 ,0x02 ,0xb5 ,0x10 ,0xc5 ,0x10 ,0xd0 ,0x06 
		,0xb5 ,0x11 ,0xc5 ,0x11 ,0xf0 ,0x09 ,0xe8 ,0xe8 ,0xe4 ,0x03 ,0xf0 ,0x06 ,0x4c ,0xaa ,0x06 ,0x4c 
		,0x35 ,0x07 ,0x60 ,0xa6 ,0x03 ,0xca ,0x8a ,0xb5 ,0x10 ,0x95 ,0x12 ,0xca ,0x10 ,0xf9 ,0xa5 ,0x02 
		,0x4a ,0xb0 ,0x09 ,0x4a ,0xb0 ,0x19 ,0x4a ,0xb0 ,0x1f ,0x4a ,0xb0 ,0x2f ,0xa5 ,0x10 ,0x38 ,0xe9 
		,0x20 ,0x85 ,0x10 ,0x90 ,0x01 ,0x60 ,0xc6 ,0x11 ,0xa9 ,0x01 ,0xc5 ,0x11 ,0xf0 ,0x28 ,0x60 ,0xe6 
		,0x10 ,0xa9 ,0x1f ,0x24 ,0x10 ,0xf0 ,0x1f ,0x60 ,0xa5 ,0x10 ,0x18 ,0x69 ,0x20 ,0x85 ,0x10 ,0xb0 
		,0x01 ,0x60 ,0xe6 ,0x11 ,0xa9 ,0x06 ,0xc5 ,0x11 ,0xf0 ,0x0c ,0x60 ,0xc6 ,0x10 ,0xa5 ,0x10 ,0x29 
		,0x1f ,0xc9 ,0x1f ,0xf0 ,0x01 ,0x60 ,0x4c ,0x35 ,0x07 ,0xa0 ,0x00 ,0xa5 ,0xfe ,0x91 ,0x00 ,0x60 
		,0xa6 ,0x03 ,0xa9 ,0x00 ,0x81 ,0x10 ,0xa2 ,0x00 ,0xa9 ,0x01 ,0x81 ,0x10 ,0x60 ,0xa2 ,0x00 ,0xea 
		,0xea ,0xca ,0xd0 ,0xfb ,0x60 
	};
	loadToMemory(0x0600,dataset,sizeof(dataset));
}


void loadDemoSimple(){
	unsigned char dataset[15] = {
		 0xa9,0x01,0x8d,0x1f,0x02,0xa9,0x05,0x8d,0x01,0x02,0xa9,0x08,0x8d,0xdd,0x05 
	};
	loadToMemory(0x0600,dataset,sizeof(dataset));
}

//
// Memory translation facilities
//

unsigned short translateZeroPage(){}

unsigned short translateZeroPageX(){}

unsigned short translateZeroPageY(){}

unsigned short translateRelative(){}

unsigned short translateAddressRelative(){
	unsigned short addA = getWMemory(register_pc);
	register_pc++;
	register_pc++;
	return addA;
}

unsigned short translateAbsolute(){}

unsigned short translateAbsoluteX(){}

unsigned short translateAbsoluteY(){}

//
// CPU facilities
//

void reset(){

	//
	// registers legen
	register_a	= 0x00;
	register_x	= 0x00;
	register_y	= 0x00;
	register_sp	= 0x00;
	register_pc	= 0x0000;
	register_flags	= 0x00;
	
	//
	// geheugen legen
	for(int i = 0 ; i < MAXPROGSIZE ; i++){
		setMemory(i,0x00);
	}
	
	//
	// stack legen
	for(int i = 0 ; i < MAXSTACKSIZE ; i++){
		stack[i] = 0x0000;
	}
}

#define FLAG_CARRY     0b00000001
#define FLAG_ZERO      0b00000010
#define FLAG_INTERRUPT 0b00000100
#define FLAG_BREAK     0b00010000
#define FLAG_OVERFLOW  0b00100000
#define FLAG_NEGATIVE  0b01000000
#define FLAG_DECIMAL   0b10000000

void setFlag(unsigned char flagtype){
	register_flags |= flagtype;
}

void unsetFlag(unsigned char flagtype){
	register_flags &= (~flagtype);
}

unsigned char checkFlag(unsigned char flagtype){
	return (register_flags & flagtype) > 0;
}

unsigned char skipme = 0;

void clc(){
	//
	// CLC clear carry
	// OPCODE 0x18
	// SIZE 0x01
	// TIJD 0x02
	//
	
	unsetFlag(FLAG_CARRY);
	skipme=2;
	if(DEBUG) printf("[INFOR] CLC \n");
}

//
// Instructionset facilities
//

void jsr(){
	//
	// JUMPS TO SUBROUTINE
	// OPCODE 0x20
	// SIZE 0x03
	// TIJD 0x06
	//
	
	unsigned short address = getWMemory(register_pc);
	pushCallstack(register_pc+1);
	register_pc = address;
	skipme = 5;
	if(DEBUG) printf("[INFOR] JSR to %x \n",address);
}

void and_immediate(){
	//
	// AND, bitwise AND with accumulator immediate
	// OPCODE 0x29
	// SIZE 0x02
	// TIME 0x02
	//
	
	unsigned char inand = getMemory(register_pc);
	unsigned char outand = register_a & inand;
	register_a = outand & 0xFF;
	register_pc++;
	skipme = 2;
	if(DEBUG) printf("[INFOR] AND to %x \n",register_a);
}

void rts(){
	//
	// RTS , Return from subroutine
	// OPCODE: 0x60
	// SIZE: 0x01
	// TIJD: 0x06
	//
	
	skipme = 6;
	unsigned short address = popCallstack();
	address++;
	register_pc = address;
	if(DEBUG) printf("[INFOR] RTS to %x \n",address);
}

void sta_zeropage(){
	//
	// STA, STORE ACCUMULATOR
	// OPCODE: 0x85
	// SIZE: 0x02
	// TIJD: 0x03
	//
	
	unsigned char val = getMemory(register_pc);
	setMemory(val,register_a);
	register_pc++;
	skipme = 3;
	if(DEBUG) printf("[INFOR] STA to %x \n",val);
}

void lda_zero_page(){
	//
	// LOAD ACCUMULATOR ZERO PAGE (direct waarde ingeven)
	// OPCODE: 0xA9
	// SIZE: 0x02
	// TIJD: 0x02
	//
	
	unsigned char val = getMemory(getMemory(register_pc));
	register_a = val;
	register_pc++;
	skipme = 3;
	if(DEBUG) printf("[INFOR] LDA to %x \n",val);
}

void lda_immediate(){
	//
	// LOAD ACCUMULATOR IMMEDIATE (direct waarde ingeven)
	// OPCODE: 0xA9
	// SIZE: 0x02
	// TIJD: 0x02
	//
	
	unsigned char val = getMemory(register_pc);
	register_a = val;
	register_pc++;
	skipme = 2;
	if(DEBUG) printf("[INFOR] LDA to %x \n",val);
}

void tya(){
	//
	// TYA Y => A
	// OPCODE: 0x98
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	register_a = register_y;
	if(DEBUG) printf("[INFOR] register Y copied to A\n");
}

void txs(){
	//
	// TXS X => S
	// OPCODE: 0x9A
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	register_sp = register_x;
	if(DEBUG) printf("[INFOR] register X copied to SP\n");
}

void txa(){
	//
	// TXA X => A
	// OPCODE: 0x8A
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	register_a = register_x;
	if(DEBUG) printf("[INFOR] register X copied to A\n");
}

void tsx(){
	//
	// TSX X => SP
	// OPCODE: 0xBA
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	register_x = register_sp;
	if(DEBUG) printf("[INFOR] register X copied to A\n");
}

void tay(){
	//
	// TAY Y = A
	// OPCODE: 0xA8
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	register_y = register_a;
	if(DEBUG) printf("[INFOR] register A copied to Y\n");
}

void tax(){
	//
	// TAX X = A
	// OPCODE: 0xAA
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	register_x = register_a;
	if(DEBUG) printf("[INFOR] register A copied to X\n");
}

void cld(){
	//
	// CLD D = 0
	// OPCODE: 0xD8
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	unsetFlag(FLAG_DECIMAL);
	if(DEBUG) printf("[INFOR] clear decimal mode flag\n");
}

void cli(){
	//
	// CLD I = 0
	// OPCODE: 0x58
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	unsetFlag(FLAG_INTERRUPT);
	if(DEBUG) printf("[INFOR] clear interupt flag\n");
}

void clv(){
	//
	// CLV O = 0
	// OPCODE: 0xB8
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	unsetFlag(FLAG_OVERFLOW);
	if(DEBUG) printf("[INFOR] clear overflow flag\n");
}

void dex(){
	//
	// DEX X--
	// OPCODE: 0xCA
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	register_x--;
	if(DEBUG) printf("[INFOR] decrement X\n");
}

void dey(){
	//
	// DEY Y--
	// OPCODE: 0x88
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	register_y--;
	if(DEBUG) printf("[INFOR] decrement Y\n");
}

void inx(){
	//
	// INX X++
	// OPCODE: 0xE8
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	register_x++;
	if(DEBUG) printf("[INFOR] increment X\n");
}

void iny(){
	//
	// INY Y++
	// OPCODE: 0xC8
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	register_y++;
	if(DEBUG) printf("[INFOR] increment Y\n");
}

void nop(){
	//
	// NOP
	// OPCODE: EA
	// SIZE: 0x01
	// TIJD: 0x01
	//
	if(DEBUG) printf("[INFOR] nothing\n");
}

void sec(){
	//
	// SEC s = 1
	// OPCODE: 0x38
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	setFlag(FLAG_CARRY);
	if(DEBUG) printf("[INFOR] set carry flag\n");
}

void sed(){
	//
	// SED d = 1
	// OPCODE: 0xF8
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	setFlag(FLAG_DECIMAL);
	if(DEBUG) printf("[INFOR] set decimal flag\n");
}

void sei(){
	//
	// SEi i = 1
	// OPCODE: 0x78
	// SIZE: 0x01
	// TIJD: 0x02
	//
	
	setFlag(FLAG_INTERRUPT);
	if(DEBUG) printf("[INFOR] set interrupt flag\n");
}

void adc_immediate(){
	unsigned char addc = getMemory(register_pc);
	register_a += addc;
	register_pc++;
}

void cmp_immediate(){
	unsigned char addc = getMemory(register_pc);
	register_pc++;
	if(register_a>=addc){
		setFlag(FLAG_ZERO);
	}else{
		setFlag(FLAG_NEGATIVE);
	}
}

void beq(){
	unsigned short regA = translateAddressRelative();
	if(register_flags & FLAG_ZERO){
		register_pc = regA;
	}
}

//
// Main routine
//

#define clear() printf("\033[H\033[J")

void init_e6502(){
	printf("6502 chip emulator by Alexandros de Regt for SanderOSUSB version %x \n",PROGVER);
//	reset();
	loadDemo();
	
	register_pc = 0x0600;
	
	//
	// Adding instructionset
	instructionset[0x18] = (unsigned long) &clc;
	instructionset[0x20] = (unsigned long) &jsr;
	instructionset[0x29] = (unsigned long) &and_immediate;
	instructionset[0x38] = (unsigned long) &sec;
	instructionset[0x58] = (unsigned long) &cli;
	instructionset[0x60] = (unsigned long) &rts;
	instructionset[0x69] = (unsigned long) &adc_immediate;
	instructionset[0x78] = (unsigned long) &sei;
	instructionset[0x85] = (unsigned long) &sta_zeropage;
	instructionset[0x88] = (unsigned long) &dey;
	instructionset[0x8A] = (unsigned long) &txa;
	instructionset[0x98] = (unsigned long) &tya;
	instructionset[0x9A] = (unsigned long) &txs;
	instructionset[0xA5] = (unsigned long) &lda_zero_page;
	instructionset[0xA8] = (unsigned long) &tay;
	instructionset[0xA9] = (unsigned long) &lda_immediate;
	instructionset[0xAA] = (unsigned long) &tax;
	instructionset[0xB8] = (unsigned long) &clv;
	instructionset[0xBA] = (unsigned long) &tsx;
	instructionset[0xC8] = (unsigned long) &iny;
	instructionset[0xC9] = (unsigned long) &cmp_immediate;
	instructionset[0xCA] = (unsigned long) &dex;
	instructionset[0xD8] = (unsigned long) &cld;
	instructionset[0xE8] = (unsigned long) &inx;
	instructionset[0xEA] = (unsigned long) &nop;
	instructionset[0xF0] = (unsigned long) &beq;
	instructionset[0xF8] = (unsigned long) &sed;
	
	while(1){
		if(skipme==0){
			unsigned char instructie = getMemory(register_pc);
			register_pc++;
			
			unsigned long functionlocation = instructionset[instructie];
			if(functionlocation==0x00000000){
				printf("\n[CPU  ] Unknown instruction [ 0x%x ] !\n",instructie);
				panic("Processor halted");
			}else{
				void (*foo)() = (void*)functionlocation;
				foo();
			}
			if(register_pc>=MAXPROGSIZE){
				break;
			}
		}else{
			skipme--;
		}
		sleep(MSSECONDS);
	}
	printf("Program finished!\n");
}
