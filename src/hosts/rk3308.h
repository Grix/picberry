/*
 * Raspberry Pi PIC Programmer using GPIO connector
 * https://github.com/WallaceIT/picberry
 * Copyright 2016 Francesco Valla
 * Copyright 2024 Gitle Mikkelsen
 *
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

// RK3308, used in Rock Pi S, Rock S0 etc. 
// NB: Mapped to GPIO2, pin index must be 0 to 31

/* GPIO registers address */
#define GPIO_BASE          (0xff240000) /* GPIO controller */
#define BLOCK_SIZE         (0x80)
#define PORTOFFSET         0

/* GPIO setup macros. Always use GPIO_IN(x) before using GPIO_OUT(x) */
#define GPIO_IN(g)		*(gpio + (0x04 / 4)) &= ~(1 << (g & 0xFF))
#define GPIO_OUT(g)		*(gpio + (0x04 / 4)) |= (1 << (g & 0xFF))

#define GPIO_SET(g)		*(gpio + (0x00 / 4)) |= (1 << (g & 0xFF))
#define GPIO_CLR(g)		*(gpio + (0x00 / 4)) &= ~(1 << (g & 0xFF))
#define GPIO_LEV(g)		((*(gpio + (0x50 / 4)) >> (g & 0xFF)) & 1)

/* default GPIO <-> PIC connections */
#define DEFAULT_PIC_CLK    16    // PGC - Output
#define DEFAULT_PIC_DATA   10    // PGD - I/O 
#define DEFAULT_PIC_MCLR   4	 // MCLR - Output 
