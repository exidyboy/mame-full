/***************************************************************************

D-Day memory map (preliminary)

Note: This game doesn't seem to support cocktail mode, which is not too
      suprising for a gun game.

0000-3fff ROM
5000-53ff Foreground RAM 1
5400-57ff Foreground RAM 2
5800-5bff Background RAM (Only the first 28 lines are visible,
						  the last 0x80 bytes probably contain color
						  information)
5c00-5fff Attributes RAM for Foreground 2
          A0-A5 seem to be ignored.
          D0 - X Flip
          D2 - Used by the software to separate area that the short shots
               cannot penetrate
          Others unknown, they don't seem to be used by this game
6000-63ff RAM

read:

6c00  Input Port #1
7000  Dip Sw #1
7400  Dip Sw #2
7800  Timer
7c00  Analog Control

write:

4000 ? (Value written to it depends on the gun's position)
6400 AY8910 #1 Control Port
6401 AY8910 #1 Write Port
6800 AY8910 #2 Control Port
6801 AY8910 #2 Write Port
7800 Bit 0 - Coin Counter 1
     Bit 1 - Coin Counter 2
	 Bit 2 - ??? Pulsated when the player is hit
	 Bit 3 - ??? Seems to be unused
	 Bit 4 - Tied to AY8910 RST. Used to turn off sound
	 Bit 5 - ??? Seem to be always on
	 Bit 6 - ???
     Bit 7 - ??? Seems to be unused


***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

extern unsigned char *dday_videoram2;
extern unsigned char *dday_videoram3;

void dday_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
void dday_vh_screenrefresh(struct osd_bitmap *bitmap);
void dday_colorram_w(int offset, int data);
int  dday_colorram_r(int offset);
void dday_control_w (int offset, int data);
void dday_AY8910_0_w(int offset, int data);
void dday_AY8910_1_w(int offset, int data);

// Note: There seems to be no way to reset this timer via hardware.
//       The game uses a difference method to reset it to 99.
//
// Thanks Zwaxy for the timer info.

#define START_TIMER 99
static int timerVal = START_TIMER;

static int dday_timer_r (int offset)
{
    int tens, units;

    tens = timerVal / 10;
    units = timerVal - tens * 10;

    return (16 * tens + units);
}

// This is not a real interrupt routine. It is just used to decrement the
// counter.
static int dday_interrupt (void)
{
    #define START_TIMER_SMALL 60
    static int timerValSmall = START_TIMER_SMALL;
    /* if the timer hits zero , start over at START_TIMER */
    timerValSmall--;
    if (timerValSmall == 0)
    {
		timerValSmall = START_TIMER_SMALL;
		timerVal--;
		if (timerVal == -1) timerVal = START_TIMER;
    }

    return ignore_interrupt();
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x3fff, MRA_ROM },
	{ 0x5000, 0x5bff, MRA_RAM },
	{ 0x5c00, 0x5fff, dday_colorram_r },
	{ 0x6000, 0x63ff, MRA_RAM },
	{ 0x6c00, 0x6c00, input_port_0_r },
	{ 0x7000, 0x7000, input_port_1_r },
	{ 0x7400, 0x7400, input_port_2_r },
	{ 0x7800, 0x7800, dday_timer_r },
	{ 0x7c00, 0x7c00, input_port_3_r },
	{ -1 }  /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x4000, 0x4000, MWA_NOP }, // ???
	{ 0x5000, 0x53ff, MWA_RAM, &dday_videoram2 },
	{ 0x5400, 0x57ff, MWA_RAM, &dday_videoram3 },
	{ 0x5800, 0x5bff, videoram_w, &videoram, &videoram_size },
	{ 0x5c00, 0x5fff, dday_colorram_w, &colorram },
	{ 0x6000, 0x63ff, MWA_RAM },
	{ 0x6400, 0x640d, dday_AY8910_0_w },
	{ 0x6800, 0x6801, dday_AY8910_1_w },
	{ 0x7800, 0x7800, dday_control_w },
	{ -1 }  /* end of table */
};



