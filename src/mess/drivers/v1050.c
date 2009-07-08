/***************************************************************************

    Visual 1050

    12/05/2009 Skeleton driver.

****************************************************************************/

/*

	TODO:

	- video
	- memory banking
	- �PB8214 chip
	- RTC58321 chip
	- floppy
	- keyboard
	- centronics
	- winchester

*/

#include "driver.h"
#include "includes/v1050.h"
#include "cpu/z80/z80.h"
#include "cpu/m6502/m6502.h"
#include "cpu/mcs48/mcs48.h"
#include "machine/msm8251.h"
#include "machine/8255ppi.h"
#include "machine/wd17xx.h"
#include "video/mc6845.h"

/* Read/Write Handlers */

static WRITE8_HANDLER( dvint_clr_w )
{
	cputag_set_input_line(space->machine, M6502_TAG, INPUT_LINE_IRQ0, CLEAR_LINE);
}

/* Memory Maps */

static ADDRESS_MAP_START( v1050_mem, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x1fff) AM_ROM
	AM_RANGE(0x2000, 0xffff) AM_RAM
ADDRESS_MAP_END

static ADDRESS_MAP_START( v1050_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_UNMAP_HIGH
//	AM_RANGE(0x84, 0x87) AM_DEVREADWRITE(CRT_Z80_PPI8255_TAG, ppi8255_r, ppi8255_w)
//	AM_RANGE(0x88, 0x88) AM_DEVREADWRITE(KB_MSM8251_TAG, msm8251_data_r, msm8251_data_w)
//	AM_RANGE(0x89, 0x89) AM_DEVREADWRITE(KB_MSM8251_TAG, msm8251_status_r, msm8251_control_w)
//	AM_RANGE(0x8c, 0x8c) AM_DEVREADWRITE(SIO_MSM8251_TAG, msm8251_data_r, msm8251_data_w)
//	AM_RANGE(0x8d, 0x8d) AM_DEVREADWRITE(SIO_MSM8251_TAG, msm8251_status_r, msm8251_control_w)
//	AM_RANGE(0x90, 0x93) AM_DEVREADWRITE(FDC_PPI8255_TAG, ppi8255_r, ppi8255_w)
	AM_RANGE(0x94, 0x97) AM_DEVREADWRITE(MB8877_TAG, wd17xx_r, wd17xx_w)
//	AM_RANGE(0x9c, 0x9f) AM_DEVREADWRITE(RTC_PPI8255_TAG, ppi8255_r, ppi8255_w)
//	AM_RANGE(0xa0, 0xa0) AM_READWRITE(vint_clr_r, vint_clr_w)
//	AM_RANGE(0xb0, 0xb0) AM_READWRITE(dint_clr_r, dint_clr_w)
//	AM_RANGE(0xc0, 0xc0) AM_DEVREADWRITE(UPB8214_TAG, pic8214_r, pic8214_w) 
//	AM_RANGE(0xd0, 0xd0) AM_WRITE(bank_w)
//	AM_RANGE(0xe0, 0xe1) AM_READWRITE(sasi_r, sasi_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( v1050_crt_mem, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x0000, 0x1fff) AM_RAM
	AM_RANGE(0x2000, 0x7fff) AM_RAM AM_BASE_MEMBER(v1050_state, video_ram)
	AM_RANGE(0x8000, 0x8000) AM_DEVWRITE(HD6845_TAG, mc6845_address_w)
	AM_RANGE(0x8001, 0x8001) AM_DEVREADWRITE(HD6845_TAG, mc6845_register_r, mc6845_register_w)
//	AM_RANGE(0x9000, 0x9003) AM_DEVREADWRITE(CRT_M6502_PPI8255_TAG, ppi8255_r, ppi8255_w)
//	AM_RANGE(0xa000, 0xa000) AM_WRITE(attr_w)
//	AM_RANGE(0xb000, 0xb000) AM_WRITE(z80int_w)
	AM_RANGE(0xc000, 0xc000) AM_WRITE(dvint_clr_w)
	AM_RANGE(0xe000, 0xffff) AM_ROM
ADDRESS_MAP_END

static ADDRESS_MAP_START( v1050_kbd_io, ADDRESS_SPACE_IO, 8 )
//	AM_RANGE(MCS48_PORT_P1, MCS48_PORT_P1) 
//	AM_RANGE(MCS48_PORT_P2, MCS48_PORT_P2) 
//	AM_RANGE(MCS48_PORT_BUS, MCS48_PORT_BUS) 
ADDRESS_MAP_END

/* Input Ports */

static INPUT_PORTS_START( v1050 )
	PORT_START("X0")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X1")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X2")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X3")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X4")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X5")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X6")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X7")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X8")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("X9")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("XA")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 

	PORT_START("XB")
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x08, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x10, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x20, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x40, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
	PORT_BIT( 0x80, IP_ACTIVE_LOW, IPT_KEYBOARD ) 
