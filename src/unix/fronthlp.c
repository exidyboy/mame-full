#include <stdarg.h> /* prolly should go in xmame.h */
#include "xmame.h"
#include "driver.h"
#include "audit.h"
#include "common.h"
#include "info.h"

static int frontend_list_clones(char *gamename);
static int frontend_list_cpu(void);
static int frontend_list_gamelistheader(void);
static int frontend_list_crcs(void);

static int list       = 0;
static int showclones = 1;
static int verbose    = 1;
static int correct    = 0;
static int incorrect  = 0;
static int not_found  = 0;

enum {
   /* standard list commands */
   LIST_LIST = 1, LIST_FULL, LIST_GAMES, LIST_DETAILS, LIST_GAMELIST,
   LIST_SOURCEFILE, LIST_COLORS, LIST_DEVICES, LIST_ROMSIZE, LIST_ROMS,
   LIST_CRC, LIST_SAMPLES, LIST_SAMDIR, VERIFY_ROMS, VERIFY_ROMSETS,
   VERIFY_SAMPLES, VERIFY_SAMPLESETS,
   /* internal verification list commands (developers only) */
   LIST_MISSINGROMS, LIST_DUPCRC, LIST_WRONGORIENTATION, LIST_WRONGMERGE,
   LIST_WRONGFPS,
   /* standard listcommands which require special handling */
   LIST_CLONES, LIST_INFO, LIST_CPU, LIST_GAMELISTHEADER
};
   
/* Mame frontend interface & commandline */
/* parsing rountines by Maurizio Zanello */

struct rc_option frontend_list_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Frontend Related", NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "list",		"l",			rc_set_int,	&list,
     NULL,		LIST_LIST,		0,		NULL,
     "List supported games matching gamename, or all, gamename may contain * and ? wildcards" },
   { "listfull",	"lf",			rc_set_int,	&list,
     NULL,		LIST_FULL,		0,		NULL,
     "Like -list, with full description" },
   { "listgames",	"lg",			rc_set_int,	&list,
     NULL,		LIST_GAMES,		0,		NULL,
     "Like -list, with manufacturer and year" },
   { "listdetails",	"ld",			rc_set_int,	&list,
     NULL,		LIST_DETAILS,		0,		NULL,
     "Like -list, with detailed info" },
   { "listgamelist",	"lgl",			rc_set_int,	&list,
     NULL,		LIST_GAMELIST,		0,		NULL,
     "Like -list, with specialy formatted extra info for generating gamelist.mame, also see -listgamelistheader" },
   { "listsourcefile",	"lsf",			rc_set_int,	&list,
     NULL,		LIST_SOURCEFILE,	0,		NULL,
     "Like -list, with driver sourcefile" },
   { "listcolors",	"lcol",			rc_set_int,	&list,
     NULL,		LIST_COLORS,		0,		NULL,
     "Like -list, with the number of colors used" },
#ifdef MESS
   { "listdevices",	"ldev",			rc_set_int,	&list,
     NULL,		LIST_DEVICES,		0,		NULL,
     "Like -list, with devices and image file extensions supported" },
#endif
   { "listromsize",	"lrs",			rc_set_int,	&list,
     NULL,		LIST_ROMSIZE,		0,		NULL,
     "Like -list, with the year and size of the roms used" },
   { "listroms",	"lr",			rc_set_int,	&list,
     NULL,		LIST_ROMS,		0,		NULL,
     "Like -list, but lists used ROMS" },
   { "listcrc",		"lcrc",			rc_set_int,	&list,
     NULL,		LIST_CRC,		0,		NULL,
     "Like -list, but lists used ROMS with crc" },
#if (HAS_SAMPLES)
   { "listsamples",	"ls",			rc_set_int,	&list,
     NULL,		LIST_SAMPLES,		0,		NULL,
     "Like -list, but lists used audio samples" },
   { "listsamdir",	"lsd",			rc_set_int,	&list,
     NULL,		LIST_SAMDIR,		0,		NULL,
     "Like -list, but lists dir where samples are taken from" },
#endif   
   { "verifyroms",	"vr",			rc_set_int,	&list,
     NULL,		VERIFY_ROMS,		0,		NULL,
     "Verify ROMS for games matching gamename, or all, gamename may contain * and ? wildcards" },
   { "verifyromsets",	"vrs",			rc_set_int,	&list,
     NULL,		VERIFY_ROMSETS,		0,		NULL,
     "Like -verifyroms, but less verbose" },
#if (HAS_SAMPLES)
   { "verifysamples",	"vs",			rc_set_int,	&list,
     NULL,		VERIFY_SAMPLES,		0,		NULL,
     "Like -verifyroms but verify audio samples instead" },
   { "verifysamplesets", "vss",			rc_set_int,	&list,
     NULL,		VERIFY_SAMPLESETS,	0,		NULL,
     "Like -verifysamples, but less verbose" },
