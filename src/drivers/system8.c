/******************************************************************************

Up'n Down, Mister Viking, Flicky, S.W.A.T., Water Match and Bull Fight are known
to run on IDENTICAL hardware (they were sold by Bally-Midway as ROM swaps).

******************************************************************************/

#include "driver.h"
#include "vidhrdw/system8.h"

/* in machine/segacrpt.c */
void mrviking_decode(void);
void flicky_decode(void);
void seganinj_decode(void);
void imsorry_decode(void);
void teddybb_decode(void);


static void system8_init_machine(void)
{
	/* skip the long IC CHECK in Teddyboy Blues and Choplifter */
	/* this is not a ROM patch, the game checks a RAM location */
	/* before doing the test */
	Machine->memory_region[0][0xeffe] = 0x4f;
	Machine->memory_region[0][0xefff] = 0x4b;

	system8_define_sprite_pixelmode(SYSTEM8_SPRITE_PIXEL_MODE1);
}

static void choplift_init_machine(void)
{
	/* skip the long IC CHECK in Teddyboy Blues and Choplifter */
	/* this is not a ROM patch, the game checks a RAM location */
	/* before doing the test */
	Machine->memory_region[0][0xeffe] = 0x4f;
	Machine->memory_region[0][0xefff] = 0x4b;

	system8_define_sprite_pixelmode(SYSTEM8_SPRITE_PIXEL_MODE2);
}


void choplift_bankswitch_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + (((data & 0x0c)>>2) * 0x4000);
	cpu_setbank(1,&RAM[bankaddress]);

	system8_videomode_w(offset,data);
}

void wbml_bankswitch_w(int offset,int data)
{
	int bankaddress;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	bankaddress = 0x10000 + (((data & 0x0c)>>2) * 0x4000);
	cpu_setbank(1,&RAM[bankaddress]);
	/* TODO: the memory system doesn't yet support bank switching on an encrypted */
	/* ROM, so we have to copy the data manually */
	memcpy(&ROM[0x8000],&RAM[bankaddress+0x20000],0x4000);

	system8_videomode_w(offset,data);
}

void system8_soundport_w(int offset, int data)
{
	soundlatch_w(0,data);
	cpu_cause_interrupt(1,Z80_NMI_INT);
}



static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0xbfff, MRA_ROM },
	{ 0xc000, 0xefff, MRA_RAM },
	{ 0xf020, 0xf03f, MRA_RAM },
	{ 0xf800, 0xfbff, MRA_RAM },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAMROM },
	{ 0xd000, 0xd1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd800, 0xdfff, system8_paletteram_w, &paletteram },
	{ 0xe000, 0xe7ff, system8_backgroundram_w, &system8_backgroundram, &system8_backgroundram_size },
	{ 0xe800, 0xeeff, MWA_RAM, &system8_videoram, &system8_videoram_size },
	{ 0xefbd, 0xefbd, MWA_RAM, &system8_scroll_y },
	{ 0xeffc, 0xeffd, MWA_RAM, &system8_scroll_x },
	{ 0xf000, 0xf3ff, system8_background_collisionram_w, &system8_background_collisionram },
	{ 0xf800, 0xfbff, system8_sprites_collisionram_w, &system8_sprites_collisionram },
	{ -1 } /* end of table */
};

static struct MemoryReadAddress wbml_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0xbfff, MRA_BANK1 },
	{ 0xc000, 0xffff, MRA_RAM },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress wbml_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xc000, 0xcfff, MWA_RAM },
	{ 0xd000, 0xd1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd800, 0xddff, system8_paletteram_w, &paletteram },
	{ 0xe000, 0xefff, wbml_paged_videoram_w },
	{ 0xf000, 0xf3ff, system8_background_collisionram_w, &system8_background_collisionram },
	{ 0xf800, 0xfbff, system8_sprites_collisionram_w, &system8_sprites_collisionram },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress choplift_writemem[] =
{
	{ 0x0000, 0xbfff, MWA_ROM },
	{ 0xd000, 0xd1ff, MWA_RAM, &spriteram, &spriteram_size },
	{ 0xd800, 0xdfff, system8_paletteram_w, &paletteram },
	{ 0xe7c0, 0xe7ff, choplifter_scroll_x_w, &system8_scrollx_ram },
	{ 0xe000, 0xe7ff, system8_videoram_w, &system8_videoram, &system8_videoram_size },
	{ 0xe800, 0xeeff, system8_backgroundram_w, &system8_backgroundram, &system8_backgroundram_size },
	{ 0xf000, 0xf3ff, system8_background_collisionram_w, &system8_background_collisionram },
	{ 0xf800, 0xfbff, system8_sprites_collisionram_w, &system8_sprites_collisionram },
	{ 0xc000, 0xfbff, MWA_RAM },
	{ -1 } /* end of table */
};

static struct IOReadPort readport[] =
{
	{ 0x0000, 0x0000, input_port_0_r },	/* joy1 */
	{ 0x0004, 0x0004, input_port_1_r },	/* joy2 */
	{ 0x0008, 0x0008, input_port_2_r },	/* coin,start */
	{ 0x000c, 0x000c, input_port_3_r },	/* DIP2 */
	{ 0x000d, 0x000d, input_port_4_r },	/* DIP1 some games read it from here... */
	{ 0x0010, 0x0010, input_port_4_r },	/* DIP1 ... and some others from here */
										/* but there are games which check BOTH! */
	{ 0x0015, 0x0015, system8_videomode_r },
	{ 0x0019, 0x0019, system8_videomode_r },	/* mirror address */
	{ -1 }	/* end of table */
};

static struct IOWritePort writeport[] =
{
	{ 0x0014, 0x0014, system8_soundport_w },	/* sound commands */
	{ 0x0015, 0x0015, system8_videomode_w },	/* video control and (in some games) bank switching */
	{ 0x0018, 0x0018, system8_soundport_w },	/* mirror address */
	{ 0x0019, 0x0019, system8_videomode_w },	/* mirror address */
	{ -1 }	/* end of table */
};

static struct IOReadPort wbml_readport[] =
{
	{ 0x0000, 0x0000, input_port_0_r },	/* joy1 */
	{ 0x0004, 0x0004, input_port_1_r },	/* joy2 */
	{ 0x0008, 0x0008, input_port_2_r },	/* coin,start */
	{ 0x000c, 0x000c, input_port_3_r },	/* DIP2 */
	{ 0x000d, 0x000d, input_port_4_r },	/* DIP1 */
	{ 0x0015, 0x0015, system8_videomode_r },
	{ 0x0016, 0x0016, wbml_bg_bankselect_r },
	{ -1 }	/* end of table */
};

static struct IOWritePort wbml_writeport[] =
{
	{ 0x0014, 0x0014, system8_soundport_w },	/* sound commands */
	{ 0x0015, 0x0015, wbml_bankswitch_w },
	{ 0x0016, 0x0016, wbml_bg_bankselect_w },
	{ -1 }	/* end of table */
};

static struct IOWritePort choplift_writeport[] =
{
	{ 0x0014, 0x0014, system8_soundport_w },	/* sound commands */
	{ 0x0015, 0x0015, choplift_bankswitch_w },
	{ -1 }	/* end of table */
};


static struct MemoryReadAddress sound_readmem[] =
{
	{ 0x0000, 0x7fff, MRA_ROM },
	{ 0x8000, 0x87ff, MRA_RAM },
	{ 0xe000, 0xe000, soundlatch_r },
	{ -1 } /* end of table */
};

