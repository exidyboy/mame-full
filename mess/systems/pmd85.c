/*******************************************************************************

PMD-85 driver by Krzysztof Strzecha

Based on PMD-85 hardware information written by Roman Dolejsi.

What's new:
-----------
06.07.2002	Preliminary driver.

Notes on emulation status and to do list:
-----------------------------------------

*******************************************************************************/

#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "vidhrdw/generic.h"
#include "includes/pmd85.h"

/* I/O ports */

PORT_READ_START( pmd85_readport )
	{ 0x00, 0xff, pmd85_io_r },
PORT_END

PORT_WRITE_START( pmd85_writeport )
	{ 0x00, 0xff, pmd85_io_w },
PORT_END

/* memory w/r functions */

MEMORY_READ_START( pmd85_readmem )
	{0x0000, 0x0fff, MRA_BANK1},
	{0x1000, 0x7fff, MRA_BANK2},
	{0x8000, 0x8fff, MRA_BANK3},
	{0xa000, 0xafff, MRA_BANK4},
	{0xc000, 0xffff, MRA_BANK5},
MEMORY_END

MEMORY_WRITE_START( pmd85_writemem )
	{0x0000, 0x0fff, MWA_BANK1},
	{0x1000, 0x7fff, MWA_BANK2},
	{0x8000, 0x8fff, MWA_BANK3},
	{0xa000, 0xafff, MWA_BANK4},
	{0xc000, 0xffff, MWA_BANK5},
MEMORY_END


/* keyboard input */
INPUT_PORTS_START (pmd85)
	PORT_START /* Row 0x00 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K0",	KEYCODE_ESC,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "1",	KEYCODE_1,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q",	KEYCODE_Q,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "A",	KEYCODE_A,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Space",	KEYCODE_SPACE,		IP_JOY_NONE )
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START /* Row 0x01 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K1",	KEYCODE_F1,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2",	KEYCODE_2,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "W",	KEYCODE_W,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "S",	KEYCODE_S,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y",	KEYCODE_Z,		IP_JOY_NONE )
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START /* Row 0x02 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K2",	KEYCODE_F2,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "3",	KEYCODE_3,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E",	KEYCODE_E,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "D",	KEYCODE_D,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "X",	KEYCODE_X,		IP_JOY_NONE )
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START /* Row 0x03 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K3",	KEYCODE_F3,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "4",	KEYCODE_4,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "R",	KEYCODE_R,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F",	KEYCODE_F,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "C",	KEYCODE_C,		IP_JOY_NONE )
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START /* Row 0x04 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K4",	KEYCODE_F4,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "5",	KEYCODE_5,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "T",	KEYCODE_T,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "G",	KEYCODE_G,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "V",	KEYCODE_V,		IP_JOY_NONE )
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START /* Row 0x05 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K5",	KEYCODE_F5,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "6",	KEYCODE_6,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z",	KEYCODE_Y,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "H",	KEYCODE_H,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "B",	KEYCODE_B,		IP_JOY_NONE )
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START /* Row 0x06 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K6",	KEYCODE_F6,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "7",	KEYCODE_7,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "U",	KEYCODE_U,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "J",	KEYCODE_J,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "N",	KEYCODE_N,		IP_JOY_NONE )
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START /* Row 0x07 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K7",	KEYCODE_F7,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "8",	KEYCODE_8,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I",	KEYCODE_I,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "K",	KEYCODE_K,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "M",	KEYCODE_M,		IP_JOY_NONE )
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START /* Row 0x08 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K8",	KEYCODE_F8,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9",	KEYCODE_9,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "O",	KEYCODE_O,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "L",	KEYCODE_L,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ",",	KEYCODE_COMMA,		IP_JOY_NONE )
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START /* Row 0x09 */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K9",	KEYCODE_F9,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "0",	KEYCODE_0,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "P",	KEYCODE_P,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ";",	KEYCODE_COLON,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, ".",	KEYCODE_STOP,		IP_JOY_NONE )
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START /* Row 0x0a */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K10",	KEYCODE_F10,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "_",	KEYCODE_MINUS,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "@",	KEYCODE_OPENBRACE,	IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, ":",	KEYCODE_QUOTE,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "/",	KEYCODE_SLASH,		IP_JOY_NONE )
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START /* Row 0x0b */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "K11",	KEYCODE_F11,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Blank",	KEYCODE_EQUALS,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "\\",	KEYCODE_CLOSEBRACE,	IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "[",	KEYCODE_BACKSLASH,	IP_JOY_NONE )
		PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START /* Row 0x0c */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "WRK",	KEYCODE_INSERT,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "INS",	KEYCODE_DEL,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "<-",	KEYCODE_LEFT,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "|<-",	KEYCODE_LALT,		IP_JOY_NONE )
		PORT_BIT (0x10, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START /* Row 0x0d */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "C-D",	KEYCODE_HOME,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "DEL",	KEYCODE_END,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "^",	KEYCODE_UP,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "END",	KEYCODE_DOWN,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "EOL1",	KEYCODE_ENTER,		IP_JOY_NONE )
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START /* Row 0x0e */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "CLR",	KEYCODE_PGUP,		IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "RCL",	KEYCODE_PGDN,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "->",	KEYCODE_RIGHT,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "->|",	KEYCODE_RALT,		IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "EOL2",	KEYCODE_TAB,		IP_JOY_NONE )
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
	PORT_START /* Row 0x0f */
		PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "RST",	KEYCODE_BACKSPACE,	IP_JOY_NONE )
		PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Shift",	KEYCODE_LSHIFT,		IP_JOY_NONE )
		PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "Shift",	KEYCODE_RSHIFT,		IP_JOY_NONE )
		PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "Stop",	KEYCODE_LCONTROL,	IP_JOY_NONE )
		PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Stop",	KEYCODE_RCONTROL,	IP_JOY_NONE )
		PORT_BIT (0x20, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x40, IP_ACTIVE_LOW, IPT_UNUSED)
		PORT_BIT (0x80, IP_ACTIVE_LOW, IPT_UNUSED)
INPUT_PORTS_END

/* machine definition */
static MACHINE_DRIVER_START( pmd85 )
	/* basic machine hardware */
	MDRV_CPU_ADD(8080, 2000000)
	MDRV_CPU_MEMORY(pmd85_readmem, pmd85_writemem)
	MDRV_CPU_PORTS(pmd85_readport, pmd85_writeport)
	MDRV_FRAMES_PER_SECOND(50)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( pmd85 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(288, 256)
	MDRV_VISIBLE_AREA(0, 288-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(sizeof (pmd85_palette) / 3)
	MDRV_COLORTABLE_LENGTH(sizeof (pmd85_colortable))
	MDRV_PALETTE_INIT( pmd85 )

	MDRV_VIDEO_START( pmd85 )
	MDRV_VIDEO_UPDATE( pmd85 )
MACHINE_DRIVER_END

static const struct IODevice io_pmd85[] = {
	{ IO_END }
};

ROM_START(pmd85)
	ROM_REGION(0x14000,REGION_CPU1,0)
	ROM_LOAD("pmd85-1.bin", 0x10000, 0x1000, 0x9bc5e6ec)
ROM_END

SYSTEM_CONFIG_START(pmd85)
	CONFIG_RAM_DEFAULT(64 * 1024)
SYSTEM_CONFIG_END


/*    YEAR  NAME   PARENT MACHINE INPUT  INIT CONFIG COMPANY  FULLNAME */
COMP( 1989, pmd85, 0,     pmd85,  pmd85, 0,   pmd85, "Tesla", "PMD-85" )
