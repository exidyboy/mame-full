/***************************************************************************

Cobra Command:
  2 BAC06 background generator chips, same as Dec0.
  1 MXC06 chip for sprites, same as Dec0.
  256 colours, palette generated by ram.

The Real Ghostbusters:
1 Deco VSC30 (M60348)
1 Deco HMC20 (M60232)

  1 playfield, same as above, with rowscroll
  1024 colours from 2 proms.
  Sprite hardware close to above, there are some unused (unknown) bits per sprite.

Super Real Darwin:
  1 playfield, x-scroll only
  Closer to earlier Darwin 4078 board than above games.

Last Mission/Shackled:
	Has 1 Deco VSC30 (M60348) (From readme file)
	Has 1 Deco HMC20 (M60232) (From readme file)

	1 playfield
	Sprite hardware same as Karnov.
	(Shackled) Palettes 8-15 for tiles seem to have priority over sprites.

Gondomania:
	Has two large square surface mount chips: [ DRL 40, 8053, 8649a ]
	Has 1 Deco VSC30 (M60348)
	Has 1 Deco HMC20 (M60232)
	Priority - all tiles with *pens* 8-15 appear over sprites with palettes 8-15.

Oscar:
	Uses MXC-06 custom chip for sprites.
	Uses BAC-06 custom chip for background.
	I can't find what makes the fix chars...
	Priority - tiles with palettes 8-15 have their *pens* 8-15 appearing over
sprites.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

static int scroll1[4],scroll2[4];
static struct tilemap *dec8_pf0_tilemap,*dec8_pf1_tilemap,*dec8_fix_tilemap;
static int dec8_pf0_control[0x20],dec8_pf1_control[0x20];
static int gfx_bank,gfx_mask,game_uses_priority,flipscreen;
static unsigned char *gfx_base;
unsigned char *dec8_pf0_data,*dec8_pf1_data,*dec8_row;

/***************************************************************************

  Convert the color PROMs into a more useable format.

  Real Ghostbusters has two 1024x8 palette PROM.
  I don't know the exact values of the resistors between the RAM and the
  RGB output. I assumed these values (the same as Commando)

  bit 7 -- 220 ohm resistor  -- GREEN
        -- 470 ohm resistor  -- GREEN
        -- 1  kohm resistor  -- GREEN
        -- 2.2kohm resistor  -- GREEN
        -- 220 ohm resistor  -- RED
        -- 470 ohm resistor  -- RED
        -- 1  kohm resistor  -- RED
  bit 0 -- 2.2kohm resistor  -- RED

  bit 7 -- unused
        -- unused
        -- unused
        -- unused
        -- 220 ohm resistor  -- BLUE
        -- 470 ohm resistor  -- BLUE
        -- 1  kohm resistor  -- BLUE
  bit 0 -- 2.2kohm resistor  -- BLUE

***************************************************************************/
void ghostb_vh_convert_color_prom(unsigned char *palette, unsigned short *colortable,const unsigned char *color_prom)
{
	int i;

	for (i = 0;i < Machine->drv->total_colors;i++)
	{
		int bit0,bit1,bit2,bit3;

		bit0 = (color_prom[0] >> 0) & 0x01;
		bit1 = (color_prom[0] >> 1) & 0x01;
		bit2 = (color_prom[0] >> 2) & 0x01;
		bit3 = (color_prom[0] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[0] >> 4) & 0x01;
		bit1 = (color_prom[0] >> 5) & 0x01;
		bit2 = (color_prom[0] >> 6) & 0x01;
		bit3 = (color_prom[0] >> 7) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;
		bit0 = (color_prom[Machine->drv->total_colors] >> 0) & 0x01;
		bit1 = (color_prom[Machine->drv->total_colors] >> 1) & 0x01;
		bit2 = (color_prom[Machine->drv->total_colors] >> 2) & 0x01;
		bit3 = (color_prom[Machine->drv->total_colors] >> 3) & 0x01;
		*(palette++) = 0x0e * bit0 + 0x1f * bit1 + 0x43 * bit2 + 0x8f * bit3;

		color_prom++;
	}
}

WRITE_HANDLER( dec8_flipscreen_w )
{
	flipscreen=data;
	tilemap_set_flip(ALL_TILEMAPS,flipscreen ? (TILEMAP_FLIPY | TILEMAP_FLIPX) : 0);
}

