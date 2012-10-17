/* This program localizes a [gaussian] source using individual, independent sensor
 * readings, which are sent to a fusion center for calculation.
 * Authors: Dario Aranguiz and Kyle Harris
 */


#include "matlab-response-protocol.h"

#define FIRST_NODE 9 
#define LAST_NODE 15
#define SINK_NODE 1 
#define SLEEP_TIMEOUT 20
#define NUM_HISTORY_ENTRIES 4
#define MAX_RETRANSMISSIONS 4

/*---------------------------------------------------------------------------*/
PROCESS(node_read_process, "Node read process");

PROCESS(sink_handler_process, "Sink handler process");
SHELL_COMMAND(sink_handler_command,
              "sink-send",
              "sink-send: used only by matlab - 'sink-send SLEEP_TIME NODE_ID'",
              &sink_handler_process);
/*---------------------------------------------------------------------------*/
LIST(history_table);
MEMB(history_mem, struct history_entry, NUM_HISTORY_ENTRIES);
static uint16_t sleep_time = 0;
static char *node_received_string = "\0";


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
		/*	printf("Runicast message received from %d.%d, seqno %d (DUPLICATE)\n",
			       from->u8[0], from->u8[1], seqno);
		*/	return; 
		}
		e->seq = seqno;
	}
	
	uint16_t cur_time = clock_time()/CLOCK_SECOND;

	//printf("Node receives: %d\n%d\n%s\n", from->u8[0], cur_time, (char *)packetbuf_dataptr());

	/* Receiving a message triggers the next process in the sequence to begin */
	
	if (rimeaddr_node_addr.u8[0] != SINK_NODE)
	{
		node_received_string = (char *)packetbuf_dataptr();
		process_start(&node_read_process, NULL);
	} else {
		char received_string[10];
		strcpy(received_string, packetbuf_dataptr());
		uint8_t counter = 0;
		for (counter = 0; counter < 10; counter++)
			if (received_string[counter] == '!') break;
		for (counter; counter < 10; counter++)
			received_string[counter] = '\0';
		printf("DATA %d %d %s\n", from->u8[0], cur_time, received_string);
		packetbuf_clear();
	}
}

