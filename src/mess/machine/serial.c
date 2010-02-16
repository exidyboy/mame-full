/* internal serial transmission */

/* select a file on host filesystem to transfer using serial method,
setup serial interface software in driver and let the transfer begin */

/* this is used in the Amstrad NC Notepad emulation */



/*

    the output starts at 1 level. It changes to 0 when the start bit has been transmitted.
    This therefore signals that data is following.
    When all data bits have been received, stop bits are transmitted with a value of 1.

    msm8251 expects this in asynchronous mode:
    packet format:

    bit count           function        value
    1                   start bit       0
    note 1              data bits       x
    note 2              parity bit      x
    note 3              stop bits       1

    Note:
    1. Data size can be defined (usual value is 8)
    2. Parity bit (if parity is set to odd or even). Value of bit
        is defined by data parity.
    3. There should be at least 1 stop bit.
*/

#include "emu.h"
#include "serial.h"


#define VERBOSE 0
#define LOG(x) do { if (VERBOSE) logerror x; } while (0)

/* a read/write bit stream. used to transmit data and to receive data */
struct data_stream
{
	/* pointer to buffer */
	unsigned char *pData;
	/* length of buffer */
	unsigned long DataLength;

	/* bit offset within current byte */
	unsigned long BitCount;
	/* byte offset within data */
	unsigned long ByteCount;
};

typedef struct _serial_t serial_t;
struct _serial_t
{
	/* transmit data bit-stream */
	struct data_stream transmit;
	/* receive data bit-stream */
	struct data_stream receive;

	/* register to receive data */
	struct serial_receive_register	receive_reg;
	/* register to transmit data */
	struct serial_transmit_register transmit_reg;

	/* connection to transmit/receive data over */
	struct serial_connection connection;

	/* data form to transmit/receive */
	struct data_form data_form;

	int transmit_state;

	/* baud rate */
	unsigned long BaudRate;

	/* baud rate timer */
	emu_timer	*timer;
};

INLINE serial_t *get_safe_token(running_device *device)
{
	assert(device != NULL);
	assert(device->token != NULL);

	return (serial_t *)device->token;
}


/* the serial streams */
static TIMER_CALLBACK(serial_device_baud_rate_callback);


/*********************************************************/

static unsigned char serial_parity_table[256];

void serial_helper_setup(void)
{
	int i;

	/* if sum of all bits in the byte is even, then the data
    has even parity, otherwise it has odd parity */
	for (i=0; i<256; i++)
	{
		int data;
		int sum;
		int b;

		sum = 0;
		data = i;

		for (b=0; b<8; b++)
		{
			sum+=data & 0x01;

			data = data>>1;
		}

		serial_parity_table[i] = sum & 0x01;
	}
}

static unsigned char serial_helper_get_parity(unsigned char data)
{
	return serial_parity_table[data & 0x0ff];
}

static void serial_device_in_callback(running_machine *machine, int id, unsigned long status)
{
/*  serial_t *ser = get_safe_token(device);

    ser->connection.input_state = status;*/
}

/***** SERIAL DEVICE ******/
void serial_device_setup(running_device *device, int baud_rate, int num_data_bits, int stop_bit_count, int parity_code)
{
	serial_t *ser = get_safe_token(device);

	ser->BaudRate = baud_rate;
	ser->data_form.word_length = num_data_bits;
	ser->data_form.stop_bit_count = stop_bit_count;
	ser->data_form.parity = parity_code;
	ser->timer = timer_alloc(device->machine, serial_device_baud_rate_callback, (void *)device);

	serial_connection_init(device->machine,&ser->connection);
	serial_connection_set_in_callback(device->machine,&ser->connection, serial_device_in_callback);

	/* signal to other end it is clear to send! */
	/* data is initially high state */
	/* set /rts */
	ser->connection.State |= SERIAL_STATE_RTS;
	/* signal to other end data is ready to be accepted */
	/* set /dtr */
	ser->connection.State |= SERIAL_STATE_DTR;
	set_out_data_bit(ser->connection.State, 1);
	serial_connection_out(device->machine,&ser->connection);
	transmit_register_reset(&ser->transmit_reg);
	receive_register_reset(&ser->receive_reg);
	receive_register_setup(&ser->receive_reg, &ser->data_form);
}


