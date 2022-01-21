# 模拟改进型Xmodem协议实现及验证

## 实验目的

在充分理解嵌入式处理器特点、RTOS及强实时嵌入式系统软件设计原则的基础上,构建自己的实时系统软件框架,并在其上模拟实现改进型Xmodem协议,自拟测试所完成的相关功能。

## 实验环境

+ VxSim仿真

+ VxWorks5.5
+ Tornado2.2

## 运行说明

程序入口函数为Project3\usrAppInit.c文件中的usrAppInit(void)

关于运行说明

1. rebuild all
3. start
3. 观察VxSim查看传输过程（包含接收方接收内容与发送方发送内容的比较）

>  详细参见README.txt

## 用于存储Xmodem协议的结构体与协议设计

### _Xmodem structure_

```c
struct xmodem_struct {
	UINT8   type; /* type of this package. SOH, EOT etc */
	UINT8   num;  /* number of this package */
	UINT8   remain_len; /* length of payload */
	UINT8 * payload; /* the pointer to payload */
	UINT8   checksum; /* checksum of payload */
};
```

我们的结构体设计如上，包含以下的元素。

+ _UINT8 type_ ：package的类型，包括这些类型，SOH EOT CAN NAK ACK。
+ _UINT8 num_：package的编号，从1开始，只有在SOH ACK才会被使用。
+ _UINT8 remain_len_：payload的长度。
+ _UINT8 * payload_：指针指向package中的实际要传输的内容
+ _UINT8 checksum_：payload的校验和

### 关于协议设计

有关于协议的组成（N不大于128）

| Byte1 | Byte2 | Byte3       | Byte4-ByteN | ByteN+1 |
| ----- | ----- | ----------- | ----------- | ------- |
| 类型  | 编号  | payload长度 | payload     | 校验和  |

我们的类型共包含以下的五种，SOH EOT CAN NAK ACK。

+ SOH

  包含类型、编号、payload长度、payload、校验和的package，用于传输文件内容

+ EOT

  仅包含类型，用于结束传输

+ CAN

  仅包含类型，用于取消发送

+ NAK

  仅包含类型，用于receiver提醒sender发送文件

+ ACK

  包含类型和编号，用于回应SOH、NAK、EOT

## 协议大致流程

![](/home/wangha/图片/bupt/embed/final/xmodem_lite.png)

如上图所示，文件的传输过程主要包含以下的这些阶段

第一阶段：Receiver发送NAK提醒Sender发送文件，并设置定时器每隔一段时间发送一个NAK，直到收到SOH包。

第二阶段：Sender发送SOH消息，包含NUM、payload、checksum等的信息，同时设置定时器每隔一段时间重发一次，Receiver收到消息后，返回ACK，包含NUM，Sender收到后，与自己此前发送的NUM进行比较，若相同，则取消重发定时器，并准备发送下一条消息。

第三阶段：Sender发送完所有的内容后，发送EOT消息，并设置定时器，每隔一段时间需要进行重发，直到收到Receiver的ACK回应。Receiver则是收到EOT，就发送ACK消息。

额外的阶段：Sender或者Receiver的重发定时触发次数超过16次后，将会发送三次CAN消息，并停止自己的TASK，任意一方接受到CAN消息后，立即停止TASK。

## API设计

为了更加方便的去使用改进后的Xmodem协议，我们设计了如下的API来帮助实现。

共设计了五个接口函数

* ```void M_xmodem_init()```

  初始化xmodem协议所需要的一些全局变量。

* ```void M_encode_msg(xmodem_struct *, UINT8 *)```

  用于将我们设计的用于存储Xmodem的结构体转化为字符流。

* ```void M_decode_msg(xmodem_struct *, UINT8 *)```

  用于将字符流转化为我们设计的Xmodem结构体。

* ```void M_recv_msg(UINT8 *, int, UINT8)```

  用于接收一个消息。
  
* ```void M_send_msg(UINT8 *, int, UINT8)```

  用于发送一个消息。

