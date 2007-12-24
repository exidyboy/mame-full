/***************************************************************************

  M.A.M.E.UI  -  Multiple Arcade Machine Emulator with User Interface
  Win32 Portions Copyright (C) 1997-2003 Michael Soderstrom and Chris Kirmse,
  Copyright (C) 2003-2007 Chris Kirmse and the MAME32/MAMEUI team.

  This file is part of MAMEUI, and may only be used, modified and
  distributed under the terms of the MAME license, in "readme.txt".
  By continuing to use, modify or distribute this file you indicate
  that you have read the license and understand and accept it fully.

 ***************************************************************************/

/***************************************************************************

    mkhelp.c

    Simple resource HIDC to Help entry tool.

    MSH - 20070815

***************************************************************************/

#include <stdio.h>
#include "osdcore.h"


static int compare( const void *arg1, const void *arg2 );
static void extract_help_ids(const char *buffer, FILE *fp);

/*-------------------------------------------------
    main - primary entry point
-------------------------------------------------*/

int main(int argc, char *argv[])
{
	const char *resourcefile, *varname, *type;
	FILE *src, *dst;
	char *buffer;
	int bytes;

	varname = "dwHelpIDs";
	type = "DWORD";

	/* needs at least three arguments */
	if (argc < 1)
	{
		fprintf(stderr,
			"Usage:\n"
			"  mkhelp <resource.rc>\n"
			"\n"
			"The default <type> is DWORD, with an assumed NULL terminator pair\n"
		);
		return 0;
	}

	/* extract arguments */
	resourcefile = argv[1];

	/* open source file */
	src = fopen(resourcefile, "rb");
	if (src == NULL)
	{
		fprintf(stderr, "Unable to open resource file '%s'\n", resourcefile);
		return 1;
	}

	/* determine file size */
	fseek(src, 0, SEEK_END);
	bytes = ftell(src);
	fseek(src, 0, SEEK_SET);

	/* allocate memory */
	buffer = malloc(bytes + 1);
	if (buffer == NULL)
	{
		fprintf(stderr, "Out of memory allocating %d byte buffer\n", bytes);
		return 1;
	}

	/* read the source file */
	fread(buffer, 1, bytes, src);
	buffer[bytes] = 0;
	fclose(src);

	/* open dest file */
	dst = stdout;

	fprintf(dst,"/*\n * Help ID array - Generated by mkhelp\n */\n\n");
	fprintf(dst, "#include <windows.h>\n#include \"resource.h\"\n#include \"resource.hm\"\n\n");
	/* write the initial header */
	fprintf(dst, "const %s %s[] =\n{\n", type, varname);

	extract_help_ids(buffer, dst);

	fprintf(dst, "\n};\n");

	/* close the files */
	free(buffer);
//	fclose(dst);
	return 0;
}

static int compare( const void *arg1, const void *arg2 )
{
   /* Compare all of both strings: */
   return _stricmp( * ( char** ) arg1, * ( char** ) arg2 );
}

static void extract_help_ids(const char *buffer, FILE *fp)
{
	const char *ptr = buffer;
	char **help_ids = malloc(500 * sizeof(char *));
	int num_help_id = 0;
	int i;

	memset(help_ids, '\0', sizeof(help_ids));

	while(*ptr) {
		if (strncmp("HIDC_", ptr, 5) == 0) {
			char id_name[128];
			char *end = id_name;
			char *id;
			memset(id_name, '\0', sizeof(id_name));
			while (*ptr && *ptr != '\x0d' && *ptr != '\x0a' ) {
				*end++ = *ptr++;
			}
			id = malloc(strlen(id_name));
			memset(id, '\0', strlen(id_name));
			memcpy(id, &id_name[1], strlen(&id_name[1]));
			help_ids[num_help_id] = id;
			num_help_id++;
		} else {
			ptr++;
		}
	}

	/* Sort using Quicksort algorithm: */
	qsort( (void *)help_ids, (size_t)num_help_id, sizeof( char * ), compare );

	// Now print them out.
	ptr = help_ids[0];
	for (i = 0; i < num_help_id; i++) {
		if (i > 0) {
			if (strcmp(ptr, help_ids[i]) == 0) {
                continue;
			}
		}
		fprintf(fp, "\t%-30s,H%s,\n", help_ids[i], help_ids[i]);
		ptr = help_ids[i];
	}
	fprintf(fp, "\t%-30i,%i\n", 0, 0);

	// free our allocations.
	for (i = 0; i < num_help_id; i++) {
		free(help_ids[i]);
	}
	free (help_ids);
}

