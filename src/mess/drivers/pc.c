/***************************************************************************

    drivers/pc.c

Driver file for IBM PC, IBM PC XT, and related machines.

    PC-XT memory map

    00000-9FFFF   RAM
    A0000-AFFFF   NOP       or videoram EGA/VGA
    B0000-B7FFF   videoram  MDA, page #0
    B8000-BFFFF   videoram  CGA and/or MDA page #1, T1T mapped RAM
    C0000-C7FFF   NOP       or ROM EGA/VGA
    C8000-C9FFF   ROM       XT HDC #1
    CA000-CBFFF   ROM       XT HDC #2
    D0000-EFFFF   NOP       or 'adapter RAM'
    F0000-FDFFF   NOP       or ROM Basic + other Extensions
    FE000-FFFFF   ROM

IBM PC 5150
===========

- Intel 8088 at 4.77 MHz derived from a 14.31818 MHz crystal
- Onboard RAM: Revision A mainboard - min. 16KB, max. 64KB
               Revision B mainboard - min. 64KB, max. 256KB
- Total RAM (using 5161 expansion chassis or ISA memory board): 512KB (rev 1 or rev 2 bios) or 640KB (rev 3 bios)
- Graphics: MDA, CGA, or MDA and CGA
- Cassette port
- Five ISA expansion slots (short type)
- Optional 8087 co-processor
- Optional up to 4 (2 internal, 2 external) 160KB single-sided or 360KB double-sided 5 1/4" floppy drives
- Optional 10MB hard disk (using 5161 expansion chassis)
- Optional Game port joystick ISA card (two analog joysticks with 2 buttons each)
- Optional Parallel port ISA card
- Optional Serial port ISA card


IBM PC-JR
=========


IBM PC-XT 5160
==============

- Intel 8088 at 4.77 MHz derived from a 14.31818 MHz crystal
- Onboard RAM: Revision A mainboard - min. 64KB, max. 256KB (4x 64KB banks); could be upgraded to 640KB with a hack)
               Revision B mainboard - min. 256KB, max 640KB (2x 256KB, 2x 64KB banks)
- Total RAM (using 5161 expansion chassis, ISA memory board, or rev B mainboard): 640KB
- Graphics: MDA, CGA, MDA and CGA, EGA, EGA and MDA, or EGA and CGA
- One internal 360KB double-sided 5 1/4" floppy drive
- PC/XT keyboard 'IBM Model F' (BIOS revisions 1 and 2)
- AT/Enhanced keyboard or 'IBM Model M' (BIOS revisions 3 and 4)
- Eight ISA expansion slots (short type)
- Optional 8087 co-processor
- Optional second internal 360KB double-sided 5 1/4" floppy drive, if no internal hard disk
- Optional 'half height' 720KB double density 3 1/2" floppy drive
- Optional 10MB hard disk via 5161 expansion chassis
- Optional 10MB or 20MB Seagate ST-412 hard disk
- Optional up to 2 external 360KB double-sided 5 1/4" floppy drive


Tandy 1000
==========

Tandy 1000 machines are similar to the IBM 5160s with CGA graphics. Tandy
added some additional graphic capabilities similar, but not equal, to
those added for the IBM PC Jr.

Tandy 1000 (8086/8088) variations:
1000				128KB-640KB RAM		4.77 MHz		v01.00.00, v01.01.00
1000A/1000HD		128KB-640KB RAM		4.77 MHz		v01.01.00
1000SX/1000AX		384KB-640KB RAM		7.16/4.77 MHz	v01.02.00
1000EX				256KB-640KB RAM		7.16/4.77 MHz	v01.02.00
1000HX				256KB-640KB RAM		7.16/4.77 MHz	v02.00.00
1000SL/1000PC		384KB-640KB RAM		8.0/4.77 MHz	v01.04.00, v01.04.01, v01.04.02, v02.00.01
1000SL/2			512KB-640KB RAM		8.0/4.77 MHz	v01.04.04
1000RL/1000RL-HD	512KB-768KB RAM		9.44/4.77 MHz	v02.00.00, v02.00.01

Tandy 1000 (80286) variations:
1000TX				640KB-768KB RAM		8.0/4.77 MHz	v01.03.00
1000TL				640KB-768KB RAM		8.0/4.77 MHz	v01.04.00, v01.04.01, v01.04.02
1000TL/2			640KB-768KB RAM		8.0/4.77 MHz	v02.00.00
1000TL/3			640KB-768KB RAM		10.0/5.0 MHz	v02.00.00
1000RLX				512KB-1024KB RAM	10.0/5.0 MHz	v02.00.00
1000RLX-HD			1024MB RAM			10.0/5.0 MHz	v02.00.00

Tandy 1000 (80386) variations:
1000RSX/1000RSX-HD	1M-9M RAM			25.0/8.0 MHz	v01.10.00

IBM PC Jr:

TODO: Which clock signals are available in a PC Jr?
      - What clock is Y1? This eventually gets passed on to the CPU (ZM40?) and some other components by a 8284 (ZM8?).
      - Is the clock attached to the Video Gate Array (ZM36?) exactly 14MHz?

***************************************************************************/


#include "driver.h"
#include "deprecat.h"

#include "machine/8255ppi.h"
#include "machine/ins8250.h"
#include "machine/mc146818.h"
#include "machine/pic8259.h"

#include "machine/pit8253.h"
#include "video/pc_vga.h"
#include "video/pc_cga.h"
#include "video/pc_mda.h"
#include "video/pc_aga.h"
#include "video/pc_t1t.h"
#include "video/pc_video.h"

#include "includes/pc_ide.h"
#include "machine/pc_fdc.h"
#include "machine/pc_joy.h"
#include "machine/pckeybrd.h"
#include "includes/pclpt.h"
#include "audio/sblaster.h"
#include "includes/pc_mouse.h"

#include "includes/europc.h"
#include "includes/tandy1t.h"
#include "includes/amstr_pc.h"
#include "includes/ibmpc.h"

#include "machine/pcshare.h"
#include "includes/pc.h"

#include "machine/pc_hdc.h"
#include "devices/printer.h"
#include "devices/mflopimg.h"
#include "devices/harddriv.h"
#include "devices/cassette.h"
#include "devices/cartslot.h"
#include "formats/pc_dsk.h"

#include "machine/8237dma.h"
#include "sound/sn76496.h"
#include "sound/3812intf.h"

#include "machine/kb_keytro.h"


#define ym3812_StdClock 3579545

/*
  adlib (YM3812/OPL2 chip), part of many many soundcards (soundblaster)
  soundblaster: YM3812 also accessible at 0x228/9 (address jumperable)
  soundblaster pro version 1: 2 YM3812 chips
   at 0x388 both accessed,
   at 0x220/1 left?, 0x222/3 right? (jumperable)
  soundblaster pro version 2: 1 OPL3 chip

  pro audio spectrum +: 2 OPL2
  pro audio spectrum 16: 1 OPL3
 */
#define ADLIB	/* YM3812/OPL2 Chip */
/*
  creative labs game blaster (CMS creative music system)
  2 x saa1099 chips
  also on sound blaster 1.0
  option on sound blaster 1.5

  jumperable? normally 0x220
*/
#define GAMEBLASTER


static READ8_HANDLER( pc_ym3812_0_r )
{
	if ((offset % 1) == 0)
		return ym3812_status_port_0_r(machine, 0);
	else
		return 0x00;
}

static WRITE8_HANDLER( pc_ym3812_0_w )
{
	if ((offset % 1) == 0)
		ym3812_control_port_0_w(machine, 0, data);
	else
		ym3812_write_port_0_w(machine, 0, data);
}

#ifdef UNUSED_FUNCTION
static WRITE16_HANDLER( pc16le_sn76496_0_w ) { write16le_with_write8_handler(sn76496_0_w, machine, offset, data, mem_mask); }
#endif

// IO Expansion, only a little bit for ibm bios self tests
//#define EXP_ON

static ADDRESS_MAP_START( pc8_map, ADDRESS_SPACE_PROGRAM, 8 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK(10)
	AM_RANGE(0xa0000, 0xbffff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_ROM
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xeffff) AM_NOP
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END



static ADDRESS_MAP_START( pc16_map, ADDRESS_SPACE_PROGRAM, 16 )
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK(10)
	AM_RANGE(0xa0000, 0xbffff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_ROM
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xeffff) AM_NOP
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START(pc8_io, ADDRESS_SPACE_IO, 8)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE(DMA8237, "dma8237", dma8237_r, dma8237_w)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE(PIC8259, "pic8259_master", pic8259_r, pic8259_w)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE(PIT8253, "pit8253", pit8253_r, pit8253_w)
	AM_RANGE(0x0060, 0x0063) AM_DEVREADWRITE(PPI8255, "ppi8255", ppi8255_r, ppi8255_w)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE(pc_page_r,			pc_page_w)
	AM_RANGE(0x00a0, 0x00a0) AM_WRITE( pc_nmi_enable_w )
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc_JOY_r,				pc_JOY_w)
#ifdef EXP_ON
	AM_RANGE(0x0210, 0x0217) AM_READWRITE(pc_EXP_r,				pc_EXP_w)
#endif
	AM_RANGE(0x0240, 0x0257) AM_READWRITE(pc_rtc_r,				pc_rtc_w)
	AM_RANGE(0x0278, 0x027b) AM_READWRITE(pc_parallelport2_r,	pc_parallelport2_w)
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE(INS8250, "ins8250_3", ins8250_r, ins8250_w)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE(INS8250, "ins8250_1", ins8250_r, ins8250_w)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc_HDC1_r,			pc_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc_HDC2_r,			pc_HDC2_w)
	AM_RANGE(0x0340, 0x0357) AM_READ(return8_FF) /* anonymous bios should not recogniced realtimeclock */
	AM_RANGE(0x0378, 0x037f) AM_READWRITE(pc_parallelport1_r,	pc_parallelport1_w)
#ifdef ADLIB
	AM_RANGE(0x0388, 0x0389) AM_READWRITE(pc_ym3812_0_r,		pc_ym3812_0_w)
#endif
	AM_RANGE(0x03bc, 0x03be) AM_READWRITE(pc_parallelport0_r,	pc_parallelport0_w)
	AM_RANGE(0x03e8, 0x03ef) AM_DEVREADWRITE(INS8250, "ins8250_2", ins8250_r, ins8250_w)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE(pc_fdc_r,				pc_fdc_w)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE(INS8250, "ins8250_0", ins8250_r, ins8250_w)
ADDRESS_MAP_END


static ADDRESS_MAP_START(pc16_io, ADDRESS_SPACE_IO, 16)
	ADDRESS_MAP_UNMAP_HIGH
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE8(DMA8237, "dma8237", dma8237_r, dma8237_w, 0xffff)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE8(PIC8259, "pic8259_master", pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE8(PIT8253, "pit8253", pit8253_r, pit8253_w, 0xffff)
	AM_RANGE(0x0060, 0x0063) AM_DEVREADWRITE8(PPI8255, "ppi8255", ppi8255_r, ppi8255_w, 0xffff)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE8(pc_page_r,				pc_page_w, 0xffff)
	AM_RANGE(0x00a0, 0x00a1) AM_WRITE8( pc_nmi_enable_w, 0x00ff )
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc16le_JOY_r,				pc16le_JOY_w)
#ifdef EXP_ON
	AM_RANGE(0x0210, 0x0217) AM_READWRITE(pc_EXP_r,					pc_EXP_w)
