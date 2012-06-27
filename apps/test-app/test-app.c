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

	static signed short pre_log = 32000;
	static signed short pre_sin = 361;
	static signed short pre_cos = -361;
	static signed short log_value = 0;
	static signed short sin_value = 0;
	static signed short cos_value = 0;
	sin_value = qsin(pre_sin);
	cos_value = qcos(pre_cos);
	log_value = qlog(pre_log);

	printf("Log of %d = %d\nSin of %d = %d\nCos of %d = %d\n",
			pre_log, log_value, pre_sin, sin_value, pre_cos, cos_value);
	
	PROCESS_END();
}
/*---------------------------------------------------------------------------*/
void shell_test_app_init()
{
	shell_register_command(&test_command);
}

