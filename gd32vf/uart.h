#ifndef __UART_H__
#define __UART_H__

#if 1==0
ver.25/03/12

UART module for gd32

usage:
  #define USART 0 //USART0
  #define UART_SIZE_PWR 8 // 256-bytes ring buffer
  #include "uart.h"
  ...
  UART_init(USART, 8000000/9600); //Init USART1 (by macro USART)
  UART_puts(USART, __TIME__ " " __DATE__ "\r\n");

usage in another *.c files:
  #define UART_DECLARATIONS 1 //USART1
  #include "uart.h"
  ...
  UART_puts(USART, __TIME__ " " __DATE__ "\r\n");
  

модуль для работы с UART в gd32

Macro-settings:
  USART - UART number: 0, 1, 2, ...
  USART_REMAP - remap number: 0(default), 1, 2, ...
  

#endif

#ifndef NULL
  #define NULL ((void*)0)
#endif

#define _UART_PIN(num,dir) UART ## num ## _ ## dir
#define UART_PIN(num,dir) _UART_PIN(num,dir)
#define _uartN_Dx(num, dir) uart ## num ## _ ## dir
#define uartN_Dx(num, dir) _uartN_Dx(num, dir)
#define _UART(num) USART##num
#define UART(num) _UART(num)
#define _UART_IRQ(num) USART##num##_IRQn
#define UART_IRQ(num) _UART_IRQ(num)
#define _RCC_USARTnEN(num) RCC_USART ## num ## EN
#define RCC_USARTnEN(num) _RCC_USARTnEN(num)

#define _UART_init(num, brr) UART ## num ## _init(brr)
#define _UART_received(num)	(uart_buf_busy(&_uartN_Dx(num,rx)))
#define _UART_avaible(num)  (uart_buf_free(&_uartN_Dx(num,tx)))
#define _UART_str_size(num) (uart_buf_str_size( &_uartN_Dx(num,rx) ) )
#define _UART_getc(num)  	uart_buf_getc(&_uartN_Dx(num,rx))
#define _UART_read(num, data, len) UART##num##_read(data, len)
#define _UART_gets(num, str, len) uart_buf_gets(&_uartN_Dx(num,rx), str, len)
#define _UART_rx_clear(num) do{_uartN_Dx(num,rx).en = _uartN_Dx(num,rx).st;}while(0)
#define _UART_write(num, data, len) UART##num##_write(data, len)
#define _UART_puts(num, str) UART##num##_puts(str)

#define UART_init(num, brr) _UART_init(num, brr)
#define UART_received(num)	_UART_received(num)
#define UART_str_size(num) _UART_str_size(num)
#define UART_avaible(num)  _UART_avaible(num)
#define UART_getc(num)  	_UART_getc(num)
#define UART_read(num, data, len) _UART_read(num, data, len)
#define UART_gets(num, str, len) _UART_gets(num, str, len)
#define UART_rx_clear(num) _UART_rx_clear(num)
#define UART_write(num, data, len) _UART_write(num, data, len)
#define UART_puts(num, str) _UART_puts(num, str)



#define _UARTn_func(n, func) UART ## n ## _ ## func
#define UARTn_func(n, func) _UARTn_func(n, func)

#define UART_putc(num, data) do{\
  uart_buf_putc(&_uartN_Dx(num,tx), data);\
  USART_CTL0(UART(num)) |= USART_CTL0_TBEIE; \
}while(0)




#ifdef USART


#include "pinmacro.h"

#ifndef UART_SIZE_PWR
  #define UART_SIZE_PWR 6
#endif
#if UART_SIZE_PWR < 8
  #define uart_size_t uint8_t
#else
  #define uart_size_t uint16_t
#endif

#define UART_SIZE (1<<UART_SIZE_PWR)
#define UART_MASK (UART_SIZE-1)

typedef struct{
  volatile uart_size_t st,en;
  volatile uint8_t arr[UART_SIZE];
}uart_buffer;

uart_buffer uartN_Dx(USART, rx); //uart1_rx
uart_buffer uartN_Dx(USART, tx); //uart1_tx

static uart_size_t uart_buf_busy(uart_buffer *buf){
  return ((buf->st - buf->en) & UART_MASK);
}
//#define uart_buf_busy(buf) (((buf)->st - (buf)->en) & UART_MASK)
#define uart_buf_free(buf) (UART_SIZE - uart_buf_busy(buf) - 1)

static uint8_t uart_buf_getc(uart_buffer *buf){
  uint8_t res;
  if(uart_buf_busy(buf) == 0)return 0;
  res = buf->arr[buf->en];
  buf->en++;
  buf->en &= UART_MASK;
  return res;
}

