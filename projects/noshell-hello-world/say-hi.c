#include "contiki.h"
#include "net/rime.h"
#include "net/rime/mesh.h"
#include "random.h"

#include "dev/leds.h"
#include "dev/serial-line.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*---------------------------------------------------------------------------*/
/*PROCESS(shell_hello_process, "hello");
SHELL_COMMAND(hello_command,
              "hello",
              "hello: local node says hello",
              &shell_hello_process);

PROCESS(shell_leds_on_process, "leds-on");
SHELL_COMMAND(leds_on_command,
              "leds-on",
              "leds-on: turn on the LEDs on the mote",
              &shell_leds_on_process);

PROCESS(shell_broadcast_hi_process, "say-hi");
SHELL_COMMAND(say_hi_command,
              "say-hi",
              "say-hi: network nodes say hello",
              &shell_broadcast_hi_process);*/
/*---------------------------------------------------------------------------*/
static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
  printf("Broadcast message received from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}
static const struct broadcast_callbacks broadcast_call = {broadcast_recv};
static struct broadcast_conn broadcast; 



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
	
//  packetbuf_copyfrom("Hop", strlen("Hop"));
//  mesh_send(&mesh, from);
}

const static struct mesh_callbacks callbacks = {recv, sent, timedout}; 
/*---------------------------------------------------------------------------*/
PROCESS(shell_broadcast_hi_process, "say-hi");
AUTOSTART_PROCESSES(&shell_broadcast_hi_process);
PROCESS_THREAD(shell_broadcast_hi_process, ev, data)
{
	static struct etimer etimer;
	
//	PROCESS_EXITHANDLER(broadcast_close(&broadcast);)
//	PROCESS_EXITHANDLER(mesh_close(&mesh); broadcast_close(&broadcast);)
	PROCESS_BEGIN();

	static int8_t counter = 0;
	static uint8_t fail_count = 0;
	mesh_open(&mesh, 132, &callbacks);
	broadcast_open(&broadcast, 129, &broadcast_call);

	while(1) {
	counter++;

    leds_on(LEDS_ALL);
    etimer_set(&etimer, 3 * CLOCK_SECOND);
    PROCESS_WAIT_UNTIL(etimer_expired(&etimer));
    leds_off(LEDS_ALL);

	etimer_set(&etimer, random_rand() % 5 * CLOCK_SECOND + 
		random_rand() % 100 * CLOCK_SECOND / 100);
	PROCESS_WAIT_UNTIL(etimer_expired(&etimer));
	packetbuf_copyfrom("Hello", 6);
	broadcast_send(&broadcast);

	etimer_set(&etimer, random_rand() % 5 * CLOCK_SECOND + 
		random_rand() % 100 * CLOCK_SECOND / 100);
    PROCESS_WAIT_UNTIL(etimer_expired(&etimer));
	char message[6] = "Hello";
	rimeaddr_t addr;
	packetbuf_copyfrom(message, sizeof(message));
	addr.u8[0] = 62;
	addr.u8[1] = 41;
	uint8_t mesh_sent = 0;
	mesh_sent = mesh_send(&mesh, &addr);
	if (mesh_sent == 0) {
		fail_count++;
		if (counter == 10 && fail_count >= 4)
		{
			mesh_open(&mesh, 132, &callbacks);
			counter = 0;
			fail_count = 0;
		} else if (counter == 10)
			counter = 0;
	}
	}

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
/*void shell_command_list_init(void)
{
	broadcast_open(&broadcast, 129, &broadcast_call);
	mesh_open(&mesh, 132, &callbacks);
	shell_register_command(&hello_command);
    shell_register_command(&leds_on_command);
	shell_register_command(&say_hi_command);
}*/
/*---------------------------------------------------------------------------*/

