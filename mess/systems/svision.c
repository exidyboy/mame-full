/******************************************************************************
 watara supervision handheld

 PeT mess@utanet.at in december 2000
******************************************************************************/

#include <assert.h>
#include "driver.h"
#include "image.h"
#include "vidhrdw/generic.h"
#include "cpu/m6502/m6502.h"

#include "includes/svision.h"
#include "devices/cartslot.h"

/*
supervision
watara

cartridge code is m65c02 or something more (65ce02?)



4 mhz quartz

right dil28 ram? 8kb?
left dil28 ???

integrated speaker
stereo phone jack
40 pin connector for cartridges
com port (9 pol dsub) pc at rs232?
looked at
5 4 3 2 1
 9 8 7 6
2 black -->vlsi
3 brown -->vlsi
4 yellow -->vlsi
5 red vlsi
7 violett
9 white

port for 6V power supply

on/off switch
volume control analog
contrast control analog




cartridge connector (look at the cartridge)
 /oe or /ce	1  40 +5v (picture side)
		a0  2  39 nc
		a1  3  38 nc
		a2  4  37 nc
		a3  5  36 nc
		a4  6  35 nc in crystball
		a5  7  34 d0
		a6  8  33 d1
		a7  9  32 d2
		a8  10 31 d3
		a9  11 30 d4
		a10 12 29 d5
		a11 13 28 d6
		a12 14 27 d7
        a13 15 26 nc
        a14 16 25 nc
        a15 17 24 nc
        a16?18 23 nc
        a17?19 22 gnd connected with 21 in crystalball
        a18?20 21 (shorter pin in crystalball)

adapter for dumping as 27c4001

cryst ball:
a16,a17,a18 not connected

delta hero:
a16,a17,a18 not connected


ordering of pins in the cartridge!
21,22 connected
idea: it is a 27512, and pin are in this ordering

+5V 40
a15 17
a12 14
a7   9
a6   8
a5   7
a4   6
a3   5
a2   4
a1   3
a0   2
d0  34
d1  33
d2  32
gnd 21!
d3  31
d4  30
d5  29
d6  28
d7  27
ce  21 (gnd)
a10 12
oe  1
a11 13
a9  11
a8  10
a13 15
a14 16
*/

static UINT8 *svision_reg;
/*
  0x2000 0xa0 something to do with video dma?
  0x2001 0xa0 something to do with video dma?
  0x2010,11,12 audio channel
   offset 0,1 frequency; offset 1 always zero?
   offset 2:
    0, 0x60-0x6f
    bit 0..3: volume??
    bit 5: on left??
    bit 6: on right??
  0x2014,15,16 audio channel
  0x2020 buttons and pad
  0x2022 0x0f ?
  0x2023 timer?
   next interrupt at 256*value?
   writing sets timer and clear interrupt request?
   fast irq in crystball needed for timing
   slower irq in deltahero with music?
  0x2026 bank switching
  0x2027
   bit 0: 0x2023 timer interrupt occured

  0x2041-0x2053
  0x3041-
 */

struct
{
    mame_timer *timer1;
    int timer1_shot;
} svision;

static void svision_timer(int param)
{
    svision.timer1_shot = TRUE;
    cpu_set_irq_line(0, M65C02_IRQ_LINE, ASSERT_LINE);
}

static void svision_update_banks(void)
{
	UINT8 *cart_rom = memory_region(REGION_USER1);
	cpu_setbank(1, cart_rom + ((svision_reg[0x26] & 0x60) << 9));
	cpu_setbank(2, cart_rom + 0xC000);
}

static  READ8_HANDLER(svision_r)
{
	int data = svision_reg[offset];
	switch (offset) {
	case 0x20:
		data = readinputport(0);
		break;

	case 0x27:
		if (svision.timer1_shot)
			data |= 1; //crystball irq routine
		break;

	case 0x24:
	case 0x25://deltahero irq routine read
		break;

	default:
		logerror("%.6f svision read %04x %02x\n",timer_get_time(),offset,data);
		break;
	}

	return data;
}

