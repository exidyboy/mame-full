#include "driver.h"

/* Mame frontend interface rountines by Maurizio Zanello */

char messversion[8] = "0.1";

static struct RunningMachine machine;
struct RunningMachine *Machine = &machine;
static const struct GameDriver *gamedrv;
static const struct MachineDriver *drv;

int nocheat;    /* 0 when the -cheat option was specified */
int mame_debug; /* !0 when -debug option is specified */

int frameskip;
int framecount; /* MESS */
char rom_name[MAX_ROM][32]; /* MESS */
char floppy_name[MAX_FLOPPY][32]; /* MESS */
char hard_name[MAX_HARD][32]; /* MESS */
char cassette_name[MAX_CASSETTE][32]; /* MESS */
int VolumePTR = 0;
int CurrentVolume = 100;

unsigned char *RAM;
unsigned char *ROM;

FILE *errorlog;

void *record;   /* for -record */
void *playback; /* for -playback */

/* HJB 980601: should be set by osd_clearbitmap */
/* tested and reset by non gfxlayer vidhrdw drivers */
int   scrbitmap_dirty = 0;

int init_machine(void);
void shutdown_machine(void);
int run_machine(void);


int run_game(int game, struct GameOptions *options)
{
	int err;
	int i;

	errorlog   = options->errorlog;
	record     = options->record;
	playback   = options->playback;
	mame_debug = options->mame_debug;

	Machine->gamedrv = gamedrv = drivers[game];
	Machine->drv = drv = gamedrv->drv;

	/* copy configuration */
	Machine->sample_rate = options->samplerate;
	Machine->sample_bits = options->samplebits;
	frameskip = options->frameskip;
	nocheat = !options->cheat;

	/* get orientation right */
	Machine->orientation = gamedrv->orientation;
	if (options->norotate)
		Machine->orientation = ORIENTATION_DEFAULT;
	if (options->ror)
	{
		if (Machine->orientation & ORIENTATION_SWAP_XY)
			Machine->orientation ^= ORIENTATION_ROTATE_180;

		Machine->orientation ^= ORIENTATION_ROTATE_90;
	}
	if (options->rol)
	{
		if (Machine->orientation & ORIENTATION_SWAP_XY)
			Machine->orientation ^= ORIENTATION_ROTATE_180;

		Machine->orientation ^= ORIENTATION_ROTATE_270;
	}
	if (options->flipx)
		Machine->orientation ^= ORIENTATION_FLIP_X;
	if (options->flipy)
		Machine->orientation ^= ORIENTATION_FLIP_Y;


	/* Do the work*/
	err = 1;

	/* MESS - set up the storage peripherals */
	for (i = 0; i < MAX_ROM; i ++)
		strcpy (rom_name[i], options->rom_name[i]);
	for (i = 0; i < MAX_FLOPPY; i ++)
		strcpy (floppy_name[i], options->floppy_name[i]);
	for (i = 0; i < MAX_HARD; i ++)
		strcpy (hard_name[i], options->hard_name[i]);
	for (i = 0; i < MAX_CASSETTE; i ++)
		strcpy (cassette_name[i], options->cassette_name[i]);
	
	if (init_machine() == 0)
	{
		if (osd_init() == 0)
		{
			if (run_machine() == 0)
				err = 0;
			else printf("Unable to start machine emulation\n");

			osd_exit();
		}
		else printf ("Unable to initialize system\n");

		shutdown_machine();
	}
	else printf("Unable to initialize machine emulation\n");

	return err;
}



/***************************************************************************

  Initialize the emulated machine (load the roms, initialize the various
  subsystems...). Returns 0 if successful.

***************************************************************************/
int init_machine(void)
{
	if (gamedrv->new_input_ports)
	{
		int total;
		const struct InputPort *from;
		struct InputPort *to;

		from = gamedrv->new_input_ports;

		total = 0;
		do
		{
			total++;
		} while ((from++)->type != IPT_END);

		if ((Machine->input_ports = malloc(total * sizeof(struct InputPort))) == 0)
		{
			printf ("Failed to allocate input ports\n");
			return 1;
		}

		from = gamedrv->new_input_ports;
		to = Machine->input_ports;

		do
		{
			memcpy(to,from,sizeof(struct InputPort));

			to++;
		} while ((from++)->type != IPT_END);
	}


#ifdef MESS
	/* The ROM loading routine should allocate the ROM and RAM space and */
	/* assign them to the appropriate Machine->memory_region blocks. */
	if ((*gamedrv->rom_load)() != 0)
	{
		free(Machine->input_ports);
		printf("Image load failed.\n");
		return 1;
	}

#else
	if (readroms (gamedrv->rom, gamedrv->name) != 0)
	{
		free(Machine->input_ports);
		printf("Failed on ROM load.\n");
		return 1;
	}


	RAM = Machine->memory_region[drv->cpu[0].memory_region];
	ROM = RAM;

	/* decrypt the ROMs if necessary */
	if (gamedrv->rom_decode) (*gamedrv->rom_decode)();

	if (gamedrv->opcode_decode)
	{
		int j;


		/* find the first available memory region pointer */
		j = 0;
		while (Machine->memory_region[j]) j++;

		if ((ROM = malloc(0x10000)) == 0)
		{
			free(Machine->input_ports);
			/* TODO: should also free the allocated memory regions */
			return 1;
		}

		Machine->memory_region[j] = ROM;

		(*gamedrv->opcode_decode)();
	}
#endif


	/* read audio samples if available */
	Machine->samples = readsamples(gamedrv->samplenames,gamedrv->name);


	/* first of all initialize the memory handlers, which could be used by the */
	/* other initialization routines */
	cpu_init();

	/* ASG 971007 move from mame.c */
	if( !initmemoryhandlers() )
	{
		free(Machine->input_ports);
		printf("Failed on memory handler init.\n");
		return 1;
	}

	if (Machine->drv->sh_init && (*Machine->drv->sh_init)(gamedrv->name) != 0)
		/* TODO: should also free the resources allocated before */
		return 1;

	return 0;
}



