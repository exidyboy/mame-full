/***************************************************************************

  gb.c

  Driver file to handle emulation of the Nintendo Gameboy.
  By:

  Hans de Goede               1998
  Anthony Kruize              2002

  Todo list:
  Done entries kept for historical reasons, besides that it's nice to see
  what is already done instead of what has to be done.

Priority:  Todo:                                                  Done:
  2        Replace Marat's  vidhrdw/gb.c  by Playboy code           *
  2        Clean & speed up vidhrdw/gb.c                            *
  2        Replace Marat's  Z80gb/Z80gb.c by Playboy code           *
  2        Transform Playboys Z80gb.c to big case method            *
  2        Clean up Z80gb.c                                         *
  2        Fix / optimise halt instruction                          *
  2        Do correct lcd stat timing                               In Progress
  2        Generate lcd stat interrupts                             *
  2        Replace Marat's code in machine/gb.c by Playboy code     ?
  1        Check, and fix if needed flags bug which troubles ffa    ?
  1        Save/restore battery backed ram                          *
  1        Add sound                                                In Progress
  0        Add supergb support                                      In Progress
  0        Add palette editting, save & restore
  0        Add somekind of backdrop support
  0        Speedups if remotly possible

  2 = has to be done before first public release
  1 = should be added later on
  0 = bells and whistles

***************************************************************************/
#include "driver.h"
#include "vidhrdw/generic.h"
#include "machine/gb.h"
#include "includes/gb.h"

/*static UINT16 gb_cpu_af_reset  = 0x01B0;
static UINT16 gbp_cpu_af_reset = 0xFFB0;*/
static UINT16 gbc_cpu_af_reset = 0x11B0;

static MEMORY_READ_START (gb_readmem)
	{ 0x0000, 0x3fff, MRA_ROM },			/* 16k fixed ROM bank */
	{ 0x4000, 0x7fff, MRA_BANK1 },			/* 16k switched ROM bank */
	{ 0x8000, 0x9fff, MRA_RAM },			/* 8k VRAM */
	{ 0xa000, 0xbfff, MRA_BANK2 },			/* 8k switched RAM bank (on cartridge) */
	{ 0xc000, 0xfe9f, MRA_RAM },			/* 8k low RAM, echo RAM, OAM RAM */
	{ 0xfea0, 0xfeff, MRA_NOP },			/* unusable */
	{ 0xff00, 0xff7f, gb_r_io },			/* gb io */
	{ 0xff80, 0xffff, MRA_RAM },			/* 127 bytes high RAM, interrupt enable io */
MEMORY_END

static MEMORY_WRITE_START (gb_writemem)
	{ 0x0000, 0x1fff, MWA_ROM },			/* 8k ROM (should really be RAM enable) */
	{ 0x2000, 0x3fff, gb_rom_bank_select },	/* ROM bank select */
	{ 0x4000, 0x5fff, gb_ram_bank_select },	/* RAM bank select */
	{ 0x6000, 0x7fff, gb_mem_mode_select },	/* RAM/ROM mode select */
	{ 0x8000, 0x9fff, MWA_RAM },			/* 8k VRAM */
	{ 0xa000, 0xbfff, MWA_BANK2 },			/* 8k switched RAM bank (on cartridge) */
	{ 0xc000, 0xfe9f, MWA_RAM },			/* 8k low RAM, echo RAM, OAM RAM */
	{ 0xfea0, 0xfeff, MWA_NOP },			/* unusable */
	{ 0xff00, 0xff7f, gb_w_io },			/* gb io */
	{ 0xff80, 0xfffe, MWA_RAM },			/* 127 bytes high RAM */
	{ 0xffff, 0xffff, gb_w_ie },			/* gb io (interrupt enable) */
MEMORY_END

static MEMORY_WRITE_START (sgb_writemem)
	{ 0x0000, 0x1fff, MWA_ROM },			/* 8k ROM (should really be RAM enable */
	{ 0x2000, 0x3fff, gb_rom_bank_select },	/* ROM bank select */
	{ 0x4000, 0x5fff, gb_ram_bank_select },	/* RAM bank select */
	{ 0x6000, 0x7fff, gb_mem_mode_select },	/* RAM/ROM mode select */
	{ 0x8000, 0x9fff, MWA_RAM },			/* 8k VRAM */
	{ 0xa000, 0xbfff, MWA_BANK2 },			/* 8k switched RAM bank (on cartridge) */
	{ 0xc000, 0xfe9f, MWA_RAM },			/* 8k low RAM, echo RAM, OAM RAM */
	{ 0xfea0, 0xfeff, MWA_NOP },			/* unusable */
	{ 0xff00, 0xff7f, sgb_w_io },			/* sgb io */
	{ 0xff80, 0xfffe, MWA_RAM },			/* 127b high RAM */
	{ 0xffff, 0xffff, gb_w_ie },			/* gb io (interrupt enable) */