static void uart_buf_putc(uart_buffer *buf, uint8_t dat){
  if(uart_buf_free(buf) < 1)return;
  buf->arr[buf->st]=dat;
  buf->st++;
  buf->st &= UART_MASK;
}

static int32_t uart_buf_str_size(uart_buffer *buf){
  uint32_t en = buf->en;
  uint8_t *arr = (uint8_t*)(buf->arr);
  uint32_t sz = (buf->st - en) & UART_MASK;
  uint32_t pos;
  if(sz == UART_SIZE - 1)return -1;
  for(uint32_t i=1; i<sz; i++){
    pos = (i + en) & UART_MASK;
    if((arr[pos] == '\r') || (arr[pos] == '\n'))return i;
  }
  return 0;
}

static char *uart_buf_gets(uart_buffer *buf, char *str, uint32_t len){
  uint32_t en = buf->en;
  uint8_t *arr = (uint8_t*)(buf->arr);
  uint32_t sz = (buf->st - en) & UART_MASK;
  uint32_t strsz;
  uint32_t pos;
  str[0] = 0;
  if(sz > len)sz = len;
  if(sz < 2)return NULL;
  for(strsz = 1; strsz < sz; strsz++){ // find '\r\n' in buffer
    pos = (strsz + en) & UART_MASK;
    if((arr[pos] == '\r') || (arr[pos] == '\n'))break;
  }
  if((arr[pos] != '\r') && (arr[pos] != '\n')){ // '\r\n' not found
    if(strsz < len)return NULL; // if user requests less bytes then return received part
  }
  
  if((arr[en] == '\r') || (arr[en] == '\n')){ //remove '\r\n' from start
    en = (en+1)&UART_MASK;
    strsz--;
  }
  
  for(uint32_t i=0; i<strsz; i++){
    str[i] = arr[ (i+en)&UART_MASK ];
  }
  str[strsz] = 0;
  
  buf->en = (strsz+en) & UART_MASK;
  return str;
}

//UART1_write
void UARTn_func(USART, write)(uint8_t *data, uint32_t len){
  while(len--){
    uart_buf_putc(&uartN_Dx(USART,tx), data[0]);
    data++;
  }
  USART_CTL0(UART(USART)) |= USART_CTL0_TBEIE;
}

//UART1_puts
void UARTn_func(USART, puts)(char *str){
  while(str[0] != 0){
    uart_buf_putc(&uartN_Dx(USART,tx), str[0]);
    str++;
  }
  USART_CTL0(UART(USART)) |= USART_CTL0_TBEIE;
}

//UART1_read
uint8_t* UARTn_func(USART, read)(uint8_t *data, uint32_t len){
  data[0] = 0;
  if(UART_received(USART) < len)return NULL;
  while(len--){
    *(data++) = UART_getc(USART);
  }
  return data;
}

#ifndef USART_REMAP
  #define USART_REMAP 0
#endif

#if USART == 0
  #if USART_REMAP == 0
    #define UART0_TX A,9 ,1,GPIO_APP50
    #define UART0_RX A,10,1,GPIO_HIZ
    #define uart_do_remap() do{ \
      AFIO_PCF0 = (AFIO_PCF0 &~AFIO_PCF0_USART0_REMAP ) | 0*AFIO_PCF0_USART0_REMAP; \
    }while(0)
  #elif USART_REMAP == 1
    #define UART0_TX B,6,1,GPIO_APP50
    #define UART0_RX B,7,1,GPIO_HIZ
    #define uart_do_remap() do{ \
      AFIO_PCF0 = (AFIO_PCF0 &~AFIO_PCF0_USART0_REMAP ) | 1*AFIO_PCF0_USART0_REMAP; \
    }while(0)
  #else
    #error USART0 remap must be defined as 0 (no remap), 1, 2 or 3  
  #endif
#endif

#if USART == 1
  #if USART_REMAP == 0
    #define UART1_TX A,2 ,1,GPIO_APP50
    #define UART1_RX A,3 ,1,GPIO_HIZ
    #define uart_do_remap() do{ \
      AFIO_PCF0 = (AFIO_PCF0 &~AFIO_PCF0_USART1_REMAP ) | 0*AFIO_PCF0_USART1_REMAP; \
    }while(0)
  #elif USART_REMAP == 1
    #define UART1_TX D,5 ,1,GPIO_APP50
    #define UART1_RX D,6 ,1,GPIO_HIZ
    #define uart_do_remap() do{ \
      AFIO_PCF0 = (AFIO_PCF0 &~AFIO_PCF0_USART1_REMAP ) | 1*AFIO_PCF0_USART1_REMAP; \
    }while(0)
  #else
    #error USART1 remap must be defined as 0 or 1
  #endif
#endif