#endif
	AM_RANGE(0x0240, 0x0257) AM_READWRITE(pc16le_rtc_r,				pc16le_rtc_w)
	AM_RANGE(0x0278, 0x027b) AM_READWRITE(pc16le_parallelport2_r,	pc16le_parallelport2_w)
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE8(INS8250, "ins8250_3", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE8(INS8250, "ins8250_1", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc16le_HDC1_r,			pc16le_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc16le_HDC2_r,			pc16le_HDC2_w)
	AM_RANGE(0x0340, 0x0357) AM_READ(return16_FFFF) /* anonymous bios should not recogniced realtimeclock */
	AM_RANGE(0x0378, 0x037f) AM_READWRITE(pc16le_parallelport1_r,	pc16le_parallelport1_w)
#ifdef ADLIB
	AM_RANGE(0x0388, 0x0389) AM_READWRITE8(pc_ym3812_0_r,			pc_ym3812_0_w, 0xffff)
#endif
	AM_RANGE(0x03bc, 0x03bf) AM_READWRITE(pc16le_parallelport0_r,	pc16le_parallelport0_w)
	AM_RANGE(0x03e8, 0x03ef) AM_DEVREADWRITE8(INS8250, "ins8250_2", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE8(pc_fdc_r,				pc_fdc_w, 0xffff)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE8(INS8250, "ins8250_0", ins8250_r, ins8250_w, 0xffff)
ADDRESS_MAP_END



static ADDRESS_MAP_START( europc_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK(10)
	AM_RANGE(0xa0000, 0xaffff) AM_NOP
	AM_RANGE(0xb0000, 0xbffff) AM_READWRITE(pc_aga_videoram_r, pc_aga_videoram_w) AM_BASE(&videoram) AM_SIZE(&videoram_size)
	AM_RANGE(0xc0000, 0xc7fff) AM_NOP
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xeffff) AM_NOP
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END



static ADDRESS_MAP_START(europc_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE(DMA8237, "dma8237", dma8237_r, dma8237_w)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE(PIC8259, "pic8259_master", pic8259_r, pic8259_w)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE(PIT8253, "pit8253", pit8253_r, pit8253_w)
	AM_RANGE(0x0060, 0x0063) AM_READWRITE(europc_pio_r,			europc_pio_w)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE(pc_page_r,			pc_page_w)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc_JOY_r,				pc_JOY_w)
	AM_RANGE(0x0250, 0x025f) AM_READWRITE(europc_jim_r,			europc_jim_w)
	AM_RANGE(0x0278, 0x027b) AM_READWRITE(pc_parallelport2_r,	pc_parallelport2_w)
	AM_RANGE(0x02e0, 0x02e0) AM_READ     (europc_jim2_r)
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE(INS8250, "ins8250_3", ins8250_r, ins8250_w)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE(INS8250, "ins8250_1", ins8250_r, ins8250_w)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc_HDC1_r,			pc_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc_HDC2_r,			pc_HDC2_w)
	AM_RANGE(0x0378, 0x037b) AM_READWRITE(pc_parallelport1_r,	pc_parallelport1_w)
#ifdef ADLIB
	AM_RANGE(0x0388, 0x0389) AM_READWRITE(pc_ym3812_0_r,		pc_ym3812_0_w)
#endif
//	AM_RANGE(0x03bc, 0x03bf) AM_READWRITE(pc16le_parallelport0_r,   pc16le_parallelport0_w)
	AM_RANGE(0x03e8, 0x03ef) AM_DEVREADWRITE(INS8250, "ins8250_2", ins8250_r, ins8250_w)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE(pc_fdc_r,				pc_fdc_w)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE(INS8250, "ins8250_0", ins8250_r, ins8250_w)
ADDRESS_MAP_END



static ADDRESS_MAP_START(tandy1000_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK(10)
	AM_RANGE(0xa0000, 0xaffff) AM_RAM
	AM_RANGE(0xb0000, 0xb7fff) AM_NOP
	AM_RANGE(0xb8000, 0xbffff) AM_READWRITE(pc_t1t_videoram_r, pc_video_videoram_w)
	AM_RANGE(0xc0000, 0xc7fff) AM_NOP
	AM_RANGE(0xc8000, 0xc9fff) AM_ROM
	AM_RANGE(0xca000, 0xcffff) AM_NOP
	AM_RANGE(0xd0000, 0xeffff) AM_NOP
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END



static ADDRESS_MAP_START(tandy1000_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE(DMA8237, "dma8237", dma8237_r, dma8237_w)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE(PIC8259, "pic8259_master", pic8259_r, pic8259_w)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE(PIT8253, "pit8253", pit8253_r, pit8253_w)
	AM_RANGE(0x0060, 0x0063) AM_READWRITE(tandy1000_pio_r,			tandy1000_pio_w)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE(pc_page_r,				pc_page_w)
	AM_RANGE(0x00c0, 0x00c0) AM_WRITE(								sn76496_0_w)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc_JOY_r,					pc_JOY_w)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE(INS8250, "ins8250_1", ins8250_r, ins8250_w)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc_HDC1_r,				pc_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc_HDC2_r,				pc_HDC2_w)
	AM_RANGE(0x0378, 0x037f) AM_READWRITE(pc_t1t_p37x_r,			pc_t1t_p37x_w)
	AM_RANGE(0x03bc, 0x03be) AM_READWRITE(pc_parallelport0_r,		pc_parallelport0_w)
	AM_RANGE(0x03d0, 0x03df) AM_READWRITE(pc_T1T_r,					pc_T1T_w)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE(pc_fdc_r,					pc_fdc_w)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE(INS8250, "ins8250_0", ins8250_r, ins8250_w)
ADDRESS_MAP_END



static ADDRESS_MAP_START(ibmpcjr_map, ADDRESS_SPACE_PROGRAM, 8 )
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK(10)
	AM_RANGE(0xa0000, 0xaffff) AM_RAM
	AM_RANGE(0xb0000, 0xb7fff) AM_NOP
	AM_RANGE(0xb8000, 0xbffff) AM_READWRITE(pc_t1t_videoram_r, pc_video_videoram_w)
	AM_RANGE(0xc0000, 0xc7fff) AM_NOP
	AM_RANGE(0xc8000, 0xc9fff) AM_ROM
	AM_RANGE(0xca000, 0xcffff) AM_NOP
	AM_RANGE(0xd0000, 0xeffff) AM_ROM
	AM_RANGE(0xf0000, 0xfffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START(ibmpcjr_io, ADDRESS_SPACE_IO, 8)
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE(DMA8237, "dma8237", dma8237_r, dma8237_w)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE(PIC8259, "pic8259_master", pic8259_r, pic8259_w)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE(PIT8253, "pit8253", pit8253_r, pit8253_w)
	AM_RANGE(0x0060, 0x0063) AM_DEVREADWRITE(PPI8255, "ppi8255", ppi8255_r, ppi8255_w)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE(pc_page_r,				pc_page_w)
	AM_RANGE(0x00a0, 0x00a0) AM_READWRITE( pcjr_nmi_enable_r, pc_nmi_enable_w )
	AM_RANGE(0x00c0, 0x00c0) AM_WRITE(								sn76496_0_w)
	AM_RANGE(0x00f0, 0x00f7) AM_READWRITE(pc_fdc_r,					pcjr_fdc_w)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc_JOY_r,					pc_JOY_w)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE(INS8250, "ins8250_1", ins8250_r, ins8250_w)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc_HDC1_r,				pc_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc_HDC2_r,				pc_HDC2_w)
	AM_RANGE(0x0378, 0x037f) AM_READWRITE(pc_t1t_p37x_r,			pc_t1t_p37x_w)
	AM_RANGE(0x03bc, 0x03be) AM_READWRITE(pc_parallelport0_r,		pc_parallelport0_w)
	AM_RANGE(0x03d0, 0x03df) AM_READWRITE(pc_T1T_r,					pc_pcjr_w)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE(INS8250, "ins8250_0", ins8250_r, ins8250_w)
ADDRESS_MAP_END



static ADDRESS_MAP_START(pc200_io, ADDRESS_SPACE_IO, 16)
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE8(DMA8237, "dma8237", dma8237_r, dma8237_w, 0xffff)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE8(PIC8259, "pic8259_master", pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE8(PIT8253, "pit8253", pit8253_r, pit8253_w, 0x00ff)
	AM_RANGE(0x0060, 0x0065) AM_READWRITE(pc1640_16le_port60_r,			pc1640_16le_port60_w)
	AM_RANGE(0x0078, 0x0079) AM_READWRITE(pc1640_16le_mouse_x_r,			pc1640_16le_mouse_x_w)
	AM_RANGE(0x007a, 0x007b) AM_READWRITE(pc1640_16le_mouse_y_r,			pc1640_16le_mouse_y_w)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE8(pc_page_r,					pc_page_w, 0xffff)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc16le_JOY_r,					pc16le_JOY_w)
	AM_RANGE(0x0278, 0x027b) AM_READWRITE(pc16le_parallelport2_r,		pc16le_parallelport2_w)
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE8(INS8250, "ins8250_3", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE8(INS8250, "ins8250_1", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc16le_HDC1_r,				pc16le_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc16le_HDC2_r,				pc16le_HDC2_w)
	AM_RANGE(0x0378, 0x037b) AM_READWRITE(pc200_16le_port378_r,			pc16le_parallelport1_w)
	AM_RANGE(0x03bc, 0x03bf) AM_READWRITE(pc16le_parallelport0_r,		pc16le_parallelport0_w)
	AM_RANGE(0x03e8, 0x03ef) AM_DEVREADWRITE8(INS8250, "ins8250_2", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE8(pc_fdc_r,					pc_fdc_w, 0xffff)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE8(INS8250, "ins8250_0", ins8250_r, ins8250_w, 0xffff)
ADDRESS_MAP_END



static ADDRESS_MAP_START( pc1640_map, ADDRESS_SPACE_PROGRAM, 16 )
	AM_RANGE(0x00000, 0x9ffff) AM_RAMBANK(10)
	AM_RANGE(0xa0000, 0xbffff) AM_NOP
	AM_RANGE(0xc0000, 0xc7fff) AM_ROM
	AM_RANGE(0xc8000, 0xcffff) AM_ROM
	AM_RANGE(0xd0000, 0xfbfff) AM_NOP
	AM_RANGE(0xfc000, 0xfffff) AM_ROM
ADDRESS_MAP_END


static ADDRESS_MAP_START(pc1640_io, ADDRESS_SPACE_IO, 16)
	AM_RANGE(0x0000, 0x000f) AM_DEVREADWRITE8(DMA8237, "dma8237", dma8237_r, dma8237_w, 0xffff)
	AM_RANGE(0x0020, 0x0021) AM_DEVREADWRITE8(PIC8259, "pic8259_master", pic8259_r, pic8259_w, 0xffff)
	AM_RANGE(0x0040, 0x0043) AM_DEVREADWRITE8(PIT8253, "pit8253", pit8253_r, pit8253_w, 0x00ff)
	AM_RANGE(0x0060, 0x0065) AM_READWRITE(pc1640_16le_port60_r,			pc1640_16le_port60_w)
	AM_RANGE(0x0070, 0x0071) AM_READWRITE(mc146818_port16le_r,		mc146818_port16le_w)
	AM_RANGE(0x0078, 0x0079) AM_READWRITE(pc1640_16le_mouse_x_r,	pc1640_16le_mouse_x_w)
	AM_RANGE(0x007a, 0x007b) AM_READWRITE(pc1640_16le_mouse_y_r,	pc1640_16le_mouse_y_w)
	AM_RANGE(0x0080, 0x0087) AM_READWRITE8(pc_page_r,				pc_page_w, 0xffff)
	AM_RANGE(0x0200, 0x0207) AM_READWRITE(pc16le_JOY_r,				pc16le_JOY_w)
	AM_RANGE(0x0278, 0x027b) AM_READWRITE(pc16le_parallelport2_r,		pc16le_parallelport2_w)
	AM_RANGE(0x02e8, 0x02ef) AM_DEVREADWRITE8(INS8250, "ins8250_3", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x02f8, 0x02ff) AM_DEVREADWRITE8(INS8250, "ins8250_1", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x0320, 0x0323) AM_READWRITE(pc16le_HDC1_r,			pc16le_HDC1_w)
	AM_RANGE(0x0324, 0x0327) AM_READWRITE(pc16le_HDC2_r,			pc16le_HDC2_w)
	AM_RANGE(0x0378, 0x037b) AM_READWRITE(pc1640_16le_port378_r,			pc16le_parallelport1_w)
	AM_RANGE(0x03bc, 0x03bf) AM_READWRITE(pc16le_parallelport0_r,		pc16le_parallelport0_w)
	AM_RANGE(0x03e8, 0x03ef) AM_DEVREADWRITE8(INS8250, "ins8250_2", ins8250_r, ins8250_w, 0xffff)
	AM_RANGE(0x03f0, 0x03f7) AM_READWRITE8(pc_fdc_r,				pc_fdc_w, 0xffff)
	AM_RANGE(0x03f8, 0x03ff) AM_DEVREADWRITE8(INS8250, "ins8250_0", ins8250_r, ins8250_w, 0xffff)
ADDRESS_MAP_END



static INPUT_PORTS_START( pcmda )
	PORT_START("IN0") /* IN0 */
	PORT_BIT ( 0x80, 0x80,	 IPT_VBLANK )
	PORT_BIT ( 0x7f, 0x7f,	 IPT_UNUSED )

	PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0xc0, 0x40, "Number of floppy drives")
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_DIPNAME( 0x30, 0x30, "Graphics adapter")
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_DIPNAME( 0x0c, 0x0c, "RAM banks")
	PORT_DIPSETTING(	0x00, "1 - 16/ 64/256K" )
	PORT_DIPSETTING(	0x04, "2 - 32/128/512K" )
	PORT_DIPSETTING(	0x08, "3 - 48/192/576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64/256/640K" )
	PORT_DIPNAME( 0x02, 0x00, "80387 installed")
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x02, DEF_STR(Yes) )
	PORT_DIPNAME( 0x01, 0x01, "Any floppy drive installed")
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x01, DEF_STR(Yes) )

	PORT_START("DSW1") /* IN2 */
	PORT_DIPNAME( 0x80, 0x80, "COM1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x80, DEF_STR(Yes) )
	PORT_DIPNAME( 0x40, 0x40, "COM2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x40, DEF_STR(Yes) )
	PORT_DIPNAME( 0x20, 0x00, "COM3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR(No) )
	PORT_DIPSETTING(	0x20, DEF_STR(Yes) )
	PORT_DIPNAME( 0x10, 0x00, "COM4: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "LPT1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "LPT2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, "LPT3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, "Game port enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )

	PORT_START("DSW2") /* IN3 */
	PORT_DIPNAME( 0xf0, 0x80, "Serial mouse")
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x08, 0x08, "HDC1 (C800:0 port 320-323)")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "HDC2 (CA00:0 port 324-327)")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PORT_INCLUDE( at_keyboard )		/* IN4 - IN11 */
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
INPUT_PORTS_END

static INPUT_PORTS_START( pccga )
	PORT_START("IN0") /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

	PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0xc0, 0x40, "Number of floppy drives")
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_DIPNAME( 0x30, 0x20, "Graphics adapter")
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_DIPNAME( 0x0c, 0x0c, "RAM banks")
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_DIPNAME( 0x02, 0x00, "80387 installed")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x01, "Floppy installed")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )

	PORT_START("DSW1") /* IN2 */
	PORT_DIPNAME( 0x80, 0x80, "COM1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "COM2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "COM3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "COM4: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "LPT1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "LPT2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, "LPT3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, "Game port enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	PORT_START("DSW2") /* IN3 */
	PORT_DIPNAME( 0xf0, 0x80, "Serial mouse")
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x08, 0x08, "HDC1 (C800:0 port 320-323)")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "HDC2 (CA00:0 port 324-327)")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PORT_INCLUDE( pc_keyboard )		/* IN4 - IN11 */
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
	PORT_INCLUDE( pcvideo_cga )
