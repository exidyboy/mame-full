/* Ramtek M79 Ambush */

#include "driver.h"
#include "vidhrdw/generic.h"
/*
 * in
 * 8000 DIP SW
 * 8002 D0=VBlank
 * 8004
 * 8005
 *
 * out
 * 8000
 * 8001 Mask Sel
 * 8002
 * 8003 D0=SelfTest LED
 *
 */

void ramtek_videoram_w(int offset,int data);

int  invaders_interrupt(void);
int  ramtek_vh_start(void);
void ramtek_vh_stop(void);
void ramtek_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh);
void ramtek_sh_update(void);
void ramtek_mask_w(int offset, int data);

extern unsigned char *ramtek_videoram;

/*
 * since these functions aren't used anywhere else, i've made them
 * static, and included them here
 */
static const int ControllerTable[32] = {
    0  , 1  , 3  , 2  , 6  , 7  , 5  , 4  ,
    12 , 13 , 15 , 14 , 10 , 11 , 9  , 8  ,
    24 , 25 , 27 , 26 , 30 , 31 , 29 , 28 ,
    20 , 21 , 23 , 22 , 18 , 19 , 17 , 16
};

static int gray5bit_controller0_r(int offset)
{
    return (input_port_2_r(0) & 0xe0) | (~ControllerTable[input_port_2_r(0) & 0x1f] & 0x1f);
}

static int gray5bit_controller1_r(int offset)
{
    return (input_port_3_r(0) & 0xe0) | (~ControllerTable[input_port_3_r(0) & 0x1f] & 0x1f);
}

static struct MemoryReadAddress readmem[] =
{
	{ 0x0000, 0x1fff, MRA_ROM },
	{ 0x4000, 0x63ff, MRA_RAM },
	{ 0x8000, 0x8000, input_port_0_r},
	{ 0x8002, 0x8002, input_port_1_r},
	{ 0x8004, 0x8004, gray5bit_controller0_r},
	{ 0x8005, 0x8005, gray5bit_controller1_r},
	{ 0xC000, 0xC07f, MRA_RAM},			/* ?? */
	{ 0xC200, 0xC27f, MRA_RAM},			/* ?? */
	{ -1 }  /* end of table */
};

void sound_w(int offset,int data)
{
}

static struct MemoryWriteAddress writemem[] =
{
	{ 0x0000, 0x1fff, MWA_ROM },
	{ 0x4000, 0x43ff, MWA_RAM },
    { 0x4400, 0x5fff, ramtek_videoram_w, &ramtek_videoram },
    { 0x6000, 0x63ff, MWA_RAM },		/* ?? */
	{ 0x8001, 0x8001, ramtek_mask_w},
	{ 0x8000, 0x8000, sound_w },
	{ 0x8002, 0x8003, sound_w },
	{ 0xC000, 0xC07f, MWA_RAM},			/* ?? */
	{ 0xC200, 0xC27f, MWA_RAM},			/* ?? */
	{ -1 }  /* end of table */
};

static struct IOReadPort readport[] =
{
	{ -1 }  /* end of table */
};

static struct IOWritePort writeport[] =
{
	{ -1 }  /* end of table */
};


INPUT_PORTS_START( m79_input_ports )
	PORT_START      /* 8000 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW, IPT_UNKNOWN )	/* dip switch */
	PORT_BIT( 0x02, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x04, IP_ACTIVE_LOW, IPT_UNUSED )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x10, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_HIGH, IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_HIGH, IPT_UNUSED )

	PORT_START      /* 8002 */
	PORT_BIT( 0x01, IP_ACTIVE_LOW,  IPT_VBLANK )
	PORT_BIT( 0x02, IP_ACTIVE_LOW,  IPT_START1 )
	PORT_BIT( 0x04, IP_ACTIVE_LOW,  IPT_COIN1  )
	PORT_BIT( 0x08, IP_ACTIVE_LOW,  IPT_TILT   )
	PORT_BIT( 0x10, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNUSED )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNUSED )

	PORT_START		/* 8004 */
	PORT_ANALOG ( 0x1f, 0x10, IPT_PADDLE, 25, 0, 0, 0x1f)
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON1 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

	PORT_START      /* 8005 */
	PORT_ANALOG ( 0x1f, 0x10, IPT_PADDLE | IPF_PLAYER2, 25, 0, 0, 0x1f)
	PORT_BIT( 0x20, IP_ACTIVE_LOW,  IPT_BUTTON2 )
	PORT_BIT( 0x40, IP_ACTIVE_LOW,  IPT_UNKNOWN )
	PORT_BIT( 0x80, IP_ACTIVE_LOW,  IPT_UNKNOWN )

