/***************************************************************************

  vidhrdw.c

  Functions to emulate the video hardware of the machine.

***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"



unsigned char *centiped_paletteram;
static int flipscreen;

static struct rectangle spritevisiblearea =
{
	1*8, 31*8-1,
	0*8, 30*8-1
};



/***************************************************************************

  Centipede doesn't have a color PROM. Eight RAM locations control
  the color of characters and sprites. The meanings of the four bits are
  (all bits are inverted):

  bit 3 alternate
        blue
        green
  bit 0 red

  The alternate bit affects blue and green, not red. The way I weighted its
  effect might not be perfectly accurate, but is reasonably close.

***************************************************************************/
static void setcolor(int pen,int data)
{
	int r,g,b;


	r = 0xff * ((~data >> 0) & 1);
	g = 0xff * ((~data >> 1) & 1);
	b = 0xff * ((~data >> 2) & 1);

	if (~data & 0x08) /* alternate = 1 */
	{
		/* when blue component is not 0, decrease it. When blue component is 0, */
		/* decrease green component. */
		if (b) b = 0xc0;
		else if (g) g = 0xc0;
	}

	osd_modify_pen(Machine->pens[pen],r,g,b);
}

void centiped_paletteram_w(int offset,int data)
{
	centiped_paletteram[offset] = data;

	/* the char palette will be effectively updated by the next interrupt handler */

	if (offset >= 12 && offset < 16)	/* sprites palette */
	{
		int start = Machine->drv->gfxdecodeinfo[1].color_codes_start;

		setcolor(start + (offset - 12),data);
	}
}

int centiped_interrupt(void)
{
	int offset;
	int slice = 3 - cpu_getiloops();
	int start = Machine->drv->gfxdecodeinfo[0].color_codes_start;


	/* set the palette for the previous screen slice to properly support */
	/* midframe palette changes in test mode */
	for (offset = 4;offset < 8;offset++)
		setcolor(4 * slice + start + (offset - 4),centiped_paletteram[offset]);

	return interrupt();
}



void centiped_vh_flipscreen_w (int offset,int data)
{
	if (flipscreen != (data & 0x80))
	{
		flipscreen = data & 0x80;
		memset(dirtybuffer,1,videoram_size);
	}
}



/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void centiped_vh_screenrefresh(struct osd_bitmap *bitmap)
{
	int offs;


	for (offs = videoram_size - 1;offs >= 0;offs--)
	{
		if (dirtybuffer[offs])
		{
			int sx,sy;


			dirtybuffer[offs] = 0;

			sx = offs % 32;
			sy = offs / 32;

			drawgfx(tmpbitmap,Machine->gfx[0],
					(videoram[offs] & 0x3f) + 0x40,
					sy / 8,	/* support midframe palette changes in test mode */
					flipscreen,flipscreen,
					8*sx,8*sy,
					&Machine->drv->visible_area,TRANSPARENCY_NONE,0);
		}
	}


	/* copy the temporary bitmap to the screen */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);


	/* Draw the sprites */
	for (offs = 0;offs < 0x10;offs++)
	{
		int spritenum,color;
		int flipx;


		spritenum = spriteram[offs] & 0x3f;
		if (spritenum & 1) spritenum = spritenum / 2 + 64;
		else spritenum = spritenum / 2;

		flipx = (spriteram[offs] & 0x80) ^ flipscreen;

		/* Centipede is unusual because the sprite color code specifies the */
		/* colors to use one by one, instead of a combination code. */
		/* bit 5-4 = color to use for pen 11 */
		/* bit 3-2 = color to use for pen 10 */
		/* bit 1-0 = color to use for pen 01 */
		/* pen 00 is transparent */
		color = spriteram[offs+0x30];
		Machine->gfx[1]->colortable[3] =
				Machine->pens[Machine->drv->gfxdecodeinfo[1].color_codes_start + ((color >> 4) & 3)];
		Machine->gfx[1]->colortable[2] =
				Machine->pens[Machine->drv->gfxdecodeinfo[1].color_codes_start + ((color >> 2) & 3)];
		Machine->gfx[1]->colortable[1] =
				Machine->pens[Machine->drv->gfxdecodeinfo[1].color_codes_start + ((color >> 0) & 3)];

		drawgfx(bitmap,Machine->gfx[1],
				spritenum,0,
				flipscreen,flipx,
				spriteram[offs + 0x20],240 - spriteram[offs + 0x10],
				&spritevisiblearea,TRANSPARENCY_PEN,0);
	}
}
