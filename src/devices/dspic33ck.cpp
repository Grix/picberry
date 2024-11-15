/*
 * Raspberry Pi PIC Programmer using GPIO connector
 * https://github.com/WallaceIT/picberry
 * Copyright 2014 Francesco Valla
 * Copyright 2024 Gitle Mikkelsen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdint.h>
#include <string.h>
#include <iostream>
#include <unistd.h>

#include "dspic33ck.h"

/* delays (in microseconds; nanoseconds are rounded to 1us) */
#define DELAY_P1   			1		// 200ns
#define DELAY_P1A			1		// 80ns
#define DELAY_P1B			1		// 80ns
#define DELAY_P2			1		// 15ns
#define DELAY_P3			1		// 15ns
#define DELAY_P4			1		// 40ns
#define DELAY_P4A			1		// 40ns
#define DELAY_P5			1		// 20ns
#define DELAY_P6			1		// 100ns
#define DELAY_P7			50000	// 50ms
#define DELAY_P8			12		// 12us
#define DELAY_P9A			10		// 10us
#define DELAY_P9B			15		// 15us - 23us max!
#define DELAY_P10			1		// 400ns
#define DELAY_P11			100000	// 20ms max, guess 10ms
#define DELAY_P12			100000	// 20ms max, guess 10ms
#define DELAY_P13			10		// 20us max, guess 10us
#define DELAY_P14			1		// 1us MAX!
#define DELAY_P15			1		// 10ns
#define DELAY_P16			0		// 0s
#define DELAY_P17   		1		// 100ns
#define DELAY_P18			1000	// 1ms
#define DELAY_P19			1		// 25ns
#define DELAY_P20			25000	// 25ms
#define DELAY_P21			1		// 1us - 500us MAX!

#define ENTER_PROGRAM_KEY	0x4D434851

#define reset_pc() send_cmd(0x040200)
#define send_nop() send_cmd(0x000000)

static unsigned int counter=0;
static uint16_t nvmcon;

/* Send a 24-bit command to the PIC (LSB first) through a SIX instruction */
void dspic33ck::send_cmd(uint32_t cmd)
{
	uint8_t i;

	GPIO_CLR(pic_data);

	/* send the SIX = 0x0000 instruction */
	for (i = 0; i < 4; i++) {
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1B);
		GPIO_CLR(pic_clk);
		delay_us(DELAY_P1A);
	}

	delay_us(DELAY_P4);

	/* send the 24-bit command */
	for (i = 0; i < 24; i++) {
		if ( (cmd >> i) & 0x00000001 )
			GPIO_SET(pic_data);
		else
			GPIO_CLR(pic_data);
		delay_us(DELAY_P1A);
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1B);
		GPIO_CLR(pic_clk);
	}

	delay_us(DELAY_P4A);

}

/* Read 16-bit data word from the PIC (LSB first) through a REGOUT inst */
uint16_t dspic33ck::read_data(void)
{
	uint8_t i;
	uint16_t data = 0;

	GPIO_CLR(pic_data);
	GPIO_CLR(pic_clk);

	/* send the REGOUT=0x0001 instruction */
	for (i = 0; i < 4; i++) {
		if ( (0x0001 >> i) & 0x001 )
			GPIO_SET(pic_data);
		else
			GPIO_CLR(pic_data);
		delay_us(DELAY_P1A);
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1B);
		GPIO_CLR(pic_clk);
	}

	delay_us(DELAY_P4);

	/* idle for 8 clock cycles, waiting for the data to be ready */
	for (i = 0; i < 8; i++) {
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1B);
		GPIO_CLR(pic_clk);
		delay_us(DELAY_P1A);
	}

	delay_us(DELAY_P5);

	GPIO_IN(pic_data);

	/* read a 16-bit data word */
	for (i = 0; i < 16; i++) {
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1B);
		data |= ( GPIO_LEV(pic_data) & 0x00000001 ) << i;
		GPIO_CLR(pic_clk);
		delay_us(DELAY_P1A);
	}

	delay_us(DELAY_P4A);
	GPIO_OUT(pic_data);
	return data;
}