WRITE_HANDLER( dec8_bac06_0_w )
{
	dec8_pf0_control[offset]=data;
}

WRITE_HANDLER( dec8_bac06_1_w )
{
	dec8_pf1_control[offset]=data;
}

WRITE_HANDLER( dec8_pf0_data_w )
{
	dec8_pf0_data[offset]=data;
	tilemap_mark_tile_dirty(dec8_pf0_tilemap,offset/2);
}

WRITE_HANDLER( dec8_pf1_data_w )
{
	dec8_pf1_data[offset]=data;
	tilemap_mark_tile_dirty(dec8_pf1_tilemap,offset/2);
}

READ_HANDLER( dec8_pf0_data_r )
{
	return dec8_pf0_data[offset];
}

READ_HANDLER( dec8_pf1_data_r )
{
	return dec8_pf1_data[offset];
}

WRITE_HANDLER( dec8_videoram_w )
{
	videoram[offset]=data;
	tilemap_mark_tile_dirty( dec8_fix_tilemap,offset/2 );
}

WRITE_HANDLER( srdarwin_videoram_w )
{
	videoram[offset]=data;
	tilemap_mark_tile_dirty( dec8_fix_tilemap,offset );
}

WRITE_HANDLER( dec8_scroll1_w )
{
	scroll1[offset]=data;
}

WRITE_HANDLER( dec8_scroll2_w )
{
	scroll2[offset]=data;
}

WRITE_HANDLER( srdarwin_control_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);

	switch (offset) {
    	case 0: /* Top 3 bits - bank switch, bottom 4 - scroll MSB */
			bankaddress = 0x10000 + (data >> 5) * 0x4000;
			cpu_setbank(1,&RAM[bankaddress]);
			scroll2[0]=data&0xf;
			return;

        case 1:
        	scroll2[1]=data;
        	return;
    }
}

WRITE_HANDLER( lastmiss_control_w )
{
	int bankaddress;
	unsigned char *RAM = memory_region(REGION_CPU1);

	/* Bottom 4 bits - bank switch, Bits 4 & 5 - Scroll MSBs */
	bankaddress = 0x10000 + (data & 0x0f) * 0x4000;
	cpu_setbank(1,&RAM[bankaddress]);

	scroll2[0]=(data>>5)&1;
	scroll2[2]=(data>>6)&1;

if (cpu_get_pc()==0xfa51) cpu_set_reset_line(1,PULSE_LINE); /* No way this can be right... */
if (cpu_get_pc()==0xf9d2) cpu_set_reset_line(1,PULSE_LINE); /* No way this can be right... */

//logerror("PC %06x - Write %02x to %04x\n",cpu_get_pc(),data,offset+0x1802);
}

WRITE_HANDLER( lastmiss_scrollx_w )
{
	scroll2[1]=data;
}

WRITE_HANDLER( lastmiss_scrolly_w )
{
	scroll2[3]=data;
}

WRITE_HANDLER( gondo_scroll_w )
{
	switch (offset) {
		case 0x0:
			scroll2[1]=data; /* X LSB */
			break;
		case 0x8:
			scroll2[3]=data; /* Y LSB */
			break;
		case 0x10:
			scroll2[0]=(data>>0)&1; /* Bit 0: X MSB */
			scroll2[2]=(data>>1)&1; /* Bit 1: Y MSB */
			/* Bit 2 is also used in Gondo & Garyoret */
			break;
	}
}

/******************************************************************************/

