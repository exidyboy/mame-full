/*
 * X-Mame generic video code
 *
 */
#define __VIDEO_C_
#include <math.h>
#include "xmame.h"

#ifdef xgl
	#include "video-drivers/glmame.h"
#endif
#include <stdio.h>
#include "driver.h"
#include "profiler.h"
#include "input.h"
#include "keyboard.h"
/* for uclock */
#include "sysdep/misc.h"
#include "effect.h"

#define FRAMESKIP_DRIVER_COUNT 2
static const int safety = 16;
static float beam_f;
int normal_widthscale = 1, normal_heightscale = 1;
int yarbsize = 0;
static char *vector_res = NULL;
static int use_auto_double = 1;
static int frameskipper = 0;
static int brightness = 100;
float brightness_paused_adjust = 1.0;
static int bitmap_depth;
static struct mame_bitmap *scrbitmap = NULL;
static int debugger_has_focus = 0;
static struct rectangle normal_visual;
static struct rectangle debug_visual;

#if (defined svgafx) || (defined xfx) 
UINT16 *color_values;
#endif


float gamma_correction = 1.0;

/* some prototypes */
static int video_handle_scale(struct rc_option *option, const char *arg,
   int priority);
static int video_verify_beam(struct rc_option *option, const char *arg,
   int priority);
static int video_verify_flicker(struct rc_option *option, const char *arg,
   int priority);
static int video_verify_bpp(struct rc_option *option, const char *arg,
   int priority);
static int video_verify_vectorres(struct rc_option *option, const char *arg,
   int priority);
static void osd_free_colors(void);
static void update_visible_area(struct mame_display *display);

