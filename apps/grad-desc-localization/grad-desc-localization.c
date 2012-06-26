/* This program localizes a [gaussian] source using individual, independent sensor
 * readings, which are sent to a fusion center for calculation.
 * Authors: Dario Aranguiz and Kyle Harris
 */


#include "grad-desc-localization.h"

#define FIRST_NODE 9
#define LAST_NODE 15
#define SINK_NODE 4
#define NUM_HISTORY_ENTRIES 4
#define MAX_RETRANSMISSIONS 4

/*---------------------------------------------------------------------------*/
PROCESS(shell_grad_desc_localization_start_process, "localize");
SHELL_COMMAND(grad_desc_command,
              "start-localize",
			  "start-localize: begins grad-desc localization",
			  &shell_grad_desc_localization_start_process);

PROCESS(sine_test, "sine");
SHELL_COMMAND(sine_command,
              "sine-test",
              "sine-test: tests sin/cos(53)",
              &sine_test);

PROCESS(sink_localization_selfstart_process, "Sink helper process");

PROCESS(node_data_gather_process, "Node data-gather process");
/*---------------------------------------------------------------------------*/
static char *received_string; //[(LAST_NODE - FIRST_NODE + 1) * 4];
LIST(history_table);
MEMB(history_mem, struct history_entry, NUM_HISTORY_ENTRIES);
void cordic_sincos(int16_t theta, char iterations, int16_t *sin_result, int16_t *cos_result);

static void
recv_runicast(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
	struct history_entry *e = NULL;
	for (e = list_head(history_table); e != NULL; e = e->next)
	{
		if (rimeaddr_cmp(&e->addr, from))
			break;
	}
	if (e == NULL)
	{
		e = memb_alloc(&history_mem);
		if (e == NULL)
			e = list_chop(history_table);
		rimeaddr_copy(&e->addr, from);
		e->seq = seqno;
		list_push(history_table, e);
	}
	else
	{
		if (e->seq == seqno)
		{
			printf("Runicast message received from %d.%d, seqno %d (DUPLICATE)\n",
			       from->u8[0], from->u8[1], seqno);
			return;
		}
		e->seq = seqno;
	}

	printf("Runicast message received from %d.%d: %s\n",
	       from->u8[0], from->u8[1], (char *)packetbuf_dataptr());

	/* Receiving a message triggers the next process in the sequence to begin */
	received_string = (char *)packetbuf_dataptr();
	if (rimeaddr_node_addr.u8[0] != SINK_NODE)
		process_start(&node_data_gather_process, NULL);
	else process_start(&sink_localization_selfstart_process, NULL);
}

static void
sent_runicast(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions)
{
	printf("Runicast message sent to %d.%d, retransmissions %d\n",
	       to->u8[0], to->u8[1], retransmissions);
}

static void
timedout_runicast(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions)
{
	printf("Runicast message timed out when sending to %d.%d, retransmissions %d\n",
	       to->u8[0], to->u8[1], retransmissions);
}

static const struct runicast_callbacks runicast_callbacks = {
                         recv_runicast,
						 sent_runicast,
						 timedout_runicast};

static struct runicast_conn runicast;

int transmit_runicast(char *message, uint8_t addr_one)
{
	rimeaddr_t recv;
	packetbuf_copyfrom(message, strlen(message));
	recv.u8[0] = addr_one;
	recv.u8[1] = 0;
	packetbuf_copyfrom(message, strlen(message));
	return runicast_send(&runicast, &recv, MAX_RETRANSMISSIONS);
}

void open_runicast(void)
{
	list_init(history_table);
	memb_init(&history_mem);
	runicast_open(&runicast, 144, &runicast_callbacks);
}

void close_runicast(void)
{
	runicast_close(&runicast);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_grad_desc_localization_start_process, ev, data)
{
	PROCESS_BEGIN();

	open_runicast();

	if (rimeaddr_node_addr.u8[0] == FIRST_NODE)
	{
		static struct etimer etimer0;
		uint8_t next_node = FIRST_NODE + 1;
		uint16_t sensor_value;
		static char message[3];

		etimer_set(&etimer0, CLOCK_SECOND/16);
		sensor_init();
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer0));
		sensor_value = sensor_read();
		sensor_uinit();
		itoa(sensor_value, message, 10);

		leds_on(LEDS_ALL);
		transmit_runicast(message, next_node);
		leds_off(LEDS_ALL);
	}	

	PROCESS_END();
}

PROCESS_THREAD(node_data_gather_process, ev, data)
{
	PROCESS_BEGIN();

	static struct etimer etimer;
	static uint8_t my_node;
	static uint8_t next_node;
	static char message[4];
	my_node = rimeaddr_node_addr.u8[0];
	next_node = my_node + 1;
	if (my_node == LAST_NODE)
		next_node = SINK_NODE;

	static uint16_t sensor_value;
	etimer_set(&etimer, CLOCK_SECOND/16);
	sensor_init();
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
	sensor_value = sensor_read();
	sensor_uinit();

	itoa(sensor_value, message, 10);
	strcat(received_string, " ");
	strcat(received_string, message);

	leds_on(LEDS_ALL);
	transmit_runicast(received_string, next_node);
	leds_off(LEDS_ALL);

	PROCESS_END();
}