/* 'Karnov' sprites, used by Gondomania, Last Mission, Shackled, Ghostbusters */
static void draw_sprites1(struct osd_bitmap *bitmap, int priority)
{
	int offs,x,y,sprite,sprite2,colour,extra,fx,fy;

	for (offs = 0;offs < 0x800;offs += 8)
	{
		y=buffered_spriteram[offs+1]+(buffered_spriteram[offs]<<8);
		if ((y&0x8000) == 0) continue;

        fx=buffered_spriteram[offs+3];

		if ((fx&0x1) == 0) continue;

		extra=fx&0x10;
        fy=fx&0x2;
        fx=fx&0x4;

		x = buffered_spriteram[offs+5]+(buffered_spriteram[offs+4]<<8);
		colour = buffered_spriteram[offs+6] >> 4;
		if (priority==1 && (colour&8)) continue;
		if (priority==2 && !(colour&8)) continue;
		sprite = buffered_spriteram[offs+7]+(buffered_spriteram[offs+6]<<8);
		sprite &= 0x0fff;

		if (extra) {y=y+16;sprite&=0xffe;}

		x = x & 0x01ff;
		y = y & 0x01ff;
		x=(x+16)%0x200;
		y=(y+16)%0x200;
		x=256 - x;
		y=256 - y;
		if (flipscreen) {
			y=240-y;
			x=240-x;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			if (extra) y=y-16;
		}

		/* Y Flip determines order of multi-sprite */
		if (extra && fy) {
			sprite2=sprite;
			sprite++;
		}
		else
			sprite2=sprite+1;

		drawgfx(bitmap,Machine->gfx[1],
				sprite,
				colour,fx,fy,x,y,
				0,TRANSPARENCY_PEN,0);

    	/* 1 more sprite drawn underneath */
    	if (extra)
    		drawgfx(bitmap,Machine->gfx[1],
				sprite2,
				colour,fx,fy,x,y+16,
				0,TRANSPARENCY_PEN,0);
	}
}

