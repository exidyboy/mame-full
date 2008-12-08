/***************************************************************************

  video/exidy.c

  Functions to emulate the video hardware of the Exidy Sorcerer

***************************************************************************/

#include "driver.h"
#include "includes/exidy.h"


VIDEO_UPDATE( exidy )
{
	int x,y;
	UINT16 pens[2];

	pens[0] = 0;
	pens[1] = 1;

	for (y=0; y<EXIDY_SCREEN_HEIGHT>>3; y++)
	{
		for (x=0; x<EXIDY_SCREEN_WIDTH>>3; x++)
		{
			int cheight, cwidth;
			int char_addr;
			int ch;

			/* get char from z80 address space */
			ch = memory_read_byte(cputag_get_address_space(screen->machine,"main",ADDRESS_SPACE_PROGRAM), 0x0f080 + (y<<6) + x) & 0x0ff;

			/* prom at 0x0f800, user chars from 0x0fc00 */
			char_addr = 0x0f800 | (ch<<3);

			for (cheight=0; cheight<8; cheight++)
			{
				int byte;
				int px,py;

				/* read byte of graphics data from z80 memory */
				/* either prom or ram */
				byte = memory_read_byte(cputag_get_address_space(screen->machine,"main",ADDRESS_SPACE_PROGRAM), char_addr|cheight);

				px = (x<<3);
				py = (y<<3)|cheight;
				for (cwidth=0; cwidth<8; cwidth++)
				{
					int pen;

					pen = (byte>>7) & 0x001;
					pen = pens[pen];

					*BITMAP_ADDR16(bitmap, py, px) = pen;
					px++;
					byte = byte<<1;
				}
			}
		}
	}
	return 0;
}