static struct MemoryWriteAddress sound_writemem[] =
{
	{ 0x0000, 0x7fff, MWA_ROM },
	{ 0x8000, 0x87ff, MWA_RAM },
	{ 0xa000, 0xa003, SN76496_0_w },	/* Choplifter writes to the four addresses */
	{ 0xc000, 0xc003, SN76496_1_w },	/* in sequence */
	{ -1 } /* end of table */
};


#define IN0_PORT \
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_COIN1 ) \
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_COIN2 ) \
	PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_SERVICE, "Service Mode", OSD_KEY_F2, IP_JOY_NONE, 0 ) \
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_COIN3 ) \
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_START1 ) \
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_START2 ) \
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_UNKNOWN ) \
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_UNKNOWN )

#define DSW1_PORT \
	PORT_DIPNAME( 0x0f, 0x0f, "A Coin/Cred", IP_KEY_NONE ) \
	PORT_DIPSETTING(    0x07, "4/1" ) \
	PORT_DIPSETTING(    0x08, "3/1" ) \
	PORT_DIPSETTING(    0x09, "2/1" ) \
	PORT_DIPSETTING(    0x05, "2/1 + Bonus each 5" ) \
	PORT_DIPSETTING(    0x04, "2/1 + Bonus each 4" ) \
	PORT_DIPSETTING(    0x0f, "1/1" ) \
	PORT_DIPSETTING(    0x03, "1/1 + Bonus each 5" ) \
	PORT_DIPSETTING(    0x02, "1/1 + Bonus each 4" ) \
	PORT_DIPSETTING(    0x01, "1/1 + Bonus each 2" ) \
	PORT_DIPSETTING(    0x06, "2/3" ) \
	PORT_DIPSETTING(    0x0e, "1/2" ) \
	PORT_DIPSETTING(    0x0d, "1/3" ) \
	PORT_DIPSETTING(    0x0c, "1/4" ) \
	PORT_DIPSETTING(    0x0b, "1/5" ) \
	PORT_DIPSETTING(    0x0a, "1/6" ) \
/*	PORT_DIPSETTING(    0x00, "1/1" ) */ \
	PORT_DIPNAME( 0xf0, 0xf0, "B Coin/Cred", IP_KEY_NONE ) \
	PORT_DIPSETTING(    0x70, "4/1" ) \
	PORT_DIPSETTING(    0x80, "3/1" ) \
	PORT_DIPSETTING(    0x90, "2/1" ) \
	PORT_DIPSETTING(    0x50, "2/1 + Bonus each 5" ) \
	PORT_DIPSETTING(    0x40, "2/1 + Bonus each 4" ) \
	PORT_DIPSETTING(    0xf0, "1/1" ) \
	PORT_DIPSETTING(    0x30, "1/1 + Bonus each 5" ) \
	PORT_DIPSETTING(    0x20, "1/1 + Bonus each 4" ) \
	PORT_DIPSETTING(    0x10, "1/1 + Bonus each 2" ) \
	PORT_DIPSETTING(    0x60, "2/3" ) \
	PORT_DIPSETTING(    0xe0, "1/2" ) \
	PORT_DIPSETTING(    0xd0, "1/3" ) \
	PORT_DIPSETTING(    0xc0, "1/4" ) \
	PORT_DIPSETTING(    0xb0, "1/5" ) \
	PORT_DIPSETTING(    0xa0, "1/6" ) \
/*	PORT_DIPSETTING(    0x00, "1/1" ) */