#endif
   { "clones",		"cl",			rc_bool,	&showclones,
     "1",		0,			0,		NULL,
     "Show / don't show bootlegs/clones in the above list commands" },
   { "listclones",	"lcl",			rc_set_int,	&list,
     NULL,		LIST_CLONES,		0,		NULL,
     "Like -list, but lists the clones of the specified game" },
   { "listinfo",        "li",			rc_set_int,	&list,
     NULL,		LIST_INFO,		0,		NULL,
     "List all available info on drivers" },
   { "listcpu",		"lc",			rc_set_int,	&list,
     NULL,		LIST_CPU,		0,		NULL,
     "List cpu usage statics per year" },
   { "listgamelistheader", "lgh",		rc_set_int,	&list,
     NULL,		LIST_GAMELISTHEADER,	0,		NULL,
     "Print header for generating gamelist.mame, also see -listgamelist" },
   { "Internal verification list commands (only for developers)", NULL, rc_seperator, NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "listmissingroms",	"lmr",			rc_set_int,	&list,
     NULL,		LIST_MISSINGROMS,		0,		NULL,
     "Like -list, but lists ROMS missing" },
   { "listdupcrc",	"ldc",			rc_set_int,	&list,
     NULL,		LIST_DUPCRC,		0,		NULL,
     "Like -list, but lists ROMS with identical crc" },
   { "listwrongorientation", "lwo",			rc_set_int,	&list,
     NULL,		LIST_WRONGORIENTATION,	0,		NULL,
     "Like -list, but lists any games which use the orientation flags wrongly" },
   { "listwrongmerge",	"lwm",			rc_set_int,	&list,
     NULL,		LIST_WRONGMERGE,	0,		NULL,
     "Like -list, but lists any games which use the clone_of field wrongly" },
   { "listwrongfps",	"lwf",			rc_set_int,	&list,
     NULL,		LIST_WRONGFPS,		0,		NULL,
     "Like -list, but lists any games which use the FPS field wrongly" },
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

/* compare string[8] using standard(?) wildchars ('?' & '*')          */
/* for this to work correctly, the shells internal wildcard expansion */
/* mechanism has to be disabled, use quotes */
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

static int myprintf(char *fmt, ...) {
  int i = 0;
  va_list args;
  
  if(verbose)
  {
     va_start(args, fmt);
     i = vfprintf(stdout_file, fmt, args);
     va_end(args);
  }
  return i;
}

static void frontend_verify(int driver, int rom)
{
   int status;
   
   if(rom)
      status = VerifyRomSet(driver, (verify_printf_proc)myprintf);
   else
      status = VerifySampleSet(driver, (verify_printf_proc)myprintf);

   if (verbose)
      fprintf(stdout_file, "%s %s ", rom? "romset":"sampleset",
         drivers[driver]->name);
   else
      fprintf(stdout_file, "%-8s  ", drivers[driver]->name);

   switch (status)
   {
      case BEST_AVAILABLE:
         fprintf(stdout_file, "best available\n");
         correct++;
         break;
      case CORRECT:
         fprintf(stdout_file, "correct\n");
         correct++;
         break;
      case NOTFOUND:
      case CLONE_NOTFOUND:
         fprintf(stdout_file, "not found\n");
         not_found++;
         break;
      case INCORRECT:
         fprintf(stdout_file, "incorrect\n");
         incorrect++;
         break;
   }
}

static int frontend_uses_roms(int driver)
{
   const struct RomModule *region, *rom;
   int total_roms = 0;
   
   for (region = rom_first_region(drivers[driver]); region; region = rom_next_region(region))
   {
      for (rom = rom_first_file(region); rom && (ROM_GETNAME(rom) ||
      ROM_GETOFFSET(rom) || ROM_GETLENGTH(rom)); rom = rom_next_file(rom))
      {
         if (ROM_GETNAME(rom) && ROM_GETNAME(rom) != (char *)-1)
         {
            total_roms++;
         }
      }
   }
   
   return total_roms;
}

char *get_description(int driver)
{
   char *p;
   char copy[BUF_SIZE];
   static char description[BUF_SIZE];
   
   snprintf(copy, BUF_SIZE, drivers[driver]->description);
   
   /* Remove the additonal description if any */
   if ((p = strstr(copy, " (")))
      *p = 0;
   
   /* Move leading "The" to the end */
   if (strncmp(copy, "The ", 4) == 0)
      snprintf(description, BUF_SIZE, "%s, The", copy+4);
   else if (strncmp(copy, "Le ", 3) == 0)
      snprintf(description, BUF_SIZE, "%s, Le", copy+3);
   else
      snprintf(description, BUF_SIZE, copy);
   
   /* Print the additional description only if we are listing clones */
   if (showclones && p)
   {
      int len = strlen(description);
      
      *p = ' ';
      snprintf(description + len, BUF_SIZE - len, p);
   }
   
   return description;
}

