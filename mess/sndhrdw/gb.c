/**************************************************************************************
* Gameboy sound emulation (c) Anthony Kruize (trandor@labyrinth.net.au)
*
* Anyways, sound on the gameboy consists of 4 separate 'channels'
*   Sound1 = Quadrangular waves with SWEEP and ENVELOPE functions  (NR10,11,12,13,14)
*   Sound2 = Quadrangular waves with ENVELOPE functions (NR21,22,23,24)
*   Sound3 = Wave patterns from WaveRAM (NR30,31,32,33,34)
*   Sound4 = White noise with an envelope (NR41,42,43,44)
*
* Each sound channel has 2 modes, namely ON and OFF...  whoa
*
* These tend to be the two most important equations in
* converting between Hertz and GB frequency registers:
* (Sounds will have a 2.4% higher frequency on Super GB.)
*       gb = 2048 - (131072 / Hz)
*       Hz  = 131072 / (2048 - gb)
*
***************************************************************************************/

#include <math.h>
#include "driver.h"
#include "includes/gb.h"

#define NR10 0xFF10
#define NR11 0xFF11
#define NR12 0xFF12
#define NR13 0xFF13
#define NR14 0xFF14
#define NR21 0xFF16
#define NR22 0xFF17
#define NR23 0xFF18
#define NR24 0xFF19
#define NR30 0xFF1A
#define NR31 0xFF1B
#define NR32 0xFF1C
#define NR33 0xFF1D
#define NR34 0xFF1E
#define NR41 0xFF20
#define NR42 0xFF21
#define NR43 0xFF22
#define NR44 0xFF23
#define NR50 0xFF24
#define NR51 0xFF25
#define NR52 0xFF26

#define LEFT 1
#define RIGHT 2

static int channel = 1;
static int rate;

/* Table of wave duties.
   Represents wave duties of 12.5%, 25%, 50% and 75% */
static float wave_duty_table[4] = { 8, 4, 2, 1.33 };
static INT32 env_length_table[8];
static INT32 swp_time_table[8];
static UINT32 period_table[2048];
static UINT32 period_mode3_table[2048];
static UINT32 length_table[64];
static UINT32 length_mode3_table[256];

struct SOUND1
{
	UINT8 on;
	UINT8 channel;
	INT32 length;
	INT32 pos;
	UINT32 period;
	UINT32 frequency;
	INT32 count;
	INT8 signal;
	INT8 mode;
	INT8 duty;
	INT32 env_value;
	INT8 env_direction;
	INT32 env_length;
	INT32 env_count;
	INT32 swp_shift;
	INT32 swp_direction;
	INT32 swp_time;
	INT32 swp_count;
};

struct SOUND2
{
	UINT8 on;
	UINT8 channel;
	INT32 length;
	INT32 pos;
	UINT32 period;
	INT32 count;
	INT8 signal;
	INT8 mode;
	INT8 duty;
	INT32 env_value;
	INT8 env_direction;
	INT32 env_length;
	INT32 env_count;
};

struct SOUND3
{
	UINT8 on;
	UINT8 channel;
	INT32 length;
	INT32 pos;
	UINT32 period;
	INT32 count;
	UINT8 offset;
	INT8 duty;
	INT8 mode;
	INT8 level;
};

struct SOUND4
{
	UINT8 on;
	UINT8 channel;
	INT32 length;
	INT32 frequency;
	INT32 count;
	INT16 signal;
	INT8 mode;
	INT32 env_value;
	INT8 env_direction;
	INT32 env_length;
	INT32 env_count;
	INT32 ply_frequency;
	INT32 ply_step;
	INT32 ply_ratio;
};

struct SOUNDC
{
	UINT8 on;
	UINT8 vol_left;
	UINT8 vol_right;
	UINT8 mode1_left;
	UINT8 mode1_right;
	UINT8 mode2_left;
	UINT8 mode2_right;
	UINT8 mode3_left;
	UINT8 mode3_right;
	UINT8 mode4_left;
	UINT8 mode4_right;
};

static struct SOUND1 snd_1;
static struct SOUND2 snd_2;
static struct SOUND3 snd_3;
static struct SOUND4 snd_4;
static struct SOUNDC snd_control;

void gameboy_init_1(void)
{
	if( !snd_1.on )
		snd_1.pos = 0;
	snd_1.on = 1;
	snd_1.count = 0;
	snd_1.env_count = 0;
	snd_1.swp_count = 0;
	snd_1.signal = 0x1;
	gb_ram[NR52] |= 0x1;
}

void gameboy_init_2(void)
{
	if( !snd_2.on )
		snd_2.pos = 0;
	snd_2.on = 1;
	snd_2.count = 0;
	snd_2.env_count = 0;
	snd_2.signal = 0x1;
	gb_ram[NR52] |= 0x2;
}

