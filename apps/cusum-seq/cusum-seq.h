/* CUSUM sequence shell command header file. */

#ifndef __CUSUM_SEQ_H__
#define __CUSUM_SEQ_H__

#include "shell.h"

void shell_sequence_init(void);
uint16_t mypow2(uint16_t base);
uint16_t log_exp_term(uint16_t var1, uint16_t var2);
int mylog(uint16_t var);
uint16_t abs_sub(uint16_t var1, uint16_t var2);
uint16_t mysqrt(uint16_t var);
void collect_data(struct shell_command *c);
void blink_LEDs(unsigned char ledv);

#endif /* __CUSUM_SEQ_H__ */
