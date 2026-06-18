#ifndef COMM_TYPES_H
#define COMM_TYPES_H

#include "cmsis_os.h"
#include <stdint.h>
#include <stdbool.h>

//Size message limits for message mail 
#define MAX_MAIL_PAYLOAD 32
#define LOCAL_BUF_SIZE 128 // A large temporary buffer to hold input until 'Enter' is pressed
#define CHUNK_SIZE     (MAX_MAIL_PAYLOAD - 1) // 31 characters + 1 for '\0'

typedef enum
{
	MSG_Emergency = 0,
	MSG_Standard = 1,
	MSG_Periodic1 = 2,
	MSG_Periodic2 = 3
}message_Type;

typedef enum{
	UART_1 = 0,
	UART_2 = 1,
	UART_3 = 2,
	NUM_UARTS = 3

}UART_ID;

typedef enum{
	UART_IDLE = 1,
	UART_INPUT = 2,
	UART_MENU = 3
}UART_STATUS;



typedef enum {
	Recipient_1 = UART_1,
	Recipient_2 = UART_2,
	Recipient_3 = UART_3,
	Recipient_Group
}Recipient_ID;

typedef enum {
  Sender_1 = UART_1,
	Sender_2 = UART_2,
	Sender_3 = UART_3,
	Sender_Auto
}Sender_ID;
	

typedef struct {
    char payload[MAX_MAIL_PAYLOAD];
    uint8_t is_fragmented; // 0 = Normal, 1 = Part of a split message
    Recipient_ID Receiver; // 1 or 2, so the Tx thread knows where it came from
	  Sender_ID Sender;    // 1 2 or 3 for intended receiver
	  uint16_t msgCnt; //mesage count for periodic messages
} Mail;

typedef struct {
	UART_STATUS UART_State;
	Recipient_ID Recipient;
	bool Mute_Sender[NUM_UARTS];
	bool Emergency_Mode;
	char local_buffer[LOCAL_BUF_SIZE];
	uint16_t index;
	bool cur_Mute_Flag; //for old mute function
	bool prev_Mute_Flag; //for old mute function
	bool Mute_Active; // this is for new mute logic
  }UART_Context;

/* Shared global objects - defined in mail.c */
extern osMailQId     mail_queue_id;
extern UART_Context  UART_ContextData[NUM_UARTS];

/* One mutex per UART - protects the INPUT state echo + fragmentation block */
extern osMutexId     uart_mutex[NUM_UARTS];

extern osThreadId T_Text1;
extern osThreadId T_Text2;
extern osThreadId T_Text3;

#endif /* COMM_TYPES_H */