void gameboy_3_init(void)
{
	if( !snd_3.on )
	{
		snd_3.pos = 0;
		snd_3.offset = 0;
		snd_3.duty = 0;
	}
	snd_3.on = 1;
	snd_3.count = 0;
	gb_ram[NR52] |= 0x4;
}

void gameboy_4_init(void)
{
	snd_4.on = 1;
	snd_4.count = 0;
	snd_4.env_count = 0;
	gb_ram[NR52] |= 0x8;
}

void gameboy_update(int param, INT16 **buffer, int length);

void gameboy_sound_w(int offset, int data)
{
	/* change in registers so update first */
	stream_update(channel,0);

	/* Only register NR52 is accessible if the sound controller is disabled */
	if( !snd_control.on && offset != NR52 )
	{
		return;
	}

	switch( offset )
	{
	/*MODE 1 */
	case NR10: /* Sweep (R/W) */
		snd_1.swp_shift = data & 0x7;
		snd_1.swp_direction = (data & 0x8) >> 3;
		snd_1.swp_direction |= snd_1.swp_direction - 1;
		snd_1.swp_time = swp_time_table[ (data & 0x70) >> 4 ];
		break;
	case NR11: /* Sound length/Wave pattern duty (R/W) */
		snd_1.duty = (data & 0xC0) >> 6;
		snd_1.length = length_table[data & 0x3F];
		break;
	case NR12: /* Envelope (R/W) */
		snd_1.env_value = data >> 4;
		snd_1.env_direction = (data & 0x8) >> 3;
		snd_1.env_direction |= snd_1.env_direction - 1;
		snd_1.env_length = env_length_table[data & 0x7];
		break;
	case NR13: /* Frequency lo (R/W) */
		snd_1.frequency = ((gb_ram[NR14]&0x7)<<8) | gb_ram[NR13];
		snd_1.period = period_table[snd_1.frequency];
		break;
	case NR14: /* Frequency hi / Initialize (R/W) */
		snd_1.frequency = ((gb_ram[NR14]&0x7)<<8) | gb_ram[NR13];
		snd_1.period = period_table[snd_1.frequency];
		if( data & 0x80 )
			gameboy_init_1();
		break;

	/*MODE 2 */
	case NR21: /* Sound length/Wave pattern duty (R/W) */
		snd_2.duty = (data & 0xC0) >> 6;
		snd_2.length = length_table[data & 0x3F];
		break;
	case NR22: /* Envelope (R/W) */
		snd_2.env_value = data >> 4;
		snd_2.env_direction = (data & 0x8 ) >> 3;
		snd_2.env_direction |= snd_2.env_direction - 1;
		snd_2.env_length = env_length_table[data & 0x7];
		break;
	case NR23: /* Frequency lo (R/W) */
		snd_2.period = period_table[((gb_ram[NR24]&0x7)<<8) | gb_ram[NR23]];
		break;
	case NR24: /* Frequency hi / Initialize (R/W) */
		snd_2.mode = (data & 0x40) >> 6;
		snd_2.period = period_table[((gb_ram[NR24]&0x7)<<8) | gb_ram[NR23]];
		if( data & 0x80 )
			gameboy_init_2();
		break;

	/*MODE 3 */
	case NR30: /* Sound On/Off (R/W) */
		snd_3.on = (data & 0x80) >> 7;
		break;
	case NR31: /* Sound Length (R/W) */
		snd_3.length = length_mode3_table[data];
		break;
	case NR32: /* Select Output Level */
		snd_3.level = (data & 0x60) >> 5;
		break;
	case NR33: /* Frequency lo (W) */
		snd_3.period = period_mode3_table[((gb_ram[NR34]&0x7)<<8) + gb_ram[NR33]];
		break;
	case NR34: /* Frequency hi / Initialize (W) */
		snd_3.mode = (data & 0x40) >> 6;
		snd_3.period = period_mode3_table[((gb_ram[NR34]&0x7)<<8) + gb_ram[NR33]];
		if( data & 0x80 )
			gameboy_3_init();
		break;

	/*MODE 4 */
	case NR41: /* Sound Length (R/W) */
		snd_4.length = length_table[data & 0x3F];
		break;
	case NR42: /* Envelope (R/W) */
		snd_4.env_value = data >> 4;
		snd_4.env_direction = (data & 0x8 ) >> 3;
		snd_4.env_direction |= snd_4.env_direction - 1;
		snd_4.env_length = env_length_table[data & 0x7];
		break;
	case NR43: /* Polynomial Counter/Frequency */
		/* TODO: NEED TO SET POLYNOMIAL STUFF HERE */
		break;
	case NR44: /* Counter/Consecutive / Initialize (R/W)  */
		snd_4.mode = (data & 0x40) >> 6;
		if( data & 0x80 )
			gameboy_4_init();
		break;

	/* CONTROL */
	case NR50: /* Channel Control / On/Off / Volume (R/W)  */
		snd_control.vol_left = data & 0x7;
		snd_control.vol_right = (data & 0x70) >> 4;
		break;
	case NR51: /* Selection of Sound Output Terminal */
		snd_control.mode1_right = data & 0x1;
		snd_control.mode1_left = (data & 0x10) >> 4;
		snd_control.mode2_right = (data & 0x2) >> 1;
		snd_control.mode2_left = (data & 0x20) >> 5;
		snd_control.mode3_right = (data & 0x4) >> 2;
		snd_control.mode3_left = (data & 0x40) >> 6;
		snd_control.mode4_right = (data & 0x8) >> 3;
		snd_control.mode4_left = (data & 0x80) >> 7;
		break;
	case NR52: /* Sound On/Off (R/W) */
		/* Only bit 7 is writable, writing to bits 0-3 does NOT enable or
		   disable sound.  They are read-only */
		snd_control.on = (data & 0x80) >> 7;
		if( !snd_control.on )
		{
			snd_1.on = 0;
			snd_2.on = 0;
			snd_3.on = 0;
			snd_4.on = 0;
			gb_ram[offset] = 0;
		}
		break;
	}
}

