/***************************************************************************
	zx.c

    system driver
	Juergen Buchmueller <pullmoll@t-online.de>, Dec 1999

	TODO:
	Check the CPU clock, scanlines, cycles per scanlines value ect.
	It seems I'm very close to the real video timing now, but it
	still isn't perfect (see also vidhrdw/zx.c)

****************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"
#include "cpu/z80/z80.h"
#include "includes/zx.h"

#define VERBOSE 0

#if VERBOSE
#define LOG(x)	if( errorlog ) fprintf x
#else
#define LOG(x)						   /* x */
#endif

MEMORY_READ_START( readmem_zx80 )
	{0x0000, 0x0fff, MRA_ROM},
	{0x4000, 0x43ff, MRA_RAM},
	{0x8000, 0xffff, MRA_NOP},
MEMORY_END

MEMORY_WRITE_START( writemem_zx80 )
	{0x0000, 0x0fff, MWA_ROM},
	{0x4000, 0x43ff, MWA_RAM},
	{0x8000, 0xffff, MWA_NOP},
MEMORY_END


MEMORY_READ_START( readmem_zx81 )
	{0x0000, 0x1fff, MRA_ROM},
	{0x4000, 0x43ff, MRA_RAM},
	{0x8000, 0xffff, MRA_NOP},
MEMORY_END

MEMORY_WRITE_START( writemem_zx81 )
	{0x0000, 0x3fff, MWA_ROM},
	{0x4000, 0x43ff, MWA_RAM},
	{0x8000, 0xffff, MWA_NOP},
MEMORY_END

MEMORY_READ_START( readmem_pc8300 )
	{0x0000, 0x1fff, MRA_ROM},
	{0x4000, 0x7fff, MRA_RAM},		   /* PC8300 comes with 16K RAM */
	{0x8000, 0xffff, MRA_NOP},
MEMORY_END

MEMORY_WRITE_START( writemem_pc8300 )
	{0x0000, 0x3fff, MWA_ROM},
	{0x4000, 0x7fff, MWA_RAM},		   /* PC8300 comes with 16K RAM */
	{0x8000, 0xffff, MWA_NOP},
MEMORY_END

static PORT_READ_START (readport)
	{0x0000, 0xffff, zx_io_r},
PORT_END

static PORT_WRITE_START (writeport)
	{0x0000, 0xffff, zx_io_w},
PORT_END

MEMORY_READ_START( readmem_pow3000 )
	{0x0000, 0x1fff, MRA_ROM},
	{0x4000, 0x7fff, MRA_RAM},		   /* Power 3000 comes with 16K RAM */
	{0x8000, 0xffff, MRA_NOP},
MEMORY_END

MEMORY_WRITE_START( writemem_pow3000 )
	{0x0000, 0x3fff, MWA_ROM},
	{0x4000, 0x7fff, MWA_RAM},		   /* Power 3000 comes with 16K RAM */
	{0x8000, 0xffff, MWA_NOP},
MEMORY_END

static PORT_READ_START (readport_pow3000)
	{0x0000, 0xffff, pow3000_io_r},
PORT_END

static PORT_WRITE_START (writeport_pow3000)
	{0x0000, 0xffff, zx_io_w},
PORT_END

INPUT_PORTS_START(zx80)
PORT_START							   /* IN0 */
PORT_DIPNAME(0x80, 0x00, "16K RAM module")
PORT_DIPSETTING(0x00, DEF_STR( No ) )
PORT_DIPSETTING(0x80, DEF_STR( Yes ) )
PORT_BIT(0x7e, 0x0f, IPT_UNUSED)
PORT_BITX(0x01, 0x00, IPT_KEYBOARD | IPF_RESETCPU, "Reset", KEYCODE_F3, IP_JOY_NONE)

