/***************************************************************************

Mr. Do Wild Ride / Mr. Do Run Run memory map (preliminary)

0000-1fff ROM
2000-37ff RAM
3800-39ff Sprites
4000-9fff ROM
a000-a008 Shared with second CPU
b000-b3ff Video RAM
b400-b7ff Color RAM

write:
a800      Watchdog reset?
b800      Trigger NMI on second CPU (?)

SECOND CPU:
0000-3fff ROM
8000-87ff RAM

read:
e000-e008 data from first CPU
c003      bit 0-3 = joystick
          bit 4-7 = ?
c005      bit 0 = fire
          bit 1 = fire (again?!)
		  bit 2 = ?
		  bit 3 = START 1
		  bit 4-6 = ?
		  bit 4 = START 2
c081      coins per play

write:
e000-e008 data for first CPU
a000      sound port 1
a400      sound port 2
a800      sound port 3
ac00      sound port 4

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



int docastle_shared0_r(int offset);
int docastle_shared1_r(int offset);
void docastle_shared0_w(int offset,int data);
void docastle_shared1_w(int offset,int data);
void docastle_nmitrigger(int offset,int data);

void dowild_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom);
int docastle_vh_start(void);
void docastle_vh_stop(void);
void docastle_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh);



static struct MemoryReadAddress readmem[] =
{
	{ 0x2000, 0x37ff, MRA_RAM },
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x9fff, MRA_ROM },
	{ 0xa000, 0xa008, docastle_shared0_r },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x2000, 0x37ff, MWA_RAM },
	{ 0x3800, 0x39ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xb000, 0xb3ff, videoram_w, &videoram, &videoram_size },
	{ 0xb400, 0xb7ff, colorram_w, &colorram },
	{ 0xa000, 0xa008, docastle_shared1_w },
	{ 0xb800, 0xb800, docastle_nmitrigger },
	{ 0xa800, 0xa800, MWA_NOP },
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x9fff, MWA_ROM },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress readmem2[] =
{
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xc003, 0xc003, input_port_0_r },
	{ 0xc083, 0xc083, input_port_0_r },
	{ 0xc005, 0xc005, input_port_1_r },
	{ 0xc085, 0xc085, input_port_1_r },
	{ 0xc007, 0xc007, input_port_2_r },
	{ 0xc087, 0xc087, input_port_2_r },
	{ 0xc002, 0xc002, input_port_3_r },
	{ 0xc082, 0xc082, input_port_3_r },
	{ 0xc001, 0xc001, input_port_4_r },
	{ 0xc081, 0xc081, input_port_4_r },
	{ 0xe000, 0xe008, docastle_shared1_r },
	{ 0x0000, 0x3fff, MRA_ROM },
	{ -1 }	/* end of table */
};

static struct MemoryWriteAddress writemem2[] =
{
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xe000, 0xe008, docastle_shared0_w },
	{ 0xa000, 0xa000, SN76496_0_w },
	{ 0xa400, 0xa400, SN76496_1_w },
	{ 0xa800, 0xa800, SN76496_2_w },
	{ 0xac00, 0xac00, SN76496_3_w },
	{ 0x0000, 0x3fff, MWA_ROM },
	{ -1 }	/* end of table */
};