int frontend_list(char *gamename)
{
   int i, j=0;
   const char *header[] = {
/*** standard list commands ***/
/* list             */ NAME" currently supports:\n",
/* listfull         */ "name      description\n"
                       "--------  -----------\n",
/* listgames        */ "year manufacturer                         name\n"
                       "---- ------------------------------------ --------------------------------\n",
/* listdetails      */ " romname driver     cpu 1    cpu 2    cpu 3    cpu 4    cpu 5    cpu 6    cpu 7    cpu 8    sound 1     sound 2     sound 3     sound 4     sound 5     name\n"
                       "-------- ---------- -------- -------- -------- -------- -------- -------- -------- -------- ----------- ----------- ----------- ----------- ----------- --------------------------\n",
/* listgamelist     */ "+----------------------------------+-------+-------+-------+-------+----------+\n"
                       "|                                  |       |Correct|       |Screen | Internal |\n"
                       "| Game Name                        |Working|Colors | Sound | Flip  |   Name   |\n"
                       "+----------------------------------+-------+-------+-------+-------+----------+\n",
/* listsourcefile   */ "name     sourcefile\n"
                       "-------- ----------\n",
/* listcolors       */ "name      colors\n"
                       "--------  ------\n",
/* listextensions   */ "name      device      image file extensions supported\n"
                       "--------  ----------  -------------------------------\n",
/* listromsize      */ "name    \tyear \tsize\n"
                       "--------\t-----\t----\n",
/* listroms         */ "",
/* listcrc          */ "CRC      filename     description\n"
                       "-------- ------------ -----------\n",
/* listsamples      */ "",
/* listsamdir       */ "name      samples dir\n"
                       "--------  -----------\n",
/* verifyroms       */ "",
/* verifyromsets    */ "name      result\n"
                       "--------  ------\n",
/* verifysamples    */ "",
/* verifysamplesets */ "name      result\n"
                       "--------  ------\n",
/*** internal verification list commands (developers only) ***/
/* listmissingroms  */ "name      clone of  description\n"
                       "--------  --------  -----------\n",
/* listdupcrc       */ "CRC      filename1    name1        filename2    name2\n"
                       "-------- ------------ --------     ------------ --------\n",
/* wrongorientation */ "",
/* wrongmerge       */ "",
/* wrongfps         */ "name      resolution  fps\n"
                       "--------  ----------  -----------\n"
   };
       
   int matching     = 0;
   int skipped      = 0;
   
   if (!gamename)
      gamename = "";
      
   /* listcommands which require special handling */
   switch(list)
   {
      /* no list requested */
      case 0:
         return 1234;
      /* listclones is a special case since the strwildcmp */
      /* also has to be done on clone_of. */
      case LIST_CLONES:
         return frontend_list_clones(gamename);
      /* listinfo is handled by the core */
      case LIST_INFO:
         print_mame_info( stdout_file, drivers ); 
         return OSD_OK;
      case LIST_CPU:
         return frontend_list_cpu();
      case LIST_GAMELISTHEADER:
         return frontend_list_gamelistheader();
      case LIST_CRC: /* list all crc-32 */
         return frontend_list_crcs();
   }
   
   fprintf(stdout_file, header[list-1]);

   for (i=0;drivers[i];i++)
   {
         if ( (showclones || drivers[i]->clone_of == 0 ||
                (drivers[i]->clone_of->flags & NOT_A_DRIVER)) &&
              !strwildcmp(gamename, drivers[i]->name) )
         {
            matching++;
            
            switch(list)
            {
               /*** standard list commands ***/
               case LIST_LIST: /* simple games list */
                  fprintf(stdout_file, "%-8s", drivers[i]->name);
                  if (!(matching % 8))
                     fprintf(stdout_file, "\n");
                  else
                     fprintf(stdout_file, "  ");
                  break;
               case LIST_FULL: /* games list with descriptions */
                  fprintf(stdout_file, "%-10s\"%s\"\n", drivers[i]->name,
                     get_description(i));
                  break;
               case LIST_GAMES:
               {
                  fprintf(stdout_file, "%-5s%-36s %s\n",
                     drivers[i]->year,
                     drivers[i]->manufacturer, get_description(i));
                  break;
               }
               case LIST_DETAILS: /* A detailed MAMELIST.TXT type roms lister */
                  /* First, the rom name */
                  fprintf(stdout_file, "%-8s ",drivers[i]->name);

                  /* source file (skip the leading path) */ 
                  fprintf(stdout_file, "%-10s ", strrchr(drivers[i]->source_file, '/') + 1);

                  /* Then, cpus */
                  for(j=0;j<MAX_CPU;j++)
                  {
                     const struct MachineCPU *x_cpu = drivers[i]->drv->cpu;
                     if (x_cpu[j].cpu_type & CPU_AUDIO_CPU)
                        fprintf(stdout_file, "[%-6s] ",cputype_name(x_cpu[j].cpu_type));
                      else
                        fprintf(stdout_file, "%-8s ",cputype_name(x_cpu[j].cpu_type));
                  }
                  fprintf(stdout_file, " ");
               
                  for(j=0;j<MAX_SOUND;j++)
                  {
                     const struct MachineSound *x_sound = drivers[i]->drv->sound;
                     if (sound_num(&x_sound[j]))
                     {
                        fprintf(stdout_file, "%dx",sound_num(&x_sound[j]));
                        fprintf(stdout_file, "%-9s ",sound_name(&x_sound[j]));
                     }
                     else
                        fprintf(stdout_file, "%-11s ",sound_name(&x_sound[j]));
                  }
                  
                  /* Lastly, the name of the game and a \newline */
                  fprintf(stdout_file, " %s\n", get_description(i));
                  break;
               case LIST_GAMELIST:
                  {
                     fprintf(stdout_file, "| %-33.33s", get_description(i));

                     if (drivers[i]->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION))
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
                              if (!(drivers[j]->flags & (GAME_NOT_WORKING | GAME_UNEMULATED_PROTECTION)))
                              {
                                 foundworking = 1;
                                 break;
                              }
                           }
                           j++;
                        }

                        if (foundworking)
                           fprintf(stdout_file, "| No(1) ");
                        else
                           fprintf(stdout_file, "|   No  ");
                     }
                     else
                        fprintf(stdout_file, "|  Yes  ");

                     if (drivers[i]->flags & GAME_WRONG_COLORS)
                        fprintf(stdout_file, "|   No  ");
                     else if (drivers[i]->flags & GAME_IMPERFECT_COLORS)
                        fprintf(stdout_file, "| Close ");
                     else
                        fprintf(stdout_file, "|  Yes  ");

                     {
                        const char **samplenames = NULL;
#if (HAS_SAMPLES || HAS_VLM5030)
                        for (j = 0;drivers[i]->drv->sound[j].sound_type && j < MAX_SOUND; j++)
                        {
#if (HAS_SAMPLES)
                           if (drivers[i]->drv->sound[j].sound_type == SOUND_SAMPLES)
                           {
                              samplenames = ((struct Samplesinterface *)drivers[i]->drv->sound[j].sound_interface)->samplenames;
                              break;
                           }
#endif
#if (HAS_VLM5030)
                           if (drivers[i]->drv->sound[j].sound_type == SOUND_VLM5030)
                           {
                              samplenames = ((struct VLM5030interface *)drivers[i]->drv->sound[j].sound_interface)->samplenames;
                              break;
                           }
#endif

                        }
#endif
                        if (drivers[i]->flags & GAME_NO_SOUND)
                           fprintf(stdout_file, "|   No  ");
                        else if (drivers[i]->flags & GAME_IMPERFECT_SOUND)
                        {
                           if (samplenames)
                              fprintf(stdout_file, "|Part(2)");
                           else
                              fprintf(stdout_file, "|Partial");
                        }
                        else
                        {
                           if (samplenames)
                              fprintf(stdout_file, "| Yes(2)");
                           else
                              fprintf(stdout_file, "|  Yes  ");
                        }
                     }

                     if (drivers[i]->flags & GAME_NO_COCKTAIL)
                        fprintf(stdout_file, "|   No  ");
                     else
                        fprintf(stdout_file, "|  Yes  ");

                     fprintf(stdout_file, "| %-8s |\n",drivers[i]->name);

                  }
                  break;
               case LIST_SOURCEFILE:
                  fprintf(stdout_file, "%-8s %s\n", drivers[i]->name,
                     drivers[i]->source_file);
                  break;
               case LIST_COLORS:
                  fprintf(stdout_file, "%-8s  %d\n", drivers[i]->name,
                     drivers[i]->drv->total_colors);
                  break;
