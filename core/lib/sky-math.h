#ifndef __SKY_MATH_H__
#define __SKY_MATH_H__

// Function declarations
unsigned short abs_sub(unsigned short var1, unsigned short var2);
unsigned short mypow2(signed short base);
unsigned short log_exp_term(unsigned short var1, unsigned short var2);
int mylog(unsigned short var);
signed short qlog(unsigned short x);
unsigned short mysqrt(unsigned short var);
signed short qsin(signed short theta);
signed short qcos(signed short theta);
void cordic_sincos(int theta, 
                   char iterations, 
                   int *sin_result,
                   int *cos_result)

#endif /* __SKY_MATH_H__ */