除此之外，还设计了用于计算校验和与用于根据协议类型获得需要发送或者接收字符流长度的接口。

+ ```UINT8 M_checksum(UINT8 *, int)```

  用于计算字符流的校验和。

+ ```int M_xmodem_get_len(xmodem_struct *)```

  用于根据协议类型获得需要发送或者接收字符流长度。

## API函数的具体实现

### 启用Xmodem协议

+ definition

  ```c
  void M_xmodem_init()
  ```

+ usage

  用于启用Xmodem协议

+ process

  创建两个消息队列mq_r2s和mq_s2r

  mq_r2s用于receiver发生消息到sender

  mq_s2r用于sender发生消息到receiver

+ code

```c
void M_xmodem_init()
{
	mq_r2s = msgQCreate(MSGQ_MAX_CAP, MSG_MAX_SIZE, MSG_Q_FIFO);
	mq_s2r = msgQCreate(MSGQ_MAX_CAP, MSG_MAX_SIZE, MSG_Q_FIFO);
}
```

### Xmodem结构体转化为字符流

+ definition

  ```c
  void M_encode_msg(xmodem_struct * xs, UINT8 * packet)
  ```

+ symbol

  xs：用于存储Xmodem协议的结构体

  packet：被转化后的字符流

+ usage

  用于将我们设计的用于存储Xmodem的结构体转化为字符流。

+ process

  1. 获取xs结构体，并判断xmodem结构体中类型
  2. 如果为NAK，则第一字节为0x15
  3. 如果为SOH，则第一字节为0x01，第二字节为number，第三字节为payload长度，从第4到4+payload的内容为payload，最后一字节为校验和
  4. 如果为ACK，则第一字节为0x06，第二字节为number
  5. 如果为EOT，则第一字节为0x14
  6. 如果为CAN，则第一字节为0x10
  7. 否则，协议类型报错，不做任何处理

+ code

```c
void M_encode_msg(xmodem_struct * xs, UINT8 * packet)
{
	UINT8 i;
	if(packet == NULL){
		printf("ERROR. packet is null.\n");
	}
	switch(xs->type){
		case NAK:
			packet[0] = NAK;
			break;
		case SOH:
			packet[0] = SOH;
			packet[1] = xs->num;
			packet[2] = xs->remain_len;
			if(xs->remain_len > 128){
				printf("ERROR. encode error in remain_len [%d].\n", xs->remain_len);
				return;
			}
			for(i = 0; i<xs->remain_len; i++){
				packet[3+i] = xs->payload[i];
			}
			packet[xs->remain_len+3] = xs->checksum;
			break;
		case ACK:
			packet[0] = ACK;
			packet[1] = xs->num;
			break;
		case EOT:
			packet[0] = EOT;
			break;
		case CAN:
			packet[0] = CAN;
			break;
		default:
			printf("ERROR. packet type is invaild.\n");
			break;
	}
}
```

### 字符流转化为Xmodem结构体

+ definition

  ```c
  void M_decode_msg(xmodem_struct * xs, UINT8 * packet)
  ```

+ symbol

  xs：用于存储Xmodem协议的结构体

  packet：需要被转化的字符流

+ usage

  用于将字符流转化为我们设计的Xmodem结构体。

+ process

  1. 获取字符流packet，并对第一字节做判断
  2. 如果为NAK，则xs的type为0x15
  3. 如果为SOH，则xs的type为0x01，xs的num为packet中的第二字节，xs的remain_len为packet的第三字节，packet中的第4到4+remainlen字节将被copy到xs的payload中，xs的checksum为最后一字节。
  4. 如果为ACK，则xs的type为0x06，xs的number为第二字节
  5. 如果为EOT，则xs的type为0x14
  6. 如果为CAN，则xs的type为0x10
  7. 否则，协议类型为0，不做其他任何处理

+ code

