/*
This file is a set of function calls and defs required for MESS.
It doesnt do much at the moment, but its here in case anyone
needs it ;-)
*/

#include "driver.h"
#include "mess/msdos.h"
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>
#include <dos.h>
#include <unistd.h>

extern struct GameOptions options;

/* fronthlp functions */
extern int strwildcmp(const char *sp1, const char *sp2);

/**********************************************************/
/* Functions called from MSDOS.C by MAME for running MESS */
/**********************************************************/
static char startup_dir[260]; /* Max Windows Path? */


/* Go back to the startup dir on exit */
void return_to_startup_dir(void)
{
    chdir(startup_dir);
}

/*
 * Detect the type of image given in 'arg':
 * 1st: user specified type (after -rom, -floppy ect.)
 * 2nd: match extensions specified by the driver
 * default: add image to the list of names for IO_CARTSLOT
 */
static int detect_image_type(int game_index, int type, char *arg)
{
	const struct GameDriver *drv = drivers[game_index];
	char *ext;

	if (options.image_count >= MAX_IMAGES)
	{
		printf("Too many image names specified!\n");
		return 1;
	}

	if (type)
	{
		if (errorlog)
			fprintf(errorlog, "User specified %s for %s\n", device_typename(type), arg);
		/* the user specified a device type */
		options.image_files[options.image_count].type = type;
		options.image_files[options.image_count].name = strdup(arg);
		options.image_count++;
		return 0;
	}

	/* Look up the filename extension in the drivers device list */
	ext = strrchr(arg, '.');
	if (ext)
	{
		const struct IODevice *dev = drv->dev;

		ext++;
		while (dev->type != IO_END)
		{
			const char *dst = dev->file_extensions;

			/* scan supported extensions for this device */
			while (dst && *dst)
			{
				if (stricmp(dst, ext) == 0)
				{
					if (errorlog)
						fprintf(errorlog, "Extension match %s [%s] for %s\n", device_typename(dev->type), dst, arg);
					options.image_files[options.image_count].type = dev->type;
					options.image_files[options.image_count].name = strdup(arg);
					options.image_count++;
					return 0;
				}
				/* skip '\0' once in the list of extensions */
				dst += strlen(dst) + 1;
			}
			dev++;
		}
	}

	type = IO_CARTSLOT;
	if (errorlog)
		fprintf(errorlog, "Default %s for %s\n", device_typename(type), arg);
	/* Every unrecognized image type is added here */
	options.image_files[options.image_count].type = type;
	options.image_files[options.image_count].name = strdup(arg);
	options.image_count++;
	return 0;
}



/* Small check to see if system supports device */
int system_supports_device(int game_index, int type)
{
    const struct IODevice *dev = drivers[game_index]->dev;

	while(dev->type!=IO_END)
	{
		if(dev->type==type)
			return 1;
		dev++;
	}
	return 0;
}


/*
 * Load images from the command line.
 * Detect aliases and substitute the list of images for them.
 */
