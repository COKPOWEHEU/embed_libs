#ifndef __UART_H__
#define __UART_H__

#if 1==0
ver.19.03.13

модуль для работы с UART в ch32vf103

макронастройки:
  USART0_EN, UART1_EN, UART2_EN - какие UARTы включать
  USART - если используется только один UART, достаточно указать его номер. Его же потом можно использовать в других макросах
  F_APB1, F_APB2 - тактовая частота соответствующей шины
  
макросы и функции:
  void UART_init(int num, uint32_t baud)
  
  void UART_putc(int num, uint8_t data)
  void UART_write(int num, uint8_t data, uint8_t len)
  void UART_puts(int num, char* str)
  uart_size_t UART_tx_count(int num)
  
  uint8_t UART_getc(int num)
  void UART_read(int num, uint8_t data, uint8_t len)
  void UART_gets(int num, char* str, uint8_t len)
  uint8_t UART_scan(int num)
  uart_size_t UART_rx_count(int num)
где
  num - номер UART (1 - 3)
  data - байт данных для приема / передачи
  str - строка для приема / передачи
  len - (максимальное) количество байт для приема / передачи
#endif
//TODO: проверить чтение
//TODO: настроить ремап портов

#include "pinmacro.h"

#ifndef UART_SIZE_PWR
  #define UART_SIZE_PWR 6
#endif
#if UART_SIZE_PWR < 8
  #define uart_size_t uint8_t
#else
  #define uart_size_t uint16_t
#endif

#if (USART==0)
  #define UART0_EN
#elif (USART==1)
  #define UART1_EN
#elif (USART==2)
  #define UART2_EN
#endif

#if (!defined(UART0_EN)) && (!defined(UART1_EN)) && (!defined(UART2_EN))
  #error define either UART0_EN of UART1_EN or UART2_EN
#endif

#ifndef UART0_REMAP
  #define UART0_TX A,9 ,1,GPIO_APP50
  #define UART0_RX A,10,1,GPIO_HIZ
#else
  #define UART0_TX B,6 ,1,GPIO_APP50
  #define UART0_RX B,7 ,1,GPIO_HIZ
#endif

#define UART1_TX A,2 ,1,GPIO_APP50
#define UART1_RX A,3 ,1,GPIO_HIZ

#define UART2_TX B,10,1,GPIO_APP50
#define UART2_RX B,11,1,GPIO_HIZ

#define UART_SIZE (1<<UART_SIZE_PWR)
#define UART_MASK (UART_SIZE-1)

typedef struct{
  volatile uart_size_t st,en;
  volatile uint8_t arr[UART_SIZE];
}uart_buffer;

uart_size_t uart_buf_size(uart_buffer *buf){
  return ((buf->st - buf->en) & UART_MASK);
}

uint8_t uart_buf_read(uart_buffer *buf){
  uint8_t res;
  if(uart_buf_size(buf) == 0)return 0;
  res = buf->arr[buf->st];
  buf->st++;
  buf->st &= UART_MASK;
  return res;
}

void uart_buf_write(uart_buffer *buf, uint8_t dat){
  if(uart_buf_size(buf)!=1){
    buf->arr[buf->en]=dat;
    buf->en++; buf->en &= UART_MASK;
  }
}

#define UART_PIN(num,dir) UART##num##_##dir
#define UART_buf(num,dir) uart##num##_##dir
#define UART(num) USART##num
#define UART_IRQ(num) USART##num##_IRQn

#define _UART_rx_count(num)	((UART_SIZE - uart_buf_size(&UART_buf(num,rx))) & UART_MASK)
#define _UART_tx_count(num)  (UART_MASK - uart_buf_size(&UART_buf(num,tx)))
#define _UART_getc(num)  	uart_buf_read(&UART_buf(num,rx))
#define _UART_scan(num)      (UART_buf(num,rx).arr[UART_buf(num,rx).st])
#define _UART_write(num, data, len) UART##num##_write(data, len)
#define _UART_puts(num, str) UART##num##_puts(str)
#define _UART_read(num, data, len) UART##num##_read(data, len)
#define _UART_gets(num, str, len) UART##num##_gets(str, len)

#define UART_rx_count(num)	_UART_rx_count(num)
#define UART_tx_count(num)  _UART_tx_count(num)
#define UART_getc(num)  	_UART_getc(num)
#define UART_scan(num)      _UART_scan(num)
#define UART_write(num, data, len) _UART_write(num, data, len)
#define UART_puts(num, str) _UART_puts(num, str)
#define UART_read(num, data, len) _UART_read(num, data, len)
#define UART_gets(num, str, len) _UART_gets(num, str, len)

#define UART_putc(num, data) do{\
  uart_buf_write(&UART_buf(num,tx), data);\
  /*UART(num)->CR1 |= USART_CTL0_TBEIE;*/\
  USART_CTL0(UART(num)) |= USART_CTL0_TBEIE; \
}while(0)


#define UART_speed(num, baud) do{ \
  if(num == 0){ \
    USART_BAUD(USART0) = F_APB2 / (baud); \
  }else{ \
    USART_BAUD(UART(num)) = F_APB1 / (baud);\
  } \
}while(0)

