/****************************************************************************

Lode Runner

**************************************************************************/

#include "driver.h"
#include "Z80/Z80.h"
#include "vidhrdw/generic.h"


extern unsigned char *lrunner_scroll_low;
extern unsigned char *lrunner_scroll_high;
void lrunner_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int lrunner_vh_start(void);
void lrunner_vh_stop(void);
void lrunner_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);

void mpatrol_io_w(int offset, int value);
int mpatrol_io_r(int offset);
void mpatrol_adpcm_reset_w(int offset,int value);
void mpatrol_sound_cmd_w(int offset, int value);

void mpatrol_adpcm_int(int data);


static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0xd000, 0xdfff, MRA_RAM },         /* Video and Color ram */
	{ 0xe000, 0xefff, MRA_RAM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0xa000, 0xa000, MWA_RAM, &lrunner_scroll_low },
	{ 0xb000, 0xb000, MWA_RAM, &lrunner_scroll_high },
{ 0xc000, 0xc0ff, MWA_RAM, &spriteram, &spriteram_size },
//	{ 0xc020, 0xc0df, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd000, 0xdfff, videoram_w, &videoram, &videoram_size },
	{ 0xe000, 0xefff, MWA_RAM },
	{ -1 }	/* end of table */
};


static struct IOReadPort readport[] =
{
	{ 0x00, 0x00, input_port_0_r },   /* coin */
	{ 0x01, 0x01, input_port_1_r },   /* player 1 control */
	{ 0x02, 0x02, input_port_2_r },   /* player 2 control */
	{ 0x03, 0x03, input_port_3_r },   /* DSW 1 */
	{ 0x04, 0x04, input_port_4_r },   /* DSW 2 */
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x00, 0x00, mpatrol_sound_cmd_w },
//	{ 0x01, 0x01, kungfum_flipscreen_w },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x001f, mpatrol_io_r },
	{ 0x0080, 0x00ff, MRA_RAM },
	{ 0xc000, 0xffff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x001f, mpatrol_io_w },
	{ 0x0080, 0x00ff, MWA_RAM },
	{ 0x0801, 0x0802, MSM5205_data_w },
	{ 0x9000, 0x9000, MWA_NOP },    /* IACK */
	{ 0xc000, 0xffff, MWA_ROM },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START	/* IN0 */
