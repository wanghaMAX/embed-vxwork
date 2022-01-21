#include <M_led.h>

#define LED_ENABLE 0

static int led_data;

void Led_Init(void)
{
#if LED_ENABLE
	rGPBCON	= (rGPBCON | (0xf<<5));
	rGPBUP 	= (rGPBUP & ~(0xf<<5));
	rGPBDAT 	= (rGPBDAT | (0xf<<5));
#else
	led_data = 0;
#endif
}

void Led_Display(int data)
{
#if LED_ENABLE
	rGPBDAT = (rGPBDAT & ~(0xf<<5)) | ((~data & 0xf)<<5);
#else
	printf("led status: %d%d%d%d\n",
		(data>>0)&0x01, (data>>1)&0x01, (data>>2)&0x01, (data>>3)&0x01);
	led_data = data;
#endif
	data = data; /* prevent warning from complier */
}

int Led_Get()
{
#if LED_ENABLE
	return (rGPBDAT & ~(0xf<<5));
#else
	return led_data;
#endif
}