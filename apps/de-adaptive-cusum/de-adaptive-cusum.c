#include <stdio.h>
#include "contiki.h"
#include "sky-transmission.h"
#include "sky-math.h"
#include "shell.h"
#include "dev/leds.h"
#include "dev/serial-line.h"

/*---------------------------------------------------------------------------*/
PROCESS(shell_adaptive_cusum_process, "ad-cusum");
SHELL_COMMAND(adaptive_cusum_command,
              "ad-cusum",
              "ad-cusum: initializes an adaptive cusum test",
              &shell_adaptive_cusum_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_adaptive_cusum_process, ev data)
{
	PROCESS_BEGIN();

	// mean_0 will already be known
	uint16_t mean_0 = 25;

	// delta and epsilon are arbitrarily chosen, will affect how quickly
	// the cusum algorithm converges as well as how noisy it is
	uint16_t delta = 0.1;
	uint16_t epsilon = 1;

	// phi_min and phi_max will be the bounds on the sensor readings (0-4000 etc)
	uint16_t phi_min = 0;
	uint16_t phi_max = 100;
	uint16_t phi_hat_a = lambda - delta/2;
	uint16_t phi_hat_a = lambda + delta/2;
	uint16_t phi_hat = 0;

	// standard CUSUM variables
	uint16_t S_n = 0;
	uint16_t minS_n = 0;
	uint16_t b = 7;
	uint16_t decision = 0;
	uint16_t stop_updating_decision_time = 0;

	while (1)
	{
		// obs right now is undefined - it will be the light reading
		uint16_t D_k = -(mypow2(obs - phi_hat_b))/2 + (mypow2(obs - phi_hat_a))/2;
		
		// recursion rule
		phi_hat_a = phi_hat_a + epsilon * D_k;
		phi_hat_b = phi_hat_a + delta;

		// phi_hat
		phi_hat = phi_hat_a + 0.5 * delta;
		if (phi_hat > phi_max) 
			phi_hat = phi_max;
		else if (phi_hat < phi_min)
			phi_hat = phi_min;
		
		// now do CUSUM normally, using phi_hat
		// find new log-likelihood statistic
		int16_t z_k = -(mypow2(obs - phi_hat))/2 + (mypow2(obs - lambda))/2;
		if (-2 < z_k && z_k < 2)
			z_k = 0;
		
		// Add the log-likelihood statistic to the current sum
		S_n = S_n + z_k;
		
		// Check to see if a change was detected or not
		if (S_n >= (minS_n + b) && (stop_updating_decision_time == 0))
		{
			decision = 1;
		} else if (S_n < minS_n)
			minS_n = S_n;
		if (decision)
			stop_updating_decision_time = 1;
	}

	

	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void shell_command_list_init(void)
{
	open_mesh();
	shell_register_command(&adaptive_cusum_command);
}