int load_image(int argc, char **argv, int j, int game_index)
{
	const char *driver = drivers[game_index]->name;
	int i;
	int res = 0;
	int type = IO_END;

	/*
	 * Take all following commandline arguments without "-" as
	 * image names or as an alias name, which is replaced by a list
	 * of images.
	 */
	for (i = j + 1; i < argc; i++)
	{
		/* skip options and their additional arguments */
		/* this should really look up the structure values for easy maintenance */
		if (argv[i][0] == '-')
		{
			if      (!stricmp(argv[i], "-cartridge")  || !stricmp(argv[i], "-cart"))
				type = IO_CARTSLOT;
			else if (!stricmp(argv[i], "-floppydisk") || !stricmp(argv[i], "-flop"))
				type = IO_FLOPPY;
			else if (!stricmp(argv[i], "-harddisk")   || !stricmp(argv[i], "-hard"))
				type = IO_HARDDISK;
			else if (!stricmp(argv[i], "-cassette")   || !stricmp(argv[i], "-cass"))
				type = IO_CASSETTE;
			else if (!stricmp(argv[i], "-printer")    || !stricmp(argv[i], "-prin"))
				type = IO_PRINTER;
			else if (!stricmp(argv[i], "-serial")     || !stricmp(argv[i], "-serl"))
				type = IO_SERIAL;
			else if (!stricmp(argv[i], "-snapshot")   || !stricmp(argv[i], "-snap"))
				type = IO_SNAPSHOT;
			else if (!stricmp(argv[i], "-quickload")  || !stricmp(argv[i], "-quik"))
				type = IO_QUICKLOAD;
			/* all other switches set type to -1 */
			else type = -1;

			if (type>IO_END && !system_supports_device(game_index, type))
			{
				if (errorlog)
					fprintf(errorlog,"Specified Device (%s) not supported by this system\n", argv[i]);
				type = -1; /* strip device if systems doesnt support it */
			}


		}
		else if (type != -1) /* only enter when valid option, otherwise get next */
		{
			/* check if this is an alias for a set of images */
			char *alias = get_alias(driver, argv[i]);

			if (alias && strlen(alias))
			{
				char *arg;

				if (errorlog)
					fprintf(errorlog, "Using alias %s (%s) for driver %s\n", argv[i], alias, driver);
				arg = strtok(alias, ",");
				while (arg)
				{
					res = detect_image_type(game_index, type, arg);
					arg = strtok(0, ",");
					type = IO_END; /* image detected, reset type */
				}
			}
			else if (type != IO_END)
			{
				if (errorlog)
					fprintf(errorlog, "NOTE: No alias found\n");
				res = detect_image_type(game_index, type, argv[i]);
				type = IO_END; /* image detected, reset type */
			}
		}
		/* If we had an error bail out now */
		if (res)
			return res;

	}
	return res;
}




/* This function contains all the -list calls from fronthlp.c for MESS */
/* Currently Supported: */
/*   -listdevices    */