INPUT_PORTS_END


static unsigned char palette[] = /* V.V */ /* Smoothed pure colors, overlays are not so contrasted */
{
	0x00,0x00,0x00, /* BLACK */
	0xff,0xff,0xff, /* WHITE */
	0xff,0x20,0x20, /* RED */
	0x20,0xff,0x20, /* GREEN */
	0xff,0xff,0x20, /* YELLOW */
	0x20,0xff,0xff, /* CYAN */
	0xff,0x20,0xff  /* PURPLE */
};

static int M79_interrupt(void)
{
                return 0x00cf;  /* RST 08h */
}

static void m79_init(void)
{
  int i;
  /* PROM data is active low */
  for(i=0; i<0x2000; i++)
   ROM[i] = ~ROM[i];
}

static struct MachineDriver machine_driver =
{
        /* basic machine hardware */
        {
                {
                    CPU_8080,
                    1996800,
                    0,
                    readmem,writemem,readport,writeport,
                    M79_interrupt, 1
                }
        },
        60, DEFAULT_REAL_60HZ_VBLANK_DURATION,  /* frames per second, vblank duration */
        1,      /* single CPU, no need for interleaving */
        0,

        /* video hardware */
        32*8, 28*8, { 0*8, 32*8-1, 0*8, 28*8-1 },
        0,      /* no gfxdecodeinfo - bitmapped display */
        sizeof(palette)/3, 0,
        0,

        VIDEO_TYPE_RASTER|VIDEO_SUPPORTS_DIRTY,
        0,
        ramtek_vh_start,
        ramtek_vh_stop,
        ramtek_vh_screenrefresh,

	/* sound hardware */
	0,0,0,0
};



ROM_START( m79_rom )
	ROM_REGION(0x10000)     /* 64k for code */
	ROM_LOAD( "M79.10T", 0x0000, 0x0200, 0xd300871e )
	ROM_LOAD( "M79.9T" , 0x0200, 0x0200, 0x56dd1b17 )
	ROM_LOAD( "M79.8T" , 0x0400, 0x0200, 0x933244cc )
	ROM_LOAD( "M79.7T" , 0x0600, 0x0200, 0x27018cad )
	ROM_LOAD( "M79.6T" , 0x0800, 0x0200, 0xfafce922 )
	ROM_LOAD( "M79.5T" , 0x0a00, 0x0200, 0x68109ca6 )
	ROM_LOAD( "M79.4T" , 0x0c00, 0x0200, 0xdb9b7c03 )
	ROM_LOAD( "M79.3TA", 0x0e00, 0x0200, 0x4e3934a7 )
	ROM_LOAD( "M79.10U", 0x1000, 0x0200, 0x1465db6b )
	ROM_LOAD( "M79.9U" , 0x1200, 0x0200, 0x9a4b1fc7 )
	ROM_LOAD( "M79.8U" , 0x1400, 0x0200, 0xdd7b62cb )
	ROM_LOAD( "M79.7U" , 0x1600, 0x0200, 0x42891d89 )
	ROM_LOAD( "M79.6U" , 0x1800, 0x0200, 0x55cf96a7 )
	ROM_LOAD( "M79.5U" , 0x1a00, 0x0200, 0xfc12c34c )
	ROM_LOAD( "M79.4U" , 0x1c00, 0x0200, 0xcc7acf0c )
	ROM_LOAD( "M79.3U" , 0x1e00, 0x0200, 0x896de571 )
 ROM_END


struct GameDriver m79amb_driver =
{
	__FILE__,
	0,
	"m79amb",
	"M79 Ambush",
	"1977",
	"Ramtek",
	"Space Invaders Team\n",
	0,
	&machine_driver,

	m79_rom,
	m79_init,
	0,
	0,
	0,      /* sound_prom */

	m79_input_ports,

	0, palette, 0,
	ORIENTATION_DEFAULT,

	0,0
};
