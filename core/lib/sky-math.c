#include "lib/sky-math.h"

// Function definitions

// |var1 - var2|
unsigned short abs_sub(unsigned short var1, unsigned short var2) {

	if(var1 > var2) {
		return var1 - var2;
	} else {
		return var2 - var1;
	}
}

// A basic squaring operation.
unsigned short mypow2(signed short base) {

	unsigned short result;

	// Overflow checking...
	if(base > 255 || base < -255) {
		// Overflow will occur... flag w/ negative output
		result = -1;
	} else {
		// No overflow will occur
		result = base * base;
	}

	return result;
}

// Caculate (var1^2) / (2 * (var2^2))
unsigned short log_exp_term(unsigned short var1, unsigned short var2) {

	unsigned short result;

	if(var2 > var1) {
		result = 0;
	} else {
		result  = mypow2(var1/var2);
		result = result / 2;
	}

	return result;
}

// Calculate log(var) (base e), a very crude approximation.
int mylog(unsigned short var) {
	int Nis;
	for(Nis = 0;Nis < 16;Nis++) {
		if((var>>Nis) <= 1) {
			break;
		}
	}
	return (69*Nis)/100;
}

// Calculate the square root of var.
unsigned short mysqrt(unsigned short var) {

	unsigned short y = 1;
	unsigned short counter = 0;
	if (var < 1)
		return 1;
	else if (var > 65535)
		return -1;
	else {
		for (counter = 0; counter < 15; counter++) {
			y = (var/y + y) / 2;
		}
		return y;
	}
}