PORT_START							   /* IN1 KEY ROW 0 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z  ", KEYCODE_Z, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "X  ", KEYCODE_X, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C  ", KEYCODE_C, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "V  ", KEYCODE_V, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN2 KEY ROW 1 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "A  ", KEYCODE_A, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "S  ", KEYCODE_S, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "D  ", KEYCODE_D, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F  ", KEYCODE_F, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G  ", KEYCODE_G, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN3 KEY ROW 2 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q  ", KEYCODE_Q, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W  ", KEYCODE_W, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E  ", KEYCODE_E, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R  ", KEYCODE_R, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T  ", KEYCODE_T, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN4 KEY ROW 3 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1  EDIT", KEYCODE_1, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2  AND", KEYCODE_2, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "3  THEN", KEYCODE_3, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4  TO", KEYCODE_4, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5  LEFT", KEYCODE_5, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN5 KEY ROW 4 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0  RUBOUT", KEYCODE_0, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9  GRAPHICS", KEYCODE_9, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8  RIGHT", KEYCODE_8, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7  UP", KEYCODE_7, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6  DOWN", KEYCODE_6, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN6 KEY ROW 5 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P  PRINT", KEYCODE_P, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O  POKE", KEYCODE_O, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I  INPUT", KEYCODE_I, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "U  IF", KEYCODE_U, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y  RETURN", KEYCODE_Y, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN7 KEY ROW 6 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER", KEYCODE_ENTER, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L  ", KEYCODE_L, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "K  ", KEYCODE_K, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "J  ", KEYCODE_J, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H  ", KEYCODE_H, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN8 KEY ROW 7 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE Pound", KEYCODE_SPACE, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".  ,", KEYCODE_STOP, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "M  >", KEYCODE_M, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "N  <", KEYCODE_N, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "B  ", KEYCODE_B, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN9 special keys 1 */
PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(LEFT)", KEYCODE_LEFT, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN10 special keys 2 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "(BACKSPACE)", KEYCODE_BACKSPACE, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(GRAPHICS)", KEYCODE_LALT, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "(RIGHT)", KEYCODE_RIGHT, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "(UP)", KEYCODE_UP, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(DOWN)", KEYCODE_DOWN, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

INPUT_PORTS_END

INPUT_PORTS_START(zx81)
PORT_START							   /* IN0 */
PORT_DIPNAME(0x80, 0x00, "16K RAM module")
PORT_DIPSETTING(0x00, DEF_STR(No))
PORT_DIPSETTING(0x80, DEF_STR(Yes))
PORT_BIT(0x7e, 0x0f, IPT_UNUSED)
PORT_BITX(0x01, 0x00, IPT_KEYBOARD | IPF_RESETCPU, "Reset", KEYCODE_F3, IP_JOY_NONE)

PORT_START							   /* IN1 KEY ROW 0 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z     :", KEYCODE_Z, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "X     ;", KEYCODE_X, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C     ?", KEYCODE_C, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "V     /", KEYCODE_V, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN2 KEY ROW 1 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "A     NEW     STOP", KEYCODE_A, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "S     SAVE    LPRINT", KEYCODE_S, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "D     DIM     SLOW", KEYCODE_D, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F     FOR     FAST", KEYCODE_F, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G     GOTO    LLIST", KEYCODE_G, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN3 KEY ROW 2 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q     PLOT    \"\"", KEYCODE_Q, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W     UNPLOT  OR", KEYCODE_W, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E     REM     STEP", KEYCODE_E, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R     RUN     <=", KEYCODE_R, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T     RAND    <>", KEYCODE_T, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN4 KEY ROW 3 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1     EDIT", KEYCODE_1, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2     AND", KEYCODE_2, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "3     THEN", KEYCODE_3, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4     TO", KEYCODE_4, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5     LEFT", KEYCODE_5, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN5 KEY ROW 4 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0     RUBOUT", KEYCODE_0, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9     GRAPHICS", KEYCODE_9, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8     RIGHT", KEYCODE_8, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7     UP", KEYCODE_7, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6     DOWN", KEYCODE_6, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN6 KEY ROW 5 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P     PRINT   \"", KEYCODE_P, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O     POKE    )", KEYCODE_O, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I     INPUT   (", KEYCODE_I, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "U     IF      $", KEYCODE_U, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y     RETURN  >=", KEYCODE_Y, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN7 KEY ROW 6 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER FUNCTION", KEYCODE_ENTER, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L     LET     =", KEYCODE_L, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "K     LIST    +", KEYCODE_K, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "J     LOAD    -", KEYCODE_J, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H     GOSUB   **", KEYCODE_H, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN8 KEY ROW 7 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE Pound", KEYCODE_SPACE, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".     ,", KEYCODE_STOP, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "M     PAUSE   >", KEYCODE_M, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "N     NEXT    <", KEYCODE_N, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "B     SCROLL  *", KEYCODE_B, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN9 special keys 1 */
PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(LEFT)", KEYCODE_LEFT, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN10 special keys 2 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "(BACKSPACE)", KEYCODE_BACKSPACE, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(GRAPHICS)", KEYCODE_LALT, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "(RIGHT)", KEYCODE_RIGHT, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "(UP)", KEYCODE_UP, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(DOWN)", KEYCODE_DOWN, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