void list_mess_info(char *gamename, char *arg, int listclones)
{

	int i, j;

	/* -listdevices */
	if (!stricmp(arg, "-listdevices"))
	{

		i = 0;
		j = 0;


		printf(" SYSTEM      DEVICE NAME (brief)   IMAGE FILE EXTENSIONS SUPPORTED    \n");
		printf("----------  --------------------  ------------------------------------\n");

		while (drivers[i])
		{
			const struct IODevice *dev = drivers[i]->dev;

			if (!strwildcmp(gamename, drivers[i]->name))
			{
				int devcount = 1;

				printf("%-13s", drivers[i]->name);

				/* if IODevice not used, print UNKNOWN */
				if (dev->type == IO_END)
					printf("%-12s\n", "UNKNOWN");

				/* else cycle through Devices */
				while (dev->type != IO_END)
				{
					const char *src = dev->file_extensions;

					if (devcount == 1)
						printf("%-12s(%s)   ", device_typename(dev->type), briefdevice_typename(dev->type));
					else
						printf("%-13s%-12s(%s)   ", "    ", device_typename(dev->type), briefdevice_typename(dev->type));

					devcount++;

					while (src && *src)
					{

						printf(".%-5s", src);
						src += strlen(src) + 1;
					}
					dev++;			   /* next IODevice struct */
					printf("\n");
				}


			}
			i++;

		}

	}

	/* -listtext */
	else if (!stricmp(arg, "-listtext"))
	{
		printf("                   ==========================================\n" );
		printf("                    M.E.S.S.  -  Multi-Emulator Super System\n"  );
		printf("                             Copyright (C) 1998-2000\n");
		printf("                                by the MESS team\n"    );
		printf("                    Official Page at: http://mess.emuverse.com\n");
		printf("                   ==========================================\n\n" );

		printf("This document is generated for MESS %s\n\n",build_version);

		printf("Please note that many people helped with this project, either directly or by\n"
		       "releasing source code which was used to write the drivers. We are not trying to\n"
		       "appropriate merit which isn't ours. See the acknowledgemnts section for a list\n"
			   "of contributors, however please note that the list is largely incomplete. See\n"
			   "also the CREDITS section in the emulator to see the people who contributed to a\n"
			   "specific driver. Again, that list might be incomplete. We apologize in advance\n"
			   "for any omission.\n\n"

			   "All trademarks cited in this document are property of their respective owners.\n"

			   "Especially, the MESS team would like to thank Nicola Salmoria and the MAME team\n"
			   "for letting us play with their code and, in fact, incorporating MESS specific\n"
			   "code into MAME.  Without it, MESS would be substantially less than what it is\n"
			   "right now! ;-)\n\n"

			   "Usage and Distribution Licence:\n"
			   "===============================\n"
			   "- MESS usage and distribution follows that of MAME.  Please read the MAME\n"
			   "  readme.txt file distributed with MESS for further information.\n\n"

			   "How to Contact The MESS Team\n"
			   "============================\n"
			   "Visit the web page at http://mess.emuverse.com to see a list of contributers\n"
			   "If you have comments, suggestions or bug reports about an existing driver, check\n"
			   "the page contacts section to find who has worked on it, and send comments to that \n"
			   "person. If you are not sure who to contact, write to Ben (ben@mame.net) - who is the \n"
			   "current coordinator of the MESS project [DOS]. \n\n"

			   "PLEASE DON'T SEND BINARY ATTACHMENTS WITHOUT ASKING FIRST, *ESPECIALLY* ROM IMAGES.\n"

			   "THESE ARE NOT SUPPORT ADDRESSES. Support questions sent to these addresses\n"
			   "*will* be ignored. Please understand that this is a *free* project, mostly\n"
			   "targeted at experienced users. We don't have the resources to provide end user\n"
			   "support. Basically, if you can't get the emulator to work, you are on your own.\n"
			   "First of all, read this doc carefully. If you still can't find an answer to\n"
			   "your question, try checking the beginner's sections that many emulation pages\n"
			   "have, or ask on the appropriate Usenet newsgroups (e.g. comp.emulators.misc)\n"
			   "or on the many emulation message boards.  The official MESS message board is at:\n"
			   "   http://mess.emuverse.com\n\n");


		printf("Also, please DO NOT SEND REQUESTS FOR NEW SYSTEMS TO ADD, unless you have some original\n");
		printf("info on the hardware or, even better, have the technical expertise needed to\n");
		printf("help us. Please don't send us information widely available on the Internet -\n");
		printf("we are perfectly capable of finding it ourselves, thank you.\n\n\n");


		printf("Complete Emulated System List\n");
		printf("=============================\n");
		printf("Here is the list of systems supported by MESS %s\n",build_version);
		if (!listclones)
			printf("Variants of the same system are not included, you can use the -listclones command\n"
				"to get a list of the alternate versions of a given system.\n");
		printf("\n"
			   "The meanings of the columns are as follows:\n"
			   "Working - \"No\" means that the emulation has shortcomings that cause the system\n"
			   "  not to work correctly. This can be anywhere from just showing a black screen\n"
			   "  to not being playable with major problems.\n"
			   "Correct Colors - \"Yes\" means that colors should be identical to the original,\n"
			   "  \"Close\" that they are very similar but wrong in places, \"No\" that they are\n"
			   "  completely wrong. \n"
			   "Sound - \"Partial\" means that sound support is either incomplete or not entirely\n"
			   "  accurate. \n"
			   "Internal Name - This is the unique name that should be specified on the command\n"
			   "  line to run the system. ROMs must be placed in the ROM path, either in a .zip\n"
			   "  file or in a subdirectory of the same name. The former is suggested, because\n"
			   "  the files will be identified by their CRC instead of requiring specific\n"
			   "  names.  NOTE! that as well as required ROM files to emulate the system, you may\n"
			   "  also attach IMAGES of files created for system specific devices (some examples of \n"
			   "  devices are cartridges, floppydisks, harddisks, etc).  See below for a complete list\n"
			   "  of a systems supported devices and common file formats used for that device\n\n");

		printf("System Information can be obtained from the SysInfo.dat file (online in the MESS UI\n"
			   "from the Machine history) or sysinfo.htm.  To generate sysinfo.htm, execute \n"
			   "dat2html.exe.\n\n\n");

		printf("+-----------------------------------------+-------+-------+-------+----------+\n");
		printf("|                                         |       |Correct|       | Internal |\n");
		printf("| System Name                             |Working|Colors | Sound |   Name   |\n");
		printf("+-----------------------------------------+-------+-------+-------+----------+\n");



			/* Generate the System List */

			 i = 0;
			while (drivers[i])
			{

				if ((listclones || drivers[i]->clone_of == 0
						|| (drivers[i]->clone_of->flags & NOT_A_DRIVER)
						) && !strwildcmp(gamename, drivers[i]->name))
				{
					char name[200],name_ref[200];

					strcpy(name,drivers[i]->description);

					/* Move leading "The" to the end */
					if (strstr(name," (")) *strstr(name," (") = 0;
					if (strncmp(name,"The ",4) == 0)
					{
						sprintf(name_ref,"%s, The ",name+4);
					}
					else
						sprintf(name_ref,"%s ",name);

					/* print the additional description only if we are listing clones */
					if (listclones)
					{
						if (strchr(drivers[i]->description,'('))
							strcat(name_ref,strchr(drivers[i]->description,'('));
					}

					//printf("| %-33.33s",name_ref);
					printf("| %-40.40s",name_ref);

					if (drivers[i]->flags & GAME_NOT_WORKING)
					{
						const struct GameDriver *maindrv;
						int foundworking;

						if (drivers[i]->clone_of && !(drivers[i]->clone_of->flags & NOT_A_DRIVER))
							maindrv = drivers[i]->clone_of;
						else maindrv = drivers[i];

						foundworking = 0;
						j = 0;
						while (drivers[j])
						{
							if (drivers[j] == maindrv || drivers[j]->clone_of == maindrv)
							{
								if ((drivers[j]->flags & GAME_NOT_WORKING) == 0)
								{
									foundworking = 1;
									break;
								}
							}
							j++;
						}

						if (foundworking)
							printf("| No(1) ");
						else
							printf("|   No  ");
					}
					else
						printf("|  Yes  ");

					if (drivers[i]->flags & GAME_WRONG_COLORS)
						printf("|   No  ");
					else if (drivers[i]->flags & GAME_IMPERFECT_COLORS)
						printf("| Close ");
					else
						printf("|  Yes  ");

					{
						const char **samplenames = 0;
						for (j = 0;drivers[i]->drv->sound[j].sound_type && j < MAX_SOUND; j++)
						{
							if (drivers[i]->drv->sound[j].sound_type == SOUND_SAMPLES)
							{
								samplenames = ((struct Samplesinterface *)drivers[i]->drv->sound[j].sound_interface)->samplenames;
								break;
							}
						}
						if (drivers[i]->flags & GAME_NO_SOUND)
							printf("|   No  ");
						else if (drivers[i]->flags & GAME_IMPERFECT_SOUND)
						{
							if (samplenames)
								printf("|Part(2)");
							else
								printf("|Partial");
						}
						else
						{
							if (samplenames)
								printf("| Yes(2)");
							else
								printf("|  Yes  ");
						}
					}

					printf("| %-8s |\n",drivers[i]->name);
				}
				i++;
			}

			printf("+-----------------------------------------+-------+-------+-------+----------+\n");
			printf("(1) There are variants of the system that work correctly\n");
			printf("(2) Needs samples provided separately\n\n\n\n\n");


		printf("QUICK MESS USAGE GUIDE!\n"
		       "=======================\n"
		       "In order to use MESS, you must at least specify at the command line\n\n"
               "      MESS <system>\n\n"
			   "This will emulate the system requested.  Note that most systems require ROMS for\n"
			   "emulation.  These system ROM files are copyright and ARE NOT supplied with MESS.\n\n"
			   "To use files created for the system emulated (IMAGES), MESS works by attaching an image\n"
			   "of the file created for the particular device of that system, for example, a cartridge,\n"
               "floppydisk, harddisk, cassette, image etc.  Therefore, in order to attach an image to the\n"
			   "system, you must specify at the command line:\n\n"
               "      MESS <system> <device> <image_name>\n\n"
			   "To manually manipulate the emulation options, you must specify:\n\n"
               "      MESS <system> <device> <image_name> <options>\n\n");
		printf("*For a complete list of systems emulated,  use: MESS -listfull\n"
			   "*For system files (ROMS) required by each system, use: MESS <system> -listroms\n"
			   "*See below for valid device names and usage."
			   "*See the MAME readme.txt and below for a detailed list of options.\n\n"
			   "Make sure you have ROMS and IMAGES in a subdirectory from your ROMPATH\n"
			   "with the same name as the system (eg ROMS/COLECO)\n\n\n");
		printf("Examples:\n\n"
			   "    MESS nes -cart zelda.nes\n"
			   "        will attach zelda.nes to the cartridge device and run MESS in\n"
			   "        the following way:\n"
			   "        <system>      = nes             (Nintendo Entertainment System)\n"
			   "        <device>      = CARTRIDGE\n"
			   "        <image_name>  = zelda.nes       (Zelda cartridge)\n"
			   "        <options>     = none specified, so default options (see mess.cfg)\n\n"
			   "    MESS coleco -cart dkong -soundcard 0\n"
			   "        will run MESS in the following way:\n\n"
			   "        <system>      = coleco          (Nintendo Entertainment System)\n"
			   "        <device>      = CARTRIDGE\n"
			   "        <image_name>  = dkong.rom       (Donkey Kong cartridge)\n"
			   "        <options>     = default options without sound (see mess.cfg)\n\n"
			   "    MESS trs80 -flop boot.dsk -flop arcade1.dsk\n"
			   "        will run MESS in the following way:\n"
			   "        <system>      = trs80           (TRs-80 model 1)\n"
			   "        <device1>     = FLOPPYDISK\n"
			   "		<image_name1> = boot.dsk        (The Trs80 boot floppy diskl)\n"
			   "        <device1>     = FLOPPYDISK\n"
			   "        <image_name2> = arcade1.dsk     (floppy Disk which contains games)\n"
			   "        <options>     = default options (all listed in mess.cfg)\n\n"
			   "    MESS cgenie -fd games1\n"
			   "        will run the system Colour Genie with one disk image loaded,\n"
			   "        automatically appending the file extension .dsk.\n\n\n\n\n");




		printf("DEVICE support list\n");
		printf("===================\n");
		printf("As mentioned, in order to fully utilise MESS, you will need to attach image files\n"
			   "to the system devices.  The following list specifies all the devices and image \n"
			   "file extensions currently supported by MESS.  Remember to use the DEVICE name \n"
			   "(or the brief name) to attach an image.  This list can easily be generated by \n"
			   "specifying:\n\n"
			   "    MESS -listdevices\n\n");
		printf("Also note that MESS has a preliminary built-in File Manager for attaching images to\n"
			   "system devices.  Use the UI (TAB key) to access.\n\n\n\n");




	}




}