unsigned long serial_device_get_state(running_device *device)
{
	serial_t *ser = get_safe_token(device);

	return ser->connection.State;
}

void serial_device_set_transmit_state(running_device *device, int state)
{
	int previous_state;
	serial_t *ser = get_safe_token(device);

	previous_state = ser->transmit_state;

	ser->transmit_state = state;

	if ((state^previous_state)!=0)
	{
		if (state)
		{
			/* start timer */
			timer_adjust_periodic(ser->timer, attotime_zero, 0, ATTOTIME_IN_HZ(ser->BaudRate));
		}
		else
		{
			/* remove timer */
			timer_reset(ser->timer, attotime_never);
		}
	}

}

/* get a bit from input stream */
static int	data_stream_get_data_bit_from_data_byte(struct data_stream *stream)
{
	int data_bit;
	int data_byte;

	if (stream->ByteCount<stream->DataLength)
	{
		/* get data from buffer */
		data_byte= stream->pData[stream->ByteCount];
	}
	else
	{
		/* over end of buffer, so return 0 */
		data_byte= 0;
	}

	/* get bit from data */
	data_bit = (data_byte>>(7-stream->BitCount)) & 0x01;

	/* update bit count */
	stream->BitCount++;
	/* ripple overflow onto byte count */
	stream->ByteCount+=stream->BitCount>>3;
	/* lock bit count into range */
	stream->BitCount &=0x07;

	/* do not let it read over end of data */
	if (stream->ByteCount>=stream->DataLength)
	{
		stream->ByteCount = stream->DataLength-1;
	}

	return data_bit;
}


void	receive_register_setup(struct serial_receive_register *receive, struct data_form *data_form)
{
	receive->bit_count = data_form->word_length + data_form->stop_bit_count;

	if (data_form->parity != SERIAL_PARITY_NONE)
	{
		receive->bit_count++;
	}
}


/* this is generic code to be used in serial chip implementations */
/* the majority of serial chips work in the same way and this code will work */
/* for them */

/* receive a bit */
void	receive_register_update_bit(struct serial_receive_register *receive, int bit)
{
	int previous_bit;

	LOG(("receive register receive bit: %1x\n",bit));
	previous_bit = receive->register_data & 1;

	/* shift previous bit 7 out */
	receive->register_data = receive->register_data<<1;
	/* shift new bit in */
	receive->register_data = (receive->register_data & 0xfffe) | bit;
	/* update bit count received */
	receive->bit_count_received++;

	/* asyncrhonouse mode */
	if (receive->flags & RECEIVE_REGISTER_WAITING_FOR_START_BIT)
	{
		/* the previous bit is stored in uart.receive char bit 0 */
		/* has the bit state changed? */
		if (((previous_bit ^ bit) & 0x01)!=0)
		{
			/* yes */
			if (bit==0)
			{
				//logerror("receive register saw start bit\n");

				/* seen start bit! */
				/* not waiting for start bit now! */
				receive->flags &=~RECEIVE_REGISTER_WAITING_FOR_START_BIT;
				receive->flags |=RECEIVE_REGISTER_SYNCHRONISED;
				/* reset bit count received */
				receive->bit_count_received = 0;
			}
		}
	}
	else
	if (receive->flags & RECEIVE_REGISTER_SYNCHRONISED)
	{
		/* received all bits? */
		if (receive->bit_count_received==receive->bit_count)
		{
			receive->bit_count_received = 0;
			receive->flags &=~RECEIVE_REGISTER_SYNCHRONISED;
			receive->flags |= RECEIVE_REGISTER_WAITING_FOR_START_BIT;
			//logerror("receive register full\n");
			receive->flags |= RECEIVE_REGISTER_FULL;
		}
	}
}

void	receive_register_reset(struct serial_receive_register *receive_reg)
{
	receive_reg->bit_count_received = 0;
	receive_reg->flags &=~RECEIVE_REGISTER_FULL;
	receive_reg->flags &=~RECEIVE_REGISTER_SYNCHRONISED;
	receive_reg->flags |= RECEIVE_REGISTER_WAITING_FOR_START_BIT;
}

