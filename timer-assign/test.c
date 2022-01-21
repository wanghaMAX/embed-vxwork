#include "vxWorks.h"



#define rGPBCON    (*(volatile unsigned *)\
          0x56000010) 
#define rGPBDAT    (*(volatile unsigned *)\
          0x56000014)
#define rGPBUP     (*(volatile unsigned *)\
          0x56000018) 


void Led_Init(void);
void Led_Display(int);


void Led_Init(void)
{
  rGPBCON	= (rGPBCON | (0xf<<5));
  rGPBUP 	= (rGPBUP & ~(0xf<<5));
  rGPBDAT 	= (rGPBDAT | (0xf<<5));
}


void Led_Display(int data)
{
  rGPBDAT = (rGPBDAT & ~(0xf<<5)) | ((~data & 0xf)<<5);
}

void paoma()
{
	int i;
	Led_Init();
	for(i=0; i<20; i++){
		Led_Display(1<<(i%4));
		taskDelay(50);
	}
}