/* Start 1 & 2 also restarts and freezes the game with stop mode on
   and are used in test mode to enter and esc the various tests */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_START2 )
	/* coin input must be active for 19 frames to be consistently recognized */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE,
		"Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 19 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0xf0, IP_ACTIVE_LOW, IPT_UNUSED )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
        PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
        PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON1 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* probably unused */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x01, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x02, 0x02, "Energy Loss", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Slow" )
	PORT_DIPSETTING(    0x00, "Fast" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x00, "5" )
	/* TODO: support the different settings which happen in Coin Mode 2 */
	PORT_DIPNAME( 0xf0, 0xf0, "Coinage", IP_KEY_NONE ) /* mapped on coin mode 1 */
	PORT_DIPSETTING(    0xa0, "6 Coins/1 Credit" )
	PORT_DIPSETTING(    0xb0, "5 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0xd0, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xe0, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x70, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x60, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x50, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x40, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	/* settings 0x10, 0x20, 0x80, 0x90 all give 1 Coin/1 Credit */

	PORT_START	/* DSW2 */
	PORT_DIPNAME( 0x01, 0x01, "Flip Screen", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x02, "Cocktail" )
/* This activates a different coin mode. Look at the dip switch setting schematic */
	PORT_DIPNAME( 0x04, 0x04, "Coin Mode", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "Mode 1" )
	PORT_DIPSETTING(    0x00, "Mode 2" )
	/* In slowmo mode, press 2 to slow game speed */
	PORT_BITX   ( 0x08, 0x08, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Slow Motion Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* In stop mode, press 2 to stop and 1 to restart */
	PORT_BITX   ( 0x10, 0x10, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Stop Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	/* In level selection mode, press 1 to select and 2 to restart */
	PORT_BITX   ( 0x20, 0x20, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Level Selection Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Invulnerability", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x80, 0x80, IPT_DIPSWITCH_NAME | IPF_TOGGLE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8*8 characters */
	1024,	/* 1024 characters */
	3,	/* 3 bits per pixel */
	{ 2*1024*8*8, 1024*8*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 8*0, 8*1, 8*2, 8*3, 8*4, 8*5, 8*6, 8*7 },
	8*8	/* every char takes 8 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,	/* 16*16 sprites */
	256,	/* 256 sprites */
	3,	/* 3 bits per pixel */
	{ 0x4000*8, 0x2000*8, 0 },
	{ 0, 1, 2, 3, 4, 5, 6, 7, 16*8+0, 16*8+1, 16*8+2, 16*8+3, 16*8+4, 16*8+5, 16*8+6, 16*8+7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8, 8*8, 9*8, 10*8, 11*8, 12*8, 13*8, 14*8, 15*8 },
	32*8	/* every sprite takes 32 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x00000, &charlayout,      0, 32 },	/* use colors   0-255 */
	{ 1, 0x06000, &spritelayout, 32*8, 32 },	/* use colors 256-511 */
	{ -1 } /* end of array */
};


static struct AY8910interface ay8910_interface =
{
	2,	/* 2 chips */
	910000,	/* .91 MHZ ?? */
	{ 255, 255 },
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0, mpatrol_adpcm_reset_w }
};

static struct MSM5205interface msm5205_interface =
{
	2,			/* 2 chips */
	4000,       /* 4000Hz playback */
	mpatrol_adpcm_int,/* interrupt function */
	{ 255, 255 }
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz (?) */
			0,
			readmem,writemem,readport, writeport,
			interrupt,1
		},
		{
			CPU_M6803 | CPU_AUDIO_CPU,
			1000000,	/* 1.0 Mhz ? */
			3,
			sound_readmem,sound_writemem,0,0,
			ignore_interrupt,1	/* interrupts are generated by the ADPCM hardware */
		}
	},
	55, 1790, /* frames per second and vblank duration from the Lode Runner manual */
	1,	/* 1 CPU slice per frame - interleaving is forced when a sound command is written */
	0,

	/* video hardware */
	64*8, 32*8, { 8*8, (64-8)*8-1, 0*8, 32*8-1 },
	gfxdecodeinfo,
	512, 512,
	lrunner_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	lrunner_vh_start,
	lrunner_vh_stop,
	lrunner_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		},
		{
			SOUND_MSM5205,
			&msm5205_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( lrunner_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "roma4c", 0x0000, 0x2000, 0x0246614e )
	ROM_LOAD( "roma4d", 0x2000, 0x2000, 0x6dfb93ed )
	ROM_LOAD( "roma4b", 0x4000, 0x2000, 0x6c27aa39 )
	ROM_LOAD( "roma4a", 0x6000, 0x2000, 0x81c14dd9 )

	ROM_REGION(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "romh1e", 0x0000, 0x2000, 0x48d59ac3 )	/* characters */
	ROM_LOAD( "romh1j", 0x2000, 0x2000, 0x30f168c5 )
	ROM_LOAD( "romh1h", 0x4000, 0x2000, 0x45820080 )
	ROM_LOAD( "romb4k", 0x6000, 0x2000, 0xd0fb93b3 )	/* sprites */
	ROM_LOAD( "romb3n", 0x8000, 0x2000, 0xd2282fc6 )
	ROM_LOAD( "romb4c", 0xa000, 0x2000, 0x0655cce1 )

	ROM_REGION(0x0600)	/* color proms */
	ROM_LOAD( "h-3m", 0x0000, 0x0100, 0x9a120c08 ) /* character palette red component */
	ROM_LOAD( "b-1m", 0x0100, 0x0100, 0x44d20000 ) /* sprite palette red component */
	ROM_LOAD( "h-3l", 0x0200, 0x0100, 0x94b6080e ) /* character palette green component */
	ROM_LOAD( "b-1n", 0x0300, 0x0100, 0x50780000 ) /* sprite palette green component */
	ROM_LOAD( "h-3n", 0x0400, 0x0100, 0x6af0070e ) /* character palette blue component */
	ROM_LOAD( "b-1l", 0x0500, 0x0100, 0x73100000 ) /* sprite palette blue component */

	ROM_REGION(0x10000)	/* 64k for the audio CPU (6803) */
	ROM_LOAD( "roma3e", 0xc000, 0x2000, 0xf5158e6d )	/* samples (ADPCM 4-bit) */
	ROM_LOAD( "roma3h", 0xe000, 0x2000, 0xc3f198a5 )
ROM_END



struct GameDriver lrunner_driver =
{
	__FILE__,
	0,
	"lrunner",
	"Lode Runner",
	"1984",
	"Irem (Broderbund license)",
	"Lee Taylor\nJohn Clegg\nAaron Giles (sound)",
	0,
	&machine_driver,

	lrunner_rom,
	0, 0,
	0,
	0,

	input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	0, 0
};