INPUT_PORTS_END

static INPUT_PORTS_START( ibm5150 )
	PORT_START("IN0") /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

	PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0xc0, 0x40, "Number of floppy drives")
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_DIPNAME( 0x30, 0x20, "Graphics adapter")
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_DIPNAME( 0x0c, 0x0c, "RAM banks")
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_DIPNAME( 0x02, 0x00, "80387 installed")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x01, "Floppy installed")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )

	PORT_START("DSW1") /* IN2 */
	PORT_DIPNAME( 0x80, 0x80, "COM1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "COM2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "COM3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "COM4: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "LPT1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "LPT2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, "LPT3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, "Game port enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	PORT_START("DSW2") /* IN3 */
	PORT_DIPNAME( 0xf0, 0x80, "Serial mouse")
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x08, 0x08, "HDC1 (C800:0 port 320-323)")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "HDC2 (CA00:0 port 324-327)")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PORT_INCLUDE( kb_keytronic )		/* IN4 - IN11 */
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
	PORT_INCLUDE( pcvideo_cga )
INPUT_PORTS_END

static INPUT_PORTS_START( europc )
	PORT_START("IN0") /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

	PORT_START("DSW0") /* IN1 */

	PORT_START("DSW1") /* IN2 */
	PORT_DIPNAME( 0x80, 0x80, "COM1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "COM2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "COM3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "COM4: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "LPT1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "LPT2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, "LPT3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, "Game port enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	PORT_START("DSW2") /* IN3 */
	PORT_DIPNAME( 0xf0, 0x80, "Serial mouse")
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x08, 0x08, "HDC1 (C800:0 port 320-323)")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "HDC2 (CA00:0 port 324-327)")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	EUROPC_KEYBOARD

	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
INPUT_PORTS_END

static INPUT_PORTS_START( bondwell )
	PORT_START("IN0") /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

	PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0xc0, 0x40, "Number of floppy drives")
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_DIPNAME( 0x30, 0x20, "Graphics adapter")
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_DIPNAME( 0x0c, 0x0c, "RAM banks")
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_DIPNAME( 0x02, 0x00, "80387 installed")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x01, "Floppy installed")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )

	PORT_START("DSW1") /* IN2 */
	PORT_DIPNAME( 0x80, 0x80, "COM1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "COM2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "COM3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "COM4: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "LPT1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "LPT2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, "LPT3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, "Game port enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	PORT_START("DSW2") /* IN3 */
	PORT_DIPNAME( 0xf0, 0x80, "Serial mouse")
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x08, 0x08, "HDC1 (C800:0 port 320-323)")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "HDC2 (CA00:0 port 324-327)")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, "Turbo Switch" )
	PORT_DIPSETTING(	0x00, "Off (4.77 MHz)" )
	PORT_DIPSETTING(	0x02, "On (12 MHz)" )
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PORT_INCLUDE( at_keyboard )		/* IN4 - IN11 */
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
INPUT_PORTS_END

static INPUT_PORTS_START( xtcga )
	PORT_START("IN0") /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

	PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0xc0, 0x40, "Number of floppy drives")
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_DIPNAME( 0x30, 0x20, "Graphics adapter")
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_DIPNAME( 0x0c, 0x0c, "RAM banks")
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_DIPNAME( 0x02, 0x00, "80387 installed")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x01, "Floppy installed")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )

	PORT_START("DSW1") /* IN2 */
	PORT_DIPNAME( 0x80, 0x80, "COM1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "COM2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "COM3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "COM4: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "LPT1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "LPT2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, "LPT3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, "Game port enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	PORT_START("DSW2") /* IN3 */
	PORT_DIPNAME( 0xf0, 0x80, "Serial mouse")
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x08, 0x08, "HDC1 (C800:0 port 320-323)")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "HDC2 (CA00:0 port 324-327)")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
//	PORT_DIPNAME( 0x02, 0x02, "Turbo Switch" )
//	PORT_DIPSETTING(	0x00, "Off (4.77 MHz)" )
//	PORT_DIPSETTING(	0x02, "On (12 MHz)" )
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PORT_INCLUDE( pc_keyboard )		/* IN4 - IN11 */
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
	PORT_INCLUDE( pcvideo_cga )
INPUT_PORTS_END

static INPUT_PORTS_START( tandy1t )
	PORT_START("IN0") /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

	PORT_START("DSW0") /* IN1 */
	PORT_BIT ( 0xff, 0xff,	 IPT_UNUSED )

	PORT_START("DSW1") /* IN2 */
	PORT_DIPNAME( 0x80, 0x80, "COM1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "COM2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_BIT ( 0x30, 0x00,	 IPT_UNUSED )
	PORT_DIPNAME( 0x08, 0x08, "LPT1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BIT ( 0x06, 0x00,	 IPT_UNUSED )
	PORT_DIPNAME( 0x01, 0x00, "Game port enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	PORT_START("DSW2") /* IN3 */
	PORT_DIPNAME( 0xf0, 0x80, "Serial mouse")
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x08, 0x08, "HDC1 (C800:0 port 320-323)")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "HDC2 (CA00:0 port 324-327)")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PORT_INCLUDE( t1000_keyboard )
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
INPUT_PORTS_END

static INPUT_PORTS_START( pc200 )
	PORT_START("IN0") /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

	PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0x07, 0x07, "Name/Language")
	PORT_DIPSETTING(	0x00, "English/less checks" )
	PORT_DIPSETTING(	0x01, DEF_STR( Italian ) ) //prego attendere
	PORT_DIPSETTING(	0x02, "V.g. v\xC3\xA4nta" )
	PORT_DIPSETTING(	0x03, "Vent et cjeblik" ) // seldom c
	PORT_DIPSETTING(	0x04, DEF_STR( Spanish ) ) //Por favor
	PORT_DIPSETTING(	0x05, DEF_STR( French ) ) //patientez
	PORT_DIPSETTING(	0x06, DEF_STR( German ) ) // bitte warten
	PORT_DIPSETTING(	0x07, DEF_STR( English ) ) // please wait
	PORT_DIPNAME( 0x08, 0x00, "37a 0x40")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x08, "0x08" )
/* 2008-05 FP: This Dip Switch overlaps the next one. 
Since pc200 is anyway NOT_WORKING, I comment out this one */
/*	PORT_DIPNAME( 0x10, 0x00, "37a 0x80")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x10, "0x10" ) */
	PORT_DIPNAME( 0x30, 0x00, "Integrated Graphics Adapter")
	PORT_DIPSETTING(	0x00, "CGA 1" )
	PORT_DIPSETTING(	0x10, "CGA 2" )
	PORT_DIPSETTING(	0x20, "external" )
	PORT_DIPSETTING(	0x30, "MDA" )
	PORT_DIPNAME( 0xc0, 0x80, "Startup Mode")
	PORT_DIPSETTING(	0x00, "external Color 80 Columns" )
	PORT_DIPSETTING(	0x40, "Color 40 Columns" )
	PORT_DIPSETTING(	0x80, "Color 80 Columns" )
	PORT_DIPSETTING(	0xc0, DEF_STR( Mono ) )

	PORT_START("DSW1") /* IN2 */
	PORT_BIT ( 0x80, 0x80,	 IPT_UNUSED ) // com 1 on motherboard
	PORT_DIPNAME( 0x40, 0x40, "COM2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "COM3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "COM4: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "LPT1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BIT ( 0x04, 0x04,	 IPT_UNUSED ) // lpt 1 on motherboard
	PORT_DIPNAME( 0x02, 0x00, "LPT3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, "Game port enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	PORT_START("DSW2") /* IN3 */
	PORT_DIPNAME( 0xf0, 0x00, "Serial mouse")
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x08, 0x08, "HDC1 (C800:0 port 320-323)")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "HDC2 (CA00:0 port 324-327)")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PORT_INCLUDE( at_keyboard )		/* IN4 - IN11 */
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */

	PORT_START("VIDEO") /* IN20 */
	PORT_CONFNAME( 0x03, 0x03, "IDA character set")
	PORT_CONFSETTING(0x00, "Greek")
	PORT_CONFSETTING(0x01, "Norwegian (Codepage 860)")
	PORT_CONFSETTING(0x02, "Portugese (Codepage 865)")
	PORT_CONFSETTING(0x03, "Default (Codepage 437)")
	PORT_CONFNAME( 0x1C, 0x00, "CGA monitor type")
	PORT_CONFSETTING(0x00, "Colour RGB")
	PORT_CONFSETTING(0x04, "Mono RGB")
	PORT_CONFSETTING(0x0C, "Television")
	PORT_BIT ( 0xE0, 0x40, IPT_UNUSED )	/* Chipset is always PPC512 */

INPUT_PORTS_END