void shutdown_machine(void)
{
	int i;

	/* free audio samples */
	freesamples(Machine->samples);
	Machine->samples = 0;

	/* ASG 971007 free memory element map */
	shutdownmemoryhandler();

	/* free the memory allocated for ROM and RAM */
	for (i = 0;i < MAX_MEMORY_REGIONS;i++)
	{
		free(Machine->memory_region[i]);
		Machine->memory_region[i] = 0;
	}

	/* free the memory allocated for input ports definition */
	free(Machine->input_ports);
	Machine->input_ports = 0;
}



static void vh_close(void)
{
	int i;


	for (i = 0;i < MAX_GFX_ELEMENTS;i++)
	{
		freegfx(Machine->gfx[i]);
		Machine->gfx[i] = 0;
	}
	freegfx(Machine->uifont);
	Machine->uifont = 0;
	osd_close_display();
	free_tile_layer(Machine->dirtylayer);
	Machine->dirtylayer = 0;
	palette_stop();
}



static int vh_open(void)
{
	int i;


	framecount = 0; /* MESS */
	for (i = 0;i < MAX_GFX_ELEMENTS;i++) Machine->gfx[i] = 0;
	Machine->uifont = 0;

	if (palette_start() != 0)
		goto badnews;

	/* convert the gfx ROMs into character sets. This is done BEFORE calling the driver's */
	/* convert_color_prom() routine (in palette_init()) because it might need to check the */
	/* Machine->gfx[] data */
	if (drv->gfxdecodeinfo)
	{
		for (i = 0;i < MAX_GFX_ELEMENTS && drv->gfxdecodeinfo[i].memory_region != -1;i++)
		{
			if ((Machine->gfx[i] = decodegfx(Machine->memory_region[drv->gfxdecodeinfo[i].memory_region]
					+ drv->gfxdecodeinfo[i].start,
					drv->gfxdecodeinfo[i].gfxlayout)) == 0)
				goto badnews;
			Machine->gfx[i]->colortable = &Machine->colortable[drv->gfxdecodeinfo[i].color_codes_start];
			Machine->gfx[i]->total_colors = drv->gfxdecodeinfo[i].total_color_codes;
		}
	}

	/* build our private user interface font */
	if ((Machine->uifont = builduifont()) == 0)
		goto badnews;


	/* if the GfxLayer system is enabled, create the dirty map needed by the */
	/* OS dependant code to selectively refresh the screen. */
	if (Machine->drv->layer)
	{
		static struct MachineLayer ml =
		{
			LAYER_TILE,
			0,0,	/* width and height, filled in later */
			/* all other fields are 0 */
		};

		ml.width = drv->screen_width;
		ml.height = drv->screen_height;
		if ((Machine->dirtylayer = create_tile_layer(&ml)) == 0)
			goto badnews;
	}
	else Machine->dirtylayer = 0;


	/* create the display bitmap, and allocate the palette */
	if ((Machine->scrbitmap = osd_create_display(
			drv->screen_width,drv->screen_height,
			drv->video_attributes)) == 0)
		goto badnews;

	/* initialize the palette - must be done after osd_create_display() */
	palette_init();

	return 0;

badnews:
	vh_close();
	printf ("vh_open failed!\n");
	return 1;
}



