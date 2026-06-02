/*******************************************************************************
 *
 *              Authors: Pablo Moreno & Jeremy Gooch
 *              Last modified: 05-26-2026 @  11:55 p.m.
 *
 * Purpose:
 *
 * This program 
 *
 * Description:
 *
 * CMSIS-RTOS for STM32F103RB.
 * This application uses three RTOS threads to handle UART input
 *
 *  Details:
 * - Input Handler Thread : Waits for UART interrupt and routes the traffic to
 *                          the appropriate Queue, ignoring invalid input.
 * *UART Commands:
 *
 * - 'Enter' Allocates the store data of the buffer into the mail queue
 *
 * *Sequence Flow:
 * [USART1 IRQ] -> 
 *
 * *Board:
 * STM32F103RB Development Board
 *
 *******************************************************************************/
 
#include "STM32F10x.h"
#include "cmsis_os.h"
#include "Board_LED.h"
#include "uart.h"
#include <math.h>
#include <stdlib.h>
#include <string.h>

//Size message limits for message mail 
#define MAX_MAIL_PAYLOAD 32
#define LOCAL_BUF_SIZE 128 // A large temporary buffer to hold input until 'Enter' is pressed
#define CHUNK_SIZE     (MAX_MAIL_PAYLOAD - 1) // 31 characters + 1 for '\0'

/* ANSI Color Escape Codes
 * NOTE: The Keil uVision simulator UART window does NOT render ANSI escape
 * codes (they show up as stray characters and corrupt the output). They are
 * disabled (empty) for simulator testing. Re-enable the real codes below when
 * driving a true ANSI terminal (Tera Term / PuTTY) in the dual-UART phase. */
#define CLR_RESET   ""  // "\033[0m"
#define CLR_TX      ""  // "\033[1;32m" Bold Green for Transmitted messages
#define CLR_RX      ""  // "\033[1;36m" Bold Cyan for Received messages
#define CLR_SYSTEM  ""  // "\033[1;31m" Bold Red for system alerts/errors


//This structu is for mail box and message handler 
typedef struct {
    char payload[MAX_MAIL_PAYLOAD];
    uint8_t is_fragmented; // 0 = Normal, 1 = Part of a split message
    uint8_t source_uart; // 1 or 2, so the Tx thread knows where it came from
} mail_t;



//Setup for Mail Queue 
osMailQDef(mail_queue, 10, mail_t); // Define a mail queue of 10 blocks

//osEvents

//Setup and define threads and priority levels
void InputHandler_Thread (void const *argument);
void MailOutput_Thread (void const *argument);
// Thread definitions 
osThreadDef(InputHandler_Thread, osPriorityAboveNormal, 1, 0); // stack from OS_STKSIZE (raised in RTX_Conf_CM.c)
osThreadDef(MailOutput_Thread, osPriorityNormal, 1, 0);

//Thread IDs
osThreadId T_main;
osThreadId T_Handler;
osThreadId T_Mail;

// Mail Q Id
osMailQId  mail_queue_id;

//Variable declarations
volatile uint8_t Text_Buffer[64];
volatile uint8_t  intKey;

//function prototype
void SendText(uint8_t *txt);

//add mutex capability
osMutexId uart_mutex;
osMutexDef(uart_mutex);

/*----------------------------------------------------------------------------
 * InputHandler_Thread:
 *
 * Description: This thread has a "higher than normal" priority, allowing
 *              It to execute anytime there is user input to UART #1. It will
 *              update the appropriate message queue 
 *---------------------------------------------------------------------------*/
void InputHandler_Thread (void const *argument) 
{
	char local_buffer[LOCAL_BUF_SIZE];
	uint16_t index = 0;
	uint16_t i;
	
	for (;;) 
	{
		// Wait for the interrupt handler to signal a new character in intKey
		osSignalWait (0x01,osWaitForever);
		char ch = (char)intKey;
		
		// 1. Echo the character back to the local terminal using SendChar.
		//    On Enter, emit a full CR+LF so the cursor drops to a fresh line
		//    instead of overwriting what the user just typed.
		osMutexWait(uart_mutex, osWaitForever);
		if (ch == '\r' || ch == '\n') {
			SendChar('\r');
			SendChar('\n');
		} else {
			SendChar(ch);
		}
		osMutexRelease(uart_mutex);
		
		// 2. Check for End of Message (User pressed Enter / Carriage Return)
		if (intKey == '\r' || intKey == '\n'){
			local_buffer[index] = '\0'; // we force a Null to terminate the string
		
			if (index > 0){ // Only send if the user actually typed something
				uint16_t remaining_bytes = index;
				uint16_t buffer_ptr = 0;
				// set a flag if the message is bigger than 32 characters 
				uint8_t fragmented_flag = (remaining_bytes > CHUNK_SIZE) ? 1 : 0;
				
				// Alert the user on their local screen that fragmentation is happening
				if (fragmented_flag){
					osMutexWait(uart_mutex, osWaitForever);
					SendText((uint8_t *)(CLR_SYSTEM "\r\n[System: Message too long. Segmenting into packets...]\r\n" CLR_RESET));
					osMutexRelease(uart_mutex);
				}
					
				// --- FRAGMENTATION LOOP ---
				while (remaining_bytes > 0){ /// once we reach the limit of 32 bytes we allocate the message on the mail. 
					// Allocate RTX Mail, populate it with data and send it 
					mail_t *mail;
					mail = (mail_t*)osMailAlloc(mail_queue_id, osWaitForever);
					if (mail == NULL){
						// Alloc failed: bail out so we never spin forever and starve MailOutput
						break;
					}
					{
						//  allocates a mail slot and fill it with data
						mail->source_uart = 1; // Originating from UART1
						mail->is_fragmented = fragmented_flag;
						
						// Calculate how many characters fit in this specific mail slice
						uint16_t bytes_to_copy = (remaining_bytes > CHUNK_SIZE) ? CHUNK_SIZE : remaining_bytes;
						//Fragmentate the message 
						for (i = 0; i < bytes_to_copy; i++){
							mail->payload[i] = local_buffer[buffer_ptr + i];
						}
						
						mail->payload[bytes_to_copy] = '\0'; // Explicitly force null-termination              
						// Ship it to the Mail Queue
						osMailPut(mail_queue_id, mail);
						// Shift tracking pointers forward
						buffer_ptr += bytes_to_copy;
						remaining_bytes -= bytes_to_copy;
					}
				}
			}
			
			index = 0; // Clear index tracking for the next fresh message
			} 
		else{
			// 3. Accumulate normal typing into buffer safely avoiding memory overflow
			if (index < LOCAL_BUF_SIZE - 1) {
					local_buffer[index++] = ch;
			}
		}
	}
}

