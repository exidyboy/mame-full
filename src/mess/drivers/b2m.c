/***************************************************************************

		Bashkiria-2M driver by Miodrag Milanovic

		28/03/2008 Preliminary driver.
		     
****************************************************************************/


#include "driver.h"
#include "cpu/i8085/i8085.h"
#include "devices/cassette.h"
#include "machine/8255ppi.h"
#include "machine/pit8253.h"
#include "machine/pic8259.h"
#include "machine/msm8251.h"
#include "machine/wd17xx.h"
#include "devices/basicdsk.h"
#include "includes/b2m.h"

    
/* Address maps */
static ADDRESS_MAP_START(b2m_mem, ADDRESS_SPACE_PROGRAM, 8)
	AM_RANGE (0x0000, 0x27ff) AM_RAMBANK(1)
 	AM_RANGE (0x2800, 0x2fff) AM_RAMBANK(2)
 	AM_RANGE (0x3000, 0x6fff) AM_RAMBANK(3)
 	AM_RANGE (0x7000, 0xdfff) AM_RAMBANK(4)
 	AM_RANGE (0xe000, 0xffff) AM_RAMBANK(5)
ADDRESS_MAP_END

static ADDRESS_MAP_START( b2m_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x1f)
	AM_RANGE(0x00, 0x03) AM_DEVREADWRITE(PIT8253, "pit8253", pit8253_r,pit8253_w)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE(PPI8255, "ppi8255_2", ppi8255_r, ppi8255_w)
  	AM_RANGE(0x08, 0x0b) AM_DEVREADWRITE(PPI8255, "ppi8255_1", ppi8255_r, ppi8255_w)
    AM_RANGE(0x0c, 0x0c) AM_READWRITE(b2m_localmachine_r,b2m_localmachine_w) 
	AM_RANGE(0x10, 0x13) AM_READWRITE(b2m_palette_r,b2m_palette_w) 	
	AM_RANGE(0x14, 0x15) AM_DEVREADWRITE(PIC8259, "pic8259", pic8259_r, pic8259_w )
	AM_RANGE(0x18, 0x18) AM_READWRITE(msm8251_data_r,msm8251_data_w)
	AM_RANGE(0x19, 0x19) AM_READWRITE(msm8251_status_r,msm8251_control_w)
  	AM_RANGE(0x1c, 0x1c) AM_READWRITE(wd17xx_status_r,wd17xx_command_w) 
  	AM_RANGE(0x1d, 0x1d) AM_READWRITE(wd17xx_track_r,wd17xx_track_w) 
  	AM_RANGE(0x1e, 0x1e) AM_READWRITE(wd17xx_sector_r,wd17xx_sector_w) 
  	AM_RANGE(0x1f, 0x1f) AM_READWRITE(wd17xx_data_r,wd17xx_data_w) 	
ADDRESS_MAP_END

static ADDRESS_MAP_START( b2m_rom_io, ADDRESS_SPACE_IO, 8 )
	ADDRESS_MAP_GLOBAL_MASK(0x1f)
	AM_RANGE(0x00, 0x03) AM_DEVREADWRITE(PIT8253, "pit8253", pit8253_r,pit8253_w)
	AM_RANGE(0x04, 0x07) AM_DEVREADWRITE(PPI8255, "ppi8255_3", ppi8255_r, ppi8255_w)
  	AM_RANGE(0x08, 0x0b) AM_DEVREADWRITE(PPI8255, "ppi8255_1", ppi8255_r, ppi8255_w)
    AM_RANGE(0x0c, 0x0c) AM_READWRITE(b2m_localmachine_r,b2m_localmachine_w) 
	AM_RANGE(0x10, 0x13) AM_READWRITE(b2m_palette_r,b2m_palette_w) 	
	AM_RANGE(0x14, 0x15) AM_DEVREADWRITE(PIC8259, "pic8259", pic8259_r, pic8259_w )
	AM_RANGE(0x18, 0x18) AM_READWRITE(msm8251_data_r,msm8251_data_w)
	AM_RANGE(0x19, 0x19) AM_READWRITE(msm8251_status_r,msm8251_control_w)
ADDRESS_MAP_END


