/*

    TODO:

    - unreliable DOS commands?
    - tape input/output
    - PL-80 plotter
    - serial printer
    - thermal printer

*/

#include "includes/comx35.h"

/* Memory Maps */

static ADDRESS_MAP_START( comx35_map, AS_PROGRAM, 8, comx35_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x3fff) AM_ROM
	AM_RANGE(0x1000, 0x17ff) AM_ROMBANK("bank2")
	AM_RANGE(0x1800, 0x3fff) AM_ROM
	AM_RANGE(0x4000, 0xbfff) AM_RAM
	AM_RANGE(0xc000, 0xdfff) AM_RAMBANK("bank1")
	AM_RANGE(0xe000, 0xefff) AM_ROMBANK("bank3")
	AM_RANGE(0xf400, 0xf7ff) AM_DEVREADWRITE(CDP1869_TAG, cdp1869_device, char_ram_r, char_ram_w)
	AM_RANGE(0xf800, 0xffff) AM_DEVREADWRITE(CDP1869_TAG, cdp1869_device, page_ram_r, page_ram_w)
ADDRESS_MAP_END

static ADDRESS_MAP_START( comx35_io_map, AS_IO, 8, comx35_state )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x01, 0x01) AM_WRITE(bank_select_w)
	AM_RANGE(0x02, 0x02) AM_READWRITE(io_r, io_w)
	AM_RANGE(0x03, 0x03) AM_DEVREAD(CDP1871_TAG, cdp1871_device, data_r)
	AM_RANGE(0x03, 0x07) AM_WRITE(cdp1869_w)
ADDRESS_MAP_END

/* Input Ports */

#define COMX35_DEVICES \
	PORT_CONFSETTING( 0x00, DEF_STR( None ) ) \
	PORT_CONFSETTING( 0x01, "DOS Card" ) \
	PORT_CONFSETTING( 0x02, "Parallel Output Interface" ) \
	PORT_CONFSETTING( 0x03, "Parallel Output Interface (F&M)" ) \
	PORT_CONFSETTING( 0x04, "RS-232C Serial Output Interface" ) \
	PORT_CONFSETTING( 0x05, "Thermal Printer Interface" ) \
	PORT_CONFSETTING( 0x06, "F&M Joycard" ) \
	PORT_CONFSETTING( 0x07, "80-Column Card" ) \
	PORT_CONFSETTING( 0x08, "RAM Card" )

