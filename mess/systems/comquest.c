/******************************************************************************
 Peter.Trauner@jk.uni-linz.ac.at September 2000

******************************************************************************/
/*
comquest plus
-------------
team concepts

laptop for childs

language german

lcd black/white, about 128x64, manual contrast control 
keyboard and 2 button joypad
speaker 2, manual volume control:2 levels
cartridge slot, serial port

512 kbyte rom on print with little isolation/case
12 pin chip on print with little isolation/case
cpu on print, soldered so nothing visible
32 kbyte sram

compuest a4 power printer
-------------------------
line oriented ink printer (12 pixel head)
for comquest serial port

3 buttons, 2 leds
bereit
druckqualitaet
zeilenvorschub/seitenvorschub

only chip on board (40? dil)
lsc43331op
team concepts
icq3250a-d
1f71lctctab973

 */

#include "driver.h"

#include "includes/comquest.h"

READ_HANDLER(comquest_read)
{
	UINT8 data=0;
	logerror("comquest read %.4x %.2x\n",offset,data);
	return data;
}

WRITE_HANDLER(comquest_write)
{
	logerror("comquest read %.4x %.2x\n",offset,data);
}

static struct MemoryReadAddress comquest_readmem[] =
{
//	{ 0x0000, 0x7fff, MRA_BANK1 },
	{ 0x0000, 0x7ffff, MRA_ROM },
//	{ 0x8000, 0xffff, MRA_RAM }, // batterie buffered
	MEMORY_TABLE_END
};

static struct MemoryWriteAddress comquest_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0xffff, MWA_RAM },
	MEMORY_TABLE_END
};

#define DIPS_HELPER(bit, name, keycode, r) \
   PORT_BITX(bit, IP_ACTIVE_HIGH, IPT_KEYBOARD, name, keycode, r)