void	receive_register_extract(struct serial_receive_register *receive_reg, struct data_form *data_form)
{
	unsigned long data_shift;
	UINT8 data;

	receive_register_reset(receive_reg);

	data_shift = 0;

	/* if parity is even or odd, there should be a parity bit in the stream! */
	if (data_form->parity!=SERIAL_PARITY_NONE)
	{
		data_shift++;
	}

	data_shift+=data_form->stop_bit_count;

	/* strip off stop bits and parity */
	data = receive_reg->register_data>>data_shift;

	/* mask off other bits so data byte has 0's in unused bits */
	data = data & (0x0ff
		>>
		(8-(data_form->word_length)));

	receive_reg->byte_received  = data;

	/* parity enable? */
	switch (data_form->parity)
	{
		case SERIAL_PARITY_NONE:
			break;

		/* check parity */
		case SERIAL_PARITY_ODD:
		case SERIAL_PARITY_EVEN:
		{
			//unsigned char computed_parity;
			//unsigned char parity_received;

			/* get state of parity bit received */
			//parity_received = (receive_reg->register_data>>data_form->stop_bit_count) & 0x01;

			/* compute parity for received bits */
			//computed_parity = serial_helper_get_parity(data);

			if (data_form->parity == SERIAL_PARITY_ODD)
			{
				/* odd parity */


			}
			else
			{
				/* even parity */


			}

		}
		break;
	}
}

/***** TRANSMIT REGISTER *****/

void	transmit_register_reset(struct serial_transmit_register *transmit_reg)
{
	transmit_reg->flags |=TRANSMIT_REGISTER_EMPTY;
}

/* used to construct data in stream format */
static void transmit_register_add_bit(struct serial_transmit_register *transmit_reg, int bit)
{
	/* combine bit */
	transmit_reg->register_data = transmit_reg->register_data<<1;
	transmit_reg->register_data &=~1;
	transmit_reg->register_data|=(bit & 0x01);
	transmit_reg->bit_count++;
}


/* generate data in stream format ready for transfer */
void transmit_register_setup(struct serial_transmit_register *transmit_reg, struct data_form *data_form,unsigned char data_byte)
{
	int i;
	unsigned char transmit_data;


	transmit_reg->bit_count_transmitted = 0;
	transmit_reg->bit_count = 0;
	transmit_reg->flags &=~TRANSMIT_REGISTER_EMPTY;

	/* start bit */
	transmit_register_add_bit(transmit_reg,0);

	/* data bits */
	transmit_data = data_byte;
	for (i=0; i<data_form->word_length; i++)
	{
		int databit;

		/* get bit from data */
		databit = (transmit_data>>(data_form->word_length-1)) & 0x01;
		/* add bit to formatted byte */
		transmit_register_add_bit(transmit_reg, databit);
		transmit_data = transmit_data<<1;
	}

	/* parity */
	if (data_form->parity!=SERIAL_PARITY_NONE)
	{
		/* odd or even parity */
		unsigned char parity;

		/* get parity */
		/* if parity = 0, data has even parity - i.e. there is an even number of one bits in the data */
		/* if parity = 1, data has odd parity - i.e. there is an odd number of one bits in the data */
		parity = serial_helper_get_parity(data_byte);

		transmit_register_add_bit(transmit_reg, parity);
	}

	/* stop bit(s) */
	for (i=0; i<data_form->stop_bit_count; i++)
	{
		transmit_register_add_bit(transmit_reg,1);
	}
}


/* get a bit from the transmit register */
static int transmit_register_get_data_bit(struct serial_transmit_register *transmit_reg)
{
	int bit;

	bit = (transmit_reg->register_data>>
		(transmit_reg->bit_count - 1 -
		transmit_reg->bit_count_transmitted)) & 0x01;

	transmit_reg->bit_count_transmitted++;

	/* have all bits of this stream formatted byte been sent? */
	if (transmit_reg->bit_count_transmitted==transmit_reg->bit_count)
	{
		/* yes - generate a new byte to send */
		transmit_reg->flags |= TRANSMIT_REGISTER_EMPTY;
	}

	return bit;
}


void	transmit_register_send_bit(running_machine *machine, struct serial_transmit_register *transmit_reg, struct serial_connection *connection)
{
	int data;

	data = transmit_register_get_data_bit(transmit_reg);

	/* set tx data bit */
	set_out_data_bit(connection->State, data);

	/* state out through connection */
	serial_connection_out(machine, connection);
}