static INPUT_PORTS_START( comx35 )
	PORT_START("D1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("0 \xE2\x96\xA0") PORT_CODE(KEYCODE_0) PORT_CHAR('0')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_1) PORT_CHAR('1') PORT_CHAR('!')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_2) PORT_CHAR('2') PORT_CHAR('"')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_3) PORT_CHAR('3') PORT_CHAR('#')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_4) PORT_CHAR('4') PORT_CHAR('$')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_5) PORT_CHAR('5') PORT_CHAR('%')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_6) PORT_CHAR('6') PORT_CHAR('&')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_7) PORT_CHAR('7') PORT_CHAR('?')

	PORT_START("D2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_8) PORT_CHAR('8') PORT_CHAR('[')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_9) PORT_CHAR('9') PORT_CHAR(']')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_STOP) PORT_CHAR('.') PORT_CHAR(':')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COMMA) PORT_CHAR(',') PORT_CHAR(';')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_MINUS) PORT_CHAR('<') PORT_CHAR('(')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_OPENBRACE) PORT_CHAR('=') PORT_CHAR('^')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_EQUALS) PORT_CHAR('>') PORT_CHAR(')')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_CLOSEBRACE) PORT_CHAR('\\') PORT_CHAR('_')

	PORT_START("D3")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_TILDE) PORT_CHAR('?')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_A) PORT_CHAR('A')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_B) PORT_CHAR('B')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_C) PORT_CHAR('C')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_D) PORT_CHAR('D')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_E) PORT_CHAR('E')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_F) PORT_CHAR('F')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_G) PORT_CHAR('G')

	PORT_START("D4")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_H) PORT_CHAR('H')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_I) PORT_CHAR('I')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_J) PORT_CHAR('J')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_K) PORT_CHAR('K')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_L) PORT_CHAR('L')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_M) PORT_CHAR('M')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_N) PORT_CHAR('N')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_O) PORT_CHAR('O')

	PORT_START("D5")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_P) PORT_CHAR('P')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Q) PORT_CHAR('Q')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_R) PORT_CHAR('R')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_S) PORT_CHAR('S')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_T) PORT_CHAR('T')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_U) PORT_CHAR('U')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_V) PORT_CHAR('V')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_W) PORT_CHAR('W')

	PORT_START("D6")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_X) PORT_CHAR('X')
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Y) PORT_CHAR('Y')
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_Z) PORT_CHAR('Z')
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_COLON) PORT_CHAR('+') PORT_CHAR('{')
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_QUOTE) PORT_CHAR('-') PORT_CHAR('|')
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_BACKSLASH) PORT_CHAR('*') PORT_CHAR('}')
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_CODE(KEYCODE_SLASH) PORT_CHAR('/') PORT_CHAR('~')
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SPACE") PORT_CODE(KEYCODE_SPACE) PORT_CHAR(' ') PORT_CHAR(8)

	PORT_START("D7")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("D8")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("CR") PORT_CODE(KEYCODE_ENTER) PORT_CHAR(13)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("ESC") PORT_CODE(KEYCODE_ESC) PORT_CHAR(UCHAR_MAMEKEY(ESC))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(UTF8_UP) PORT_CODE(KEYCODE_UP) PORT_CHAR(UCHAR_MAMEKEY(UP))
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(UTF8_RIGHT) PORT_CODE(KEYCODE_RIGHT) PORT_CHAR(UCHAR_MAMEKEY(RIGHT))
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(UTF8_LEFT) PORT_CODE(KEYCODE_LEFT) PORT_CHAR(UCHAR_MAMEKEY(LEFT))
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME(UTF8_DOWN) PORT_CODE(KEYCODE_DOWN) PORT_CHAR(UCHAR_MAMEKEY(DOWN))
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("DEL") PORT_CODE(KEYCODE_BACKSPACE) PORT_CHAR(8)
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("D9")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("D10")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("D11")
	PORT_BIT( 0xff, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("MODIFIERS")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("SHIFT") PORT_CODE(KEYCODE_LSHIFT) PORT_CODE(KEYCODE_RSHIFT) PORT_CHAR(UCHAR_SHIFT_1)
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("CNTL") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL) PORT_CHAR(UCHAR_MAMEKEY(LCONTROL)) PORT_CHAR(UCHAR_MAMEKEY(RCONTROL))
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("RESET")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD ) PORT_NAME("RT") PORT_CODE(KEYCODE_F10) PORT_CHAR(UCHAR_MAMEKEY(F10)) PORT_CHANGED(comx35_reset, NULL)

	PORT_START("JOY1")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )    PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )  PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(1) PORT_8WAY
	PORT_BIT( 0xf0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("JOY2")
	PORT_BIT( 0x01, IP_ACTIVE_HIGH, IPT_JOYSTICK_UP )    PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x02, IP_ACTIVE_HIGH, IPT_JOYSTICK_DOWN )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x04, IP_ACTIVE_HIGH, IPT_JOYSTICK_LEFT )  PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x08, IP_ACTIVE_HIGH, IPT_JOYSTICK_RIGHT ) PORT_PLAYER(2) PORT_8WAY
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(1)
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_BUTTON1 ) PORT_PLAYER(2)
	PORT_BIT( 0xc0, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START("EXPANSION")
	PORT_CONFNAME( 0x0f, 0x00, "Expansion Slot")
	COMX35_DEVICES

	PORT_START("SLOT1")
	PORT_CONFNAME( 0x0f, 0x01, "Expansion Box Slot 1")
	COMX35_DEVICES

	PORT_START("SLOT2")
	PORT_CONFNAME( 0x0f, 0x02, "Expansion Box Slot 2")
	COMX35_DEVICES

	PORT_START("SLOT3")
	PORT_CONFNAME( 0x0f, 0x07, "Expansion Box Slot 3")
	COMX35_DEVICES

	PORT_START("SLOT4")
	PORT_CONFNAME( 0x0f, 0x08, "Expansion Box Slot 4")
	COMX35_DEVICES

	PORT_START("PRINTER")
	PORT_CONFNAME( 0x04, 0x00, "Printer")
	PORT_CONFSETTING( 0x00, "Parallel" )
	PORT_CONFSETTING( 0x01, "Serial" )
	PORT_CONFSETTING( 0x02, "Thermal" )
	PORT_CONFSETTING( 0x03, "COMX PL-80 Plotter" )
INPUT_PORTS_END

/* CDP1802 Interface */

READ_LINE_MEMBER( comx35_state::clear_r )
{
	return m_reset;
}

READ_LINE_MEMBER( comx35_state::ef2_r )
{
	if (m_iden)
	{
		// interrupts disabled: PAL/NTSC
		return m_vis->pal_ntsc_r();
	}
	else
	{
		// interrupts enabled: keyboard repeat
		return m_kbe->rpt_r();
	}
}

READ_LINE_MEMBER( comx35_state::ef4_r )
{
	return (m_cassette->input() < 0) | m_cdp1802_ef4;
}

static COSMAC_SC_WRITE( comx35_sc_w )
{
	comx35_state *state = device->machine().driver_data<comx35_state>();

	switch (sc)
	{
	case COSMAC_STATE_CODE_S0_FETCH:
		// not connected
		break;

	case COSMAC_STATE_CODE_S1_EXECUTE:
		// every other S1 triggers a DMAOUT request
		if (state->m_dma)
		{
			state->m_dma = 0;

			if (!state->m_iden)
			{
				state->m_maincpu->set_input_line(COSMAC_INPUT_LINE_DMAOUT, ASSERT_LINE);
			}
		}
		else
		{
			state->m_dma = 1;
		}
		break;

	case COSMAC_STATE_CODE_S2_DMA:
		// DMA acknowledge clears the DMAOUT request
		state->m_maincpu->set_input_line(COSMAC_INPUT_LINE_DMAOUT, CLEAR_LINE);
		break;

	case COSMAC_STATE_CODE_S3_INTERRUPT:
		// interrupt acknowledge clears the INT request
		state->m_maincpu->set_input_line(COSMAC_INPUT_LINE_INT, CLEAR_LINE);
		break;
	}
}

WRITE_LINE_MEMBER( comx35_state::q_w )
{
	m_cdp1802_q = state;

	if (m_iden && state)
	{
		// enable interrupts
		m_iden = 0;
	}

	// cassette output
	m_cassette->output(state ? +1.0 : -1.0);
	
	// expansion bus
	m_expansion->q_w(state);
}

static COSMAC_INTERFACE( cosmac_intf )
{
	DEVCB_LINE_VCC,								// wait
	DEVCB_DRIVER_LINE_MEMBER(comx35_state, clear_r),// clear
	DEVCB_NULL,									// EF1
	DEVCB_DRIVER_LINE_MEMBER(comx35_state, ef2_r),	// EF2
	DEVCB_NULL,									// EF3
	DEVCB_DRIVER_LINE_MEMBER(comx35_state, ef4_r),	// EF4
	DEVCB_DRIVER_LINE_MEMBER(comx35_state, q_w),	// Q
	DEVCB_NULL,									// DMA in
	DEVCB_NULL,									// DMA out
	comx35_sc_w,								// SC
	DEVCB_NULL,									// TPA
	DEVCB_NULL									// TPB
};

/* CDP1871 Interface */

READ_LINE_MEMBER( comx35_state::shift_r )
{
	return BIT(input_port_read(machine(), "MODIFIERS"), 0);
}

READ_LINE_MEMBER( comx35_state::control_r )
{
	return BIT(input_port_read(machine(), "MODIFIERS"), 1);
}

static CDP1871_INTERFACE( kbc_intf )
{
	DEVCB_INPUT_PORT("D1"),
	DEVCB_INPUT_PORT("D2"),
	DEVCB_INPUT_PORT("D3"),
	DEVCB_INPUT_PORT("D4"),
	DEVCB_INPUT_PORT("D5"),
	DEVCB_INPUT_PORT("D6"),
	DEVCB_INPUT_PORT("D7"),
	DEVCB_INPUT_PORT("D8"),
	DEVCB_INPUT_PORT("D9"),
	DEVCB_INPUT_PORT("D10"),
	DEVCB_INPUT_PORT("D11"),
	DEVCB_DRIVER_LINE_MEMBER(comx35_state, shift_r),
	DEVCB_DRIVER_LINE_MEMBER(comx35_state, control_r),
	DEVCB_NULL,
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, COSMAC_INPUT_LINE_EF3),
	DEVCB_NULL // polled
};

