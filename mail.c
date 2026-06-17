/*----------------------------------------------------------------------------
	
	Designers Guide to the Cortex-M Family
	CMSIS mailbox Example
*----------------------------------------------------------------------------*/
#include "STM32F10x.h"
#include "cmsis_os.h"
#include "Board_LED.h"
#include "uart.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

//Created libraries 
#include "comm_types.h"
#include "uart_dispatch.h"
#include "mail_fragment.h"
#include "uart_rx_communication.h"

//Size message limits for message mail 

bool mute_Flag;
	
// Mail Q Id
osMailQId  mail_queue_id;
//Setup for Mail Queue 
osMailQDef(mail_queue, 100, Mail); // Define a mail queue of 10 blocks

// Mail Q Id
osMailQId  mail_queue_id;

volatile uint8_t Text_Buffer[64];
volatile uint8_t  intKey1, intKey2, intKey3;

int UART_Periodic_Index = 0;

//count variables for auto messages
uint16_t UART1_seq_count1 = 0;
uint16_t UART2_seq_count1 = 0;
uint16_t UART3_seq_count1 = 0;

uint16_t UART1_seq_count2 = 0;
uint16_t UART2_seq_count2 = 0;
uint16_t UART3_seq_count2 = 0;


	
//Setup and define threads and priority levels
void UART1_Rx_Thread (void const *argument);
void UART2_Rx_Thread (void const *argument);
void UART3_Rx_Thread (void const *argument);

void Tx_Routing_Thread (void const *argument);
void Auto_Mess1_Thread (void const *argument);
void Auto_Mess2_Thread (void const *argument);
//void emergency_Thread (void const *argument);

//virtual timers
void callback(void const *param);
void callback1(void const *param);

osTimerDef(timer0_handle, callback);
osTimerDef(timer1_handle, callback1);
osTimerDef(timer2_handle, callback);
osTimerDef(timer3_handle, callback);

//MessageQueues
osMessageQId UART1; 
osMessageQId UART2;
osMessageQId UART3;

//Define Message Queues
osMessageQDef (UART1,0x16,unsigned char);
osMessageQDef (UART2,0x16,unsigned char);
osMessageQDef (UART3,0x16,unsigned char);

//osThreadDef(emergency_Thread, osPriorityAboveNormal, 1, 0);
osThreadDef(UART1_Rx_Thread, osPriorityNormal, 1, 0);
osThreadDef(UART2_Rx_Thread, osPriorityNormal, 1, 0);
osThreadDef(UART3_Rx_Thread, osPriorityNormal, 1, 0);

osThreadDef(Tx_Routing_Thread, osPriorityNormal, 1, 0);
osThreadDef(Auto_Mess1_Thread, osPriorityNormal, 1, 0);
osThreadDef(Auto_Mess2_Thread, osPriorityNormal, 1, 0);


//Thread IDs 
osThreadId T_Emergency;
osThreadId T_Text1;
osThreadId T_Text2;
osThreadId T_Text3;
osThreadId T_Routing;
osThreadId T_Auto_mess1;
osThreadId T_Auto_mess2;


osThreadId T_Message1;
osThreadId T_Message2;

//function prototype
void SendText1(uint8_t *txt);
void SendText2(uint8_t *txt);
void SendText3(uint8_t *txt);
void create_Periodic_MSG(UART_ID id);
void Send_User_Menu(UART_ID Output_ID);
void Prepare_Output_Header(char* Header_Buffer, Sender_ID);

osEvent  UART1_Input;
osEvent  UART2_Input;
osEvent  UART3_Input;


//add mutex capability
osMutexId uart1_mutex;
osMutexDef(uart1_mutex);

osMutexId uart2_mutex;
osMutexDef(uart2_mutex);

osMutexId uart3_mutex;
osMutexDef(uart3_mutex);

//setup and define message queues
osMessageQId Q_UART1;         
osMessageQId Q_UART2;
osMessageQId Q_UART3;


osMessageQDef (Q_UART1,0x16,unsigned char);
osMessageQDef (Q_UART2, 0x16, unsigned char);
osMessageQDef (Q_UART3, 0x16, unsigned char);

//context variables per UART

uint8_t UART1_RxAllow;
uint8_t UART2_RXAllow;