/* 'Dec0' sprites, used by Cobra Command, Oscar */
static void draw_sprites2(struct osd_bitmap *bitmap, int priority)
{
	int offs,x,y,sprite,colour,multi,fx,fy,inc,flash,mult;

	/* Sprites */
	for (offs = 0;offs < 0x800;offs += 8)
	{
		y =buffered_spriteram[offs+1]+(buffered_spriteram[offs]<<8);
 		if ((y&0x8000) == 0) continue;
		x = buffered_spriteram[offs+5]+(buffered_spriteram[offs+4]<<8);
		colour = ((x & 0xf000) >> 12);
		flash=x&0x800;
		if (flash && (cpu_getcurrentframe() & 1)) continue;

		if (priority==1 &&  (colour&4)) continue;
		if (priority==2 && !(colour&4)) continue;

		fx = y & 0x2000;
		fy = y & 0x4000;
		multi = (1 << ((y & 0x1800) >> 11)) - 1;	/* 1x, 2x, 4x, 8x height */

											/* multi = 0   1   3   7 */
		sprite = buffered_spriteram[offs+3]+(buffered_spriteram[offs+2]<<8);
		sprite &= 0x0fff;

		x = x & 0x01ff;
		y = y & 0x01ff;
		if (x >= 256) x -= 512;
		if (y >= 256) y -= 512;
		x = 240 - x;
		y = 240 - y;

		sprite &= ~multi;
		if (fy)
			inc = -1;
		else
		{
			sprite += multi;
			inc = 1;
		}

		if (flipscreen) {
			y=240-y;
			x=240-x;
			if (fx) fx=0; else fx=1;
			if (fy) fy=0; else fy=1;
			mult=16;
		}
		else mult=-16;

		while (multi >= 0)
		{
			drawgfx(bitmap,Machine->gfx[1],
					sprite - multi * inc,
					colour,
					fx,fy,
					x,y + mult * multi,
					&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
			multi--;
		}
	}
}

static void srdarwin_drawsprites(struct osd_bitmap *bitmap, int pri)
{
	int offs;

	/* Sprites */
	for (offs = 0;offs < 0x200;offs += 4)
	{
		int multi,fx,sx,sy,sy2,code,color;

		code = buffered_spriteram[offs+3] + ( ( buffered_spriteram[offs+1] & 0xe0 ) << 3 );
		sx = (241 - buffered_spriteram[offs+2]);
	//if (sx < -7) sx += 256;

		sy = buffered_spriteram[offs];
		color = (buffered_spriteram[offs+1] & 0x03) + ((buffered_spriteram[offs+1] & 0x08) >> 1);

		if (pri==0 && color!=0) continue;
		if (pri==1 && color==0) continue;

		fx = buffered_spriteram[offs+1] & 0x04;
		multi = buffered_spriteram[offs+1] & 0x10;

		if (flipscreen) {
			sy=240-sy;
			sx=240-sx;
			if (fx) fx=0; else fx=1;
			sy2=sy-16;
		}
		else sy2=sy+16;

    	drawgfx(bitmap,Machine->gfx[1],
        		code,
				color,
				fx,flipscreen,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
        if (multi)
    		drawgfx(bitmap,Machine->gfx[1],
				code+1,
				color,
				fx,flipscreen,
				sx,sy2,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}

/* Draw character tiles, each game has different colour masks */
static void draw_characters(struct osd_bitmap *bitmap, int mask, int shift)
{
	int mx,my,tile,color,offs;

	for (offs = 0x800 - 2;offs >= 0;offs -= 2) {
		tile=videoram[offs+1]+((videoram[offs]&0xf)<<8);

		if (!tile) continue;

		color=(videoram[offs]&mask)>>shift;
		mx = (offs/2) % 32;
		my = (offs/2) / 32;

		drawgfx(bitmap,Machine->gfx[0],
				tile,color,0,0,8*mx,8*my,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}
}

/******************************************************************************/

void cobracom_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	tilemap_set_scrollx( dec8_pf0_tilemap,0, (dec8_pf0_control[0x10]<<8)+dec8_pf0_control[0x11] );
	tilemap_set_scrolly( dec8_pf0_tilemap,0, (dec8_pf0_control[0x12]<<8)+dec8_pf0_control[0x13] );
	tilemap_set_scrollx( dec8_pf1_tilemap,0, (dec8_pf1_control[0x10]<<8)+dec8_pf1_control[0x11] );
	tilemap_set_scrolly( dec8_pf1_tilemap,0, (dec8_pf1_control[0x12]<<8)+dec8_pf1_control[0x13] );
	dec8_flipscreen_w(0,dec8_pf0_control[0]>>7);

	gfx_mask=3;
	gfx_bank=3;
	gfx_base=dec8_pf0_data;
	tilemap_update(dec8_pf0_tilemap);

	gfx_bank=2;
	gfx_base=dec8_pf1_data;
	tilemap_update(dec8_pf1_tilemap);
	tilemap_update(dec8_fix_tilemap);

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);
	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap,dec8_pf0_tilemap,0);
	draw_sprites2(bitmap,1);
	tilemap_draw(bitmap,dec8_pf1_tilemap,0);
	draw_sprites2(bitmap,2);
	tilemap_draw(bitmap,dec8_fix_tilemap,0);
}

/******************************************************************************/

static void get_bac0_tile_info( int tile_index )
{
	int tile,color,offs=tile_index<<1;

	tile=(gfx_base[offs]<<8) | gfx_base[offs+1];
	color=tile >> 12;
	if (color>7 && game_uses_priority) tile_info.priority=1; else tile_info.priority=0;

	SET_TILE_INFO(gfx_bank,tile&0xfff,color&gfx_mask)
}

static UINT32 bac0_scan_rows(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return ((col & 0x0f) + ((row & 0x0f) << 4)) + ((col & 0x10) << 5) + ((row & 0x10) << 4);
}

static void get_cobracom_fix_tile_info( int tile_index )
{
	int offs=tile_index<<1;
	int tile=videoram[offs+1]+(videoram[offs]<<8);
	int color=(tile&0xe000) >> 13;

	SET_TILE_INFO(0,tile&0xfff,color)
}

int cobracom_vh_start(void)
{
	game_uses_priority=0;
	dec8_pf0_tilemap = tilemap_create(get_bac0_tile_info,bac0_scan_rows,0,16,16,32,32);
	dec8_pf1_tilemap = tilemap_create(get_bac0_tile_info,bac0_scan_rows,TILEMAP_TRANSPARENT,16,16,32,32);
	dec8_fix_tilemap = tilemap_create(get_cobracom_fix_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!dec8_pf0_tilemap || !dec8_pf1_tilemap || !dec8_fix_tilemap)
		return 1;

	dec8_pf1_tilemap->transparent_pen = 0;
	dec8_fix_tilemap->transparent_pen = 0;

	return 0;
}

/******************************************************************************/

void ghostb_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
   if (dec8_pf0_control[0]&0x4) { /* Rowscroll */
 		int offs;

		tilemap_set_scroll_rows(dec8_pf0_tilemap,512);
		for (offs = 0;offs < 512;offs+=2)
			tilemap_set_scrollx( dec8_pf0_tilemap,offs/2, (dec8_pf0_control[0x10]<<8)+dec8_pf0_control[0x11] + (dec8_row[offs]<<8)+dec8_row[offs+1] );
	} else {
		tilemap_set_scroll_rows(dec8_pf0_tilemap,1);
		tilemap_set_scrollx( dec8_pf0_tilemap,0, (dec8_pf0_control[0x10]<<8)+dec8_pf0_control[0x11] );
	}
	tilemap_set_scrolly( dec8_pf0_tilemap,0, (dec8_pf0_control[0x12]<<8)+dec8_pf0_control[0x13] );

	tilemap_update(ALL_TILEMAPS);
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);
	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap,dec8_pf0_tilemap,0);
	draw_sprites1(bitmap,0);
	tilemap_draw(bitmap,dec8_fix_tilemap,0);
}

