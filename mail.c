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

//Size message limits for message mail 
#define MAX_MAIL_PAYLOAD 32
#define LOCAL_BUF_SIZE 128 // A large temporary buffer to hold input until 'Enter' is pressed
#define CHUNK_SIZE     (MAX_MAIL_PAYLOAD - 1) // 31 characters + 1 for '\0'

bool mute_Flag;

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
    uint8_t is_fragmented; // 0 = Normal, 1 = first chunk of a fragmented message, 2 = continuation chunk
    Recipient_ID Receiver; // 1 or 2, so the Tx thread knows where it came from
	  Sender_ID Sender;    // 1 2 or 3 for intended receiver
	  uint16_t msgCnt; //mesage count for periodic messages
} Mail;

	

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



typedef struct {
	UART_STATUS UART_State;
	Recipient_ID Recipient;
	bool Mute_Sender[NUM_UARTS];
	bool Emergency_Mode;
  }UART_Context;

	
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
char local_buffer[LOCAL_BUF_SIZE];
	uint16_t index = 0;
	uint16_t i;
  bool cur_Mute_Flag = false;
	bool prev_Mute_Flag = true;
		
	//static UART_STATUS UART3_Status = UART_IDLE;
	
	UART_ContextData[UART_1].Emergency_Mode = false;
	UART_ContextData[UART_1].Mute_Sender[UART_2] = false;
	UART_ContextData[UART_1].Mute_Sender[UART_3] = false;
	UART_ContextData[UART_1].Recipient = Recipient_Group;
	UART_ContextData[UART_1].UART_State = UART_IDLE;
	

	for (;;)
	{

    	osSignalWait (0x01,osWaitForever);

			UART1_Input = osMessageGet(Q_UART1, 0);
			intKey1 = (uint8_t)UART1_Input.value.v;

		  switch(UART_ContextData[UART_1].UART_State)
			{
			  case(UART_IDLE):
				  {
				    if (intKey1 == '\r' || intKey1 == '\n') 
							{
			          SendChar1('\r');
			          SendChar1('\n');
				      }
			       else if(intKey1 == '1')
						   {
                 SendChar1(intKey1);
			           Send_User_Menu(UART_1);
			           UART_ContextData[UART_1].UART_State = UART_MENU;								 
						   }
							else
							{
							   	UART_ContextData[UART_1].UART_State = UART_INPUT;
							    SendChar1(intKey1);
								  			if(index < LOCAL_BUF_SIZE - 1) 
				                  {
					                    local_buffer[index++] = intKey1;
			                     }	
							}
			
			
			      }
					break;
						
				case(UART_INPUT):
				{
				 if (intKey1 == '\r' || intKey1 == '\n') 
							{
			          SendChar1('\r');
			          SendChar1('\n');
				      }	
							else{
							SendChar1(intKey1);
							
							}
					
				// 2. Check for End of Message (User pressed Enter / Carriage Return)
		  if (intKey1 == '\r' || intKey1 == '\n'){
			  local_buffer[index] = '\0'; // we force a Null to terminate the string
		
			if (index > 0){ // Only send if the user actually typed something
				uint16_t remaining_bytes = index;
				uint16_t buffer_ptr = 0;
				// set a flag if the message is bigger than 32 characters 
				uint8_t fragmented_flag = (remaining_bytes > CHUNK_SIZE) ? 1 : 0;
				
				// Alert the user on their local screen that fragmentation is happening
				if (fragmented_flag){
					//osMutexWait(uart_mutex, osWaitForever);
					SendText1((uint8_t *)("\r\n[System: Message too long. Segmenting into packets...]\r\n"));
					//osMutexRelease(uart_mutex);
				}
					
				// --- FRAGMENTATION LOOP ---
				uint16_t chunk_index = 0;
				while (remaining_bytes > 0){
					// Allocate RTX Mail, populate it with data and send it
					Mail *mail;
					mail = (Mail*)osMailAlloc(mail_queue_id, osWaitForever);
					if (mail == NULL){
						// Alloc failed: bail out so we never spin forever and starve MailOutput
						break;
					}
					{
						//  allocates a mail slot and fill it with data
						mail->Sender = Sender_1; // Originating from UART1
						// 0 = not fragmented, 1 = first chunk of a fragmented message, 2 = continuation chunk
						mail->is_fragmented = (!fragmented_flag) ? 0 : ((chunk_index == 0) ? 1 : 2);
						mail->Receiver = 	UART_ContextData[UART_1].Recipient;
						// Calculate how many characters fit in this specific mail slice.
						// If we'd split mid-word, back up to the last space so whole words
						// stay together on each line.
						uint16_t bytes_to_copy;
						if (remaining_bytes <= CHUNK_SIZE){
							bytes_to_copy = remaining_bytes;
						} else {
							// Search backwards from CHUNK_SIZE for the last space
							bytes_to_copy = CHUNK_SIZE; // Fallback: hard split (no space found)
							for (i = CHUNK_SIZE; i > 0; i--) {
								if (local_buffer[buffer_ptr + i - 1] == ' ') {
									bytes_to_copy = i - 1; // Copy up to (not including) the space
									break;
								}
							}
						}
						//Fragmentate the message
						for (i = 0; i < bytes_to_copy; i++){
							mail->payload[i] = local_buffer[buffer_ptr + i];
						}

						mail->payload[bytes_to_copy] = '\0'; // Explicitly force null-termination
						// Ship it to the Mail Queue
						osMailPut(mail_queue_id, mail);
						// Shift tracking pointers forward past the copied bytes
						buffer_ptr += bytes_to_copy;
						remaining_bytes -= bytes_to_copy;
						chunk_index++;
						// Skip the space we split on so the next line has no leading space
						if (remaining_bytes > 0 && local_buffer[buffer_ptr] == ' ') {
							buffer_ptr++;
							remaining_bytes--;
						}
					}
				}
			}
			UART_ContextData[UART_1].UART_State = UART_IDLE;
			index = 0; // Clear index tracking for the next fresh message
			} 
		else
			{
			// 3. Accumulate normal typing into buffer safely avoiding memory overflow
			if(index < LOCAL_BUF_SIZE - 1) 
				{
					local_buffer[index++] = intKey1;
			  }	
	    }
				}
				break;
				
				case(UART_MENU):
				{
				  		if (intKey1 == '\r' || intKey1 == '\n') 
							{
			          SendChar1('\r');
			          SendChar1('\n');
				      }
							switch(intKey1)
							{
							  case('g'):
								{
								  UART_ContextData[UART_1].Recipient = Recipient_Group;
									UART_ContextData[UART_1].UART_State = UART_IDLE;	
									SendText1("Messaging: Group\n");
									break;
								}
								case('2'):
								{
								  UART_ContextData[UART_1].Recipient = Recipient_2;
									UART_ContextData[UART_1].UART_State = UART_IDLE;
									SendText1("Messaging: User 2\n");
									break;
								}
								case('3'):
								{
								  UART_ContextData[UART_1].Recipient = Recipient_3;
									UART_ContextData[UART_1].UART_State = UART_IDLE;
								  SendText1("Messaging: User 3\n");
									break;
								}
								case('m'):
								{
									
									if(cur_Mute_Flag == false && prev_Mute_Flag == true){
									cur_Mute_Flag = true;
									prev_Mute_Flag = false;
								  UART_ContextData[UART_1].Mute_Sender[UART_1] = true;
									UART_ContextData[UART_1].Mute_Sender[UART_3] = true;
									SendText1("Messages Muted\n");
									UART_ContextData[UART_1].UART_State = UART_IDLE;
	                continue;
									}
									else if(cur_Mute_Flag == true && prev_Mute_Flag == false)
									{
									cur_Mute_Flag = false;
									prev_Mute_Flag = true;
									UART_ContextData[UART_1].Mute_Sender[UART_1] = false;
									UART_ContextData[UART_1].Mute_Sender[UART_3] = false;
									SendText1("Messages UnMuted\n");
									UART_ContextData[UART_1].UART_State = UART_IDLE;
                  continue;
									}
									prev_Mute_Flag = cur_Mute_Flag;
									break;
								}
								case('9'):
								{
								  SendText1("Menu Closed, Message traffic will resume...\n");
								  UART_ContextData[UART_1].UART_State = UART_IDLE;
									break;
								}
							  default:
									SendText1("Invalid Entry, please select valid menu item...!\nOr press '9' to exit!\n");
								  //UART_ContextData[UART_3].UART_State = UART_IDLE;
									break;
							
							
							}
				
				}
				break;
					}//end switch
	

  }	


	}