MEMORY_END

static MEMORY_READ_START (gbc_readmem)
	{ 0x0000, 0x3fff, MRA_ROM },			/* 16k fixed ROM bank */
	{ 0x4000, 0x7fff, MRA_BANK1 },			/* 16k switched ROM bank */
	{ 0x8000, 0x9fff, MRA_BANK4 },			/* 8k switched VRAM bank */
	{ 0xa000, 0xbfff, MRA_BANK2 },			/* 8k switched RAM bank (on cartridge) */
	{ 0xc000, 0xcfff, MRA_RAM },			/* 4k fixed RAM bank */
	{ 0xd000, 0xdfff, MRA_BANK3 },			/* 4k switched RAM bank */
	{ 0xe000, 0xfe9f, MRA_RAM },			/* echo RAM, OAM RAM */
	{ 0xfea0, 0xfeff, MRA_NOP },			/* unusable */
	{ 0xff00, 0xff7f, gb_r_io },			/* gb io */
	{ 0xff80, 0xffff, MRA_RAM },			/* 127 bytes high RAM, interrupt enable io */
MEMORY_END

static MEMORY_WRITE_START (gbc_writemem)
	{ 0x0000, 0x1fff, MWA_ROM },			/* 8k ROM (should really be RAM enable */
	{ 0x2000, 0x3fff, gb_rom_bank_select },	/* ROM bank select */
	{ 0x4000, 0x5fff, gb_ram_bank_select },	/* RAM bank select */
	{ 0x6000, 0x7fff, gb_mem_mode_select },	/* RAM/ROM mode select */
	{ 0x8000, 0x9fff, MWA_BANK4 },			/* 8k switched VRAM bank */
	{ 0xa000, 0xbfff, MWA_BANK2 },			/* 8k switched RAM bank (on cartridge) */
	{ 0xc000, 0xcfff, MWA_RAM },			/* 4k fixed RAM bank */
	{ 0xd000, 0xdfff, MWA_BANK3 },			/* 4k switched RAM bank */
	{ 0xe000, 0xfeff, MWA_RAM },			/* echo RAM, OAM RAM */
//{ 0xe000, 0xfeff, MWA_RAM/*, &videoram, &videoram_size*/ }, /* video & sprite ram */
	{ 0xff00, 0xff7f, gbc_w_io },			/* gbc io */
	{ 0xff80, 0xfffe, MWA_RAM },			/* 127b high RAM */
	{ 0xffff, 0xffff, gb_w_ie },			/* gb io (interrupt enable) */
MEMORY_END


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ -1 } /* end of array */
};

INPUT_PORTS_START( gameboy )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1       )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2       )
	/*PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN, "Select", KEYCODE_LSHIFT, IP_JOY_DEFAULT ) */
	/*PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN, "Start",  KEYCODE_Z,      IP_JOY_DEFAULT ) */
	PORT_BITX( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD, "Select", KEYCODE_5, IP_JOY_DEFAULT )
	PORT_BITX( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD, "Start",  KEYCODE_1, IP_JOY_DEFAULT )
INPUT_PORTS_END

static unsigned char palette[] =
{
/* Simple black and white palette */
/*	0xFF,0xFF,0xFF,
	0xB0,0xB0,0xB0,
	0x60,0x60,0x60,
	0x00,0x00,0x00 */

/* Possibly needs a little more green in it */
	0xFF,0xFB,0x87,
	0xB1,0xAE,0x4E,
	0x84,0x80,0x4E,
	0x4E,0x4E,0x4E
};

/* Initialise the palette */
static PALETTE_INIT( gb )
{
	int i;
	for (i = 0; i < (sizeof(palette) / 3); i++)
	{
		palette_set_color(i, palette[i*3+0], palette[i*3+1], palette[i*3+2]);
		colortable[i] = i;
	}
}

static PALETTE_INIT( sgb )
{
	int i, r, g, b;

	for( i = 0; i < 32768; i++ )
	{
		r = (i & 0x1F) << 3;
		g = ((i >> 5) & 0x1F) << 3;
		b = ((i >> 10) & 0x1F) << 3;
		palette_set_color( i, r, g, b );
	}

	/* Some default colours for non-SGB games */
	colortable[0] = 32767;
	colortable[1] = 21140;
	colortable[2] = 10570;
	colortable[3] = 0;
	/* The rest of the colortable can be black */
	for( i = 4; i < 8*16; i++ )
		colortable[i] = 0;
}