void gameboy_update(int param, INT16 **buffer, int length)
{
	INT16 sample, left, right;

	while( length-- > 0 )
	{
		left = right = 0;

		/* Mode 1 - Wave with Envelope and Sweep */
		if( snd_1.on )
		{
			sample = snd_1.signal & snd_1.env_value;
			snd_1.pos++;
			if( snd_1.pos == (UINT32)(snd_1.period / wave_duty_table[snd_1.duty]) >> 16)
			{
				snd_1.signal = -snd_1.signal;
			}
			else if( snd_1.pos > (snd_1.period >> 16) )
			{
				snd_1.pos = 0;
				snd_1.signal = -snd_1.signal;
			}

			if( snd_1.length && snd_1.mode )
			{
				snd_1.count++;
				if( snd_1.count >= snd_1.length )
				{
					snd_1.on = 0;
					gb_ram[NR52] &= 0xFE;
				}
			}

			if( snd_1.env_length )
			{
				snd_1.env_count++;
				if( snd_1.env_count >= snd_1.env_length )
				{
					snd_1.env_count = 0;
					snd_1.env_value += snd_1.env_direction;
					if( snd_1.env_value < 0 )
						snd_1.env_value = 0;
					if( snd_1.env_value > 15 )
						snd_1.env_value = 15;
				}
			}

			/* Is this correct? */
			if( snd_1.swp_time && snd_1.swp_shift )
			{
				snd_1.swp_count++;
				if( snd_1.swp_count >= snd_1.swp_time )
				{
					snd_1.swp_count = 0;
					if( snd_1.swp_direction > 0 )
					{
						snd_1.frequency -= snd_1.frequency / (1 << snd_1.swp_shift );
						if( snd_1.frequency <= 0 )
						{
							snd_1.on = 0;
							gb_ram[NR52] &= 0xFE;
						}
					}
					else
						snd_1.frequency += snd_1.frequency / (1 << snd_1.swp_shift );

					snd_1.period = period_table[snd_1.frequency];
				}
			}

			if( snd_control.mode1_left )
				left += sample;
			if( snd_control.mode1_right )
				right += sample;
		}

		/* Mode 2 - Wave with Envelope */
		if( snd_2.on )
		{
			sample = snd_2.signal & snd_2.env_value;
			snd_2.pos++;
			if( snd_2.pos == (UINT32)(snd_2.period / wave_duty_table[snd_2.duty]) >> 16)
			{
				snd_2.signal = -snd_2.signal;
			}
			else if( snd_2.pos > (snd_2.period >> 16) )
			{
				snd_2.pos = 0;
				snd_2.signal = -snd_2.signal;
			}

			if( snd_2.length && snd_2.mode )
			{
				snd_2.count++;
				if( snd_2.count >= snd_2.length )
				{
					snd_2.on = 0;
					gb_ram[NR52] &= 0xFD;
				}
			}

			if( snd_2.env_length )
			{
				snd_2.env_count++;
				if( snd_2.env_count > snd_2.env_length )
				{
					snd_2.env_count = 0;
					snd_2.env_value += snd_2.env_direction;
					if( snd_2.env_value < 0 )
						snd_2.env_value = 0;
					if( snd_2.env_value > 15 )
						snd_2.env_value = 15;
				}
			}

			if( snd_control.mode2_left )
				left += sample;
			if( snd_control.mode2_right )
				right += sample;
		}

		/* Mode 3 - Wave patterns from WaveRAM */
		if( snd_3.on )
		{
			/* NOTE: This is close, but not quite right.
			   The problem is that the calculation for the period
			   is too course, resulting in wrong notes occasionally.
			   The most common side effect is the same note played twice */
			sample = gb_ram[0xFF30 + snd_3.offset];
			if( !snd_3.duty )
			{
				sample >>= 4;
			}

			sample &= 0xF;
			sample -= 8;

			if( snd_3.level )
				sample >>= snd_3.level - 1;
			else
				sample = 0;

			snd_3.pos++;
			if( snd_3.pos > (UINT32)(((snd_3.period ) >> 16 ) / 32) )
/*			if( (snd_3.pos<<16) >= (UINT32)(((snd_3.period / 31) + (1<<16))) ) */
			{
				snd_3.pos = 0;
				snd_3.duty = !snd_3.duty;
				if( !snd_3.duty )
				{
					snd_3.offset++;
					if( snd_3.offset > 0xF )
						snd_3.offset = 0;
				}
			}

			if( snd_3.length && snd_3.mode )
			{
				snd_3.count++;
				if( snd_3.count >= snd_3.length )
				{
					snd_3.on = 0;
					gb_ram[NR52] &= 0xFB;
				}
			}

			if( snd_control.mode3_left )
				left += sample;
			if( snd_control.mode3_right )
				right += sample;
		}

		/* Mode 4 - Noise with Envelope */
		if( snd_4.on )
		{
			/* Below is a hack, a big hack, but it will do until we figure out
			   a proper polynomial white noise generator. */
			sample = rand() & snd_4.env_value;

			if( snd_4.length && snd_4.mode )
			{
				snd_4.count++;
				if( snd_4.count >= snd_4.length )
				{
					snd_4.on = 0;
					gb_ram[NR52] &= 0xF7;
				}
			}

			if( snd_4.env_length )
			{
				snd_4.env_count++;
				if( snd_4.env_count >= snd_4.env_length )
				{
					snd_4.env_count = 0;
					snd_4.env_value += snd_4.env_direction;
					if( snd_4.env_value < 0 )
						snd_4.env_value = 0;
					if( snd_4.env_value > 15 )
						snd_4.env_value = 15;
				}
			}

			if( snd_control.mode4_left )
				left += sample;
			if( snd_control.mode4_right )
				right += sample;
		}

		/* Adjust for master volume */
		left *= snd_control.vol_left;
		right *= snd_control.vol_right;

		/* pump up the volume */
		left <<= 6;
		right <<= 6;

		/* Update the buffers */
		*(buffer[0]++) = left;
		*(buffer[1]++) = right;
	}

	gb_ram[NR52] = (gb_ram[NR52]&0xf0) | snd_1.on | (snd_2.on << 1) | (snd_3.on << 2) | (snd_4.on << 3);
}