INPUT_PORTS_END

INPUT_PORTS_START(pow3000)
PORT_START							   /* IN0 */
PORT_BIT(0xfe, 0x0f, IPT_UNUSED)
PORT_BITX(0x01, 0x00, IPT_KEYBOARD | IPF_RESETCPU, "Reset", KEYCODE_F3, IP_JOY_NONE)

PORT_START							   /* IN1 KEY ROW 0 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SHIFT", KEYCODE_LSHIFT, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "Z     CLS??", KEYCODE_Z, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "X     AUTO??", KEYCODE_X, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "C     ???", KEYCODE_C, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "V     LEFT", KEYCODE_V, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN2 KEY ROW 1 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "A     UP??", KEYCODE_A, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "S     DOWN??", KEYCODE_S, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "D     RIGHT", KEYCODE_D, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "F     ", KEYCODE_F, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "G     ", KEYCODE_G, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN3 KEY ROW 2 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "Q     ", KEYCODE_Q, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "W     ", KEYCODE_W, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "E     ", KEYCODE_E, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "R     ", KEYCODE_R, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "T     ", KEYCODE_T, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN4 KEY ROW 3 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "1     ", KEYCODE_1, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "2     ", KEYCODE_2, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "3     ", KEYCODE_3, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "4     ", KEYCODE_4, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "5     ", KEYCODE_5, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN5 KEY ROW 4 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "0     ", KEYCODE_0, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "9     ", KEYCODE_9, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "8     ", KEYCODE_8, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "7     ", KEYCODE_7, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "6     ", KEYCODE_6, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN6 KEY ROW 5 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "P     ", KEYCODE_P, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "O     ", KEYCODE_O, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "I     ", KEYCODE_I, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "U     ", KEYCODE_U, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "Y     ", KEYCODE_Y, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN7 KEY ROW 6 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "ENTER ", KEYCODE_ENTER, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "L     ", KEYCODE_L, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "K     ", KEYCODE_K, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "J     ", KEYCODE_J, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "H     ", KEYCODE_H, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN8 KEY ROW 7 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "SPACE ", KEYCODE_SPACE, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, ".     ", KEYCODE_STOP, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "M     ", KEYCODE_M, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "N     ", KEYCODE_N, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "B     ", KEYCODE_B, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN9 special keys 1 */
PORT_BIT(0x0f, IP_ACTIVE_LOW, IPT_UNUSED)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(LEFT)", KEYCODE_LEFT, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

PORT_START							   /* IN10 special keys 2 */
PORT_BITX(0x01, IP_ACTIVE_LOW, IPT_KEYBOARD, "(UP)", KEYCODE_UP, IP_JOY_NONE)
PORT_BITX(0x02, IP_ACTIVE_LOW, IPT_KEYBOARD, "(DOWN)", KEYCODE_DOWN, IP_JOY_NONE)
PORT_BITX(0x04, IP_ACTIVE_LOW, IPT_KEYBOARD, "(RIGHT)", KEYCODE_RIGHT, IP_JOY_NONE)
PORT_BITX(0x08, IP_ACTIVE_LOW, IPT_KEYBOARD, "(BACKSPACE)", KEYCODE_BACKSPACE, IP_JOY_NONE)
PORT_BITX(0x10, IP_ACTIVE_LOW, IPT_KEYBOARD, "(GRAPHICS)", KEYCODE_LALT, IP_JOY_NONE)
PORT_BIT(0xe0, IP_ACTIVE_LOW, IPT_UNUSED)

