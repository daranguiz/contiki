#include "rr-trans.h"
#include "contiki.h"
#include "shell.h"

#include "dev/leds.h"
#include "dev/serial-line.h"
#include <stdio.h>
#include <stdlib.h>
#include "node-id.h"
#include "global-sensor.h"

//#include "sky-transmission.h"
#include "net/rime.h"
#include "net/rime/unicast.h"
#include <string.h>

#define FIRST_NODE 9
#define LAST_NODE 15
#define FREQUENCY 25
#define RELIABLE 0
#define SINK 4

/*---------------------------------------------------------------------------*/
PROCESS(shell_conn_fix_process, "conn-fix");
SHELL_COMMAND(conn_fix_command,
              "conn-fix",
              "conn-fix: reinitializes connection",
              &shell_conn_fix_process);

PROCESS(round_robin_blink_process, "rr-blink");

PROCESS(shell_round_robin_start_process, "rr-start");
SHELL_COMMAND(round_robin_start_command,
              "rr-start",
			  "rr-start: blinks connected motes",
			  &shell_round_robin_start_process);

PROCESS(shell_local_read_test_process, "local-test");
SHELL_COMMAND(local_read_test_command,
              "local-test",
			  "local-test: tests light readings on serial port",
			  &shell_local_read_test_process);
/*---------------------------------------------------------------------------*/
int transmit_unicast(char *message, uint8_t addr_one);
uint16_t datatoint(const char *str);
static char *received_string;

static void
recv_uc(struct unicast_conn *c, const rimeaddr_t *from)
{
	printf("Unicast data received from %d.%d: %s\n",
			from->u8[0], from->u8[1], (char *)packetbuf_dataptr());

   	received_string = (char *)packetbuf_dataptr();	
//	transmit_unicast("Next is packetbuf", 4);
//	transmit_unicast(received_string, 4);

#if RELIABLE == 1
	/* If receiving "sent" from a prior node, respond back that it was received
	 * and start the next process.
	 * If receiving "received" from a prior node, end the failsafe process
	 */
	if (strcmp("Sent", (char *)packetbuf_dataptr()) == 1)
	{
		transmit_unicast("Received", from->u8[0]);
		process_start(&round_robin_blink_process, NULL);
	} else process_exit(&round_robin_blink_process);
#else 
//	if (from->u8[0] == LAST_NODE)
//		return;
//	else
		process_start(&round_robin_blink_process, NULL);
#endif

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
PROCESS_THREAD(shell_conn_fix_process, ev, data)
{
	PROCESS_BEGIN();

	open_unicast();

	PROCESS_END();
}

PROCESS_THREAD(shell_round_robin_start_process, ev, data)
{
	PROCESS_BEGIN();

	if (rimeaddr_node_addr.u8[0] == FIRST_NODE)
	{ 
		static struct etimer etimer0;
		uint8_t next_node = FIRST_NODE + 1;
		uint16_t sensor_value;
		static char message[5];

		sensor_init();
		sensor_value = sensor_read();
		sensor_uinit();
		itoa(sensor_value, message, 10);

		etimer_set(&etimer0, CLOCK_SECOND/FREQUENCY);
		leds_on(LEDS_ALL);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer0));
		transmit_unicast(message, next_node);
		leds_off(LEDS_ALL);
	}
	
	PROCESS_END();
}


PROCESS_THREAD(round_robin_blink_process, ev, data)
{
	PROCESS_BEGIN();

	static struct etimer etimer;
	static uint8_t my_node;
	static char message[5];
	my_node = rimeaddr_node_addr.u8[0];
	static uint8_t next_node;
	next_node = my_node + 1;
	if (my_node == LAST_NODE)
		next_node = FIRST_NODE;

	static uint16_t new_data;
	static uint16_t received_data;
	received_data = atoi(received_string);
	//itoa(received_data, message, 10);
	//transmit_unicast(message, 4);
	sensor_init();
	new_data = sensor_read();
	sensor_uinit();
	
	received_data = received_data * (my_node - FIRST_NODE);
	new_data = new_data + received_data;
	new_data = new_data / (my_node - FIRST_NODE + 1);
	itoa(new_data, message, 10);

	etimer_set(&etimer, CLOCK_SECOND/FREQUENCY);
	leds_on(LEDS_ALL);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
	transmit_unicast(message, next_node);
	if (my_node == LAST_NODE)
		transmit_unicast(message, SINK);
	leds_off(LEDS_ALL);

#if RELIABLE == 1
	while (1)
	{
		etimer_set(&etimer, CLOCK_SECOND);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
		transmit_unicast("Sent", next_node);
	}
#endif

	PROCESS_END();
}

PROCESS_THREAD(shell_local_read_test_process, ev, data)
{
	PROCESS_BEGIN();

	/*
	sensor_init();
	static uint16_t sensor_value;
	sensor_value = sensor_read();
	sensor_uinit();
	*/

	char *message;
	message = "123\0";
	uint16_t i = 12;
	i = strtol(message, NULL, 10);
	printf("The number is: %d\n", i);


	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void shell_rr_trans_init(void)
{
//	runicast_open(&runicast, 144, &runicast_callbacks);
	open_unicast();
	shell_register_command(&conn_fix_command);
	shell_register_command(&round_robin_start_command);
	shell_register_command(&local_read_test_command);
}

/*
uint16_t datatoint(const char *str) 
{
    const char *strptr = str;

    if(str == NULL) {
        return 0;
    }

    while(*strptr == ' ') {
        ++strptr;
    }
    
	uint16_t value = atoi(strptr);
	return value;
}
*/
/*---------------------------------------------------------------------------*/

