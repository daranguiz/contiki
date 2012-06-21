#include "rr-trans.h"
#include "contiki.h"
#include "shell.h"

#include "dev/leds.h"
#include "dev/serial-line.h"
#include <stdio.h>
#include <stdlib.h>
#include "node-id.h"
#include "lib/list.h"
#include "lib/memb.h"
#include "sky-transmission.h"

#include "global-sensor.h"
#include "net/rime.h"
#include <string.h>

#define FIRST_NODE 9
#define LAST_NODE 15
#define FREQUENCY 15
#define RELIABLE 0 
#define SINK 4
#define TIME_OUT 3
#define MAX_RETRANSMISSIONS 4
#define NUM_HISTORY_ENTRIES 4

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
			  "rr-start: starts the round-robin collection",
			  &shell_round_robin_start_process);

PROCESS(shell_local_read_test_process, "local-test");
SHELL_COMMAND(local_read_test_command,
              "local-test",
			  "local-test: tests light readings on serial port",
			  &shell_local_read_test_process);

PROCESS(shell_round_robin_end_process, "rr-end");
SHELL_COMMAND(round_robin_end_command,
              "rr-end",
			  "rr-end: ends the round-robin collection",
			  &shell_round_robin_end_process);
/*---------------------------------------------------------------------------*/
static char *received_string;

struct history_entry
{
	struct history_entry *next;
	rimeaddr_t addr;
	uint8_t seq;
};
LIST(history_table);
MEMB(history_mem, struct history_entry, NUM_HISTORY_ENTRIES);

static void
recv_runicast(struct runicast_conn *c, const rimeaddr_t *from, uint8_t seqno)
{
	struct history_entry *e = NULL;
	for (e = list_head(history_table); e != NULL; e = e->next)
	{
		if (rimeaddr_cmp(&e->addr, from))
		{
			break;
		}
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

	received_string = (char *)packetbuf_dataptr();
	if (rimeaddr_node_addr.u8[0] != SINK)
		process_start(&round_robin_blink_process, NULL);
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
static const struct runicast_callbacks runicast_callbacks = {recv_runicast,
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
	runicast_send(&runicast, &recv, MAX_RETRANSMISSIONS);
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
PROCESS_THREAD(shell_conn_fix_process, ev, data)
{
	PROCESS_BEGIN();

	open_runicast();
	open_unicast();
//	list_init(history_table);
//	memb_init(&history_mem);

	PROCESS_END();
}

PROCESS_THREAD(shell_round_robin_start_process, ev, data)
{
	PROCESS_BEGIN();
	
	open_runicast();
	open_unicast();

	if (rimeaddr_node_addr.u8[0] == FIRST_NODE)
	{ 
		static struct etimer etimer0;
		uint8_t next_node = FIRST_NODE + 1;
		uint16_t sensor_value;
		static char message[5];

		etimer_set(&etimer0, CLOCK_SECOND/8);
		sensor_init();
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer0));
		sensor_value = sensor_read();
		sensor_uinit();
		itoa(sensor_value, message, 10);

    	etimer_set(&etimer0, CLOCK_SECOND/FREQUENCY);
		leds_on(LEDS_ALL);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer0));
		transmit_runicast(message, next_node);
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
	etimer_set(&etimer, CLOCK_SECOND/8);
	sensor_init();
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
	new_data = sensor_read();
	sensor_uinit();
		
	received_data = received_data * (my_node - FIRST_NODE);
	new_data = new_data + received_data;
	new_data = new_data / (my_node - FIRST_NODE + 1);
	itoa(new_data, message, 10);

	etimer_set(&etimer, CLOCK_SECOND/FREQUENCY);
	leds_on(LEDS_ALL);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
	if (my_node == LAST_NODE)
	{
		transmit_unicast(message, SINK);
		transmit_runicast("0", next_node);
	} else
		transmit_runicast(message, next_node);
	leds_off(LEDS_ALL);

#if RELIABLE == 1
	etimer_set(&etimer, CLOCK_SECOND/TIME_OUT);
			
	while (!strcmp(received_string, "Received"))
	{
		if (etimer_expired(&etimer) && !strcmp(received_string, "Received"))
		{
			transmit_unicast("Resending data", SINK);
			transmit_unicast(message, next_node);
		}
	}
#endif

	PROCESS_END();
}

PROCESS_THREAD(shell_round_robin_end_process, ev, data)
{
	close_unicast();
	close_runicast();
	process_exit(&round_robin_blink_process);
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
	
	/*
	char *message;
	message = "123\0";
	uint16_t i = 12;
	i = strtol(message, NULL, 10);
	printf("The number is: %d\n", i);
	*/

	static char *test_string;
   	*test_string = "Received\0";
	if (strcmp(test_string, "Received"))
		printf("Equal\n");
	else printf("Not equal\n");
	
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void shell_rr_trans_init(void)
{
	open_runicast();
	open_unicast();
	shell_register_command(&conn_fix_command);
	shell_register_command(&round_robin_start_command);
	shell_register_command(&local_read_test_command);
	shell_register_command(&round_robin_end_command);
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