/* enter program mode */
void dspic33ck::enter_program_mode(void)
{
	int i;

	GPIO_IN(pic_mclr);
	GPIO_OUT(pic_mclr);

	GPIO_CLR(pic_clk);

	GPIO_CLR(pic_mclr);		/*  remove VDD from MCLR pin */
	delay_us(DELAY_P6);
	GPIO_SET(pic_mclr);		/*  apply VDD to MCLR pin */
	delay_us(DELAY_P21);
	GPIO_CLR(pic_mclr);		/* remove VDD from MCLR pin */
	delay_us(DELAY_P18);

	/* Shift in the "enter program mode" key sequence (MSB first) */
	for (i = 31; i > -1; i--) {
		if ( (ENTER_PROGRAM_KEY >> i) & 0x01 )
			GPIO_SET(pic_data);
		else
			GPIO_CLR(pic_data);
		delay_us(DELAY_P1A);
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1B);
		GPIO_CLR(pic_clk);

	}
	GPIO_CLR(pic_data);
	delay_us(DELAY_P19);
	GPIO_SET(pic_mclr);
	delay_us(DELAY_P7);

	/* idle for 5 clock cycles */
	for (i = 0; i < 5; i++) {
		GPIO_SET(pic_clk);
		delay_us(DELAY_P1B);
		GPIO_CLR(pic_clk);
		delay_us(DELAY_P1A);
	}

}

/* exit program mode */
void dspic33ck::exit_program_mode(void)
{
	GPIO_CLR(pic_clk);
	GPIO_CLR(pic_data);
	delay_us(DELAY_P16);
	GPIO_CLR(pic_mclr);		/* remove VDD from MCLR pin */
	delay_us(DELAY_P17);	/* wait (at least) P17 */
	GPIO_SET(pic_mclr);
	GPIO_IN(pic_mclr);
}

/* read the device ID and revision; returns only the id */
bool dspic33ck::read_device_id(void)
{
	bool found = 0;

	uint32_t addr = 0xFF0000;

	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	send_cmd(0x200000 | ((addr & 0x00FF0000) >> 12));	// MOV #<Address23:16>, W0
	send_cmd(0x20FCC7);
	send_cmd(0x8802A0);
	send_cmd(0x200006 | ((addr & 0x0000FFFF) << 4));	// MOV #<Address15:0>, W6

	send_nop();
	send_cmd(0xBA8B96);
	send_nop();
	send_nop();
	send_nop();
	send_nop();
	send_nop();
	device_rev = read_data();
	send_cmd(0xBA0B96);
	send_nop();
	send_nop();
	send_nop();
	send_nop();
	send_nop();
	device_id = read_data();

	fprintf(stderr, "devid: 0x%04x , devrev: 0x%04x\n", device_id, device_rev);

	for (unsigned short i=0;i < sizeof(piclist)/sizeof(piclist[0]);i++){

		if (piclist[i].device_id == device_id){

			strcpy(name, piclist[i].name);
			mem.code_memory_size = piclist[i].code_memory_size;
			if (mem.code_memory_size == 0x005EFF)
			{
				mem.program_memory_size = 0x005FFF;
			}
			else
			{
				mem.program_memory_size = 0x00AFFF;
			}
			if (i <= 5)
			{
				subfamily = SF_DSPIC33CKxxMP;
			}
			else
			{
				subfamily = SF_DSPIC33CKxxMC;
			}
			if (flags.debug)
				fprintf(stderr, "program memory: 0x%06x, subfamily: %d\n", mem.program_memory_size, subfamily);

			mem.location = (uint16_t*) calloc(mem.program_memory_size,sizeof(uint16_t));
			mem.filled = (bool*) calloc(mem.program_memory_size,sizeof(bool));
			found = 1;
			break;
		}
	}

	return found;

}

