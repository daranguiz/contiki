#include "say-hi.h"
#include "contiki.h"
#include "shell.h"

#include "dev/leds.h"
#include "dev/serial-line.h"
#include <stdio.h>
#include "node-id.h"

//#include "sky-transmission.h"
#include "net/rime.h"
#include "net/rime/unicast.h"
#include "net/rime/mesh.h"
#include "net/rime/runicast.h"
#include <string.h>

#define FIRST_NODE 9
#define LAST_NODE 10
#define MAX_RETRANSMISSIONS 4
#define NUM_HISTORY_ENTRIES 4

/*---------------------------------------------------------------------------*/
PROCESS(shell_say_hi_process, "say-hi");
SHELL_COMMAND(say_hi_command,
              "say-hi",
              "say-hi: blinks and says hello",
              &shell_say_hi_process);

PROCESS(shell_round_robin_blink_process, "rr-blink");

PROCESS(shell_round_robin_start_process, "rr-start");
SHELL_COMMAND(round_robin_start_command,
              "rr-start",
			  "rr-start: blinks connected motes",
			  &shell_round_robin_start_process);
/*---------------------------------------------------------------------------*/
static void
recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{
	printf("Unicast data received from %d.%d: %s\n",
			from->u8[0], from->u8[1], (char *)packetbuf_dataptr());

	process_start(&shell_round_robin_blink_process, NULL);
}

static const struct unicast_callbacks unicast_callbacks = {recv_uc};

static struct unicast_conn uc;

int transmit_unicast(char *message, uint8_t addr_one)
{
	rimeaddr_t addr;
	packetbuf_copyfrom(message, strlen(message));
	addr.u8[0] = addr_one;
	addr.u8[1] = 0;
	if (!rimeaddr_cmp(&addr, &rimeaddr_node_addr))
		return unicast_send(&uc, &addr);
	else return 0;
}

void open_unicast()
{
	unicast_open(&uc, 146, &unicast_callbacks);
}

void close_unicast()
{
	unicast_close(&uc);
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_say_hi_process, ev, data)
{
//	static struct etimer etimer;
	
	PROCESS_BEGIN();
/*
	leds_off(LEDS_ALL);
	etimer_set(&etimer, 3 * CLOCK_SECOND);
	PROCESS_WAIT_EVENT();
	leds_off(LEDS_ALL);

	char message[6]= "Hello";
	transmit_mesh(message,1,0);
*/

	open_unicast();
	PROCESS_END();
}

PROCESS_THREAD(shell_round_robin_start_process, ev, data)
{
	PROCESS_BEGIN();

	if (rimeaddr_node_addr.u8[0] == FIRST_NODE)
	{ 
		uint8_t successful = 0;
		static struct etimer etimer0;
		uint8_t next_node = FIRST_NODE + 1;
		
		etimer_set(&etimer0, CLOCK_SECOND);
		leds_on(LEDS_ALL);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer0));
		successful = transmit_unicast("Hello", next_node);
		leds_off(LEDS_ALL);
		if (!successful)
			leds_on(LEDS_ALL);
	}
	

	PROCESS_END();
}


PROCESS_THREAD(shell_round_robin_blink_process, ev, data)
{
	PROCESS_BEGIN();

	static struct etimer etimer;
	uint8_t my_node = rimeaddr_node_addr.u8[0];
	uint8_t next_node = my_node + 1;
	uint8_t successful = 0;
	if (my_node == LAST_NODE)
		next_node = FIRST_NODE;

	etimer_set(&etimer, CLOCK_SECOND);
	leds_on(LEDS_ALL);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
	successful = transmit_unicast("Hello", next_node);
	leds_off(LEDS_ALL);
	if (!successful)
		leds_on(LEDS_ALL);

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void shell_hello_world_init(void)
{
//	runicast_open(&runicast, 144, &runicast_callbacks);
//	open_unicast();
	shell_register_command(&say_hi_command);
	shell_register_command(&round_robin_start_command);
}
/*---------------------------------------------------------------------------*/

