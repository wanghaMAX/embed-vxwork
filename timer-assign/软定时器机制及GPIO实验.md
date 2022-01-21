# 软定时器机制及GPIO实验

## 实验目的

在了解实时嵌入式操作系统对定时器需求的基础上，练习并掌握为实时应用提供不同类型软定时器的有效方法和管理机制，并构件以消息驱动的有限状态机开发框架打下基础。掌握GPIO的工作原理和控制方式。 

## 实验环境

+ VxSim仿真

+ TQ2440(ARM920T)
+ VxWorks5.5
+ Tornado2.2

## 系统结构

大致的运行流程说明如下

1. 若软定时器机制启用，则tick触发M_ISR程序
2. 遍历所有的Timer，并将他们的Expire时间减一，判断是否为0
3. 如果不为0，则没有到期，访问下一个timer
4. 如果为0，则timer到期，判断timer的类型
5. 若为单次定时器，则触发回调函数，将定时器状态改为COMPLETE，访问下一个timer
6. 若为周期性定时器，则周期数减一，判断周期数是否为0
7. 如果周期为0，则触发回调函数，并将定时器状态改为COMPLETE，访问下一个timer
8. 如果周期数不为0，则触发回调函数，访问下一个timer

系统的整体结构如下

![](/home/wangha/图片/bupt/embed/timer/整体框架.png)

## 运行说明

程序入口函数为Project2\usrAppInit.c文件中的usrAppInit(void)

+ 跑马灯实现
  1. 去除函数task_marquee_start()的注释
  2. rebuild all
  3. start
+ 多LED独立周期闪烁
  1. 去除函数task_ddled_start()的注释
  2. rebuild all
  3. start

或者可以使用Shell，当程序运行后，调用函数task_marquee_start()即可实现跑马灯效果，调用函数task_ddled_start()即可实现多LED独立周期闪烁

>  详细参见README.tct

## API设计

共设计了四个接口函数

* ```M_timer * M_timer_create(UINT8, timer_cb, void *, UINT, UINT)```

  用于创建一个软定时器timer。

* ```void M_timer_delete(M_timer *)```

  用于销毁一个软定时器timer。

* ```void M_timer_start(M_timer *)```

  用于启动定时器。

* ```void M_timer_stop(M_timer *)```

  用于暂停定时器。


除此之外，还设计了用于监控定时器和用于初始化定时器机制的函数接口

+ ```void M_timer_query_cb(void *, void *)```

  用于监控定时器，同时，该函数也是一个定时中断回调函数

+ ```void M_timer_init()```

  启用软定时器机制

## 定时器结构体与API函数等的具体实现

### _Timer structure_

```c
struct M_timer {
	UINT8  type;   /* type of timer: ONE_SHOT OR PERIODIC */
	timer_cb cb;   /* callback function */
	void * arg;    /* arguments of callback function */
	struct M_timer * next; /* next timer */
	UINT   state;  /* state of timer */
	UINT   period; /* number of periods when type if PERIODIC */
	UINT   delay;  /* ticks in each periods */
	UINT   expire; /* expire time in timer, after expire timer trigger */
};
```

我们的结构体设计如上，包含以下的元素。

+ _UINT8  type_ ：定时器类型，单次定时或周期性定时
+ _timer\_cb cb_：该定时器超时后的回调函数
+ _void * arg_：上面回调函数的参数
+ _struct M_timer * next_：链表形式串联的后一个timer
+ _UINT   state_：timer的状态，包含未使用、运行中、完成（与暂停状态共用）
+ _UINT   period_：时钟的周期数
+ _UINT   delay_：每个周期内超时的ticks数
+ _UINT   expire_：用于记录当前周期内距离超时的ticks数

### _ISR_

中断服务程序的功能仅仅是将refcnt自加

refcnt的作用是用于记录中断的产生，当refcnt大于0的时候，就会使得timer wheel中的timer中expire时间减一，当expire到0的时候，中断回调函数触发。