INPUT_PORTS_START( starjack_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x06, 0x06, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x38, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "20000 50000" )
	PORT_DIPSETTING(    0x20, "30000 70000" )
	PORT_DIPSETTING(    0x10, "40000 90000" )
	PORT_DIPSETTING(    0x00, "50000 110000" )
	PORT_DIPSETTING(    0x38, "20000" )
	PORT_DIPSETTING(    0x28, "30000" )
	PORT_DIPSETTING(    0x18, "40000" )
	PORT_DIPSETTING(    0x08, "50000" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "Easy" )
	PORT_DIPSETTING(    0x80, "Medium" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( starjacs_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x06, 0x06, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x08, 0x08, "Ship", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "Single" )
	PORT_DIPSETTING(    0x00, "Multi" )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "30000 70000" )
	PORT_DIPSETTING(    0x20, "40000 90000" )
	PORT_DIPSETTING(    0x10, "50000 110000" )
	PORT_DIPSETTING(    0x00, "60000 130000" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "Easy" )
	PORT_DIPSETTING(    0x80, "Medium" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( mrviking_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x10, 0x10, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x20, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( upndown_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x06, 0x06, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x38, 0x38, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x38, "10000" )
	PORT_DIPSETTING(    0x30, "20000" )
	PORT_DIPSETTING(    0x28, "30000" )
	PORT_DIPSETTING(    0x20, "40000" )
	PORT_DIPSETTING(    0x18, "50000" )
	PORT_DIPSETTING(    0x10, "60000" )
	PORT_DIPSETTING(    0x08, "70000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "Easy" )
	PORT_DIPSETTING(    0x80, "Medium" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPSETTING(    0x00, "Hardest" )
INPUT_PORTS_END

INPUT_PORTS_START( flicky_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_2WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x02, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "30000 80000 160000" )
	PORT_DIPSETTING(    0x20, "30000 100000 200000" )
	PORT_DIPSETTING(    0x10, "40000 120000 240000" )
	PORT_DIPSETTING(    0x00, "40000 140000 280000" )
	PORT_DIPNAME( 0xc0, 0xc0, "Difficulty?", IP_KEY_NONE )
	PORT_DIPSETTING(    0xc0, "Easy?" )
	PORT_DIPSETTING(    0x80, "Medium?" )
	PORT_DIPSETTING(    0x40, "Hard?" )
	PORT_DIPSETTING(    0x00, "Hardest?" )
INPUT_PORTS_END

INPUT_PORTS_START( pitfall2_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x06, 0x06, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x06, "3")
	PORT_DIPSETTING(    0x04, "4" )
	PORT_DIPSETTING(    0x02, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x18, 0x18, "Starting Stage", IP_KEY_NONE )
	PORT_DIPSETTING(    0x18, "1")
	PORT_DIPSETTING(    0x10, "2" )
	PORT_DIPSETTING(    0x08, "3" )
	PORT_DIPSETTING(    0x00, "4" )
	PORT_DIPNAME( 0x20, 0x00, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "No")
	PORT_DIPSETTING(    0x00, "Yes")
	PORT_DIPNAME( 0x40, 0x40, "Time", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "3 minutes" )
	PORT_DIPSETTING(    0x00, "2 minutes" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( seganinj_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_BUTTON3 | IPF_COCKTAIL )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0C, 0x0C, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0C, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( imsorry_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0C, 0x0C, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0C, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "30000" )
	PORT_DIPSETTING(    0x20, "40000" )
	PORT_DIPSETTING(    0x10, "50000" )
	PORT_DIPSETTING(    0x00, "None" )
	PORT_DIPNAME( 0x40, 0x40, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( teddybb_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x02, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Off" )
	PORT_DIPSETTING(    0x02, "On" )
	PORT_DIPNAME( 0x0C, 0x0C, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x0C, "3" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x30, 0x30, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x30, "100k 400k" )
	PORT_DIPSETTING(    0x20, "200k 600k" )
	PORT_DIPSETTING(    0x10, "400k 800k" )
	PORT_DIPSETTING(    0x00, "600k" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x80, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( myhero_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0C, 0x0C, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0C, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x40, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
INPUT_PORTS_END

INPUT_PORTS_START( wbdeluxe_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_UNKNOWN ) /* Has to be 0 otherwise the game resets */
												/* if you die after level 1. */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* down - unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* up - unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_2WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_2WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* down - unused */
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* up - unused */
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START      /* DSW1 */
	DSW1_PORT

	PORT_START	/* DSW0 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "3" )
	PORT_DIPSETTING(    0x08, "4" )
	PORT_DIPSETTING(    0x04, "5" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x10, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "30000 100000 170000" )
	PORT_DIPSETTING(    0x00, "30000 120000 210000" )
	PORT_DIPNAME( 0x20, 0x20, "Allow Continue", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "No" )
	PORT_DIPSETTING(    0x20, "Yes" )
	PORT_DIPNAME( 0x40, 0x40, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x40, "Easy" )
	PORT_DIPSETTING(    0x00, "Hard" )
	PORT_DIPNAME( 0x80, 0x00, "Energy Consumption", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Slow" )
	PORT_DIPSETTING(    0x80, "Fast" )
INPUT_PORTS_END

INPUT_PORTS_START( choplift_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Cabinet", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Upright" )
	PORT_DIPSETTING(    0x01, "Cocktail" )
	PORT_DIPNAME( 0x02, 0x00, "Demo Sounds", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x0c, "3")
	PORT_DIPSETTING(    0x08, "2" )
	PORT_DIPSETTING(    0x04, "4" )
	PORT_BITX( 0,       0x00, IPT_DIPSWITCH_SETTING | IPF_CHEAT, "Infinite", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Bonus Life", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "20000,70000,...")
	PORT_DIPSETTING(    0x20, "50000,100000,...")
	PORT_DIPNAME( 0x40, 0x00, "Difficulty", IP_KEY_NONE )
	PORT_DIPSETTING(    0x00, "Easy" )
	PORT_DIPSETTING(    0x40, "Hard" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW0 */
	DSW1_PORT
INPUT_PORTS_END

INPUT_PORTS_START( wbml_input_ports )
	PORT_START	/* IN1 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY )

	PORT_START	/* IN2 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_BUTTON2 | IPF_COCKTAIL )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_BUTTON1 | IPF_COCKTAIL )
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_UNKNOWN )
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN  | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_JOYSTICK_UP    | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT | IPF_8WAY | IPF_COCKTAIL )
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT  | IPF_8WAY | IPF_COCKTAIL )

	PORT_START	/* IN0 */
	IN0_PORT

	PORT_START	/* DSW1 */
	PORT_DIPNAME( 0x01, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x01, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x02, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x02, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x0c, 0x0c, "Lives", IP_KEY_NONE )
	PORT_DIPSETTING(    0x04, "3" )
	PORT_DIPSETTING(    0x0c, "4" )
	PORT_DIPSETTING(    0x08, "5" )
/*	PORT_DIPSETTING(    0x00, "4" ) */
	PORT_DIPNAME( 0x10, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x10, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x20, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x20, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_BITX(    0x40, 0x40, IPT_DIPSWITCH_NAME | IPF_CHEAT, "Test Mode", IP_KEY_NONE, IP_JOY_NONE, 0 )
	PORT_DIPSETTING(    0x40, "Off" )
	PORT_DIPSETTING(    0x00, "On" )
	PORT_DIPNAME( 0x80, 0x00, "Unknown", IP_KEY_NONE )
	PORT_DIPSETTING(    0x80, "Off" )
	PORT_DIPSETTING(    0x00, "On" )

	PORT_START      /* DSW0 */
	DSW1_PORT
INPUT_PORTS_END



static struct GfxLayout charlayout =
{
	8,8,	/* 8 by 8 */
	2048,	/* 2048 characters */
	3,		/* 3 bits per pixel */
	{ 0, 2048*8*8, 2*2048*8*8 },			/* plane */
	{ 0, 1, 2, 3, 4, 5, 6, 7},			/* x bit */
	{ 0, 8, 16, 24, 32, 40, 48, 56},	/* y bit */
	8*8
};

static struct GfxLayout choplift_charlayout =
{
	8,8,	/* 8 by 8 */
	4096,	/* 4096 characters */
	3,	/* 3 bits per pixel */
	{ 0, 4096*8*8, 2*4096*8*8 },		/* plane */
	{ 0, 1, 2, 3, 4, 5, 6, 7},		/* x bit */
	{ 0, 8, 16, 24, 32, 40, 48, 56},	/* y bit */
	8*8
};


static struct GfxDecodeInfo gfxdecodeinfo[] =
{
	/* sprites use colors 0-511, but are not defined here */
	{ 1, 0x0000, &charlayout, 512, 128 },
	{ -1 } /* end of array */
};

static struct GfxDecodeInfo choplift_gfxdecodeinfo[] =
{
	/* sprites use colors 0-511, but are not defined here */
	{ 1, 0x0000, &choplift_charlayout, 512, 128 },
	{ -1 } /* end of array */
};



static struct SN76496interface sn76496_interface =
{
	2,		/* 2 chips */
	2500000,	/* 20 MHz / 8 ?*/
	{ 255, 255 }
};



static struct MachineDriver system8_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* My Hero has 2 OSCs 8 & 20 MHz (Cabbe Info) */
			0,		/* memory region */
			readmem,writemem,readport,writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,
			3,		/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,4			/* NMIs are caused by the main CPU */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,					/* single CPU, no need for interleaving */
	system8_init_machine,

	/* video hardware */
	256, 256,				/* screen_width, screen_height */
	{ 0*8, 32*8-1, 0*8, 28*8-1 },			/* struct rectangle visible_area */
	gfxdecodeinfo,				/* GfxDecodeInfo */
	2048,					/* total colors */
	2048,					/* color table length */
	system8_vh_convert_color_prom,		/* convert color prom routine */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,					/* vh_init routine */
	system8_vh_start,			/* vh_start routine */
	system8_vh_stop,			/* vh_stop routine */
	system8_vh_screenrefresh,		/* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};

/* driver with reduced visible area for scrolling games */
static struct MachineDriver system8_small_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,	/* My Hero has 2 OSCs 8 & 20 MHz (Cabbe Info) */
			0,		/* memory region */
			readmem,writemem,readport,writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,
			3,		/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,4			/* NMIs are caused by the main CPU */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,					/* single CPU, no need for interleaving */
	system8_init_machine,

	/* video hardware */
	256, 256,				/* screen_width, screen_height */
	{ 0*8+8, 32*8-1-8, 0*8, 28*8-1 },			/* struct rectangle visible_area */
	gfxdecodeinfo,				/* GfxDecodeInfo */
	2048,					/* total colors */
	2048,					/* color table length */
	system8_vh_convert_color_prom,		/* convert color prom routine */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,					/* vh_init routine */
	system8_vh_start,			/* vh_start routine */
	system8_vh_stop,			/* vh_stop routine */
	system8_vh_screenrefresh,		/* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};

static struct MachineDriver pitfall2_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			3650000,			/* 3.65 MHz ? changing it to 4 makes the title disappear */
			0,				/* memory region */
			readmem,writemem,readport,writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			3000000,			/* 3 Mhz ? */
			3,				/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,4			/* NMIs are caused by the main CPU */
		},

	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,					/* single CPU, no need for interleaving */
	system8_init_machine,

	/* video hardware */
	256, 256,				/* screen_width, screen_height */
	{ 0*8, 32*8-1, 0*8, 28*8-1 },		/* struct rectangle visible_area */
	gfxdecodeinfo,				/* GfxDecodeInfo */
	2048,					/* total colors */
	2048,					/* color table length */
	system8_vh_convert_color_prom,	        /* convert color prom routine */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,					/* vh_init routine */
	system8_vh_start,			/* vh_start routine */
	system8_vh_stop,			/* vh_stop routine */
	system8_vh_screenrefresh,		/* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};

static struct MachineDriver choplift_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,			/* 4 MHz ? */
			0,				/* memory region */
			wbml_readmem,choplift_writemem,wbml_readport,choplift_writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,			/* 4 Mhz ? */
			4,				/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,4			/* NMIs are caused by the main CPU */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	        /* frames per second, vblank duration */
	10,					        /* single CPU, no need for interleaving */
	choplift_init_machine,

	/* video hardware */
	256, 256,					/* screen_width, screen_height */
	{ 0*8, 32*8-1, 0*8, 28*8-1 },			/* struct rectangle visible_area */
	choplift_gfxdecodeinfo,			        /* GfxDecodeInfo */
	2048,						/* total colors */
	2048,						/* color table length */
	system8_vh_convert_color_prom,	/* convert color prom routine */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,								/* vh_init routine */
	system8_vh_start,				/* vh_start routine */
	system8_vh_stop,				/* vh_stop routine */
	choplifter_vh_screenrefresh,		        /* vh_update routine */

	/* sound hardware */
	0,0,0,0,
	{
		{
			SOUND_SN76496,
			&sn76496_interface
		}
	}
};

static struct MachineDriver wbml_machine_driver =
{
	/* basic machine hardware */
	{
		{
			CPU_Z80,
			4000000,			/* 4 MHz ? */
			0,				/* memory region */
			wbml_readmem,wbml_writemem,wbml_readport,wbml_writeport,
			interrupt,1
		},
		{
			CPU_Z80 | CPU_AUDIO_CPU,
			4000000,			/* 4 Mhz ? */
			4,				/* memory region */
			sound_readmem,sound_writemem,0,0,
			interrupt,4			/* NMIs are caused by the main CPU */
		},
	},
	60, DEFAULT_60HZ_VBLANK_DURATION,	/* frames per second, vblank duration */
	1,					/* single CPU, no need for interleaving */
	choplift_init_machine,

	/* video hardware */
	256, 256,				/* screen_width, screen_height */
	{ 0*8, 32*8-1, 0*8, 28*8-1 },		/* struct rectangle visible_area */
	choplift_gfxdecodeinfo,			/* GfxDecodeInfo */
	1536, 1536,
	system8_vh_convert_color_prom,	        /* convert color prom routine */

	VIDEO_TYPE_RASTER | VIDEO_MODIFIES_PALETTE,
	0,					/* vh_init routine */
	system8_vh_start,			/* vh_start routine */
	system8_vh_stop,			/* vh_stop routine */
	wbml_vh_screenrefresh,		        /* vh_update routine */

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

ROM_START( starjack_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "5320B", 0x0000, 0x2000, 0x17306628 )
	ROM_LOAD( "5321A", 0x2000, 0x2000, 0xeb13729b )
	ROM_LOAD( "5322A", 0x4000, 0x2000, 0x20a44092 )
	ROM_LOAD( "5323",  0x6000, 0x2000, 0xcbe12a9d )
	ROM_LOAD( "5324",  0x8000, 0x2000, 0xa5b12405 )
	ROM_LOAD( "5325",  0xa000, 0x2000, 0x1110e8e0 )

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5331", 0x0000, 0x2000, 0x4f2c7db0 )
	ROM_LOAD( "5330", 0x2000, 0x2000, 0x8a282108 )
	ROM_LOAD( "5329", 0x4000, 0x2000, 0xa43fd3fd )
	ROM_LOAD( "5328", 0x6000, 0x2000, 0xf6ff65a1 )
	ROM_LOAD( "5327", 0x8000, 0x2000, 0x524b0071 )
	ROM_LOAD( "5326", 0xa000, 0x2000, 0xef63c1fd )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "5318", 0x0000, 0x4000, 0x6086ba1c )
	ROM_LOAD( "5319", 0x4000, 0x4000, 0x2e80bddc )

	ROM_REGION(0x10000) 		/* 64k for sound cpu */
	ROM_LOAD( "5332", 0x0000, 0x2000, 0x6281a187 )
ROM_END

ROM_START( starjacs_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "SJA1IC29", 0x0000, 0x2000, 0x7e425e80 )
	ROM_LOAD( "SJA1IC30", 0x2000, 0x2000, 0xbf736f7d )
	ROM_LOAD( "SJA1IC31", 0x4000, 0x2000, 0x5a73537f )
	ROM_LOAD( "SJA1IC32", 0x6000, 0x2000, 0xf1747034 )
	ROM_LOAD( "SJA1IC33", 0x8000, 0x2000, 0xb55a3b02 )
	ROM_LOAD( "SJA1IC34", 0xa000, 0x2000, 0x81dd84e7 )

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "5331",     0x0000, 0x2000, 0x4f2c7db0 )
	ROM_LOAD( "SJA1IC65", 0x2000, 0x2000, 0x8e282508 )
	ROM_LOAD( "5329",     0x4000, 0x2000, 0xa43fd3fd )
	ROM_LOAD( "SJA1IC64", 0x6000, 0x2000, 0xfaff61a1 )
	ROM_LOAD( "5327",     0x8000, 0x2000, 0x524b0071 )
	ROM_LOAD( "SJA1IC63", 0xa000, 0x2000, 0xf363c5fd )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	/* SJA1IC86 and SJA1IC93 in the original set were bad, so I'm using the ones */
	/* from the Sega version. However I suspect the real ones should be slightly */
	/* different. */
	ROM_LOAD( "5318", 0x0000, 0x4000, 0x6086ba1c )
	ROM_LOAD( "5319", 0x4000, 0x4000, 0x2e80bddc )

	ROM_REGION(0x10000) 		/* 64k for sound cpu */
	ROM_LOAD( "5332", 0x0000, 0x2000, 0x6281a187 )
ROM_END

ROM_START( upndown_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "UPND5679.BIN", 0x0000, 0x2000, 0xf4bca48a )
	ROM_LOAD( "UPND5680.BIN", 0x2000, 0x2000, 0xa24d61c1 )
	ROM_LOAD( "UPND5681.BIN", 0x4000, 0x2000, 0x3aa82e56 )
	ROM_LOAD( "UPND5682.BIN", 0x6000, 0x2000, 0x4ba5c101 )
	ROM_LOAD( "UPND5683.BIN", 0x8000, 0x2000, 0x14e87170 )
	ROM_LOAD( "UPND5684.BIN", 0xA000, 0x2000, 0xdfbc8d48 )

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "UPND5527.BIN", 0x0000, 0x2000, 0x0caa0934 )
	ROM_LOAD( "UPND5526.BIN", 0x2000, 0x2000, 0x377bebb9 )
	ROM_LOAD( "UPND5525.BIN", 0x4000, 0x2000, 0x0bf84b26 )
	ROM_LOAD( "UPND5524.BIN", 0x6000, 0x2000, 0x8aacb4ae )
	ROM_LOAD( "UPND5523.BIN", 0x8000, 0x2000, 0x3971bd0d )
	ROM_LOAD( "UPND5522.BIN", 0xA000, 0x2000, 0xbca7b3a9 )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "UPND5514.BIN", 0x0000, 0x4000, 0x953bcab7 )
	ROM_LOAD( "UPND5515.BIN", 0x4000, 0x4000, 0xb93737a3 )

	ROM_REGION(0x10000) 		/* 64k for sound cpu */
	ROM_LOAD( "UPND5528.BIN", 0x0000, 0x2000, 0x058ca4a2 )
ROM_END

ROM_START( mrviking_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "VEPR5873", 0x0000, 0x2000, 0x09ac75ec )
	ROM_LOAD( "VEPR5874", 0x2000, 0x2000, 0x29dd566b )
	ROM_LOAD( "VEPR5975", 0x4000, 0x2000, 0x3129e8e5 )
	ROM_LOAD( "VEPR5876", 0x6000, 0x2000, 0x1cc75bf1 )
	ROM_LOAD( "VEPR5755", 0x8000, 0x2000, 0x724cbf9c )
	ROM_LOAD( "VEPR5756", 0xa000, 0x2000, 0x1150cffc )

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "VEPR5762", 0x0000, 0x2000, 0xea1613ce )
	ROM_LOAD( "VEPR5761", 0x2000, 0x2000, 0x8fea105a )
	ROM_LOAD( "VEPR5760", 0x4000, 0x2000, 0x1d49c9fb )
	ROM_LOAD( "VEPR5759", 0x6000, 0x2000, 0x2faa3840 )
	ROM_LOAD( "VEPR5758", 0x8000, 0x2000, 0x9c224f40 )
	ROM_LOAD( "VEPR5757", 0xa000, 0x2000, 0x3dc24aa4 )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "VIEPR574", 0x0000, 0x4000, 0xcbfc40b2 )
	ROM_LOAD( "VEPR5750", 0x4000, 0x4000, 0xf392a85c )

	ROM_REGION(0x10000) 		/* 64k for sound cpu */
	ROM_LOAD( "VEPR5763", 0x0000, 0x2000, 0x8e6f1da3 )
ROM_END

ROM_START( flicky_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "EPR5978", 0x0000, 0x4000, 0x54f40c56 )	/* encrypted */
	ROM_LOAD( "EPR5979", 0x4000, 0x4000, 0x2cbebf9a )	/* encrypted */

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "EPR6001", 0x0000, 0x4000, 0x933e3ed6 )
	ROM_LOAD( "EPR6000", 0x4000, 0x4000, 0x9b3e5b02 )
	ROM_LOAD( "EPR5999", 0x8000, 0x4000, 0x91f438f4 )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "EPR5855", 0x0000, 0x4000, 0x55f40962 )
	ROM_LOAD( "EPR5856", 0x4000, 0x4000, 0xae813df9 )

	ROM_REGION(0x10000) 		/* 64k for sound cpu */
	ROM_LOAD( "EPR5869", 0x0000, 0x2000, 0x9e237d9b )
ROM_END

ROM_START( pitfall2_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "EPR6623" , 0x0000, 0x4000, 0xf9cbca17 )
	ROM_LOAD( "EPR6624A", 0x4000, 0x4000, 0xec525d6e )
	ROM_LOAD( "EPR6625" , 0x8000, 0x4000, 0x3eb82172 )

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "EPR6474A", 0x0000, 0x2000, 0x2516b784 )
	ROM_LOAD( "EPR6473" , 0x2000, 0x2000, 0x9b50df0c )
	ROM_LOAD( "EPR6472A", 0x4000, 0x2000, 0x352e5484 )
	ROM_LOAD( "EPR6471A", 0x6000, 0x2000, 0x7649c70f )
	ROM_LOAD( "EPR6470A", 0x8000, 0x2000, 0xa1bfc31d )
	ROM_LOAD( "EPR6469A", 0xa000, 0x2000, 0x680bebef )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "EPR6454A" , 0x0000, 0x4000, 0x460b8fe3 )
	ROM_LOAD( "EPR6455"  , 0x4000, 0x4000, 0x83827770 )

	ROM_REGION(0x10000)		/* 64k for sound cpu */
	ROM_LOAD( "EPR6462"  , 0x0000, 0x2000, 0x0ecd1add )