INPUT_PORTS_START( comquest )
	PORT_START
	DIPS_HELPER( 0x001, "EIN", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x002, "Esc", KEYCODE_ESC, CODE_NONE)
	DIPS_HELPER( 0x004, "F1", KEYCODE_F1, CODE_NONE)
	DIPS_HELPER( 0x008, "F2", KEYCODE_F2, CODE_NONE)
	DIPS_HELPER( 0x010, "F3", KEYCODE_F3, CODE_NONE)
	DIPS_HELPER( 0x020, "F4", KEYCODE_F4, CODE_NONE)
	DIPS_HELPER( 0x040, "F5", KEYCODE_F5, CODE_NONE)
	DIPS_HELPER( 0x080, "F6", KEYCODE_F6, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x001, "F7", KEYCODE_F7, CODE_NONE)
	DIPS_HELPER( 0x002, "F8", KEYCODE_F8, CODE_NONE)
	DIPS_HELPER( 0x004, "F9", KEYCODE_F9, CODE_NONE)
	DIPS_HELPER( 0x008, "Druck", KEYCODE_PRTSCR, CODE_NONE)
	DIPS_HELPER( 0x010, "AUS", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x020, "1          !", KEYCODE_1, CODE_NONE)
	DIPS_HELPER( 0x040, "2          \"", KEYCODE_2, CODE_NONE)
	DIPS_HELPER( 0x080, "3          Paragraph", KEYCODE_3, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x001, "4          $", KEYCODE_4, CODE_NONE)
	DIPS_HELPER( 0x002, "5          %%", KEYCODE_5, CODE_NONE)
	DIPS_HELPER( 0x004, "6          &", KEYCODE_6, CODE_NONE)
	DIPS_HELPER( 0x008, "7          /", KEYCODE_7, CODE_NONE)
	DIPS_HELPER( 0x010, "8          (", KEYCODE_8, CODE_NONE)
	DIPS_HELPER( 0x020, "9          )", KEYCODE_9, CODE_NONE)
	DIPS_HELPER( 0x040, "0          =", KEYCODE_0, CODE_NONE)
	DIPS_HELPER( 0x080, "sharp-s    ?", KEYCODE_EQUALS, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x001, "acute", KEYCODE_MINUS, CODE_NONE)
	DIPS_HELPER( 0x002, "delete", KEYCODE_BACKSPACE, CODE_NONE)
	DIPS_HELPER( 0x004, "tab", KEYCODE_TAB, CODE_NONE)
	DIPS_HELPER( 0x008, "Q", KEYCODE_Q, CODE_NONE)
	DIPS_HELPER( 0x010, "W", KEYCODE_W, CODE_NONE)
	DIPS_HELPER( 0x020, "E", KEYCODE_E, CODE_NONE)
	DIPS_HELPER( 0x040, "R", KEYCODE_R, CODE_NONE)
	DIPS_HELPER( 0x080, "T          +", KEYCODE_T, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x001, "Z          -", KEYCODE_Y, CODE_NONE)
	DIPS_HELPER( 0x002, "U          4", KEYCODE_U, CODE_NONE)
	DIPS_HELPER( 0x004, "I          5", KEYCODE_I, CODE_NONE)
	DIPS_HELPER( 0x008, "O          6", KEYCODE_O, CODE_NONE)
	DIPS_HELPER( 0x010, "P", KEYCODE_P, CODE_NONE)
	DIPS_HELPER( 0x020, "Diaresis-U", KEYCODE_OPENBRACE, CODE_NONE)
	DIPS_HELPER( 0x040, "+          *", KEYCODE_CLOSEBRACE, CODE_NONE)
	DIPS_HELPER( 0x080, "capslock", KEYCODE_CAPSLOCK, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x001, "A", KEYCODE_A, CODE_NONE)
	DIPS_HELPER( 0x002, "S", KEYCODE_S, CODE_NONE)
	DIPS_HELPER( 0x004, "D", KEYCODE_D, CODE_NONE)
	DIPS_HELPER( 0x008, "F", KEYCODE_F, CODE_NONE)
	DIPS_HELPER( 0x010, "G          mul", KEYCODE_G, CODE_NONE)
	DIPS_HELPER( 0x020, "H          div", KEYCODE_H, CODE_NONE)
	DIPS_HELPER( 0x040, "J          1", KEYCODE_J, CODE_NONE)
	DIPS_HELPER( 0x080, "K          2", KEYCODE_K, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x001, "L          3", KEYCODE_L, CODE_NONE)
	DIPS_HELPER( 0x002, "Diaresis-O", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x004, "Diaresis-A", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x008, "Enter", KEYCODE_ENTER, CODE_NONE)
	DIPS_HELPER( 0x010, "left-shift", KEYCODE_LSHIFT, CODE_NONE)
	DIPS_HELPER( 0x020, "Y", KEYCODE_Z, CODE_NONE)
	DIPS_HELPER( 0x040, "X", KEYCODE_X, CODE_NONE)
	DIPS_HELPER( 0x080, "C", KEYCODE_C, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x001, "V", KEYCODE_V, CODE_NONE)
	DIPS_HELPER( 0x002, "B          root", KEYCODE_B, CODE_NONE)
	DIPS_HELPER( 0x004, "N          square", KEYCODE_N, CODE_NONE)
	DIPS_HELPER( 0x008, "M", KEYCODE_M, CODE_NONE)
	DIPS_HELPER( 0x010, ",          ;", KEYCODE_COMMA, CODE_NONE)
	DIPS_HELPER( 0x020, ".          :", KEYCODE_STOP, CODE_NONE)
	DIPS_HELPER( 0x040, "-          _", KEYCODE_SLASH, CODE_NONE)
	DIPS_HELPER( 0x080, "right-shift", KEYCODE_RSHIFT, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x001, "Entf", KEYCODE_DEL, CODE_NONE)
	DIPS_HELPER( 0x002, "Strg", KEYCODE_LCONTROL, KEYCODE_RCONTROL)
	DIPS_HELPER( 0x004, "Alt", KEYCODE_LALT, KEYCODE_RALT)
	DIPS_HELPER( 0x008, "AC", CODE_DEFAULT, CODE_NONE)
	DIPS_HELPER( 0x010, "Space", KEYCODE_SPACE, CODE_NONE)
	DIPS_HELPER( 0x020, "Spieler    left", KEYCODE_LEFT, CODE_NONE)
	DIPS_HELPER( 0x040, "Stufe      up", KEYCODE_UP, CODE_NONE)
	DIPS_HELPER( 0x080, "Antwort    down", KEYCODE_DOWN, CODE_NONE)
	PORT_START
	DIPS_HELPER( 0x001, "           right", KEYCODE_RIGHT, CODE_NONE)
#if 0
/*
  left button, right button
  joypad:
  left button, right button
  4 or 8 directions
*/
	DIPS_HELPER( 0x002, "", KEYCODE_7, CODE_NONE)
	DIPS_HELPER( 0x004, "", KEYCODE_6, CODE_NONE)
	DIPS_HELPER( 0x008, "", KEYCODE_7, CODE_NONE)
	DIPS_HELPER( 0x010, "", KEYCODE_6, CODE_NONE)
	DIPS_HELPER( 0x020, "", KEYCODE_7, CODE_NONE)
	DIPS_HELPER( 0x040, "", KEYCODE_6, CODE_NONE)
	DIPS_HELPER( 0x080, "", KEYCODE_7, CODE_NONE)
#endif

INPUT_PORTS_END

