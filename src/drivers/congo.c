/***************************************************************************

Congo Bongo memory map (preliminary)

0000-1fff ROM 1
2000-3fff ROM 2
4000-5fff ROM 3
6000-7fff ROM 4

8000-87ff RAM 1
8800-8fff RAM 2
a000-a3ff Video RAM
a400-a7ff Color RAM

8400-8fff sprites

read:
c000      IN0
c001      IN1
c002      DSW1
c003      DSW2
c008      IN2

write:
c000-c002 ?
c006      ?
c018      coinAen, used to enable input on coin slot A
c019      coinBen, used to enable input on coin slot B
c01a      serven,  used to enable input on service sw
c01b      counterA,coin counter for slot A
c01c      counterB,coin counter for slot B
c01d      background enable
c01e      flip screen
c01f      interrupt enable

c028-c029 background playfield position
c031      sprite enable ?
c032      sprite start
c033      sprite count


interrupts:
VBlank triggers IRQ, handled with interrupt mode 1
NMI causes a ROM/RAM test.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"


extern unsigned char *congo_background_position;
extern unsigned char *congo_background_enable;
void zaxxon_vh_convert_color_prom(unsigned char *palette, unsigned char *colortable,const unsigned char *color_prom);
int  congo_vh_start(void);
void congo_vh_stop(void);
void congo_vh_screenrefresh(struct osd_bitmap *bitmap);

void congo_daio(int offset, int data);



static struct MemoryReadAddress readmem[] =
{
	{ 0x8000, 0x8fff, MRA_RAM },
	{ 0xa000, 0x83ff, MRA_RAM },
	{ 0xa000, 0xa7ff, MRA_RAM },
	{ 0xc000, 0xc000, input_port_0_r },
	{ 0xc001, 0xc001, input_port_1_r },
	{ 0xc008, 0xc008, input_port_2_r },
	{ 0xc002, 0xc002, input_port_3_r },
	{ 0xc003, 0xc003, input_port_4_r },
	{ 0x0000, 0x7fff, MRA_ROM },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x8000, 0x83ff, MWA_RAM },
	{ 0xa000, 0xa3ff, videoram_w, &videoram, &videoram_size },
	{ 0xa400, 0xa7ff, colorram_w, &colorram },
	{ 0x8400, 0x8fff, MWA_RAM, &spriteram },
	{ 0xc01f, 0xc01f, interrupt_enable_w },
	{ 0xc028, 0xc029, MWA_RAM, &congo_background_position },
	{ 0xc01d, 0xc01d, MWA_RAM, &congo_background_enable },
	{ 0xc038, 0xc038, soundlatch_w },
	{ 0xc030, 0xc033, MWA_NOP }, /* ??? */
	{ 0xc01e, 0xc01e, MWA_NOP }, /* flip unused for now */
	{ 0xc018, 0xc018, MWA_NOP }, /* coinAen */
	{ 0xc019, 0xc019, MWA_NOP }, /* coinBen */
	{ 0xc01a, 0xc01a, MWA_NOP }, /* serven */
	{ 0xc01b, 0xc01c, coin_counter_w }, /* counterA, counterB */
	{ 0x0000, 0x7fff, MWA_ROM },
	{ -1 }  /* end of table */
};


static struct MemoryReadAddress sh_readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x47ff, MRA_RAM },
	{ 0x8000, 0x8003, soundlatch_r },
	{ -1 }
};
static struct MemoryWriteAddress sh_writemem[] =
{
	{ 0x6000, 0x6003, SN76496_0_w },
	{ 0xa000, 0xa003, SN76496_1_w },
	{ 0x4000, 0x47ff, MWA_RAM },
	{ 0x8000, 0x8003, congo_daio },
	{ 0x0000, 0x2000, MWA_ROM },
	{ -1 }
};



/***************************************************************************

  Congo Bongo uses NMI to trigger the self test. We use a fake input port
  to tie that event to a keypress.

***************************************************************************/
static int congo_interrupt(void)
{
	if (readinputport(5) & 1)	/* get status of the F2 key */
		return nmi_interrupt();	/* trigger self test */
	else return interrupt();
}