ROM_END

ROM_START( seganinj_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "ic116.bin", 0x0000, 0x4000, 0x5d7df5a9 )
	ROM_LOAD( "ic109.bin", 0x4000, 0x4000, 0xd54cf260 )
	ROM_LOAD( "7151.96",   0x8000, 0x4000, 0x51e6fc7e )

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "6593.62", 0x0000, 0x2000, 0xc6859747 )
	ROM_LOAD( "6592.61", 0x2000, 0x2000, 0xb3e748af )
	ROM_LOAD( "6591.64", 0x4000, 0x2000, 0xc0e0f1a2 )
	ROM_LOAD( "6590.63", 0x6000, 0x2000, 0x1d5eb106 )
	ROM_LOAD( "6589.66", 0x8000, 0x2000, 0x90532037 )
	ROM_LOAD( "6588.65", 0xA000, 0x2000, 0x1db3f33b )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "6546.117", 0x0000, 0x4000, 0x111f4dd1 )
	ROM_LOAD( "6548.04",  0x4000, 0x4000, 0x75cd29ef )
	ROM_LOAD( "6547.110", 0x8000, 0x4000, 0xe45abeae )
	ROM_LOAD( "6549.05",  0xC000, 0x4000, 0x640faed7 )

	ROM_REGION(0x10000) 		/* 64k for sound cpu */
	ROM_LOAD( "6559.120", 0x0000, 0x2000, 0x1b6c3eb6 )