struct rc_option video_opts[] = {
   /* name, shortname, type, dest, deflt, min, max, func, help */
   { "Video Related",	NULL,			rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "bpp",		"b",			rc_int,		&options.color_depth,
     "0",		0,			0,		video_verify_bpp,
     "Specify the colordepth the core should render, one of: auto(0), 8, 16" },
   { "arbheight",	"ah",			rc_int,		&yarbsize,
     "0",		0,			4096,		NULL,
     "Scale video to exactly this height (0 = disable)" },
   { "heightscale",	"hs",			rc_int,		&normal_heightscale,
     "1",		1,			8,		NULL,
     "Set Y-Scale aspect ratio" },
   { "widthscale",	"ws",			rc_int,		&normal_widthscale,
     "1",		1,			8,		NULL,
     "Set X-Scale aspect ratio" },
   { "scale",		"s",			rc_use_function, NULL,
     NULL,		0,			0,		video_handle_scale,
     "Set X-Y Scale to the same aspect ratio. For vector games scale (and also width- and heightscale) may have value's like 1.5 and even 0.5. For scaling of regular games this will be rounded to an int" },
   { "effect",		"ef",			rc_int,		&effect,
     EFFECT_NONE,	EFFECT_NONE,		EFFECT_LAST,	NULL,
     "Video effect:\n"
	     "0 = none (default)\n"
	     "1 = scale2x (smooth scaling effect)\n"
	     "2 = scan2 (light scanlines)\n"
	     "3 = rgbstripe (3x2 rgb vertical stripes)\n"
	     "4 = rgbscan (2x3 rgb horizontal scanlines)\n"
	     "5 = scan3 (3x3 deluxe scanlines)\n" },
   { "autodouble",	"adb",			rc_bool,	&use_auto_double,
     "1",		0,			0,		NULL,
     "Enable/disable automatic scale doubling for 1:2 pixel aspect ratio games" },
   { "scanlines",	"sl",			rc_bool,	&use_scanlines,
     "0",		0,			0,		NULL,
     "Enable/disable displaying simulated scanlines" },
   { "artwork",		"a",			rc_bool,	&options.use_artwork,
     "1",		0,			0,		NULL,
     "Use/don't use artwork if available" },
   { "frameskipper",	"fsr",			rc_int,		&frameskipper,
     "0",		0,			FRAMESKIP_DRIVER_COUNT-1, NULL,
     "Select which autoframeskip and throttle routines to use. Available choices are:\n0 Dos frameskip code\n1 Enhanced frameskip code by William A. Barath" },
   { "throttle",	"th",			rc_bool,	&throttle,
     "1",		0,			0,		NULL,
     "Enable/disable throttle" },
   { "sleepidle",	"si",			rc_bool,	&sleep_idle,
     "0",		0,			0,		NULL,
     "Enable/disable sleep during idle" },
   { "autoframeskip",	"afs",			rc_bool,	&autoframeskip,
     "1",		0,			0,		NULL,
     "Enable/disable autoframeskip" },
   { "maxautoframeskip", "mafs",		rc_int,		&max_autoframeskip,
     "8",		0,			FRAMESKIP_LEVELS-1, NULL,
     "Set highest allowed frameskip for autoframeskip" },
   { "frameskip",	"fs",			rc_int,		&frameskip,
     "0",		0,			FRAMESKIP_LEVELS-1, NULL,
     "Set frameskip when not using autoframeskip" },
   { "brightness",	"brt",			rc_int,		&brightness,
     "100",		0,			100,		NULL,
     "Set the brightness (0-100%%)" },
   { "gamma-correction", "gc",			rc_float,	&gamma_correction,
     "1.0",		0.5,			2.0,		NULL,
     "Set the gamma-correction (0.5-2.0)" },
   { "norotate",	"nr",			rc_set_int,	&options.norotate,
     NULL,		1,			0,		NULL,
     "Disable rotation" },
   { "ror",		"rr",			rc_set_int,	&options.ror,
     NULL,		1,			0,		NULL,
     "Rotate display 90 degrees rigth" },
   { "rol",		"rl",			rc_set_int,	&options.rol,
     NULL,		1,			0,		NULL,
     "Rotate display 90 degrees left" },
   { "flipx",		"fx",			rc_set_int,	&options.flipx,
     NULL,		1,			0,		NULL,
     "Flip X axis" },
   { "flipy",		"fy",			rc_set_int,	&options.flipy,
     NULL,		1,			0,		NULL,
     "Flip Y axis" },
   { "Vector Games Related", NULL,		rc_seperator,	NULL,
     NULL,		0,			0,		NULL,
     NULL },
   { "vectorres",	"vres",			rc_string,	&vector_res,
     NULL,		0,			0,		video_verify_vectorres,
     "Always scale vectorgames to XresxYres, keeping their aspect ratio. This overrides the scale options" },
   { "beam",		"B",			rc_float,	&beam_f,
     "1.0",		1.0,			16.0,		video_verify_beam,
     "Set the beam size for vector games" },
   { "flicker",		"f",			rc_float,	&options.vector_flicker,
     "0.0",		0.0,			100.0,		video_verify_flicker,
     "Set the flicker for vector games" },
   { "antialias",	"aa",			rc_bool,	&options.antialias,
     "1",		0,			0,		NULL,
     "Enable/disable antialiasing" },
   { "translucency",	"t",			rc_bool,	&options.translucency,
     "1",		0,			0,		NULL,
     "Enable/disable tranlucency" },
   { NULL,		NULL,			rc_link,	display_opts,
     NULL,		0,			0,		NULL,
     NULL },     
   { NULL,		NULL,			rc_end,		NULL,
     NULL,		0,			0,		NULL,
     NULL }
};

static int video_handle_scale(struct rc_option *option, const char *arg,
   int priority)
{
   if (rc_set_option2(video_opts, "widthscale", arg, priority))
      return -1;
   if (rc_set_option2(video_opts, "heightscale", arg, priority))
      return -1;
      
   option->priority = priority;
   
   return 0;
}

static int video_verify_beam(struct rc_option *option, const char *arg,
   int priority)
{
   options.beam = (int)(beam_f * 0x00010000);
   if (options.beam < 0x00010000)
      options.beam = 0x00010000;
   else if (options.beam > 0x00100000)
      options.beam = 0x00100000;

   option->priority = priority;
   
   return 0;
}

static int video_verify_flicker(struct rc_option *option, const char *arg,
   int priority)
{
   if (options.vector_flicker < 0.0)
      options.vector_flicker = 0.0;
   else if (options.vector_flicker > 100.0)
      options.vector_flicker = 100.0;

   option->priority = priority;
   
   return 0;
}

static int video_verify_bpp(struct rc_option *option, const char *arg,
   int priority)
{
   if( (options.color_depth != 0) &&
       (options.color_depth != 8) &&
       (options.color_depth != 15) &&
       (options.color_depth != 16) &&
       (options.color_depth != 32) )
   {
      options.color_depth = 0;
      fprintf(stderr, "error: invalid value for bpp: %s\n", arg);
      return -1;
   }

   option->priority = priority;
   
   return 0;
}