#ifdef MESS
               case LIST_DEVICES: /* list devices */
                   if(drivers[i]->dev && (drivers[i]->dev->type != IO_END))
                   {
                      const struct IODevice *dev = drivers[i]->dev;
                      
                      j = 0;
                      
                      fprintf(stdout_file, "%-8s  ", drivers[i]->name);
                      
                      while (dev->type != IO_END)
                      {
                         const char *src = dev->file_extensions;
                         
                         if (!j) /* first time ? */
                            fprintf(stdout_file, "%-10s  ",
                               device_typename(dev->type));
                         else
                            fprintf(stdout_file, "%-8s  %-10s  ", "",
                               device_typename(dev->type));

                         while (*src)
                         {
                            fprintf(stdout_file, ".%-3s  ", src);
                            src += strlen(src) + 1;
                         }
                         fprintf(stdout_file, "\n");
                         j++;
                         dev++;
                      }
                   }
                   else
                      skipped++;
                   break;
#endif
               case LIST_ROMSIZE:
                  {
                     const struct RomModule *region, *rom, *chunk;

                     j = 0;
                     for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
                        for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
                           for (chunk = rom_first_chunk(rom); chunk; chunk = rom_next_chunk(chunk))
                              j += ROM_GETLENGTH(chunk);

                              printf("%-8s\t%-5s\t%u\n", drivers[i]->name, drivers[i]->year, j);
                  }
                  break;

               case LIST_ROMS: /* game roms list */
                  if(!frontend_uses_roms(i))
                  {
                     skipped++;
                     continue;
                  }
                  
                  printromlist(drivers[i]->rom, drivers[i]->name);
                  fprintf(stdout_file, "\n");
                  break;