static WRITE8_HANDLER(svision_w)
{
	svision_reg[offset] = data;
	switch (offset) {
	case 0x26: /* bits 5,6 memory management for a000? */
		svision_update_banks();
		break;

	case 0x23:	/* delta hero irq routine write */
		cpu_set_irq_line(0, M65C02_IRQ_LINE, CLEAR_LINE);
		svision.timer1_shot=FALSE;
		timer_reset(svision.timer1, TIME_IN_CYCLES(data*256, 0));
		break;

	case 0x10:
	case 0x11:
	case 0x12:
		svision_soundport_w(svision_channel+0, offset&3, data);
		break;

	case 0x14:
	case 0x15:
	case 0x16:
		svision_soundport_w(svision_channel+1, offset&3, data);
		break;

	default:
		logerror("%.6f svision write %04x %02x\n",timer_get_time(),offset,data);
		break;
	}
}

static ADDRESS_MAP_START( readmem , ADDRESS_SPACE_PROGRAM, 8)
    AM_RANGE( 0x0000, 0x1fff) AM_READ( MRA8_RAM )
    AM_RANGE( 0x2000, 0x3fff) AM_READ( svision_r )
    AM_RANGE( 0x4000, 0x5fff) AM_READ( MRA8_RAM ) //?
	AM_RANGE( 0x6000, 0x7fff) AM_READ( MRA8_ROM )
	AM_RANGE( 0x8000, 0xbfff) AM_READ( MRA8_BANK1 )
	AM_RANGE( 0xc000, 0xffff) AM_READ( MRA8_BANK2 )
ADDRESS_MAP_END