UART_Context UART_ContextData[NUM_UARTS];
/*----------------------------------------------------------------------------
  Timer callback function. Toggle the LED associated with the timer
 *---------------------------------------------------------------------------*/
void callback(void const *param)
{
	osSignalSet(T_Auto_mess1, 0x01);
	/*
	switch(UART_Periodic_Index)
	{
		case 0:
			create_Periodic_MSG(UART_1);
		  UART_Periodic_Index = 1;
		break;

		case 1:
			create_Periodic_MSG(UART_2);
		  UART_Periodic_Index = 2;
		break;

		case 2:
			create_Periodic_MSG(UART_3);
		  UART_Periodic_Index = 0;
		break;

		default:
			break;
			
			
	}  */
}

/*----------------------------------------------------------------------------
  Timer callback function. Toggle the LED associated with the timer
 *---------------------------------------------------------------------------*/
void callback1(void const *param)
{
	osSignalSet(T_Auto_mess2, 0x01);
	/*
	switch(UART_Periodic_Index)
	{
		case 0:
			create_Periodic_MSG(UART_1);
		  UART_Periodic_Index = 1;
		break;

		case 1:
			create_Periodic_MSG(UART_2);
		  UART_Periodic_Index = 2;
		break;

		case 2:
			create_Periodic_MSG(UART_3);
		  UART_Periodic_Index = 0;
		break;

		default:
			break;
			
			
	}  */
}

/*----------------------------------------------------------------------------
 * :
 *
 * Description: 
 *---------------------------------------------------------------------------*/
void UART1_Rx_Thread (void const *argument) 
{
	UART_Context_Init(UART_1); //with this we define how this thread will operate, to whom it will talk and internal funcionality 
	
	for (;;) 
	{
		osSignalWait (0x01,osWaitForever);

		UART1_Input = osMessageGet(Q_UART1, 0);
		intKey1 = (uint8_t)UART1_Input.value.v;
		
		UART_Rx_Process(UART_1, intKey1); //we call the functionality of the thread
	}	
}

/*----------------------------------------------------------------------------
 * :
 *
 * Description: 
 *---------------------------------------------------------------------------*/
void UART2_Rx_Thread (void const *argument) 
{
	UART_Context_Init(UART_2); //with this we define how this thread will operate, to whom it will talk and internal funcionality 
	
	for (;;) 
	{
    osSignalWait (0x02,osWaitForever);

		UART2_Input = osMessageGet(Q_UART2, 0);
		intKey2 = (uint8_t)UART2_Input.value.v;
		
		UART_Rx_Process(UART_2, intKey2);
	}	
}

/*----------------------------------------------------------------------------
 * :
 *
 * Description: 
 *---------------------------------------------------------------------------*/
void UART3_Rx_Thread (void const *argument) 
{
	UART_Context_Init(UART_3); //with this we define how this thread will operate, to whom it will talk and internal funcionality 
	
	for (;;) 
	{
    osSignalWait (0x02,osWaitForever);

		UART3_Input = osMessageGet(Q_UART3, 0);
		intKey3 = (uint8_t)UART3_Input.value.v;
		
		UART_Rx_Process(UART_3, intKey3);
	}	
}

/*----------------------------------------------------------------------------
 * Tx_Routing_Thread
 *
 * Pulls completed/fragmented Mail items off mail_queue_id and writes them to
 * the right UART(s) using the UART_SendText() dispatch table.
 *---------------------------------------------------------------------------*/
void Tx_Routing_Thread (void const *argument)
{
	for (;;)
	{
		char Header_Buffer[64];
		uint8_t index;

		// Block until mail arrives from the queue
		osEvent evt = osMailGet(mail_queue_id, osWaitForever);

		if (evt.status == osEventMail)
		{
			Mail *mail = (Mail*)evt.value.p;

			Prepare_Output_Header(Header_Buffer, mail->Sender);

			switch (mail->Receiver)
			{
				case Recipient_1:
				case Recipient_2:
				case Recipient_3:
				{
					UART_ID target = (UART_ID)mail->Receiver;

					if (mail->is_fragmented != 2) UART_SendText(target, (uint8_t*)Header_Buffer);
					UART_SendText(target, (uint8_t*)mail->payload);
					UART_SendText(target, (uint8_t*)"\r\n");
					break;
				}

				case Recipient_Group:
					for (index = 0; index < NUM_UARTS; index++)
					{
						if ((index == (uint8_t)mail->Sender) ||
							(UART_ContextData[index].Mute_Sender[mail->Sender] == true))
						{
							continue;
						}

						if (mail->is_fragmented != 2) UART_SendText((UART_ID)index, (uint8_t*)Header_Buffer);
						UART_SendText((UART_ID)index, (uint8_t*)mail->payload);
						UART_SendText((UART_ID)index, (uint8_t*)"\r\n");
					}
					break;

				default:
					break;
			}

			// Recycle the mail memory block back to RTX Pool
			osMailFree(mail_queue_id, mail);
		}
	}
}