INPUT_PORTS_START( dowild_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as not used */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as 2 Player Fire */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as 2 Player Jump */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as not used */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as test */
/* coin input must be active for 32 frames to be consistently recognized */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, "Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 32)
	PORT_DIPNAME( 0x08, 0x08, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as not used */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as not used */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Flip Screen?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Extra", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_DIPNAME( 0x40, 0x40, "Special", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Given" )
	PORT_DIPSETTING(    0x00, "Not Given" )
	PORT_DIPNAME( 0x80, 0x80, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "2 Coins/1 Credits" )
	PORT_DIPSETTING(    0x07, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x09, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	/* 0x01, 0x02, 0x03, 0x04, 0x05 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "2 Coins/1 Credits" )
	PORT_DIPSETTING(    0x70, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x90, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	/* 0x10, 0x20, 0x30, 0x40, 0x50 all give 1 Coin/1 Credit */
INPUT_PORTS_END

INPUT_PORTS_START( jjack_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as not used */
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as 2 Player Fire */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as 2 Player Jump */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as not used */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as test */
/* coin input must be active for 32 frames to be consistently recognized */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, "Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 32)
	PORT_DIPNAME( 0x08, 0x08, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as not used */
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* reported as not used */

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Flip Screen?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Extra?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_DIPNAME( 0xc0, 0xc0, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "2" )
	PORT_DIPSETTING(    0xc0, "3" )
	PORT_DIPSETTING(    0x80, "4" )
	PORT_DIPSETTING(    0x40, "5" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "2 Coins/1 Credits" )
	PORT_DIPSETTING(    0x07, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x09, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	/* 0x01, 0x02, 0x03, 0x04, 0x05 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "2 Coins/1 Credits" )
	PORT_DIPSETTING(    0x70, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x90, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	/* 0x10, 0x20, 0x30, 0x40, 0x50 all give 1 Coin/1 Credit */
INPUT_PORTS_END


INPUT_PORTS_START( dorunrun_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_4WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_4WAY | IPF_COCKTAIL )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Reported as Test */
/* coin input must be active for 32 frames to be consistently recognized */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, "Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 32)
	PORT_DIPNAME( 0x08, 0x08, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_DIPNAME( 0x04, 0x04, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x04, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Flip Screen?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "Extra", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_DIPNAME( 0x40, 0x40, "Special", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Given" )
	PORT_DIPSETTING(    0x00, "Not Given" )
	PORT_DIPNAME( 0x80, 0x80, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "3" )
	PORT_DIPSETTING(    0x00, "5" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "2 Coins/1 Credits" )
	PORT_DIPSETTING(    0x07, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x09, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	/* 0x01, 0x02, 0x03, 0x04, 0x05 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "2 Coins/1 Credits" )
	PORT_DIPSETTING(    0x70, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x90, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	/* 0x10, 0x20, 0x30, 0x40, 0x50 all give 1 Coin/1 Credit */
INPUT_PORTS_END


INPUT_PORTS_START( kickridr_input_ports )
	PORT_START	/* IN0 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_START1 )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_START2 )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_TILT )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN ) /* Reported as Test */
/* coin input must be active for 32 frames to be consistently recognized */
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_COIN3 | IPF_IMPULSE, "Coin Aux", IP_KEY_DEFAULT, IP_JOY_DEFAULT, 32)
	PORT_DIPNAME( 0x08, 0x08, "Freeze", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_COIN2 )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_COIN1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x03, 0x03, "Difficulty?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x03, "Easy" )
	PORT_DIPSETTING(    0x02, "Medium" )
	PORT_DIPSETTING(    0x01, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
	PORT_BITX(    0x04, 0x04, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Rack Test", OSD_KEY_F1, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x04, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x08, 0x08, "Flip Screen?", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x10, 0x10, "DSW4", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x20, "Cocktail" )
	PORT_DIPNAME( 0x40, 0x40, "DSW2", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "DSW1", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "On" )
	PORT_DIPSETTING(    0x00, "Off" )

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x0f, 0x0f, "Coin B", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x08, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0x0a, "2 Coins/1 Credits" )
	PORT_DIPSETTING(    0x07, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0x0f, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x09, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0x0e, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0x0d, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0x0c, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0x0b, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	/* 0x01, 0x02, 0x03, 0x04, 0x05 all give 1 Coin/1 Credit */
	PORT_DIPNAME( 0xf0, 0xf0, "Coin A", IP_KEY_NONE )
	PORT_DIPSETTING(    0x60, "4 Coins/1 Credit" )
	PORT_DIPSETTING(    0x80, "3 Coins/1 Credit" )
	PORT_DIPSETTING(    0xa0, "2 Coins/1 Credits" )
	PORT_DIPSETTING(    0x70, "3 Coins/2 Credits" )
	PORT_DIPSETTING(    0xf0, "1 Coin/1 Credit" )
	PORT_DIPSETTING(    0x90, "2 Coins/3 Credits" )
	PORT_DIPSETTING(    0xe0, "1 Coin/2 Credits" )
	PORT_DIPSETTING(    0xd0, "1 Coin/3 Credits" )
	PORT_DIPSETTING(    0xc0, "1 Coin/4 Credits" )
	PORT_DIPSETTING(    0xb0, "1 Coin/5 Credits" )
	PORT_DIPSETTING(    0x00, "Free Play" )
	/* 0x10, 0x20, 0x30, 0x40, 0x50 all give 1 Coin/1 Credit */
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,    /* 8*8 characters */
	512,    /* 512 characters */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the bitplanes are packed in one nibble */
	{ 0, 4, 8, 12, 16, 20, 24, 28 },
	{ 0*32, 1*32, 2*32, 3*32, 4*32, 5*32, 6*32, 7*32 },
	32*8   /* every char takes 32 consecutive bytes */
};
static struct GfxLayout spritelayout =
{
	16,16,  /* 16*16 sprites */
	256,    /* 256 sprites */
	4,      /* 4 bits per pixel */
	{ 0, 1, 2, 3 }, /* the bitplanes are packed in one nibble */
	{ 0, 4, 8, 12, 16, 20, 24, 28,
			32, 36, 40, 44, 48, 52, 56, 60 },
	{ 0*64, 1*64, 2*64, 3*64, 4*64, 5*64, 6*64, 7*64,
			8*64, 9*64, 10*64, 11*64, 12*64, 13*64, 14*64, 15*64 },
	128*8  /* every sprite takes 128 consecutive bytes */
};



static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	{ 1, 0x0000, &charlayout,       0, 64 },
	{ 1, 0x4000, &spritelayout, 64*16, 32 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	4,	/* 4 chips */
	4000000,	/* 4 Mhz? */
	{ 255, 255, 255, 255 }
};



static struct MachineDriver machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* 4 Mhz */
			0,
			readmem,writemem,0,0,
			interrupt,1
		},
		{
			CPU_Z80,
			4000000,	/* 4 Mhz */
			3,	/* memory region #3 */
			readmem2, writemem2,0,0,
			interrupt,8
		}
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,	/* 1 CPU slice per frame - interleaving is forced when communication takes place */
	0,

	/* video hardware */
	32*8, 32*8, { 1*8, 31*8-1, 4*8, 28*8-1 },
	gfxdecodeinfo,
	258, 96*16,
	dowild_vh_convert_color_prom,

	VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
	0,
	docastle_vh_start,
	docastle_vh_stop,
	docastle_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};



/***************************************************************************

  Game driver(s)

***************************************************************************/

ROM_START( dowild_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "W1",  0x0000, 0x2000, 0xaeac2454 )
	ROM_LOAD( "W3",  0x4000, 0x2000, 0x54fca40a )
	ROM_LOAD( "W4",  0x6000, 0x2000, 0xb549e115 )
	ROM_LOAD( "W2",  0x8000, 0x2000, 0x443cb69e )

	ROM_REGION(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "W5",  0x0000, 0x4000, 0x2df6441e )
	ROM_LOAD( "W6",  0x4000, 0x2000, 0x2b17a47b )
	ROM_LOAD( "W7",  0x6000, 0x2000, 0xa68f1277 )
	ROM_LOAD( "W8",  0x8000, 0x2000, 0xff188768 )
	ROM_LOAD( "W9",  0xa000, 0x2000, 0xfec01f0c )

	ROM_REGION(0x0100)	/* color prom */
	ROM_LOAD( "dowild.clr",  0x0000, 0x0100, 0xd0599f79 )

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "W10", 0x0000, 0x4000, 0x431c3ca4 )
ROM_END

ROM_START( jjack_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "j1.bin",  0x0000, 0x2000, 0xcd2da76d )
	ROM_LOAD( "j3.bin",  0x4000, 0x2000, 0xaadcd8e6 )
	ROM_LOAD( "j4.bin",  0x6000, 0x2000, 0x9306bcd2 )
	ROM_LOAD( "j2.bin",  0x8000, 0x2000, 0x1c2c2a84 )

	ROM_REGION(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "j5.bin",  0x0000, 0x4000, 0xe4549bd8 )
	ROM_LOAD( "j6.bin",  0x4000, 0x2000, 0x62d40f3c )
	ROM_LOAD( "j7.bin",  0x6000, 0x2000, 0x2e94b664 )
	ROM_LOAD( "j8.bin",  0x8000, 0x2000, 0x0914de92 )
	ROM_LOAD( "j9.bin",  0xa000, 0x2000, 0xd5d6f378 )

	ROM_REGION(0x0400)	/* proms */
	ROM_LOAD( "bprom1.bin",  0x0000, 0x0200, 0x337ebd08 ) /* color prom */
	ROM_LOAD( "bprom2.bin",  0x0200, 0x0200, 0xb72016ca ) /* ??? */

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "j0.bin",  0x0000, 0x4000, 0x25a030a6 )
ROM_END

ROM_START( dorunrun_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "2764.P1",  0x0000, 0x2000, 0x8ce02038 )
	ROM_LOAD( "2764.L1",  0x4000, 0x2000, 0x8a8056e4 )
	ROM_LOAD( "2764.K1",  0x6000, 0x2000, 0xafdb2121 )
	ROM_LOAD( "2764.N1",  0x8000, 0x2000, 0xffaac0aa )

	ROM_REGION(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "27128.A3", 0x0000, 0x4000, 0x62269746 )
	ROM_LOAD( "2764.M4",  0x4000, 0x2000, 0xd51d97c3 )
	ROM_LOAD( "2764.L4",  0x6000, 0x2000, 0x423b4513 )
	ROM_LOAD( "2764.J4",  0x8000, 0x2000, 0xe53d66a5 )
	ROM_LOAD( "2764.H4",  0xa000, 0x2000, 0xf652c7f0 )

	ROM_REGION(0x0100)	/* color prom */
	ROM_LOAD( "dorunrun.clr",  0x0000, 0x0100, 0x6cce092a )

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "27128.P7", 0x0000, 0x4000, 0x0f9be91f )
ROM_END

ROM_START( spiero_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "SP1.BIN",  0x0000, 0x2000, 0xcc3428ee )
	ROM_LOAD( "SP3.BIN",  0x4000, 0x2000, 0xcc45a37d )
	ROM_LOAD( "SP4.BIN",  0x6000, 0x2000, 0xc9964244 )
	ROM_LOAD( "SP2.BIN",  0x8000, 0x2000, 0x8e065230 )

	ROM_REGION(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "SP5.BIN",  0x0000, 0x4000, 0x81ddb357 )
	ROM_LOAD( "SP6.BIN",  0x4000, 0x2000, 0x5dc7b99b )
	ROM_LOAD( "SP7.BIN",  0x6000, 0x2000, 0x37eb2d0d )
	ROM_LOAD( "SP8.BIN",  0x8000, 0x2000, 0x08e60916 )
	ROM_LOAD( "SP9.BIN",  0xa000, 0x2000, 0xff4c1b7e )

	ROM_REGION(0x0400)	/* proms */
	ROM_LOAD( "bprom1.bin",  0x0000, 0x0200, 0x265118c5 ) /* color prom */
	ROM_LOAD( "bprom2.bin",  0x0200, 0x0200, 0xb72016ca ) /* ??? */

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "SP0.BIN",  0x0000, 0x4000, 0x0f9be91f )
ROM_END

ROM_START( kickridr_rom )
	ROM_REGION(0x10000)	/* 64k for code */
	ROM_LOAD( "K1",  0x0000, 0x2000, 0xbc77e7f3 )
	ROM_LOAD( "K3",  0x4000, 0x2000, 0x299a68c4 )
	ROM_LOAD( "K4",  0x6000, 0x2000, 0xe4e2c6a0 )
	ROM_LOAD( "K2",  0x8000, 0x2000, 0x1325e773 )

	ROM_REGION(0xc000)	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "K5",  0x0000, 0x4000, 0x197f8237 )
	ROM_LOAD( "K6",  0x4000, 0x2000, 0xdc19cfbf )
	ROM_LOAD( "K7",  0x6000, 0x2000, 0x2f8ac3fe )
	ROM_LOAD( "K8",  0x8000, 0x2000, 0x88eecb70 )
	ROM_LOAD( "K9",  0xa000, 0x2000, 0x50dc5584 )

	ROM_REGION(0x0100)	/* color prom */
	ROM_LOAD( "kickridr.clr",  0x0000, 0x0100, 0x54154173 )

	ROM_REGION(0x10000)	/* 64k for the second CPU */
	ROM_LOAD( "K10", 0x0000, 0x4000, 0xd77bd167 )
ROM_END



static int dowild_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2020],"\x01\x00\x00",3) == 0 &&
			memcmp(&RAM[0x2068],"\x01\x00\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x2020],10*8);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void dowild_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x2020],10*8);
		osd_fclose(f);
	}
}