INPUT_PORTS_END

static struct GfxLayout zx_pixel_layout =
{
	8, 1,							   /* 8x1 pixels */
	256,							   /* 256 codes */
	1,								   /* 1 bit per pixel */
	{0},							   /* no bitplanes */
	/* x offsets */
	{0, 1, 2, 3, 4, 5, 6, 7},
	/* y offsets */
	{0},
	8								   /* one byte per code */
};

static struct GfxLayout zx_char_layout =
{
	8, 8,							   /* 8x8 pixels */
	64,								   /* 64 codes */
	1,								   /* 1 bit per pixel */
	{0},							   /* no bitplanes */
	/* x offsets */
	{0, 1, 2, 3, 4, 5, 6, 7},
	/* y offsets */
	{0 * 8, 1 * 8, 2 * 8, 3 * 8, 4 * 8, 5 * 8, 6 * 8, 7 * 8},
	8 * 8							   /* eight bytes per code */
};

static struct GfxDecodeInfo zx80_gfxdecodeinfo[] =
{
	{REGION_GFX1, 0x0000, &zx_pixel_layout, 0, 2},
	{REGION_CPU1, 0x0e00, &zx_char_layout, 0, 2},
	{-1}							   /* end of array */
};

static struct GfxDecodeInfo zx81_gfxdecodeinfo[] =
{
	{REGION_GFX1, 0x0000, &zx_pixel_layout, 0, 2},
	{REGION_CPU1, 0x1e00, &zx_char_layout, 0, 2},
	{-1}							   /* end of array */
};

static struct GfxDecodeInfo pc8300_gfxdecodeinfo[] =
{
	{REGION_GFX1, 0x0000, &zx_pixel_layout, 0, 2},
	{REGION_GFX2, 0x0000, &zx_char_layout, 0, 2},
	{-1}							   /* end of array */
};

static struct GfxDecodeInfo pow3000_gfxdecodeinfo[] =
{
	{REGION_GFX1, 0x0000, &zx_pixel_layout, 0, 2},
	{REGION_GFX2, 0x0000, &zx_char_layout, 0, 2},
	{-1}							   /* end of array */
};

static unsigned char zx80_palette[] =
{
	255,255,255,	/* white */
	  0,  0,  0,	/* black */
};

static unsigned char zx81_palette[] =
{
	255,255,255,	/* white */
	  0,  0,  0,	/* black */
};

static unsigned char ts1000_palette[] =
{
	 64,244,244,	/* cyan */
	  0,  0,  0,	/* black */
};

static unsigned short zx_colortable[] =
{
	0, 1,							   /* white on black */
	1, 0							   /* black on white */
};


/* Initialise the palette */
static PALETTE_INIT( zx80 )
{
	palette_set_colors(0, zx80_palette, sizeof(zx80_palette) / 3);
	memcpy(colortable, zx_colortable, sizeof (zx_colortable));
}

static PALETTE_INIT( zx81 )
{
	palette_set_colors(0, zx81_palette, sizeof(zx81_palette) / 3);
	memcpy(colortable, zx_colortable, sizeof (zx_colortable));
}

static PALETTE_INIT( ts1000 )
{
	palette_set_colors(0, ts1000_palette, sizeof(ts1000_palette) / 3);
	memcpy(colortable, zx_colortable, sizeof (zx_colortable));
}

static struct DACinterface dac_interface =
{
	1,								   /* number of DACs */
	{100}							   /* volume */
};

#define CPU_CLOCK	3227500
#define CYCLES_PER_SCANLINE 207

