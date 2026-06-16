#ifndef COMM_TYPES_H
#define COMM_TYPES_H

#include "cmsis_os.h"
#include <stdint.h>
#include <stdbool.h>

/* Size limits for mail/message handling */
#define MAX_MAIL_PAYLOAD 32
#define LOCAL_BUF_SIZE   128                  /* temp buffer until 'Enter' is pressed */
#define CHUNK_SIZE       (MAX_MAIL_PAYLOAD - 1)

typedef enum {
    MSG_Emergency = 0,
    MSG_Standard  = 1,
    MSG_Periodic1 = 2,
    MSG_Periodic2 = 3
} message_Type;

typedef enum {
    UART_1 = 0,
    UART_2 = 1,
    UART_3 = 2,
    NUM_UARTS = 3
} UART_ID;

typedef enum {
    UART_IDLE  = 1,
    UART_INPUT = 2,
    UART_MENU  = 3
} UART_STATUS;

typedef enum {
    Recipient_1 = UART_1,
    Recipient_2 = UART_2,
    Recipient_3 = UART_3,
    Recipient_Group
} Recipient_ID;

typedef enum {
    Sender_1 = UART_1,
    Sender_2 = UART_2,
    Sender_3 = UART_3,
    Sender_Auto
} Sender_ID;

typedef struct {
    char payload[MAX_MAIL_PAYLOAD];
    uint8_t is_fragmented;     /* 0 = normal, 1 = first chunk, 2 = continuation chunk */
    Recipient_ID Receiver;
    Sender_ID Sender;
    uint16_t msgCnt;
} Mail;

/* Per-UART runtime context (state machine + input buffer) */
typedef struct {
    UART_STATUS UART_State;
    Recipient_ID Recipient;
    bool Mute_Sender[NUM_UARTS];
    bool Emergency_Mode;
    bool Mute_Active;               /* toggles on/off with menu option 'm' */
    char local_buffer[LOCAL_BUF_SIZE];
    uint16_t index;
} UART_Context;

/* Shared global objects - defined in mail.c */
extern osMailQId     mail_queue_id;
extern UART_Context  UART_ContextData[NUM_UARTS];

extern osThreadId T_Text1;
extern osThreadId T_Text2;
extern osThreadId T_Text3;

#endif /* COMM_TYPES_H */