static PALETTE_INIT( gbc )
{
	int i, r, g, b;

	for( i = 0; i < 32768; i++ )
	{
		r = (i & 0x1F) << 3;
		g = ((i >> 5) & 0x1F) << 3;
		b = ((i >> 10) & 0x1F) << 3;
		palette_set_color( i, r, g, b );
	}

	/* Background is initialised as white */
	for( i = 0; i < 8*4; i++ )
		colortable[i] = 32767;
	/* Sprites are supposed to be uninitialized, but we'll make them black */
	for( i = 8*4; i < 16*4; i++ )
		colortable[i] = 0;
}

static struct CustomSound_interface gameboy_sound_interface =
{ gameboy_sh_start, 0, 0 };


static MACHINE_DRIVER_START( gameboy )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80GB, 4194304)			/* 4.194304 Mhz */
	MDRV_CPU_MEMORY(gb_readmem, gb_writemem)
	MDRV_CPU_VBLANK_INT(gb_scanline_interrupt, 154 * 3)	/* 1 int each scanline ! */

	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( gb )
	MDRV_MACHINE_STOP( gb )

	MDRV_VIDEO_START( generic_bitmapped )
	MDRV_VIDEO_UPDATE( generic_bitmapped )

	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(20*8, 18*8)
	MDRV_VISIBLE_AREA(0*8, 20*8-1, 0*8, 18*8-1)
	MDRV_GFXDECODE(gfxdecodeinfo)
	MDRV_PALETTE_LENGTH(sizeof(palette) / sizeof(palette[0]) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof(palette) / sizeof(palette[0]) / 3)
	MDRV_PALETTE_INIT(gb)

    /* sound hardware */
	MDRV_SOUND_ADD(CUSTOM, gameboy_sound_interface)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( supergb )
	MDRV_IMPORT_FROM(gameboy)
	MDRV_CPU_REPLACE("main", Z80GB, 4295454)			/* 4.295454 Mhz */
	MDRV_CPU_MEMORY(gb_readmem, sgb_writemem)

	MDRV_MACHINE_INIT( sgb )

	MDRV_SCREEN_SIZE(32*8, 28*8)
	MDRV_VISIBLE_AREA(0*8, 32*8-1, 0*8, 28*8-1)
	MDRV_PALETTE_LENGTH(32768)
	MDRV_COLORTABLE_LENGTH(8*16)	/* 8 palettes of 16 colours */
	MDRV_PALETTE_INIT(sgb)
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( gbcolor )
	MDRV_IMPORT_FROM(gameboy)
	MDRV_CPU_MODIFY("main")
	MDRV_CPU_MEMORY(gbc_readmem, gbc_writemem)
	MDRV_CPU_CONFIG(gbc_cpu_af_reset)

	MDRV_MACHINE_INIT(gbc)

	MDRV_PALETTE_LENGTH(32768)
	MDRV_COLORTABLE_LENGTH(16*4)	/* 16 palettes of 4 colours */
	MDRV_PALETTE_INIT(gbc)
MACHINE_DRIVER_END

static const struct IODevice io_gameboy[] =
{
	{
		IO_CARTSLOT,		/* type */
		1,					/* count */
		"gb\0gmb\0cgb\0gbc\0sgb\0",		/* file extensions */
		IO_RESET_ALL,		/* reset if file changed */
		OSD_FOPEN_DUMMY,	/* open mode */
		0,
		gb_load_rom,		/* init */
		NULL,				/* exit */
		NULL,				/* info */
		NULL,				/* open */
		NULL,				/* close */
		NULL,				/* status */
		NULL,				/* seek */
		NULL,				/* tell */
		NULL,				/* input */
		NULL,				/* output */
		NULL,				/* input_chunk */
		NULL				/* output_chunk */
	},
	{ IO_END }
};

#define io_supergb  io_gameboy
#define io_gbcolor  io_gameboy


/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( gameboy )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_END

ROM_START( supergb )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
	ROM_REGION( 0x2000,  REGION_GFX1, 0 )	/* SGB border */
ROM_END

ROM_START( gbcolor )
	ROM_REGION( 0x10000, REGION_CPU1, 0 )
ROM_END


/*     YEAR  NAME     PARENT   MACHINE  INPUT    INIT  COMPANY     FULLNAME         FLAGS*/
CONSX( 1990, gameboy, 0,       gameboy, gameboy, 0,    "Nintendo", "GameBoy",       GAME_IMPERFECT_SOUND )
CONSX( 1994, supergb, gameboy, supergb, gameboy, 0,    "Nintendo", "Super GameBoy", GAME_IMPERFECT_SOUND )
CONSX( 1998, gbcolor, gameboy, gbcolor, gameboy, 0,    "Nintendo", "GameBoy Color", GAME_IMPERFECT_SOUND )