static int video_verify_vectorres(struct rc_option *option, const char *arg,
   int priority)
{
   if(sscanf(arg, "%dx%d", &options.vector_width, &options.vector_height) != 2)
   {
      options.vector_width = options.vector_height = 0;
      fprintf(stderr, "error: invalid value for vectorres: %s\n", arg);
      return -1;
   }

   option->priority = priority;
   
   return 0;
}

int osd_create_display(const struct osd_create_params *params, UINT32 *rgb_components)
{
   int r, g, b;
   struct mame_display dummy_display;

   bitmap_depth = (params->depth != 15) ? params->depth : 16;

   current_palette = normal_palette = NULL;
   debug_visual.min_x = 0;
   debug_visual.max_x = options.debug_width - 1;
   debug_visual.min_y = 0;
   debug_visual.max_y = options.debug_height - 1;

   if (use_auto_double)
   {
      if ( (params->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK) ==
         VIDEO_PIXEL_ASPECT_RATIO_1_2)
      {
         if (params->orientation & ORIENTATION_SWAP_XY)
            normal_widthscale  *= 2;
         else
            normal_heightscale *= 2;
      }
      if ( (params->video_attributes & VIDEO_PIXEL_ASPECT_RATIO_MASK) ==
         VIDEO_PIXEL_ASPECT_RATIO_2_1)
      {
         if (params->orientation & ORIENTATION_SWAP_XY)
            normal_heightscale *= 2;
         else
            normal_widthscale  *= 2;
      }
   }
  
   visual_width     = params->width;
   visual_height    = params->height;
   widthscale       = normal_widthscale;
   heightscale      = normal_heightscale;
   use_aspect_ratio = normal_use_aspect_ratio;
   video_fps        = params->fps;
   
   if (sysdep_create_display(bitmap_depth) != OSD_OK)
      return -1;
   
   /* a lot of display_targets need to have the display initialised before
      initialising any input devices */
   if (osd_input_initpost() != OSD_OK) return -1;
   
   if (bitmap_depth == 16) fprintf(stderr_file,"Using 16bpp video mode\n");

   if (!(normal_palette = sysdep_palette_create(bitmap_depth, 65536)))
   {
      return 1;
   }

   /* alloc the total number of colors that can be used by the palette */
   if (sysdep_display_alloc_palette(65536))
   {
      osd_free_colors();
      return 1;
   }

   sysdep_palette_set_gamma(normal_palette, gamma_correction);
   sysdep_palette_set_brightness(normal_palette, 
      brightness * brightness_paused_adjust);

   /* initialize the palette to a fixed 5-5-5 mapping */
   for (r = 0; r < 32; r++)
      for (g = 0; g < 32; g++)
         for (b = 0; b < 32; b++)
         {
            int idx = (r << 10) | (g << 5) | b;
            sysdep_palette_set_pen(normal_palette, idx, r, g, b);
         }

   current_palette = normal_palette;

   /* fill in the resulting RGB components */
   if (rgb_components)
   {
      if (bitmap_depth == 32)
      {
         rgb_components[0] = (0xff << 16) | (0x00 << 8) | 0x00;
         rgb_components[1] = (0x00 << 16) | (0xff << 8) | 0x00;
         rgb_components[2] = (0x00 << 16) | (0x00 << 8) | 0xff;
      }
      else
      {
         rgb_components[0] = 0x7c00;
         rgb_components[1] = 0x03e0;
         rgb_components[2] = 0x001f;
      }
   }

   return 0;
}   

void osd_close_display (void)
{
   osd_free_colors();
   sysdep_display_close();
}

