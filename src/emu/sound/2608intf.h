#ifndef __2608INTF_H__
#define __2608INTF_H__

#include "fm.h"
#include "ay8910.h"

struct YM2608interface
{
	const struct AY8910interface ay8910_intf;
	void ( *handler )( int irq );	/* IRQ handler for the YM2608 */
	int pcmrom;		/* Delta-T memory region ram/rom */
};

/************************************************/
/* Chip 0 functions             */
/************************************************/
READ8_HANDLER( YM2608_status_port_0_A_r );
READ8_HANDLER( YM2608_status_port_0_B_r );
READ8_HANDLER( YM2608_read_port_0_r );
WRITE8_HANDLER( YM2608_control_port_0_A_w );
WRITE8_HANDLER( YM2608_control_port_0_B_w );
WRITE8_HANDLER( YM2608_data_port_0_A_w );
WRITE8_HANDLER( YM2608_data_port_0_B_w );

/************************************************/
/* Chip 1 functions             */
/************************************************/
READ8_HANDLER( YM2608_status_port_1_A_r );
READ8_HANDLER( YM2608_status_port_1_B_r );
READ8_HANDLER( YM2608_read_port_1_r );
WRITE8_HANDLER( YM2608_control_port_1_A_w );
WRITE8_HANDLER( YM2608_control_port_1_B_w );
WRITE8_HANDLER( YM2608_data_port_1_A_w );
WRITE8_HANDLER( YM2608_data_port_1_B_w );

#endif /* __2608INTF_H__ */