static struct GfxLayout comquest_charlayout =
{
        32,1,
        256,                                    /* 256 characters */
        1,                      /* 1 bits per pixel */
        { 0 },                  /* no bitplanes; 1 bit per pixel */
        /* x offsets */
        { 
			0, 0, 0, 0,
			1, 1, 1, 1, 
			2, 2, 2, 2, 
			3, 3, 3, 3, 
			4, 4, 4, 4, 
			5, 5, 5, 5, 
			6, 6, 6, 6, 
			7, 7, 7, 7
        },
        /* y offsets */
        { 0 },
        1*8
};

static struct GfxDecodeInfo comquest_gfxdecodeinfo[] = {
	{ REGION_GFX1, 0x0000, &comquest_charlayout,                     0, 2 },
    { -1 } /* end of array */
};

static int comquest_frame_int(void)
{
	return 0;
}

static unsigned char comquest_palette[2][3] =
{
	{ 0, 0, 0 },
	{ 255,255,255 }
};

static unsigned short comquest_colortable[1][2] = {
	{ 0, 1 },
};

static void comquest_init_colors (unsigned char *sys_palette,
							  unsigned short *sys_colortable,
							  const unsigned char *color_prom)
{
	memcpy (sys_palette, comquest_palette, sizeof (comquest_palette));
	memcpy(sys_colortable, comquest_colortable,sizeof(comquest_colortable));
}

static void comquest_machine_init(void)
{
//	UINT8 *mem=memory_region(REGION_USER1);
//	cpu_setbank(1,mem+0x00000);
}

static struct MachineDriver machine_driver_comquest =
{
	/* basic machine hardware */
	{
		{
			CPU_I8039, //?
/*
  8 bit bus, integrated io, serial io?,

  starts at address zero?

  not saturn, although very similar hardware compared to hp48g (build into big plastic case)
  not sc61860, 62015?
  not cdp1802
  not tms9900?
  not z80
  not 6502
  not i86
  not 6809
  not 68008?
  not tms32010
  not t11
  not arm
  not 8039
*/
			4000000, //?
			comquest_readmem,comquest_writemem,0,0,
			comquest_frame_int, 1,
        }
	},
	/* frames per second, VBL duration */
	60, DEFAULT_60HZ_VBLANK_DURATION, // lcd!
	1,				/* single CPU */
	comquest_machine_init,
	0,//pc1401_machine_stop,

	// 160 x 102
	64*4, 128, { 0, 64*4 - 1, 0, 128 - 1},
	comquest_gfxdecodeinfo,			   /* graphics decode info */
	// 16 out of 4096
	sizeof (comquest_palette) / sizeof (comquest_palette[0]) ,
	sizeof (comquest_colortable) / sizeof(comquest_colortable[0][0]),
	comquest_init_colors,		/* convert color prom */

	VIDEO_TYPE_RASTER,	/* video flags */
	0,						/* obsolete */
    comquest_vh_start,
	comquest_vh_stop,
	comquest_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		// unknown ?
		{ 0 }
    }
};

ROM_START(comquest)
//	ROM_REGION(0x10000,REGION_CPU1)
//	ROM_REGION(0x80000,REGION_USER1)
	ROM_REGION(0x100000,REGION_CPU1)
	ROM_LOAD("comquest.bin", 0x00000, 0x80000, 0x2bf4b1a8)
/*
000 +16kbyte code 
040 16kbyte code
080 8kbyte code
0a0 8kbyte code
0c0 16kbyte code 
100 16kbyte code
140 16kbyte code
180 16kbyte code
1c0 16kbyte code
200 16kbyte code
240 8kkbyte
260 16kbyte !
2a0 8kbyte
2c0 8kbyte
2e0 8kbyte
300 +text
720 empty
740 empty
760 empty
780 16kb
7c0 16kb
 */

	ROM_REGION(0x100,REGION_GFX1)
ROM_END

static const struct IODevice io_comquest[] = {
#if 0
	{
		IO_CARTSLOT,					/* type */
		1,								/* count */
		"bin\0",						/* file extensions */
		NULL,							/* private */
		a2600_id_rom,					/* id */
		a2600_load_rom,					/* init */
		NULL,							/* exit */
		NULL,							/* info */
		NULL,							/* open */
		NULL,							/* close */
		NULL,							/* status */
		NULL,							/* seek */
		NULL,							/* tell */
		NULL,							/* input */
		NULL,							/* output */
		NULL,							/* input_chunk */
		NULL							/* output_chunk */
	},
#endif
    { IO_END }
};

void init_comquest(void)
{
	int i;
	UINT8 *gfx=memory_region(REGION_GFX1);
	for (i=0; i<256; i++) gfx[i]=i;
}

/*    YEAR  NAME      PARENT    MACHINE   INPUT     INIT      MONITOR	COMPANY   FULLNAME */
CONS( 19??, comquest, 0, 		comquest, comquest, comquest, "Data Concepts",  "Comquest Plus German")
