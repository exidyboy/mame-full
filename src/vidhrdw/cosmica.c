/*************************************************************************
 Universal Cosmic Alien Driver
 (c)Lee Taylor May 1998, All rights reserved.

 For use only in offical Mame releases.
 Not to be distrabuted as part of any commerical work.
***************************************************************************
Functions to emulate the video hardware of the machine.
***************************************************************************/

#include "driver.h"
#include "vidhrdw/generic.h"

unsigned char *cosmicalien_videoram;

enum
{
	black, blue, red, purple, green, cyan, yellow, white, orange
};

static unsigned char bitmap_lookup[32][32] =
{
{0x0,0x0,0x0,0x0,0x62,0x62,0x32,0x32,0x32,0x32,0x32,0x12,0x12,0x12,0x12,0x12,0x72,0x72,0x72,0x72,0x72,0x72,0x72,0x72,0x72,0x51,0x51,0x51,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x51,0x51,0x51,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x11,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x17,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x66,0x66,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x55,0x55,0x55,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x55,0x55,0x55,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x76,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x15,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x32,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x22,0x77,0x77,0x77,0x77,0x77,0x77,0x77,0x0,0x0,0x0,0x0,},
{0x0,0x0,0x0,0x0,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x33,0x0,0x0,0x0,0x0,}
};




/***************************************************************************

  Start the video hardware emulation.

***************************************************************************/
int cosmicalien_vh_start(void)
{
	if ((tmpbitmap = osd_create_bitmap(Machine->drv->screen_width,Machine->drv->screen_height)) == 0)
		return 1;

	return 0;
}


/***************************************************************************

  Stop the video hardware emulation.

***************************************************************************/
void cosmicalien_vh_stop(void)
{
	osd_free_bitmap(tmpbitmap);
}



void cosmicalien_stars_w(int offset,int data)
{
}


void cosmicalien_videoram_w(int offset,int data)
{
	if (cosmicalien_videoram[offset] != data)
	{
		int i,x,y;


		cosmicalien_videoram[offset] = data;
                x = offset / 32 + 32;
		y = 256-8 - 8 * (offset % 32);

		for (i = 0;i < 8;i++)
		{

//                        if (data & 0x01) tmpbitmap->line[y][x] = Machine->pens[white];
                        if (data & 0x01) tmpbitmap->line[y][x] = Machine->pens[((bitmap_lookup[(y/8)& 0xf][(x/8)]) & 0x7)];
                        else tmpbitmap->line[y][x] = Machine->pens[black];

			osd_mark_dirty(x,y,x,y,0);

                        y++;
                        data >>= 1;
		}
	}
}


/***************************************************************************

  Draw the game screen in the given osd_bitmap.
  Do NOT call osd_update_display() from this function, it will be called by
  the main emulation engine.

***************************************************************************/
void cosmicalien_vh_screenrefresh(struct osd_bitmap *bitmap,int full_refresh)
{
	int offs;
	/* copy the bitmaped mapped graphics */
	copybitmap(bitmap,tmpbitmap,0,0,0,0,&Machine->drv->visible_area,TRANSPARENCY_NONE,0);

	/* Draw the sprites */
	for (offs = spriteram_size - 4;offs >= 0;offs -= 4)
	{
		int sx,sy,spritecode;


		sx = (spriteram[offs + 1]);
		sy =  spriteram[offs+2] - 16;

		spritecode =  ~spriteram[offs] & 0xff;

		drawgfx(bitmap,Machine->gfx[0],
				spritecode,
				spriteram[offs+3] & 0x0f,
				0,0,
				sx,sy,
				&Machine->drv->visible_area,TRANSPARENCY_PEN,0);
	}

}