ROM_END

ROM_START( seganinu_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "7149.116", 0x0000, 0x4000, 0x94ccb6ae )
	ROM_LOAD( "7150.109", 0x4000, 0x4000, 0x5699ffdd )
	ROM_LOAD( "7151.96",  0x8000, 0x4000, 0x51e6fc7e )

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "6593.62", 0x0000, 0x2000, 0xc6859747 )
	ROM_LOAD( "6592.61", 0x2000, 0x2000, 0xb3e748af )
	ROM_LOAD( "6591.64", 0x4000, 0x2000, 0xc0e0f1a2 )
	ROM_LOAD( "6590.63", 0x6000, 0x2000, 0x1d5eb106 )
	ROM_LOAD( "6589.66", 0x8000, 0x2000, 0x90532037 )
	ROM_LOAD( "6588.65", 0xA000, 0x2000, 0x1db3f33b )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "6546.117", 0x0000, 0x4000, 0x111f4dd1 )
	ROM_LOAD( "6548.04",  0x4000, 0x4000, 0x75cd29ef )
	ROM_LOAD( "6547.110", 0x8000, 0x4000, 0xe45abeae )
	ROM_LOAD( "6549.05",  0xC000, 0x4000, 0x640faed7 )

	ROM_REGION(0x10000) 		/* 64k for sound cpu */
	ROM_LOAD( "6559.120", 0x0000, 0x2000, 0x1b6c3eb6 )
ROM_END