static void osd_change_display_settings(struct rectangle *new_visual,
   struct sysdep_palette_struct *new_palette, int new_widthscale,
   int new_heightscale, int new_use_aspect_ratio)
{
   int new_visual_width, new_visual_height;
  
   /* always update the visual info */
   visual = *new_visual;
   
   /* calculate the new visual width / height */
   new_visual_width  = visual.max_x - visual.min_x + 1;
   new_visual_height = visual.max_y - visual.min_y + 1;
   
   if( current_palette != new_palette )
      current_palette = new_palette;
   
   if( (visual_width     != new_visual_width    ) ||
       (visual_height    != new_visual_height   ) ||
       (widthscale       != new_widthscale      ) ||
       (heightscale      != new_heightscale     ) ||
       (use_aspect_ratio != new_use_aspect_ratio) )
   {
      sysdep_display_close();
      
      visual_width     = new_visual_width;
      visual_height    = new_visual_height;
      widthscale       = new_widthscale;
      heightscale      = new_heightscale;
      use_aspect_ratio = new_use_aspect_ratio;
      
      if (sysdep_create_display(bitmap_depth) != OSD_OK)
      {
         /* oops this sorta sucks */
         fprintf(stderr_file, "Argh, resizing the display failed in osd_set_visible_area, aborting\n");
         exit(1);
      }
      
      /* only realloc the palette if it has been initialised */
      if (current_palette)
      {
         if (sysdep_display_alloc_palette(video_colors_used))
         {
            /* better restore the video mode before calling exit() */
            sysdep_display_close();
            /* oops this sorta sucks */
            fprintf(stderr_file, "Argh, (re)allocating the palette failed in osd_set_visible_area, aborting\n");
            exit(1);
         }
      }
      
      /* to stop keys from getting stuck */
      xmame_keyboard_clear();
         
      /* for debugging only */
      fprintf(stderr_file, "viswidth = %d, visheight = %d,"
              "visstartx= %d, visstarty= %d\n",
               visual_width, visual_height, visual.min_x,
               visual.min_y);
   }
}

static void update_visible_area(struct mame_display *display)
{
   normal_visual.min_x = display->game_visible_area.min_x;
   normal_visual.max_x = display->game_visible_area.max_x;
   normal_visual.min_y = display->game_visible_area.min_y;
   normal_visual.max_y = display->game_visible_area.max_y;

   /* round to 8, since the new dirty code works with 8x8 blocks,
      and we need to round to sizeof(long) for the long copies anyway */
   if (normal_visual.min_x & 7)
   {
      if((normal_visual.min_x - (normal_visual.min_x & ~7)) < 4)
         normal_visual.min_x &= ~7;
       else
         normal_visual.min_x = (normal_visual.min_x + 7) & ~7;
   }
   if ((normal_visual.max_x + 1) & 7)
   {
      if(((normal_visual.max_x + 1) - ((normal_visual.max_x + 1) & ~7)) > 4)
         normal_visual.max_x = ((normal_visual.max_x + 1 + 7) & ~7) - 1;
       else
         normal_visual.max_x = ((normal_visual.max_x + 1) & ~7) - 1;
   }
   
   if(!debugger_has_focus)
      osd_change_display_settings(&normal_visual, normal_palette,
         normal_widthscale, normal_heightscale, normal_use_aspect_ratio);
   
   set_ui_visarea (normal_visual.min_x, normal_visual.min_y, normal_visual.max_x, normal_visual.max_y);
}

static void update_palette(struct mame_display *display)
{
	int i, j;

	/* loop over dirty colors in batches of 32 */
	for (i = 0; i < display->game_palette_entries; i += 32)
	{
		UINT32 dirtyflags = display->game_palette_dirty[i / 32];
		if (dirtyflags)
		{
			display->game_palette_dirty[i / 32] = 0;
			
			/* loop over all 32 bits and update dirty entries */
			for (j = 0; j < 32; j++, dirtyflags >>= 1)
				if (dirtyflags & 1)
				{
					/* extract the RGB values */
					rgb_t rgbvalue = display->game_palette[i + j];
					int r = RGB_RED(rgbvalue);
					int g = RGB_GREEN(rgbvalue);
					int b = RGB_BLUE(rgbvalue);

					sysdep_palette_set_pen(current_palette, 
						i + j, r, g, b);
				}
		}
	}
}

static void osd_free_colors(void)
{
   if(normal_palette)
   {
      sysdep_palette_destroy(normal_palette);
      normal_palette = NULL;
   }
   if(debug_palette)
   {
      sysdep_palette_destroy(debug_palette);
      debug_palette = NULL;
   }
}

static int skip_next_frame = 0;

typedef int (*skip_next_frame_func)(int show_fps_counter, struct mame_bitmap *bitmap);
static skip_next_frame_func skip_next_frame_functions[FRAMESKIP_DRIVER_COUNT] =
{
   dos_skip_next_frame,
   barath_skip_next_frame
};

int osd_skip_this_frame(void)
{   
   return skip_next_frame;
}

int osd_get_frameskip(void)
{
	return autoframeskip ? -(frameskip + 1) : frameskip;
}

