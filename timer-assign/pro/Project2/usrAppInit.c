/* usrAppInit.c - stub application initialization routine */

/* Copyright 1984-1998 Wind River Systems, Inc. */

/*
modification history
--------------------
01a,02jun98,ms   written
*/

/*
DESCRIPTION
Initialize user application code.
*/ 

/******************************************************************************
*
* usrAppInit - initialize the users application
*/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "M_timer.h"
#include "M_led.h"

int cnt_led;

void task_timer_start();
void task_timer_query();
void task_marquee_start();
void task_marquee_cb(void *, void *);
void task_ddled_start();
void task_leda_cb(void *, void *);
void task_ledb_cb(void *, void *);
void task_ledc_cb(void *, void *);
void task_ledd_cb(void *, void *);

void usrAppInit (void)
    {
#ifdef	USER_APPL_INIT
	USER_APPL_INIT;		/* for backwards compatibility */
#endif

/* add application specific code here */
	task_timer_start();
/*	task_marquee_start();
	task_ddled_start(); */
    }

/* init timers and led */
void task_timer_start()
{
	M_timer_init();

	/* task for timer wheel */
	taskSpawn("task_timer_wheel",
		90,
		0x0100,
		2048*3,
		task_timer_wheel,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	/* task for timer query */
	task_timer_query();

	/* init led */
	Led_Init();
}

/* callback function for marquee */
void task_marquee_cb(void * p, void * arg)
{
	p = p;
	arg = arg;

	Led_Display(1<<(cnt_led%4));
	cnt_led++;
}

/* create a timer for marquee */
void task_marquee_start()
{
	M_timer * timer;
	timer = M_timer_create(M_TIMER_PERIODIC, task_marquee_cb, "marqueue\n", 16, 50);
	M_timer_start(timer);
}

/* callback function for leda */
void task_leda_cb(void * p, void * arg)
{
	int data = Led_Get();
	p = p;
	arg = arg;
	data ^= 1<<0;
	Led_Display(data);
}

/* callback function for ledb */
void task_ledb_cb(void * p, void * arg)
{
	int data = Led_Get();
	p = p;
	arg = arg;
	data ^= 1<<1;
	Led_Display(data);
}

/* callback function for ledc */
void task_ledc_cb(void * p, void * arg)
{
	int data = Led_Get();
	p = p;
	arg = arg;
	data ^= 1<<2;
	Led_Display(data);
}

/* callback function for ledd */
void task_ledd_cb(void * p, void * arg)
{
	int data = Led_Get();
	p = p;
	arg = arg;
	data ^= 1<<3;
	Led_Display(data);
}

/* create a timer for leds control, each led has self frequent */
void task_ddled_start()
{
	M_timer * timera, * timerb, * timerc, * timerd;
	timera = M_timer_create(M_TIMER_PERIODIC, task_leda_cb, "leda\n", 32, 10);
	M_timer_start(timera);
	timerb = M_timer_create(M_TIMER_PERIODIC, task_ledb_cb, "ledb\n", 16, 20);
	M_timer_start(timerb);
	timerc = M_timer_create(M_TIMER_PERIODIC, task_ledc_cb, "ledc\n", 8,  40);
	M_timer_start(timerc);
	timerd = M_timer_create(M_TIMER_PERIODIC, task_ledd_cb, "ledd\n", 4,  80);
	M_timer_start(timerd);
}

/* create a timer for query timers */
void task_timer_query()
{
	M_timer * timer;
	timer = M_timer_create(M_TIMER_PERIODIC, M_timer_query_cb, "0", -1, 500);
	M_timer_start(timer);
}


