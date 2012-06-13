#ifndef __MEAN_SHIFT_H__
#define __MEAN_SHIFT_H__

#include "shell.h"

void shell_mean_shift_init(void);
char shell_datatochar(const char *str);
void sensor_init(void);
void sensor_uinit(void);
uint16_t sensor_read(void);
uint16_t mysqrt(uint16_t num);
uint16_t mystdev(uint16_t *sample_array, uint16_t mean);
void blink_LEDs(unsigned char ledv);

#endif