INPUT_PORTS_START( input_ports )
	PORT_START      /* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_COIN1 )
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_COIN2 )
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_BUTTON1 ) // Fire Button
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_START2 )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON2 ) // Distance Button
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNKNOWN ) // Doesn't seem to be
                                                  // accessed

	PORT_START      /* DS0 */
	PORT_DIPNAME( 0x03, 0x01, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0x01, "3" )
	PORT_DIPSETTING(    0x02, "4" )
	PORT_DIPSETTING(    0x03, "5" )
	PORT_DIPNAME( 0x0c, 0x00, "Extended Play At", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "4,000" )
	PORT_DIPSETTING(    0x04, "6,000" )
	PORT_DIPSETTING(    0x08, "8,000" )
	PORT_DIPSETTING(    0x0c, "10,000" )
	PORT_DIPNAME( 0x30, 0x10, "Difficulty", IP_KEY_NONE )  // Easiest - No Bombs, No Troop Carriers
	PORT_DIPSETTING(    0x30, "Easiest" )                  // Easy    - No Bombs, Troop Carriers
	PORT_DIPSETTING(    0x20, "Easy" )                     // Hard    - Bombs, Troop Carriers
	PORT_DIPSETTING(    0x10, "Hard" )
  //PORT_DIPSETTING(    0x00, "Hard" ) // Same as 0x10
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE ) // Doesn't seem to be used
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x40, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Start with 20,000 Pts", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DS0 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x0d, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x09, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x07, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x05, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x03, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x01, "1 Coin/8 Credits" )
	PORT_DIPSETTING(    0x0e, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0c, "2 Coins/2 Credits" )
	PORT_DIPSETTING(    0x0a, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x08, "2 Coins/4 Credits" )
	PORT_DIPSETTING(    0x06, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x04, "2 Coins/6 Credits" )
	PORT_DIPSETTING(    0x02, "2 Coins/7 Credits" )
	PORT_DIPSETTING(    0x00, "2 Coins/8 Credits" )
	PORT_DIPNAME( 0xf0, 0xf0, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0xd0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x90, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x70, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x50, "1 Coin/6 Credits" )
	PORT_DIPSETTING(    0x30, "1 Coin/7 Credits" )
	PORT_DIPSETTING(    0x10, "1 Coin/8 Credits" )
	PORT_DIPSETTING(    0xe0, "2 Coins/1 Credit" )
	PORT_DIPSETTING(    0xc0, "2 Coins/2 Credits" )
	PORT_DIPSETTING(    0xa0, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x80, "2 Coins/4 Credits" )
	PORT_DIPSETTING(    0x60, "2 Coins/5 Credits" )
	PORT_DIPSETTING(    0x40, "2 Coins/6 Credits" )
	PORT_DIPSETTING(    0x20, "2 Coins/7 Credits" )
	PORT_DIPSETTING(    0x00, "2 Coins/8 Credits" )

	PORT_START      /* IN0 */
	PORT_ANALOG (0xff, 96, IPT_PADDLE, 20, 0, 0, 191 )
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	2,      /* 2 bits per pixel */
	{ 0, 0x0800*8 }, /* the two bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxLayout charlayout2 =
{
	8,8,    /* 8*8 characters */
	256,    /* 256 characters */
	3,      /* 3 bits per pixel */
	{ 0, 0x0800*8, 0x1000*8 }, /* the three bitplanes are separated */
	{ 0, 1, 2, 3, 4, 5, 6, 7 },
	{ 0*8, 1*8, 2*8, 3*8, 4*8, 5*8, 6*8, 7*8 },
	8*8     /* every char takes 8 consecutive bytes */
};

static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout , 1*8, 1 },
	{ 1, 0x1000, &charlayout2, 0,   1 },
	{ 1, 0x2800, &charlayout , 1*8, 1 },
	{ -1 } /* end of array */
};


