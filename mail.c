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


//Size message limits for message mail 
#define MAX_MAIL_PAYLOAD 32
#define LOCAL_BUF_SIZE 128 // A large temporary buffer to hold input until 'Enter' is pressed
#define CHUNK_SIZE     (MAX_MAIL_PAYLOAD - 1) // 31 characters + 1 for '\0'
#define CLR_SYSTEM  ""

typedef struct {
    char payload[MAX_MAIL_PAYLOAD];
    uint8_t is_fragmented; // 0 = Normal, 1 = Part of a split message
    uint8_t sendID; // 1 or 2, so the Tx thread knows where it came from
	  uint8_t recID;    // 1 2 or 3 for intended receiver
	  uint16_t msgCnt; //mesage count for periodic messages
} Mail;



// Mail Q Id
osMailQId  mail_queue_id;
//Setup for Mail Queue 
osMailQDef(mail_queue, 10, Mail); // Define a mail queue of 10 blocks

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

typedef enum
{
	MSG_Emergency = 0,
	MSG_Standard = 1,
	MSG_Periodic1 = 2,
	MSG_Periodic2 = 3
}message_Type;

typedef enum{
	UART_1 = 1,
	UART_2 = 2,
	UART_3 = 3

}UART_ID;

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

osEvent  UARTx;


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
	
	
	
	for (;;) 
	{
    	osSignalWait (0x01,osWaitForever);
		
			if (intKey1 == '\r' || intKey1 == '\n') {
			SendChar1('\r');
			SendChar1('\n');
		} else {
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
						mail->sendID = 1; // Originating from UART1
						// 0 = not fragmented, 1 = first chunk of a fragmented message, 2 = continuation chunk
						mail->is_fragmented = (!fragmented_flag) ? 0 : ((chunk_index == 0) ? 1 : 2);
						mail->recID = UART_2;
						
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

			index = 0; // Clear index tracking for the next fresh message
			}
		else{
			// 3. Accumulate normal typing into buffer safely avoiding memory overflow
			if (index < LOCAL_BUF_SIZE - 1) {
					local_buffer[index++] = intKey1;
			}
		}
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
	
	
	
	for (;;) 
	{
    	osSignalWait (0x01,osWaitForever);
		
			if (intKey2 == '\r' || intKey2 == '\n') {
			SendChar2('\r');
			SendChar2('\n');
		} else {
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
						mail->sendID = 2; // Originating from UART2
						// 0 = not fragmented, 1 = first chunk of a fragmented message, 2 = continuation chunk
						mail->is_fragmented = (!fragmented_flag) ? 0 : ((chunk_index == 0) ? 1 : 2);
						mail->recID = UART_3;

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

			index = 0; // Clear index tracking for the next fresh message
			}
		else{
			// 3. Accumulate normal typing into buffer safely avoiding memory overflow
			if (index < LOCAL_BUF_SIZE - 1) {
					local_buffer[index++] = intKey2;
			}
		
		
		
		
		
		
	}

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
	
	
	
	for (;;) 
	{
    	osSignalWait (0x02,osWaitForever);
		
			if (intKey3 == '\r' || intKey3 == '\n') {
			SendChar3('\r');
			SendChar3('\n');
		} else {
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
						mail->sendID = UART_3; // Originating from UART3
						// 0 = not fragmented, 1 = first chunk of a fragmented message, 2 = continuation chunk
						mail->is_fragmented = (!fragmented_flag) ? 0 : ((chunk_index == 0) ? 1 : 2);
						mail->recID = UART_1;

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

			index = 0; // Clear index tracking for the next fresh message
			}
		else{
			// 3. Accumulate normal typing into buffer safely avoiding memory overflow
			if (index < LOCAL_BUF_SIZE - 1) {
					local_buffer[index++] = intKey3;
			}
		
		
		
		
		
		
	}

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
		
	 // Block until mail arrives from the queue
        osEvent evt = osMailGet(mail_queue_id, osWaitForever);
        
        if (evt.status == osEventMail) 
        {
            Mail *mail = (Mail*)evt.value.p;
            
            //osMutexWait(uart_mutex1, osWaitForever);
            
            // Set the incoming text color to Cyan for clear tracking
            //SendText((uint8_t *)CLR_RX);
        switch(mail->recID)
				{
					case 1:
            // Print out the payload piece (only show "[rec]:" once, for the first chunk)
            if (mail->is_fragmented != 2) SendText1((uint8_t *)"[rec]:");
            SendText1((uint8_t *)mail->payload);
						SendText1((uint8_t *)("\r\n" CLR_SYSTEM)); // fragments are word-wrapped, so each one gets its own line
          break;
					case 2:
						if (mail->is_fragmented != 2) SendText2((uint8_t *)"[rec]:");
						SendText2((uint8_t *)mail->payload);
						SendText2((uint8_t *)("\r\n" CLR_SYSTEM));
					break;
					case 3:
						if (mail->is_fragmented != 2) SendText3((uint8_t *)"[rec]:");
						SendText3((uint8_t *)mail->payload);
						SendText3((uint8_t *)("\r\n" CLR_SYSTEM));
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
	UART_ID current_out = UART_1;
	
	for (;;) 
	{
		
		osSignalWait(0x01, osWaitForever);
    
		uint16_t count;
		uint8_t UART_num;
		UART_ID output_id;
		
		switch(current_out)
	{
	  case(UART_1):
			UART1_seq_count2 += 1;
		  count = UART1_seq_count2;
		  UART_num = 1;
		  output_id = current_out;
		  current_out = UART_2;
		  //sprintf(Text_Buffer, "Periodic Message number %d to User #1.\r\n", UART1_seq_count1);
			//SendText1(Text_Buffer);
		  
			//Mail *mail1;
		  //mail1 = (Mail*)osMailAlloc(mail_queue_id, osWaitForever);
		  //if (mail1 == NULL)
				//{
			     // Alloc failed: bail out so we never spin forever and starve MailOutput
					  //break;
				//}
			//mail1->sendID = 4;			
			//sprintf(mail1->payload, "Auto Message #%u for UART1.\n" , UART1_seq_count1);
			//mail1->recID = UART_1;
			//osMailPut(mail_queue_id, mail1);

		break;
				
		case(UART_2):
			UART2_seq_count2 += 1;
		
			count = UART2_seq_count2;
		  UART_num = 2;
			output_id = current_out;
			current_out = UART_3;
		  //sprintf(Text_Buffer, "Periodic Message number %d to User #2.\r\n", UART2_seq_count1);
			//SendText2(Text_Buffer);
		 // Mail *mail2;
		 // mail2 = (Mail*)osMailAlloc(mail_queue_id, osWaitForever);
		 // if (mail2 == NULL)
				//{
			     // Alloc failed: bail out so we never spin forever and starve MailOutput
					//  break;
				//}
		//	mail2->sendID = 4;			
			//sprintf(mail2->payload, "Auto Message #%u for UART2. \n" , UART2_seq_count1);
			//mail2->recID = UART_2;
			//osMailPut(mail_queue_id, mail2);
		
		break;
				
		case(UART_3):
			UART3_seq_count2 += 1;
		
			count = UART3_seq_count2;
		  UART_num = 3;
			output_id = current_out;
			current_out = UART_1;
		  //sprintf(Text_Buffer, "Periodic Message number %d to User #3.\r\n", UART3_seq_count1);
			//SendText3(Text_Buffer);
		  //Mail *mail3;
		  //mail3 = (Mail*)osMailAlloc(mail_queue_id, osWaitForever);
		 // if (mail3 == NULL)
				//{
			     // Alloc failed: bail out so we never spin forever and starve MailOutput
					  //break;
				//}
			//mail3->sendID = 4;			
			//sprintf(mail3->payload, "Auto Message #%u for UART3. \n" , UART3_seq_count1);
			//mail3->recID = UART_3;
			//osMailPut(mail_queue_id, mail3);
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
			mail->sendID = 4;			
			sprintf(mail->payload, "\nHearbeat message #%u for uart #%d.\n" , count, UART_num);
			mail->recID = output_id;
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
	UART_ID current_out = UART_1;
	
	for (;;) 
	{
		
		osSignalWait(0x01, osWaitForever);
    
		uint16_t count;
		uint8_t UART_num;
		UART_ID output_id;
		
		switch(current_out)
	{
	  case(UART_1):
			UART1_seq_count1 += 1;
		  count = UART1_seq_count1;
		  UART_num = 1;
		  output_id = current_out;
		  current_out = UART_2;
		  //sprintf(Text_Buffer, "Periodic Message number %d to User #1.\r\n", UART1_seq_count1);
			//SendText1(Text_Buffer);
		  
			//Mail *mail1;
		  //mail1 = (Mail*)osMailAlloc(mail_queue_id, osWaitForever);
		  //if (mail1 == NULL)
				//{
			     // Alloc failed: bail out so we never spin forever and starve MailOutput
					  //break;
				//}
			//mail1->sendID = 4;			
			//sprintf(mail1->payload, "Auto Message #%u for UART1.\n" , UART1_seq_count1);
			//mail1->recID = UART_1;
			//osMailPut(mail_queue_id, mail1);

		break;
				
		case(UART_2):
			UART2_seq_count1 += 1;
		
			count = UART2_seq_count1;
		  UART_num = 2;
			output_id = current_out;
			current_out = UART_3;
		  //sprintf(Text_Buffer, "Periodic Message number %d to User #2.\r\n", UART2_seq_count1);
			//SendText2(Text_Buffer);
		 // Mail *mail2;
		 // mail2 = (Mail*)osMailAlloc(mail_queue_id, osWaitForever);
		 // if (mail2 == NULL)
				//{
			     // Alloc failed: bail out so we never spin forever and starve MailOutput
					//  break;
				//}
		//	mail2->sendID = 4;			
			//sprintf(mail2->payload, "Auto Message #%u for UART2. \n" , UART2_seq_count1);
			//mail2->recID = UART_2;
			//osMailPut(mail_queue_id, mail2);
	
		break;
				
		case(UART_3):
			UART3_seq_count1 += 1;
		
			count = UART3_seq_count1;
		  UART_num = 3;
			output_id = current_out;
			current_out = UART_1;
		  //sprintf(Text_Buffer, "Periodic Message number %d to User #3.\r\n", UART3_seq_count1);
			//SendText3(Text_Buffer);
		  //Mail *mail3;
		  //mail3 = (Mail*)osMailAlloc(mail_queue_id, osWaitForever);
		 // if (mail3 == NULL)
				//{
			     // Alloc failed: bail out so we never spin forever and starve MailOutput
					  //break;
				//}
			//mail3->sendID = 4;			
			//sprintf(mail3->payload, "Auto Message #%u for UART3. \n" , UART3_seq_count1);
			//mail3->recID = UART_3;
			//osMailPut(mail_queue_id, mail3);
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
			mail->sendID = 4;			
			sprintf(mail->payload, "\nAUTO #%u for uart #%d. \n" , count, UART_num);
			mail->recID = output_id;
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
	UART1 = osMessageCreate(osMessageQ(UART1),NULL);		
	
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

	osSignalSet	(T_Text1,0x01);
	//osMessagePut(UART1,'1', 0);
	
} 


void USART2_IRQHandler (void) {
 
    intKey2 = (int8_t) (USART2->DR & 0x1FF);
		osSignalSet	(T_Text2,0x01);
    //osMessagePut(UART1,'2', 0);
}

// UART3 Interupt Handler
void USART3_IRQHandler (void) {

    intKey3 = (int8_t) (USART3->DR & 0x1FF);
	
		osSignalSet	(T_Text3,0x02);
    //osMessagePut(UART1,'3', 0);
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


/* void create_Periodic_MSG(UART_ID id)
{
  switch(id)
	{
	  case(1):
			UART1_seq_count1 += 1;
		  //sprintf(Text_Buffer, "Periodic Message number %d to User #1.\r\n", UART1_seq_count1);
			//SendText1(Text_Buffer);
		  
			Mail *mail1;
		  mail1 = (Mail*)osMailAlloc(mail_queue_id, osWaitForever);
		  if (mail1 == NULL)
				{
			     // Alloc failed: bail out so we never spin forever and starve MailOutput
					  break;
				}
			mail1->sendID = 4;			
			sprintf(mail1->payload, "Auto Message #%u for UART1.\n" , UART1_seq_count1);
			mail1->recID = UART_1;
			osMailPut(mail_queue_id, mail1);

		break;
				
		case(2):
			UART2_seq_count1 += 1;
		  //sprintf(Text_Buffer, "Periodic Message number %d to User #2.\r\n", UART2_seq_count1);
			//SendText2(Text_Buffer);
		  Mail *mail2;
		  mail2 = (Mail*)osMailAlloc(mail_queue_id, osWaitForever);
		  if (mail2 == NULL)
				{
			     // Alloc failed: bail out so we never spin forever and starve MailOutput
					  break;
				}
			mail2->sendID = 4;			
			sprintf(mail2->payload, "Auto Message #%u for UART2. \n" , UART2_seq_count1);
			mail2->recID = UART_2;
			osMailPut(mail_queue_id, mail2);
		
		break;
				
		case(3):
			UART3_seq_count1 += 1;
		  //sprintf(Text_Buffer, "Periodic Message number %d to User #3.\r\n", UART3_seq_count1);
			//SendText3(Text_Buffer);
		  Mail *mail3;
		  mail3 = (Mail*)osMailAlloc(mail_queue_id, osWaitForever);
		  if (mail3 == NULL)
				{
			     // Alloc failed: bail out so we never spin forever and starve MailOutput
					  break;
				}
			mail3->sendID = 4;			
			sprintf(mail3->payload, "Auto Message #%u for UART3. \n" , UART3_seq_count1);
			mail3->recID = UART_3;
			osMailPut(mail_queue_id, mail3);
		break;
		
		default:
			break;
	
	
	}




} */