static void get_ghostb_fix_tile_info( int tile_index )
{
	int offs=tile_index<<1;
	int tile=videoram[offs+1]+(videoram[offs]<<8);
	int color=(tile&0xc00) >> 10;

	SET_TILE_INFO(0,tile&0x3ff,color)
}

int ghostb_vh_start(void)
{
	dec8_pf0_tilemap = tilemap_create(get_bac0_tile_info,bac0_scan_rows,0,16,16,32,32);
	dec8_fix_tilemap = tilemap_create(get_ghostb_fix_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);
	dec8_fix_tilemap->transparent_pen = 0;

	if (!dec8_pf0_tilemap || !dec8_fix_tilemap)
		return 1;

	gfx_bank=2;
	gfx_mask=0xf;
	gfx_base=dec8_pf0_data;

	return 0;
}

/******************************************************************************/

void oscar_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	tilemap_set_scrollx( dec8_pf0_tilemap,0, (dec8_pf0_control[0x10]<<8)+dec8_pf0_control[0x11] );
	tilemap_set_scrolly( dec8_pf0_tilemap,0, (dec8_pf0_control[0x12]<<8)+dec8_pf0_control[0x13] );
	dec8_flipscreen_w(0,dec8_pf0_control[1]>>7);

	tilemap_update(ALL_TILEMAPS);
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);
	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap,dec8_pf0_tilemap,TILEMAP_BACK | 0);
	tilemap_draw(bitmap,dec8_pf0_tilemap,TILEMAP_BACK | 1);
	tilemap_draw(bitmap,dec8_pf0_tilemap,TILEMAP_FRONT | 0);
	draw_sprites2(bitmap,0);
	tilemap_draw(bitmap,dec8_pf0_tilemap,TILEMAP_FRONT | 1);
	tilemap_draw(bitmap,dec8_fix_tilemap,0);
}

static void get_oscar_fix_tile_info( int tile_index )
{
	int offs=tile_index<<1;
	int tile=videoram[offs+1]+(videoram[offs]<<8);
	int color=(tile&0xf000) >> 14;

	SET_TILE_INFO(0,tile&0xfff,color)
}

int oscar_vh_start(void)
{
	dec8_pf0_tilemap = tilemap_create(get_bac0_tile_info,bac0_scan_rows,TILEMAP_SPLIT,16,16,32,32);
	dec8_fix_tilemap = tilemap_create(get_oscar_fix_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!dec8_pf0_tilemap || !dec8_fix_tilemap)
		return 1;

	dec8_fix_tilemap->transparent_pen = 0;
	dec8_pf0_tilemap->transmask[0] = 0x00ff; /* Bottom 8 pens */
	dec8_pf0_tilemap->transmask[1] = 0xff00; /* Top 8 pens */
	game_uses_priority=1;
	gfx_bank=2;
	gfx_mask=0x7;
	gfx_base=dec8_pf0_data;

	return 0;
}

/******************************************************************************/

void lastmiss_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	tilemap_set_scrollx( dec8_pf0_tilemap,0, ((scroll2[0]<<8)+scroll2[1]) );
	tilemap_set_scrolly( dec8_pf0_tilemap,0, ((scroll2[2]<<8)+scroll2[3]) );
	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	memset(palette_used_colors+256,PALETTE_COLOR_USED,256);
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);
	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap,dec8_pf0_tilemap,0);
	draw_sprites1(bitmap,0);
	tilemap_draw(bitmap,dec8_fix_tilemap,0);
}

