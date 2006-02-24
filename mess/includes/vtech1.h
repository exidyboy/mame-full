#include "devices/snapquik.h"

/* from machine/vtech1.c */

extern char vtech1_frame_message[64+1];
extern int vtech1_frame_time;

extern int vtech1_latch;


#define LASER310_MAIN_OSCILLATOR 17734000 /* 17.734MHz */


/******************************************************************************
 Machine Initialisations
******************************************************************************/

MACHINE_RESET( laser110 );
MACHINE_RESET( laser200 );
MACHINE_RESET( laser210 );
MACHINE_RESET( laser310 );


/******************************************************************************
 Devices
******************************************************************************/

DEVICE_LOAD( vtech1_floppy );
SNAPSHOT_LOAD( vtech1 );


/******************************************************************************
 Read/Write Handlers
******************************************************************************/

READ8_HANDLER ( vtech1_printer_r );
WRITE8_HANDLER( vtech1_printer_w );
READ8_HANDLER ( vtech1_fdc_r );
WRITE8_HANDLER( vtech1_fdc_w );
READ8_HANDLER ( vtech1_joystick_r );
READ8_HANDLER ( vtech1_lightpen_r );
READ8_HANDLER ( vtech1_keyboard_r );
WRITE8_HANDLER( vtech1_latch_w );
READ8_HANDLER ( vtech1_serial_r );
WRITE8_HANDLER( vtech1_serial_w );
WRITE8_HANDLER( vtech1_memory_bank_w );


/******************************************************************************
 Interrup & Video
******************************************************************************/

void vtech1_interrupt(void);

VIDEO_START( vtech1 );