/*----------------------------------------------------------------------------
 * :
 *
 * Description: 
 *---------------------------------------------------------------------------*/
void Auto_Mess2_Thread (void const *argument) 
{
	Recipient_ID current_out = Recipient_1;
	
	for (;;) 
	{
		
		osSignalWait(0x01, osWaitForever);
    
		uint16_t count;
		uint8_t UART_num;
		Recipient_ID output_id;
		
		switch(current_out)
	{
	  case(Recipient_1):
			UART1_seq_count2 += 1;
		  count = UART1_seq_count2;
		  UART_num = 1;
		  output_id = current_out;
		  current_out = Recipient_2;

		break;
				
		case(Recipient_2):
			UART2_seq_count2 += 1;
		
			count = UART2_seq_count2;
		  UART_num = 2;
			output_id = current_out;
			current_out = Recipient_3;
		
		break;
				
		case(Recipient_3):
			UART3_seq_count2 += 1;
		
			count = UART3_seq_count2;
		  UART_num = 3;
			output_id = current_out;
			current_out = Recipient_1;
		break;
		
		default:
			break;
	
		
		
	}
	Mail *mail;
		mail = (Mail*)osMailAlloc(mail_queue_id, osWaitForever);
		if (mail == NULL)
				{
			     // Alloc failed: bail out so we never spin forever and starve MailOutput
					  //break;
				}
			mail->Sender = Sender_Auto;	
			mail->is_fragmented = 0;				
			sprintf(mail->payload, "\nHearbeat message #%u for uart #%d." , count, UART_num);
			mail->Receiver = output_id;
			osMailPut(mail_queue_id, mail);
		
  }	

}	




/*----------------------------------------------------------------------------
 * :
 *
 * Description: 
 *---------------------------------------------------------------------------*/
void Auto_Mess1_Thread (void const *argument) 
{
	Recipient_ID current_out = Recipient_1;
	
	for (;;) 
	{
		
		osSignalWait(0x01, osWaitForever);
    
		uint16_t count;
		uint8_t UART_num;
		Recipient_ID output_id;
		
		switch(current_out)
	{
	  case(Recipient_1):
			UART1_seq_count1 += 1;
		  count = UART1_seq_count1;
		  UART_num = 1;
		  output_id = current_out;
		  current_out = Recipient_2;

		break;
				
		case(Recipient_2):
			UART2_seq_count1 += 1;
		
			count = UART2_seq_count1;
		  UART_num = 2;
			output_id = current_out;
			current_out = Recipient_3;
		
		break;
				
		case(Recipient_3):
			UART3_seq_count1 += 1;
		
			count = UART3_seq_count1;
		  UART_num = 3;
			output_id = current_out;
			current_out = Recipient_1;
		break;
		
		
		
		default:
			break;
	
		
		
	}
	Mail *mail;
		mail = (Mail*)osMailAlloc(mail_queue_id, osWaitForever);
		if (mail == NULL)
				{
			     // Alloc failed: bail out so we never spin forever and starve MailOutput
					  //break;
				}
			mail->Sender = Sender_Auto;
			mail->is_fragmented = 0;				
			sprintf(mail->payload, "\nAUTO #%u for uart #%d." , count, UART_num);
			mail->Receiver = output_id;
			osMailPut(mail_queue_id, mail);
		
  }	

}	