void shackled_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	tilemap_set_scrollx( dec8_pf0_tilemap,0, ((scroll2[0]<<8)+scroll2[1]) );
	tilemap_set_scrolly( dec8_pf0_tilemap,0, ((scroll2[2]<<8)+scroll2[3]) );
	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	memset(palette_used_colors+256,PALETTE_COLOR_USED,256);
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);
	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap,dec8_pf0_tilemap,TILEMAP_BACK | 0);
	tilemap_draw(bitmap,dec8_pf0_tilemap,TILEMAP_BACK | 1);
	tilemap_draw(bitmap,dec8_pf0_tilemap,TILEMAP_FRONT | 0);
	draw_sprites1(bitmap,0);
	tilemap_draw(bitmap,dec8_pf0_tilemap,TILEMAP_FRONT | 1);
	tilemap_draw(bitmap,dec8_fix_tilemap,0);
}

static UINT32 lastmiss_scan_rows(UINT32 col,UINT32 row,UINT32 num_cols,UINT32 num_rows)
{
	/* logical (col,row) -> memory offset */
	return ((col & 0x0f) + ((row & 0x0f) << 4)) + ((col & 0x10) << 4) + ((row & 0x10) << 5);
}

static void get_lastmiss_tile_info( int tile_index )
{
	int offs=tile_index*2;
	int tile=dec8_pf0_data[offs+1]+(dec8_pf0_data[offs]<<8);
	int color=tile >> 12;

	if (color>7 && game_uses_priority) tile_info.priority=1; else tile_info.priority=0;

	SET_TILE_INFO(2,tile&0xfff,color)
}

static void get_lastmiss_fix_tile_info( int tile_index )
{
	int offs=tile_index<<1;
	int tile=videoram[offs+1]+(videoram[offs]<<8);
	int color=(tile&0xc000) >> 14;

	SET_TILE_INFO(0,tile&0xfff,color)
}

int lastmiss_vh_start(void)
{
	dec8_pf0_tilemap = tilemap_create(get_lastmiss_tile_info,lastmiss_scan_rows,0,16,16,32,32);
	dec8_fix_tilemap = tilemap_create(get_lastmiss_fix_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!dec8_pf0_tilemap || !dec8_fix_tilemap)
		return 1;

	dec8_fix_tilemap->transparent_pen = 0;
	game_uses_priority=0;

	return 0;
}

int shackled_vh_start(void)
{
	dec8_pf0_tilemap = tilemap_create(get_lastmiss_tile_info,lastmiss_scan_rows,TILEMAP_SPLIT,16,16,32,32);
	dec8_fix_tilemap = tilemap_create(get_lastmiss_fix_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!dec8_pf0_tilemap || !dec8_fix_tilemap)
		return 1;

	dec8_fix_tilemap->transparent_pen = 0;
	dec8_pf0_tilemap->transmask[0] = 0x000f; /* Bottom 12 pens */
	dec8_pf0_tilemap->transmask[1] = 0xfff0; /* Top 4 pens */
	game_uses_priority=1;

	return 0;
}

/******************************************************************************/

void srdarwin_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	tilemap_set_scrollx( dec8_pf0_tilemap,0, (scroll2[0]<<8)+scroll2[1] );
	tilemap_update(ALL_TILEMAPS);

	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);

	tilemap_render(ALL_TILEMAPS);
	tilemap_draw(bitmap,dec8_pf0_tilemap,TILEMAP_BACK | 1);
	tilemap_draw(bitmap,dec8_pf0_tilemap,TILEMAP_BACK | 0);
	tilemap_draw(bitmap,dec8_pf0_tilemap,TILEMAP_FRONT | 1);
	srdarwin_drawsprites(bitmap,0); /* Priority may not be right on later levels */
	tilemap_draw(bitmap,dec8_pf0_tilemap,TILEMAP_FRONT | 0);
	srdarwin_drawsprites(bitmap,1);
	tilemap_draw(bitmap,dec8_fix_tilemap,0);
}

static void get_srdarwin_fix_tile_info( int tile_index )
{
	int tile=videoram[tile_index];
	int color=0; /* ? */

	if (color>1) tile_info.priority=1; else tile_info.priority=0;

	SET_TILE_INFO(0,tile,color)
}

static void get_srdarwin_tile_info(int tile_index)
{
	int tile=dec8_pf0_data[2*tile_index+1]+(dec8_pf0_data[2*tile_index]<<8);
	int color=tile >> 12;
	int bank;

	tile=tile&0xfff;
	bank=(tile/0x100)+2;

	SET_TILE_INFO(bank,tile,color)
}

