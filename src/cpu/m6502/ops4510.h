/*****************************************************************************
 *
 *	 ops4510.h
 *	 Addressing mode and opcode macros for 4510 CPU
 *
 *	 Copyright (c) 2000 Peter Trauner, all rights reserved.
 *   documentation preliminary databook
 *	 documentation by michael steil mist@c64.org
 *	 available at ftp://ftp.funet.fi/pub/cbm/c65
 *
 *	 - This source code is released as freeware for non-commercial purposes.
 *	 - You are free to use and redistribute this code in modified or
 *	   unmodified form, provided you list me in the credits.
 *	 - If you modify this source code, you must add a notice to each modified
 *	   source file that it has been changed.  If you're a nice person, you
 *	   will clearly mark each change too.  :)
 *	 - If you wish to use this for commercial purposes, please contact me at
 *	   pullmoll@t-online.de
 *	 - The author of this copywritten work reserves the right to change the
 *	   terms of its usage and license at any time, including retroactively
 *	 - This entire notice must remain in the source code.
 *
 *****************************************************************************/

#define m65ce02 m4510
#define m65ce02_ICount m4510_ICount

#define M4510_MEM(addr) (m4510.mem[(addr)>>13]+(addr))

#undef CHANGE_PC
#define CHANGE_PC change_pc20(M4510_MEM(PCD))

/***************************************************************
 *  RDOP    read an opcode
 ***************************************************************/
#undef RDOP
#define RDOP() cpu_readop(m4510.mem[PCD>>13]+PCW++)

/***************************************************************
 *  RDOPARG read an opcode argument
 ***************************************************************/
#undef RDOPARG
#define RDOPARG() cpu_readop_arg(m4510.mem[PCD>>13]+PCW++)

/***************************************************************
 *  RDMEM   read memory
 ***************************************************************/
#undef RDMEM
#define RDMEM(addr) cpu_readmem20(M4510_MEM(addr))

/***************************************************************
 *  WRMEM   write memory
 ***************************************************************/
#undef WRMEM
#define WRMEM(addr,data) cpu_writemem20(M4510_MEM(addr),data)

#undef MAP
#define MAP 																\
{																			\
	UINT16 low, high;														\
	low = m4510.low;														\
	high = m4510.high;														\
	m4510.low = m4510.a | (m4510.x<<8); 									\
	m4510.high = m4510.y | (m4510.z<<8);									\
	m4510.a = low & 0xff;													\
	m4510.x = low >> 8; 													\
	m4510.y = high & 0xff;													\
	m4510.z = high >> 8;													\
	m4510.mem[0] = (m4510.low & 0x1000) ? (m4510.low & 0xfff) << 8 : 0; 	\
	m4510.mem[1] = (m4510.low & 0x2000) ? (m4510.low & 0xfff) << 8 : 0; 	\
	m4510.mem[2] = (m4510.low & 0x4000) ? (m4510.low & 0xfff) << 8 : 0; 	\
	m4510.mem[3] = (m4510.low & 0x8000) ? (m4510.low & 0xfff) << 8 : 0; 	\
	m4510.mem[4] = (m4510.high & 0x1000) ? (m4510.high & 0xfff) << 8 : 0;	\
	m4510.mem[5] = (m4510.high & 0x2000) ? (m4510.high & 0xfff) << 8 : 0;	\
	m4510.mem[6] = (m4510.high & 0x4000) ? (m4510.high & 0xfff) << 8 : 0;	\
	m4510.mem[7] = (m4510.high & 0x8000) ? (m4510.high & 0xfff) << 8 : 0;	\
	CHANGE_PC;																\
}

