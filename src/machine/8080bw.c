/***************************************************************************

  machine.c

  Functions to emulate general aspects of the machine (RAM, ROM, interrupts,
  I/O ports)

***************************************************************************/

#include "driver.h"


static int shift_data1,shift_data2,shift_amount;


void invaders_shift_amount_w(int offset,int data)
{
	shift_amount = data;
}

void invaders_shift_data_w(int offset,int data)
{
	shift_data2 = shift_data1;
	shift_data1 = data;
}


#define SHIFT  (((((shift_data1 << 8) | shift_data2) << (shift_amount & 0x07)) >> 8) & 0xff)


int invaders_shift_data_r(int offset)
{
	return SHIFT;
}

int invaders_shift_data_rev_r(int offset)
{
	int	ret = SHIFT;

	ret = ((ret & 0x01) << 7)
	    | ((ret & 0x02) << 5)
	    | ((ret & 0x04) << 3)
	    | ((ret & 0x08) << 1)
	    | ((ret & 0x10) >> 1)
	    | ((ret & 0x20) >> 3)
	    | ((ret & 0x40) >> 5)
	    | ((ret & 0x80) >> 7);

	return ret;
}

int invaders_shift_data_comp_r(int offset)
{
	return SHIFT ^ 0xff;
}


int invaders_interrupt(void)
{
	static int count;

	count++;

	if (count & 1)
		return 0x00cf;  /* RST 08h */
	else
		return 0x00d7;  /* RST 10h */
}

/****************************************************************************
	Extra / Different functions for Boot Hill                (MJC 300198)
****************************************************************************/

int boothill_shift_data_r(int offset)
{
	if (shift_amount < 0x10)
		return invaders_shift_data_r(0);
    else
    	return invaders_shift_data_rev_r(0);
}

/* Grays Binary again! */

static const int BootHillTable[8] = {
	0x00, 0x40, 0x60, 0x70, 0x30, 0x10, 0x50, 0x50
};

int boothill_port_0_r(int offset)
{
    return (input_port_0_r(0) & 0x8F) | BootHillTable[input_port_3_r(0) >> 5];
}

int boothill_port_1_r(int offset)
{
    return (input_port_1_r(0) & 0x8F) | BootHillTable[input_port_4_r(0) >> 5];
}

/*
 * Space Encounters uses rotary controllers on input ports 0 & 1
 * each controller responds 0-63 for reading, with bit 7 as
 * fire button.
 *
 * The controllers look like they returns Grays binary,
 * so I use a table to translate my simple counter into it!
 */

static const int ControllerTable[64] = {
    0  , 1  , 3  , 2  , 6  , 7  , 5  , 4  ,
    12 , 13 , 15 , 14 , 10 , 11 , 9  , 8  ,
    24 , 25 , 27 , 26 , 30 , 31 , 29 , 28 ,
    20 , 21 , 23 , 22 , 18 , 19 , 17 , 16 ,
    48 , 49 , 51 , 50 , 54 , 55 , 53 , 52 ,
    60 , 61 , 63 , 62 , 58 , 59 , 57 , 56 ,
    40 , 41 , 43 , 42 , 46 , 47 , 45 , 44 ,
    36 , 37 , 39 , 38 , 34 , 35 , 33 , 32
};

int gray6bit_controller0_r(int offset)
{
    return (input_port_0_r(0) & 0xc0) + (ControllerTable[input_port_0_r(0) & 0x3f] ^ 0x3f);
}

int gray6bit_controller1_r(int offset)
{
    return (input_port_1_r(0) & 0xc0) + (ControllerTable[input_port_1_r(0) & 0x3f] ^ 0x3f);
}

int seawolf_port_0_r (int offset)
{
	return (input_port_0_r(0) & 0xe0) + ControllerTable[input_port_0_r(0) & 0x1f];
}


static int desertgu_controller_select;

int desertgu_port_1_r(int offset)
{
	return readinputport(desertgu_controller_select ? 1 : 2);
}

void desertgu_controller_select_w(int offset, int data)
{
	desertgu_controller_select = data & 0x08;
}