/* Machine Drivers */

static const cassette_interface cassette_intf =
{
	cassette_default_formats,
	NULL,
	(cassette_state)(CASSETTE_STOPPED | CASSETTE_MOTOR_ENABLED | CASSETTE_SPEAKER_ENABLED),
	NULL,
	NULL
};

static const floppy_interface floppy_intf =
{
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL,
	FLOPPY_STANDARD_5_25_DSDD,
	FLOPPY_OPTIONS_NAME(comx35),
	"floppy_5_25",
	NULL
};

WRITE_LINE_MEMBER( comx35_state::ef4_w )
{
	m_cdp1802_ef4 = state;
}

static COMX_EXPANSION_INTERFACE( expansion_intf )
{
	DEVCB_CPU_INPUT_LINE(CDP1802_TAG, COSMAC_INPUT_LINE_INT),
	DEVCB_DRIVER_LINE_MEMBER(comx35_state, ef4_w),
	DEVCB_NULL,
	DEVCB_NULL,
	DEVCB_NULL
};

static SLOT_INTERFACE_START( comx_expansion_cards )
	SLOT_INTERFACE("eb", COMX_EB)
	SLOT_INTERFACE("fd", COMX_FD)
	SLOT_INTERFACE("clm", COMX_CLM)
	SLOT_INTERFACE("ram", COMX_RAM)
	SLOT_INTERFACE("joy", COMX_JOY)
	SLOT_INTERFACE("prn", COMX_PRN)
	SLOT_INTERFACE("thm", COMX_THM)