<span id="timer_wheel"> _timer wheel_ </span>中记录了所有的timer，每一次tick的中断便会遍历其中所有的定时器。

```c
void M_isr(int num)
{
	num = num; /* prevent warning due to complier */
	refcnt ++;
	tickAnnounce();
}
```

### 启用软定时器机制

+ definition

  ```c
  void M_timer_init()
  ```

+ usage

  用于启用软定时器机制

+ process

  1. 将ISR挂载到系统时钟
  2. 初始化refcnt
  3. 初始化timer_wheel

+ code

```c
void M_timer_init()
{
	/* connect M_isr to sysclk */
	sysClkConnect(M_isr, 0);
	/* reference counter init*/
	refcnt = 0;
	/* timer_wheel init */
	M_timer_wheel = NULL;
}
```

### 创建定时器

+ definition

  ```c
  M_timer * M_timer_create(UINT8 type, timer_cb cb, void * arg, UINT period, UINT expire_time)
  ```

+ symbol

  type: timer的类型

  cb: timer触发时的回调函数

  arg: 回调函数的参数

  period: 如果为周期性定时器，表示其周期数

  expire_time: 每个周期的ticks数

+ usage

  用于创建一个定时器

+ process

  1. 申请M_timer结构体timer
  2. 对timer的各个元素赋值
  3. 插入到[timer wheel](#timer_wheel)

+ code

```c
M_timer * M_timer_create(UINT8 type, timer_cb cb, void * arg,
                                     UINT period, UINT expire_time)
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
```

### 销毁定时器

+ definition

  ```c
  void M_timer_delete(M_timer * timer_target)
  ```

+ symbol

  timer_target: 需要删除的timer

+ usage

  用于销毁一个定时器

+ process

  1. 判断是否为timer wheel中的第一个定时器
  2. 若是，则timer_wheel的头指针赋值为当前timer的下一个指针，并释放当前timer的内存
  3. 若不是，则遍历timer wheel找到需要释放的timer，另前一timer的next变为当前timer的next

+ code

```c
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
```

### 启用定时器

+ definition

  ```c
  void M_timer_start(M_timer * timer)
  ```

+ symbol

  timer: 需要启用的timer

+ usage

  用于启用一个timer

+ process

  修改timer的状态(state)为_RUNNING_

+ code

```c
void M_timer_start(M_timer * timer)
{
	/* change state */
	timer->state = M_TIMER_RUNNING;
}
```

### 暂停定时器

+ definition

  ```c
  void M_timer_stop(M_timer * timer)
  ```

+ symbol

  timer: 需要暂停的timer

+ usage

  用于暂停一个timer

+ process

  修改timer的状态(state)为_COMPLETE_

+ code

```c
void M_timer_stop(M_timer * timer)
{
	/* change state */
	timer->state = M_TIMER_COMPLETE;
}
```

### 实际用于处理tick的函数

+ definition

  ```c
  void task_timer_wheel()
  ```

+ usage

  当sysclk触发一个tick后，refcnt会自加，该函数用于处理refcnt，并每一个tick都会遍历timer wheel中的所有timer，每个timer的expire都减一，若expire为0，则触发该timer的回调函数。

+ process

  过程较为复杂，大致如下

  1. tick触发M_ISR程序后，遍历timer wheel
  2. 将每一个Timer的Expire时间减一，判断是否为0
  3. 如果不为0，则没有到期，访问下一个timer
  4. 如果为0，则timer到期，判断timer的类型
  5. 若为单次定时器，则触发回调函数，将定时器状态改为COMPLETED，访问下一个timer
  6. 若为周期性定时器，则周期数减一，判断周期数是否为0
  7. 如果周期为0，则触发回调函数，并将定时器状态改为COMPLETED，访问下一个timer
  8. 如果周期数不为0，则触发回调函数，访问下一个timer

+ code

```c
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
```

### 监控timer

+ definition

  ```c
  void M_timer_query_cb(void * p, void * arg)
  ```

+ symbol

  p: 该回调函数对应的timer

  arg: 用户自定义的参数

+ usage

  用于监控timer

+ process

  1. 遍历timer wheel中所有的timer
  2. 对各种timer进行统计并打印

+ code

```c
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
```

对应的定时器创建代码如下。

```c
void task_timer_query()
{
	M_timer * timer;
	timer = M_timer_create(M_TIMER_PERIODIC, M_timer_query_cb, "0", -1, 500);
	M_timer_start(timer);
}
```

### LED初始化

+ definition

  ```c
  void Led_Init(void)
  ```

+ usage

  初始化LED灯，为方便我们调试，我们设置led_data来模拟真实LED灯的状态。

+ process

  1. 如果有LED灯，则初始化rGPBCON、rGPBUP、rGPBDAT
  2. 若无，则令led_data 为 0

+ code

```c
void Led_Init(void)
{
#if LED_ENABLE
	rGPBCON	= (rGPBCON | (0xf<<5));
	rGPBUP 	= (rGPBUP & ~(0xf<<5));
	rGPBDAT = (rGPBDAT | (0xf<<5));
#else
	led_data = 0;
#endif
}
```

### LED显示

+ definition

  ```c
  void Led_Display(int data)
  ```

+ usage

  按照data来显示led灯

+ process

  1. 如果有LED灯，则更改rGPBDAT
  2. 若无，则修改led_data的数据为data

+ code

```c
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
```

### LED状态获取

+ definition

  ```c
  int Led_Get()
  ```

+ usage

  获取GPBDAT或led_data中的数据

+ process

  1. 如果有LED灯，则返回rGPBDAT的数据
  2. 若无，则返回led_data的数据

+ code

```c
int Led_Get()
{
#if LED_ENABLE
	return (rGPBDAT & ~(0xf<<5));
#else
	return led_data;
#endif
}
```

## 测试案例

### 跑马灯

+ 模拟效果图（实际效果见附件中的视频）

  <img src="/home/wangha/图片/bupt/embed/timer/marquee.png" style="zoom: 80%;" />

+ 定时器创建

直接调用我们的API创建定时器，并启用该定时器。

我们的定时器的属性设置为

1. 周期性定时器
2. 回调函数为task_marquee_cb
3. 回调函数的参数为"marqueue\n"
4. 周期为16
5. 每个周期包含50个ticks

```c
void task_marquee_start()
{
	M_timer * timer;
	timer = M_timer_create(M_TIMER_PERIODIC, task_marquee_cb, "marqueue\n", 16, 50);
	M_timer_start(timer);
}
```

+ 定时器中的回调函数

cnt_led为我们设置的一个全局变量，用于记录是第几次回调触发

利用cnt_led即可实现跑马灯

```c
void task_marquee_cb(void * p, void * arg)
{
	p = p;
	arg = arg;
	Led_Display(1<<(cnt_led%4));
	cnt_led++;
}
```

+ windview

  由于我们的软定时器机制中设置了死循环while(1)，因此，windview图是在结束死循环后，再upload生成的。

  ![](/home/wangha/图片/bupt/embed/timer/wv_marquee.png)

### 多LED独立周期闪烁

+ 模拟效果图（实际效果见附件中的视频）

  <img src="/home/wangha/图片/bupt/embed/timer/ddled.png" style="zoom:80%;" />

+ 定时器创建

为了实现每个led灯以不同的周期闪烁，我们设置了4个定时器，分别控制每个led灯。

我们的定时器的属性设置为

1. 周期性定时器
2. 回调函数分别为task_leda_cb、task_ledb_cb、task_ledc_cb、task_ledd_cb
3. 回调函数的参数分别为"leda\n"、"ledb\n"、"ledc\n"、"ledd\n"
4. 周期为32、16、8、4
5. 周期对应包含10、20、40、80个ticks

```c
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
```

+ 定时器中的回调函数

回调函数的过程为：

获取寄存器中的值，对值进行异或，led灯展示。

四个回调函数的具体实现如下。

```c
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
```

+ windview

  ![](/home/wangha/图片/bupt/embed/timer/wv_ddled.png)