static INPUT_PORTS_START( pc1512 )
	PORT_START("IN0") /* IN0 */
	PORT_BIT ( 0xf0, 0xf0,	 IPT_UNUSED )
	PORT_BIT ( 0x08, 0x08,	 IPT_VBLANK )
	PORT_BIT ( 0x07, 0x07,	 IPT_UNUSED )

	PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0x07, 0x07, "Name/Language")
	PORT_DIPSETTING(	0x00, "English/less checks" )
	PORT_DIPSETTING(	0x01, DEF_STR( Italian ) ) //prego attendere
	PORT_DIPSETTING(	0x02, "V.g. v\xC3\xA4nta" )
	PORT_DIPSETTING(	0x03, "Vent et cjeblik" ) // seldom c
	PORT_DIPSETTING(	0x04, DEF_STR( Spanish ) ) //Por favor
	PORT_DIPSETTING(	0x05, DEF_STR( French ) ) //patientez
	PORT_DIPSETTING(	0x06, DEF_STR( German ) ) // bitte warten
	PORT_DIPSETTING(	0x07, DEF_STR( English ) ) // please wait
	PORT_BIT( 0x20, 0x20,	IPT_UNUSED ) // pc1512 integrated special cga
	PORT_BIT( 0xc0, 0x00,	IPT_UNUSED ) // not used in pc1512
	PORT_BIT( 0xe00, 0x00,	IPT_UNUSED ) // not used in pc1512
	PORT_BIT( 0xe000, 0x00,	IPT_UNUSED ) // not used in pc1512

	PORT_START("DSW1") /* IN2 */
	PORT_BIT ( 0x80, 0x80,	 IPT_UNUSED ) // com 1 on motherboard
	PORT_DIPNAME( 0x40, 0x40, "COM2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "COM3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "COM4: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "LPT1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BIT ( 0x04, 0x04,	 IPT_UNUSED ) // lpt 1 on motherboard
	PORT_DIPNAME( 0x02, 0x00, "LPT3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, "Game port enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	PORT_START("DSW2") /* IN3 */
	PORT_DIPNAME( 0xf0, 0x00, "Serial mouse")
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x08, 0x08, "HDC1 (C800:0 port 320-323)")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "HDC2 (CA00:0 port 324-327)")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PORT_INCLUDE( amstrad_keyboard )	/* IN4 - IN14 */

	PORT_INCLUDE( pc_joystick_none )

	PORT_INCLUDE( pcvideo_pc1512 )
INPUT_PORTS_END

static INPUT_PORTS_START( pc1640 )
	PORT_START("IN0")	/* IN0 */
	PORT_DIPNAME( 0x08, 0x08, "VGA 1")
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x00, "VGA 2")
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x00, "VGA 3")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x01, "VGA 4")
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x10, 0x10, "Paradise EGA 5")
	PORT_DIPSETTING(	0x10, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x20, 0x20, "Paradise EGA 6")
	PORT_DIPSETTING(	0x20, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x40, 0x40, "Paradise EGA 7")
	PORT_DIPSETTING(	0x40, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x80, 0x80, "Paradise EGA 8")
	PORT_DIPSETTING(	0x80, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0x07, 0x07, "Name/Language")
//	PORT_DIPSETTING(    0x00, "PC 512k" ) // machine crashes with ega bios at 0xc0000
	PORT_DIPSETTING(	0x01, DEF_STR( Italian ) ) //prego attendere
	PORT_DIPSETTING(	0x02, "V.g. v\xC3\xA4nta" )
	PORT_DIPSETTING(	0x03, "Vent et cjeblik" ) // seldom c
	PORT_DIPSETTING(	0x04, DEF_STR( Spanish ) ) //Por favor
	PORT_DIPSETTING(	0x05, DEF_STR( French ) ) //patientez
	PORT_DIPSETTING(	0x06, DEF_STR( German ) ) // bitte warten
	PORT_DIPSETTING(	0x07, DEF_STR( English ) ) // please wait
	PORT_BIT( 0x20, 0x00,	IPT_UNUSED ) // not pc1512 integrated special cga
	PORT_DIPNAME( 0x40, 0x00, "37a 0x40")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x40, "0x40" )
	PORT_DIPNAME( 0x80, 0x00, "37a 0x80")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x80, "0x80" )
#if 0
	PORT_DIPNAME( 0x200, 0x00, "37a 0x20 after 27[8a] read (font?)")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x200, "0x20" )
	PORT_DIPNAME( 0x400, 0x00, "37a 0x40 after 27[8a] read (font?)")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x400, "0x40" )
	PORT_DIPNAME( 0x800, 0x00, "37a 0x80 after 27[8a] read (font?)")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x800, "0x80" )
	PORT_DIPNAME( 0x2000, 0x00, "37a 0x20 after 427a read")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x2000, "0x20" )
	PORT_DIPNAME( 0x4000, 0x00, " 37a 0x40 after 427a read")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x4000, "0x40" )
	PORT_DIPNAME( 0x8000, 0x00, " 37a 0x80 after 427a read")
	PORT_DIPSETTING(	0x00, "0x00" )
	PORT_DIPSETTING(	0x8000, "0x80" )
#else
	PORT_DIPNAME( 0xe00, 0x600, "37a after 427a read")
	PORT_DIPSETTING(	0x000, "?0standard" ) // diaresis a, seldom c, acut u, circumflex e, grave a, acute e
	PORT_DIPSETTING(	0x200, "?scandinavian" ) //diaresis a, o slashed, acute u, circumflex e, grave a, acute e
	PORT_DIPSETTING(	0x600, "?spain" ) // tilde a, seldom c, acute u, circumflex e, grave a, acute e
	PORT_DIPSETTING(	0xa00, "?greeck" ) // E, circumflex ???,micro, I, Z, big kappa?
	PORT_DIPSETTING(	0xe00, "?standard" ) // diaresis a, seldom e, acute u, circumflex e, grave a, acute e
	PORT_DIPNAME( 0xe000, 0x00, "37a after 427a read")
	PORT_DIPSETTING(	0x0000, "?Integrated Graphic Adapter IGA (EGA)" )
	PORT_DIPSETTING(	0x2000, "?External EGA/VGA" )
	PORT_DIPSETTING(	0x6000, "CGA 40 Columns" )
	PORT_DIPSETTING(	0xa000, "CGA 80 Columns" )
	PORT_DIPSETTING(	0xe000, "MDA/Hercules/Multiple Graphic Adapters" )
#endif

	PORT_START("DSW1") /* IN2 */
	PORT_BIT ( 0x80, 0x80,	 IPT_UNUSED ) // com 1 on motherboard
	PORT_DIPNAME( 0x40, 0x40, "COM2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "COM3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "COM4: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "LPT1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_BIT ( 0x04, 0x04,	 IPT_UNUSED ) // lpt 1 on motherboard
	PORT_DIPNAME( 0x02, 0x00, "LPT3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, "Game port enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	PORT_START("DSW2") /* IN3 */
	PORT_DIPNAME( 0xf0, 0x00, "Serial mouse")
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x08, 0x08, "HDC1 (C800:0 port 320-323)")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "HDC2 (CA00:0 port 324-327)")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_BIT( 0x02, 0x02,	IPT_UNUSED ) /* no turbo switch */
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PORT_INCLUDE( amstrad_keyboard )	/* IN4 - IN14 */

//	PORT_INCLUDE( pc_mouse_microsoft )  /* IN12 - IN14 */

INPUT_PORTS_END