ROM_START( nprinces_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "nprinces.001", 0x0000, 0x4000, 0xc5d77061 )	/* encrypted */
	ROM_LOAD( "nprinces.002", 0x4000, 0x4000, 0xdbaf37b9 )	/* encrypted */
	ROM_LOAD( "7151.96",      0x8000, 0x4000, 0x51e6fc7e )

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "nprinces.011", 0x0000, 0x4000, 0x7b934bad )
	ROM_LOAD( "nprinces.010", 0x4000, 0x4000, 0x74b9f50f )
	ROM_LOAD( "nprinces.009", 0x8000, 0x4000, 0xa81465ce )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "6546.117", 0x0000, 0x4000, 0x111f4dd1 )
	ROM_LOAD( "6548.04",  0x4000, 0x4000, 0x75cd29ef )
	ROM_LOAD( "6547.110", 0x8000, 0x4000, 0xe45abeae )
	ROM_LOAD( "6549.05",  0xC000, 0x4000, 0x640faed7 )

	ROM_REGION(0x10000) 		/* 64k for sound cpu */
	ROM_LOAD( "6559.120", 0x0000, 0x2000, 0x1b6c3eb6 )
ROM_END

ROM_START( imsorry_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "EPR6676.116", 0x0000, 0x4000, 0x2fbc048a )	/* encrypted */
	ROM_LOAD( "EPR6677.109", 0x4000, 0x4000, 0xf469b72f )	/* encrypted */
	ROM_LOAD( "EPR6678.96",  0x8000, 0x4000, 0xa0d133c9 )

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "EPR6684.U62", 0x0000, 0x2000, 0x187fed0f )
	ROM_LOAD( "EPR6683.U61", 0x2000, 0x2000, 0xb8bc525c )
	ROM_LOAD( "EPR6682.U64", 0x4000, 0x2000, 0xe2a11725 )
	ROM_LOAD( "EPR6681.U63", 0x6000, 0x2000, 0x30eda313 )
	ROM_LOAD( "EPR6680.U66", 0x8000, 0x2000, 0x86eeddee )
	ROM_LOAD( "EPR6674.U65", 0xA000, 0x2000, 0xe49ee552 )

	ROM_REGION(0x8000)	/* 32k for sprites data */
	ROM_LOAD( "EPR66XX.117", 0x0000, 0x4000, 0x74bbf555 )
	ROM_LOAD( "EPR66XX.U04", 0x4000, 0x4000, 0x0d3cb5da )

	ROM_REGION(0x10000) 		/* 64k for sound cpu */
	ROM_LOAD( "EPR6656.113", 0x0000, 0x2000, 0x50a6ee34 )
ROM_END

ROM_START( teddybb_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "6768.116", 0x0000, 0x4000, 0x3cf801d6 )	/* encrypted */
	ROM_LOAD( "6769.109", 0x4000, 0x4000, 0x81ca459c )	/* encrypted */
	ROM_LOAD( "6770.96",  0x8000, 0x4000, 0xb23d4b9b )

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "6776.62", 0x0000, 0x2000, 0x49ceeb9a )
	ROM_LOAD( "6775.61", 0x2000, 0x2000, 0xa1971e53 )
	ROM_LOAD( "6774.64", 0x4000, 0x2000, 0xa18e5f9c )
	ROM_LOAD( "6773.63", 0x6000, 0x2000, 0xfca42ca6 )
	ROM_LOAD( "6772.66", 0x8000, 0x2000, 0xbe626a10 )
	ROM_LOAD( "6771.65", 0xA000, 0x2000, 0xd8e8b816 )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "6735.117", 0x0000, 0x4000, 0x80439145 )
	ROM_LOAD( "6737.004", 0x4000, 0x4000, 0x1db4d610 )
	ROM_LOAD( "6736.110", 0x8000, 0x4000, 0x8759c703 )
	ROM_LOAD( "6738.005", 0xC000, 0x4000, 0x57a9291d )

	ROM_REGION(0x10000) 		/* 64k for sound cpu */
	ROM_LOAD( "6748.120", 0x0000, 0x2000, 0xe5fa8a54 )
ROM_END

ROM_START( myhero_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "EPR6963B.116", 0x0000, 0x4000, 0x7a64afc6 )
	ROM_LOAD( "EPR6964A.109", 0x4000, 0x4000, 0xffc41534 )
	ROM_LOAD( "EPR6965.96",   0x8000, 0x4000, 0x0dd388ef )

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "EPR6966.U62", 0x0000, 0x2000, 0x6917061f )
	ROM_LOAD( "EPR6961.U61", 0x2000, 0x2000, 0x3b22387e )
	ROM_LOAD( "EPR6960.U64", 0x4000, 0x2000, 0xe2befe90 )
	ROM_LOAD( "EPR6959.U63", 0x6000, 0x2000, 0xce1dde31 )
	ROM_LOAD( "EPR6958.U66", 0x8000, 0x2000, 0x9972d01a )
	ROM_LOAD( "EPR6958.U65", 0xA000, 0x2000, 0x3888973e )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "EPR6921.117", 0x0000, 0x4000, 0x8fbbe309 )
	ROM_LOAD( "EPR6923.U04", 0x4000, 0x4000, 0x475b7a49 )
	ROM_LOAD( "EPR6922.110", 0x8000, 0x4000, 0xb0224d0a )
	ROM_LOAD( "EPR6924.U05", 0xC000, 0x4000, 0x18f73c5d )

	ROM_REGION(0x10000) 		/* 64k for sound cpu */
	ROM_LOAD( "EPR69XX.120", 0x0000, 0x2000, 0x49f0862c )
ROM_END

ROM_START( wbdeluxe_rom )
	ROM_REGION(0x10000)		/* 64k for code */
	ROM_LOAD( "WBD1.BIN", 0x0000, 0x2000, 0xfc881b62 )
	ROM_LOAD( "WBD2.BIN", 0x2000, 0x2000, 0x620b2985 )
	ROM_LOAD( "WBD3.BIN", 0x4000, 0x2000, 0x16949c5e )
	ROM_LOAD( "WBD4.BIN", 0x6000, 0x2000, 0x95cc4dd6 )
	ROM_LOAD( "WBD5.BIN", 0x8000, 0x2000, 0xbb73e2a9 )
	ROM_LOAD( "WBD6.BIN", 0xA000, 0x2000, 0xc57f0e3b )

	ROM_REGION(0xC000) 		/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "WB.008", 0x0000, 0x4000, 0x18e4fd80 )
	ROM_LOAD( "WB.007", 0x4000, 0x4000, 0x140ec290 )
	ROM_LOAD( "WB.006", 0x8000, 0x4000, 0xbd90ec0c )

	ROM_REGION(0x10000)	/* 64k for sprites data */
	ROM_LOAD( "WB.004", 0x0000, 0x4000, 0xa849864b )
	ROM_CONTINUE(       0x8000, 0x4000 )
	ROM_LOAD( "WB.005", 0x4000, 0x4000, 0x7c7d23f3 )
	ROM_CONTINUE(       0xC000, 0x4000 )

	ROM_REGION(0x10000) 		/* 64k for sound cpu */
	ROM_LOAD( "WB.009", 0x0000, 0x2000, 0xb2d5545b )
ROM_END