#if USART == 2
  #if USART_REMAP == 0
    #define UART2_TX B,10,1,GPIO_APP50
    #define UART2_RX B,11,1,GPIO_HIZ
    #define uart_do_remap() do{ \
      PM_BITMASK(AFIO_PCF0, AFIO_PCF0_USART2_REMAP, 0); \
    }while(0)
  #elif USART_REMAP == 1
    #define UART2_TX C,10,1,GPIO_APP50
    #define UART2_RX C,11,1,GPIO_HIZ
    #define uart_do_remap() do{ \
      PM_BITMASK(AFIO_PCF0, AFIO_PCF0_USART2_REMAP, 1); \
    }while(0)
  #elif USART_REMAP == 2
    #define UART2_TX D,8,1,GPIO_APP50
    #define UART2_RX D,9,1,GPIO_HIZ
    #define uart_do_remap() do{ \
      PM_BITMASK(AFIO_PCF0, AFIO_PCF0_USART2_REMAP, 3); \
    }while(0)
  #else
    #error USART2 remap must be defined as 0, 1, or 2
  #endif
#endif

//UART1_init
void UARTn_func(USART, init)(uint16_t brr){
  GPIO_config( UART_PIN(USART, RX) );
  GPIO_config( UART_PIN(USART, TX) );
  RCU_APB2EN |= RCU_APB2EN_AFEN;
  if(USART == 0){
    RCU_APB2EN |= RCU_APB2EN_USART0EN;
  }else if(USART == 1){
    RCU_APB1EN |= RCU_APB1EN_USART1EN;
  }else{
    RCU_APB1EN |= RCU_APB1EN_USART2EN;
  }
  uart_do_remap();
  USART_BAUD(UART(USART)) = (brr);
  USART_CTL0(UART(USART)) = USART_CTL0_UEN | USART_CTL0_TEN | USART_CTL0_REN | USART_CTL0_RBNEIE;
  USART_CTL1(UART(USART)) = 0;
  USART_CTL2(UART(USART)) = 0;
  USART_GP(UART(USART)) = 0;
  uartN_Dx(USART,rx).st=0; uartN_Dx(USART,rx).en=0; uartN_Dx(USART,tx).st=0; uartN_Dx(USART,tx).en=0;
  eclic_set_vmode( UART_IRQ(USART) ); \
  eclic_enable_interrupt( UART_IRQ(USART) ); \
}

#define UART_speed(num, brr) do{USART_BAUD(UART(USART)) = (brr);}while(0)
//#define UART_parity_enable(num) do{UART(USART)->CTLR1 |= USART_CTLR1_PCE;}while(0)
//#define UART_parity_disable(num) do{UART(USART)->CTLR1 &=~ USART_CTLR1_PCE;}while(0)
//#define UART_parity_even(num) do{UART(USART)->CTLR1 &=~ USART_CTLR1_PS;}while(0)
//#define UART_parity_odd(num) do{UART(USART)->CTLR1 |= USART_CTLR1_PS;}while(0)
//#define UART_wordlen(num, len) do{if(len == 9)UART(USART)->CTLR1 |= USART_CTLR1_M; else UART(USART)->CTLR1 &=~ USART_CTLR1_M;}while(0)

///////////////////////////////////////////////////////////////////////////////////////////////
//             Interrupt
///////////////////////////////////////////////////////////////////////////////////////////////
#define _uart_IRQ(num) USART ## num ## _IRQHandler
#define uart_IRQ(num) _uart_IRQ(num)
//__attribute__((interrupt)) void USART2_IRQHandler(void){
__attribute__((interrupt)) void uart_IRQ(USART)(void){
  if( USART_STAT(UART(USART)) & USART_STAT_RBNE ){
    uint8_t temp = USART_DATA(UART(USART));
    uart_buf_putc(&uartN_Dx(USART, rx), temp);
  }else if( USART_STAT(UART(USART)) & USART_STAT_TBE ){
    if(uart_buf_busy(&uartN_Dx(USART, tx)) != 0){
      USART_DATA(UART(USART)) = uart_buf_getc(&uartN_Dx(USART, tx));
    }else{
      USART_CTL0(UART(USART)) &=~ USART_CTL0_TBEIE;
    }
  }
}

#elif defined UART_DECLARATIONS

//UART1_write
void UARTn_func(UART_DECLARATIONS, write)(uint8_t *data, uint32_t len);
//UART1_puts
void UARTn_func(UART_DECLARATIONS, puts)(char *str);
//UART1_read
void UARTn_func(UART_DECLARATIONS, read)(uint8_t *data, uint32_t len);
//UART1_gets
void UARTn_func(UART_DECLARATIONS, gets)(char *str, uint32_t len);
//UART1_init
void UARTn_func(UART_DECLARATIONS, init)(uint16_t brr);

#else

  #error define either USART or UART_DECLARATIONS macro

#endif

#endif