SLOT_INTERFACE_END

static MACHINE_CONFIG_START( pal, comx35_state )
	/* basic system hardware */
	MCFG_CPU_ADD(CDP1802_TAG, COSMAC, CDP1869_CPU_CLK_PAL)
	MCFG_CPU_PROGRAM_MAP(comx35_map)
	MCFG_CPU_IO_MAP(comx35_io_map)
	MCFG_CPU_CONFIG(cosmac_intf)

	/* sound and video hardware */
	MCFG_FRAGMENT_ADD(comx35_pal_video)

	/* peripheral hardware */
	MCFG_CDP1871_ADD(CDP1871_TAG, kbc_intf, CDP1869_CPU_CLK_PAL / 8)
	MCFG_WD1770_ADD(WD1770_TAG, comx35_wd17xx_interface)
	MCFG_QUICKLOAD_ADD("quickload", comx35_comx, "comx", 0)
	MCFG_CASSETTE_ADD(CASSETTE_TAG, cassette_intf)
	MCFG_PRINTER_ADD("printer")

	MCFG_FLOPPY_2_DRIVES_ADD(floppy_intf)
	MCFG_COMXPL80_ADD()
	
	// expansion bus
	MCFG_COMX_EXPANSION_BUS_ADD(CDP1802_TAG, CDP1869_CPU_CLK_PAL, expansion_intf)
	//MCFG_COMX_EXPANSION_SLOT_ADD("slot", comx_expansion_cards, NULL)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("32K")