```c
void M_decode_msg(xmodem_struct * xs, UINT8 * packet)
{
	UINT8 i;
	UINT8 * payload;
	if(packet == NULL || xs == NULL){
		return;
	}
	switch(packet[0]){
		case NAK:
			xs->type = NAK;
			break;
		case SOH:
			xs->type = SOH;
			xs->num = packet[1];
			xs->remain_len = packet[2];
			if(xs->remain_len > 128){
				printf("ERROR. decode error in remain_len.\n");
				return;
			}
			payload = (UINT8 *)M_mem_get(xs->remain_len);
			for(i = 0; i<xs->remain_len; i++){
				payload[i] = packet[3+i];
			}
			xs->payload = payload;
			xs->checksum = packet[xs->remain_len+3];
			break;
		case ACK:
			xs->type = ACK;
			xs->num = packet[1];
			break;
		case EOT:
			xs->type = EOT;
			break;
		case CAN:
			xs->type = CAN;
			break;
		default:
			xs->type = 0;
			break;
	}
}
```

### 发送字符流（消息）

+ definition

  ```c
  void M_send_msg(UINT8 * msg, int len, UINT8 whoiam)
  ```

+ symbol

  msg: 需要被发送的msg的头指针

  len: 需要被发送的最大长度

  whoami：指定发送者的身份

+ usage

  用于发送一个消息

+ process

  1. 判断发送者的身份
  2. 若是文件的发送者，则将消息发送到mq_s2r消息队列
  3. 若是文件的接收者，则将消息发送到mq_r2s消息队列
  4. 若都不是，则报错

+ code

```c
void M_send_msg(UINT8 * msg, int len, UINT8 whoiam)
{
	switch(whoiam){
		case SENDER:
			msgQSend(mq_s2r, msg, len, WAIT_FOREVER, MSG_PRI_NORMAL);
			break;
		case RECVER:
			msgQSend(mq_r2s, msg, len, WAIT_FOREVER, MSG_PRI_NORMAL);
			break;
		default:
			printf("ERROR. no specifc who i am.\n");
			break;
	}
}
```

### 接收字符流（消息）

+ definition

  ```c
  void M_recv_msg(UINT8 *msg, int len, UINT8 whoiam)
  ```

+ symbol

  msg: 被接收的msg的头指针

  len: 需要去接收的msg的最大长度

  whoami：指定接收者的身份

+ usage

  用于接收一个消息

+ process

  1. 判断接收者的身份
  2. 若是文件的发送者，则接收mq_r2s消息队列的消息
  3. 若是文件的接收者，则接收mq_s2r消息队列的消息
  4. 若都不是，则报错

+ code

```c
void M_recv_msg(UINT8 *msg, int len, UINT8 whoiam)
{
	switch(whoiam){
		case SENDER:
			msgQReceive(mq_r2s, msg, len, WAIT_FOREVER);
			break;
		case RECVER:
			msgQReceive(mq_s2r, msg, len, WAIT_FOREVER);
			break;
		default:
			printf("ERROR. no specifc who i am.\n");
			break;
	}
}
```

### 计算字符流的校验和

+ definition

  ```c
  UINT8 M_checksum(UINT8 * packet, int len)
  ```

+ symbol

  packet: 需要被计算的字符流的头指针

  len：从头指针之后的len个字节将会被计算

+ usage

  用于计算字符流的校验和

+ process

  从第0到len-1个字节均进行&运算

+ code

```c
UINT8 M_checksum(UINT8 * packet, int len)
{
	int i;
	UINT8 checksum = 0;
	UINT8 mask = 0xff;
	for(i=0;i<len;i++){
		checksum = mask & (checksum + packet[i]);
	}
	return checksum;
}
```

### 根据协议类型得到字符流长度

+ definition

  ```c
  int M_xmodem_get_len(xmodem_struct * xs)
  ```

+ symbol

  xs：需要被判断的类型的Xmodem结构体

+ return

  返回该类型对应的字符流长度

+ usage

  根据协议类型获得需要发送或者接收字符流长度