static void serial_protocol_none_sent_char(running_device *device)
{
	int i;
	int bit;
	unsigned char data_byte;
	serial_t *ser = get_safe_token(device);

	/* generate byte to transmit */
	data_byte = 0;
	for (i=0; i<ser->data_form.word_length; i++)
	{
		data_byte = data_byte<<1;
		bit = data_stream_get_data_bit_from_data_byte(&ser->transmit);
		data_byte = data_byte|bit;
	}
	/* setup register */
	transmit_register_setup(&ser->transmit_reg,&ser->data_form, data_byte);

	logerror("serial device transmitted char: %02x\n",data_byte);
}

static TIMER_CALLBACK(serial_device_baud_rate_callback)
{
	serial_t *ser = get_safe_token((running_device*)ptr);

	/* receive data into receive register */
	receive_register_update_bit(&ser->receive_reg, get_in_data_bit(ser->connection.input_state));

	if (ser->receive_reg.flags & RECEIVE_REGISTER_FULL)
	{
		//logerror("SERIAL DEVICE\n");
		receive_register_extract(&ser->receive_reg, &ser->data_form);

		logerror("serial device receive char: %02x\n",ser->receive_reg.byte_received);
	}

	/* is transmit empty? */
	if (ser->transmit_reg.flags & TRANSMIT_REGISTER_EMPTY)
	{
		/* char has been sent, execute callback */
		serial_protocol_none_sent_char((running_device*)ptr);
	}

	/* other side says it is clear to send? */
	if (ser->connection.input_state & SERIAL_STATE_CTS)
	{
		/* send bit */
		transmit_register_send_bit(machine, &ser->transmit_reg, &ser->connection);
	}
}

/* connect the specified connection to this serial device */
void serial_device_connect(running_device *device, struct serial_connection *connection)
{
	serial_t *ser = get_safe_token(device);
	serial_connection_link(device->machine, connection, &ser->connection);
}


/* load image */
static int serial_device_load_internal(running_device *image, unsigned char **ptr, int *pDataSize)
{
	int datasize;
	unsigned char *data;

	/* get file size */
	datasize = image_length(image);

	if (datasize!=0)
	{
		/* malloc memory for this data */
		data = (unsigned char *)malloc(datasize);

		if (data!=NULL)
		{
			/* read whole file */
			image_fread(image, data, datasize);

			*ptr = data;
			*pDataSize = datasize;

			logerror("File loaded!\r\n");

			/* ok! */
			return 1;
		}
	}
	return 0;
}

/* reset position in stream */
static void data_stream_reset(struct data_stream *stream)
{
	/* reset byte offset */
	stream->ByteCount= 0;
	/* reset bit count */
	stream->BitCount = 0;
}

/* free stream */
static void data_stream_free(struct data_stream *stream)
{
	if (stream->pData!=NULL)
	{
		free(stream->pData);
		stream->pData = NULL;
	}
	stream->DataLength = 0;
}

/* initialise stream */
static void data_stream_init(struct data_stream *stream, unsigned char *pData, unsigned long DataLength)
{
	stream->pData = pData;
	stream->DataLength = DataLength;
	data_stream_reset(stream);
}

DEVICE_START(serial)
{
	//serial_t *ser = get_safe_token(device);
}

DEVICE_RESET(serial)
{
//  serial_t *ser = get_safe_token(device);
}

DEVICE_IMAGE_LOAD(serial)
{
	int data_length;
	unsigned char *data;

	serial_t *ser = get_safe_token(image);

	/* load file and setup transmit data */
	if (serial_device_load_internal(image, &data, &data_length))
	{
		data_stream_init(&ser->transmit, data, data_length);
		return INIT_PASS;
	}

	return INIT_FAIL;
}


DEVICE_IMAGE_UNLOAD(serial)
{
	serial_t *ser = get_safe_token(image);

	/* stop transmit */
	serial_device_set_transmit_state(image, 0);
	/* free streams */
	data_stream_free(&ser->transmit);
	data_stream_free(&ser->receive);
}