static MACHINE_DRIVER_START( zx80 )
	/* basic machine hardware */
	MDRV_CPU_ADD_TAG("main", Z80, CPU_CLOCK)
	MDRV_CPU_MEMORY(readmem_zx80, writemem_zx80)
	MDRV_CPU_PORTS(readport, writeport)
	MDRV_FRAMES_PER_SECOND(1.0 * CPU_CLOCK / 310 / CYCLES_PER_SCANLINE)
	MDRV_VBLANK_DURATION(0)
	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_INIT( zx80 )

    /* video hardware */
	MDRV_VIDEO_ATTRIBUTES(VIDEO_TYPE_RASTER)
	MDRV_SCREEN_SIZE(32*8, 310)
	MDRV_VISIBLE_AREA(0, 32*8-1, 48, 310-32-1)
	MDRV_GFXDECODE( zx80_gfxdecodeinfo )
	MDRV_PALETTE_LENGTH(6)
	MDRV_COLORTABLE_LENGTH(4)
	MDRV_PALETTE_INIT( zx80 )

	MDRV_VIDEO_START( zx )
	MDRV_VIDEO_UPDATE( zx )

	/* sound hardware */
	MDRV_SOUND_ADD(DAC, dac_interface)
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( zx81 )
	MDRV_IMPORT_FROM( zx80 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_MEMORY( readmem_zx81, writemem_zx81 )

	MDRV_MACHINE_INIT( zx81 )
	MDRV_GFXDECODE( zx81_gfxdecodeinfo )
	MDRV_PALETTE_INIT( zx81 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ts1000 )
	MDRV_IMPORT_FROM( zx81 )
	MDRV_FRAMES_PER_SECOND(1.0 * CPU_CLOCK / 262 / CYCLES_PER_SCANLINE)

	MDRV_SCREEN_SIZE(32*8, 262)
	MDRV_VISIBLE_AREA(0, 32*8-1, 17, 262-15-1)
	MDRV_PALETTE_INIT( ts1000 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pc8300 )
	MDRV_IMPORT_FROM( ts1000 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_MEMORY( readmem_pc8300, writemem_pc8300 )

	MDRV_MACHINE_INIT( pc8300 )
	MDRV_GFXDECODE( pc8300_gfxdecodeinfo )
	MDRV_PALETTE_INIT( zx81 )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pow3000 )
	MDRV_IMPORT_FROM( ts1000 )
	MDRV_CPU_MODIFY( "main" )
	MDRV_CPU_MEMORY( readmem_pow3000, writemem_pow3000 )

	MDRV_MACHINE_INIT( pow3000 )
	MDRV_GFXDECODE( pow3000_gfxdecodeinfo )
	MDRV_PALETTE_INIT( zx81 )
MACHINE_DRIVER_END


ROM_START(zx80)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("zx80.rom",    0x0000, 0x1000, 0x4c7fc597)
    ROM_REGION(0x00100, REGION_GFX1,0)
	/* filled in by zx_init_driver */
ROM_END


ROM_START(aszmic)
    ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("aszmic.rom",  0x0000, 0x1000, 0x6c123536)
    ROM_REGION(0x00100, REGION_GFX1,0)
    /* filled in by zx_init_driver */
ROM_END

ROM_START(zx81)
        ROM_REGION(0x10000, REGION_CPU1,0)
        ROM_LOAD("zx81.rom",    0x0000, 0x2000, 0xfcbbd617)
        ROM_REGION(0x00100, REGION_GFX1,0)
        /* filled in by zx_init_driver */
ROM_END                                                                                                                                       

ROM_START(zx81a)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("zx81a.rom",    0x0000, 0x2000, 0x4b1dd6eb)
	ROM_REGION(0x00100, REGION_GFX1,0)
	/* filled in by zx_init_driver */
ROM_END

ROM_START(zx81b)
        ROM_REGION(0x10000, REGION_CPU1,0)
        ROM_LOAD("zx81b.rom",    0x0000, 0x2000, 0x522c37b8)
        ROM_REGION(0x00100, REGION_GFX1,0)
        /* filled in by zx_init_driver */
ROM_END                                                                                                                                       

ROM_START(ts1000)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("zx81a.rom",  0x0000, 0x2000, 0x4b1dd6eb)
	ROM_REGION(0x00100, REGION_GFX1,0)
	/* filled in by zx_init_driver */
ROM_END

ROM_START(pc8300)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("8300_org.rom",0x0000, 0x2000, 0xa350f2b1)
	ROM_REGION(0x00100, REGION_GFX1,0)
	/* filled in by zx_init_driver */
	ROM_REGION(0x00200, REGION_GFX2,0)
	ROM_LOAD("8300_fnt.bin",0x0000, 0x0200, 0x6bd0408c)
ROM_END

ROM_START(pow3000)
	ROM_REGION(0x10000, REGION_CPU1,0)
	ROM_LOAD("pow3000.rom", 0x0000, 0x2000, 0x8a49b2c3)
	ROM_REGION(0x00100, REGION_GFX1,0)
	/* filled in by zx_init_driver */
	ROM_REGION(0x00200, REGION_GFX2,0)
	ROM_LOAD("pow3000.chr", 0x0000, 0x0200, 0x1c42fe46)
ROM_END

ROM_START(lambda)
        ROM_REGION(0x10000, REGION_CPU1,0)
        ROM_LOAD("lambda.rom",0x0000, 0x2000, 0x8a49b2c3)
        ROM_REGION(0x00100, REGION_GFX1,0)
        /* filled in by zx_init_driver */
        ROM_REGION(0x00200, REGION_GFX2,0)
        ROM_LOAD("8300_fnt.bin",0x0000, 0x0200, 0x6bd0408c)
ROM_END                                                                                                                                       

static const struct IODevice io_zx80[] =
{
	{
		IO_CASSETTE,		/* type */
		1,					/* count */
		"80\0o\0",          /* file extensions */
		IO_RESET_ALL,		/* reset if file changed */
		OSD_FOPEN_DUMMY,	/* open mode */
        NULL,               /* id */
		zx_cassette_init,	/* init */
		zx_cassette_exit,	/* exit */
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
	{IO_END}
};

static const struct IODevice io_zx81[] =
{
	{
		IO_CASSETTE,		/* type */
		1,					/* count */
		"81\0p\0",          /* file extensions */
		IO_RESET_ALL,		/* reset if file changed */
		OSD_FOPEN_DUMMY,	/* open mode */
        NULL,               /* id */
		zx_cassette_init,	/* init */
		zx_cassette_exit,	/* exit */
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
	{IO_END}
};

#define io_aszmic	io_zx80
#define io_zx81a	io_zx81
#define io_zx81b	io_zx81
#define io_ts1000	io_zx81
#define io_pc8300	io_zx81
#define io_pow3000	io_zx81
#define io_lambda	io_zx81

SYSTEM_CONFIG_START(zx80)
SYSTEM_CONFIG_END

SYSTEM_CONFIG_START(zx81)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/
/*	  YEAR 	NAME	 PARENT	MACHINE		INPUT		INIT	CONFIG	COMPANY		FULLNAME */
COMPX(1980,	zx80,    0,		zx80,		zx80,		zx,		zx80,	"Sinclair Research", "ZX-80",		GAME_NOT_WORKING)
COMPX(1981,	aszmic,  zx80,	zx80,		zx80,		zx,		zx80,	"Sinclair Research", "ZX.Aszmic",	GAME_NOT_WORKING)
COMPX(1981,	zx81,    0,		zx81,		zx81,       zx,		zx81,	"Sinclair Research", "ZX-81",           GAME_NOT_WORKING)
COMPX(198?,	zx81a,   zx81,	zx81,		zx81,		zx,		zx81,	"Sinclair Research", "ZX-81 (2nd rev)",	GAME_NOT_WORKING)
COMPX(198?,	zx81b,   zx81,	zx81,		zx81,		zx,		zx81,	"Sinclair Research", "ZX-81 (3rd rev)", GAME_NOT_WORKING)
COMPX(1982,	ts1000,  zx81,	ts1000,		zx81,		zx,		zx81,	"Timex Sinclair",    "Timex Sinclair 1000",		GAME_NOT_WORKING | GAME_ALIAS)
COMPX(1984,	pc8300,  zx81,	pc8300,		pow3000,	zx,		zx81,	"Your Computer",     "PC8300",		GAME_NOT_WORKING)
COMPX(1983,	pow3000, zx81,	pow3000,	pow3000,  	zx,		zx81,	"Creon Enterprises", "Power 3000",	GAME_NOT_WORKING)
COMPX(1982,	lambda,  zx81,	pc8300,		pow3000,	zx,		zx81,	"Lambda Electronics Ltd","Lambda 8300",	GAME_NOT_WORKING)