int main (void) 
{

	osKernelInitialize ();                    // initialize CMSIS-RTOS
	
	USART1_Init (); 
  //configure USART interrupts
	
  //Configure and enable USART1 interrupt 
  NVIC->ICPR[USART1_IRQn/32] = 1UL << (USART1_IRQn%32);  //clear any previous pending interrupt flag 
  NVIC->IP[USART1_IRQn] = 0x80; // NVIC_SetPriority(USART1_IRQn, 0x80); //set priority to 0x80  
  NVIC->ISER[USART1_IRQn/32] = 1UL << (USART1_IRQn%32); //set interrupt enable bit  
  USART1->CR1 |= USART_CR1_RXNEIE; //enable USART receiver not empty interrupt  
	
	//Configure and enable USART2 interrupt 
	USART2_Init ();
  NVIC->ICPR[USART2_IRQn/32] = 1UL << (USART2_IRQn%32); //clear any previous pending interrupt flag
  NVIC->IP[USART2_IRQn] = 0x80; //set priority to 0x80
  NVIC->ISER[USART2_IRQn/32] = 1UL << (USART2_IRQn%32); //set interrupt enable bit
  USART2->CR1 |= USART_CR1_RXNEIE; //enable USART receiver not empty interrupt
	
	//Configure and enable USART3 interrupt
	USART3_Init(); //Configure and enable USART3 interrupt
  NVIC->ICPR[USART3_IRQn/32] = 1UL << (USART3_IRQn%32);   // clear any previous pending interrupt flag
  NVIC->IP[USART3_IRQn] = 0x80;                           // set priority to 0x80
  NVIC->ISER[USART3_IRQn/32] = 1UL << (USART3_IRQn%32);   // set interrupt enable bit
  USART3->CR1 |= USART_CR1_RXNEIE;   
	
	
	LED_Initialize ();
	
	SendText1("Welcome, user 1\n \n");
	SendText2("Welcome, user 2\n \n");
	SendText3("Welcome, user 3\n \n");
	
	mail_queue_id = osMailCreate(osMailQ(mail_queue), NULL);
	
	
	T_Routing = osThreadCreate(osThread(Tx_Routing_Thread), NULL);
	T_Auto_mess1 = osThreadCreate(osThread(Auto_Mess1_Thread), NULL);
	T_Auto_mess2 = osThreadCreate(osThread(Auto_Mess2_Thread), NULL);
	T_Text1 = osThreadCreate(osThread(UART1_Rx_Thread), NULL);
	T_Text2 = osThreadCreate(osThread(UART2_Rx_Thread), NULL);
	T_Text3 = osThreadCreate(osThread(UART3_Rx_Thread), NULL);
	
	
	//create the message queues
	Q_UART1 = osMessageCreate(osMessageQ(Q_UART1),NULL);					
	Q_UART2 = osMessageCreate(osMessageQ(Q_UART2),NULL);
	Q_UART3 = osMessageCreate(osMessageQ(Q_UART3),NULL);

	//create mutex object
  uart1_mutex = osMutexCreate(osMutex(uart1_mutex));
	uart2_mutex = osMutexCreate(osMutex(uart2_mutex));
	uart3_mutex = osMutexCreate(osMutex(uart3_mutex));
	
	osTimerId timer0 = osTimerCreate(osTimer(timer0_handle), osTimerPeriodic, (void *)0);	
	osTimerStart(timer0, 58000);

	osTimerId timer1 = osTimerCreate(osTimer(timer1_handle), osTimerPeriodic, (void *)0);	
	osTimerStart(timer1, 48000);	


	osKernelStart ();                         // start thread execution 
	
}


/*----------------------------------------------------------------------------
USART1_IRQHandler: This is the IRQ handler for UART #1 input.
*---------------------------------------------------------------------------*/
void USART1_IRQHandler (void)
{ 
  intKey1 = (int8_t) (USART1->DR & 0x1FF);
	osMessagePut(Q_UART1, intKey1, 0);
	osSignalSet	(T_Text1,0x01);
	
} 


void USART2_IRQHandler (void) {
 
    intKey2 = (int8_t) (USART2->DR & 0x1FF);
		osMessagePut(Q_UART2, intKey2, 0);
		osSignalSet	(T_Text2,0x02);
}

// UART3 Interupt Handler
void USART3_IRQHandler (void) {

    intKey3 = (int8_t) (USART3->DR & 0x1FF);
		osMessagePut(Q_UART3, intKey3, 0);
		osSignalSet	(T_Text3,0x02);
}
