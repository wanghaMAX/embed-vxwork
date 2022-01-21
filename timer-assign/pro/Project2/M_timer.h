#ifndef M_TIMER
#define M_TIMER

#include <stdio.h>

/* timer type */
#define M_TIMER_ONE_SHOT  1
#define M_TIMER_PERIODIC  0

/* timer state */
#define M_TIMER_UNUSED    0
#define M_TIMER_RUNNING   1
#define M_TIMER_COMPLETE  2

typedef void (*timer_cb)(void *, void *arg);

struct M_timer {
	UINT8  type;   /* type of timer: ONE_SHOT OR PERIODIC */
	timer_cb cb;   /* callback function */
	void * arg;    /* arguments of callback function */
	struct M_timer * next; /* next timer */
	UINT8  state;  /* state of timer */
	UINT   period; /* number of periods when type if PERIODIC */
	UINT   delay;  /* ticks in each periods */
	UINT   expire; /* expire time in timer, after expire timer trigger */
};
typedef struct M_timer M_timer;

void task_timer_wheel();

void M_isr(int);
void M_timer_init();
M_timer * M_timer_create(UINT8, timer_cb, void *, UINT, UINT);
void M_timer_delete(M_timer *);
void M_timer_stop(M_timer *);
void M_timer_start(M_timer *);
void M_timer_query_cb(void *, void *);

#endif