/* Check if the device is blank */
uint8_t dspic33ck::blank_check(void)
{
	// TODO

	uint32_t addr;
	unsigned short i;
	uint16_t data[8], raw_data[6];
	uint8_t ret = 0;

	if(!flags.debug) cerr << "[ 0%]";

	counter=0;

	/* exit reset vector */
	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	/* Output data to W0:W5; repeat until all desired code memory is read. */
	for(addr=0; addr < mem.code_memory_size; addr=addr+8) {

		if((addr & 0x0000FFFF) == 0){
			send_cmd(0x200000 | ((addr & 0x00FF0000) >> 12) );	// MOV #<DestAddress23:16>, W0
			send_cmd(0x8802A0);									// MOV W0, TBLPAG
			send_cmd(0x200006 | ((addr & 0x0000FFFF) << 4) );	// MOV #<DestAddress15:0>, W6
		}

		/* Fetch the next four memory locations and put them to W0:W5 */
		send_cmd(0xEB0380);	// CLR W7
		send_nop();
		send_cmd(0xBA1B96);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBD6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBA1BB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBA1B96);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBD6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBA0BB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();

		/* read six data words (16 bits each) */
		for(i=0; i<6; i++){
			send_cmd(0x887C40 + i);
			send_nop();
			raw_data[i] = read_data();
			send_nop();
		}

		send_nop();
		send_nop();
		send_nop();
		reset_pc();
		send_nop();
		send_nop();
		send_nop();

		/* store data correctly */
		data[0] = raw_data[0];
		data[1] = raw_data[1] & 0x00FF;
		data[3] = (raw_data[1] & 0xFF00) >> 8;
		data[2] = raw_data[2];
		data[4] = raw_data[3];
		data[5] = raw_data[4] & 0x00FF;
		data[7] = (raw_data[4] & 0xFF00) >> 8;
		data[6] = raw_data[5];

		if(counter != addr*100/mem.code_memory_size){
			counter = addr*100/mem.code_memory_size;
			fprintf(stderr, "\b\b\b\b\b[%2d%%]", counter);	
		}

		for(i=0; i<8; i++){
			if(flags.debug)			
				fprintf(stderr, "\n addr = 0x%06X data = 0x%04X",
								(addr+i), data[i]);
			if ((i%2 == 0 && data[i] != 0xFFFF) || (i%2 == 1 && data[i] != 0x00FF)) {
				if(!flags.debug) cerr << "\b\b\b\b\b";
				ret = 1;
				addr = mem.code_memory_size + 10;
				break;
			}
		}
	}

	if(addr <= (mem.code_memory_size + 8)){
		if(!flags.debug) cerr << "\b\b\b\b\b";
		ret = 0;
	};

	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();
	
	return ret;
}

/* Bulk erase the chip */
void dspic33ck::bulk_erase(void)
{

    send_nop();
    send_nop();
    send_nop();
    reset_pc();
    send_nop();
    send_nop();
    send_nop();

	send_cmd(0x2400EA);
	send_cmd(0x88468A);
	send_nop();
	send_nop();

	send_cmd(0x200551);
	send_cmd(0x8846B1);
	send_cmd(0x200AA1);
	send_cmd(0x8846B1);
	//if (subfamily == SF_DSPIC33CKxxMP)
	//	send_cmd(0xA8F1A1);
	//else //if (subfamily == SF_DSPIC33CKxxMC)
		send_cmd(0xA8E8D1); // There seems to be something wrong with the official documentation, it says it should be 0xA8F1A1 for xxMP chips, but only 0xA8E8D1 works, as for MC chips.
	send_nop();
	send_nop();
	send_nop();

	delay_us(DELAY_P11);

	/* wait while the erase operation completes */
	do{
		send_nop();
		send_cmd(0x804680);
		send_nop();		
		send_cmd(0x887E60);
		send_nop();
		nvmcon = read_data();
		send_nop();
		send_nop();
		send_nop();
		reset_pc();
		send_nop();
		send_nop();
		send_nop();
	} while((nvmcon & 0x8000) == 0x8000);
	
	if(flags.client) fprintf(stdout, "@FIN");
}