MACHINE_CONFIG_END

static MACHINE_CONFIG_START( ntsc, comx35_state )
	/* basic system hardware */
	MCFG_CPU_ADD(CDP1802_TAG, COSMAC, CDP1869_CPU_CLK_NTSC)
	MCFG_CPU_PROGRAM_MAP(comx35_map)
	MCFG_CPU_IO_MAP(comx35_io_map)
	MCFG_CPU_CONFIG(cosmac_intf)

	/* sound and video hardware */
	MCFG_FRAGMENT_ADD(comx35_ntsc_video)

	/* peripheral hardware */
	MCFG_CDP1871_ADD(CDP1871_TAG, kbc_intf, CDP1869_CPU_CLK_NTSC / 8)
	MCFG_WD1770_ADD(WD1770_TAG, comx35_wd17xx_interface)
	MCFG_QUICKLOAD_ADD("quickload", comx35_comx, "comx", 0)
	MCFG_CASSETTE_ADD(CASSETTE_TAG, cassette_intf)
	MCFG_PRINTER_ADD("printer")

	MCFG_FLOPPY_2_DRIVES_ADD(floppy_intf)
	MCFG_COMXPL80_ADD()
	
	// expansion bus
	MCFG_COMX_EXPANSION_BUS_ADD(CDP1802_TAG, CDP1869_CPU_CLK_NTSC, expansion_intf)
	//MCFG_COMX_EXPANSION_SLOT_ADD("slot", comx_expansion_cards, NULL)

	/* internal ram */
	MCFG_RAM_ADD(RAM_TAG)
	MCFG_RAM_DEFAULT_SIZE("32K")
MACHINE_CONFIG_END

/* ROMs */

