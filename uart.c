#include <stm32F10x.h>

/*----------------------------------------------------------------------------
  Initialize UART pins, Baudrate
 *----------------------------------------------------------------------------*/
void USART1_Init (void) {
  int i;

  RCC->APB2ENR |=  (   1UL <<  0);        /* enable clock Alternate Function  */
  AFIO->MAPR   &= ~(   1UL <<  2);        /* clear USART1 remap               */

  RCC->APB2ENR |=  (   1UL <<  2);        /* enable GPIOA clock               */
  GPIOA->CRH   &= ~(0xFFUL <<  4);        /* clear PA9, PA10                  */
  GPIOA->CRH   |=  (0x0BUL <<  4);        /* USART1 Tx (PA9) output push-pull */
  GPIOA->CRH   |=  (0x04UL <<  8);        /* USART1 Rx (PA10) input floating  */

  RCC->APB2ENR |=  (   1UL << 14);        /* enable USART#1 clock             */

  USART1->BRR   = 0x0271;                 /* 115200 baud @ PCLK2 72MHz        */
  USART1->CR1   = ((   1UL <<  2) |       /* enable RX                        */
                   (   1UL <<  3) |       /* enable TX                        */
                   (   0UL << 12) );      /* 1 start bit, 8 data bits         */
  USART1->CR2   = 0x0000;                 /* 1 stop bit                       */
  USART1->CR3   = 0x0000;                 /* no flow control                  */
  for (i = 0; i < 0x1000; i++) __NOP();   /* avoid unwanted output            */

  USART1->CR1  |= ((   1UL << 13) );      /* enable USART                     */
}


void USART2_Init (void){
    int i;
 
    AFIO->MAPR |= 1<<3;           // set USART2 remap
    RCC->APB2ENR |= 1<<5;          // enable clock for GPIOD
    GPIOD->CRL &= ~(0xFF << 20);       // Clear PD5, PD6
    GPIOD->CRL |= (0x0B << 20);       // USART2 Tx (PD5) output push-pull
    GPIOD->CRL |= (0x04 << 24);       // USART2 Rx (PD6) input floating
    RCC->APB1ENR |= 1<<17;          // enable clock for USART2
    USART2->BRR   = 0x0271;                 /* 115200 baud @ PCLK2 72MHz        */
    USART2->CR1   = ((   1UL <<  2) |    //Enable RX
                   (   1UL <<  3) |    //Enable TX
                   (   0UL << 13) );    //1 start bit, 8 data bits
    USART2->CR2 &= ~(3<<12);         //force 1 stop bit
    USART2->CR3 &= ~(3<<8);         //force no flow control
    //USART2->CR1 &= ~(3<<9);        //force no parity
    //USART2->CR1 |= 3<<2;          // RX, TX enable
    //USART2->CR1 |= 1<<5;          // Rx interrupts if required
    NVIC->ISER[1] = (1 << 6);        // enable interruptsif required
    for (i = 0; i < 0x1000; i++) __NOP();   /* avoid unwanted output            */
    USART2->CR1 |= 1<<13;          // USART enable
}


/*----------------------------------------------------------------------------
  Initialize UART3 pins, Baudrate
 *----------------------------------------------------------------------------*/
void USART3_Init (void) {

    RCC->APB2ENR |= 1;                                  // enable clock for AF
    AFIO->MAPR |= AFIO_MAPR_USART3_REMAP_PARTIALREMAP;  // set USART3 to partical remap to use PC10 and PC11
    RCC->APB2ENR |= 1<<4;                               // enable clock for GPIOC
    
    // since our pins are above 8, we use H instead of L
    // since we are using 10 & 11 we will be using bits 8-15 in the CRH register
    /* USART3 Tx (PC10) output push-pull */             
    GPIOC->CRH   &= ~(0xFFUL <<  8);        
    GPIOC->CRH   |=  (0x0BUL <<  8);       
    /* USART3 Rx (PC11) input floating  */
    GPIOC->CRH   |=  (0x04UL <<  12);     
    
    RCC->APB1ENR |=  RCC_APB1ENR_USART3EN; /* enable clock for USART3         */
    USART3->BRR = 0x138;                  /* set baudrate -115.2kB from 36MHz */
    
    USART3->CR1   = ((   1UL <<  2) |     /* enable RX                        */
                   (   1UL <<  3) |       /* enable TX                        */
                   (   0UL << 12) );      /* 1 start bit, 8 data bits         */
    
    USART3->CR2   = 0x0000;                 /* 1 stop bit                       */
    USART3->CR3   = 0x0000;                 /* no flow control                  */
    
    USART3->CR1 |= 1<<13;                 /* USART3 enable                    */

}

/*----------------------------------------------------------------------------
  SendChar1
  Write character to Serial Port.
 *----------------------------------------------------------------------------*/
int SendChar1 (uint8_t ch)  {

  while (!(USART1->SR & USART_SR_TXE));
  USART1->DR = ((uint16_t)ch & 0x1FF);

  return (ch);
}


/*----------------------------------------------------------------------------
  SendChar2
  Write character to Serial Port.
 *----------------------------------------------------------------------------*/
int SendChar2 (uint8_t ch)  {

  while (!(USART2->SR & USART_SR_TXE));
  USART2->DR = ((uint16_t)ch & 0x1FF);

  return (ch);
}

/*----------------------------------------------------------------------------
  SendChar2
  Write character to Serial Port.
 *----------------------------------------------------------------------------*/
int SendChar3 (uint8_t ch)  {

  while (!(USART3->SR & USART_SR_TXE));
  USART3->DR = ((uint16_t)ch & 0x1FF);

  return (ch);
}

/*----------------------------------------------------------------------------
  GetKey1
  Read character to Serial Port.
 *----------------------------------------------------------------------------*/
uint8_t GetKey1 (void)  {

  while (!(USART1->SR & USART_SR_RXNE));

  return ((uint8_t)(USART1->DR & 0x1FF));
}


/*----------------------------------------------------------------------------
  GetKey2
  Read character to Serial Port.
 *----------------------------------------------------------------------------*/
uint8_t GetKey2 (void)  {

  while (!(USART1->SR & USART_SR_RXNE));

  return ((uint8_t)(USART1->DR & 0x1FF));
}