static INPUT_PORTS_START( xtvga )
	PORT_START("IN0") /* IN0 */
	PORT_DIPNAME( 0x08, 0x08, "VGA 1")
	PORT_DIPSETTING(	0x08, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x04, 0x04, "VGA 2")
	PORT_DIPSETTING(	0x04, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x02, 0x02, "VGA 3")
	PORT_DIPSETTING(	0x02, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )
	PORT_DIPNAME( 0x01, 0x00, "VGA 4")
	PORT_DIPSETTING(	0x01, DEF_STR( Off ) )
	PORT_DIPSETTING(	0x00, DEF_STR( On ) )

	PORT_START("DSW0") /* IN1 */
	PORT_DIPNAME( 0xc0, 0x40, "Number of floppy drives")
	PORT_DIPSETTING(	0x00, "1" )
	PORT_DIPSETTING(	0x40, "2" )
	PORT_DIPSETTING(	0x80, "3" )
	PORT_DIPSETTING(	0xc0, "4" )
	PORT_DIPNAME( 0x30, 0x00, "Graphics adapter")
	PORT_DIPSETTING(	0x00, "EGA/VGA" )
	PORT_DIPSETTING(	0x10, "Color 40x25" )
	PORT_DIPSETTING(	0x20, "Color 80x25" )
	PORT_DIPSETTING(	0x30, "Monochrome" )
	PORT_DIPNAME( 0x0c, 0x0c, "RAM banks")
	PORT_DIPSETTING(	0x00, "1 - 16  64 256K" )
	PORT_DIPSETTING(	0x04, "2 - 32 128 512K" )
	PORT_DIPSETTING(	0x08, "3 - 48 192 576K" )
	PORT_DIPSETTING(	0x0c, "4 - 64 256 640K" )
	PORT_DIPNAME( 0x02, 0x00, "80387 installed")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x01, "Floppy installed")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x01, DEF_STR( Yes ) )

	PORT_START("DSW1") /* IN2 */
	PORT_DIPNAME( 0x80, 0x80, "COM1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x80, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x40, 0x40, "COM2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x40, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x20, 0x00, "COM3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x20, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x10, 0x00, "COM4: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x10, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x08, 0x08, "LPT1: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x00, "LPT2: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x00, "LPT3: enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x02, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x01, 0x00, "Game port enable")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(    0x01, DEF_STR( Yes ) )

	PORT_START("DSW2") /* IN3 */
	PORT_DIPNAME( 0xf0, 0x80, "Serial mouse")
	PORT_DIPSETTING(	0x80, "COM1" )
	PORT_DIPSETTING(	0x40, "COM2" )
	PORT_DIPSETTING(	0x20, "COM3" )
	PORT_DIPSETTING(	0x10, "COM4" )
	PORT_DIPSETTING(    0x00, DEF_STR( None ) )
	PORT_DIPNAME( 0x08, 0x08, "HDC1 (C800:0 port 320-323)")
	PORT_DIPSETTING(	0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x08, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x04, 0x04, "HDC2 (CA00:0 port 324-327)")
	PORT_DIPSETTING(    0x00, DEF_STR( No ) )
	PORT_DIPSETTING(	0x04, DEF_STR( Yes ) )
	PORT_DIPNAME( 0x02, 0x02, "Turbo Switch" )
	PORT_DIPSETTING(	0x00, "Off (4.77 MHz)" )
	PORT_DIPSETTING(	0x02, "On (12 MHz)" )
	PORT_BIT( 0x01, 0x01,	IPT_UNUSED )

	PORT_INCLUDE( at_keyboard )		/* IN4 - IN11 */
	PORT_INCLUDE( pc_mouse_microsoft )	/* IN12 - IN14 */
	PORT_INCLUDE( pc_joystick )			/* IN15 - IN19 */
INPUT_PORTS_END

static const unsigned i86_address_mask = 0x000fffff;

#if defined(ADLIB)
/* irq line not connected to pc on adlib cards (and compatibles) */
static void pc_irqhandler(running_machine *machine, int linestate) {}

static const ym3812_interface pc_ym3812_interface =
{
	pc_irqhandler
};
#endif


#define MDRV_CPU_PC(mem, port, type, clock, vblankfunc)	\
	MDRV_CPU_ADD("main", type, clock)				\
	MDRV_CPU_PROGRAM_MAP(mem##_map, 0)			\
	MDRV_CPU_IO_MAP(port##_io, 0)				\
	MDRV_CPU_VBLANK_INT_HACK(vblankfunc, 4)					\
	MDRV_CPU_CONFIG(i86_address_mask)


static MACHINE_DRIVER_START( pcmda )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", V20, 4772720)
	MDRV_CPU_PROGRAM_MAP(pc8_map, 0)
	MDRV_CPU_IO_MAP(pc8_io, 0)
	MDRV_CPU_VBLANK_INT_HACK(pc_frame_interrupt, 4)

	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START(pc)
	MDRV_MACHINE_RESET(pc)

	MDRV_DEVICE_ADD( "pit8253", PIT8253 )
	MDRV_DEVICE_CONFIG( ibm5150_pit8253_config )

	MDRV_DEVICE_ADD( "dma8237", DMA8237 )
	MDRV_DEVICE_CONFIG( ibm5150_dma8237_config )

	MDRV_DEVICE_ADD( "pic8259_master", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_master_config )

	MDRV_DEVICE_ADD( "pic8259_slave", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_slave_config )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( ibm5160_ppi8255_interface )

	MDRV_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MDRV_IMPORT_FROM( pcvideo_mda )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(pc_ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD("saa1099.1", SAA1099, 4772720)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("saa1099.2", SAA1099, 4772720)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
	MDRV_DEVICE_ADD("printer2", PRINTER)
	MDRV_DEVICE_ADD("printer3", PRINTER)

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pcherc )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", V20, 4772720)
	MDRV_CPU_PROGRAM_MAP(pc8_map, 0)
	MDRV_CPU_IO_MAP(pc8_io, 0)
	MDRV_CPU_VBLANK_INT_HACK(pc_frame_interrupt, 4)

	MDRV_MACHINE_START(pc)
	MDRV_MACHINE_RESET(pc)

	MDRV_DEVICE_ADD( "pit8253", PIT8253 )
	MDRV_DEVICE_CONFIG( ibm5150_pit8253_config )

	MDRV_DEVICE_ADD( "dma8237", DMA8237 )
	MDRV_DEVICE_CONFIG( ibm5150_dma8237_config )

	MDRV_DEVICE_ADD( "pic8259_master", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_master_config )

	MDRV_DEVICE_ADD( "pic8259_slave", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_slave_config )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( ibm5160_ppi8255_interface )

	MDRV_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MDRV_IMPORT_FROM( pcvideo_hercules )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(pc_ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD("saa1099.1", SAA1099, 4772720)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("saa1099.2", SAA1099, 4772720)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
	MDRV_DEVICE_ADD("printer2", PRINTER)
	MDRV_DEVICE_ADD("printer3", PRINTER)

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( ibm5150 )
	/* basic machine hardware */
	MDRV_CPU_ADD("main", I8088, XTAL_14_31818MHz/3)
	MDRV_CPU_PROGRAM_MAP(pc8_map, 0)
	MDRV_CPU_IO_MAP(pc8_io, 0)
	MDRV_CPU_CONFIG(i86_address_mask)

	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START(pc)
	MDRV_MACHINE_RESET(pc)

	MDRV_DEVICE_ADD( "pit8253", PIT8253 )
	MDRV_DEVICE_CONFIG( ibm5150_pit8253_config )

	MDRV_DEVICE_ADD( "dma8237", DMA8237 )
	MDRV_DEVICE_CONFIG( ibm5150_dma8237_config )

	MDRV_DEVICE_ADD( "pic8259_master", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_master_config )

	MDRV_DEVICE_ADD( "pic8259_slave", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_slave_config )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( ibm5150_ppi8255_interface )

	MDRV_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MDRV_IMPORT_FROM( pcvideo_cga )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(pc_ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD("saa1099.1", SAA1099, 4772720)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("saa1099.2", SAA1099, 4772720)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif

	MDRV_IMPORT_FROM( kb_keytronic )

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
	MDRV_DEVICE_ADD("printer2", PRINTER)
	MDRV_DEVICE_ADD("printer3", PRINTER)

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )
MACHINE_DRIVER_END

static MACHINE_DRIVER_START( pccga )
	/* basic machine hardware */
	MDRV_CPU_PC(pc8, pc8, I8088, 4772720, pc_frame_interrupt)	/* 4,77 Mhz */

	MDRV_INTERLEAVE(1)

	MDRV_MACHINE_START(pc)
	MDRV_MACHINE_RESET(pc)

	MDRV_DEVICE_ADD( "pit8253", PIT8253 )
	MDRV_DEVICE_CONFIG( ibm5150_pit8253_config )

	MDRV_DEVICE_ADD( "dma8237", DMA8237 )
	MDRV_DEVICE_CONFIG( ibm5150_dma8237_config )

	MDRV_DEVICE_ADD( "pic8259_master", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_master_config )

	MDRV_DEVICE_ADD( "pic8259_slave", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_slave_config )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( ibm5160_ppi8255_interface )

	MDRV_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MDRV_IMPORT_FROM( pcvideo_cga )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(pc_ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD("saa1099.1", SAA1099, 4772720)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("saa1099.2", SAA1099, 4772720)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif

//	MDRV_IMPORT_FROM( kb_keytronic )

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
	MDRV_DEVICE_ADD("printer2", PRINTER)
	MDRV_DEVICE_ADD("printer3", PRINTER)

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( europc )
	/* basic machine hardware */
	MDRV_CPU_PC(europc, europc, I8088, 4772720*2, pc_frame_interrupt)

	MDRV_MACHINE_START(pc)
	MDRV_MACHINE_RESET(pc)

	MDRV_DEVICE_ADD( "pit8253", PIT8253 )
	MDRV_DEVICE_CONFIG( ibm5150_pit8253_config )

	MDRV_DEVICE_ADD( "dma8237", DMA8237 )
	MDRV_DEVICE_CONFIG( ibm5150_dma8237_config )

	MDRV_DEVICE_ADD( "pic8259_master", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_master_config )

	MDRV_DEVICE_ADD( "pic8259_slave", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_slave_config )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( ibm5150_ppi8255_interface )

	MDRV_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MDRV_IMPORT_FROM( pcvideo_aga )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(pc_ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif

	MDRV_NVRAM_HANDLER( europc_rtc )

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
	MDRV_DEVICE_ADD("printer2", PRINTER)
	MDRV_DEVICE_ADD("printer3", PRINTER)

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ibm5160 )
	/* basic machine hardware */
	MDRV_CPU_PC(pc8, pc8, I8088, XTAL_14_31818MHz/3, pc_frame_interrupt)

	MDRV_MACHINE_START(pc)
	MDRV_MACHINE_RESET(pc)

	MDRV_DEVICE_ADD( "pit8253", PIT8253 )
	MDRV_DEVICE_CONFIG( ibm5150_pit8253_config )

	MDRV_DEVICE_ADD( "dma8237", DMA8237 )
	MDRV_DEVICE_CONFIG( ibm5150_dma8237_config )

	MDRV_DEVICE_ADD( "pic8259_master", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_master_config )

	MDRV_DEVICE_ADD( "pic8259_slave", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_slave_config )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( ibm5160_ppi8255_interface )

	MDRV_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MDRV_IMPORT_FROM( pcvideo_cga )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(pc_ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD("saa1099.1", SAA1099, 4772720)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("saa1099.2", SAA1099, 4772720)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif

//	MDRV_IMPORT_FROM( kb_keytronic )

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
	MDRV_DEVICE_ADD("printer2", PRINTER)
	MDRV_DEVICE_ADD("printer3", PRINTER)

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pc200 )
	/* basic machine hardware */
	MDRV_CPU_PC(pc1640, pc200, I8086, 8000000, pc_frame_interrupt)

	MDRV_MACHINE_START(pc)
	MDRV_MACHINE_RESET(pc)

	MDRV_DEVICE_ADD( "pit8253", PIT8253 )
	MDRV_DEVICE_CONFIG( ibm5150_pit8253_config )

	MDRV_DEVICE_ADD( "dma8237", DMA8237 )
	MDRV_DEVICE_CONFIG( ibm5150_dma8237_config )

	MDRV_DEVICE_ADD( "pic8259_master", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_master_config )

	MDRV_DEVICE_ADD( "pic8259_slave", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_slave_config )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( ibm5150_ppi8255_interface )

	MDRV_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MDRV_IMPORT_FROM( pcvideo_pc200 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
	MDRV_DEVICE_ADD("printer2", PRINTER)
	MDRV_DEVICE_ADD("printer3", PRINTER)

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pc1512 )
	/* basic machine hardware */
	MDRV_CPU_PC(pc1640, pc1640, I8086, 8000000, pc_frame_interrupt)

	MDRV_MACHINE_START(pc)
	MDRV_MACHINE_RESET(pc)

	MDRV_DEVICE_ADD( "pit8253", PIT8253 )
	MDRV_DEVICE_CONFIG( ibm5150_pit8253_config )

	MDRV_DEVICE_ADD( "dma8237", DMA8237 )
	MDRV_DEVICE_CONFIG( ibm5150_dma8237_config )

	MDRV_DEVICE_ADD( "pic8259_master", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_master_config )

	MDRV_DEVICE_ADD( "pic8259_slave", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_slave_config )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( ibm5150_ppi8255_interface )

	MDRV_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MDRV_IMPORT_FROM( pcvideo_pc1512 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_NVRAM_HANDLER( mc146818 )

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
	MDRV_DEVICE_ADD("printer2", PRINTER)
	MDRV_DEVICE_ADD("printer3", PRINTER)

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( pc1640 )
	/* basic machine hardware */
	MDRV_CPU_PC(pc1640, pc1640, I8086, 8000000, pc_vga_frame_interrupt)

	MDRV_MACHINE_START(pc)
	MDRV_MACHINE_RESET(pc)

	MDRV_DEVICE_ADD( "pit8253", PIT8253 )
	MDRV_DEVICE_CONFIG( ibm5150_pit8253_config )

	MDRV_DEVICE_ADD( "dma8237", DMA8237 )
	MDRV_DEVICE_CONFIG( ibm5150_dma8237_config )

	MDRV_DEVICE_ADD( "pic8259_master", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_master_config )

	MDRV_DEVICE_ADD( "pic8259_slave", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_slave_config )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( ibm5150_ppi8255_interface )

	MDRV_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MDRV_IMPORT_FROM(pcvideo_pc1640)
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_NVRAM_HANDLER( mc146818 )

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
	MDRV_DEVICE_ADD("printer2", PRINTER)
	MDRV_DEVICE_ADD("printer3", PRINTER)

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( xtvga )
	/* basic machine hardware */
	MDRV_CPU_PC(pc16, pc16, I8086, 12000000, pc_vga_frame_interrupt)

	MDRV_MACHINE_START(pc)
	MDRV_MACHINE_RESET(pc)

	MDRV_DEVICE_ADD( "pit8253", PIT8253 )
	MDRV_DEVICE_CONFIG( ibm5150_pit8253_config )

	MDRV_DEVICE_ADD( "dma8237", DMA8237 )
	MDRV_DEVICE_CONFIG( ibm5150_dma8237_config )

	MDRV_DEVICE_ADD( "pic8259_master", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_master_config )

	MDRV_DEVICE_ADD( "pic8259_slave", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_slave_config )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( ibm5150_ppi8255_interface )

	MDRV_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_2", ibm5150_com_interface[2] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_3", ibm5150_com_interface[3] )			/* TODO: Verify model */

	/* video hardware */
	MDRV_IMPORT_FROM( pcvideo_vga )
	MDRV_SCREEN_MODIFY("main")
	MDRV_SCREEN_REFRESH_RATE(60)
	MDRV_SCREEN_VBLANK_TIME(ATTOSECONDS_IN_USEC(2500)) /* not accurate */

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#ifdef ADLIB
	MDRV_SOUND_ADD("ym3812", YM3812, ym3812_StdClock)
	MDRV_SOUND_CONFIG(pc_ym3812_interface)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
#endif
#ifdef GAMEBLASTER
	MDRV_SOUND_ADD("saa1099.1", SAA1099, 4772720)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
	MDRV_SOUND_ADD("saa1099.2", SAA1099, 4772720)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 0.50)
#endif

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( t1000hx )
	/* basic machine hardware */
	MDRV_CPU_PC(tandy1000, tandy1000, I8088, 8000000, pc_frame_interrupt)

	MDRV_MACHINE_START(pc)
	MDRV_MACHINE_RESET(pc)

	MDRV_DEVICE_ADD( "pit8253", PIT8253 )
	MDRV_DEVICE_CONFIG( ibm5150_pit8253_config )

	MDRV_DEVICE_ADD( "dma8237", DMA8237 )
	MDRV_DEVICE_CONFIG( ibm5150_dma8237_config )

	MDRV_DEVICE_ADD( "pic8259_master", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_master_config )

	MDRV_DEVICE_ADD( "pic8259_slave", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_slave_config )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( ibm5150_ppi8255_interface )

	MDRV_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */

	/* video hardware */
	MDRV_IMPORT_FROM( pcvideo_t1000 )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MDRV_SOUND_ADD("sn76496", SN76496, 2386360)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_NVRAM_HANDLER( tandy1000 )

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
	MDRV_DEVICE_ADD("printer2", PRINTER)
	MDRV_DEVICE_ADD("printer3", PRINTER)

	/* harddisk */
	MDRV_IMPORT_FROM( pc_hdc )
MACHINE_DRIVER_END


static MACHINE_DRIVER_START( ibmpcjr )
	/* basic machine hardware */
	MDRV_CPU_PC(ibmpcjr, ibmpcjr, I8088, 4900000, pcjr_frame_interrupt)	/* TODO: Get correct cpu frequency, probably XTAL_14_31818MHz/3 */

	MDRV_MACHINE_START(pc)
	MDRV_MACHINE_RESET(pcjr)

	MDRV_DEVICE_ADD( "pit8253", PIT8253 )
	MDRV_DEVICE_CONFIG( pcjr_pit8253_config )

	MDRV_DEVICE_ADD( "dma8237", DMA8237 )
	MDRV_DEVICE_CONFIG( ibm5150_dma8237_config )

	MDRV_DEVICE_ADD( "pic8259_master", PIC8259 )
	MDRV_DEVICE_CONFIG( pcjr_pic8259_master_config )

	MDRV_DEVICE_ADD( "pic8259_slave", PIC8259 )
	MDRV_DEVICE_CONFIG( ibm5150_pic8259_slave_config )

	MDRV_DEVICE_ADD( "ppi8255", PPI8255 )
	MDRV_DEVICE_CONFIG( pcjr_ppi8255_interface )

	MDRV_INS8250_ADD( "ins8250_0", ibm5150_com_interface[0] )			/* TODO: Verify model */
	MDRV_INS8250_ADD( "ins8250_1", ibm5150_com_interface[1] )			/* TODO: Verify model */

	/* video hardware */
	MDRV_IMPORT_FROM( pcvideo_pcjr )

	/* sound hardware */
	MDRV_SPEAKER_STANDARD_MONO("mono")
	MDRV_SOUND_ADD("speaker", SPEAKER, 0)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)
	MDRV_SOUND_ADD("sn76496", SN76496, 2386360)
	MDRV_SOUND_ROUTE(ALL_OUTPUTS, "mono", 1.00)

	MDRV_NVRAM_HANDLER( tandy1000 )

	/* printer */
	MDRV_DEVICE_ADD("printer", PRINTER)
	MDRV_DEVICE_ADD("printer2", PRINTER)
	MDRV_DEVICE_ADD("printer3", PRINTER)
MACHINE_DRIVER_END

#if 0
	//pcjr roms? (incomplete dump, most likely 64 kbyte)
	// basic c1.20
    ROM_LOAD("basic.rom", 0xf6000, 0x8000, CRC(0c19c1a8))
	// ???
    ROM_LOAD("bios.rom", 0x??000, 0x2000, CRC(98463f95))

	/* turbo xt */
	/* basic c1.10 */
    ROM_LOAD("rom05.bin", 0xf6000, 0x2000, CRC(80d3cf5d))
    ROM_LOAD("rom04.bin", 0xf8000, 0x2000, CRC(673a4acc))
    ROM_LOAD("rom03.bin", 0xfa000, 0x2000, CRC(aac3fc37))
    ROM_LOAD("rom02.bin", 0xfc000, 0x2000, CRC(3062b3fc))
	/* sw1 0x60 readback fails write 301 to screen fe3b7 */
	/* disk problems no disk gives 601 */
	/* 5000-026 08/16/82 */
    ROM_LOAD("rom01.bin", 0xfe000, 0x2000, CRC(5c3f0256))

	/* anonymous works nice */
    ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, CRC(031aafad))

	ROM_LOAD("bondwell.bin", 0xfe000, 0x2000, CRC(d435a405))

	/* europc */
    ROM_LOAD("50145", 0xf8000, 0x8000, CRC(1775a11d)) // V2.07
//    ROM_LOAD("eurobios.bin", 0xf8000, 0x8000, CRC(52185223)) scrap
	/* cga, hercules character set */
    ROM_LOAD("50146", 0x00000, 0x02000, CRC(1305dcf5)) //D1.0

	// ibm pc
	// most likely 8 kbyte chips
    ROM_LOAD("basicpc.bin", 0xf6000, 0x8000, CRC(ebacb791)) // IBM C1.1
	// split into 8 kbyte parts
	// the same as in the basic c1.10 as in the turboxt
	// 1501-476 10/27/82
    ROM_LOAD("biospc.bin", 0xfe000, 0x2000, CRC(e88792b3))

	/* tandy 1000 hx */
    ROM_LOAD("tandy1t.rom", 0xf0000, 0x10000, CRC(d37a1d5f))

	// ibm xt
    ROM_LOAD("xthdd.c8", 0xc8000, 0x2000, CRC(a96317da))
    ROM_LOAD("biosxt.bin", 0xf0000, 0x10000, CRC(36c32fde)) // BASIC C1.1
	// split into 2 chips for 16 bit access
    ROM_LOAD_EVEN("ibmxt.0", 0xf0000, 0x8000, CRC(83727c42))
    ROM_LOAD_ODD("ibmxt.1", 0xf0000, 0x8000, CRC(2a629953))

	/* pc xt mfm controller
       2 harddisks 17 sectors, 4 head, 613 tracks
       serves 2 controllers? 0x320-3, 0x324-7, dma 3, irq5
       movable, works at 0xee000 */
	/* western digital 06/28/89 */
    ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4))

	/* lcs 6210d asic i2.1 09/01/1988 */
	/* problematic, currently showing menu and calls int21 (hangs)! */
    ROM_LOAD("xthdd.rom",  0xc8000, 0x02000, CRC(a96317da))
