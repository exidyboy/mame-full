/* Modified for MESS!!! */
/* (Based on the 5/16/98 version of fronthlp.c) */

#include "driver.h"
#include "strings.h"

/* Mame frontend interface & commandline */
/* parsing rountines by Maurizio Zanello */

/* compare string[8] using standard(?) DOS wildchars ('?' & '*')      */
/* for this to work correctly, the shells internal wildcard expansion */
/* mechanism has to be disabled. Look into msdos.c */

void get_rom_path (int argc, char **argv, int game_index);

static const struct GameDriver *gamedrv;

int strwildcmp(const char *sp1, const char *sp2)
{
	char s1[9], s2[9];
	int i, l1, l2;
	char *p;

	strncpy(s1, sp1, 8); s1[8] = 0; if (s1[0] == 0) strcpy(s1, "*");

	strncpy(s2, sp2, 8); s2[8] = 0; if (s2[0] == 0) strcpy(s2, "*");

	p = strchr(s1, '*');
	if (p)
	{
		for (i = p - s1; i < 8; i++) s1[i] = '?';
		s1[8] = 0;
	}

	p = strchr(s2, '*');
	if (p)
	{
		for (i = p - s2; i < 8; i++) s2[i] = '?';
		s2[8] = 0;
	}

	l1 = strlen(s1);
	if (l1 < 8)
	{
		for (i = l1 + 1; i < 8; i++) s1[i] = ' ';
		s1[8] = 0;
	}

	l2 = strlen(s2);
	if (l2 < 8)
	{
		for (i = l2 + 1; i < 8; i++) s2[i] = ' ';
		s2[8] = 0;
	}

	for (i = 0; i < 8; i++)
	{
		if (s1[i] == '?' && s2[i] != '?') s1[i] = s2[i];
		if (s2[i] == '?' && s1[i] != '?') s2[i] = s1[i];
	}

	return stricmp(s1, s2);
}