+ process

  1. 获取xs结构体，并判断xmodem结构体中类型
  2. 如果为NAK，则返回1
  3. 如果为SOH，则返回payload的长度加4
  4. 如果为ACK，则返回2
  5. 如果为EOT，则返回1
  6. 如果为CAN，则返回1
  7. 否则，协议类型错误，返回0

+ code

```c
int M_xmodem_get_len(xmodem_struct * xs)
{
	switch(xs->type){
		case NAK:
			return 1;
		case SOH:
			/* packet len == remain len + flag*4 */
			return xs->remain_len+4;
		case ACK:
			return 2;
		case EOT:
			return 1;
		case CAN:
			return 1;
	}
	return 0;
}
```

## Sender Task & Receiver Task

### Sender Task设计

+ 关于流程说明

  Sender Task 首先处于WAIT状态。

  当收到NAK时，处理NAK请求，并发送SOH包，同时启动SOH定时器进行重发，发送完成后判断文件是否发送完，若发送完，则停止任务。

  当收到ACK，处理ACK请求，判断文件是否发完，若未发送完，则发送下一个SOH包，重启SOH定时器，直到收到ACK时停止。若发送完，则发送EOT请求并启用EOT定时器重发，直到收到ACK时，停止任务。

  当收到CAN，则停止任务。

+ 状态流程图

  ![](/home/wangha/图片/bupt/embed/final/receiver.png)

+ code

```c
void task_sender(){
	char * str;
	xmodem_struct * xsr, * xss;
	int lenr, lens, i, num, keep_running;
	UINT8 * msgs, * msgr;
	M_timer * timer_soh, * timer_eot;
	argv_struct * argv_soh, * argv_eot;

	str = file;

	/* init */
	keep_running = 1;
	num = 1;
	lenr = MSG_MAX_SIZE;
	lens = MSG_MAX_SIZE;
	xsr = (xmodem_struct *)M_mem_get(sizeof(xmodem_struct));
	xss = (xmodem_struct *)M_mem_get(sizeof(xmodem_struct));
	msgr = M_mem_get(MSG_MAX_SIZE);
	msgs = M_mem_get(MSG_MAX_SIZE);
	xsr->type = 0x00;

	/* init timer */
	argv_soh = M_mem_get(sizeof(argv_struct));
	timer_soh = M_timer_create(M_TIMER_PERIODIC, task_sender_soh_cb, argv_soh, 3, 900);
	argv_eot = M_mem_get(sizeof(argv_struct));
	timer_eot = M_timer_create(M_TIMER_PERIODIC, task_sender_eot_cb, argv_eot, 3, 900);

	while(keep_running){
		/* receive msg */
		M_recv_msg(msgr, lenr, SENDER);

		/* decode msg */
		M_decode_msg(xsr, msgr);

		switch(xsr->type){
			case NAK:
				printf("sender: receive NAK. \n");
				/* encode SOH */
				xss->type = SOH;
				if(num > 1){
					/* TODO SOH has sent, sender maybe not be received, */
					/* maybe send too fast, so we resend SOH package */
					M_timer_start(timer_soh);
					printf("sender: timer_soh start. \n");
					break;
				}
				xss->num = num++;
				xss->remain_len = str_len-(128*(num-1)) > 0 ?
					128 : str_len-(128*(num-2));
				xss->payload = str+128*(num-2);
				xss->checksum = M_checksum(xss->payload, xss->remain_len);
				lens = M_xmodem_get_len(xss);
				M_encode_msg(xss, msgs);
				M_send_msg(msgs, lens, SENDER);
				printf("sender: send SOH. \n");
				/* setting timer for resend */
				argv_soh->arg1 = (void *)msgs;
				argv_soh->arg2 = (void *)&lens;
				break;
			case ACK:
				printf("sender: receive ACK. \n");
				if(xsr->num == 0){
					/* receive ack for eot */
					/* so we stop timer and finish*/
					M_timer_stop(timer_eot);
					printf("sender: timer_eot stop. \n");
					/* finish */
					printf("sender: finish. \n");
					keep_running = 0;
					break;
				}
				if(xsr->num != num-1){
					/* TODO resend msg with right num */
					printf("sender: number error. \n");
					break;
				}
				/* receive a right package */
				/* stop timer_soh */
				if(timer_soh->state == M_TIMER_RUNNING){
					M_timer_stop(timer_soh);
					printf("sender: timer_soh stop. \n");
				}
				/* encode EOT and send if file has sent out */
				if(128*(num-1) >= str_len){
					/* encode EOT */
					xss->type = EOT;
					lens = M_xmodem_get_len(xss);
					M_encode_msg(xss, msgs);
					M_send_msg(msgs, lens, SENDER);
					printf("sender: send EOT. \n");
					/* setting timer for resend */
					argv_eot->arg1 = (void *)msgs;
					argv_eot->arg2 = (void *)&lens;
					M_timer_start(timer_eot);
					printf("sender: timer_eot start. \n");
					break;
				}
				/* encode SOH */
				xss->type = SOH;
				xss->num = num++;
				xss->remain_len = str_len-(128*(num-1)) > 0 ?
					128 : str_len-(128*(num-2));
				xss->payload = str+128*(num-2);
				xss->checksum = M_checksum(xss->payload, xss->remain_len);
				lens = M_xmodem_get_len(xss);
				M_encode_msg(xss, msgs);
				M_send_msg(msgs, lens, SENDER);
				printf("sender: send SOH. \n");
				/* setting timer for resend */
				argv_soh->arg1 = (void *)msgs;
				argv_soh->arg2 = (void *)&lens;
				M_timer_start(timer_soh);
				printf("sender: timer_soh start. \n");
				break;
			case CAN:
				printf("sender: receive CAN. \n");
				/* finish */
				printf("sender: finish. \n");
				keep_running = 0;
				break;
			default:
				printf("sender: ERROR. receive error type package.\n");
				break;
		}
	}

	/* free */
	M_mem_put(msgs);
	M_mem_put(msgr);
	M_mem_put(xsr);
	M_mem_put(xss);
	M_mem_put(argv_soh);
	M_mem_put(argv_eot);

	/* free timer */
	M_timer_delete(timer_soh);
	M_timer_delete(timer_eot);
}
```