/* Read PIC memory and write the contents to a .hex file */
// TODO
void dspic33ck::read(char *outfile, uint32_t start, uint32_t count)
{
	uint32_t addr, startaddr, stopaddr;
	uint16_t data[8], raw_data[6];
	int i=0;

	startaddr = start;
	stopaddr = mem.code_memory_size;
	if(count != 0 && count < stopaddr){
		stopaddr = startaddr + count;
		fprintf(stderr, "Read only %d memory locations, from %06X to %06X\n",
				count, startaddr, stopaddr);
	}

	if(!flags.debug) cerr << "[ 0%]";
	if(flags.client) fprintf(stdout, "@000");
	counter=0;

	/* exit reset vector */
	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	/* Output data to W0:W5; repeat until all desired code memory is read. */
	for(addr=startaddr; addr < stopaddr; addr=addr+8) {

		if((addr & 0x0000FFFF) == 0 || startaddr != 0){
			send_cmd(0x200000 | ((addr & 0x00FF0000) >> 12) );	// MOV #<DestAddress23:16>, W0
			send_cmd(0x8802A0);									// MOV W0, TBLPAG
			send_cmd(0x200006 | ((addr & 0x0000FFFF) << 4) );	// MOV #<DestAddress15:0>, W6
			startaddr = 0;
		}

		/* Fetch the next four memory locations and put them to W0:W5 */
		send_cmd(0xEB0380);	// CLR W7
		send_nop();
		send_cmd(0xBA1B96);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBD6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBA1BB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBA1B96);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBADBD6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_cmd(0xBA0BB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();

		/* read six data words (16 bits each) */
		for(i=0; i<6; i++){
			send_cmd(0x887E60 + i);
			send_nop();
			raw_data[i] = read_data();
			send_nop();
		}

		send_nop();
		send_nop();
		send_nop();
		reset_pc();
		send_nop();
		send_nop();
		send_nop();

		/* store data correctly */
		data[0] = raw_data[0];
		data[1] = raw_data[1] & 0x00FF;
		data[3] = (raw_data[1] & 0xFF00) >> 8;
		data[2] = raw_data[2];
		data[4] = raw_data[3];
		data[5] = raw_data[4] & 0x00FF;
		data[7] = (raw_data[4] & 0xFF00) >> 8;
		data[6] = raw_data[5];

		for(i=0; i<8; i++){
			if (flags.debug)
				fprintf(stderr, "\n addr = 0x%06X data = 0x%04X",
						(addr+i), data[i]);

			if (i%2 == 0 && data[i] != 0xFFFF) {
				mem.location[addr+i]        = data[i];
				mem.filled[addr+i]      = 1;
			}

			if (i%2 == 1 && data[i] != 0x00FF) {
				mem.location[addr+i]        = data[i];
				mem.filled[addr+i]      = 1;
			}
		}

		if(counter != addr*100/stopaddr){
			counter = addr*100/stopaddr;
			if(flags.client)
				fprintf(stdout,"@%03d", counter);
			if(!flags.debug)
				fprintf(stderr,"\b\b\b\b\b[%2d%%]", counter);
		}

		/* TODO: checksum */
	}


	//addr = 0x00F80004;

	/*
	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	send_cmd(0x200F80);
	send_cmd(0x8802A0);
	send_cmd(0x200046);
	send_cmd(0x20F887);
	send_nop();

	for (i = 0; i<8; i++) {
		send_cmd(0xBA0BB6);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		data[0] = read_data();
		if (data[0] != 0xFFFF) {
			mem.location[addr+2*i] = data[0];
			mem.filled[addr+2*i] = 1;
		}
	}*/

	addr = 0x00005F00;
	if (mem.program_memory_size == 0x00AFFF)
		addr = 0x0000AF00;

	for (unsigned short i = 0; i < 15; addr += 4, i += 1) {

		if (i == 1)
			addr += 12; // jump to offset 0x10 after first config value

		send_nop();
		send_nop();
		send_nop();
		reset_pc();
		send_nop();
		send_nop();
		send_nop();

		send_cmd(0x200000 | ((addr & 0x00FF0000) >> 12));	// MOV #<Address23:16>, W0
		send_cmd(0x20FCC7);
		send_cmd(0x8802A0);
		send_cmd(0x200006 | ((addr & 0x0000FFFF) << 4));	// MOV #<Address15:0>, W6

		send_nop();
		send_cmd(0xBA8B96);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		uint16_t data_1 = read_data() | 0xFF00;

		if (data_1 != 0xFFFF) {
			mem.location[addr] = data_1;
			mem.filled[addr] = 1;
		}

		send_cmd(0xBA0B96);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		uint16_t data_2 = read_data();

		if (data_2 != 0xFFFF) {
			mem.location[addr + 1] = data_2;
			mem.filled[addr + 1] = 1;
		}

		//fprintf(stderr, " - %s: 0x%04x%04x\n", regname[i], data_1, data_2);

		cerr << endl;
	}

	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	if(!flags.debug) cerr << "\b\b\b\b\b";
	if(flags.client) fprintf(stdout, "@FIN");
	write_inhx(&mem, outfile);
}