static void
sent_runicast(struct runicast_conn *c, const rimeaddr_t *to, uint8_t retransmissions)
{
/*	printf("Runicast message sent to %d.%d, retransmissions %d\n",
	       to->u8[0], to->u8[1], retransmissions); */
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
static void
broadcast_recv(struct broadcast_conn *c, const rimeaddr_t *from)
{
	node_received_string = (char *)packetbuf_dataptr();
	process_start(&node_read_process, NULL);
}

static const struct broadcast_callbacks broadcast_call = {broadcast_recv};

static struct broadcast_conn broadcast;

int transmit_broadcast(char *message)
{
	packetbuf_copyfrom(message, strlen(message));
	return broadcast_send(&broadcast);
}

void open_broadcast(void)
{
	broadcast_open(&broadcast, 129, &broadcast_call);
}

void close_broadcast(void)
{
	broadcast_close(&broadcast);
}

/*---------------------------------------------------------------------------*/
int16_t parse_sleep_value()
{
	/* String received will either be of the form "SINGLE sleep_time" or
     * "VECTOR NODE 1 2 3 SLEEP 0 2 12"
	 * From this, we want to pull the proper sleep information for the sensor
	 */
	int sleep_time = 0;

	printf("node_received_string: %s\n", node_received_string);

	if (!strcmp(strtok(node_received_string, " "), "SINGLE"))
	{
		sleep_time = atoi(strtok(NULL, " "));
	}
	else 
	{
		char *cur_string;
		cur_string = strtok(NULL, " "); // Done twice - once returns "NODE"
		cur_string = strtok(NULL, " "); // Twice returns first node_id
		int8_t string_counter = 0;
		int8_t my_id_counter = -1;
		
		while (strcmp(cur_string, "SLEEP") != 0)
		{
			if (atoi(cur_string) == rimeaddr_node_addr.u8[0]) {
				my_id_counter = string_counter;
			}
			string_counter++;
			cur_string = strtok(NULL, " ");
		}
	
		if (my_id_counter != -1)
		{
			for (my_id_counter; my_id_counter >= 0; my_id_counter--)
				cur_string = strtok(NULL, " ");
			sleep_time = atoi(cur_string);
		}
		else sleep_time = -1;
	}
	return sleep_time;
}

PROCESS_THREAD(node_read_process, ev, data)
{
	PROCESS_BEGIN();

	sleep_time = parse_sleep_value();

	if (sleep_time != -1)
	{	
		static struct etimer etimer;
		static int16_t sensor_value = 0;
		static char message[3];
		
		etimer_set(&etimer, CLOCK_SECOND * sleep_time);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
	
		etimer_set(&etimer, CLOCK_SECOND/16);
		leds_on(LEDS_ALL);
		sensor_init();
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
		sensor_value = sensor_read();
		sensor_uinit();
		itoa(sensor_value, message, 10);
		strcat(message, "!\0");
	
		transmit_runicast(message, SINK_NODE);
		leds_off(LEDS_ALL);
	}

	packetbuf_clear();

	PROCESS_END();
}

/*Sends the sleep time to a certain node
 *Syntax is either "sink-send SINGLE sleep_time node_id"
 *Ex. "sink-send SINGLE 200 12"	
 *Or
 *"sink-sind VECTOR NODE {node_id's} SLEEP {sleep_times}"
 *Ex. "sink-send VECTOR NODE 1 2 3 SLEEP 12 5 10" 
 */
PROCESS_THREAD(sink_handler_process, ev, data)
{
	PROCESS_BEGIN();

	char *input_string;
	input_string = (char *) malloc(strlen(data));
	strcpy(input_string, data);

	if (!strcmp(strtok(data, " "), "SINGLE"))
	{
		uint8_t next_node = atoi(strtok(NULL, " "));
		char message[20] = "SINGLE \0";
		char *sleep = strtok(NULL, " ");
		strcat(message, sleep);
		transmit_runicast(message, next_node);
	}
	else
	{
		transmit_broadcast(input_string);
	}

	free(input_string);
	
	packetbuf_clear();
		
	PROCESS_END();
}	

/*---------------------------------------------------------------------------*/
void shell_matlab_response_protocol_init()
{
	open_runicast();
	open_unicast();
	open_broadcast();
	shell_register_command(&sink_handler_command);
}


/*---------------------------------------------------------------------------*/
/*
 * Deprecated Processes


PROCESS(node_timeout_process, "Node timeout process");

PROCESS_THREAD(node_timeout_process, ev, data)
{
	PROCESS_BEGIN();
	
	while (1)
	{
		static struct etimer timeout;
		int16_t sensor_value = 0;
		static char message[3];

		etimer_set(&timeout, CLOCK_SECOND * SLEEP_TIMEOUT);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timeout));

		etimer_set(&timeout, CLOCK_SECOND/16);
		leds_on(LEDS_ALL);
		sensor_init();
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&timeout));
		sensor_value = sensor_read() - my_noise;
		sensor_uinit();
		itoa(sensor_value, message, 10);
		strcat(message, "!\0");
	
		transmit_runicast(message, SINK_NODE);
		leds_off(LEDS_ALL);
	}
	
	PROCESS_END();
}

PROCESS(shell_sleepy_trilat_start_process, "Sleepy-Trilat Start Process");
SHELL_COMMAND(sleepy_trilat_command,
              "strilat",
			  "strilat: begins sleepy tracking and trilateration",
			  &shell_sleepy_trilat_start_process);


PROCESS_THREAD(shell_sleepy_trilat_start_process, ev, data)
{
	PROCESS_BEGIN();

	open_runicast();
	my_node = rimeaddr_node_addr.u8[0];

	if (my_node != SINK_NODE)
	{	
		static struct etimer etimer0;
		static char message[4];
		static int i = 0;
		
		etimer_set(&etimer0, CLOCK_SECOND/16);
		sensor_init();
//		leds_on(LEDS_ALL);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer0));
		for (i = 1; i <= 100; i++)
		{
			etimer_set(&etimer0, CLOCK_SECOND/50);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer0));			
			my_noise = sensor_read() + my_noise;
		}
		sensor_uinit();
		my_noise = my_noise / 100; 
		
		etimer_set(&etimer0, CLOCK_SECOND * (my_node - FIRST_NODE + 1));
		leds_on(LEDS_ALL);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer0));
		leds_off(LEDS_ALL);	

		itoa(my_noise, message, 10);
		strcat(message, "!\0");

		transmit_runicast(message, SINK_NODE);

//		process_start(&node_timeout_process, NULL);
	}

	PROCESS_END();
}
*/