ROM_START( chplft_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "7124.90", 0x00000, 0x8000, 0xe67e1670 )
	ROM_LOAD( "7125.91", 0x10000, 0x8000, 0x56d3a903 )
	ROM_LOAD( "7126.92", 0x18000, 0x8000, 0x2f4c41fa )

	ROM_REGION(0x18000) 	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "7127.4", 0x00000, 0x8000, 0x684454da )
	ROM_LOAD( "7128.5", 0x08000, 0x8000, 0x60640aac )
	ROM_LOAD( "7129.6", 0x10000, 0x8000, 0x0e274493 )

	ROM_REGION(0x20000)	/* 128k for sprites data */
	ROM_LOAD( "7121.87", 0x00000, 0x8000, 0xa4f7add7 )
	ROM_LOAD( "7120.86", 0x08000, 0x8000, 0xb59f8d71 )
	ROM_LOAD( "7123.89", 0x10000, 0x8000, 0x23911e63 )
	ROM_LOAD( "7122.88", 0x18000, 0x8000, 0x087b75e1 )

	ROM_REGION(0x0300)		/* color proms */
	ROM_LOAD( "pr7119.20", 0x0000, 0x0100, 0xd3a30307 ) /* palette red component */
	ROM_LOAD( "pr7118.14", 0x0100, 0x0100, 0xb8d9080f ) /* palette green component */
	ROM_LOAD( "pr7117.8",  0x0200, 0x0100, 0xfafd090d ) /* palette blue component */

	ROM_REGION(0x10000)   /* 64k for sound cpu */
	ROM_LOAD( "7130.126", 0x0000, 0x8000, 0xf3ed7509 )
ROM_END

ROM_START( chplftb_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "7152.90", 0x00000, 0x8000, 0x59a80b20 )
	ROM_LOAD( "7153.91", 0x10000, 0x8000, 0xb6d3a903 )
	ROM_LOAD( "7154.92", 0x18000, 0x8000, 0x0c4c50fa )

	ROM_REGION(0x18000) 	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "7127.4", 0x00000, 0x8000, 0x684454da )
	ROM_LOAD( "7128.5", 0x08000, 0x8000, 0x60640aac )
	ROM_LOAD( "7129.6", 0x10000, 0x8000, 0x0e274493 )

	ROM_REGION(0x20000)	/* 128k for sprites data */
	ROM_LOAD( "7121.87", 0x00000, 0x8000, 0xa4f7add7 )
	ROM_LOAD( "7120.86", 0x08000, 0x8000, 0xb59f8d71 )
	ROM_LOAD( "7123.89", 0x10000, 0x8000, 0x23911e63 )
	ROM_LOAD( "7122.88", 0x18000, 0x8000, 0x087b75e1 )

	ROM_REGION(0x0300)		/* color proms */
	ROM_LOAD( "pr7119.20", 0x0000, 0x0100, 0xd3a30307 ) /* palette red component */
	ROM_LOAD( "pr7118.14", 0x0100, 0x0100, 0xb8d9080f ) /* palette green component */
	ROM_LOAD( "pr7117.8",  0x0200, 0x0100, 0xfafd090d ) /* palette blue component */

	ROM_REGION(0x10000)   /* 64k for sound cpu */
	ROM_LOAD( "7130.126", 0x0000, 0x8000, 0xf3ed7509 )
ROM_END

ROM_START( chplftbl_rom )
	ROM_REGION(0x20000)	/* 128k for code */
	ROM_LOAD( "7124bl.90", 0x00000, 0x8000, 0xaa09ffb1 )
	ROM_LOAD( "7125.91",   0x10000, 0x8000, 0x56d3a903 )
	ROM_LOAD( "7126.92",   0x18000, 0x8000, 0x2f4c41fa )

	ROM_REGION(0x18000) 	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "7127.4", 0x00000, 0x8000, 0x684454da )
	ROM_LOAD( "7128.5", 0x08000, 0x8000, 0x60640aac )
	ROM_LOAD( "7129.6", 0x10000, 0x8000, 0x0e274493 )

	ROM_REGION(0x20000)	/* 128k for sprites data */
	ROM_LOAD( "7121.87", 0x00000, 0x8000, 0xa4f7add7 )
	ROM_LOAD( "7120.86", 0x08000, 0x8000, 0xb59f8d71 )
	ROM_LOAD( "7123.89", 0x10000, 0x8000, 0x23911e63 )
	ROM_LOAD( "7122.88", 0x18000, 0x8000, 0x087b75e1 )

	ROM_REGION(0x0300)		/* color proms */
	ROM_LOAD( "pr7119.20", 0x0000, 0x0100, 0xd3a30307 ) /* palette red component */
	ROM_LOAD( "pr7118.14", 0x0100, 0x0100, 0xb8d9080f ) /* palette green component */
	ROM_LOAD( "pr7117.8",  0x0200, 0x0100, 0xfafd090d ) /* palette blue component */

	ROM_REGION(0x10000)   /* 64k for sound cpu */
	ROM_LOAD( "7130.126", 0x0000, 0x8000, 0xf3ed7509 )
ROM_END

ROM_START( wbml_rom )
	ROM_REGION(0x40000)		/* 256k for code */
	ROM_LOAD( "WBML.01", 0x20000, 0x8000, 0xa5329b82 )	/* Unencrypted opcodes */
	ROM_CONTINUE(        0x00000, 0x8000 )              /* Now load the operands in RAM */
	ROM_LOAD( "WBML.02", 0x30000, 0x8000, 0xcdafb89f )	/* Unencrypted opcodes */
	ROM_CONTINUE(        0x10000, 0x8000 )
	ROM_LOAD( "WBML.03", 0x38000, 0x8000, 0x31cd6733 )	/* Unencrypted opcodes */
	ROM_CONTINUE(        0x18000, 0x8000 )

	ROM_REGION(0x18000) 	/* temporary space for graphics (disposed after conversion) */
	ROM_LOAD( "WBML.08", 0x00000, 0x8000, 0xef89ac27 )
	ROM_LOAD( "WBML.09", 0x08000, 0x8000, 0x60a6011e )
	ROM_LOAD( "WBML.10", 0x10000, 0x8000, 0x563078ba )

	ROM_REGION(0x20000)	/* 128k for sprites data */
	ROM_LOAD( "WBML.05", 0x00000, 0x8000, 0x7d7dd491 )
	ROM_LOAD( "WBML.04", 0x08000, 0x8000, 0x5ba6090a )
	ROM_LOAD( "WBML.07", 0x10000, 0x8000, 0xf3a6277a )
	ROM_LOAD( "WBML.06", 0x18000, 0x8000, 0x387db381 )

	ROM_REGION(0x0300)		/* color proms */
	ROM_LOAD( "prom3.bin", 0x0000, 0x0100, 0xbbbe0406 )
	ROM_LOAD( "prom2.bin", 0x0100, 0x0100, 0x6261050f )
	ROM_LOAD( "prom1.bin", 0x0200, 0x0100, 0xb0bd0401 )

	ROM_REGION(0x10000)		/* 64k for sound cpu */
	ROM_LOAD( "WBML.11", 0x0000, 0x8000, 0x8057f681 )
ROM_END



static void wbml_decode(void)
{
	int A;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];

	for (A = 0x0000;A < 0x8000;A++)
	{
		ROM[A] = RAM[A+0x20000];
	}
}



static int upndown_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xc93f],"\x01\x00\x00",3) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xc93f],(6*10)+3);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void upndown_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xc93f],(6*10)+3);
		osd_fclose(f);
	}
}


static int pitfall2_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xD302],"\x02\x00\x59\x41",4) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xD300],56);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void pitfall2_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xD300],56);
		osd_fclose(f);
	}
}


static int seganinj_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xEF2E],"\x54\x41\x43\x00",4) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xEF01],48);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void seganinj_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xEF01],48);
		osd_fclose(f);
	}
}


static int myhero_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xd339],"\x59\x2E\x49\x00",4) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xD300],61);
			osd_fclose(f);

			/* copy the high score to the work RAM as well */
			RAM[0xC017] = RAM[0xD300];
			RAM[0xC018] = RAM[0xD301];
		}
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void myhero_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xD300],61);
		osd_fclose(f);
	}
}


