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

#include <iostream>

#include "../common.h"
#include "device.h"

using namespace std;

#define SF_DSPIC33CK32		0x00
#define SF_DSPIC33CK64		0x01

class dspic33ck : public Pic{

	public:
		void enter_program_mode(void);
		void exit_program_mode(void);
		bool setup_pe(void){return true;};
		bool read_device_id(void);
		void bulk_erase(void);
		void dump_configuration_registers(void);
		void read(char *outfile, uint32_t start, uint32_t count);
		void write(char *infile);
		uint8_t blank_check(void);

	protected:
		void send_cmd(uint32_t cmd);
		uint16_t read_data(void);

		/*
		* DEVICES SECTION
		*                       ID       NAME           	  MEMSIZE
		*/
		pic_device piclist[6] = {{0x8E00, "dsPIC33CK32MP102", 0x005EFF},
								  {0x8E01, "dsPIC33CK32MP103", 0x005EFF},
								  {0x8E02, "dsPIC33CK32MP105", 0x005EFF},
								  {0x8E10, "dsPIC33CK64MP102", 0x00AEFF},
								  {0x8E11, "dsPIC33CK64MP103", 0x00AEFF},
								  {0x8E12, "dsPIC33CK64MP105", 0x00AEFF}};
};