/* Input ports */
static INPUT_PORTS_START( b2m )
	PORT_START("LINE0")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("U") PORT_CODE(KEYCODE_U)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("A") PORT_CODE(KEYCODE_A)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("T") PORT_CODE(KEYCODE_T)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("L") PORT_CODE(KEYCODE_L)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("W") PORT_CODE(KEYCODE_W)		
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F6") PORT_CODE(KEYCODE_F6)		
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F") PORT_CODE(KEYCODE_F)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F7") PORT_CODE(KEYCODE_F7)
	PORT_START("LINE1")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("J") PORT_CODE(KEYCODE_J)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Y") PORT_CODE(KEYCODE_Y)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("V") PORT_CODE(KEYCODE_V)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("K") PORT_CODE(KEYCODE_K)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("R") PORT_CODE(KEYCODE_R)		
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Q") PORT_CODE(KEYCODE_Q)		
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("B") PORT_CODE(KEYCODE_B)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("[") PORT_CODE(KEYCODE_OPENBRACE)
	PORT_START("LINE2")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("D") PORT_CODE(KEYCODE_D)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F8") PORT_CODE(KEYCODE_F8)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("E") PORT_CODE(KEYCODE_E)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("N") PORT_CODE(KEYCODE_N)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("C") PORT_CODE(KEYCODE_C)		
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("H") PORT_CODE(KEYCODE_H)		
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Z") PORT_CODE(KEYCODE_Z)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("G") PORT_CODE(KEYCODE_G)
	PORT_START("LINE3")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("]") PORT_CODE(KEYCODE_CLOSEBRACE)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("X") PORT_CODE(KEYCODE_X)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("O") PORT_CODE(KEYCODE_O)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F9") PORT_CODE(KEYCODE_F9)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("I") PORT_CODE(KEYCODE_I)		
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("P") PORT_CODE(KEYCODE_P)		
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("S") PORT_CODE(KEYCODE_S)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("M") PORT_CODE(KEYCODE_M)
	PORT_START("LINE4")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("8") PORT_CODE(KEYCODE_8)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("7") PORT_CODE(KEYCODE_7)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("6") PORT_CODE(KEYCODE_6)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("5") PORT_CODE(KEYCODE_5)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("4") PORT_CODE(KEYCODE_4)		
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("3") PORT_CODE(KEYCODE_3)		
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("2") PORT_CODE(KEYCODE_2)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_MINUS)
	PORT_START("LINE5")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(",") PORT_CODE(KEYCODE_COMMA)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(";") PORT_CODE(KEYCODE_COLON)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("=") PORT_CODE(KEYCODE_EQUALS)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("`") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("1") PORT_CODE(KEYCODE_1)		
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("\\") PORT_CODE(KEYCODE_SLASH)		
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("0") PORT_CODE(KEYCODE_0)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("9") PORT_CODE(KEYCODE_9)
	PORT_START("LINE6")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Space") PORT_CODE(KEYCODE_SPACE)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Tab") PORT_CODE(KEYCODE_TAB)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Backspace") PORT_CODE(KEYCODE_BACKSPACE)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("*") PORT_CODE(KEYCODE_TILDE)		
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F12") PORT_CODE(KEYCODE_F12)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("-") PORT_CODE(KEYCODE_TILDE)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Enter") PORT_CODE(KEYCODE_ENTER)
	PORT_START("LINE7")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Rshift") PORT_CODE(KEYCODE_RSHIFT)		
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LAlt") PORT_CODE(KEYCODE_LALT)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LCtrl") PORT_CODE(KEYCODE_LCONTROL) PORT_CODE(KEYCODE_RCONTROL)
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Esc") PORT_CODE(KEYCODE_ESC)		
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("???") PORT_CODE(KEYCODE_TILDE)		
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F11") PORT_CODE(KEYCODE_F11)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("LShift") PORT_CODE(KEYCODE_LSHIFT)
	PORT_START("LINE8")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Home") PORT_CODE(KEYCODE_HOME)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Right") PORT_CODE(KEYCODE_RIGHT)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("??") PORT_CODE(KEYCODE_TILDE)		
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Left") PORT_CODE(KEYCODE_LEFT)		
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PgdN") PORT_CODE(KEYCODE_PGDN)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Down") PORT_CODE(KEYCODE_DOWN)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("end") PORT_CODE(KEYCODE_END)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("ins") PORT_CODE(KEYCODE_INSERT)
	PORT_START("LINE9")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("/") PORT_CODE(KEYCODE_BACKSLASH)
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Del") PORT_CODE(KEYCODE_DEL)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("'") PORT_CODE(KEYCODE_QUOTE)		
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME(".") PORT_CODE(KEYCODE_STOP)		
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("PgUp") PORT_CODE(KEYCODE_PGUP)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("Up") PORT_CODE(KEYCODE_UP)
	PORT_START("LINE10")
		PORT_BIT(0x01, IP_ACTIVE_HIGH, IPT_UNUSED) 
		PORT_BIT(0x02, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x04, IP_ACTIVE_HIGH, IPT_UNUSED)
		PORT_BIT(0x08, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F5") PORT_CODE(KEYCODE_F5)		
		PORT_BIT(0x10, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F4") PORT_CODE(KEYCODE_F4)
		PORT_BIT(0x20, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F3") PORT_CODE(KEYCODE_F3)
		PORT_BIT(0x40, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F2") PORT_CODE(KEYCODE_F2)
		PORT_BIT(0x80, IP_ACTIVE_HIGH, IPT_KEYBOARD) PORT_NAME("F1") PORT_CODE(KEYCODE_F1)
	PORT_START("MONITOR")
		PORT_CONFNAME(0x01, 0x01, "Monitor")
			PORT_CONFSETTING(0x01, "Color")		
			PORT_CONFSETTING(0x00, "B/W")			
INPUT_PORTS_END
 
/* Machine driver */
static MACHINE_DRIVER_START( b2m )
    /* basic machine hardware */
    MDRV_CPU_ADD("main", 8080, 2000000)
    MDRV_CPU_PROGRAM_MAP(b2m_mem, 0) 
    MDRV_CPU_IO_MAP(b2m_io, 0)
    MDRV_CPU_VBLANK_INT("main", b2m_vblank_interrupt)
	                             
    MDRV_MACHINE_START( b2m )
    MDRV_MACHINE_RESET( b2m )
 		
    /* video hardware */    	
	MDRV_SCREEN_ADD("main", RASTER)      	
	MDRV_SCREEN_REFRESH_RATE(50)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */
	MDRV_SCREEN_FORMAT(BITMAP_FORMAT_INDEXED16)
	MDRV_SCREEN_SIZE(384, 256)
	MDRV_SCREEN_VISIBLE_AREA(0, 384-1, 0, 256-1)
	MDRV_PALETTE_LENGTH(4)
	MDRV_PALETTE_INIT(b2m)	
	
	MDRV_DEVICE_ADD( "pit8253", PIT8253 )
	MDRV_DEVICE_CONFIG( b2m_pit8253_intf )

	MDRV_DEVICE_ADD( "ppi8255_1", PPI8255 )
	MDRV_DEVICE_CONFIG( b2m_ppi8255_interface_1 )

	MDRV_DEVICE_ADD( "ppi8255_2", PPI8255 )
	MDRV_DEVICE_CONFIG( b2m_ppi8255_interface_2 )
		    
	MDRV_DEVICE_ADD( "ppi8255_3", PPI8255 )
	MDRV_DEVICE_CONFIG( b2m_ppi8255_interface_3 )

	MDRV_DEVICE_ADD( "pic8259", PIC8259 )
	MDRV_DEVICE_CONFIG( b2m_pic8259_config )
		
	MDRV_VIDEO_START(b2m)
    MDRV_VIDEO_UPDATE(b2m)    	
    
    MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("custom", CUSTOM, 0)
	MDRV_SOUND_CONFIG(b2m_sound_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)	
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( b2mrom )
    MDRV_IMPORT_FROM(b2m)
    MDRV_CPU_MODIFY("main")        
  	MDRV_CPU_IO_MAP(b2m_rom_io, 0)
MACHINE_DRIVER_END
 
 static void b2m_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_LOAD:							info->load = device_load_b2m_floppy; break;

		/* --- the following bits of info are returned as NULL-terminated strings --- */
		case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy(info->s = device_temp_str(), "cpm"); break;

		default:										legacybasicdsk_device_getinfo(devclass, state, info); break;
	}	
}
/* ROM definition */

ROM_START( b2m )
    ROM_REGION( 0x12000, "main", ROMREGION_ERASEFF )
    ROM_LOAD( "b2m.rom", 0x10000, 0x2000, CRC(3f3214d6) SHA1(dd93e7fbabf14d1aed6777fe1ccfe0a3ca8fcaf2) )
ROM_END

ROM_START( b2mrom )
    ROM_REGION( 0x22000, "main", ROMREGION_ERASEFF )
    ROM_LOAD( "bios2.rom",  0x10000, 0x2000, CRC(c22a98b7) SHA1(7de91e653bf4b191ded62cf21532578268e4a2c1) )
    ROM_LOAD( "ramdos.sys", 0x12000, 0x60c0, CRC(91ed6df0) SHA1(4fd040f2647a6b7930c330c75560a035027d0606) )
ROM_END

static SYSTEM_CONFIG_START(b2m)	
 	CONFIG_RAM_DEFAULT(128 * 1024)
 	CONFIG_DEVICE(b2m_floppy_getinfo);
SYSTEM_CONFIG_END

/* Driver */
 
/*    YEAR  NAME   	PARENT  COMPAT  MACHINE 	INPUT   	INIT  	 CONFIG COMPANY 				 FULLNAME   FLAGS */
COMP( 1989, b2m, 	0, 	 	0,		b2m, 		b2m, 		b2m, 	 b2m,  	"BNPO",					 "Bashkiria-2M",	 GAME_NOT_WORKING)
COMP( 1989, b2mrom,	b2m, 	0,		b2mrom,		b2m, 		b2m, 	 b2m,  	"BNPO",					 "Bashkiria-2M ROM-disk",	 GAME_NOT_WORKING)