PROCESS_THREAD(sink_localization_selfstart_process, ev, data)
{
	PROCESS_BEGIN();

	static uint16_t readings[LAST_NODE - FIRST_NODE + 1];
	static uint8_t i = 0;
	static char *parsed_value;
	
	parsed_value = strtok(received_string, " ");
	for (i = 0; i <= (LAST_NODE - FIRST_NODE); i++)
	{
		readings[i] = atoi(parsed_value);
		parsed_value = strtok(NULL, " ");
	}

	/* Calculations */
	
	transmit_runicast("\0", FIRST_NODE);

	PROCESS_END();
}


//******************************************************************************
//
//   Description: This file contains a function to caluculate sine and cosine
//                using CORDIC rotation on a 16 bit MSP430. The function takes
//                any angle in degrees and outputs the answers in Q.15 fixed 
//                point format[(floating point) = (fixed point)/ 2^15]. 
//
//                In creating this file, I referenced the 2004 presentation of
//                fixed point two's complement CORDIC arithmetic presentation
//                by Titi Trandafir of Microtrend Systems which contained an 
//                assembly language program utilizing CORDIC. I also referenced
//                the CORDIC wikipedia page.
//
//   T. Brower
//   Version    1.00
//   Feb 2011
//     IAR Embedded Workbench Kickstart (Version: 5.20.1)
//******************************************************************************

// Table of arctan's for use with CORDIC algorithm
// Store in decimal representation N = ((2^16)*angle_deg) / 180
#define ATAN_TAB_N 16
uint16_t atantable[ATAN_TAB_N] = {  0x4000,   //atan(2^0) = 45 degrees
                                0x25C8,   //atan(2^-1) = 26.5651
                                0x13F6,   //atan(2^-2) = 14.0362
                                0x0A22,   //7.12502
                                0x0516,   //3.57633
                                0x028B,   //1.78981
                                0x0145,   //0.895174
                                0x00A2,   //0.447614
                                0x0051,   //0.223808
                                0x0029,   //0.111904
                                0x0014,   //0.05595
                                0x000A,   //0.0279765
                                0x0005,   //0.0139882
                                0x0003,   //0.0069941
                                0x0002,   //0.0035013
                                0x0001    //0.0017485
};

// Function to computer sine/cosine using CORDIC
// Inputs: 
//  theta = any (integer) angle in degrees
//  iterations =  number of iterations for CORDIC algorithm, up to 16, 
//                the ideal value seems to be 13
//  *sin_result = pointer to where you want the sine result
//  *cos_result = pointer to where you want the cosine result

void cordic_sincos(int16_t theta, 
                   char iterations, 
                   int16_t *sin_result,
                   int16_t *cos_result){
  int16_t sigma, s, x1, x2, y, i, quadAdj, shift;
  int16_t *atanptr = atantable;

  //Limit iterations to number of atan values in our table
  iterations = (iterations > ATAN_TAB_N) ? ATAN_TAB_N : iterations;

  //Shift angle to be in range -180 to 180
  while(theta < -180) theta += 180;
  while(theta > 180) theta -= 180;

  //Shift angle to be in range -90 to 90
  if (theta < -90){
    theta = theta + 180;
    quadAdj = -1;
  } else if (theta > 90){
    theta = theta - 180;
    quadAdj = -1;
  } else{
    quadAdj = 1;
  }

  //Shift angle to be in range -45 to 45
  if (theta < -45){
    theta = theta + 90;
    shift = -1;
  } else if (theta > 45){
    theta = theta - 90;
    shift = 1;
  } else{
    shift = 0;
  }

  //convert angle to decimal representation N = ((2^16)*angle_deg) / 180
  if(theta < 0){
    theta = -theta;
    theta = ((uint16_t)theta<<10)/45;   //Convert to decimal representation of angle
    theta = (uint16_t)theta<<4;
    theta = -theta;
  } else{
    theta = ((uint16_t)theta<<10)/45;   //Convert to decimal representation of angle
    theta = (uint16_t)theta<<4;
  }

  //Initial values
  x1 = 0x4DBA;    //this will be the cosine result, 
                  //initially the magic number 0.60725293
  y = 0;          //y will contain the sine result
  s = 0;          //s will contain the final angle
  sigma = 1;      //direction from target angle
  
  for (i=0; i<iterations; i++){
    sigma = (theta - s) > 0 ? 1 : -1;
    if(sigma < 0){
      x2 = x1 + (y >> i);
      y = y - (x1 >> i);
      x1 = x2;
      s -= *atanptr++;
    } else{
      x2 = x1 - (y >> i);
      y = y + (x1 >> i);
      x1 = x2;
      s += *atanptr++;
    }

  }

  //Correct for possible overflow in cosine result
  if(x1 < 0) x1 = -x1;
  
  //Push final values to appropriate registers
  if(shift > 0){
    *sin_result = x1;
    *cos_result = -y;
  } else if (shift < 0){
    *sin_result = -x1;
    *cos_result = y;
  } else {
    *sin_result = y;
    *cos_result = x1;
  }

  //Adjust for sign change if angle was in quadrant 3 or 4
  *sin_result = quadAdj * *sin_result;
  *cos_result = quadAdj * *cos_result;
}
/*---------------------------------------------------------------------------*/
void shell_grad_desc_localization_init()
{
	open_runicast();
	shell_register_command(&grad_desc_command);
}

/* General steps for mote adaptation:
 * - Get sensor reading
 * - Concatenate it with the already present data string 
 * --- *Note, looks like this "100 101 483" etc
 * - Pass on new data string to next node
 * - At end, pass final data string to sink node
 * 
 * Sink node (start process in the init function so it runs on sink?):
 * - Parse string and pass data individually into array
 * - Calculations
 * - Flag iteration as complete
 * - Send command to continue data collection to the first node
 */