/*----------------------------------------------------------------------------
 * :
 *
 * Description: 
 *---------------------------------------------------------------------------*/
void UART2_Rx_Thread (void const *argument) 
{
char local_buffer[LOCAL_BUF_SIZE];
	uint16_t index = 0;
	uint16_t i;

		
	//static UART_STATUS UART3_Status = UART_IDLE;
	
	UART_ContextData[UART_2].Emergency_Mode = false;
	UART_ContextData[UART_2].Mute_Sender[UART_1] = false;
	UART_ContextData[UART_2].Mute_Sender[UART_3] = false;
	UART_ContextData[UART_2].Recipient = Recipient_Group;
	UART_ContextData[UART_2].UART_State = UART_IDLE;
	

	for (;;)
	{

    	osSignalWait (0x02,osWaitForever);

			UART2_Input = osMessageGet(Q_UART2, 0);
			intKey2 = (uint8_t)UART2_Input.value.v;

		  switch(UART_ContextData[UART_2].UART_State)
			{
			  case(UART_IDLE):
				  {
				    if (intKey2 == '\r' || intKey2 == '\n') 
							{
			          SendChar2('\r');
			          SendChar2('\n');
				      }
			       else if(intKey2 == '1')
						   {
                 SendChar2(intKey2);
			           Send_User_Menu(UART_2);
			           UART_ContextData[UART_2].UART_State = UART_MENU;								 
						   }
							else
							{
							   	UART_ContextData[UART_2].UART_State = UART_INPUT;
							    SendChar2(intKey2);
								  			if(index < LOCAL_BUF_SIZE - 1) 
				                  {
					                    local_buffer[index++] = intKey2;
			                     }	
							}
			
			
			      }
					break;
						
				case(UART_INPUT):
				{
				 if (intKey2 == '\r' || intKey2 == '\n') 
							{
			          SendChar2('\r');
			          SendChar2('\n');
				      }	
							else{
							SendChar2(intKey2);
							
							}
					
				// 2. Check for End of Message (User pressed Enter / Carriage Return)
		  if (intKey2 == '\r' || intKey2 == '\n'){
			  local_buffer[index] = '\0'; // we force a Null to terminate the string
		
			if (index > 0){ // Only send if the user actually typed something
				uint16_t remaining_bytes = index;
				uint16_t buffer_ptr = 0;
				// set a flag if the message is bigger than 32 characters 
				uint8_t fragmented_flag = (remaining_bytes > CHUNK_SIZE) ? 1 : 0;
				
				// Alert the user on their local screen that fragmentation is happening
				if (fragmented_flag){
					//osMutexWait(uart_mutex, osWaitForever);
					SendText2((uint8_t *)("\r\n[System: Message too long. Segmenting into packets...]\r\n"));
					//osMutexRelease(uart_mutex);
				}
					
				// --- FRAGMENTATION LOOP ---
				uint16_t chunk_index = 0;
				while (remaining_bytes > 0){
					// Allocate RTX Mail, populate it with data and send it
					Mail *mail;
					mail = (Mail*)osMailAlloc(mail_queue_id, osWaitForever);
					if (mail == NULL){
						// Alloc failed: bail out so we never spin forever and starve MailOutput
						break;
					}
					{
						//  allocates a mail slot and fill it with data
						mail->Sender = Sender_2; // Originating from UART2
						// 0 = not fragmented, 1 = first chunk of a fragmented message, 2 = continuation chunk
						mail->is_fragmented = (!fragmented_flag) ? 0 : ((chunk_index == 0) ? 1 : 2);
						mail->Receiver = 	UART_ContextData[UART_2].Recipient;
						// Calculate how many characters fit in this specific mail slice.
						// If we'd split mid-word, back up to the last space so whole words
						// stay together on each line.
						uint16_t bytes_to_copy;
						if (remaining_bytes <= CHUNK_SIZE){
							bytes_to_copy = remaining_bytes;
						} else {
							// Search backwards from CHUNK_SIZE for the last space
							bytes_to_copy = CHUNK_SIZE; // Fallback: hard split (no space found)
							for (i = CHUNK_SIZE; i > 0; i--) {
								if (local_buffer[buffer_ptr + i - 1] == ' ') {
									bytes_to_copy = i - 1; // Copy up to (not including) the space
									break;
								}
							}
						}
						//Fragmentate the message
						for (i = 0; i < bytes_to_copy; i++){
							mail->payload[i] = local_buffer[buffer_ptr + i];
						}

						mail->payload[bytes_to_copy] = '\0'; // Explicitly force null-termination
						// Ship it to the Mail Queue
						osMailPut(mail_queue_id, mail);
						// Shift tracking pointers forward past the copied bytes
						buffer_ptr += bytes_to_copy;
						remaining_bytes -= bytes_to_copy;
						chunk_index++;
						// Skip the space we split on so the next line has no leading space
						if (remaining_bytes > 0 && local_buffer[buffer_ptr] == ' ') {
							buffer_ptr++;
							remaining_bytes--;
						}
					}
				}
			}
			UART_ContextData[UART_2].UART_State = UART_IDLE;
			index = 0; // Clear index tracking for the next fresh message
			} 
		else
			{
			// 3. Accumulate normal typing into buffer safely avoiding memory overflow
			if(index < LOCAL_BUF_SIZE - 1) 
				{
					local_buffer[index++] = intKey2;
			  }	
	    }
				}
				break;
				
				case(UART_MENU):
				{
				  		if (intKey2 == '\r' || intKey2 == '\n') 
							{
			          SendChar2('\r');
			          SendChar2('\n');
				      }
							switch(intKey2)
							{
							  case('g'):
								{
								  UART_ContextData[UART_2].Recipient = Recipient_Group;
									UART_ContextData[UART_2].UART_State = UART_IDLE;	
									SendText2("Messaging: Group\n");
									break;
								}
								case('1'):
								{
								  UART_ContextData[UART_2].Recipient = Recipient_1;
									UART_ContextData[UART_2].UART_State = UART_IDLE;
									SendText2("Messaging: User 1\n");
									break;
								}
								case('3'):
								{
								  UART_ContextData[UART_2].Recipient = Recipient_3;
									UART_ContextData[UART_2].UART_State = UART_IDLE;
								  SendText2("Messaging: User 3\n");
									break;
								}
								case('m'):
								{
								  UART_ContextData[UART_2].Mute_Sender[UART_1] = true;
									UART_ContextData[UART_2].Mute_Sender[UART_3] = true;
									SendText2("Messages Muted\n");
									UART_ContextData[UART_2].UART_State = UART_IDLE;
									break;
								}
								case('9'):
								{
								  SendText3("Menu Closed, Message traffic will resume...\n");
								  UART_ContextData[UART_2].UART_State = UART_IDLE;
									break;
								}
							  default:
									SendText2("Invalid Entry, please select valid menu item...!\nOr press '9' to exit!\n");
								  //UART_ContextData[UART_3].UART_State = UART_IDLE;
									break;
							
							
							}
				
				}
				break;
					}//end switch
	

  }	


}	