void osd_debugger_focus(int new_debugger_focus)
{
   if( (!debugger_has_focus &&  new_debugger_focus) || 
       ( debugger_has_focus && !new_debugger_focus))
   {
      if(new_debugger_focus)
         osd_change_display_settings(&debug_visual, debug_palette,
            1, 1, 0);
      else
         osd_change_display_settings(&normal_visual, normal_palette,
            normal_widthscale, normal_heightscale, normal_use_aspect_ratio);
      
      debugger_has_focus = new_debugger_focus;
   }
}

/* Update the display. */
void osd_update_video_and_audio(struct mame_display *display)
{
   struct rectangle updatebounds = display->game_bitmap_update;
   static int showfps = 0, showfpstemp = 0; 
   int skip_this_frame;
   struct mame_bitmap *current_bitmap = display->game_bitmap;
   
/* save the active bitmap for use in osd_clearbitmap, I know this
      sucks blame the core ! */
   scrbitmap = display->game_bitmap;

   if (input_ui_pressed(IPT_UI_FRAMESKIP_INC))
   {
      if (autoframeskip)
      {
	 autoframeskip = 0;
	 frameskip = 0;
      }
      else
      {
	 if (frameskip == FRAMESKIP_LEVELS-1)
	 {
	    frameskip = 0;
	    autoframeskip = 1;
	 }
	 else frameskip++;
      }

      if (showfps == 0) showfpstemp = 2 * Machine->drv->frames_per_second;
   }

   if (input_ui_pressed(IPT_UI_FRAMESKIP_DEC))
   {
      if (autoframeskip)
      {
	 autoframeskip = 0;
	 frameskip = FRAMESKIP_LEVELS-1;
      }
      else
      {
	 if (frameskip == 0) autoframeskip = 1;
	 else frameskip--;
      }
      
      if (showfps == 0)	showfpstemp = 2 * Machine->drv->frames_per_second;
   }
   
   if (!keyboard_pressed(KEYCODE_LSHIFT) && !keyboard_pressed(KEYCODE_RSHIFT)
       && !keyboard_pressed(KEYCODE_LCONTROL) && !keyboard_pressed(KEYCODE_RCONTROL)
       && input_ui_pressed(IPT_UI_THROTTLE))
   {
      throttle ^= 1;
   }
   
   if (input_ui_pressed(IPT_UI_THROTTLE) && (keyboard_pressed(KEYCODE_RSHIFT) || keyboard_pressed(KEYCODE_LSHIFT)))
   {
      sleep_idle ^= 1;
   }
   
   if (!keyboard_pressed(KEYCODE_LSHIFT) && !keyboard_pressed(KEYCODE_RSHIFT)
       && !keyboard_pressed(KEYCODE_LCONTROL) && !keyboard_pressed(KEYCODE_RCONTROL)
       && input_ui_pressed(IPT_UI_SHOW_FPS))
   {
      if (showfpstemp)
      {
	 showfpstemp = 0;
	 schedule_full_refresh();
      }
      else
      {
	 showfps ^= 1;
	 if (showfps == 0)
	 {
	    schedule_full_refresh();
	 }
      }
   }
   
   if (keyboard_pressed (KEYCODE_LCONTROL))
   { 
      if (keyboard_pressed_memory (KEYCODE_INSERT))
         frameskipper = 0;
      if (keyboard_pressed_memory (KEYCODE_HOME))
         frameskipper = 1;
   }
   
   if (display->debug_bitmap)
   {
      if (input_ui_pressed(IPT_UI_TOGGLE_DEBUG))
         osd_debugger_focus(!debugger_has_focus);
      if (debugger_has_focus)
         current_bitmap = display->debug_bitmap;
   }
   /* this should not happen I guess, but better safe than sorry */
   else if (debugger_has_focus)
      osd_debugger_focus(0);

   if (keyboard_pressed (KEYCODE_LSHIFT))
   {
      int widthscale_mod  = 0;
      int heightscale_mod = 0;
      
      if (keyboard_pressed_memory (KEYCODE_INSERT))
         widthscale_mod = 1;
      if (keyboard_pressed_memory (KEYCODE_DEL))
         widthscale_mod = -1;
      if (keyboard_pressed_memory (KEYCODE_HOME))
         heightscale_mod = 1;
      if (keyboard_pressed_memory (KEYCODE_END))
         heightscale_mod = -1;
      if (keyboard_pressed_memory (KEYCODE_PGUP))
      {
         widthscale_mod  = 1;
         heightscale_mod = 1;
      }
      if (keyboard_pressed_memory (KEYCODE_PGDN))
      {
         widthscale_mod  = -1;
         heightscale_mod = -1;
      }
      if (widthscale_mod || heightscale_mod)
      {
         normal_widthscale  += widthscale_mod;
         normal_heightscale += heightscale_mod;
         
         if (normal_widthscale > 8)
            normal_widthscale = 8;
         else if (normal_widthscale < 1)
            normal_widthscale = 1;
         if (normal_heightscale > 8)
            normal_heightscale = 8;
         else if (normal_heightscale < 1)
            normal_heightscale = 1;
         
         if (!debugger_has_focus)
            osd_change_display_settings(&normal_visual, normal_palette,
               normal_widthscale, normal_heightscale, normal_use_aspect_ratio);
      }
   }
   
   if (showfpstemp)         /* MAURY_BEGIN: nuove opzioni */
   {
      showfpstemp--;
      if (showfpstemp == 0) schedule_full_refresh();
   }

   skip_this_frame = skip_next_frame;
   skip_next_frame =
      (*skip_next_frame_functions[frameskipper])(showfps || showfpstemp,
        display->game_bitmap);
   
   if (sound_stream && sound_enabled)
      sound_stream_update(sound_stream);

   /* if the visible area has changed, update it */
   if (display->changed_flags & GAME_VISIBLE_AREA_CHANGED)
      update_visible_area(display);

   /* if the debugger focus changed, update it */
   /*if (display->changed_flags & DEBUG_FOCUS_CHANGED)
      win_set_debugger_focus(display->debug_focus); */

   /* if the game palette has changed, update it */
   if (display->changed_flags & GAME_PALETTE_CHANGED)
      update_palette(display);

   if (skip_this_frame == 0)
   {
      profiler_mark(PROFILER_BLIT);
      sysdep_update_display(current_bitmap);
      profiler_mark(PROFILER_END);
   }

   /* update the debugger */
   /*if (display->changed_flags & DEBUG_BITMAP_CHANGED)
   {
      win_update_debug_window(display->debug_bitmap, display->debug_palette);
      debugger_was_visible = 1;
   }*/

   /* if the LEDs have changed, update them */
   if (display->changed_flags & LED_STATE_CHANGED)
      sysdep_set_leds(display->led_state);
   
   osd_poll_joysticks();
}