static int wbdeluxe_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xC100],"\x20\x11\x20\x20",4) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			int i;


			osd_fread(f,&RAM[0xC100],320);
			osd_fclose(f);

			/* copy the high score to the screen as well */
			for (i = 0; i < 6; i++) /* 6 digits are stored, one per byte */
			{
				if (RAM[0xC102 + i] == 0x20)  /* spaces */
					RAM[0xE858 + i * 2] = 0x01;
				else                          /* digits */
					RAM[0xE858 + i * 2] = RAM[0xC102 + i] - 0x30 + 0x10;
			}
		}
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void wbdeluxe_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[Machine->drv->cpu[0].memory_region];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xC100],320);
		osd_fclose(f);
	}
}


static int choplift_hiload(void)
{
	void *f;
	unsigned char *choplifter_ram = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&choplifter_ram[0xEF00],"\x00\x00\x05\x00",4) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&choplifter_ram[0xEF00],49);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;	/* we can't load the hi scores yet */
}

static void choplift_hisave(void)
{
	void *f;
	unsigned char *choplifter_ram = Machine->memory_region[0];


	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&choplifter_ram[0xEF00],49);
		osd_fclose(f);
	}
}


static int wbml_hiload(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];


	/* check if the hi score table has already been initialized */
	if (memcmp(&RAM[0xc17b],"\x00\x00\x03",3) == 0)
	{
		if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,0)) != 0)
		{
			osd_fread(f,&RAM[0xc179],8);
			osd_fclose(f);
		}
		return 1;
	}
	else return 0;  /* we can't load the hi scores yet */
}

static void wbml_hisave(void)
{
	void *f;
	unsigned char *RAM = Machine->memory_region[0];

	if ((f = osd_fopen(Machine->gamedrv->name,0,OSD_FILETYPE_HIGHSCORE,1)) != 0)
	{
		osd_fwrite(f,&RAM[0xc179],8);
		osd_fclose(f);
	}
}



struct GameDriver starjack_driver =
{
	__FILE__,
	0,
	"starjack",
	"Star Jacker (Sega)",
	"1983",
	"Sega",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&system8_small_machine_driver,

	starjack_rom,
	0, 0,					/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	starjack_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver starjacs_driver =
{
	__FILE__,
	&starjack_driver,
	"starjacs",
	"Star Jacker (Stern)",
	"1983",
	"Stern",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&system8_small_machine_driver,

	starjacs_rom,
	0, 0,					/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	starjacs_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver upndown_driver =
{
	__FILE__,
	0,
	"upndown",
	"Up'n Down",
	"1983",
	"Sega",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&system8_machine_driver,

	upndown_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	upndown_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	upndown_hiload, upndown_hisave
};

struct GameDriver mrviking_driver =
{
	__FILE__,
	0,
	"mrviking",
	"Mister Viking",
	"1984",
	"Sega",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&system8_small_machine_driver,

	mrviking_rom,
	0, mrviking_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	mrviking_input_ports,

	0, 0, 0,
	ORIENTATION_ROTATE_270,
	0, 0
};

struct GameDriver flicky_driver =
{
	__FILE__,
	0,
	"flicky",
	"Flicky",
	"1984",
	"Sega",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&system8_machine_driver,

	flicky_rom,
	0, flicky_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	flicky_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver pitfall2_driver =
{
	__FILE__,
	0,
	"pitfall2",
	"Pitfall II",
	"1985",
	"Sega",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&pitfall2_machine_driver,

	pitfall2_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	pitfall2_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	pitfall2_hiload, pitfall2_hisave
};

struct GameDriver seganinj_driver =
{
	__FILE__,
	0,
	"seganinj",
	"Sega Ninja",
	"1985",
	"Sega",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&system8_machine_driver,

	seganinj_rom,
	0, seganinj_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	seganinj_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	seganinj_hiload, seganinj_hisave
};

struct GameDriver seganinu_driver =
{
	__FILE__,
	&seganinj_driver,
	"seganinu",
	"Sega Ninja (not encrypted)",
	"1985",
	"Sega",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&system8_machine_driver,

	seganinu_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	seganinj_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	seganinj_hiload, seganinj_hisave
};

struct GameDriver nprinces_driver =
{
	__FILE__,
	&seganinj_driver,
	"nprinces",
	"Ninja Princess",
	"1985",
	"Sega",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&system8_machine_driver,

	nprinces_rom,
	0, flicky_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	seganinj_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	seganinj_hiload, seganinj_hisave
};

struct GameDriver imsorry_driver =
{
	__FILE__,
	0,
	"imsorry",
	"I'm Sorry",
	"1985",
	"Coreland / Sega",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&system8_machine_driver,

	imsorry_rom,
	0, imsorry_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	imsorry_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver teddybb_driver =
{
	__FILE__,
	0,
	"teddybb",
	"TeddyBoy Blues",
	"1985",
	"Sega",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&system8_machine_driver,

	teddybb_rom,
	0, teddybb_decode,		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	teddybb_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	0, 0
};

struct GameDriver myhero_driver =
{
	__FILE__,
	0,
	"myhero",
	"My Hero",
	"1985",
	"Sega",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&system8_machine_driver,

	myhero_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	myhero_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	myhero_hiload, myhero_hisave
};

struct GameDriver wbdeluxe_driver =
{
	__FILE__,
	0,
	"wbdeluxe",
	"Wonder Boy Deluxe",
	"1986",
	"Sega (Escape license)",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&system8_machine_driver,

	wbdeluxe_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	wbdeluxe_input_ports,

	0, 0, 0,
	ORIENTATION_DEFAULT,
	wbdeluxe_hiload, wbdeluxe_hisave
};

struct GameDriver chplft_driver =
{
	__FILE__,
	0,
	"chplft",
	"Choplifter",
	"1985",
	"Sega",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	GAME_NOT_WORKING,
	&choplift_machine_driver,

	chplft_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	choplift_input_ports,

	PROM_MEMORY_REGION(3),0,0,
	ORIENTATION_DEFAULT,
	choplift_hiload, choplift_hisave
};

struct GameDriver chplftb_driver =
{
	__FILE__,
	&chplft_driver,
	"chplftb",
	"Choplifter (alternate)",
	"1985",
	"Sega",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&choplift_machine_driver,

	chplftb_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	choplift_input_ports,

	PROM_MEMORY_REGION(3),0,0,
	ORIENTATION_DEFAULT,
	choplift_hiload, choplift_hisave
};

struct GameDriver chplftbl_driver =
{
	__FILE__,
	&chplft_driver,
	"chplftbl",
	"Choplifter (bootleg)",
	"1985",
	"bootleg",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&choplift_machine_driver,

	chplftbl_rom,
	0, 0,   				/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	choplift_input_ports,

	PROM_MEMORY_REGION(3),0,0,
	ORIENTATION_DEFAULT,
	choplift_hiload, choplift_hisave
};

struct GameDriver wbml_driver =
{
	__FILE__,
	0,
	"wbml",
	"Wonder Boy in Monster Land",
	"1987",
	"bootleg",
	"Jarek Parchanski\nNicola Salmoria\nMirko Buffoni\nRoberto Ventura (hardware info)",
	0,
	&wbml_machine_driver,

	wbml_rom,
	0, wbml_decode,   		/* ROM decode and opcode decode functions */
	0,      				/* Sample names */
	0,

	wbml_input_ports,

	PROM_MEMORY_REGION(3), 0, 0,
	ORIENTATION_DEFAULT,
	wbml_hiload, wbml_hisave
};
