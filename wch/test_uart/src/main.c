#include "ch32v30x.h"
#include "hardware.h"
#include "pinmacro.h"
#define USART 1
#define UART_SIZE_PWR 6
#include "uart.h"

#ifndef NULL
  #define NULL ((void*)0)
#endif
void SystemInit(void){}
void sleep(uint32_t t){while(t--)asm volatile("nop");}


void uart_u32(uint32_t val){
  char buf[11];
  char *ch = &(buf[10]);
  ch[0] = 0;
  do{
    ch--;
    ch[0] = (val % 10) + '0';
    val /= 10;
  }while(val);
  UART_puts(USART, ch);
}

int main(){
  RCC->APB2PCENR |= RCC_IOPAEN | RCC_IOPBEN | RCC_IOPCEN | RCC_AFIOEN;
  GPIO_config(RLED); GPIO_config(GLED); GPIO_config(YLED);
  GPO_OFF(RLED); GPO_OFF(GLED); GPO_OFF(YLED);
  
  UART_init(USART, 8000000 / 9600);
  
  UART_puts(USART, "\r\n\r\n");
  UART_puts(USART, __TIME__ " " __DATE__ "\r\n");
  UART_puts(USART, "Type 'Help'\r\n");
  
  char buf[20];
  while(1){
    GPO_T(YLED);
    sleep(100000);

    int32_t sz = UART_str_size(USART);
    if(sz > 0){
      GPO_T(GLED);
      UART_gets(USART, buf, sizeof(buf));
      if(buf[0] == 'H' && buf[1] == 'e' && buf[2] == 'l' && buf[3] == 'p'){
        UART_puts(USART, "Demo 'help' command. Another command does not supported\r\n");
      }else{
        UART_puts(USART, "Wrong command [");
        UART_puts(USART, buf);
        UART_puts(USART, "] (");
        uart_u32(sz);
        UART_puts(USART, " characters). Type 'Help'\r\n");
      }
    }else if(sz < 0){
      UART_puts(USART, "String too long\r\n");
      UART_rx_clear(USART);
    }
  }
}