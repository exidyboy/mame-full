/***************************************************************************

PK-01 Lviv driver by Krzysztof Strzecha

Notes:
1. Only one colour palette is emulated.
2. .lvt tape images are not supported.
3. LIST command crash Lviv.
4. Printer is not emulated. 

Lviv technical information
--------------------------
CPU:
		8080 2MHz

Memory map:
		0000-3fff RAM
		4000-7fff RAM / Video RAM
		8000-bfff RAM
		c000-ffff ROM
		
Interrupts:
	IRQ:
		50Hz vertical sync (?)

Ports:
	C0-C3	8255 PPI
		Port A:
		Port B:
		Port C:
			bit 7: not used
			bit 6: not used
			bit 5: not used
			bit 4: tape in
			bit 3: not used
			bit 2: not used
			bit 1: 0 - video ram, 1 - ram
			bit 0: tape out
	D0-D3	8255 PPI
		Port A:
			keyboard scaning
		Port B:
			keyboard reading
		Port C:
			keyboard scaning/reading

***************************************************************************/

#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "machine/8255ppi.h"
#include "vidhrdw/generic.h"
#include "includes/lviv.h"

/* I/O ports */

PORT_READ_START( lviv_readport )
	{ 0xc0, 0xc3, ppi8255_0_r },
	{ 0xd0, 0xd3, ppi8255_1_r },
PORT_END

PORT_WRITE_START( lviv_writeport )
	{ 0xc0, 0xc3, ppi8255_0_w },
	{ 0xd0, 0xd3, ppi8255_1_w },
PORT_END

/* memory w/r functions */

MEMORY_READ_START( lviv_readmem )
	{0x0000, 0x3fff, MRA_BANK1},
	{0x4000, 0x7fff, MRA_BANK2},
	{0x8000, 0xbfff, MRA_BANK3},
	{0xc000, 0xffff, MRA_BANK4},
MEMORY_END

MEMORY_WRITE_START( lviv_writemem )
	{0x0000, 0x3fff, MWA_BANK5},
	{0x4000, 0x7fff, MWA_BANK6},
	{0x8000, 0xbfff, MWA_BANK7},
	{0xc000, 0xffff, MWA_BANK8},
MEMORY_END


