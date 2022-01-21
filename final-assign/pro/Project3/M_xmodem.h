#ifndef M_XMODEM
#define M_XMODEM

#include <stdio.h>

/* max capacity for msg_queue */
#define MSGQ_MAX_CAP 256
/* max size for the msg in msg_queue */
#define MSG_MAX_SIZE 192

/* flag about me, sender or receiver */
#define SENDER 0x01
#define RECVER 0x02

/* Header flag */
#define SOH 0x01
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18

/* structure for xmodem protocol */
struct xmodem_struct {
	UINT8   type; /* header flag */
	UINT8   num;  /* package number */
	UINT8   remain_len; /* length of payload */
	UINT8 * payload; /* part of file we want to send */
	UINT8   checksum; /* checksum of payload */
};

typedef struct xmodem_struct xmodem_struct;

/* init xmodem */
void M_xmodem_init();

/* interface for encoding and decoding packet */
/* encode structure -> binary */
/* decode binary -> structure */
void M_encode_msg(xmodem_struct *, UINT8 * packet);
void M_decode_msg(xmodem_struct *, UINT8 * packet);

/* checksum */
UINT8 M_checksum(UINT8 *, int);
int M_xmodem_get_len(xmodem_struct *);

/* interface for sending and receive packet */
void M_recv_msg(UINT8 *, int, UINT8);
void M_send_msg(UINT8 *, int, UINT8);

#endif