### Receiver Task设计

+ 关于流程说明

  Receiver Task 首先发送NAK请求，并进入WAIT状态，同时启动NAK定时器进行重发，直到收到第一条SOH。

  当收到SOH时，处理SOH请求，并发送ACK包。

  当收到EOT时，处理EOT请求，并发送ACK包，停止任务。

  当收到CAN，则停止任务。

+ 状态流程图

  ![](/home/wangha/图片/bupt/embed/final/sender.png)

+ code

```c
void task_receiver(){
	xmodem_struct * xsr, * xss;
	UINT8 * msgr, * msgs;
	int lens, lenr, num, keep_running, i;
	char * str;
	M_timer * timer_nak;
	argv_struct * argv_nak;

	/* init varaible */
	lens = MSG_MAX_SIZE;
	lenr = MSG_MAX_SIZE;
	xss = (xmodem_struct *)M_mem_get(sizeof(xmodem_struct));
	xsr = (xmodem_struct *)M_mem_get(sizeof(xmodem_struct));
	msgs = (UINT8 *)M_mem_get(lens);
	msgr = (UINT8 *)M_mem_get(lenr);
	num = 1;
	str = (char *)M_mem_get(str_len);
	keep_running = 1;

	/* start of transmission */
	/* encode */
	xss->type = NAK;
	lens = M_xmodem_get_len(xss);
	M_encode_msg(xss, msgs);

	/* send nak */
	M_send_msg(msgs, lens, RECVER);
	printf("receiver: send nak. \n");
	/* setting timer for resend */
	argv_nak = M_mem_get(sizeof(argv_struct));
	argv_nak->arg1 = (void *)msgs;
	argv_nak->arg2 = (void *)&lens;
	timer_nak = M_timer_create(M_TIMER_PERIODIC, task_receiver_nak_cb, argv_nak, 10, 500);
	M_timer_start(timer_nak);
	printf("receiver: timer_nak start. \n");

	while(keep_running){
		/* receive */
		M_recv_msg(msgr, lenr, RECVER);
		M_decode_msg(xsr, msgr);
		switch(xsr->type){
			case SOH:
				printf("receiver: receive SOH. \n");
				if(xsr->num != num ||
				   xsr->checksum != M_checksum(xsr->payload, xsr->remain_len)){
					/* no operations and wait next msg */
					printf("receiver: number error. \n");
					break;
				}
				/* receive right msg */
				if(xsr->num == 1){
					/* stop timer_nak */
					M_timer_stop(timer_nak);
					printf("receiver: timer_nak stop. \n");
					M_timer_delete(timer_nak);
				}
				for(i=0; i<xsr->remain_len; i++)
					str[128*(num-1)+i] = xsr->payload[i];
				printf("receiver: string now [%d] \n", 128*(num-1)+xsr->remain_len);
				/* encode ACK */
				xss->type = ACK;
				xss->num = num;
				lens = M_xmodem_get_len(xss);
				M_encode_msg(xss, msgs);
				/* send msg */
				M_send_msg(msgs, lens, RECVER);
				printf("receiver: send ack \n");
				num++;
				break;
			case EOT:
				printf("receiver: receive EOT. \n");
				/* encode ACK */
				xss->type = ACK;
				xss->num = 0;
				lens = M_xmodem_get_len(xss);
				M_encode_msg(xss, msgs);
				/* send msg */
				M_send_msg(msgs, lens, RECVER);
				printf("receiver: send ack \n");
				/* finish */
				printf("receiver: finish. \n");
				/* check if receive right file */
				for(i=0; i<str_len; i++){
					if(str[i] != file[i]){
						printf("-----NO_MATCHING(at %d byte)------\n", i);
						break;
					}
				}
				printf("-----MATCHING(%d bytes)------\n", str_len);

				keep_running = 0;
				break;
			case CAN:
				printf("receiver: receive CAN. \n");
				/* finish */
				printf("receiver: finish. \n");
				keep_running = 0;
				break;
			default:
				printf("receiver: ERROR. receive error type package. \n");
				break;
		}
	}
	/* free */
	M_mem_put(msgr);
	M_mem_put(msgs);
	M_mem_put(xss);
	M_mem_put(xsr);
	M_mem_put(str);
	M_mem_put(argv_nak);

	/* free timer */
	M_timer_delete(timer_nak);
}
```

