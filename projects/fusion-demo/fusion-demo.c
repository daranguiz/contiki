/* This program localizes a [gaussian] source using individual, independent sensor
 * readings, which are sent to a fusion center for calculation.
 * Authors: Dario Aranguiz and Kyle Harris
 */


#include "fusion-demo.h"
#define MU_VALUE 2000

/*---------------------------------------------------------------------------*/
PROCESS(sleepy_cusum_process, "Sleepy-CUSUM Process");
AUTOSTART_PROCESSES(&sleepy_cusum_process);

/*---------------------------------------------------------------------------*/

// Pre- and post-change distributions, assuming gaussian
static uint16_t std_dev_0 = 1;
static int16_t mean_0 = 0;
static uint16_t std_dev_1 = 1;
static int16_t mean_1 = 1;

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(sleepy_cusum_process, ev, data)
{
	PROCESS_BEGIN();

	static struct etimer etimer;
	static uint8_t counter = 0;	
	int16_t std_dev_array[50];
	mean_0 = 0;
	std_dev_0 = 0;
	SENSORS_ACTIVATE(button_sensor);


	// Wait 2 seconds after reset to collect pre-change distribution
	SENSORS_ACTIVATE(light_sensor);
	etimer_set(&etimer, CLOCK_SECOND*2);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));

	// Gather pre-change observations and calculate mean
	for (counter = 0; counter < 50; counter++)
	{
		etimer_set(&etimer, CLOCK_SECOND/50);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
		std_dev_array[counter] = light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);
		mean_0 += std_dev_array[counter];
	}
	SENSORS_DEACTIVATE(light_sensor);
	mean_0 = mean_0/50;

	// Calculate standard deviation for pre-change
	for (counter = 0; counter < 50; counter++)
		std_dev_0 = mypow2(std_dev_array[counter] - mean_0);
	std_dev_0 = mysqrt(std_dev_0 / 49);
	if (std_dev_0 == 0) 
		std_dev_0 = 1;

	// Light up LED mometarily to signify that pre-change distribution completed
	leds_on(LEDS_ALL);
	etimer_set(&etimer, CLOCK_SECOND/2);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
	leds_off(LEDS_ALL);

/*----------Post-Change--------------*/

	// Wait until user button is pressed to take post-change data
	PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
	SENSORS_DEACTIVATE(button_sensor);
	mean_1 = 0;
	std_dev_1 = 0;

	// Wait 2 seconds after button was pressed to start gathering data
	SENSORS_ACTIVATE(light_sensor);
	etimer_set(&etimer, CLOCK_SECOND*2);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));

	// Gather post-change observations and calculate mean
	for (counter = 0; counter < 50; counter++)
	{
		etimer_set(&etimer, CLOCK_SECOND/25);
		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
		std_dev_array[counter] = light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);
		mean_1 += std_dev_array[counter];
	}
	SENSORS_DEACTIVATE(light_sensor);
	mean_1 = mean_1/50;

	// Calculate standard deviation for post-change
	for (counter = 0; counter < 50; counter++)
		std_dev_1 = mypow2(std_dev_array[counter] - mean_1);
	std_dev_1 = mysqrt(std_dev_1 / 49);
	if (std_dev_1 == 0)
		std_dev_1 = 1;

	// Light up LED momentarily to signify that post-change distribution completed
	leds_on(LEDS_ALL);
	etimer_set(&etimer, CLOCK_SECOND/2);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
	leds_off(LEDS_ALL);

/*----------------Sleepy Cusum-------------------------*/
	
	// Other parameters
	static uint16_t alpha = 1; // ==alpha/1000
	static uint16_t beta = 1; // == beta/1000
 
	// Variables
	static uint16_t change_occurred = 0;
	static uint16_t sleep = 0;
	static int16_t S_n = 0;
	static int16_t z_k = 0;
	static uint16_t sensor_value = 0;
	

	static int16_t b = 20;
	//b = (qlog((1000 - beta)) - qlog(alpha))/2048;
	printf("b = %d\n", b);
	
	//Program flow:
	// If it sleeps, then it uninitializes the sensor and goes to top of loop
	// Otherwise, it keeps sensor on and keeps taking readings
	// signed short qlog(unsigned short var) returns Q11, or ret/2048=ans

	SENSORS_ACTIVATE(light_sensor);
	etimer_set(&etimer, CLOCK_SECOND/16);
	PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
	while (1)
	{
		leds_on(LEDS_RED);
		sensor_value = light_sensor.value(LIGHT_SENSOR_PHOTOSYNTHETIC);

		int post_term = mypow2(sensor_value-mean_1)/(2*mypow2(std_dev_1));	
		int pre_term = mypow2(sensor_value-mean_0)/(2*mypow2(std_dev_0));
		printf("post_term = %d\npre_term = %d\n", post_term, pre_term);	

	
		z_k = (qlog(std_dev_0) - qlog(std_dev_1))/2048;
		z_k = z_k - post_term; 
		z_k = z_k + pre_term;
		S_n = S_n + z_k;

		if (post_term == 32767)
			S_n = -32767;
		if (pre_term == 32767)
			S_n = 32767;
		printf("z_k = %d\nS_n = %d\n", z_k, S_n);

		if (S_n >= b)
		{
			change_occurred = 1;
		} 
		else if (S_n < 0)
		{
			sleep = 1;
			SENSORS_DEACTIVATE(light_sensor);
		}		

		if (!change_occurred) leds_off(LEDS_BLUE);

		if (change_occurred)
		{
			leds_on(LEDS_BLUE);
			S_n = 0;
			change_occurred = 0;
			printf("Change occurred!\n\n");
		}
		else if (sleep)
		{
			printf("Sleep time = %d\n\n", -S_n / MU_VALUE);
			etimer_set(&etimer, CLOCK_SECOND * -S_n / MU_VALUE);
			leds_off(LEDS_RED);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
			leds_on(LEDS_RED);
			S_n = 0;
			SENSORS_ACTIVATE(light_sensor);
			etimer_set(&etimer, CLOCK_SECOND/16);
			PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&etimer));
			sleep = 0;		
		}
	}

	PROCESS_END();
}