/* Write contents of the .hex file to the PIC */
void dspic33ck::write(char *infile)
{
	uint16_t i,j,k;
	bool skip;
	uint32_t data[8],raw_data[6];
	uint32_t addr = 0;

	unsigned int filled_locations=1;

	const char *regname[] = {"FSEC","FBSLIM","FSIGN","FOSCSEL","FOSC","FWDT","FPOR","FICD","FDMTIVTL","FDMTIVTH","FDMTCNTL","FDMTCNTH","FDMT","FDEVOPT","FALTREG"};

	filled_locations = read_inhx(infile, &mem);
	if(!filled_locations) return;

	bulk_erase();

	/* Exit reset vector */
	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	/* WRITE CODE MEMORY */
	if(!flags.debug) cerr << "[ 0%]";
	if(flags.client) fprintf(stdout, "@000");
	counter=0;

	for (addr = 0; addr < mem.code_memory_size; ){

		skip = 1;

		for(k=0; k<256; k+=2)
			if(mem.filled[addr+k]) skip = 0;

		if(skip){
			addr=addr+256;
			continue;
		}

		send_cmd(0x200FAC);
		send_cmd(0x8802AC);

		for(j=0;j<4;j++){
			if (mem.filled[addr+j]) data[j] = mem.location[addr+j];
			else data[j] = 0xFFFF;
			if (flags.debug)
				fprintf(stderr,"\n  Writing 0x%04X to address 0x%06X ", data[j], addr+j );
		}

		send_cmd(0x200000 | (data[0] << 4));										// MOV #<LSW0>, W0
		send_cmd(0x200001 | (0x00FFFF & ((data[3] << 8) | (data[1] & 0x00FF))) <<4);// MOV #<MSB1:MSB0>, W1
		send_cmd(0x200002 | (data[2] << 4));										// MOV #<LSW1>, W2

		/* set W6+W7 and load latches */
		send_cmd(0xEB0300);
		send_nop();
		send_cmd(0xEB0380);
		send_nop();
		send_cmd(0xBB0BB6);
		send_nop();
		send_nop();
		send_cmd(0xBBDBB6);
		send_nop();
		send_nop();
		send_cmd(0xBBEBB6);
		send_nop();
		send_nop();
		send_cmd(0xBB0B96);
		send_nop();
		send_nop();

		/* Set the NVMADRU/NVMADR register-pair to point to the correct row */
		send_cmd(0x200003 | ((addr & 0x0000FFFF) << 4));
		send_cmd(0x200004 | ((addr & 0x00FF0000) >> 12));
		send_cmd(0x884693);
		send_cmd(0x8846A4);

		/* Set the NVMCON to program two instruction words */
		send_cmd(0x24001A);
		send_nop();
		send_cmd(0x88468A);
		send_nop();
		send_nop();

		/* Initiate the write cycle */
		send_cmd(0x200551);
		send_cmd(0x8846B1);
		send_cmd(0x200AA1);
		send_cmd(0x8846B1);
		//if (subfamily == SF_DSPIC33CKxxMP)
		//	send_cmd(0xA8F1A1);
		//else //if (subfamily == SF_DSPIC33CKxxMC)
			send_cmd(0xA8E8D1); // There seems to be something wrong with the official documentation, it says it should be 0xA8F1A1 for xxMP chips, but only 0xA8E8D1 works, as for MC chips.
		send_nop();
		send_nop();
		send_nop();

		delay_us(DELAY_P13);

		// Wait until finished
		do{
			send_nop();
			send_cmd(0x804680);
			send_nop();
			send_cmd(0x887E60);
			send_nop();
			nvmcon = read_data();
			send_nop();
			send_nop();
			send_nop();
			reset_pc();
			send_nop();
			send_nop();
			send_nop();
		} while((nvmcon & 0x8000) == 0x8000);

		if(counter != addr*100/filled_locations){
			if(flags.client)
				fprintf(stdout,"@%03d", (addr*100/(filled_locations+0x100)));
			if(!flags.debug)
				fprintf(stderr,"\b\b\b\b\b[%2d%%]", addr*100/(filled_locations+0x100));
			counter = addr*100/filled_locations;
		}

		addr += 4;
	};

	if(!flags.debug) cerr << "\b\b\b\b\b\b";
	if(flags.client) fprintf(stdout, "@100");

	delay_us(100000);

	/* WRITE CONFIGURATION REGISTERS */
	if(flags.debug)
		cerr << endl << "Writing Configuration registers..." << endl;

	send_nop();
	send_nop();
	send_nop();
	reset_pc();
	send_nop();
	send_nop();
	send_nop();

	send_cmd(0x200FAC);
	send_cmd(0x8802AC);

	addr = 0x00005F00;
	if (mem.program_memory_size == 0x00AFFF)
		addr = 0x0000AF00;

	for(i=0; i<15; i+=1, addr+=4){

		if (i == 1)
			addr += 12; // jump to 0x10 offset after first register

		if(mem.filled[addr]){

			send_cmd(0x200000 | ((0x0000FFFF & mem.location[addr]) << 4));
			send_cmd(0x200001 | ((0x0000FFFF & mem.location[addr+1]) << 4));
			send_cmd(0x200002 | ((0x0000FFFF & mem.location[addr+2]) << 4));
			send_cmd(0x200003 | ((0x0000FFFF & mem.location[addr+3]) << 4));

			/* set W3 and load latches */
			send_cmd(0xEB0300);
			send_nop();
			send_cmd(0xBB0B00);
			send_nop();
			send_nop();
			send_cmd(0xBB9B01);
			send_nop();
			send_nop();
			send_cmd(0xBB0B02);
			send_nop();
			send_nop();
			send_cmd(0xBB9B03);
			send_nop();
			send_nop();

			/* Set the NVMADRU/NVMADR register-pair to point to the correct row */
			send_cmd(0x200004 | ((addr & 0x0000FFFF) << 4));
			send_cmd(0x200005 | ((addr & 0x00FF0000) >> 12));
			send_cmd(0x884694);
			send_cmd(0x8846A5);

			/* Set the NVMCON register to program one Configuration register */
			send_cmd(0x24001A);
			send_nop();
			send_cmd(0x88468A);
			send_nop();
			send_nop();

			/* Initiate the write cycle */
			send_cmd(0x200551);
			send_cmd(0x8846B1);
			send_cmd(0x200AA1);
			send_cmd(0x8846B1);
			//if (subfamily == SF_DSPIC33CKxxMP)
			//	send_cmd(0xA8F1A1);
			//else //if (subfamily == SF_DSPIC33CKxxMC)
				send_cmd(0xA8E8D1); // There seems to be something wrong with the official documentation, it says it should be 0xA8F1A1 for xxMP chips, but only 0xA8E8D1 works, as for MC chips.
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();

			delay_us(DELAY_P20);

			// Wait until finished
			do{
				send_nop();
				send_cmd(0x804680);
				send_nop();
				send_cmd(0x887E60);
				send_nop();
				nvmcon = read_data();
				send_nop();
				send_nop();
				send_nop();
				reset_pc();
				send_nop();
				send_nop();
				send_nop();
			} while((nvmcon & 0x8000) == 0x8000);

			if (flags.debug)
			{
				fprintf(stderr, "\n - %s set to 0x%01x, 0x%01x, 0x%01x, 0x%01x",
					regname[i], mem.location[addr], mem.location[addr+1], mem.location[addr+2], mem.location[addr+3]);
			}
		}
		else if(flags.debug)
				fprintf(stderr,"\n - %s left unchanged", regname[i]);

	}

	if(flags.debug) cerr << endl;

	delay_us(100000);

	/* VERIFY CODE MEMORY */
	if(!flags.noverify){
		if(!flags.debug) cerr << "[ 0%]";
		if(flags.client) fprintf(stdout, "@000");
		counter = 0;

		send_nop();
		send_nop();
		send_nop();
		reset_pc();
		send_nop();
		send_nop();
		send_nop();

		for(addr=0; addr < mem.code_memory_size; addr=addr+8) {

			skip=1;

			for(k=0; k<8; k+=2)
				if(mem.filled[addr+k])
					skip = 0;

			if(skip) continue;

			send_cmd(0x200000 | ((addr & 0x00FF0000) >> 12) );	// MOV #<DestAddress23:16>, W0
			send_cmd(0x8802A0);									// MOV W0, TBLPAG
			send_cmd(0x200006 | ((addr & 0x0000FFFF) << 4) );	// MOV #<DestAddress15:0>, W6

			/* Fetch the next four memory locations and put them to W0:W5 */
			send_cmd(0xEB0380);	// CLR W7
			send_nop();
			send_cmd(0xBA1B96);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_cmd(0xBADBB6);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_cmd(0xBADBD6);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_cmd(0xBA1BB6);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_cmd(0xBA1B96);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_cmd(0xBADBB6);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_cmd(0xBADBD6);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_cmd(0xBA0BB6);
			send_nop();
			send_nop();
			send_nop();
			send_nop();
			send_nop();

			/* read six data words (16 bits each) */
			for(i=0; i<6; i++){
				send_cmd(0x887E60 + i);
				send_nop();
				raw_data[i] = read_data();
				send_nop();
			}

			send_nop();
			send_nop();
			send_nop();
			reset_pc();
			send_nop();
			send_nop();
			send_nop();

			/* store data correctly */
			data[0] = raw_data[0];
			data[1] = raw_data[1] & 0x00FF;
			data[3] = (raw_data[1] & 0xFF00) >> 8;
			data[2] = raw_data[2];
			data[4] = raw_data[3];
			data[5] = raw_data[4] & 0x00FF;
			data[7] = (raw_data[4] & 0xFF00) >> 8;
			data[6] = raw_data[5];

			for(i=0; i<8; i++){
				if (flags.debug)
					fprintf(stderr, "\n addr = 0x%06X data = 0x%04X", (addr+i), data[i]);

				if(mem.filled[addr+i] && data[i] != mem.location[addr+i]){
					fprintf(stderr,"\n\n ERROR at address %06X: written %04X but %04X read!\n\n",
									addr+i, mem.location[addr+i], data[i]);
					return;
				}

			}

			if(counter != addr*100/filled_locations){
				if(flags.client)
					fprintf(stdout,"@%03d", (addr*100/(filled_locations+0x100)));
				if(!flags.debug)
					fprintf(stderr,"\b\b\b\b\b[%2d%%]", addr*100/(filled_locations+0x100));
				counter = addr*100/filled_locations;
			}
		}

		if(!flags.debug) cerr << "\b\b\b\b\b";
		if(flags.client) fprintf(stdout, "@FIN");
	}
	else{
		if(flags.client) fprintf(stdout, "@FIN");
	}

}