int srdarwin_vh_start(void)
{
	dec8_pf0_tilemap = tilemap_create(get_srdarwin_tile_info,tilemap_scan_rows,TILEMAP_SPLIT,16,16,32,16);
	dec8_fix_tilemap = tilemap_create(get_srdarwin_fix_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);

	if (!dec8_pf0_tilemap || !dec8_fix_tilemap)
		return 1;

	dec8_fix_tilemap->transparent_pen = 0;
	dec8_pf0_tilemap->transmask[0] = 0x00ff; /* Bottom 8 pens */
	dec8_pf0_tilemap->transmask[1] = 0xff00; /* Top 8 pens */

	return 0;
}

/******************************************************************************/

void gondo_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	tilemap_set_scrollx( dec8_pf0_tilemap,0, ((scroll2[0]<<8)+scroll2[1]) );
	tilemap_set_scrolly( dec8_pf0_tilemap,0, ((scroll2[2]<<8)+scroll2[3]) );
	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	memset(palette_used_colors+256,PALETTE_COLOR_USED,256);
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);
	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap,dec8_pf0_tilemap,TILEMAP_BACK);
	draw_sprites1(bitmap,2);
	tilemap_draw(bitmap,dec8_pf0_tilemap,TILEMAP_FRONT);
	draw_sprites1(bitmap,1);
	tilemap_draw(bitmap,dec8_fix_tilemap,0);
}

void garyoret_vh_screenrefresh(struct osd_bitmap *bitmap, int full_refresh)
{
	tilemap_set_scrollx( dec8_pf0_tilemap,0, ((scroll2[0]<<8)+scroll2[1]) );
	tilemap_set_scrolly( dec8_pf0_tilemap,0, ((scroll2[2]<<8)+scroll2[3]) );
	tilemap_update(ALL_TILEMAPS);

	palette_init_used_colors();
	memset(palette_used_colors+256,PALETTE_COLOR_USED,256);
	if (palette_recalc())
		tilemap_mark_all_pixels_dirty(ALL_TILEMAPS);
	tilemap_render(ALL_TILEMAPS);

	tilemap_draw(bitmap,dec8_pf0_tilemap,0);
	draw_sprites1(bitmap,0);
	tilemap_draw(bitmap,dec8_pf0_tilemap,1);
	tilemap_draw(bitmap,dec8_fix_tilemap,0);
}

static void get_gondo_fix_tile_info( int tile_index )
{
	int offs=tile_index*2;
	int tile=videoram[offs+1]+(videoram[offs]<<8);
	int color=(tile&0x7000) >> 12;

	SET_TILE_INFO(0,tile&0xfff,color)
}

static void get_gondo_tile_info( int tile_index )
{
	int offs=tile_index*2;
	int tile=dec8_pf0_data[offs+1]+(dec8_pf0_data[offs]<<8);
	int color=tile>> 12;

	if (color>7 && game_uses_priority) tile_info.priority=1; else tile_info.priority=0;

	SET_TILE_INFO(2,tile&0xfff,color)
}

int gondo_vh_start(void)
{
	dec8_fix_tilemap=tilemap_create(get_gondo_fix_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);
	dec8_pf0_tilemap=tilemap_create(get_gondo_tile_info,tilemap_scan_rows,TILEMAP_SPLIT,16,16,32,32);

	if (!dec8_fix_tilemap || !dec8_pf0_tilemap)
		return 1;

	dec8_fix_tilemap->transparent_pen = 0;
	dec8_pf0_tilemap->transmask[0] = 0x00ff; /* Bottom 8 pens */
	dec8_pf0_tilemap->transmask[1] = 0xff00; /* Top 8 pens */
	game_uses_priority=0;

	return 0;
}

int garyoret_vh_start(void)
{
	dec8_fix_tilemap=tilemap_create(get_gondo_fix_tile_info,tilemap_scan_rows,TILEMAP_TRANSPARENT,8,8,32,32);
	dec8_pf0_tilemap=tilemap_create(get_gondo_tile_info,tilemap_scan_rows,TILEMAP_SPLIT,16,16,32,32);

	if (!dec8_fix_tilemap || !dec8_pf0_tilemap)
		return 1;

	dec8_fix_tilemap->transparent_pen = 0;
	game_uses_priority=1;

	return 0;
}

/******************************************************************************/