#define UART_init(num, baud) do{\
  GPIO_config( UART_PIN(num,RX) ); \
  GPIO_config( UART_PIN(num,TX) ); \
  RCU_APB2EN |= RCU_APB2EN_AFEN;\
  if(num == 0){\
    RCU_APB2EN |= RCU_APB2EN_USART0EN;\
  }else if(num == 1){ \
    RCU_APB1EN |= RCU_APB1EN_USART1EN;\
  }else{ \
    RCU_APB1EN |= RCU_APB1EN_USART1EN;\
  }\
  UART_speed(num, baud); \
  USART_CTL0(UART(num)) = USART_CTL0_UEN | USART_CTL0_TEN | USART_CTL0_REN | USART_CTL0_RBNEIE;\
  USART_CTL1(UART(num)) = 0;\
  USART_CTL2(UART(num)) = 0;\
  USART_GP(UART(num)) = 0;\
  UART_buf(num,rx).st=0; UART_buf(num,rx).en=0; UART_buf(num,tx).st=0; UART_buf(num,tx).en=0;\
  eclic_set_vmode( UART_IRQ(num) ); \
  eclic_enable_interrupt( UART_IRQ(num) ); \
}while(0)

///////////////////////////////////////////////////////////////////////////////////////////////
//             UART_0
///////////////////////////////////////////////////////////////////////////////////////////////
#ifdef UART0_EN
static uart_buffer uart0_rx;
static uart_buffer uart0_tx;

void USART0_IRQHandler(void){
  if( USART_STAT(USART0) & USART_STAT_RBNE ){
    uint8_t temp = USART_DATA(USART0);
    uart_buf_write(&uart0_rx, temp);
  }else if( USART_STAT(USART0) & USART_STAT_TBE ){
    if(uart_buf_size(&uart0_tx) != 0)USART_DATA(USART0) = uart_buf_read(&uart0_tx);
      else USART_CTL0(USART0) &=~ USART_CTL0_TBEIE;
  }
}

void UART0_write(uint8_t *data, uint8_t len){
  while(len--)UART_putc(0, *(data++));
}

void UART0_puts(char *str){
  while(str[0] != 0)UART_putc(0, *(str++));
}

void UART0_read(uint8_t *data, uint8_t len){
  while(len--){
    while(UART_rx_count(0) == 0){}
    *(data++) = UART_getc(0);
  }
}

void UART0_gets(char *str, uint8_t len){
  while(len--){
    while(UART_rx_count(0) == 0){}
    str[0] = UART_getc(0);
    if(str[0] == 0 || str[0] == 13)break;
    str++;
  }
  if(str[0] != 0){
    if(len < 3)str[0] = 0;
      else{ str[0] = 0x0A; str[1] = 0x0D; str[2] = 0; }
  }
}
#endif


///////////////////////////////////////////////////////////////////////////////////////////////
//             UART_1
///////////////////////////////////////////////////////////////////////////////////////////////
#ifdef UART1_EN
static uart_buffer uart1_rx;
static uart_buffer uart1_tx;

void USART1_IRQHandler(void){
  if( USART_STAT(USART1) & USART_STAT_RBNE ){
    uint8_t temp = USART_DATA(USART1);
    uart_buf_write(&uart1_rx, temp);
  }else if( USART_STAT(USART1) & USART_STAT_TBE ){
    if(uart_buf_size(&uart1_tx) != 0)USART_DATA(USART1) = uart_buf_read(&uart1_tx);
      else USART_CTL0(USART1) &=~ USART_CTL0_TBEIE;
  }
}

void UART1_write(uint8_t *data, uint8_t len){
  while(len--)UART_putc(1, *(data++));
}

void UART1_puts(char *str){
  while(str[0] != 0)UART_putc(1, *(str++));
}

void UART1_read(uint8_t *data, uint8_t len){
  while(len--){
    while(UART_rx_count(1) == 0){}
    *(data++) = UART_getc(1);
  }
}

void UART1_gets(char *str, uint8_t len){
  while(len--){
    while(UART_rx_count(1) == 0){}
    str[0] = UART_getc(1);
    if(str[0] == 0 || str[0] == 13)break;
    str++;
  }
  if(str[0] != 0){
    if(len < 3)str[0] = 0;
      else{ str[0] = 0x0A; str[1] = 0x0D; str[2] = 0; }
  }
}
#endif


///////////////////////////////////////////////////////////////////////////////////////////////
//             UART_2
///////////////////////////////////////////////////////////////////////////////////////////////
#ifdef UART2_EN
static uart_buffer uart2_rx;
static uart_buffer uart2_tx;

void USART2_IRQHandler(void){
  if( USART_STAT(USART2) & USART_STAT_RBNE ){
    uint8_t temp = USART_DATA(USART2);
    uart_buf_write(&uart2_rx, temp);
  }else if( USART_STAT(USART2) & USART_STAT_TBE ){
    if(uart_buf_size(&uart2_tx) != 0)USART_DATA(USART2) = uart_buf_read(&uart2_tx);
      else USART_CTL0(USART2) &=~ USART_CTL0_TBEIE;
  }
}

void UART2_write(uint8_t *data, uint8_t len){
  while(len--)UART_putc(2, *(data++));
}

void UART2_puts(char *str){
  while(str[0] != 0)UART_putc(2, *(str++));
}

void UART_read(uint8_t *data, uint8_t len){
  while(len--){
    while(UART_rx_count(2) == 0){}
    *(data++) = UART_getc(2);
  }
}

void UART2_gets(char *str, uint8_t len){
  while(len--){
    while(UART_rx_count(2) == 0){}
    str[0] = UART_getc(2);
    if(str[0] == 0 || str[0] == 13)break;
    str++;
  }
  if(str[0] != 0){
    if(len < 3)str[0] = 0;
      else{ str[0] = 0x0A; str[1] = 0x0D; str[2] = 0; }
  }
}
#endif


#endif
