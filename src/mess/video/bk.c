/***************************************************************************

        BK video driver by Miodrag Milanovic

        10/03/2008 Preliminary driver.

****************************************************************************/


#include "emu.h"
#include "includes/bk.h"

UINT16 *bk0010_video_ram;

VIDEO_START( bk0010 )
{
}

VIDEO_UPDATE( bk0010 )
{
	UINT16 code;
	int y, x, b;
	int nOfs;

	nOfs = (bk_scrool - 728) % 256;

	for (y = 0; y < 256; y++)
	{
		for (x = 0; x < 32; x++)
		{
			code = bk0010_video_ram[((y+nOfs) %256)*32 + x];
			for (b = 0; b < 16; b++)
			{
				*BITMAP_ADDR16(bitmap, y, x*16 + b) =  (code >> b) & 0x01;
			}
		}
	}
	return 0;
}
