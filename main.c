/*******************************************************************************
 *
 *              Authors: Pablo Moreno & Jeremy Gooch
 *              Last modified: 05-26-2026 @  11:55 p.m.
 *
 * Purpose:
 *
 * This program is modified code based on the Designers Guide to the Cortex-M
 * Family CMSIS-RTOS examples. It implements a PWM signal generator and a
 * Sawtooth waveform generator using CMSIS-RTOS threads on the STM32F103RB.
 *
 * Description:
 *
 * CMSIS-RTOS PWM and Sawtooth Signal Generator for STM32F103RB.
 * This application uses three RTOS threads to handle UART input, PWM control,
 * and Sawtooth waveform generation. UART interrupts capture user commands and
 * signal an input handler thread, which then sends messages to the correct
 * control thread to adjust the Duty cycle of the PWM or modify the amplitude
 * of the sawtooth waveform. 
 *
 *  Details:
 * - Input Handler Thread : Waits for UART interrupt and routes the traffic to
 *                          the appropriate Queue, ignoring invalid input.
 * - PWM Thread           : Modifies the PWM duty cycle using 'D' and 'd'.
 * - Sawtooth Thread      : Generates a Sawtooth waveform with 10 ms sample time,
 *                          amplitude adjusted with 'A' or 'a'.
 *
 * *UART Commands:
 *
 * - 'D' : Increases PWM duty cycle by 10%.
 * - 'd' : Decreases PWM duty cycle by 10%.
 * - 'A' : Increases Sawtooth amplitude by 100.
 * - 'a' : Decreases Sawtooth amplitude by 100.
 * - all other input - ignored.
 *
 * *Signal Behavior:
 *
 * - PWM signal is generated using LED on/off timing.
 * - Sawtooth signal increments by 10 every 10 ms.
 * - Sawtooth resets to 0 when it reaches the current amplitude.
 *
 * *Sequence Flow:
 * [USART1 IRQ] -> Input Handler Thread -> PWM Thread / Sawtooth Thread
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

//setup and define message queues
osMessageQId Q_PWM;         
osMessageQId Q_Sawtooth;    

osMessageQDef (Q_PWM,0x16,unsigned char);
osMessageQDef (Q_Sawtooth, 0x16, unsigned char);

//osEvents
osEvent  PWM_result;
osEvent  SawTooth_result;


//Setup and define threads and priority levels
void InputHandler_Thread (void const *argument);
void PWM_Thread (void const *argument);
void Sawtooth_Thread (void const *argument);

osThreadDef(InputHandler_Thread, osPriorityAboveNormal, 1, 0);
osThreadDef(PWM_Thread, osPriorityNormal, 1, 0);
osThreadDef(Sawtooth_Thread, osPriorityNormal, 1, 0);

//Thread IDs
osThreadId T_PWM;
osThreadId T_SawTooth;
osThreadId T_main;
osThreadId T_Handler;

//Variable declarations
volatile uint32_t Duty_Cycle_H = 500;
volatile uint32_t Duty_Cycle_L = 500;
volatile uint8_t Text_Buffer[64];
volatile uint8_t  intKey;
volatile int16_t  amplitude = 100;
volatile uint16_t  Sawtooth_out = 0;

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
 *              update the appropriate message queue for the PWM or SawTooth
 *							thread with valid input ('A', 'a', 'D', 'd'). Any other input
 *							will not impact tasking or thread operation.
 *---------------------------------------------------------------------------*/
void InputHandler_Thread (void const *argument) 
{
	for (;;) 
	{
		osSignalWait (0x01,osWaitForever);

		//check for valid input and update queues accordingly	
		if((intKey == 'A') || (intKey == 'a'))
	    {
	      osMessagePut(Q_Sawtooth,intKey, 0);
	    }
	  else if((intKey == 'D') || (intKey == 'd'))
	    {
		    osMessagePut(Q_PWM,intKey, 0);
	    }	
    //if invalid input, no action performed
	}

}

/*----------------------------------------------------------------------------
 * PWM_Thread:
 * 
 * Description: This thread is responsible for the PWM output by toggling an
 *              LED high and low depending on the duty cycle, and changing the
 *              Duty cycle based on user input - incrementing or decrementing
 *              by 10% clamping to percentage high/low bounds when needed. 
 *---------------------------------------------------------------------------*/