static int dorunrun_hiload(void)
{
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0x2010],"\x00\x10\x00",3) == 0 &&
			memcmp(&RAM[0x2198],"\x00\x10\x00",3) == 0)
	{
		void *f;


		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0x2010],50*8);
			osd_fclose(f);
		}

		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void dorunrun_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0x2010],50*8);
		osd_fclose(f);
	}
}



struct GameDriver dowild_driver =
{
	__FILE__,
	0,
	"dowild",
	"Mr. Do's Wild Ride",
	"1984",
	"Universal",
	"Mirko Buffoni\nNicola Salmoria\nGary Walton\nSimon Walls\nMarco Cassili",
	0,
	&machine_driver,

	dowild_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	dowild_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	dowild_hiload, dowild_hisave
};

struct GameDriver jjack_driver =
{
	__FILE__,
	0,
	"jjack",
	"Jumping Jack",
	"1984",
	"Universal",
	"Mirko Buffoni\nNicola Salmoria\nGary Walton\nSimon Walls\nMarco Cassili",
	0,
	&machine_driver,

	jjack_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	jjack_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_ROTATE_270,

	dowild_hiload, dowild_hisave
};

struct GameDriver dorunrun_driver =
{
	__FILE__,
	0,
	"dorunrun",
	"Mr. Do! Run Run",
	"1984",
	"Universal",
	"Mirko Buffoni\nNicola Salmoria\nGary Walton\nSimon Walls\nMarco Cassili",
	0,
	&machine_driver,

	dorunrun_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	dorunrun_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	dorunrun_hiload, dorunrun_hisave
};

struct GameDriver spiero_driver =
{
	__FILE__,
	&dorunrun_driver,
	"spiero",
	"Super Piero (Japan)",
	"1987",
	"Universal",
	"Mirko Buffoni\nNicola Salmoria\nGary Walton\nSimon Walls\nMarco Cassili",
	0,
	&machine_driver,

	spiero_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	dorunrun_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	dorunrun_hiload, dorunrun_hisave
};

struct GameDriver kickridr_driver =
{
	__FILE__,
	0,
	"kickridr",
	"Kick Rider",
	"1984",
	"Universal",
	"Mirko Buffoni\nNicola Salmoria\nGary Walton\nSimon Walls\nMarco Cassili",
	0,
	&machine_driver,

	kickridr_rom,
	0, 0,
	0,
	0,	/* sound_prom */

	kickridr_input_ports,

	PROM_MEMORY_REGION(2), 0, 0,
	ORIENTATION_DEFAULT,

	dowild_hiload, dowild_hisave
};
