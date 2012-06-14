#include "say-hi.h"
#include "contiki.h"
#include "shell.h"

#include "dev/leds.h"
#include "dev/serial-line.h"
#include <stdio.h>

#include "sky-transmission.h"


/*---------------------------------------------------------------------------*/
PROCESS(shell_say_hi_process, "say-hi");
SHELL_COMMAND(say_hi_command,
              "say-hi",
              "say-hi: blinks and says hello",
              &shell_say_hi_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_say_hi_process, ev, data)
{
	static struct etimer etimer;
	
	PROCESS_BEGIN();

	leds_on(LEDS_ALL);
	etimer_set(&etimer, 3 * CLOCK_SECOND);
	PROCESS_WAIT_EVENT();
	leds_off(LEDS_ALL);

	char message[6]= "Hello";
	transmit_broadcast(message);

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void shell_command_list_init(void)
{
	open_broadcast();
	shell_register_command(&say_hi_command);
}
/*---------------------------------------------------------------------------*/

