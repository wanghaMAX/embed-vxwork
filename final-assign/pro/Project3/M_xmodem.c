#include <stdio.h>
#include <msgQLib.h>
#include "M_mem.h"
#include "M_timer.h"
#include "M_xmodem.h"

/* msg_queue for transmission */
/* s2r means sender to receiver, r2s means receiver to sender */
static MSG_Q_ID mq_s2r;
static MSG_Q_ID mq_r2s;

void M_xmodem_init()
{
	mq_r2s = msgQCreate(MSGQ_MAX_CAP, MSG_MAX_SIZE, MSG_Q_FIFO);
	mq_s2r = msgQCreate(MSGQ_MAX_CAP, MSG_MAX_SIZE, MSG_Q_FIFO);
}

/*
 * msg setting.
 * NAK. NAK
 * SOH. SOH.NUM.REMAIN.PAYLOAD.CHECKSUM
 * ACK. ACK.NUM
 * EOT. EOT
 * CAN. CAN
 */

/* encode structure -> binary */
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

/* decode binary -> structure */
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

/* send a msg for SENDER or RECEIVER */
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

/* receive a msg for SENDER or RECEIVER */
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

/* compute checksum */
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