/*----------------------------------------------------------------------------
 * MailOutput_Thread:
 *
 * Description: Monitored mail queue output. Receives chunks, applies text 
 * color formats, and outputs them to the respective destination.
 *---------------------------------------------------------------------------*/
void MailOutput_Thread (void const *argument) 
{
    for (;;) 
    {
        // Block until mail arrives from the queue
        osEvent evt = osMailGet(mail_queue_id, osWaitForever);
        
        if (evt.status == osEventMail) 
        {
            mail_t *mail = (mail_t*)evt.value.p;
            
            osMutexWait(uart_mutex, osWaitForever);
            
            // Set the incoming text color to Cyan for clear tracking
            //SendText((uint8_t *)CLR_RX);
            
            // Print out the payload piece
            SendText((uint8_t *)mail->payload);
            
            if (mail->is_fragmented == 0) 
            {
                // This was a complete, standard message
                // Append newline and reset formatting colors
                SendText((uint8_t *)("\r\n" CLR_RESET));
            }
            else 
            {
                // If it's a fragment, we intentionally skip printing "\r\n" 
                // so the next fragment appends right next to it!
                SendText((uint8_t *)CLR_RESET);
            }
            
            osMutexRelease(uart_mutex);
            
            // Recycle the mail memory block back to RTX Pool
            osMailFree(mail_queue_id, mail);
        }
    }
}


/*----------------------------------------------------------------------------
  Main: Initialize and start RTX Kernel
 *---------------------------------------------------------------------------*/

int main (void) 
{
	osKernelInitialize ();                    						// initialize CMSIS-RTOS
	
	USART1_Init (); 
  //configure USART interrupt ... so we can read user inputs using interrupt 
  //Configure and enable USART1 interrupt 
  NVIC->ICPR[USART1_IRQn/32] = 1UL << (USART1_IRQn%32);  //clear any previous pending interrupt flag 
  NVIC->IP[USART1_IRQn] = 0x80; // NVIC_SetPriority(USART1_IRQn, 0x80); //set priority to 0x80  
  NVIC->ISER[USART1_IRQn/32] = 1UL << (USART1_IRQn%32); //set interrupt enable bit  
  USART1->CR1 |= USART_CR1_RXNEIE; //enable USART receiver not empty interrupt 
	
	//LED_Initialize ();
	
	//create the message queues
	
	//create mutex object
  uart_mutex = osMutexCreate(osMutex(uart_mutex));
	// Create mail queue in thread 
	mail_queue_id = osMailCreate(osMailQ(mail_queue), NULL);
	
	//Create Threads
	T_Handler = osThreadCreate(osThread(InputHandler_Thread), NULL);
	T_Mail = osThreadCreate(osThread(MailOutput_Thread), NULL);
	osKernelStart ();                         						// start thread execution 
	
	//terminate the main thread to free up resources
	T_main = osThreadGetId (); 
  osThreadTerminate(T_main);
}

/*----------------------------------------------------------------------------
USART1_IRQHandler: This is the IRQ handler for UART #1 input.
*---------------------------------------------------------------------------*/
void USART1_IRQHandler (void)
{ 
  intKey = (int8_t) (USART1->DR & 0x1FF); 

  //Send signal to the handler thread that there is user input from UART
  osSignalSet(T_Handler,0x01);
} 

/*----------------------------------------------------------------------------
SendText: This function receives a pointer to a text string and parses the
          chars one at a time and sends to the UART output window.
*---------------------------------------------------------------------------*/
void SendText(uint8_t *text)
{
  char currChar;
  currChar = *text;
    
  while(currChar != '\0')
		{
		  SendChar(currChar);
      text++;
      currChar = *text;
	  }
}