#if (HAS_SAMPLES || HAS_VLM5030)
               case LIST_SAMPLES: /* game samples list */
               case LIST_SAMDIR:  /* games list with samples directories */
                  {
                     int found = 0;
                     
                     for (j = 0; drivers[i]->drv->sound[j].sound_type && j < MAX_SOUND; j++ )
                     {
                        const char **samplenames = NULL;
#if (HAS_SAMPLES)
                        if( drivers[i]->drv->sound[j].sound_type == SOUND_SAMPLES )
                           samplenames = ((struct Samplesinterface *)drivers[i]->drv->sound[j].sound_interface)->samplenames;
#endif                        
#if (HAS_VLM5030)
                        if( drivers[i]->drv->sound[j].sound_type == SOUND_VLM5030 )
                           samplenames = ((struct VLM5030interface *)drivers[i]->drv->sound[j].sound_interface)->samplenames;
#endif                        
                        if (samplenames && samplenames[0])
                        {
                           found = 1;
                           
                           if(list == LIST_SAMPLES)
                           {
                              int k = 0;
                              
                              while (samplenames[k] != 0)
                              {
                                 printf("%s\n", samplenames[k]);
                                 k++;
                              }
                           }
                           else
                           {
                              printf("%-10s",drivers[i]->name);
                              if (samplenames[0][0] == '*')
                                 printf("%s\n",samplenames[0]+1);
                              else
                                 printf("%s\n",drivers[i]->name);
                           }
                        }
                     }
                     if (!found)
                        skipped++;
                  }
                  break;
               case VERIFY_SAMPLESETS:
                  verbose = 0;
                  /* fall through */
               case VERIFY_SAMPLES:
                  {
                     const char **samplenames = NULL;
                     
                     for( j = 0; drivers[i]->drv->sound[j].sound_type && j < MAX_SOUND; j++ )
                     {
#if (HAS_SAMPLES)
                        if( drivers[i]->drv->sound[j].sound_type == SOUND_SAMPLES )
                           samplenames = ((struct Samplesinterface *)drivers[i]->drv->sound[j].sound_interface)->samplenames;
#endif
#if (HAS_VLM5030)
                        if( drivers[i]->drv->sound[j].sound_type == SOUND_VLM5030 )
                           samplenames = ((struct VLM5030interface *)drivers[i]->drv->sound[j].sound_interface)->samplenames;
#endif
                     }
                     
                     /* ignore games that need no samples */
                     if (samplenames == NULL || samplenames[0] == NULL)
                        skipped++;
                     else
                        frontend_verify(i, 0);
                  }
                  break;