/*----------------------------------------------------------------------------
 * :
 *
 * Description: 
 *---------------------------------------------------------------------------*/
void UART3_Rx_Thread (void const *argument) 
{
char local_buffer[LOCAL_BUF_SIZE];
	uint16_t index = 0;
	uint16_t i;

		
	//static UART_STATUS UART3_Status = UART_IDLE;
	
	UART_ContextData[UART_3].Emergency_Mode = false;
	UART_ContextData[UART_3].Mute_Sender[UART_1] = false;
	UART_ContextData[UART_3].Mute_Sender[UART_2] = false;
	UART_ContextData[UART_3].Recipient = Recipient_Group;
	UART_ContextData[UART_3].UART_State = UART_IDLE;
	

	for (;;)
	{

    	osSignalWait (0x02,osWaitForever);

			UART3_Input = osMessageGet(Q_UART3, 0);
			intKey3 = (uint8_t)UART3_Input.value.v;

		  switch(UART_ContextData[UART_3].UART_State)
			{
			  case(UART_IDLE):
				  {
				    if (intKey3 == '\r' || intKey3 == '\n') 
							{
			          SendChar3('\r');
			          SendChar3('\n');
				      }
			       else if(intKey3 == '1')
						   {
                 SendChar3(intKey3);
			           Send_User_Menu(UART_3);
			           UART_ContextData[UART_3].UART_State = UART_MENU;								 
						   }
							else
							{
							   	UART_ContextData[UART_3].UART_State = UART_INPUT;
							    SendChar3(intKey3);
								  			if(index < LOCAL_BUF_SIZE - 1) 
				                  {
					                    local_buffer[index++] = intKey3;
			                     }	
							}
			
			
			      }
					break;
						
				case(UART_INPUT):
				{
				 if (intKey3 == '\r' || intKey3 == '\n') 
							{
			          SendChar3('\r');
			          SendChar3('\n');
				      }	
							else{
							SendChar3(intKey3);
							
							}
					
				// 2. Check for End of Message (User pressed Enter / Carriage Return)
		  if (intKey3 == '\r' || intKey3 == '\n'){
			  local_buffer[index] = '\0'; // we force a Null to terminate the string
		
			if (index > 0){ // Only send if the user actually typed something
				uint16_t remaining_bytes = index;
				uint16_t buffer_ptr = 0;
				// set a flag if the message is bigger than 32 characters 
				uint8_t fragmented_flag = (remaining_bytes > CHUNK_SIZE) ? 1 : 0;
				
				// Alert the user on their local screen that fragmentation is happening
				if (fragmented_flag){
					//osMutexWait(uart_mutex, osWaitForever);
					SendText3((uint8_t *)("\r\n[System: Message too long. Segmenting into packets...]\r\n"));
					//osMutexRelease(uart_mutex);
				}
					
				// --- FRAGMENTATION LOOP ---
				uint16_t chunk_index = 0;
				while (remaining_bytes > 0){
					// Allocate RTX Mail, populate it with data and send it
					Mail *mail;
					mail = (Mail*)osMailAlloc(mail_queue_id, osWaitForever);
					if (mail == NULL){
						// Alloc failed: bail out so we never spin forever and starve MailOutput
						break;
					}
					{
						//  allocates a mail slot and fill it with data
						mail->Sender = Sender_3; // Originating from UART3
						// 0 = not fragmented, 1 = first chunk of a fragmented message, 2 = continuation chunk
						mail->is_fragmented = (!fragmented_flag) ? 0 : ((chunk_index == 0) ? 1 : 2);
						mail->Receiver = 	UART_ContextData[UART_3].Recipient;
						// Calculate how many characters fit in this specific mail slice.
						// If we'd split mid-word, back up to the last space so whole words
						// stay together on each line.
						uint16_t bytes_to_copy;
						if (remaining_bytes <= CHUNK_SIZE){
							bytes_to_copy = remaining_bytes;
						} else {
							// Search backwards from CHUNK_SIZE for the last space
							bytes_to_copy = CHUNK_SIZE; // Fallback: hard split (no space found)
							for (i = CHUNK_SIZE; i > 0; i--) {
								if (local_buffer[buffer_ptr + i - 1] == ' ') {
									bytes_to_copy = i - 1; // Copy up to (not including) the space
									break;
								}
							}
						}
						//Fragmentate the message
						for (i = 0; i < bytes_to_copy; i++){
							mail->payload[i] = local_buffer[buffer_ptr + i];
						}

						mail->payload[bytes_to_copy] = '\0'; // Explicitly force null-termination
						// Ship it to the Mail Queue
						osMailPut(mail_queue_id, mail);
						// Shift tracking pointers forward past the copied bytes
						buffer_ptr += bytes_to_copy;
						remaining_bytes -= bytes_to_copy;
						chunk_index++;
						// Skip the space we split on so the next line has no leading space
						if (remaining_bytes > 0 && local_buffer[buffer_ptr] == ' ') {
							buffer_ptr++;
							remaining_bytes--;
						}
					}
				}
			}
			UART_ContextData[UART_3].UART_State = UART_IDLE;
			index = 0; // Clear index tracking for the next fresh message
			} 
		else
			{
			// 3. Accumulate normal typing into buffer safely avoiding memory overflow
			if(index < LOCAL_BUF_SIZE - 1) 
				{
					local_buffer[index++] = intKey3;
			  }	
	    }
				}
				break;
				
				case(UART_MENU):
				{
				  		if (intKey3 == '\r' || intKey3 == '\n') 
							{
			          SendChar3('\r');
			          SendChar3('\n');
				      }
							switch(intKey3)
							{
							  case('g'):
								{
								  UART_ContextData[UART_3].Recipient = Recipient_Group;
									UART_ContextData[UART_3].UART_State = UART_IDLE;	
									SendText3("Messaging: Group\n");
									break;
								}
								case('1'):
								{
								  UART_ContextData[UART_3].Recipient = Recipient_1;
									UART_ContextData[UART_3].UART_State = UART_IDLE;
									SendText3("Messaging: User 1\n");
									break;
								}
								case('2'):
								{
								  UART_ContextData[UART_3].Recipient = Recipient_2;
									UART_ContextData[UART_3].UART_State = UART_IDLE;
								  SendText3("Messaging: User 2\n");
									break;
								}
								case('m'):
								{
								  UART_ContextData[UART_3].Mute_Sender[UART_1] = true;
									UART_ContextData[UART_3].Mute_Sender[UART_2] = true;
									SendText3("Messages Muted\n");
									UART_ContextData[UART_3].UART_State = UART_IDLE;
									break;
								}
								case('9'):
								{
								  SendText3("Menu Closed, Message traffic will resume...\n");
								  UART_ContextData[UART_3].UART_State = UART_IDLE;
									break;
								}
							  default:
									SendText3("Invalid Entry, please select valid menu item...!\nOr press '9' to exit!\n");
								  UART_ContextData[UART_3].UART_State = UART_MENU;
									break;
							
							
							}
				
				}
				break;
					}//end switch
	

  }	

}	

