#include <stdio.h>

#define NUM_TERMS 16

int log_table[NUM_TERMS] = { 11357, //ln(256)
                             8517, //ln(64)
                             5678, //ln(16)
							 2839, //ln(4)
							 1420, //ln(2)
							 830, //ln(3/2)
							 457, //ln(5/4)
							 241, //ln(9/8)
							 124, //ln(17/16)
							 63, //ln(33/32)
							 32, //ln(65/64)
							 16, //ln(129/128)
							 8, //ln(257/256)
							 4, //ln(513/512)
							 2, //ln(1025/1024)
							 1, //ln(2049/2048)
};

int k_table[5] = {256, 64, 16, 4, 2};

int main()
{
	int x = 500;
	int y = 0;
	int counter = 0;
	printf("Value for x? ");
	scanf("%d", &x);

	for (counter = 0; counter <= 12; counter++)
	{
		if (counter == 12)
			return -1;
		if (x >> counter < 1)
			break;
	}

	y = counter * log_table[4];
	x = x * 2048 / (1 << counter);

	for (counter = 0; counter < NUM_TERMS; counter++)
	{
		if (counter < 5)
		{
			while (k_table[counter] * x <= 2048)
			{
				x = x * k_table[counter];
				y = y - log_table[counter];
			}
		}
		else while ((1 + (1 << (counter - 4))) * x / (1 << (counter - 4)) <= 2048)
		{
			x = x * (1 + (1 << (counter , 4))) / (1 << (counter - 4));
			y = y - log_table[counter];
		}
	}

	printf("ln(x) in Q11 = %d\nln(x) by integer approximation = %d\n", y, y/2048);
}