/*****************************************************************************
 * device, directory and file functions
 *****************************************************************************/

static int num_devices = 0;
static char dos_devices[32*2];
static char dos_device[2];
static char dos_filemask[260];

static int fnmatch(const char *f1, const char *f2)
{
	while (*f1 && *f2)
	{
		if (*f1 == '*')
		{
			/* asterisk is not the last character? */
			if (f1[1])
			{
				/* skip until first occurance of the character after the asterisk */
                while (*f2 && toupper(f1[1]) != toupper(*f2))
					f2++;
				/* skip repetitions of the character after the asterisk */
				while (*f2 && toupper(f1[1]) == toupper(f2[1]))
					f2++;
			}
			else
			{
				/* skip until end of string */
                while (*f2)
					f2++;
			}
        }
		else
		if (*f1 == '?')
		{
			/* skip one character */
            f2++;
		}
		else
		{
			/* mismatch? */
            if (toupper(*f1) != toupper(*f2))
				return 0;
            /* skip one character */
			f2++;
		}
		/* skip mask */
        f1++;
	}
	/* no match if anything is left */
	if (*f1 || *f2)
		return 0;
    return 1;
}

int osd_num_devices(void)
{
	if (num_devices == 0)
	{
		union REGS r;
        int dev, previous_dev;
		r.h.ah = 0x19;	/* get current drive */
		intdos(&r,&r);
		previous_dev = r.h.al;	/* save current drive */
		for (dev = 0; dev < 26; dev++)
		{
			r.h.ah = 0x0e;		/* select drive */
			r.h.dl = dev;		/* DL */
			intdos(&r,&r);
			r.h.ah = 0x19;		/* get current drive */
			intdos(&r,&r);
			if (r.h.al == dev)	/* successful? */
			{
				dos_devices[num_devices*2+0] = 'A' + dev;
				dos_devices[num_devices*2+1] = '\0';
				num_devices++;
			}
        }
		r.h.ah = 0x0e;		/* select previous drive again */
		r.h.dl = previous_dev;
		intdos(&r,&r);
    }
	return num_devices;
}