/***************************************************************************

  This function takes care of refreshing the screen, processing user input,
  and throttling the emulation speed to obtain the required frames per second.

***************************************************************************/
int updatescreen(void)
{
	static int showvoltemp = 0; /* MESS, M.Z.: new options */
#ifdef BETA_VERSION
	static int beta_count;
#endif

	/* LBO - moved most of the key-handling stuff to the OS routines so menu selections       */
	/* can get trapped as well rather than having a sick hack in the osd_key_pressed routine  */
        if (osd_handle_event())
	{
#ifdef BETA_VERSION
		beta_count = 0;
#endif
		return 1;
	}


	/* if the user pressed the reset key, reset the emulation */
        if (osd_key_pressed(UI_KEY_RESET))
		machine_reset();


	if (++framecount > frameskip)
	{
		framecount = 0;

		(*drv->vh_update)(Machine->scrbitmap);  /* update screen */

		/* This call is for the cheat, it must be called at least each frames */
		if (nocheat == 0) DoCheat(CurrentVolume);

#ifdef BETA_VERSION
		if (beta_count < 5 * Machine->drv->frames_per_second)
		{
			beta_count += frameskip+1;

			if (beta_count < 5 * Machine->drv->frames_per_second)
			{
				int trueorientation;
				int i,x;
				char volstr[25];


				trueorientation = Machine->orientation;
				Machine->orientation = ORIENTATION_DEFAULT;

				x = (Machine->uiwidth - 12*Machine->uifont->width)/2;
				strcpy(volstr,"BETA VERSION");

				for (i = 0;i < 12;i++)
					drawgfx(Machine->scrbitmap,Machine->uifont,(unsigned int)volstr[i],DT_COLOR_RED,
						0,0,x+(i+1)*Machine->uifont->width+Machine->uixmin,Machine->uiheight/2+Machine->uiymin,0,TRANSPARENCY_NONE,0);

				Machine->orientation = trueorientation;
			}
			else
				osd_clearbitmap(Machine->scrbitmap);
		}
#endif

		osd_poll_joystick();

		osd_update_display();
	}

	/* update audio. Do it after the speed throttling to be in better sync. */
/*	ASG 980417 -- moved to the update function in cpuintrf.c
	sound_update();*/

	return 0;
}



/***************************************************************************

  Run the emulation. Start the various subsystems and the CPU emulation.
  Returns non zero in case of error.

***************************************************************************/
int run_machine(void)
{
	int res = 1;


	if (vh_open() == 0)
	{
		if (drv->vh_start == 0 || (*drv->vh_start)() == 0)      /* start the video hardware */
		{
			if (sound_start() == 0) /* start the audio hardware */
			{
				int i;
				struct DisplayText dt[2];


/* MESS */
#ifndef macintosh /* LBO - This text is displayed in a dialog box. */
				dt[0].text = "PLEASE DO NOT DISTRIBUTE THE SOURCE CODE AND/OR THE EXECUTABLE "
						"APPLICATION WITH ANY IMAGES OF COMPUTER/CONSOLE MEDIA.\n"
						"DOING AS SUCH WILL HARM ANY FURTHER DEVELOPMENT OF MESS AND COULD "
						"RESULT IN LEGAL ACTION BEING TAKEN BY THE LAWFUL COPYRIGHT HOLDERS "
						"OF THE ORIGINAL MEDIA.\n\n"
						"IF YOU DO NOT AGREE WITH THESE CONDITIONS THEN PLEASE PRESS ESC NOW.";

				dt[0].color = DT_COLOR_RED;
				dt[0].x = 0;
				dt[0].y = 0;
				dt[1].text = 0;
				displaytext(dt,1);

				i = osd_read_key();
				while (osd_key_pressed(i));             /* wait for key release */
                                if (i != UI_KEY_ESCAPE)
#endif
				{

					showcredits();  /* show the driver credits */

					showgameinfo();  /* show info about the game */

					osd_clearbitmap(Machine->scrbitmap);
					osd_update_display();

#ifndef MESS
					/* free the graphics ROMs, they are no longer needed */
					/* TODO: instead of hardcoding region 1, use a flag to mark regions */
					/*       which can be freed after initialization. */
					free(Machine->memory_region[1]);
					Machine->memory_region[1] = 0;
#endif

					/* load input ports settings (keys, dip switches, and so on) */
					load_input_port_settings();

					if (nocheat == 0) InitCheat();

					cpu_run();      /* run the emulation! */

					if (nocheat == 0) StopCheat();

					/* save input ports settings */
					save_input_port_settings();

					/* the following MUST be done after hiscore_save() otherwise */
					/* some 68000 games will not work */
					sound_stop();
					if (drv->vh_stop) (*drv->vh_stop)();
				}

				res = 0;
			}
			else printf("Unable to start audio emulation\n");
		}
		else printf("Unable to start video emulation\n");

		vh_close();
	}
	else printf("Unable to initialize display\n");

	return res;
}



int mame_highscore_enabled(void)
{
	/* disable high score when record/playback is on */
	if (record != 0 || playback != 0) return 0;

	/* disable high score when cheats are used */
	if (he_did_cheat != 0) return 0;

	return 1;
}