DEVICE_GET_INFO( serial )
{
	switch ( state )
	{
		case DEVINFO_INT_CLASS:	                    info->i = DEVICE_CLASS_PERIPHERAL;          			break;
		case DEVINFO_INT_TOKEN_BYTES:				info->i = sizeof(serial_t);								break;
		case DEVINFO_INT_INLINE_CONFIG_BYTES:		info->i = 0;											break;
		case DEVINFO_INT_IMAGE_TYPE:	            info->i = IO_SERIAL;                                	break;
		case DEVINFO_INT_IMAGE_READABLE:            info->i = 1;                                        	break;
		case DEVINFO_INT_IMAGE_WRITEABLE:			info->i = 1;                                        	break;
		case DEVINFO_INT_IMAGE_CREATABLE:	    	info->i = 1;                                        	break;

		case DEVINFO_FCT_START:		                info->start = DEVICE_START_NAME( serial );          	break;
		case DEVINFO_FCT_RESET:						info->reset = DEVICE_RESET_NAME( serial );				break;
		case DEVINFO_FCT_IMAGE_LOAD:		        info->f = (genf *) DEVICE_IMAGE_LOAD_NAME( serial );	break;
		case DEVINFO_FCT_IMAGE_UNLOAD:		        info->f = (genf *) DEVICE_IMAGE_UNLOAD_NAME(serial );	break;
		case DEVINFO_STR_NAME:		                strcpy( info->s, "Serial port");	                    break;
		case DEVINFO_STR_FAMILY:                    strcpy(info->s, "Serial port");	                        break;
		case DEVINFO_STR_IMAGE_FILE_EXTENSIONS:	    strcpy(info->s, "");                                    break;
		case DEVINFO_STR_VERSION:					strcpy(info->s, "1.0");									break;
		case DEVINFO_STR_SOURCE_FILE:				strcpy(info->s, __FILE__);								break;
		case DEVINFO_STR_CREDITS:					strcpy(info->s, "Copyright the MESS Team"); 			break;
	}
}

/*******************************************************************************/
/*******************************************************************************/
/********* SERIAL CONNECTION ***********/


/* this converts state at this end to a state the other end can accept */
/* e.g. CTS at this end becomes RTS at other end.
        RTS at this end becomes CTS at other end.
        TX at this end becomes RX at other end.
        RX at this end becomes TX at other end.
        etc

        The same thing is done inside the serial null-terminal lead */

static unsigned long serial_connection_spin_bits(unsigned long input_status)
{

	return
		/* cts -> rts */
		(((input_status & 0x01)<<1) |
		/* rts -> cts */
		((input_status>>1) & 0x01) |
		/* dsr -> dtr */
		(((input_status>>2) & 0x01)<<3) |
		/* dtr -> dsr */
		(((input_status>>3) & 0x01)<<2) |
		/* rx -> tx */
		(((input_status>>4) & 0x01)<<5) |
		/* tx -> rx */
		(((input_status>>5) & 0x01)<<4));
}


/* setup callbacks for connection */
void	serial_connection_init(running_machine *machine, struct serial_connection *connection)
{
	connection->out_callback = NULL;
	connection->in_callback = NULL;
}

/* set callback which will be executed when in status has changed */
void	serial_connection_set_in_callback(running_machine *machine, struct serial_connection *connection, void (*in_cb)(running_machine *machine, int id, unsigned long status))
{
	connection->in_callback = in_cb;
}

/* output new state through connection */
void	serial_connection_out(running_machine *machine, struct serial_connection *connection)
{
	if (connection->out_callback!=NULL)
	{
		unsigned long state_at_other_end;

		state_at_other_end = serial_connection_spin_bits(connection->State);

		connection->out_callback(machine, connection->id, state_at_other_end);
	}
}

/* join two serial connections together */
void	serial_connection_link(running_machine *machine, struct serial_connection *connection_a, struct serial_connection *connection_b)
{
	/* both connections should have their in connection setup! */
	/* the in connection is the callback they use to update their state based
    on the output from the other side */
	connection_a->out_callback = connection_b->in_callback;
	connection_b->out_callback = connection_a->in_callback;

	/* let b know the state of a */
	serial_connection_out(machine,connection_a);
	/* let a know the state of b */
	serial_connection_out(machine,connection_b);
}
