#include "driver.h"
#include "vidhrdw/generic.h"
#include "includes/channelf.h"

int channelf_val_reg;
int channelf_row_reg;
int channelf_col_reg;

static UINT8 channelf_palette[] = {
	0x00, 0x00, 0x00,	/* black */
	0xff, 0xff, 0xff,	/* white */
	0xff, 0x00, 0x00,	/* red	 */
	0x00, 0xff, 0x00,	/* green */
	0x00, 0x00, 0xff,	/* blue  */
	0xbf, 0xbf, 0xbf,	/* ltgray  */
	0xbf, 0xff, 0xbf,	/* ltgreen */
	0xbf, 0xbf, 0xff	/* ltblue  */
};

#define BLACK	0
#define WHITE   1
#define RED     2
#define GREEN   3
#define BLUE    4
#define LTGRAY  5
#define LTGREEN 6
#define LTBLUE	7

static UINT16 colormap[] = {
	BLACK,   WHITE, WHITE, WHITE,
	LTBLUE,  BLUE,  RED,   GREEN,
	LTGRAY,  BLUE,  RED,   GREEN,
	LTGREEN, BLUE,  RED,   GREEN,
};

/* Initialise the palette */
PALETTE_INIT( channelf )
{
	palette_set_colors(0, channelf_palette, sizeof(channelf_palette) / 3);
	memcpy(colortable,colormap,0);
}

VIDEO_START( channelf )
{
	videoram_size = 0x2000;
	videoram = auto_malloc(videoram_size);
	return video_start_generic();
}

static void plot_4_pixel(int x, int y, int color)
{
	int pen;

	if (x < Machine->visible_area.min_x ||
		x + 1 >= Machine->visible_area.max_x ||
		y < Machine->visible_area.min_y ||
		y + 1 >= Machine->visible_area.max_y)
		return;

	if (color >= 16)
		return;

    pen = Machine->pens[colormap[color]];

	plot_pixel(Machine->scrbitmap, x, y, pen);
	plot_pixel(Machine->scrbitmap, x+1, y, pen);
	plot_pixel(Machine->scrbitmap, x, y+1, pen);
	plot_pixel(Machine->scrbitmap, x+1, y+1, pen);
}

int recalc_palette_offset(int reg1, int reg2)
{
	/* Note: This is based on the very strange decoding they    */
	/*       used to determine which palette this line is using */

	switch(reg1*4+reg2)
	{
		case 0:
			return 0;
		case 8:
			return 4;
		case 3:
			return 8;
		case 15:
			return 12;
		default:
			return 0; /* This should be an error condition */
	}
}

VIDEO_UPDATE( channelf )
{
	int x,y,offset, palette_offset;
	int full_refresh = 1;

	for(y=0;y<64;y++)
	{
		palette_offset = recalc_palette_offset(videoram[y*128+125]&3,videoram[y*128+126]&3);
		for (x=0;x<128;x++)
		{
			offset = y*128+x;
			if ( full_refresh || dirtybuffer[offset] )
				plot_4_pixel(x*2, y*2, palette_offset+(videoram[offset]&3));
		}
	}
}