#endif

ROM_START( ibm5150 )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) /* WDC Expansion ROM C8000-C9FFF */
//	ROM_LOAD("600963.u12", 0xc8000, 0x02000, CRC(f3daf85f) SHA1(3bd29538832d3084cbddeec92593988772755283))	/* Tandon/Western Digital Fixed Disk Adapter 600963-001__TYPE_5.U12.2764.bin */
//	ROM_LOAD("62x0822.12d", 0xc8000, 0x02000, CRC(4cdd2193) SHA1(fe8f88333b5e13e170bf637a9a0090383dee454d))	/* Xebec Fixed Disk Adapter 62X0822__(M)_AMI_8621MAB__S68B364-P__(C)IBM_CORP_1982,1985__PHILIPPINES.12D.2364.bin */

	/* IBM PC 5150 (rev 3: 1501-476 10/27/82) 5-screw case w/1501981 CGA Card, ROM Basic 1.1 */
	ROM_SYSTEM_BIOS( 0, "default", "IBM PC 5150 1501471 10/27/82" )
	ROMX_LOAD("5000019.u29", 0xf6000, 0x2000, CRC(80d3cf5d) SHA1(64769b7a8b60ffeefa04e4afbec778069a2840c9), ROM_BIOS(1))		/* ROM Basic 1.1 F6000-F7FFF; IBM P/N: 5000019, FRU: 6359109 */
	ROMX_LOAD("5000021.u30", 0xf8000, 0x2000, CRC(673a4acc) SHA1(082ae803994048e225150f771794ca305f73d731), ROM_BIOS(1))		/* ROM Basic 1.1 F8000-F9FFF; IBM P/N: 5000021, FRU: 6359111 */
	ROMX_LOAD("5000022.u31", 0xfa000, 0x2000, CRC(aac3fc37) SHA1(c9e0529470edf04da093bb8c8ae2536c688c1a74), ROM_BIOS(1))		/* ROM Basic 1.1 FA000-FBFFF; IBM P/N: 5000022, FRU: 6359112 */
	ROMX_LOAD("5000023.u32", 0xfc000, 0x2000, CRC(3062b3fc) SHA1(5134dd64721cbf093d059ee5d3fd09c7f86604c7), ROM_BIOS(1))		/* ROM Basic 1.1 FC000-FDFFF; IBM P/N: 5000023, FRU: 6359113 */
	ROMX_LOAD("1501476.u33", 0xfe000, 0x2000, CRC(e88792b3) SHA1(40fce6a94dda4328a8b608c7ae2f39d1dc688af4), ROM_BIOS(1))

	/* IBM PC 5150 (rev 1: 04/24/81) 2-screw case w/MDA Card, ROM Basic 1.0 */
	/* ROM Basic 1.0 had a bug: Doing ".1 / 10" would result in the wrong answer. May have been fixed in 1.1 */
	ROM_SYSTEM_BIOS( 1, "rev1", "IBM PC 5150 ??????? 04/24/81" )
	ROMX_LOAD("5700019.u29", 0xf6000, 0x2000, CRC(b59e8f6c) SHA1(7a5db95370194c73b7921f2d69267268c69d2511), ROM_BIOS(2))		/* ROM Basic 1.0 F6000-F7FFF */
	ROMX_LOAD("5700027.u30", 0xf8000, 0x2000, CRC(bfff99b8) SHA1(ca2f126ba69c1613b7b5a4137d8d8cf1db36a8e6), ROM_BIOS(2))		/* ROM Basic 1.0 F8000-F9FFF */
	ROMX_LOAD("5700035.u31", 0xfa000, 0x2000, CRC(9fe4ec11) SHA1(89af8138185938c3da3386f97d3b0549a51de5ef), ROM_BIOS(2))		/* ROM Basic 1.0 FA000-FBFFF */
	ROMX_LOAD("5700043.u32", 0xfc000, 0x2000, CRC(ea2794e6) SHA1(22fe58bc853ffd393d5e2f98defda7456924b04f), ROM_BIOS(2))		/* ROM Basic 1.0 FC000-FDFFF */
	ROMX_LOAD("5700051.u33", 0xfe000, 0x2000, NO_DUMP, ROM_BIOS(2))

	/* IBM PC 5150 (rev 2: 10/19/81) 2-screw case w/MDA Card, ROM Basic 1.0 */
	ROM_SYSTEM_BIOS( 2, "rev2", "IBM PC 5150 5700671 10/19/81" )
	ROMX_LOAD("5700019.u29", 0xf6000, 0x2000, CRC(b59e8f6c) SHA1(7a5db95370194c73b7921f2d69267268c69d2511), ROM_BIOS(3))		/* ROM Basic 1.0 F6000-F7FFF */
	ROMX_LOAD("5700027.u30", 0xf8000, 0x2000, CRC(bfff99b8) SHA1(ca2f126ba69c1613b7b5a4137d8d8cf1db36a8e6), ROM_BIOS(3))		/* ROM Basic 1.0 F8000-F9FFF */
	ROMX_LOAD("5700035.u31", 0xfa000, 0x2000, CRC(9fe4ec11) SHA1(89af8138185938c3da3386f97d3b0549a51de5ef), ROM_BIOS(3))		/* ROM Basic 1.0 FA000-FBFFF */
	ROMX_LOAD("5700043.u32", 0xfc000, 0x2000, CRC(ea2794e6) SHA1(22fe58bc853ffd393d5e2f98defda7456924b04f), ROM_BIOS(3))		/* ROM Basic 1.0 FC000-FDFFF */
	ROMX_LOAD("5700671.u33", 0xfe000, 0x2000, CRC(b7d4ec46) SHA1(bdb06f846c4768f39eeff7e16b6dbff8cd2117d2), ROM_BIOS(3))

	/* Z80 on the Xebec Hard Disk Controller */
//	ROM_REGION(0x10000, "cpu1", 0)
//	ROM_LOAD("104839re.12a", 0x0000, 0x1000, CRC(3ad32fcc) SHA1(0127fa520aaee91285cb46a640ed835b4554e4b3))	/* Xebec Hard Disk Controller 104839RE__COPYRIGHT__XEBEC_1986.12A.2732.bin */

	/* Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f))

	/* 8051 keytronic keyboard controller */
	ROM_REGION( 0x2000, KEYTRONIC_KB3270PC_CPU, 0 )
	ROM_LOAD("14166.bin", 0x0000, 0x2000, CRC(1aea1b53) SHA1(b75b6d4509036406052157bc34159f7039cdc72e))
ROM_END

ROM_START( ibmpca )
	ROM_REGION(0x100000,"main",0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))	/* WDC Expansion ROM C8000-C9FFF */
	ROM_LOAD("basicc11.f6", 0xf6000, 0x2000, CRC(80d3cf5d) SHA1(64769b7a8b60ffeefa04e4afbec778069a2840c9))
	ROM_LOAD("basicc11.f8", 0xf8000, 0x2000, CRC(673a4acc) SHA1(082ae803994048e225150f771794ca305f73d731))
	ROM_LOAD("basicc11.fa", 0xfa000, 0x2000, CRC(aac3fc37) SHA1(c9e0529470edf04da093bb8c8ae2536c688c1a74))
	ROM_LOAD("basicc11.fc", 0xfc000, 0x2000, CRC(3062b3fc) SHA1(5134dd64721cbf093d059ee5d3fd09c7f86604c7))
	ROM_LOAD("pc081682.bin", 0xfe000, 0x2000, CRC(5c3f0256) SHA1(b42c78abd0a9c630a2f972ad2bae46d83c3a2a09))

	/* Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f))
ROM_END

ROM_START( bondwell )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a)) // taken from other machine
	ROM_LOAD("bondwell.bin", 0xfe000, 0x2000, CRC(d435a405) SHA1(a57c705d1144c7b61940b6f5c05d785c272fc9bb))

	/* Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f))
ROM_END

ROM_START( pcmda )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))	/* WDC Expansion ROM C8000-C9FFF */
	ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, CRC(031aafad) SHA1(a641b505bbac97b8775f91fe9b83d9afdf4d038f))
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("mda.rom",     0x00000, 0x02000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f)) // taken from original IBM MDA
ROM_END

