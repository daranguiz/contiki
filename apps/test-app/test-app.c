#include "test-app.h"

/*---------------------------------------------------------------------------*/
PROCESS(shell_test_app_process, "test-app");
SHELL_COMMAND(test_command,
              "test-app",
              "test-app: runs a test app",
              &shell_test_app_process);
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(shell_test_app_process, ev, data)
{
	PROCESS_BEGIN();

	static signed short sin_value = 0;
	static signed short cos_value = 0;
	static signed short log_value = 0;
	sin_value = qsin(53);
	cos_value = qcos(53);
	log_value = qlog(53);

	printf("Sine of 53 = %d\nCosine of 53 = %d\nLog of 655 = %d\n",
			sin_value, cos_value, log_value);
	
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void shell_test_app_init()
{
	shell_register_command(&test_command);
}