#endif
               case VERIFY_ROMSETS:
                  verbose = 0;
                  /* fall through */
               case VERIFY_ROMS:
                  /* ignore games that need no roms */
                  if (!frontend_uses_roms(i))
                     skipped++;
                  else
                     frontend_verify(i, 1);
                  break;
                  
               /*** internal verification list commands (developers only) ***/
               case LIST_MISSINGROMS:
                  if (RomsetMissing (i))
                  {
                     fprintf(stdout_file, "%-10s%-10s%s\n", drivers[i]->name,
                        (drivers[i]->clone_of) ? drivers[i]->clone_of->name : "",
                        get_description(i));
                     not_found++;
                  }
                  break;
               case LIST_DUPCRC:
                  {
                     const struct RomModule *region, *rom;
                     int found = 0;

                     if(!frontend_uses_roms(i))
                     {
                        skipped++;
                        continue;
                     }
                     
                     for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
                        for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
                           if (ROM_GETCRC(rom))
                              for (j = i + 1; drivers[j]; j++)
                              {
                                 const struct RomModule *region1, *rom1;

                                 for (region1 = rom_first_region(drivers[j]); region1; region1 = rom_next_region(region1))
                                    for (rom1 = rom_first_file(region1); rom1; rom1 = rom_next_file(rom1))
                                       if (strcmp(ROM_GETNAME(rom), ROM_GETNAME(rom1)) && ROM_GETCRC(rom) == ROM_GETCRC(rom1))
                                       {
                                          fprintf(stdout_file, "%08x %-12s %-8s <-> %-12s %-8s\n",ROM_GETCRC(rom), ROM_GETNAME(rom),drivers[i]->name, ROM_GETNAME(rom1),drivers[j]->name);
                                          found = 1;
                                       }
                              }

                     if (found)
                        incorrect++;
                     else
                        correct++;
                  }
                  break;
               case LIST_WRONGORIENTATION: /* list drivers which incorrectly use the orientation and visible area fields */
                  if(!(drivers[i]->drv->video_attributes & VIDEO_TYPE_VECTOR) &&
                     ((drivers[i]->drv->default_visible_area.max_x - drivers[i]->drv->default_visible_area.min_x + 1) <=
                      (drivers[i]->drv->default_visible_area.max_y - drivers[i]->drv->default_visible_area.min_y + 1)) &&
                     /* list of valid exceptions */
                     strcmp(drivers[i]->name,"crater") &&
                     strcmp(drivers[i]->name,"mpatrol") &&
                     strcmp(drivers[i]->name,"troangel") &&
                     strcmp(drivers[i]->name,"travrusa") &&
                     strcmp(drivers[i]->name,"kungfum") &&
                     strcmp(drivers[i]->name,"battroad") &&
                     strcmp(drivers[i]->name,"vigilant") &&
                     strcmp(drivers[i]->name,"sonson") &&
                     strcmp(drivers[i]->name,"brkthru") &&
                     strcmp(drivers[i]->name,"darwin") &&
                     strcmp(drivers[i]->name,"exprraid") &&
                     strcmp(drivers[i]->name,"sidetrac") &&
                     strcmp(drivers[i]->name,"targ") &&
                     strcmp(drivers[i]->name,"spectar") &&
                     strcmp(drivers[i]->name,"venture") &&
                     strcmp(drivers[i]->name,"mtrap") &&
                     strcmp(drivers[i]->name,"pepper2") &&
                     strcmp(drivers[i]->name,"hardhat") &&
                     strcmp(drivers[i]->name,"fax") &&
                     strcmp(drivers[i]->name,"circus") &&
                     strcmp(drivers[i]->name,"robotbwl") &&
                     strcmp(drivers[i]->name,"crash") &&
                     strcmp(drivers[i]->name,"ripcord") &&
                     strcmp(drivers[i]->name,"starfire") &&
                     strcmp(drivers[i]->name,"fireone") &&
                     strcmp(drivers[i]->name,"renegade") &&
                     strcmp(drivers[i]->name,"battlane") &&
                     strcmp(drivers[i]->name,"megatack") &&
                     strcmp(drivers[i]->name,"killcom") &&
                     strcmp(drivers[i]->name,"challeng") &&
                     strcmp(drivers[i]->name,"kaos") &&
                     strcmp(drivers[i]->name,"formatz") &&
                     strcmp(drivers[i]->name,"bankp") &&
                     strcmp(drivers[i]->name,"liberatr") &&
                     strcmp(drivers[i]->name,"toki") &&
                     strcmp(drivers[i]->name,"stactics") &&
                     strcmp(drivers[i]->name,"sprint1") &&
                     strcmp(drivers[i]->name,"sprint2") &&
                     strcmp(drivers[i]->name,"nitedrvr") &&
                     strcmp(drivers[i]->name,"punchout") &&
                     strcmp(drivers[i]->name,"spnchout") &&
                     strcmp(drivers[i]->name,"armwrest") &&
                     strcmp(drivers[i]->name,"route16") &&
                     strcmp(drivers[i]->name,"stratvox") &&
                     strcmp(drivers[i]->name,"irobot") &&
                     strcmp(drivers[i]->name,"leprechn") &&
                     strcmp(drivers[i]->name,"starcrus") &&
                     strcmp(drivers[i]->name,"astrof") &&
                     strcmp(drivers[i]->name,"tomahawk") &&
                     strcmp(drivers[i]->name,"astrocde") &&
                     strcmp(drivers[i]->name,"vic20") &&
                     strcmp(drivers[i]->name,"vc20") &&
                     strcmp(drivers[i]->name,"p2000t") &&
                     strcmp(drivers[i]->name,"kim1"))
                  {
                      fprintf(stdout_file, "%s %dx%d\n",drivers[i]->name,
                         drivers[i]->drv->default_visible_area.max_x - drivers[i]->drv->default_visible_area.min_x + 1,
                         drivers[i]->drv->default_visible_area.max_y - drivers[i]->drv->default_visible_area.min_y + 1);
                      incorrect++;
                  } else correct++;
                  break;
               case LIST_WRONGMERGE: /* list duplicate crc-32 with different ROM name in clone sets */
                  {
                     const struct RomModule *region, *rom;
                     int found = 0;
                     
                     if(!frontend_uses_roms(i))
                     {
                        skipped++;
                        continue;
                     }

                     for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
                        for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
                            if (ROM_GETCRC(rom))
                                for (j = 0; drivers[j]; j++)
                                {
                                    if (j != i && drivers[j]->clone_of && (drivers[j]->clone_of->flags & NOT_A_DRIVER) == 0 && (drivers[j]->clone_of == drivers[i] || (i < j && drivers[j]->clone_of == drivers[i]->clone_of)))
                                    {
                                        const struct RomModule *region1, *rom1;
                                        int match = 0;
                                        
                                        for (region1 = rom_first_region(drivers[j]); region1; region1 = rom_next_region(region1))
                                        {
                                            for (rom1 = rom_first_file(region1); rom1; rom1 = rom_next_file(rom1))
                                            {
                                                if (!strcmp(ROM_GETNAME(rom), ROM_GETNAME(rom1)))
                                                {
                                                   if (ROM_GETCRC(rom1) && ROM_GETCRC(rom) != ROM_GETCRC(rom1) && ROM_GETCRC(rom) != BADCRC(ROM_GETCRC(rom1)))
                                                   {
                                                      fprintf(stdout_file,"%-12s %08x %-8s <-> %08x %-8s\n", ROM_GETNAME(rom), ROM_GETCRC(rom), drivers[i]->name, ROM_GETCRC(rom1), drivers[j]->name);
                                                      found = 1;
                                                   }
                                                   else
                                                      match = 1;
                                                }
                                            }
                                        }

                                        if (match == 0)
                                        {
                                           for (region1 = rom_first_region(drivers[j]); region1; region1 = rom_next_region(region1))
                                           {
                                              for (rom1 = rom_first_file(region1); rom1; rom1 = rom_next_file(rom1))
                                              {
                                                 if (strcmp(ROM_GETNAME(rom), ROM_GETNAME(rom1)) && ROM_GETCRC(rom) == ROM_GETCRC(rom1))
                                                 {
                                                    fprintf(stdout_file, "%08x %-12s %-8s <-> %-12s %-8s\n", ROM_GETCRC(rom), ROM_GETNAME(rom), drivers[i]->name, ROM_GETNAME(rom1), drivers[j]->name); found = 1;
                                                 }
                                              }
                                           }
                                        }
                                          
                                    }
                                 }

                     if (found)
                        incorrect++;
                     else
                        correct++;
                  }
                  break;
               case LIST_WRONGFPS: /* list drivers with too high frame rate */
                  if ((drivers[i]->drv->video_attributes & VIDEO_TYPE_VECTOR) == 0 &&
                     (drivers[i]->clone_of == 0 ||
                        (drivers[i]->clone_of->flags & NOT_A_DRIVER)) &&
                     drivers[i]->drv->frames_per_second > 57 &&
                     drivers[i]->drv->default_visible_area.max_y - drivers[i]->drv->default_visible_area.min_y + 1 > 244 &&
                     drivers[i]->drv->default_visible_area.max_y - drivers[i]->drv->default_visible_area.min_y + 1 <= 256)
                  {
                     fprintf(stdout_file, "%-8s  %-4dx%4d   %fHz\n",
                        drivers[i]->name,
                        drivers[i]->drv->default_visible_area.max_x -
                           drivers[i]->drv->default_visible_area.min_x + 1,
                        drivers[i]->drv->default_visible_area.max_y -
                           drivers[i]->drv->default_visible_area.min_y + 1,
                        drivers[i]->drv->frames_per_second);
                     incorrect++;
                  }
                  else
                     correct++;
                  break;
            }
         }
   }
   
   /* print footer for those -list options which need one */
   switch(list)
   {
      case LIST_GAMELIST:
         fprintf(stdout_file,
            "+----------------------------------+-------+-------+-------+-------+----------+\n\n"
            "(1) There are variants of the game (usually bootlegs) that work correctly\n"
#if (HAS_SAMPLES)
            "(2) Needs samples provided separately\n"
#endif
            );
         break;
   }
   
   if (matching == 0)
   {
      fprintf(stderr_file, "Error: \"%s\" is not supported!\n", gamename);
      return 1;
   }
      
   fprintf(stdout_file, "\n\n");
   fprintf(stdout_file, "Total Supported: %d", i);
   if (matching != i)
   {
      fprintf(stdout_file, ", Matching \"%s\": %d\n", gamename, matching);
   }
   else
   {
      fprintf(stdout_file, "\n");
   }
   if (skipped) fprintf(stdout_file, "Displayed: %d, Skipped: %d, because they don't use any roms/samples/devices\n", matching-skipped, skipped);
   if (correct+incorrect) fprintf(stdout_file, "Found: %d, of which %d correct and %d incorrect\n", correct+incorrect, correct, incorrect);
   if (not_found) fprintf(stdout_file, "Not found: %d\n", not_found);
   fflush(stdout_file);
      
   if (incorrect > 0)
      return 2;
   else
      return 0;
}