ROM_START( pcherc )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))	/* WDC Expansion ROM C8000-C9FFF */
	ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, CRC(031aafad) SHA1(a641b505bbac97b8775f91fe9b83d9afdf4d038f))
	ROM_REGION(0x1000,"gfx1", 0)
	ROM_LOAD("um2301.bin",  0x00000, 0x1000, CRC(0827bdac) SHA1(15f1aceeee8b31f0d860ff420643e3c7f29b5ffc))
ROM_END


ROM_START( pc )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))	/* WDC Expansion ROM C8000-C9FFF */
//	ROM_LOAD("xthdd.rom",  0xc8000, 0x02000, CRC(a96317da))
	ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, CRC(031aafad) SHA1(a641b505bbac97b8775f91fe9b83d9afdf4d038f))

	/* Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f))
ROM_END

ROM_START( europc )
	ROM_REGION(0x100000,"main", 0)
	// hdd bios integrated!
	ROM_LOAD("50145", 0xf8000, 0x8000, CRC(1775a11d) SHA1(54430d4d0462860860397487c9c109e6f70db8e3)) // V2.07
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("50146", 0x00000, 0x02000, CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //D1.0
ROM_END


ROM_START( ibmpcjr )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("bios.rom", 0xf0000, 0x10000,CRC(31e3a7aa) SHA1(1f5f7013f18c08ff50d7942e76c4fbd782412414))

	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("cga.chr",     0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END

#ifdef UNUSED_DEFINITION
ROM_START( t1000 )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))	// not sure about this one
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, CRC(61dbf242) SHA1(555b58d8aa8e0b0839259621c44b832d993beaef))	// not sure about this one
	ROM_SYSTEM_BIOS( 0, "v010000", "v010000" )
	ROMX_LOAD("v010000.f0", 0xf0000, 0x10000, NO_DUMP, ROM_BIOS(1))
	ROM_SYSTEM_BIOS( 1, "v010100", "v010100" )
	ROMX_LOAD("v010100.f0", 0xf0000, 0x10000, CRC(b6760881) SHA1(8275e4c48ac09cf36685db227434ca438aebe0b9), ROM_BIOS(2))
	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END

ROM_START( t1000a )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))	// not sure about this one
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, CRC(61dbf242) SHA1(555b58d8aa8e0b0839259621c44b832d993beaef))	// not sure about this one
	ROM_LOAD("v010100.f0", 0xf0000, 0x10000, CRC(b6760881) SHA1(8275e4c48ac09cf36685db227434ca438aebe0b9))
	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END

ROM_START( t1000ex )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))	// not sure about this one
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, CRC(61dbf242) SHA1(555b58d8aa8e0b0839259621c44b832d993beaef))	// not sure about this one
	ROM_LOAD("v010200.f0", 0xf0000, 0x10000, CRC(0e016ecf) SHA1(2f5ac8921b7cba56b02122ef772f5f11bbf6d8a2))
	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END
#endif

ROM_START( t1000hx )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, CRC(61dbf242) SHA1(555b58d8aa8e0b0839259621c44b832d993beaef))
	ROM_LOAD("v020000.f0", 0xf0000, 0x10000, CRC(d37a1d5f) SHA1(5ec031c31a7967cc3fd53a535d81833e4a1c385e))
	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END

#ifdef UNUSED_DEFINITION
ROM_START( t1000sl )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))	// not sure about this one
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, CRC(61dbf242) SHA1(555b58d8aa8e0b0839259621c44b832d993beaef))	// not sure about this one
	ROM_SYSTEM_BIOS( 0, "v010400", "v010400" )
	ROMX_LOAD("v010400.f0", 0xf0000, 0x10000, NO_DUMP, ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v010401", "v010401" )
	ROMX_LOAD("v010401.f0", 0xf0000, 0x10000, NO_DUMP, ROM_BIOS(2) )
	ROM_SYSTEM_BIOS( 2, "v010402", "v010402" )
	ROMX_LOAD("v010402.f0", 0xf0000, 0x10000, NO_DUMP, ROM_BIOS(3) )
	ROM_SYSTEM_BIOS( 3, "v020001", "v020001" )
	ROMX_LOAD("v020001.f0", 0xf0000, 0x10000, NO_DUMP, ROM_BIOS(4) )
	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END

ROM_START( t1000sl2 )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))	// not sure about this one
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, CRC(61dbf242) SHA1(555b58d8aa8e0b0839259621c44b832d993beaef))	// not sure about this one
	ROM_LOAD("v010404.f0", 0xf0000, 0x10000, NO_DUMP )
	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END
#endif

ROM_START( t1000sx )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))	// not sure about this one
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, CRC(61dbf242) SHA1(555b58d8aa8e0b0839259621c44b832d993beaef))	// not sure about this one
	ROM_LOAD("v010200.f0", 0xf0000, 0x10000, CRC(0e016ecf) SHA1(2f5ac8921b7cba56b02122ef772f5f11bbf6d8a2))
	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END

#ifdef UNUSED_DEFINITION
ROM_START( t1000rl )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))	// not sure about this one
	// partlist says it has 1 128kbyte rom
	ROM_LOAD("t1000hx.e0", 0xe0000, 0x10000, CRC(61dbf242) SHA1(555b58d8aa8e0b0839259621c44b832d993beaef))	// not sure about this one
	ROM_SYSTEM_BIOS( 0, "v020000", "v020000" )
	ROMX_LOAD("v020000.f0", 0xf0000, 0x10000, CRC(d37a1d5f) SHA1(5ec031c31a7967cc3fd53a535d81833e4a1c385e), ROM_BIOS(1) )
	ROM_SYSTEM_BIOS( 1, "v020001", "v020001" )
	ROMX_LOAD("v020001.f0", 0xf0000, 0x10000, NO_DUMP, ROM_BIOS(2) )
	ROM_REGION(0x08000,"gfx1", 0)
	// expects 8x9 charset!
	ROM_LOAD("50146", 0x00000, 0x02000, BAD_DUMP CRC(1305dcf5) SHA1(aca488a16ae4ff05a1f4d14574379ff49cd48343)) //taken from europc, 9th blank
ROM_END
#endif

ROM_START( ibm5160 )
	ROM_REGION16_LE(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
//	ROM_LOAD("600963.u12", 0xc8000, 0x02000, CRC(f3daf85f) SHA1(3bd29538832d3084cbddeec92593988772755283))  /* Tandon/Western Digital Fixed Disk Adapter 600963-001__TYPE_5.U12.2764.bin */

	ROM_SYSTEM_BIOS( 0, "rev1", "IBM XT 5160 08/16/82" )	/* ROM at u18 marked as BAD_DUMP for now, as current dump, while likely correct, was regenerated from a number of smaller dumps, and needs a proper redump. */
	ROMX_LOAD("5000027.u19", 0xf0000, 0x8000, CRC(fc982309) SHA1(2aa781a698a21c332398d9bc8503d4f580df0a05), ROM_BIOS(1) )
	ROMX_LOAD("5000026.u18", 0xf8000, 0x8000, BAD_DUMP CRC(3c9b0ac3) SHA1(271c9f4cef5029a1560075550b67c3395db09fef), ROM_BIOS(1) )

	ROM_SYSTEM_BIOS( 1, "rev2", "IBM XT 5160 11/08/82" )	/* Same as PC 5155 BIOS and 3270-PC BIOS */
	ROMX_LOAD("5000027.u19", 0xf0000, 0x8000, CRC(fc982309) SHA1(2aa781a698a21c332398d9bc8503d4f580df0a05), ROM_BIOS(2) ) /* MK37050N-4 */
	ROMX_LOAD("1501512.u18", 0xf8000, 0x8000, CRC(79522c3d) SHA1(6bac726d8d033491d52507278aa388ec04cf8b7e), ROM_BIOS(2) ) /* MK38036N-25 */

	ROM_SYSTEM_BIOS( 2, "rev3", "IBM XT 5160 01/10/86" )    /* Has enhanced keyboard support and a 3.5" drive */
	ROMX_LOAD("62x0854.u19", 0xf0000, 0x8000, CRC(b5fb0e83) SHA1(937b43759ffd472da4fb0fe775b3842f5fb4c3b3), ROM_BIOS(3) )
	ROMX_LOAD("62x0851.u18", 0xf8000, 0x8000, CRC(1054f7bd) SHA1(e7d0155813e4c650085144327581f05486ed1484), ROM_BIOS(3) )

	ROM_SYSTEM_BIOS( 3, "rev4", "IBM XT 5160 05/09/86" )	/* Minor bugfixes to keyboard code, supposedly */
	ROMX_LOAD("68x4370.u19", 0xf0000, 0x8000, CRC(758ff036) SHA1(045e27a70407d89b7956ecae4d275bd2f6b0f8e2), ROM_BIOS(4))
	ROMX_LOAD("62x0890.u18", 0xf8000, 0x8000, CRC(4f417635) SHA1(daa61762d3afdd7262e34edf1a3d2df9a05bcebb), ROM_BIOS(4))

//	ROM_SYSTEM_BIOS( 4, "xtdiag", "IBM XT 5160 w/Supersoft Diagnostics" )    /* ROMs marked as BAD_DUMP for now. We expect the data to be in a different ROM chip layout */
//	ROMX_LOAD("basicc11.f6", 0xf6000, 0x2000, BAD_DUMP CRC(80d3cf5d) SHA1(64769b7a8b60ffeefa04e4afbec778069a2840c9), ROM_BIOS(5) )
//	ROMX_LOAD("basicc11.f8", 0xf8000, 0x2000, BAD_DUMP CRC(673a4acc) SHA1(082ae803994048e225150f771794ca305f73d731), ROM_BIOS(5) )
//	ROMX_LOAD("basicc11.fa", 0xfa000, 0x2000, BAD_DUMP CRC(aac3fc37) SHA1(c9e0529470edf04da093bb8c8ae2536c688c1a74), ROM_BIOS(5) )
//	ROMX_LOAD("basicc11.fc", 0xfc000, 0x2000, BAD_DUMP CRC(3062b3fc) SHA1(5134dd64721cbf093d059ee5d3fd09c7f86604c7), ROM_BIOS(5) )
//	ROMX_LOAD("xtdiag.bin", 0xfe000, 0x2000, CRC(4e89a4d8) SHA1(39a28fb2fe9f1aeea24ed2c0255cebca76e37ed7), ROM_BIOS(5) )

	/* Character rom */
	ROM_REGION(0x2000,"gfx1", 0)
	ROM_LOAD("5788005.u33", 0x00000, 0x2000, CRC(0bf56d70) SHA1(c2a8b10808bf51a3c123ba3eb1e9dd608231916f))
ROM_END


ROM_START( xtvga )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("et4000.bin", 0xc0000, 0x8000, CRC(f01e4be0) SHA1(95d75ff41bcb765e50bd87a8da01835fd0aa01d5))
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	ROM_LOAD("pcxt.rom",    0xfe000, 0x02000, CRC(031aafad) SHA1(a641b505bbac97b8775f91fe9b83d9afdf4d038f))
ROM_END

ROM_START( pc200 )
//    ROM_REGION(0x100000,"main", 0)
	ROM_REGION16_LE(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	// special bios at 0xe0000 !?
	ROM_LOAD16_BYTE("pc20v2.0", 0xfc001, 0x2000, CRC(41302eb8) SHA1(8b4b2afea543b96b45d6a30365281decc15f2932)) // v2
	ROM_LOAD16_BYTE("pc20v2.1", 0xfc000, 0x2000, CRC(71b84616) SHA1(4135102a491b25fc659d70b957e07649f3eacf24)) // v2
	// also mapped to f0000, f4000, f8000
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("40109.bin",     0x00000, 0x08000, CRC(a8b67639) SHA1(99663bfb61798526e092205575370c2ad34249a1))
ROM_END

ROM_START( pc20 )
//    ROM_REGION(0x100000,"main", 0)
	ROM_REGION16_LE(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))

	// special bios at 0xe0000 !?
	// This is probably referring to a check for the Amstrad RP5-2 diagnostic
	// card, which can be plugged into an Amstrad XT for troubleshooting purposes.
	// - John Elliott
	ROM_LOAD16_BYTE("pc20v2.0", 0xfc001, 0x2000, CRC(41302eb8) SHA1(8b4b2afea543b96b45d6a30365281decc15f2932)) // v2
	ROM_LOAD16_BYTE("pc20v2.1", 0xfc000, 0x2000, CRC(71b84616) SHA1(4135102a491b25fc659d70b957e07649f3eacf24)) // v2
	// also mapped to f0000, f4000, f8000
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("40109.bin",     0x00000, 0x08000, CRC(a8b67639) SHA1(99663bfb61798526e092205575370c2ad34249a1))
ROM_END