int gameboy_sh_start(const struct MachineSound* driver)
{
	int n;
	const char *names[2] = { "Gameboy left", "Gameboy right" };
	const int volume[2] = { MIXER( 50, MIXER_PAN_LEFT ), MIXER( 50, MIXER_PAN_RIGHT ) };

	memset(&snd_1, 0, sizeof(snd_1));
	memset(&snd_2, 0, sizeof(snd_2));
	memset(&snd_3, 0, sizeof(snd_3));
	memset(&snd_4, 0, sizeof(snd_4));

	channel = stream_init_multi(2, names, volume, Machine->sample_rate, 0, gameboy_update);

	rate = Machine->sample_rate;

	/* Calculate the envelope and sweep tables */
	for( n = 0; n < 8; n++ )
	{
		env_length_table[n] = (n * ((1 << 16) / 64) * rate) >> 16;
		swp_time_table[n] = (((n << 16) / 128) * rate) >> 15;
	}

	/* Calculate the period tables */
	for( n = 0; n < 2048; n++ )
	{
		period_table[n] = ((1 << 16) / (131072 / (2048 - n))) * rate;
		period_mode3_table[n] = ((1 << 16) / (65536 / (2048 - n))) * rate;
	}

	/* Calculate the length table */
	for( n = 0; n < 64; n++ )
	{
		length_table[n] = ((64 - n) * ((1 << 16)/256) * rate) >> 16;
	}

	/* Calculate the length table for mode 3 */
	for( n = 0; n < 64; n++ )
	{
		length_mode3_table[n] = ((256 - n) * ((1 << 16)/2) * rate) >> 16;
/*		length_mode3_table[n] = ((256 - n) * ((1 << 16)/256) * rate) >> 16; */
	}

	return 0;
}