### 关于定时器回调函数的说明

+ 用于Receiver的重发NAK的定时器的回调函数

  由于我设计的定时器传入参数仅为一个，因此，当需要传入多个参数时，需要使用结构体。

  当前我的结构体_argv struct_中包含msg和len，从而当设定的ticks超时后便触发当前函数，重新向mq_r2s发送消息，此处的msg为此前发送的NAK包，长度也和之前发送时相同。

  ```c
  void task_receiver_nak_cb(void * p, void * arg){
  	UINT8 * msg;
  	int len;
  	argv_struct * argv = (argv_struct *)arg;
  	printf("task_receiver_nak_cb");
  	msg = (UINT8 *)argv->arg1;
  	len = *(int  *)argv->arg2;
  	printf("msg %x len: %d\n", msg[0], len);
  	M_send_msg(msg, len, RECVER);
  }
  ```

+ 用于Sender的重发SOH的定时器的回调函数

  该回调函数用于文件的发送者重发SOH包，需要的参数包含此前发送的msg和对应的长度，msg和len均与第一次发该包时相同。

  ```c
  void task_sender_soh_cb(void * p, void * arg){
  	UINT8 * msg;
  	int len;
  	argv_struct * argv = (argv_struct *)arg;
  	printf("task_sender_soh_cb");
  	msg = (UINT8 *)argv->arg1;
  	len = *(int  *)argv->arg2;
  	M_send_msg(msg, len, SENDER);
  }
  ```

