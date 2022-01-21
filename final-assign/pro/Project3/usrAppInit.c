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
#include <strlib.h>
#include "M_mem.h"
#include "M_timer.h"
#include "M_xmodem.h"

/* this structure can help passing more args for timer */
struct argv_struct{
	void * arg1; /* msg */
	void * arg2; /* length of msg */
	void * arg3;
};
typedef struct argv_struct argv_struct;

void task_sender();
void task_receiver();
static void task_receiver_nak_cb(void * p, void * arg);
static void task_sender_soh_cb(void * p, void * arg);
static void task_sender_eot_cb(void * p, void * arg);

const static char * file = "qwertyuiqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgqwertyuiopoiuytrewqwertyuiopiuywqdfghhfjgjkdshgopoiuytrewqwertyuiopiuywqdfghhfjgjkdshg";
const static int str_len = 1129;

void usrAppInit (void)
    {
#ifdef	USER_APPL_INIT
	USER_APPL_INIT;		/* for backwards compatibility */
#endif

	/* add application specific code here */
    /* enable mem & timer & xmodem */
   	M_mem_init();
   	M_timer_init();
   	M_xmodem_init();
	/* enable timer wheel for xmodem */
	taskSpawn("task_timer_wheel",
		90,
		0x0100,
		2048*3,
		task_timer_wheel,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	/* task for sender */
	taskSpawn("task_sender",
		88,
		0x0100,
		2048*3,
		task_sender,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

	/* task for receiver */
	taskSpawn("task_receiver",
		89,
		0x0100,
		2048*3,
		task_receiver,
		0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
   }

/* callback function for resending nak */
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

/* callback function for resending soh */
void task_sender_soh_cb(void * p, void * arg){
	UINT8 * msg;
	int len;
	argv_struct * argv = (argv_struct *)arg;
	printf("task_sender_soh_cb");
	msg = (UINT8 *)argv->arg1;
	len = *(int  *)argv->arg2;
	M_send_msg(msg, len, SENDER);
}

/* callback function for resending eot */
void task_sender_eot_cb(void * p, void * arg){
	UINT8 * msg;
	int len;
	argv_struct * argv = (argv_struct *)arg;
	printf("task_sender_eot_cb");
	msg = (UINT8 *)argv->arg1;
	len = *(int  *)argv->arg2;
	M_send_msg(msg, len, SENDER);
}

/* task for receiver */
void task_receiver(){
	/* xmodem structure for receive and send */
	xmodem_struct * xsr, * xss;
	/* msg for receive and send */
	UINT8 * msgr, * msgs;
	/* length to send or receive */
	int lens, lenr, keep_running, i;
	/* recording the number in package */
	int num;
	/* store payload received */
	char * str;
	/* timer and args for nak */
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

	/* free timer */
	M_timer_delete(timer_nak);
	M_mem_put(argv_nak);
}

/* task for sender */
void task_sender(){
	/* the file we wanna send */
	char * str;
	/* xmodem structure for receive and send */
	xmodem_struct * xsr, * xss;
	/* length to send or receive */
	int lenr, lens, i, keep_running;
	/* recording the number in we next send in package */
	int num;
	/* msg for receive and send */
	UINT8 * msgs, * msgr;
	/* timer and args for nak and eot */
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

	/* free timer */
	M_timer_delete(timer_soh);
	M_timer_delete(timer_eot);
	M_mem_put(argv_soh);
	M_mem_put(argv_eot);
}