/* write to screen the configuration registers, without saving them anywhere */
void dspic33ck::dump_configuration_registers(void)
{
	const char* regname[] = { "FSEC","FBSLIM","FSIGN","FOSCSEL","FOSC","FWDT","FPOR","FICD","FDMTIVTL","FDMTIVTH","FDMTCNTL","FDMTCNTH","FDMT","FDEVOPT","FALTREG" };

	cerr << endl << "Configuration registers:" << endl << endl;

	uint32_t addr = 0x00005F00;
	if (mem.program_memory_size == 0x00AFFF)
		addr = 0x0000AF00;

	for (unsigned short i = 0; i < 15; addr += 4, i += 1) {

		if (i == 1)
			addr += 12; // jump to offset 0x10 after first config value

		send_nop();
		send_nop();
		send_nop();
		reset_pc();
		send_nop();
		send_nop();
		send_nop();

		send_cmd(0x200000 | ((addr & 0x00FF0000) >> 12));	// MOV #<Address23:16>, W0
		send_cmd(0x20FCC7);
		send_cmd(0x8802A0);
		send_cmd(0x200006 | ((addr & 0x0000FFFF) << 4));	// MOV #<Address15:0>, W6

		send_nop();
		send_cmd(0xBA8B96);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		uint16_t data_1 = read_data() | 0xFF00;
		send_cmd(0xBA0B96);
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		send_nop();
		uint16_t data_2 = read_data();
		fprintf(stderr, " - %s: 0x%04x%04x\n", regname[i], data_1, data_2);

		cerr << endl;
	}
}

