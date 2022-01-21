#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "M_timer.h"
#include "tickLib.h"
#include "sysLib.h"

static M_timer  * M_timer_wheel; /* soft timers wheel */
static int refcnt; /* used to trigger timer_wheel */

/* task for time wheel */
void task_timer_wheel()
{
	M_timer * timer = M_timer_wheel;
	while(1){
		if(refcnt < 1) continue;
		refcnt --;
		timer = M_timer_wheel;
		/* do after every tick */
		/* check every timer */
		while(timer){
			/* if timer is unused or completed */
			if(timer->state == M_TIMER_UNUSED ||
			   timer->state == M_TIMER_COMPLETE){
				timer = timer->next;
				continue;
			}
			/* if timer is running */
			timer->expire --;
			if(timer->expire == 0){
				switch(timer->type){
					case M_TIMER_ONE_SHOT:
						/* handle timer which just run once */
						(*timer->cb)((void *)timer, timer->arg);
						M_timer_stop((void *)timer);
						break;
					case M_TIMER_PERIODIC:
						/* handle periodic timer */
						(*timer->cb)(timer, timer->arg);
						M_timer_stop(timer);
						if(timer->period < 0){
							/* negative means unlimited period */
							M_timer_start(timer);
						}else if(timer->period > 0){
							M_timer_start(timer);
							timer->period --;
							timer->expire = timer->delay;
						}else{
							/* do nothing due to period was run out */
						}
						break;
					default:
						/* applicantion should not go here */
						printf("TIMER TYPE ERROR.");
						break;
				}
			}
			timer = timer->next;
		}
	}
}

/* ISR */
void M_isr(int num)
{
	num = num; /* prevent warning due to complier */
	refcnt ++;
	tickAnnounce();
}

/* Init timer, just init once at begin */
void M_timer_init()
{
	/* connect M_isr to sysclk */
	sysClkConnect(M_isr, 0);

	/* reference counter init*/
	refcnt = 0;

	/* timer_wheel init */
	M_timer_wheel = NULL;
}

/* Create a timer */
M_timer * M_timer_create(UINT8 type, timer_cb cb, void * arg, UINT period, UINT expire_time)
{
	M_timer * timer;

	/* timer init */
	timer = malloc(sizeof(M_timer));
	timer->type   = type;
	timer->cb     = cb;
	timer->arg    = arg;
	timer->period = period;
	timer->delay  = expire_time;
	timer->expire = expire_time;
	timer->state  = M_TIMER_UNUSED;

	/* timer wheel append */
	timer->next   = M_timer_wheel;
	M_timer_wheel = timer;
	return timer;
}

/* Delete a timer */
void M_timer_delete(M_timer * timer_target)
{
	M_timer * timer;
	M_timer * timer_prev;
	timer_prev = M_timer_wheel;
	/* timer in header */
	if(timer_prev == timer_target){
		M_timer_wheel = M_timer_wheel->next;
		free(timer_prev);
		return;
	}
	/* else */
	timer = timer_prev->next;
	while(timer != NULL){
		if(timer == timer_target){
			timer_prev->next = timer->next;
			free(timer);
			return;
		}
		timer_prev = timer;
		timer = timer->next;
	}
}

/* start a timer */
void M_timer_start(M_timer * timer)
{
	/* change state */
	timer->state = M_TIMER_RUNNING;
}

/* stop a timer */
void M_timer_stop(M_timer * timer)
{
	/* change state */
	timer->state = M_TIMER_COMPLETE;
}

/* query a timer, callback function for query task */
void M_timer_query_cb(void * p, void * arg)
{
	int t_r, t_c, t_u;
	M_timer * timer;

	/*prevent warning from complier*/
	p = p;
	arg = arg;

	t_r = 0;
	t_c = 0;
	t_u = 0;
	timer = M_timer_wheel;
	while(timer){
		switch(timer->state){
			case M_TIMER_RUNNING:
				t_r++;
				break;
			case M_TIMER_UNUSED:
				t_u++;
				break;
			case M_TIMER_COMPLETE:
				t_c++;
				break;
		}
		timer = timer->next;
	}
	printf("\n====TIMER INFO====\n");
	printf("timer unused  : %d\n", t_u);
	printf("timer running : %d\n", t_r);
	printf("timer complete: %d\n", t_c);
	printf("\n");
}