/* almost the same as Zaxxon; UP and DOWN are inverted, and the joystick is 4 way. */
INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNKNOWN )	/* probably unused (the self test doesn't mention it) */
	/* the coin inputs must stay active for exactly one frame, otherwise */
	/* the game will keep inserting coins. */
	PORT_BITX(0x20, IP_ACTIVE_HIGH, IPT_COIN1 | IPF_IMPULSE,
			"Coin A", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	PORT_BITX(0x40, IP_ACTIVE_HIGH, IPT_COIN2 | IPF_IMPULSE,
			"Coin B", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )
	/* Coin Aux doesn't need IMPULSE to pass the test, but it still needs it */
	/* to avoid the freeze. */
	PORT_BITX(0x80, IP_ACTIVE_HIGH, IPT_COIN3 | IPF_IMPULSE,
			"Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 1 )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "10000" )
	PORT_DIPSETTING(    0x01, "20000" )
	PORT_DIPSETTING(    0x02, "30000" )
	PORT_DIPSETTING(    0x00, "40000" )
	PORT_DIPNAME( 0x0c, 0x0c, "Difficulty???", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "Easy?" )
	PORT_DIPSETTING(    0x04, "Medium?" )
	PORT_DIPSETTING(    0x08, "Hard?" )
	PORT_DIPSETTING(    0x00, "Hardest?" )
	PORT_DIPNAME( 0x30, 0x30, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "3" )
	PORT_DIPSETTING(    0x10, "4" )
	PORT_DIPSETTING(    0x20, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x40, 0x40, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x80, "Cocktail" )

	PORT_START	/* DSW1 */
 	PORT_DIPNAME( 0x0f, 0x03, "B Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0f, "4/1" )
	PORT_DIPSETTING(    0x07, "3/1" )
	PORT_DIPSETTING(    0x0b, "2/1" )
	PORT_DIPSETTING(    0x06, "2/1+Bonus each 5" )
	PORT_DIPSETTING(    0x0a, "2/1+Bonus each 2" )
	PORT_DIPSETTING(    0x03, "1/1" )
	PORT_DIPSETTING(    0x02, "1/1+Bonus each 5" )
	PORT_DIPSETTING(    0x0c, "1/1+Bonus each 4" )
	PORT_DIPSETTING(    0x04, "1/1+Bonus each 2" )
	PORT_DIPSETTING(    0x0d, "1/2" )
	PORT_DIPSETTING(    0x08, "1/2+Bonus each 5" )
	PORT_DIPSETTING(    0x00, "1/2+Bonus each 4" )
	PORT_DIPSETTING(    0x05, "1/3" )
	PORT_DIPSETTING(    0x09, "1/4" )
	PORT_DIPSETTING(    0x01, "1/5" )
	PORT_DIPSETTING(    0x0e, "1/6" )
	PORT_DIPNAME( 0xf0, 0x30, "A Coin/Cred", IP_KEY_NONE )
	PORT_DIPSETTING(    0xf0, "4/1" )
	PORT_DIPSETTING(    0x70, "3/1" )
	PORT_DIPSETTING(    0xb0, "2/1" )
	PORT_DIPSETTING(    0x60, "2/1+Bonus each 5" )
	PORT_DIPSETTING(    0xa0, "2/1+Bonus each 2" )
	PORT_DIPSETTING(    0x30, "1/1" )
	PORT_DIPSETTING(    0x20, "1/1+Bonus each 5" )
	PORT_DIPSETTING(    0xc0, "1/1+Bonus each 4" )
	PORT_DIPSETTING(    0x40, "1/1+Bonus each 2" )
	PORT_DIPSETTING(    0xd0, "1/2" )
	PORT_DIPSETTING(    0x80, "1/2+Bonus each 5" )
	PORT_DIPSETTING(    0x00, "1/2+Bonus each 4" )
	PORT_DIPSETTING(    0x50, "1/3" )
	PORT_DIPSETTING(    0x90, "1/4" )
	PORT_DIPSETTING(    0x10, "1/5" )
	PORT_DIPSETTING(    0xe0, "1/6" )

	PORT_START	/* FAKE */
	/* This fake input port is used to get the status of the F2 key, */
	/* and activate the test mode, which is triggered by a NMI */
	PORT_BITX(0x01, IP_ACTIVE_HIGH, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
INPUT_PORTS_END



static struct GfxLayout charlayout1 =
{
	8,8,	/* 8*8 characters */
	256,	/* 256 characters */
	3,	/* 3 bits per pixel (actually 2, the third plane is 0) */
	{ 2*256*8*8, 256*8*8, 0 },	/* the two bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },	/* the bitplanes are separated */
	{ 7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	8*8	/* every char takes 8 consecutive bytes */
};

static struct GfxLayout spritelayout =
{
	32,32,	/* 32*32 sprites */
	128,	/* 128 sprites */
	3,	/* 3 bits per pixel */
	{ 2*128*128*8, 128*128*8, 0 },    /* the bitplanes are separated */
	{ 103*8, 102*8, 101*8, 100*8, 99*8, 98*8, 97*8, 96*8,
			71*8, 70*8, 69*8, 68*8, 67*8, 66*8, 65*8, 64*8,
			39*8, 38*8, 37*8, 36*8, 35*8, 34*8, 33*8, 32*8,
			7*8, 6*8, 5*8, 4*8, 3*8, 2*8, 1*8, 0*8 },
	{ 0, 1, 2, 3, 4, 5, 6, 7,
			8*8+0, 8*8+1, 8*8+2, 8*8+3, 8*8+4, 8*8+5, 8*8+6, 8*8+7,
			16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7,
			24*8+0, 24*8+1, 24*8+2, 24*8+3, 24*8+4, 24*8+5, 24*8+6, 24*8+7 },
	128*8	/* every sprite takes 128 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout1,  0, 32 },	/* characters */
	{ 1, 0x1800, &charlayout2,  0, 32 },	/* background tiles */
	{ 1, 0x7800, &spritelayout, 0, 32 },	/* sprites */
	{ -1 } /* end of array */
};



static unsigned char color_prom[] =
{
	/* U68 - palette */
	0x00,0x00,0xF0,0xF0,0x66,0x5D,0xAE,0xF6,0x00,0x00,0xF0,0xF6,0xA4,0x70,0xAE,0xF6,
	0x00,0x00,0xF0,0x36,0x70,0x28,0xAE,0xF6,0x00,0x00,0xF0,0x06,0xF0,0xE0,0xAE,0xF6,
	0x00,0x00,0xF0,0x38,0x34,0x20,0xAE,0xF6,0x00,0x00,0xF6,0xAE,0x24,0x07,0x04,0xAC,
	0x00,0x00,0x06,0xAE,0x00,0x00,0x00,0x00,0x00,0x07,0x00,0x00,0x36,0x00,0x00,0x00,
	0x00,0x07,0x07,0x00,0x36,0x36,0x00,0x00,0x00,0x07,0x07,0x07,0x36,0x36,0x36,0x00,
	0x00,0x14,0x03,0x02,0x80,0xF0,0xF6,0x18,0x00,0x14,0x03,0x02,0x80,0x1B,0xF6,0x03,
	0x00,0x14,0x03,0x02,0x80,0x20,0xF6,0x18,0x00,0x14,0x03,0x02,0x80,0x03,0xF6,0xA4,
	0x00,0x14,0x03,0x16,0xD0,0xF0,0xF6,0xA4,0x00,0x14,0x03,0x02,0x80,0x9B,0xF6,0xA4,
	0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0xB4,0xAC,0xA4,0x28,0xD0,0xF6,0xE0,
	0x00,0x07,0x2D,0x04,0x07,0x07,0xEC,0x00,0x00,0x07,0xB0,0xE0,0x80,0x82,0xF6,0x00,
	0x00,0x07,0x2D,0x16,0x66,0x28,0x5D,0x00,0x00,0x00,0xF6,0xAE,0x1B,0x07,0x04,0xAC,
	0x00,0x26,0x5D,0x03,0x02,0xEC,0xF6,0xE0,0x00,0x38,0x28,0xF6,0xD0,0xFF,0xFF,0xFF,
	0x00,0x26,0xF0,0xC0,0x80,0x82,0xF6,0x00,0x00,0x07,0xAE,0xA5,0x66,0x5D,0xF6,0x00,
	0x00,0x07,0x26,0x26,0x16,0x16,0xF6,0xD0,0x00,0x07,0xB0,0xD0,0xC0,0x80,0xF6,0x00,
	0x00,0x24,0x06,0x16,0x04,0x03,0xF6,0xD0,0x00,0x05,0x24,0x2D,0x1B,0x12,0xF6,0xD0,
	0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
};



static struct SN76496interface sn76496_interface =
{
	2,	/* 2 chips */
	4000000,	/* 4 Mhz??? */
	{ 255, 255 }
};

static struct Samplesinterface samples_interface =
{
	5	/* 5 channels */
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3072000,	/* 3.072 Mhz ?? */
			0,
			readmem,writemem,0,0,
			congo_interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			2000000,
			3,
			sh_readmem, sh_writemem, 0,0,
			interrupt, 4
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	32*8, 32*8, { 2*8, 30*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	256,32*8,
	zaxxon_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	congo_vh_start,
	congo_vh_stop,
	congo_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		},
		{
			SOUND_SAMPLES,
			&samples_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( congo_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "congo1.bin",  0x0000, 0x2000, 0x8f3c6024 )
	ROM_LOAD( "congo2.bin",  0x2000, 0x2000, 0xb9faa34e )
	ROM_LOAD( "congo3.bin",  0x4000, 0x2000, 0x059defc5 )
	ROM_LOAD( "congo4.bin",  0x6000, 0x2000, 0xdd66f8be )

	ROM_REGION(0x13800)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "congo5.bin",  0x00000, 0x1000, 0x863539ed )	/* characters */
	/* 1000-1800 empty space to convert the characters as 3bpp instead of 2 */
	ROM_LOAD( "congo8.bin",  0x01800, 0x2000, 0xb060d10e )	/* background tiles */
	ROM_LOAD( "congo9.bin",  0x03800, 0x2000, 0xf10944e7 )
	ROM_LOAD( "congo10.bin", 0x05800, 0x2000, 0xccf64f98 )
	ROM_LOAD( "congo12.bin", 0x07800, 0x2000, 0x96781f3e )	/* sprites */
	ROM_LOAD( "congo13.bin", 0x09800, 0x2000, 0x5e0d138d )
	ROM_LOAD( "congo11.bin", 0x0b800, 0x2000, 0x4a0cef9a )
	ROM_LOAD( "congo14.bin", 0x0d800, 0x2000, 0x33c05a1e )
	ROM_LOAD( "congo16.bin", 0x0f800, 0x2000, 0xdd85484b )
	ROM_LOAD( "congo15.bin", 0x11800, 0x2000, 0xb3f44d2e )
	ROM_REGION(0x8000)      /* background data */
	ROM_LOAD( "congo6.bin", 0x0000, 0x2000, 0x785c8c22 )
	/* 2000-3fff empty space to match Zaxxon */
	ROM_LOAD( "congo7.bin", 0x4000, 0x2000, 0x87817173 )
	/* 6000-7fff empty space to match Zaxxon */

	ROM_REGION(0x10000) /*64K for the sound cpu*/
	ROM_LOAD( "congo17.bin", 0x0000, 0x2000 ,0xe4e0223c)
ROM_END

ROM_START( tiptop_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "tiptop1.bin",  0x0000, 0x2000, 0x83b2291a )
	ROM_LOAD( "tiptop2.bin",  0x2000, 0x2000, 0x85d20018 )
	ROM_LOAD( "tiptop3.bin",  0x4000, 0x2000, 0x98290647 )
	ROM_LOAD( "tiptop4.bin",  0x6000, 0x2000, 0x58baf652 )

	ROM_REGION(0x13800)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "tiptop5.bin",  0x00000, 0x1000, 0x863539ed )	/* characters */
	/* 1000-1800 empty space to convert the characters as 3bpp instead of 2 */
	ROM_LOAD( "tiptop8.bin",  0x01800, 0x2000, 0xb060d10e )	/* background tiles */
	ROM_LOAD( "tiptop9.bin",  0x03800, 0x2000, 0xf10944e7 )
	ROM_LOAD( "tiptop10.bin", 0x05800, 0x2000, 0xccf64f98 )
	ROM_LOAD( "tiptop12.bin", 0x07800, 0x2000, 0x96781f3e )	/* sprites */
	ROM_LOAD( "tiptop13.bin", 0x09800, 0x2000, 0x5e0d138d )
	ROM_LOAD( "tiptop11.bin", 0x0b800, 0x2000, 0x4a0cef9a )
	ROM_LOAD( "tiptop14.bin", 0x0d800, 0x2000, 0x33c05a1e )
	ROM_LOAD( "tiptop16.bin", 0x0f800, 0x2000, 0xdd85484b )
	ROM_LOAD( "tiptop15.bin", 0x11800, 0x2000, 0xb3f44d2e )
	ROM_REGION(0x8000)      /* background data */
	ROM_LOAD( "tiptop6.bin", 0x0000, 0x2000, 0x785c8c22 )
	/* 2000-3fff empty space to match Zaxxon */
	ROM_LOAD( "tiptop7.bin", 0x4000, 0x2000, 0x87817173 )
	/* 6000-7fff empty space to match Zaxxon */

	ROM_REGION(0x10000) /*64K for the sound cpu*/
	ROM_LOAD( "tiptop17.bin", 0x0000, 0x2000 ,0xe4e0223c)
ROM_END


/* Samples for Congo Bongo, these are needed for now    */
/* as I haven't gotten around to calculate a formula for the */
/* simple oscillators producing the drums VL*/
/* As for now, thanks to Tim L. for providing samples */

static const char *congo_samplenames[] =
{
	"*congo",
	"gorilla.sam",
	"bass.sam",
	"congaa.sam",
	"congab.sam",
	"rim.sam",
	0
};



static int hiload(void)
{
        unsigned char *RAM = Machine->memory_region[0];
	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x8030],"\x00\x89\x00",3) == 0 &&
			memcmp(&RAM[0x8099],"\x00\x37\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x8020],21*6);
			RAM[0x80bd] = RAM[0x8030];
			RAM[0x80be] = RAM[0x8031];
			RAM[0x80bf] = RAM[0x8032];
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}



static void hisave(void)
{
        unsigned char *RAM = Machine->memory_region[0];
	/* make sure that the high score table is still valid (entering the */
	/* test mode corrupts it) */
	if (memcmp(&RAM[0x8030],"\x00\x00\x00",3) != 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
		{
			osd_fwrite(f,&RAM[0x8020],21*6);
			osd_fclose(f);
		}
	}
}



struct GameDriver congo_driver =
{
	"Congo Bongo",
	"congo",
	"Ville Laitinen (MAME driver)\nNicola Salmoria (Zaxxon driver)\nTim Lindquist (color & sound info)",
	&machine_driver,

	congo_rom,
	0, 0,
	congo_samplenames,
	0,	/* sound_prom */

	input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};

struct GameDriver tiptop_driver =
{
	"Tip Top",
	"tiptop",
	"Ville Laitinen (MAME driver)\nNicola Salmoria (Zaxxon driver)\nTim Lindquist (color & sound info)",
	&machine_driver,

	tiptop_rom,
	0, 0,
	congo_samplenames,
	0,	/* sound_prom */

	input_ports,

	color_prom, 0, 0,
	ORIENTATION_DEFAULT,

	hiload, hisave
};