ROM_START( ppc512 )
//    ROM_REGION(0x100000,"main", 0)
	ROM_REGION16_LE(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	// special bios at 0xe0000 !?
	ROM_LOAD16_BYTE("40107.v1", 0xfc001, 0x2000, CRC(4e37e769) SHA1(88be3d3375ec3b0a7041dbcea225b197e50d4bfe)) // v1.9
	ROM_LOAD16_BYTE("40108.v1", 0xfc000, 0x2000, CRC(4f0302d9) SHA1(e4d69ca98c3b98f3705a2902b16746360043f039)) // v1.9
	// also mapped to f0000, f4000, f8000
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("40109.bin",     0x00000, 0x08000, CRC(a8b67639) SHA1(99663bfb61798526e092205575370c2ad34249a1))
ROM_END

ROM_START( ppc640 )
//    ROM_REGION(0x100000,"main", 0)
	ROM_REGION16_LE(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	// special bios at 0xe0000 !?
	ROM_LOAD16_BYTE("40107.v2", 0xfc001, 0x2000, CRC(0785b63e) SHA1(4dbde6b9e9500298bb6241a8daefd85927f1ad28)) // v2.1
	ROM_LOAD16_BYTE("40108.v2", 0xfc000, 0x2000, CRC(5351cf8c) SHA1(b4dbf11b39378ab4afd2107d3fe54a99fffdedeb)) // v2.1
	// also mapped to f0000, f4000, f8000
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("40109.bin",     0x00000, 0x08000, CRC(a8b67639) SHA1(99663bfb61798526e092205575370c2ad34249a1))
ROM_END

ROM_START( pc1512 )
//    ROM_REGION(0x100000,"main", 0)
	ROM_REGION16_LE(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	ROM_LOAD16_BYTE("40044.v1", 0xfc001, 0x2000, CRC(668fcc94) SHA1(74002f5cc542df442eec9e2e7a18db3598d8c482)) // v1
	ROM_LOAD16_BYTE("40043.v1", 0xfc000, 0x2000, CRC(f72f1582) SHA1(7781d4717917262805d514b331ba113b1e05a247)) // v1
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("40045.bin",     0x00000, 0x02000, CRC(dd5e030f) SHA1(7d858bbb2e8d6143aa67ab712edf5f753c2788a7))
ROM_END

ROM_START( pc1512v2 )
//    ROM_REGION(0x100000,"main", 0)
	ROM_REGION16_LE(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	ROM_LOAD16_BYTE("40043.v2", 0xfc001, 0x2000, CRC(d2d4d2de) SHA1(c376fd1ad23025081ae16c7949e88eea7f56e1bb)) // v2
	ROM_LOAD16_BYTE("40044.v2", 0xfc000, 0x2000, CRC(1aec54fa) SHA1(b12fd73cfc35a240ed6da4dcc4b6c9910be611e0)) // v2
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("40078.bin",     0x00000, 0x02000, CRC(ae9c0d04) SHA1(bc8dc4dcedeea5bc1c04986b1f105ad93cb2ebcd))
ROM_END

ROM_START( pc1640 )
//    ROM_REGION(0x100000,"main", 0)
	ROM_REGION16_LE(0x100000,"main", 0)
	ROM_LOAD("40100", 0xc0000, 0x8000, CRC(d2d1f1ae) SHA1(98302006ee38a17c09bd75504cc18c0649174e33)) // this bios seams to be made for the amstrad pc
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	ROM_LOAD16_BYTE("40043.v3", 0xfc001, 0x2000, CRC(e40a1513) SHA1(447eff2057e682e51b1c7593cb6fad0e53879fa8)) // v3
	ROM_LOAD16_BYTE("40044.v3", 0xfc000, 0x2000, CRC(f1c074f3) SHA1(a055ea7e933d137623c22fe24004e870653c7952))
	ROM_REGION(0x08100,"gfx1", 0)
	ROM_LOAD("40045.bin",     0x00000, 0x02000, CRC(dd5e030f) SHA1(7d858bbb2e8d6143aa67ab712edf5f753c2788a7))
ROM_END

ROM_START( dgone )
	ROM_REGION(0x100000,"main", 0)
	ROM_LOAD("wdbios.rom",  0xc8000, 0x02000, CRC(8e9e2bd4) SHA1(601d7ceab282394ebab50763c267e915a6a2166a))
	ROM_LOAD( "dgone.bin",  0xf8000, 0x08000, CRC(2c38c86e) SHA1(c0f85a000d1d13cd354965689e925d677822549e))

	ROM_REGION(0x08100, "gfx1", 0)
	ROM_LOAD("cga.chr", 0x00000, 0x01000, CRC(42009069) SHA1(ed08559ce2d7f97f68b9f540bddad5b6295294dd))
ROM_END


static void ibmpc_cassette_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info) {
	switch( state ) {
	case MESS_DEVINFO_INT_COUNT:						info->i = 1; break;
	case MESS_DEVINFO_INT_CASSETTE_DEFAULT_STATE:		info->i = CASSETTE_PLAY | CASSETTE_MOTOR_DISABLED | CASSETTE_SPEAKER_ENABLED; break;

	default:											cassette_device_getinfo(devclass, state, info); break;
	}
}


static void ibmpc_floppy_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	/* floppy */
	switch(state)
	{
		/* --- the following bits of info are returned as 64-bit signed integers --- */
		case MESS_DEVINFO_INT_COUNT:							info->i = 2; break;

		/* --- the following bits of info are returned as pointers to data or functions --- */
		case MESS_DEVINFO_PTR_FLOPPY_OPTIONS:				info->p = (void *) floppyoptions_pc; break;

		default:										floppy_device_getinfo(devclass, state, info); break;
	}
}


static void pcjr_cartslot_getinfo(const mess_device_class *devclass, UINT32 state, union devinfo *info)
{
	switch( state )
	{
	/* --- the following bits of info are returned as 64-bit signed integers --- */
	case MESS_DEVINFO_INT_COUNT:						info->i = 2; break;
	case MESS_DEVINFO_INT_MUST_BE_LOADED:				info->i = 0; break;

	/* --- the following bits of info are returned as pointers to data or functions --- */
	case MESS_DEVINFO_PTR_LOAD:							info->load = DEVICE_IMAGE_LOAD_NAME( pcjr_cartridge ); break;

	/* --- the following bits of info are returned as NULL-terminated strings --- */
	case MESS_DEVINFO_STR_FILE_EXTENSIONS:				strcpy( info->s = device_temp_str(), "jrc" ); break;

	default:											cartslot_device_getinfo( devclass, state, info ); break;
	}
}

static SYSTEM_CONFIG_START(ibm5150)
	CONFIG_RAM_DEFAULT( 640 * 1024 )
	CONFIG_DEVICE(ibmpc_cassette_getinfo)
	CONFIG_DEVICE(ibmpc_floppy_getinfo)
SYSTEM_CONFIG_END


static SYSTEM_CONFIG_START(ibm5160)
	CONFIG_RAM_DEFAULT( 640 * 1024 )
	CONFIG_DEVICE(ibmpc_floppy_getinfo)
SYSTEM_CONFIG_END


static SYSTEM_CONFIG_START(pcjr)
	CONFIG_RAM_DEFAULT( 640 * 1024 )
	CONFIG_DEVICE(ibmpc_cassette_getinfo)
	CONFIG_DEVICE(ibmpc_floppy_getinfo)
	CONFIG_DEVICE(pcjr_cartslot_getinfo)
SYSTEM_CONFIG_END

/***************************************************************************

  Game driver(s)

***************************************************************************/

/*     YEAR     NAME        PARENT  COMPAT  MACHINE     INPUT       INIT        CONFIG   COMPANY     FULLNAME */
COMP(  1981,	ibm5150,	0,			0,	ibm5150,    ibm5150,    ibm5150,    ibm5150, "International Business Machines",  "IBM PC 5150" , 0)
COMP(  1984,	dgone,		ibm5150,	0,	pccga,      pccga,	    pccga,	    ibm5160, "Data General",  "Data General/One" , GAME_NOT_WORKING)	/* CGA, 2x 3.5" disk drives */
COMP(  1987,	pc,			ibm5150,	0,	pccga,      pccga,		pccga,	    ibm5160, "",  "PC (CGA)" , 0)
COMP(  1985,	bondwell,	ibm5150,	0,	pccga,		bondwell,   bondwell,	ibm5160, "Bondwell Holding",  "BW230 (PRO28 Series)", GAME_NOT_WORKING )
COMP(  1988,	europc,		ibm5150,	0,	europc,     europc,		europc,     ibm5160, "Schneider Rdf. AG",  "EURO PC", GAME_NOT_WORKING)

// pcjr (better graphics, better sound)
COMP(  1983,	ibmpcjr,	ibm5150,	0,	ibmpcjr,    tandy1t,	pcjr,       pcjr,    "International Business Machines",  "IBM PC Jr", GAME_NOT_WORKING|GAME_IMPERFECT_COLORS )
COMP(  1987,	t1000hx,	ibm5150,	0,	t1000hx,    tandy1t,	t1000hx,	ibm5160, "Tandy Radio Shack",  "Tandy 1000HX", 0)
COMP(  1987,	t1000sx,	ibm5150,	0,	t1000hx,    tandy1t,	t1000hx,	ibm5160, "Tandy Radio Shack",  "Tandy 1000SX", GAME_NOT_WORKING)

// xt class (pc but 8086)
COMP(  1982,	ibm5160,	ibm5150,	0,	ibm5160,    xtcga,		pccga,		ibm5160, "International Business Machines",  "IBM XT 5160" , 0)
COMP(  1988,	pc200,		ibm5150,	0,	pc200,		pc200,		pc200,		ibm5160, "Sinclair Research",  "PC200 Professional Series", GAME_NOT_WORKING)
COMP(  1988,	pc20,		ibm5150,	0,	pc200,		pc200,		pc200,		ibm5160, "Amstrad plc",  "Amstrad PC20" , GAME_NOT_WORKING)
COMP(  1987,	ppc512,		ibm5150,	0,	pc200,		pc200,		pc200,		ibm5160, "Amstrad plc",  "Amstrad PPC512", GAME_NOT_WORKING)
COMP(  1987,	ppc640,		ibm5150,	0,	pc200,		pc200,		pc200,		ibm5160, "Amstrad plc",  "Amstrad PPC640", GAME_NOT_WORKING)
COMP(  1986,	pc1512,		ibm5150,	0,	pc1512,     pc1512,		pc1512,		ibm5160, "Amstrad plc",  "Amstrad PC1512 (version 1)", GAME_NOT_WORKING)
COMP(  198?,	pc1512v2,	ibm5150,	0,	pc1512,     pc1512,		pc1512,		ibm5160, "Amstrad plc",  "Amstrad PC1512 (version 2)", GAME_NOT_WORKING)
COMP(  1987,	pc1640,		ibm5150,	0,	pc1640,     pc1640,		pc1640,		ibm5160, "Amstrad plc",  "Amstrad PC1640 / PC6400 (US)", GAME_NOT_WORKING )
// pc2086 pc1512 with vga??
COMP ( 1987,	pcmda,		ibm5150,	0,	pcmda,      pcmda,		pcmda,	    ibm5160, "",  "PC (MDA)" , 0)
COMP ( 1987,    pcherc,		ibm5150,	0,	pcherc,     pcmda,      pcmda,      ibm5160, "MESS",  "PC (Hercules)" , 0)
COMP ( 1987,	xtvga,		ibm5150,	0,	xtvga,      xtvga,		pc_vga,     ibm5160, "",  "PC/XT (VGA, MF2 Keyboard)" , GAME_NOT_WORKING)