/* keyboard input */
INPUT_PORTS_START (lviv)
	PORT_START /* 2nd PPI port A bit 0 low */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "6",	KEYCODE_6,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "7",	KEYCODE_7,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8",	KEYCODE_8,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Unk",	KEYCODE_INSERT,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Tab",	KEYCODE_TAB,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "=",	KEYCODE_EQUALS,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "0",	KEYCODE_0,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "9",	KEYCODE_9,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port A bit 1 low */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "G",	KEYCODE_G,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "[",	KEYCODE_OPENBRACE,	IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "]",	KEYCODE_CLOSEBRACE,	IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Enter",	KEYCODE_ENTER,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Run",	KEYCODE_DEL,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "*",	KEYCODE_QUOTE,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "H",	KEYCODE_H,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z",	KEYCODE_Z,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port A bit 2 low */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "R",	KEYCODE_R,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O",	KEYCODE_O,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "L",	KEYCODE_L,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Back",	KEYCODE_BACKSPACE,	IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ".",	KEYCODE_STOP,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\",	KEYCODE_BACKSLASH,	IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "V",	KEYCODE_V,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "D",	KEYCODE_D,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port A bit 3 low */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Space",	KEYCODE_SPACE,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "B",	KEYCODE_B,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "@",	KEYCODE_ESC,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Symbol",	KEYCODE_RSHIFT,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "_",	KEYCODE_MINUS,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Latin",	KEYCODE_RCONTROL,	IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "/",	KEYCODE_SLASH,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, ",",	KEYCODE_COMMA,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port A bit 4 low */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cls",	KEYCODE_HOME,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "~G Unk",	KEYCODE_F1,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "~B Unk",	KEYCODE_F2,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "5",	KEYCODE_5,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "4",	KEYCODE_4,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "3",	KEYCODE_3,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "2",	KEYCODE_2,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "1",	KEYCODE_1,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port A bit 5 low */
		PORT_BIT (0x01, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x02, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "J",	KEYCODE_J,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "N",	KEYCODE_N,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "E",	KEYCODE_E,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "K",	KEYCODE_K,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "U",	KEYCODE_U,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "C",	KEYCODE_C,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port A bit 6 low */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, ";",	KEYCODE_COLON,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Russian",	KEYCODE_LCONTROL,	IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Keyword",	KEYCODE_CAPSLOCK,	IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "P",	KEYCODE_P,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "A",	KEYCODE_A,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "W",	KEYCODE_W,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y",	KEYCODE_Y,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "F",	KEYCODE_F,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port A bit 7 low */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Unk",	KEYCODE_LSHIFT,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q",	KEYCODE_Q,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "^",	KEYCODE_TILDE,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "X",	KEYCODE_X,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T",	KEYCODE_T,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "I",	KEYCODE_I,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "M",	KEYCODE_M,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "S",	KEYCODE_S,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port C bit 0 low */
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Unk",	KEYCODE_F6,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Unk",	KEYCODE_F5,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Sound",	KEYCODE_F4,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "~R Unk",	KEYCODE_F3,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port C bit 1 low */
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Unk",	KEYCODE_F7,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "F0",	KEYCODE_F8,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Cload",	KEYCODE_F9,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Csave",	KEYCODE_F10,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port C bit 2 low */
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Arrow",	KEYCODE_END,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Unk",	KEYCODE_SCRLOCK,	IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Edit",	KEYCODE_F12,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "List",	KEYCODE_F11,		IP_JOY_NONE )
	PORT_START /* 2nd PPI port C bit 3 low */
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Right",	KEYCODE_RIGHT,		IP_JOY_NONE )
		PORT_BITX(0x20, IP_ACTIVE_LOW, IPT_KEYBOARD, "Up",	KEYCODE_UP,		IP_JOY_NONE )
		PORT_BITX(0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Left",	KEYCODE_LEFT,		IP_JOY_NONE )
		PORT_BITX(0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Down",	KEYCODE_DOWN,		IP_JOY_NONE )
INPUT_PORTS_END

static struct Speaker_interface lviv_speaker_interface=
{
 1,
 {50},
};

static struct Wave_interface lviv_wave_interface = {
	1,		/* 1 cassette recorder */
	{ 50 }		/* mixing levels in percent */
};

/* machine definition */

static	struct MachineDriver machine_driver_lviv =
{
	/* basic machine hardware */
	{
		{
			CPU_8080,
			2000000,				  
			lviv_readmem, lviv_writemem,
			lviv_readport, lviv_writeport,
			0, 0,
		},
	},
	50,					/* frames per second */
	DEFAULT_60HZ_VBLANK_DURATION,		/* vblank duration */
	1,
	lviv_init_machine,
	lviv_stop_machine,

	/* video hardware */
	256,					/* screen width */
	256,					/* screen height */
	{0, 256 - 1, 0, 256 - 1},		/* visible_area */
	0,					/* graphics decode info */
	sizeof (lviv_palette) / 3,
	sizeof (lviv_colortable),		/* colors used for the characters */
	lviv_init_palette,			/* initialise palette */

	VIDEO_TYPE_RASTER,
	0,
	lviv_vh_start,
	lviv_vh_stop,
	lviv_vh_screenrefresh,

	/* sound hardware */
	0, 0, 0, 0,
	{
		{
			SOUND_SPEAKER,
			&lviv_speaker_interface,
		},
		{
			SOUND_WAVE,
			&lviv_wave_interface
		}
	}
};

static const struct IODevice io_lviv[] = {
    {
	IO_QUICKLOAD,		/* type */
	1,			/* count */
	"lvt\0",        	/* file extensions */
	IO_RESET_NONE,		/* reset if file changed */
        NULL,               	/* id */
	lviv_snap_load,		/* init */
	lviv_snap_exit,		/* exit */
        NULL,		        /* info */
        NULL,           	/* open */
        NULL,               	/* close */
        NULL,               	/* status */
        NULL,               	/* seek */
	NULL,			/* tell */
        NULL,           	/* input */
        NULL,               	/* output */
        NULL,               	/* input_chunk */
        NULL                	/* output_chunk */
    },
    IO_CASSETTE_WAVE(1,"wav\0",NULL,lviv_tape_init,lviv_tape_exit),
    { IO_END }
};

#define io_lviv2 io_lviv

ROM_START(lviv)
	ROM_REGION(0x14000,REGION_CPU1,0)
	ROM_LOAD("lviv.bin", 0x10000, 0x4000, 0xf171c282)
ROM_END

ROM_START(lviv2)
	ROM_REGION(0x14000,REGION_CPU1,0)
	ROM_LOAD("lviv2.bin", 0x10000, 0x4000, 0x44a347d9)
ROM_END

/*    YEAR    NAME  PARENT  MACHINE   INPUT  INIT  COMPANY     FULLNAME */
COMP( 1986, lviv,      0,  lviv, lviv,    0,      "", "PK-01 Lviv" )
COMP( 1986, lviv2,  lviv,  lviv, lviv,    0,      "", "PK-01 Lviv rev. 2" )