void PWM_Thread (void const *argument) 
{
	//local float variable for output to UART window
	float PWM_OutputUART;

	for (;;) 
	{
	  //check message queue
	  PWM_result = osMessageGet(Q_PWM, 0);
		
		//if there is a new message:
		if(PWM_result.status == osEventMessage)
		  {
		    //see if the input requires action and ensure clamping 
        //to range, increment or decrement as appropriate.				
		    if((PWM_result.value.v == 'D') && (Duty_Cycle_H != 1000))
		      {
			      Duty_Cycle_H = (Duty_Cycle_H + 100);
			      Duty_Cycle_L = 1000 - Duty_Cycle_H;
						
			      //calculate and format float value for UART output
			      PWM_OutputUART = Duty_Cycle_H/1000.00f;
			      
						//use sprintf function to put in format to be sent
						//through the SendText function.
			      sprintf(Text_Buffer, "PWM Thread - increased duty cycle to %0.2f \r\n", PWM_OutputUART);
			
						//use mutex to ensure reliable transmission of data to UART window
		        osMutexWait(uart_mutex, osWaitForever);
		
		        SendText(Text_Buffer);
		
		        osMutexRelease(uart_mutex);			
      
          }				 
		    else if((PWM_result.value.v == 'd') && (Duty_Cycle_H != 0))
		      {
		        Duty_Cycle_H = (Duty_Cycle_H - 100);
			      Duty_Cycle_L = (1000 - Duty_Cycle_H);

			      PWM_OutputUART = Duty_Cycle_H/1000.00f;
			
			      sprintf(Text_Buffer, "PWM Thread - decreased duty cycle to %0.2f \r\n", PWM_OutputUART);
			
		        osMutexWait(uart_mutex, osWaitForever);
		
		        SendText(Text_Buffer);
		
		        osMutexRelease(uart_mutex);		
		      }
 	      }
			/* Toggle output waveform according to duty cycle -
		     if Duty cycle is 100%, keep LED output HIGH,
			   if Duty cycle is 0%, keep LED output LOW,
				 otherwise, toggle according to duty cycle  */
		  if(Duty_Cycle_H == 1000)
			{ LED_On(0);
			  osDelay(10);}
		  else if(Duty_Cycle_L == 1000)
			{ LED_Off(0);
			  osDelay(10);}
		  else
		    {
		      LED_On(0);
		      osDelay(Duty_Cycle_H);
		      LED_Off(0);
	        osDelay(Duty_Cycle_L);
		    }
	  }

}

/*----------------------------------------------------------------------------
 * Sawtooth_Thread:
 * 
 * Description: This thread is responsible for generating a sawtooth waveform
 *              it receives information on user input via the message queue
 *              and updates the amplitude of the sawtooth waveform accordingly.
 *              initially, the sawtooth waveform will have an amplitude of 100,
 *              depending on user input, it can be varied from 0 to 1000 by 
 *              increments of 100.
 * 
 *---------------------------------------------------------------------------*/
void Sawtooth_Thread (void const *argument) 
{
	for (;;) 
	{
		//Increment the Sawtooth by 10 each time task runs
		Sawtooth_out = Sawtooth_out+10; 
		//reset to 0 when max is reached
		if(Sawtooth_out >= amplitude)
		{
		  Sawtooth_out = 0;
		}
		//check message queue
		SawTooth_result = osMessageGet(Q_Sawtooth, 0);
		
		if(SawTooth_result.status == osEventMessage)
		  {
			
		    if((SawTooth_result.value.v == 'A') && (amplitude != 1000))
		      {
			      amplitude += 100;
				
			      sprintf(Text_Buffer, "SawTooth Thread - increased Amplitude to %d \r\n", amplitude);
			
		        osMutexWait(uart_mutex, osWaitForever);
		
		        SendText(Text_Buffer);
		
		        osMutexRelease(uart_mutex);			
		        }
		    else if((SawTooth_result.value.v == 'a') && (amplitude != 0))
		      {
            amplitude -= 100;		
				
			      sprintf(Text_Buffer, "SawTooth Thread - decreased Amplitude to %d \r\n", amplitude);
			
		        osMutexWait(uart_mutex, osWaitForever);
		
		        SendText(Text_Buffer);
		
		        osMutexRelease(uart_mutex);	

		      }
 	    }
		
    osDelay(10);
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
	
	LED_Initialize ();
	
	//create the message queues
	Q_PWM = osMessageCreate(osMessageQ(Q_PWM),NULL);					
	Q_Sawtooth = osMessageCreate(osMessageQ(Q_Sawtooth),NULL);
	
	//create mutex object
  uart_mutex = osMutexCreate(osMutex(uart_mutex));
	
	//Create Threads
	T_Handler = osThreadCreate(osThread(InputHandler_Thread), NULL);
	T_PWM = osThreadCreate(osThread(PWM_Thread), NULL);
	T_SawTooth =	osThreadCreate(osThread(Sawtooth_Thread), NULL);
	
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
