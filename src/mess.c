#include "driver.h"

/* Mame frontend interface rountines by Maurizio Zanello */

char messversion[] = "0.2 ("__DATE__")";

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
static int settingsloaded;

int bitmap_dirty;	/* set by layer_mark_full_screen_dirty() */

unsigned char *ROM;

FILE *errorlog;

void *record;   /* for -record */
void *playback; /* for -playback */


int init_machine(void);
void shutdown_machine(void);
int run_machine(void);


int run_game(int game, struct GameOptions *options)
{
	int err;

	if (game < 0)
	{
		printf ("A valid system was not chosen.\n");
		return 1;
	}
	
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
		/* if only one of the components is inverted, switch them */
		if ((Machine->orientation & ORIENTATION_ROTATE_180) == ORIENTATION_FLIP_X ||
				(Machine->orientation & ORIENTATION_ROTATE_180) == ORIENTATION_FLIP_Y)
			Machine->orientation ^= ORIENTATION_ROTATE_180;

		Machine->orientation ^= ORIENTATION_ROTATE_90;
	}
	if (options->rol)
	{
		/* if only one of the components is inverted, switch them */
		if ((Machine->orientation & ORIENTATION_ROTATE_180) == ORIENTATION_FLIP_X ||
				(Machine->orientation & ORIENTATION_ROTATE_180) == ORIENTATION_FLIP_Y)
			Machine->orientation ^= ORIENTATION_ROTATE_180;

		Machine->orientation ^= ORIENTATION_ROTATE_270;
	}
	if (options->flipx)
		Machine->orientation ^= ORIENTATION_FLIP_X;
	if (options->flipy)
		Machine->orientation ^= ORIENTATION_FLIP_Y;


	/* Do the work*/
	err = 1;

{
	int i;
	
	/* MESS - set up the storage peripherals */
	for (i = 0; i < MAX_ROM; i ++)
		strcpy (rom_name[i], options->rom_name[i]);
	for (i = 0; i < MAX_FLOPPY; i ++)
		strcpy (floppy_name[i], options->floppy_name[i]);
	for (i = 0; i < MAX_HARD; i ++)
		strcpy (hard_name[i], options->hard_name[i]);
	for (i = 0; i < MAX_CASSETTE; i ++)
		strcpy (cassette_name[i], options->cassette_name[i]);
}
	
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


	if (gamedrv->rom)
	{
		if (readroms () != 0)
		{
			free(Machine->input_ports);
			printf("Failed on mandatory ROM load.\n");
			return 1;
		}

{
		extern unsigned char *RAM;
		RAM = Machine->memory_region[drv->cpu[0].memory_region];
		ROM = RAM;
}
		/* decrypt the ROMs if necessary */
		if (gamedrv->rom_decode) (*gamedrv->rom_decode)();

		if (gamedrv->opcode_decode)
		{
			int j;


			/* find the first available memory region pointer */
			j = 0;
			while (Machine->memory_region[j]) j++;

			/* allocate a ROM array of the same length of memory region #0 */
			if ((ROM = malloc(gamedrv->rom->offset)) == 0)
			{
				free(Machine->input_ports);
				/* TODO: should also free the allocated memory regions */
				return 1;
			}

			Machine->memory_region[j] = ROM;

			(*gamedrv->opcode_decode)();
		}
	}


	/* read audio samples if available */
	Machine->samples = readsamples(gamedrv->samplenames,gamedrv->name);


	/* first of all initialize the memory handlers, which could be used by the */
	/* other initialization routines */
	cpu_init();

	/* load input ports settings (keys, dip switches, and so on) */
	settingsloaded = load_input_port_settings();

#ifdef MESS
	/* The ROM loading routine should allocate the ROM and RAM space and */
	/* assign them to the appropriate Machine->memory_region blocks. */
	if (*gamedrv->rom_load && (*gamedrv->rom_load)() != 0)
	{
		free(Machine->input_ports);
		printf("Image load failed.\n");
		return 1;
	}
#endif

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
	static int showvoltemp = 0; /* M.Z.: new options */
	static int need_to_clear_bitmap;
#ifdef BETA_VERSION
	static int beta_count;