ROM_START( comx35p )
	ROM_REGION( 0x10000, CDP1802_TAG, 0 )
	ROM_DEFAULT_BIOS( "basic100e" )

	ROM_SYSTEM_BIOS( 0, "basic100", "COMX BASIC V1.00" )
	ROMX_LOAD( "comx_10.u21",			0x0000, 0x4000, CRC(68d0db2d) SHA1(062328361629019ceed9375afac18e2b7849ce47), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "basic100e", "COMX BASIC V1.00 with Expansion Box" )
	ROMX_LOAD( "comx_10.u21",			0x0000, 0x4000, CRC(68d0db2d) SHA1(062328361629019ceed9375afac18e2b7849ce47), ROM_BIOS(2) )
	ROMX_LOAD( "expansion.e5",			0xe000, 0x1000, CRC(52cb44e2) SHA1(3f9a3d9940b36d4fee5eca9f1359c99d7ed545b9), ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "fmbasic31", "F&M BASIC V3.1 with Expansion Box" )
	ROMX_LOAD( "comx_10.u21",			0x0000, 0x4000, CRC(68d0db2d) SHA1(062328361629019ceed9375afac18e2b7849ce47), ROM_BIOS(3) )
	ROMX_LOAD( "f&m.expansion.3.1.e5",	0xe000, 0x1000, CRC(818ca2ef) SHA1(ea000097622e7fd472d53e7899e3c83773433045), ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "fmbasic32", "F&M BASIC V3.2 with Expansion Box" )
	ROMX_LOAD( "comx_10.u21",			0x0000, 0x4000, CRC(68d0db2d) SHA1(062328361629019ceed9375afac18e2b7849ce47), ROM_BIOS(4) )
	ROMX_LOAD( "f&m.expansion.3.2.e5",	0xe000, 0x1000, CRC(0f0fc960) SHA1(eb6b6e7bc9e761d13554482025d8cb5e260c0619), ROM_BIOS(4) )
	ROM_SYSTEM_BIOS( 4, "basic101", "COMX BASIC V1.01" )
	ROMX_LOAD( "comx_11.u21",			0x0000, 0x4000, CRC(609d89cd) SHA1(799646810510d8236fbfafaff7a73d5170990f16), ROM_BIOS(5) )

	ROM_REGION( 0x2000, "fdc", 0 ) /* Disc Controller Card */
	ROM_LOAD( "d.o.s. v1.2.f4",			0x0000, 0x2000, CRC(cf4ecd2e) SHA1(290e19bdc89e3c8059e63d5ae3cca4daa194e1fe) )

	ROM_REGION( 0x2000, "printer", 0 )
	ROM_LOAD( "printer.bin",			0x0000, 0x0800, CRC(3bbc2b2e) SHA1(08bf7ea4174713ab24969c553affd5c1401876b8) )

	ROM_REGION( 0x2000, "printer_fm", 0 )
	ROM_LOAD( "f&m.printer.1.2.bin",	0x0000, 0x1000, CRC(2feb997d) SHA1(ee9cb91042696c88ff5f2f44d2f702dc93369ba0) )

	ROM_REGION( 0x2000, "rs232", 0 )
	ROM_LOAD( "rs232.bin",				0x0000, 0x0800, CRC(926ff2d1) SHA1(be02bd388bba0211ea72d4868264a63308e4318d) )

	ROM_REGION( 0x2000, "thermal", 0 )
	ROM_LOAD( "thermal.bin",			0x0000, 0x1000, CRC(41a72ba8) SHA1(3a8760c78bd8c7bec2dbf26657b930c9a6814803) )

	ROM_REGION( 0x2000, "eprom", 0 )
	ROM_LOAD( "f&m.eprom.board.1.1.bin",0x0000, 0x0800, CRC(a042a31a) SHA1(13831a1350aa62a87988bfcc99c4b7db8ef1cf62) )

	ROM_REGION( 0x2000, "80column", 0 ) /* 80 Column Card */
	ROM_LOAD( "p 1.0.cl1",				0x0000, 0x0800, CRC(b417d30a) SHA1(d428b0467945ecb9aec884211d0f4b1d8d56d738) ) // V1.0
	ROM_LOAD( "p 1.1.cl1",				0x0000, 0x0800, CRC(0a2eaf19) SHA1(3f1f640caef964fb47aaa147cab6d215c2b30e9d) ) // V1.1

	ROM_REGION( 0x800, "chargen", 0 ) /* 80 Column Card */
	ROM_LOAD( "c 1.0.cl4",				0x0000, 0x0800, CRC(69dd7b07) SHA1(71d368adbb299103d165eab8359a97769e463e26) ) // V1.0
	ROM_LOAD( "c 1.1.cl4",				0x0000, 0x0800, CRC(dc9b5046) SHA1(4e041cec03dda6dba5e2598d060c49908a4fab2a) ) // V1.1
ROM_END

#define rom_comx35n rom_comx35p

/* System Drivers */

//    YEAR  NAME        PARENT  COMPAT  MACHINE     INPUT     INIT  COMPANY                       FULLNAME            FLAGS
COMP( 1983, comx35p,	0,		0,		pal,		comx35,   0,	"Comx World Operations Ltd",	"COMX 35 (PAL)",	GAME_IMPERFECT_SOUND )
COMP( 1983, comx35n,	comx35p,0,		ntsc,		comx35,   0,	"Comx World Operations Ltd",	"COMX 35 (NTSC)",	GAME_IMPERFECT_SOUND )