int frontend_help (int argc, char **argv)
{
	int i, j;
	int list = 0;
	int verify = 0;
	int help = 1;    /* by default is TRUE */
	char gamename[9];

	/* covert '/' in '-' */
	for (i = 1;i < argc;i++) if (argv[i][0] == '/') argv[i][0] = '-';

	/* by default display the help unless */
	/* a game or an utility are specified */

	strcpy(gamename, "");

	for (i = 1;i < argc;i++)
	{
		/* find the FIRST "gamename" field (without '-') */
		if ((strlen(gamename) == 0) && (argv[i][0] != '-'))
		{
			strncpy(gamename, argv[i], 8);
			gamename[8] = 0;
		}
	}

	for (i = 1; i < argc; i++)
	{
		/* check for front-end utilities */
		if (!stricmp(argv[i],"-list")) list = 1;
		if (!stricmp(argv[i],"-listfull")) list = 2;
        if (!stricmp(argv[i],"-listdetails")) list = 7; /* A detailed MAMELIST.TXT type roms lister */

#ifdef MAME_DEBUG /* do not put this into a public release! */
		if (!stricmp(argv[i],"-lmr")) list = 6;
#endif

	}

	if ((strlen(gamename)> 0) || list || verify) help = 0;

	for (i = 1;i < argc;i++)
	{
		/* ...however, I WANT the help! */
		if (!stricmp(argv[i],"-?") || !stricmp(argv[i],"-h") || !stricmp(argv[i],"-help"))
			help = 1;
	}

	if (help)  /* brief help - useful to get current version info */
	{
		printf("M.E.S.S. v%s - Multiple Emulation Super System\n"
				"Portions copyright (C) 1997-98 by the MESS Team\n"
				"(Full copyright listing can be found in readme.txt)\n\n",messversion);
		showdisclaimer();
		printf("Usage:  MESS machine [image] [options]\n\n"
				"        MESS -list      for a brief list of supported systems\n"
				"        MESS -listfull  for a full list of supported systems\n\n"
				"See readme.txt for a complete list of options.\n");

		return 0;
	}

	switch (list)  /* front-end utilities ;) */
	{
		case 1: /* simple games list */
			printf("\nMESS currently supports the following systems:\n\n");
			i = 0; j = 0;
			while (drivers[i])
			{
				if (!strwildcmp(gamename, drivers[i]->name))
				{
					printf("%10s",drivers[i]->name);
					j++;
					if (!(j % 7)) printf("\n");
				}
				i++;
			}
			if (j % 7) printf("\n");
			printf("\n");
			if (j != i) printf("Total systems displayed: %4d - ", j);
			printf("Total systems supported: %4d\n", i);
			return 0;
			break;
		case 2: /* games list with descriptions */
			printf("Name:     Description:\n");
			i = 0;
			while (drivers[i])
			{
				if (!strwildcmp(gamename, drivers[i]->name))
					printf("%-10s\"%s\"\n",drivers[i]->name,drivers[i]->description);
				i++;
			}
			return 0;
			break;
		case 6:
			return 0;
			break;

        case 7: /* A detailed MAMELIST.TXT type roms lister */

            /* First, we shall print the header */

            printf(" system    cpu 1   cpu 2   cpu 3   sound 1   sound 2   sound 3   name\n");
            printf("--------   -----   -----   -----   -------   -------   -------   --------------------------\n");

            /* Let's cycle through the drivers */

            i = 0;

            while (drivers[i])
			{
                /* Dummy structs to fetch the information from */

                const struct MachineDriver *x_driver = drivers[i]->drv;
                const struct MachineCPU *x_cpu = x_driver->cpu;
                const struct MachineSound *x_sound = x_driver->sound;

                /* First, the rom name */

                printf("%-10s ",drivers[i]->name);

                /* Then, cpus */

                for(j=0;j<MAX_CPU-1;j++) /* Increase to table to 4, when a game with 4 cpus will appear */
                {
                    switch(x_cpu[j].cpu_type & (~CPU_FLAGS_MASK | CPU_AUDIO_CPU))
                    {
                        case 0:         printf("        "); break;
                        case CPU_Z80:   printf("Z80     "); break;
                        case CPU_M6502: printf("M6502   "); break;
                        case CPU_I86:   printf("I86     "); break;
                        case CPU_I8039: printf("I8039   "); break;
                        case CPU_M6803: printf("M6808   "); break;
                        case CPU_M6805: printf("M6805   "); break;
                        case CPU_M6809: printf("M6809   "); break;
                        case CPU_M68000:printf("M68000  "); break;

                        case CPU_Z80   |CPU_AUDIO_CPU: printf("[Z80]   "); break; /* Brackets mean that the cpu is only needed for sound. In cpu flags, 0x8000 means it */
                        case CPU_M6502 |CPU_AUDIO_CPU: printf("[M6502] "); break;
                        case CPU_I86   |CPU_AUDIO_CPU: printf("[I86]   "); break;
                        case CPU_I8039 |CPU_AUDIO_CPU: printf("[I8039] "); break;
                        case CPU_M6803 |CPU_AUDIO_CPU: printf("[M6808] "); break;
                        case CPU_M6805 |CPU_AUDIO_CPU: printf("[M6805] "); break;
                        case CPU_M6809 |CPU_AUDIO_CPU: printf("[M6809] "); break;
                        case CPU_M68000|CPU_AUDIO_CPU: printf("[M68000]"); break;
                    }
                }

                for(j=0;j<MAX_CPU-1;j++) /* Increase to table to 4, when a game with 4 cpus will appear */
                {

                    /* Dummy int to hold the number of specific sound chip.
                       In every multiple-chip interface, number of chips
                       is defined as the first variable, and it is integer. */

                    int *x_num = x_sound[j].sound_interface;

                    switch(x_sound[j].sound_type)
                    {
                        case 0: printf("          "); break; /* These don't have a number of chips, only one possible */
                        case SOUND_CUSTOM:  printf("Custom    "); break;
                        case SOUND_SAMPLES: printf("Samples   "); break;
                        case SOUND_NAMCO:   printf("Namco     "); break;
                        case SOUND_TMS5220: printf("TMS5520   "); break;
                        case SOUND_VLM5030: printf("VLM5030   "); break;

                        default:

                                /* Let's print out the number of the chips */

                                printf("%dx",*x_num);

                                /* Then the chip's name */

                                switch(x_sound[j].sound_type)
                                {
                                    case SOUND_DAC:     printf("DAC     "); break;
                                    case SOUND_AY8910:  printf("AY-8910 "); break;
                                    case SOUND_YM2203:  printf("YM-2203 "); break;
                                    case SOUND_YM2151:  printf("YM-2151 "); break;
                                    case SOUND_YM2151_ALT: printf("YM-2151a"); break;
                                    case SOUND_YM3812:  printf("YM-3812 "); break;
                                    case SOUND_SN76496: printf("SN76496 "); break;
                                    case SOUND_POKEY:   printf("Pokey   "); break;
                                    case SOUND_NES:     printf("NES     "); break;
                                    case SOUND_ADPCM:   printf("ADPCM   "); break;
                                    case SOUND_OKIM6295:printf("OKI6295 "); break;
                                    case SOUND_MSM5205: printf("MSM5205 "); break;
                                }
                                break;
                    }
                }

                /* Lastly, the name of the game and a \newline */

                printf("%s\n",drivers[i]->description);
                i++;
            }
            return 0;
            break;
	}

	/* use a special return value if no frontend function used */

	return 1234;
}