#endif

	int skipme = 1;


	/* if the user pressed KEY_FAST_EXIT, stop the emulation */
	if (osd_key_pressed(OSD_KEY_FAST_EXIT))
	{
#ifdef BETA_VERSION
		beta_count = 0;
#endif
		return 1;
	}


	/* if the user pressed KEY_RESET_MACHINE, reset the emulation */
	if (osd_key_pressed(OSD_KEY_RESET_MACHINE))
	{
		while (osd_key_pressed(OSD_KEY_RESET_MACHINE));
		machine_reset();
	}


	if (osd_key_pressed(OSD_KEY_VOLUME_DOWN))
	{
		/* decrease volume */
		if (CurrentVolume > 0) CurrentVolume--;
		osd_set_mastervolume(CurrentVolume);
		showvoltemp = 50;
	}

	if (osd_key_pressed(OSD_KEY_VOLUME_UP))
	{
		/* increase volume */
		if (CurrentVolume < 100) CurrentVolume++;
		osd_set_mastervolume(CurrentVolume);
		showvoltemp = 50;
	}                                          /* MAURY_END: new options */

	if (osd_key_pressed(OSD_KEY_PAUSE)) /* pause the game */
	{
		struct DisplayText dt[2];
		int count = 0;


		dt[0].text = "PAUSED";
		dt[0].color = DT_COLOR_RED;
		dt[0].x = (Machine->uiwidth - Machine->uifont->width * strlen(dt[0].text)) / 2;
		dt[0].y = (Machine->uiheight - Machine->uifont->height) / 2;
		dt[1].text = 0;

		osd_set_mastervolume(0);

		while (osd_key_pressed(OSD_KEY_PAUSE))
			osd_update_audio();     /* give time to the sound hardware to apply the volume change */

		while (osd_key_pressed(OSD_KEY_PAUSE) == 0 && osd_key_pressed(OSD_KEY_UNPAUSE) == 0)
		{
			if (osd_key_pressed(OSD_KEY_CONFIGURE)) setup_menu(); /* call the configuration menu */

			osd_clearbitmap(Machine->scrbitmap);
			(*drv->vh_update)(Machine->scrbitmap,1);  /* redraw screen */

			if (count < Machine->drv->frames_per_second / 2)
				displaytext(dt,0);      /* make PAUSED blink */
			else
				osd_update_display();

			count = (count + 1) % (Machine->drv->frames_per_second / 1);
		}

		while (osd_key_pressed(OSD_KEY_UNPAUSE)) ;	 /* wait for key release */
		while (osd_key_pressed(OSD_KEY_PAUSE)) ;     /* ditto */

		osd_set_mastervolume(CurrentVolume);
		osd_clearbitmap(Machine->scrbitmap);
	}

	/* if the user pressed KEY_CONFIGURE, go to the setup menu */
	if (osd_key_pressed(OSD_KEY_CONFIGURE))
	{
		osd_set_mastervolume(0);

		while (osd_key_pressed(OSD_KEY_CONFIGURE))
			osd_update_audio();     /* give time to the sound hardware to apply the volume change */

		if (setup_menu()) return 1;

		osd_set_mastervolume(CurrentVolume);
	}

	/* if the user pressed KEY_SHOW_GFX, show the character set */
	if (osd_key_pressed(OSD_KEY_SHOW_GFX))
	{
		osd_set_mastervolume(0);

		while (osd_key_pressed(OSD_KEY_SHOW_GFX))
			osd_update_audio();     /* give time to the sound hardware to apply the volume change */

		if (showcharset()) return 1;

		osd_set_mastervolume(CurrentVolume);
	}

	/* see if we recomend skipping this frame */
	if (++framecount > frameskip)
	{
		framecount = 0;
		skipme = 0;
	}
	skipme = osd_skip_this_frame (skipme);
	
	/* if not, go for it */
	if (!skipme)
	{
		if (need_to_clear_bitmap)
		{
			osd_clearbitmap(Machine->scrbitmap);
			need_to_clear_bitmap = 0;
		}

		(*drv->vh_update)(Machine->scrbitmap,bitmap_dirty);  /* update screen */
		bitmap_dirty = 0;

		/* This call is for the cheat, it must be called at least each frames */
		if (nocheat == 0) DoCheat(CurrentVolume);

		if (showvoltemp)
		{                     /* volume-meter */
			int trueorientation;
			int i,x;
			char volstr[25];


			showvoltemp--;

			if (showvoltemp)
			{
				trueorientation = Machine->orientation;
				Machine->orientation = ORIENTATION_DEFAULT;

				x = (Machine->uiwidth - 24*Machine->uifont->width)/2;
				strcpy(volstr,"                      ");
				for (i = 0;i < (CurrentVolume/5);i++) volstr[i+1] = '\x15';

				drawgfx(Machine->scrbitmap,Machine->uifont,16,DT_COLOR_RED,0,0,x,Machine->uiheight/2+Machine->uiymin,0,TRANSPARENCY_NONE,0);
				drawgfx(Machine->scrbitmap,Machine->uifont,17,DT_COLOR_RED,0,0,x+23*Machine->uifont->width,Machine->uiheight/2+Machine->uiymin,0,TRANSPARENCY_NONE,0);
				for (i = 0;i < 22;i++)
					drawgfx(Machine->scrbitmap,Machine->uifont,(unsigned int)volstr[i],DT_COLOR_WHITE,
						0,0,x+(i+1)*Machine->uifont->width+Machine->uixmin,Machine->uiheight/2+Machine->uiymin,0,TRANSPARENCY_NONE,0);

				Machine->orientation = trueorientation;
			}
			else
				need_to_clear_bitmap = 1;
		}

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
				need_to_clear_bitmap = 1;
		}
#endif

		osd_update_display();
	}

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
#ifndef MESS
				/* free the graphics ROMs, they are no longer needed */
				/* TODO: instead of hardcoding region 1, use a flag to mark regions */
				/*       which can be freed after initialization. */
				free(Machine->memory_region[1]);
				Machine->memory_region[1] = 0;
#endif
				if (settingsloaded == 0)
				{
					/* if there is no saved config, it must be first time we run this game, */
					/* so show the disclaimer and driver credits. */
					showcopyright();
					showcredits();
				}

				showgameinfo();  /* show info about the game */

				if (nocheat == 0) InitCheat();

				cpu_run();      /* run the emulation! */

				if (nocheat == 0) StopCheat();

				/* save input ports settings */
				save_input_port_settings();

				/* the following MUST be done after hiscore_save() otherwise */
				/* some 68000 games will not work */
				sound_stop();
				if (drv->vh_stop) (*drv->vh_stop)();

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
