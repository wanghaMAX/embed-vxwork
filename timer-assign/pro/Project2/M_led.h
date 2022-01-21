#ifndef M_LED
#define M_LED

#include "vxWorks.h"

#define rGPBCON    (*(volatile unsigned *)\
          0x56000010) 
#define rGPBDAT    (*(volatile unsigned *)\
          0x56000014)
#define rGPBUP     (*(volatile unsigned *)\
          0x56000018)

void Led_Init(void);
void Led_Display(int);
int  Led_Get();

#endif