static int frontend_list_clones(char *gamename)
{
   /* listclones is a special case since the strwildcmp */
   /* also has to be done on clone_of. */
   int i;
   
   fprintf(stdout_file, "Name:    Clone of:\n");
   for (i=0;drivers[i];i++)
   {
      if(drivers[i]->clone_of &&
         !(drivers[i]->clone_of->flags & NOT_A_DRIVER) &&
         ( !strwildcmp(gamename,drivers[i]->name) ||
           !strwildcmp(gamename,drivers[i]->clone_of->name)))
         fprintf(stdout_file, "%-8s %-8s\n",drivers[i]->name,drivers[i]->clone_of->name);
   }
   return 0;
}

static int frontend_list_cpu(void)
{
   int i,j;
   int year;
   
   for (j = 1;j < CPU_COUNT;j++)
      fprintf(stdout_file, "\t%s", cputype_name(j));
      
   fprintf(stdout_file, "\n");
   
   for (year = 1980;year <= 1995;year++)
   {
      int count[CPU_COUNT];
      
      for (j = 0;j < CPU_COUNT;j++)
         count[j] = 0;
      
      i = 0;
      
      while (drivers[i])
      {
         if (drivers[i]->clone_of == 0 || (drivers[i]->clone_of->flags & NOT_A_DRIVER))
         {
            const struct MachineDriver *x_driver = drivers[i]->drv;
            const struct MachineCPU *x_cpu = x_driver->cpu;
            
            if (atoi(drivers[i]->year) == year)
            {
/*               for (j = 0;j < MAX_CPU;j++) */
j = 0;  /* count only the main cpu */
                  count[x_cpu[j].cpu_type & ~CPU_FLAGS_MASK]++;
            }
         }
         i++;
      }
      
      fprintf(stdout_file, "%d", year);
      for (j = 1;j < CPU_COUNT;j++)
         fprintf(stdout_file, "\t%d", count[j]);
         
      fprintf(stdout_file, "\n");
   }
   return OSD_OK;
}