static ADDRESS_MAP_START( writemem , ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE( 0x0000, 0x1fff) AM_WRITE( MWA8_RAM )
    AM_RANGE( 0x2000, 0x3fff) AM_WRITE( svision_w) AM_BASE( &svision_reg )
	AM_RANGE( 0x4000, 0x5fff) AM_WRITE( MWA8_RAM )
	AM_RANGE( 0x6000, 0xffff) AM_WRITE( MWA8_ROM )
ADDRESS_MAP_END

INPUT_PORTS_START( svision )
	PORT_START
    PORT_BIT(		0x01, IP_ACTIVE_LOW, IPT_JOYSTICK_RIGHT)
    PORT_BIT(		0x02, IP_ACTIVE_LOW, IPT_JOYSTICK_LEFT )
    PORT_BIT(		0x04, IP_ACTIVE_LOW, IPT_JOYSTICK_DOWN )
    PORT_BIT(		0x08, IP_ACTIVE_LOW, IPT_JOYSTICK_UP   )
	PORT_BIT_NAME(	0x10, IP_ACTIVE_LOW, IPT_BUTTON2,	"B" )
	PORT_BIT_NAME(	0x20, IP_ACTIVE_LOW, IPT_BUTTON1,	"A" )
	PORT_BIT_NAME(	0x40, IP_ACTIVE_LOW, IPT_SELECT,	"Select" )
	PORT_BIT_NAME(	0x80, IP_ACTIVE_LOW, IPT_START,		"Start/Pause" )
INPUT_PORTS_END

/* most games contain their graphics in roms, and have hardware to
   draw complete rectangular objects */

/* palette in red, green, blue triples */
static unsigned char svision_palette[] =
{
	/* these are necessary to appease the MAME core */
	0x00, 0x00, 0x00,
	0xff, 0xff, 0xff,
#if 0
    /* greens grabbed from a scan of a handheld
     * in its best adjustment for contrast
	 */
	53, 73, 42,
	42, 64, 47,
	22, 42, 51,
	22, 25, 32
#else
	/* grabbed from chris covell's black white pics */
	0xe0, 0xe0, 0xe0,
	0xb9, 0xb9, 0xb9,
	0x54, 0x54, 0x54,
	0x12, 0x12, 0x12
#endif
};

static PALETTE_INIT( svision )
{
	palette_set_colors(0, svision_palette, sizeof(svision_palette) / 3);
}

static VIDEO_UPDATE( svision )
{
	int x, y;
	UINT8 *vram = memory_region(REGION_CPU1) + 0x4000 + (svision_reg[2] / 4);
	UINT8 *vram_line;
	UINT16 *line;
	UINT8 b;

	for (y = 0; y < 160; y++)
	{
		line = (UINT16 *) bitmap->line[y];
		vram_line = &vram[y * 0x30];

		for (x = 0; x < 160; x += 4)
		{
			b = *(vram_line++);
			line[3] = ((b >> 6) & 0x03) + 2;
			line[2] = ((b >> 4) & 0x03) + 2;
			line[1] = ((b >> 2) & 0x03) + 2;
			line[0] = ((b >> 0) & 0x03) + 2;
			line += 4;
		}
	}
}

static INTERRUPT_GEN( svision_frame_int )
{
	cpu_set_nmi_line(0, PULSE_LINE);
}

static DRIVER_INIT( svision )
{
	svision.timer1 = timer_alloc(svision_timer);
}

static MACHINE_INIT( svision )
{
    svision.timer1_shot = FALSE;
	svision_update_banks();
}

struct CustomSound_interface svision_sound_interface =
{
	svision_custom_start,
	NULL,
	svision_custom_update
};


static MACHINE_DRIVER_START( svision )
	/* basic machine hardware */
	MDRV_CPU_ADD(M65C02, 4000000)        /* ? stz used! speed? */
	MDRV_CPU_PROGRAM_MAP(readmem, writemem)
	MDRV_CPU_VBLANK_INT(svision_frame_int, 1)
	MDRV_FRAMES_PER_SECOND(60)
	MDRV_VBLANK_DURATION(DEFAULT_REAL_60HZ_VBLANK_DURATION)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( svision )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)	/* lcd */
	MDRV_SCREEN_SIZE(160, 160)
	MDRV_VISIBLE_AREA(0, 160-1, 0, 160-1)
	MDRV_PALETTE_LENGTH(sizeof(svision_palette) / (sizeof(svision_palette[0]) * 3))
	MDRV_PALETTE_INIT( svision )

	MDRV_VIDEO_UPDATE( svision )

	/* sound hardware */
	MDRV_SOUND_ATTRIBUTES(SOUND_SUPPORTS_STEREO)
	MDRV_SOUND_ADD(CUSTOM, svision_sound_interface)
MACHINE_DRIVER_END

ROM_START(svision)
	ROM_REGION( 0x8000, REGION_CPU1, 0)
	ROM_REGION(0x10000, REGION_USER1, 0)
ROM_END

/* deltahero
 c000
  dd6a clear 0x2000 at ($57/58) (0x4000)
  deb6 clear hardware regs
   e35d clear hardware reg
   e361 clear hardware reg
  e3a4
 c200

 nmi c053 ?
 irq c109
      e3f7
      def4
 routines:
 dd6a clear 0x2000 at ($57/58) (0x4000)
 */

static DEVICE_LOAD( svision_cart )
{
	return cartslot_load_generic(file, REGION_USER1, 0, 1, 0x10000, CARTLOAD_MUSTBEPOWEROFTWO);
}

SYSTEM_CONFIG_START(svision)
	CONFIG_DEVICE_CARTSLOT_REQ(1, "bin\0ws\0sv\0", NULL, NULL, device_load_svision_cart, NULL, NULL, NULL)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*    YEAR  NAME		PARENT	COMPAT	MACHINE		INPUT		INIT		CONFIG		COMPANY		FULLNAME */
CONSX(1992,	svision,	0,		0,		svision,	svision,	svision,	svision,	"Watara",	"Super Vision", GAME_IMPERFECT_SOUND)
// marketed under a ton of firms and names