void osd_set_gamma(float gamma)
{
   sysdep_palette_set_gamma(normal_palette, gamma);
   if(debug_palette)
      sysdep_palette_set_gamma(debug_palette, gamma);
}

float osd_get_gamma(void)
{
   return sysdep_palette_get_gamma(normal_palette);
}

/* brightess = percentage 0-100% */
void osd_set_brightness(int _brightness)
{
   brightness = _brightness;
   sysdep_palette_set_brightness(normal_palette, brightness *
      brightness_paused_adjust);
   if (debug_palette)
      sysdep_palette_set_brightness(debug_palette, brightness);
}

int osd_get_brightness(void)
{
   return brightness;
}

#ifndef xgl
void osd_save_snapshot(struct mame_bitmap *bitmap, 
		const struct rectangle *bounds)
{
   save_screen_snapshot(bitmap, bounds);
}
#endif

void osd_pause(int paused)
{
   if (paused) 
      brightness_paused_adjust = 0.65;
   else
      brightness_paused_adjust = 1.0;

   osd_set_brightness(brightness);
}

const char *osd_get_fps_text(const struct performance_info *performance)
{
	static char buffer[1024];
	char *dest = buffer;
	
	/* display the FPS, frameskip, percent, fps and target fps */
	dest += sprintf(dest, "%s%2d%4d%%%4d/%d fps", 
			autoframeskip ? "auto" : "fskp", frameskip, 
			(int)(performance->game_speed_percent + 0.5), 
			(int)(performance->frames_per_second + 0.5),
			(int)(Machine->drv->frames_per_second + 0.5));

	/* for vector games, add the number of vector updates */
	if (Machine->drv->video_attributes & VIDEO_TYPE_VECTOR)
	{
		dest += sprintf(dest, "\n %d vector updates", performance->vector_updates_last_second);
	}
	else if (performance->partial_updates_this_frame > 1)
	{
		dest += sprintf(dest, "\n %d partial updates", performance->partial_updates_this_frame);
	}
	
	/* return a pointer to the static buffer */
	return buffer;
}
