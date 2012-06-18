#include "say-hi.h"
#include "contiki.h"
#include "shell.h"

#include "dev/leds.h"
#include "dev/serial-line.h"
#include <stdio.h>
#include "node-id.h"

//#include "sky-transmission.h"
#include "net/rime.h"
#include "net/rime/mesh.h"

static uint8_t current_node = 0;
static struct mesh_conn mesh;
 
static void
sent(struct mesh_conn *c)
{
	printf("packet sent\n");
}

static void
timedout(struct mesh_conn *c)
{
	printf("packet timedout\n");
}

static void
recv(struct mesh_conn *c, const rimeaddr_t *from, uint8_t hops)
{
	printf("Mesh data received from %d.%d: %s (%d)\n",
           from->u8[0], from->u8[1],
           (char *)packetbuf_dataptr(), packetbuf_datalen());
	current_node = from->u8[0];
}

const static struct mesh_callbacks callbacks = {recv, sent, timedout};

/*---------------------------------------------------------------------------*/
PROCESS(shell_say_hi_process, "say-hi");
SHELL_COMMAND(say_hi_command,
              "say-hi",
              "say-hi: blinks and says hello",
              &shell_say_hi_process);
PROCESS(shell_round_robin_blink_process, "rr-blink");
SHELL_COMMAND(round_robin_blink_command,
              "rr-blink",
			  "rr-blink: blinks motes 9-15",
			  &shell_round_robin_blink_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_say_hi_process, ev, data)
{
	static struct etimer etimer;
	
	PROCESS_BEGIN();
/*
	leds_off(LEDS_ALL);
	etimer_set(&etimer, 3 * CLOCK_SECOND);
	PROCESS_WAIT_EVENT();
	leds_off(LEDS_ALL);

	char message[6]= "Hello";
	transmit_mesh(message,1,0);
*/
	PROCESS_END();
}

PROCESS_THREAD(shell_round_robin_blink_process, ev, data)
{
	PROCESS_BEGIN();

	uint8_t my_node = rimeaddr_node_addr.u8[0];
	static uint8_t done = 0;
	rimeaddr_t addr;
	packetbuf_copyfrom("Hello", strlen("Hello"));
	addr.u8[0] = my_node + 1;
	addr.u8[1] = 0;

	if (my_node == 9)
	{
		leds_on(LEDS_ALL);
		mesh_send(&mesh, &addr);
		done = 1;
	}
	while (1)
	{
		if (done == 1) break;

		PROCESS_WAIT_EVENT();
		if (current_node == my_node)
		{
			leds_on(LEDS_ALL);
			mesh_send(&mesh, &addr);
			done = 1;		
		}
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void shell_hello_world_init(void)
{
	mesh_open(&mesh, 132, &callbacks);
	shell_register_command(&say_hi_command);
	shell_register_command(&round_robin_blink_command);
}
/*---------------------------------------------------------------------------*/