/*----------------------------------------------------------------------------
 * :
 *
 * Description: 
 *---------------------------------------------------------------------------*/
void Tx_Routing_Thread (void const *argument) 
{
	for (;;) 
	{
		char header_buffer[64];
		uint8_t index;
		char Header_Buffer[64];
		
	 // Block until mail arrives from the queue
        osEvent evt = osMailGet(mail_queue_id, osWaitForever);
        
        if (evt.status == osEventMail) 
        {
            Mail *mail = (Mail*)evt.value.p;
            
            //osMutexWait(uart_mutex1, osWaitForever);
            
            // Set the incoming text color to Cyan for clear tracking
            //SendText((uint8_t *)CLR_RX);
            Prepare_Output_Header(Header_Buffer, mail->Sender);
					switch(mail->Receiver)
				{
					case Recipient_1:
            // Print out the payload piece (only show the header once, for the first chunk)
					  if(UART_ContextData[UART_1].Mute_Sender[UART_2] != false){
					  if (mail->is_fragmented != 2) SendText1(Header_Buffer);
            SendText1((uint8_t *)mail->payload);
						SendText1((uint8_t *)"\r\n");
						}
          break;
					case Recipient_2:
						if (mail->is_fragmented != 2) SendText2(Header_Buffer);
						SendText2((uint8_t *)mail->payload);
						SendText2((uint8_t *)"\r\n");
					break;
					case Recipient_3:
						if (mail->is_fragmented != 2) SendText3(Header_Buffer);
						SendText3((uint8_t *)mail->payload);
						SendText3((uint8_t *)"\r\n");
					break;
           
					case Recipient_Group:
						for( index = 0; index < NUM_UARTS; index++){
						
							if((index == mail->Sender) || (UART_ContextData[index].Mute_Sender[mail->Sender] == true) )
							{
							  continue; 
							
							}
							else
							{
							   switch(index){
								   
									 case UART_1:
									 {
										 if((UART_ContextData[UART_1].Mute_Sender[UART_2] == false) &&
											 (UART_ContextData[UART_1].Mute_Sender[UART_3] == false))
												{
										 if (mail->is_fragmented != 2) SendText1(Header_Buffer);
									   SendText1(mail->payload);
										 SendText1("\r\n");
											 }
										break;

									 }
								   case UART_2:
									 {
										 if (mail->is_fragmented != 2) SendText2(Header_Buffer);
									   SendText2(mail->payload);
										 SendText2 ("\r\n");
									 break;
									 }
								   case UART_3:
									 {
										 if (mail->is_fragmented != 2) SendText3(Header_Buffer);
									   SendText3(mail->payload);
										 SendText3("\r\n");
									 break;
									 }
										 

								 }
							
							
							}
						}
						break;
            //osMutexRelease(uart_mutex);

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

/*----------------------------------------------------------------------------
SendText: This function receives a pointer to a text string and parses the
          chars one at a time and sends to the UART output window.
*---------------------------------------------------------------------------*/
void SendText1(uint8_t *text)
{
  char currChar;
  currChar = *text;
    
  while(currChar != '\0')
		{
		  SendChar1(currChar);
      text++;
      currChar = *text;
	  }
}

/*----------------------------------------------------------------------------
SendText: This function receives a pointer to a text string and parses the
          chars one at a time and sends to the UART output window.
*---------------------------------------------------------------------------*/
void SendText2(uint8_t *text)
{
  char currChar;
  currChar = *text;
    
  while(currChar != '\0')
		{
		  SendChar2(currChar);
      text++;
      currChar = *text;
	  }
}

/*----------------------------------------------------------------------------
SendText: This function receives a pointer to a text string and parses the
          chars one at a time and sends to the UART output window.
*---------------------------------------------------------------------------*/
void SendText3(uint8_t *text)
{
  char currChar;
  currChar = *text;
    
  while(currChar != '\0')
		{
		  SendChar3(currChar);
      text++;
      currChar = *text;
	  }
}


void Send_User_Menu(UART_ID Output_ID){

  switch(Output_ID){
	
		case(UART_1):
		{
		  SendText1("\nWelcome to message menu: \n"
			           "2 - Message User 2\n"
			           "3 - Message User 3\n"
                 "g - Message GROUP\n"			
			           "m - Mute messages\n");
			break;
		case(UART_2):
		{		  SendText2("\nWelcome to message menu: \n"
			           "1 - Message User 1\n"
			           "3 - Message User 3\n"
                 "g - Message GROUP\n"			
			           "m - Mute messages\n");
			break;
		 
		
		}
		case(UART_3):
		{		  SendText3("\nWelcome to message menu: \n"
			           "2 - Message User 2\n"
			           "3 - Message User 3\n"
                 "g - Message GROUP\n"			
			           "m - Mute messages\n");
			break;
		 
		
		}		
		}
	
	}


}

void Prepare_Output_Header(char* Header_Buffer, Sender_ID Sender){
  

  switch(Sender){
		
		case(Sender_1):
		{
		  sprintf(Header_Buffer, "\n[Rec from User 1:] \n");
		break;
		}
	  case(Sender_2):
		{
		  sprintf(Header_Buffer, "\n[Rec from User 2:] \n");
		break;
			
		}
	  case(Sender_3):
		{
		  sprintf(Header_Buffer, "\n[Rec from User 3:] \n");
		break;		
		}
		case(Sender_Auto):
		{
		  sprintf(Header_Buffer, "\n[Rec from Auto:] \n");
		break;
					
		
		
		}
	}

}