+ 用于Sender的重发EOT的定时器的回调函数

  该回调函数用于文件的发送者重发EOT包，需要的参数包含此前发送的msg和对应的长度，msg和len均与第一次发该包时相同。

  ```c
  void task_sender_eot_cb(void * p, void * arg){
  	UINT8 * msg;
  	int len;
  	argv_struct * argv = (argv_struct *)arg;
  	printf("task_sender_eot_cb");
  	msg = (UINT8 *)argv->arg1;
  	len = *(int  *)argv->arg2;
  	M_send_msg(msg, len, SENDER);
  }
  ```

## 协议的改进内容

1. byte3改为了payload的字节数
2. 等待连接阶段，Receiver每隔500ticks发一次NAK
3. Sender发出SOH数据帧的同时，设定300ticks超时定时器，若未收到ACK则重发
4. Sender发出EOT数据帧的同时，设定300ticks超时定时器，若未收到ACK则重发

## 测试案例

由于截图过长，因此我们选择展示我们的日志来进行说明。

以下是我们的日志输出。

```

                VxWorks

Copyright 1984-2002  Wind River Systems, Inc.

            CPU: VxSim for Windows
   Runtime Name: VxWorks
Runtime Version: 5.5
    BSP version: 1.2/1
        Created: Dec 26 2020, 09:32:10
  WDB Comm Type: WDB_COMM_PIPE
            WDB: Ready.

sender: receive NAK. 
sender: send SOH. 
receiver: send nak. 
receiver: timer_nak start. 
receiver: receive SOH. 
receiver: timer_nak stop. 
receiver: string now [128] 
sender: receive ACK. 
sender: send SOH. 
sender: timer_soh start. 
receiver: send ack 
receiver: receive SOH. 
receiver: string now [256] 
sender: receive ACK. 
sender: timer_soh stop. 
sender: send SOH. 
sender: timer_soh start. 
receiver: send ack 
receiver: receive SOH. 
receiver: string now [384] 
sender: receive ACK. 
sender: timer_soh stop. 
sender: send SOH. 
sender: timer_soh start. 
receiver: send ack 
receiver: receive SOH. 
receiver: string now [512] 
sender: receive ACK. 
sender: timer_soh stop. 
sender: send SOH. 
sender: timer_soh start. 
receiver: send ack 
receiver: receive SOH. 
receiver: string now [640] 
sender: receive ACK. 
sender: timer_soh stop. 
sender: send SOH. 
sender: timer_soh start. 
receiver: send ack 
receiver: receive SOH. 
receiver: string now [768] 
sender: receive ACK. 
sender: timer_soh stop. 
sender: send SOH. 
sender: timer_soh start. 
receiver: send ack 
receiver: receive SOH. 
receiver: string now [896] 
sender: receive ACK. 
sender: timer_soh stop. 
sender: send SOH. 
sender: timer_soh start. 
receiver: send ack 
receiver: receive SOH. 
receiver: string now [1024] 
sender: receive ACK. 
sender: timer_soh stop. 
sender: send SOH. 
sender: timer_soh start. 
receiver: send ack 
receiver: receive SOH. 
receiver: string now [1129] 
sender: receive ACK. 
sender: timer_soh stop. 
sender: send EOT. 
sender: timer_eot start. 
receiver: send ack 
receiver: receive EOT. 
sender: receive ACK. 
sender: timer_eot stop. 
sender: finish. 
receiver: send ack 
receiver: finish. 
-----MATCHING(1129 bytes)------
```

可以从上述看到，我们总共发送了1129Bytes，分9次发送，并且全部收到，和原始发送的均相同。

其中，timer_nak启动1次，停止1次。timer_soh启动9次，停止9次。timer_eot启动1次，停止1次。