/* This is handmade */
static unsigned char dday_color_prom[] =
{
	/* palette red component*/
	0x00,0x00,0x00,0x00,0xff,0xff,0xff,0xff,
	0x00,0xc0,0x00,0x40,

	/* palette green component*/
	0x00,0xff,0xff,0x00,0x00,0xff,0x00,0xff,
	0x00,0xc0,0x00,0x40,

	/* palette blue component*/
	0x00,0xff,0x00,0xff,0xff,0xff,0x00,0xff,
	0x00,0xc0,0x00,0x40,

	/* The following are the real PROMs, currently not supported */
	/* prom.m11 */
	0x09,0x00,0x00,0x0F,0x0F,0x0F,0x03,0x09,0x0F,0x0F,0x03,0x09,0x0F,0x00,0x03,0x09,
	0x09,0x00,0x00,0x0F,0x0F,0x0F,0x03,0x09,0x0F,0x0F,0x03,0x09,0x0F,0x00,0x03,0x09,
	0x00,0x0F,0x0F,0x00,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,
	0x00,0x0F,0x0F,0x00,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,
	0x00,0x0B,0x0F,0x00,0x00,0x0F,0x00,0x0F,0x00,0x0B,0x00,0x00,0x00,0x0F,0x00,0x0F,
	0x00,0x0B,0x0F,0x00,0x00,0x0F,0x00,0x0F,0x00,0x0B,0x00,0x00,0x00,0x0F,0x00,0x0F,
	0x00,0x0F,0x0F,0x00,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,
	0x00,0x0F,0x0F,0x00,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,
	0x0F,0x00,0x00,0x0F,0x0F,0x0B,0x0F,0x09,0x0F,0x0B,0x0F,0x09,0x0F,0x00,0x0F,0x09,
	0x0F,0x00,0x00,0x0F,0x0F,0x0B,0x0F,0x09,0x0F,0x0B,0x0F,0x09,0x0F,0x00,0x0F,0x09,
	0x00,0x0F,0x0F,0x00,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,
	0x00,0x0F,0x0F,0x00,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,
	0x00,0x0B,0x0F,0x00,0x00,0x0F,0x00,0x0F,0x00,0x0B,0x00,0x00,0x00,0x0F,0x0F,0x0F,
	0x00,0x0B,0x0F,0x00,0x00,0x0F,0x00,0x0F,0x00,0x0B,0x00,0x00,0x00,0x0F,0x0F,0x0F,
	0x00,0x0F,0x0F,0x00,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,
	0x00,0x0F,0x0F,0x00,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,0x00,0x0F,
	/* prom.m3 */
	0x00,0x00,0x00,0x00,0x00,0x0F,0x0B,0x0F,0x00,0x00,0x00,0x0B,0x00,0x00,0x00,0x0B,
	0x0F,0x00,0x00,0x0F,0x0B,0x00,0x00,0x00,0x0B,0x00,0x00,0x00,0x0B,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x0F,0x00,0x00,0x00,0x0F,0x00,0x00,0x00,0x0F,0x00,0x00,0x00,0x0F,
	0x00,0x0B,0x0F,0x00,0x00,0x00,0x00,0x0F,0x00,0x0B,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x0B,0x0B,0x0F,0x00,0x00,0x0F,0x0B,0x00,0x00,0x0F,0x0B,
	0x00,0x00,0x00,0x0D,0x0B,0x00,0x0F,0x0D,0x0B,0x00,0x0F,0x0D,0x0B,0x00,0x0F,0x0D,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x0F,0x00,0x00,0x00,0x0F,0x00,0x00,0x00,0x0F,0x00,0x00,0x00,0x0F,
	0x00,0x0B,0x0F,0x00,0x00,0x00,0x00,0x0F,0x00,0x0B,0x00,0x00,0x0F,0x0F,0x0F,0x0F,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0F,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,
	/* prom.m8 */
	0x00,0x00,0x0B,0x0D,0x00,0x00,0x0D,0x0F,0x00,0x0B,0x00,0x0D,0x00,0x0B,0x00,0x0D,
	0x0F,0x00,0x00,0x0F,0x0D,0x0B,0x00,0x03,0x0D,0x0B,0x00,0x03,0x0D,0x0D,0x00,0x03,
	0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x0F,0x00,0x00,0x0D,0x0D,0x0F,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x0F,
	0x00,0x00,0x07,0x0F,0x00,0x00,0x07,0x0F,0x00,0x00,0x07,0x0F,0x00,0x00,0x00,0x0F,
	0x00,0x0B,0x0F,0x00,0x00,0x00,0x0D,0x0F,0x00,0x0B,0x07,0x00,0x00,0x00,0x00,0x0F,
	0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x0F,0x00,0x00,0x0D,0x0D,0x0F,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x03,0x0F,0x00,0x00,0x00,0x0F,0x00,0x03,0x0F,0x00,0x00,0x03,0x0F,0x00,
	0x00,0x00,0x00,0x0D,0x00,0x03,0x0F,0x03,0x00,0x03,0x0F,0x03,0x00,0x0F,0x0F,0x03,
	0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x0F,0x00,0x00,0x0D,0x0D,0x0F,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x0F,
	0x00,0x00,0x07,0x0F,0x00,0x00,0x07,0x0F,0x00,0x00,0x07,0x0F,0x00,0x00,0x00,0x0F,
	0x00,0x0B,0x0F,0x00,0x00,0x0D,0x0D,0x0F,0x00,0x0B,0x07,0x00,0x00,0x0F,0x0F,0x0F,
	0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x0F,0x00,0x00,0x0D,0x0D,0x0F,0x00,0x0F,0x00,0x00,0x00,0x00,0x00,0x00,
};