static int frontend_list_gamelistheader(void)
{
   fprintf(stdout_file,
      "This is the complete list of games supported by %s %s.\n",
      NAME, build_version);
   if (!showclones)
      fprintf(stdout_file,
         "Variants of the same game are not included, you can use the -listclones command\n"
         "to get a list of the alternate versions of a given game.\n");
   fprintf(stdout_file, "\n"
      "This list is generated automatically and is not 100%% accurate (particularly in\n"
      "the Screen Flip column). Please let us know of any errors so we can correct\n"
      "them.\n"
      "\n"
      "Here are the meanings of the columns:\n"
      "\n"
      "Working\n"
      "=======\n"
      "  NO: Emulation is still in progress; the game does not work correctly. This\n"
      "  means anything from major problems to a black screen.\n"
      "\n"
      "Correct Colors\n"
      "==============\n"
      "    YES: Colors should be identical to the original.\n"
      "  CLOSE: Colors are nearly correct.\n"
      "     NO: Colors are completely wrong. \n" 
      "  \n"
      "  Note: In some cases, the color PROMs for some games are not yet available.\n"
      "  This causes a NO GOOD DUMP KNOWN message on startup (and, of course, the game\n"
      "  has wrong colors). The game will still say YES in this column, however,\n"
      "  because the code to handle the color PROMs has been added to the driver. When\n"
      "  the PROMs are available, the colors will be correct.\n"
      "\n"
      "Sound\n"
      "=====\n"
      "  PARTIAL: Sound support is incomplete or not entirely accurate. \n"
      "\n"
      "  Note: Some original games contain analog sound circuitry, which is difficult\n"
      "  to emulate. Thereforce, these emulated sounds may be significantly different.\n"
      "\n"
      "Screen Flip\n"
      "===========\n"
      "  Many games were offered in cocktail-table models, allowing two players to sit\n"
      "  across from each other; the game's image flips 180 degrees for each player's\n"
      "  turn. Some games also have a \"Flip Screen\" DIP switch setting to turn the\n"
      "  picture (particularly useful with vertical games).\n"
      "  In many cases, this feature has not yet been emulated.\n"
      "\n"
      "Internal Name\n"
      "=============\n"
      "  This is the unique name that must be used when running the game from a\n"
      "  command line.\n"
      "\n"
      "  Note: Each game's ROM set must be placed in the ROM path, either in a .zip\n"
      "  file or in a subdirectory with the game's Internal Name. The former is\n"
      "  suggested, because the files will be identified by their CRC instead of\n"
      "  requiring specific names.\n\n");
   fprintf(stdout_file, "+----------------------------------+-------+-------+-------+-------+----------+\n");
   fprintf(stdout_file, "|                                  |       |Correct|       |Screen | Internal |\n");
   fprintf(stdout_file, "| Game Name                        |Working|Colors | Sound | Flip  |   Name   |\n");
   fprintf(stdout_file, "+----------------------------------+-------+-------+-------+-------+----------+\n");
   return OSD_OK;
}

static int frontend_list_crcs(void)
{
   int i;
   for (i = 0; drivers[i]; i++)
   {
      const struct RomModule *region, *rom;
      for (region = rom_first_region(drivers[i]); region; region = rom_next_region(region))
         for (rom = rom_first_file(region); rom; rom = rom_next_file(rom))
         fprintf(stdout_file,"%08x %-12s %s\n",ROM_GETCRC(rom),ROM_GETNAME(rom),drivers[i]->description);
}

   return OSD_OK;
}