const char *osd_get_device_name(int idx)
{
	if (idx < num_devices)
        return &dos_devices[idx*2];
    return "";
}

void osd_change_device(const char *device)
{
        char chdir_device[4];

	dos_device[0] = device[0];
	dos_device[1] = '\0';

        chdir_device[0] = device[0];
        chdir_device[1] = ':';
        chdir_device[2] = '/';
        chdir_device[3] = '\0';


        chdir(chdir_device);

}

void osd_change_directory(const char *directory)
{
		if (!startup_dir[0])
		{
			getcwd(startup_dir,sizeof(startup_dir));
			atexit(return_to_startup_dir);
		}

        chdir(directory);

}

static char dos_cwd[260];

const char *osd_get_cwd(void)
{
        getcwd(dos_cwd, 260);

        return dos_cwd;
}

void *osd_dir_open(const char *mess_dirname, const char *filemask)
{
	DIR *dir;

	strcpy(dos_filemask, filemask);

    dir = opendir(".");

    return dir;
}

int osd_dir_get_entry(void *dir, char *name, int namelength, int *is_dir)
{
	int len;
    struct dirent *d;

    name[0] = '\0';
	*is_dir = 0;

    if (!dir)
		return 0;

    d = readdir(dir);
	while (d)
	{
		struct stat st;

		strncpy(name, d->d_name, namelength-1);
		name[namelength-1]='\0';

		len = strlen(name);

		if( stat(d->d_name, &st) == 0 )
			*is_dir = S_ISDIR(st.st_mode);

		if (*is_dir)
			return len;
		else
		if (fnmatch(dos_filemask, d->d_name))
			return len;
		else
		{
			/* no match, zap the name and type again */
			name[0] = '\0';
			*is_dir = 0;
        }
		d = readdir(dir);
	}
	return 0;
}

void osd_dir_close(void *dir)
{
	if (dir)
		closedir(dir);
}