static struct AY8910interface ay8910_interface =
{
	2,      /* 2 chips */
	1000000,	/* 1.0 MHz ? */
	{ 0x20ff , 0x20ff }, /* ??? */
	{ 0 },
	{ 0 },
	{ 0 },
	{ 0 }
};


static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			2000000,     /* 2 Mhz ? */
			0,
			readmem,writemem,0,0,
			dday_interrupt,1
		}
	},
	60, DEFAULT_REAL_60HZ_VBLANK_DURATION, /* frames per second, vblank duration */
	1,      /* single CPU, no need for interleaving */
	0,

	/* video hardware */
	32*8, 32*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
	gfxdecodeinfo,
	12,8+4,
	dday_vh_convert_color_prom,

	VIDEO_TYPE_RASTER,
	0,
	generic_vh_start,
	generic_vh_stop,
	dday_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_AY8910,
			&ay8910_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( dday_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "e8_63-c.bin", 0x0000, 0x1000, 0xd98d8133 )
	ROM_LOAD( "e7_64-c.bin", 0x1000, 0x1000, 0xb7a2e008 )
	ROM_LOAD( "e6_65-c.bin", 0x2000, 0x1000, 0xb96d198b )
	ROM_LOAD( "e5_66-c.bin", 0x3000, 0x1000, 0x462fc99b )

	ROM_REGION(0x3800)      /* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "j8_70-c.bin", 0x0000, 0x0800, 0xeb906e16 )
	ROM_LOAD( "j9_69-c.bin", 0x0800, 0x0800, 0xcbf2783c )
	ROM_LOAD( "k2_71.bin",   0x1000, 0x0800, 0xc1045ebc )
	ROM_LOAD( "k3_72.bin",   0x1800, 0x0800, 0x4ceb70af )
	ROM_LOAD( "k4_73.bin",   0x2000, 0x0800, 0x64539441 )
	ROM_LOAD( "k6_74.bin",   0x2800, 0x0800, 0x11150ecf )
	ROM_LOAD( "k7_75.bin",   0x3000, 0x0800, 0xc763dbc1 )

	// There are also 2 unused ROMs in this ROM set. Don't know what they do
ROM_END



static int hiload(void)
{
	unsigned char *RAM = Machine->memory_region[0];

	/* check if the hi score table has already been initialized */
	if (RAM[0x537d] == 0x20)
	{
		void *f;

		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x6237],0x03);
			osd_fclose(f);

			// Move it to the screen, too
			RAM[0x5379] =  (RAM[0x6237] & 0x0f)       | 0x20;
			RAM[0x537a] = ((RAM[0x6238] & 0xf0) >> 4) | 0x20;
			RAM[0x537b] =  (RAM[0x6238] & 0x0f)       | 0x20;
			RAM[0x537c] = ((RAM[0x6239] & 0xf0) >> 4) | 0x20;
			RAM[0x537d] =  (RAM[0x6239] & 0x0f)       | 0x20;
		}

		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}



static void hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x6237],0x03);
		osd_fclose(f);
	}
}


struct GameDriver dday_driver =
{
	"D-Day",
	"dday",
	"Zsolt Vasvari\nHowie Cohen\nChris Moore\nBrad Oliver",
	&machine_driver,

	dday_rom,
	0, 0,
	0,
	0,      /* sound_prom */

	input_ports,

	dday_color_prom, 0, 0,
	ORIENTATION_FLIP_X,

	hiload, hisave
};