INPUT_PORTS_END

/* MB8877 Interface */

static WD17XX_CALLBACK( v1050_mb8877_callback )
{
//	v1050_state *driver_state = device->machine->driver_data;

	switch (state)
	{
	case WD17XX_IRQ_CLR:
//		if (driver_state->f_int_enb) upb8214_r2_w(driver_state->upb8214, 0);
		break;

	case WD17XX_IRQ_SET:
//		if (driver_state->f_int_enb) upb8214_r2_w(driver_state->upb8214, 1);
		break;

	case WD17XX_DRQ_CLR:
		cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_NMI, CLEAR_LINE);
		break;

	case WD17XX_DRQ_SET:
		cputag_set_input_line(device->machine, Z80_TAG, INPUT_LINE_NMI, ASSERT_LINE);
		break;
	}
}

static const wd17xx_interface v1050_wd17xx_intf = 
{ 
	v1050_mb8877_callback
};

/* Machine Initialization */

static MACHINE_START( v1050 )
{
	v1050_state *state = machine->driver_data;

	/* find devices */
	state->upb8214 = devtag_get_device(machine, UPB8214_TAG);

	/* setup memory banking */

	/* register for state saving */
//	state_save_register_global(machine, state->);
}

/* Machine Driver */

static MACHINE_DRIVER_START( v1050 )
	MDRV_DRIVER_DATA(v1050_state)

	/* basic machine hardware */
    MDRV_CPU_ADD(Z80_TAG, Z80, XTAL_16MHz/4)
    MDRV_CPU_PROGRAM_MAP(v1050_mem)
    MDRV_CPU_IO_MAP(v1050_io)

    MDRV_CPU_ADD(M6502_TAG, M6502, XTAL_15_36MHz/16)
    MDRV_CPU_PROGRAM_MAP(v1050_crt_mem)

    MDRV_CPU_ADD(I8049_TAG, I8049, XTAL_4_608MHz)
	MDRV_CPU_IO_MAP(v1050_kbd_io)

    MDRV_MACHINE_START(v1050)

    /* video hardware */
	MDRV_IMPORT_FROM(v1050_video)

	/* devices */
	MDRV_WD1793_ADD(MB8877_TAG, v1050_wd17xx_intf )
MACHINE_DRIVER_END

/* ROMs */

ROM_START( v1050 )
	ROM_REGION( 0x10000, Z80_TAG, 0 )
	ROM_LOAD( "244-032.u86", 0x0000, 0x2000, CRC(46f847a7) SHA1(374db7a38a9e9230834ce015006e2f1996b9609a) )

	ROM_REGION( 0x10000, M6502_TAG, 0 )
	ROM_LOAD( "244-033.u77", 0xe000, 0x2000, CRC(c0502b66) SHA1(bc0015f5b14f98110e652eef9f7c57c614683be5) )

	ROM_REGION( 0x800, I8049_TAG, 0 )
	ROM_LOAD( "20-08049-410.z5", 0x0000, 0x0800, NO_DUMP )
	ROM_LOAD( "22-02716-074.z6", 0x0000, 0x0800, NO_DUMP )
ROM_END

/* System Configuration */

static SYSTEM_CONFIG_START( v1050 )
	CONFIG_RAM_DEFAULT( 128 * 1024 )
SYSTEM_CONFIG_END

/* System Drivers */

/*    YEAR	NAME	PARENT	COMPAT	MACHINE	INPUT	INIT	CONFIG	COMPANY								FULLNAME		FLAGS */
COMP( 1983, v1050,	0,		0,		v1050,	v1050,	0,		v1050,	"Visual Technology Incorporated",	"Visual 1050",	GAME_NOT_WORKING )
