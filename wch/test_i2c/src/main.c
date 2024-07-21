#include "ch32v30x.h"
#include "hardware.h"
#include "clock.h"
#define USART 1
#define UART_SIZE_PWR 10
#include "uart.h"


#ifndef NULL
  #define NULL ((void*)0)
#endif
#define __TIME_S__ ((__TIME__[0]-'0')*36000+(__TIME__[1]-'0')*3600 + (__TIME__[3]-'0')*600+(__TIME__[4]-'0')*60 + (__TIME__[6]-'0')*10+(__TIME__[7]-'0'))
typedef uint32_t size_t;
void *memcpy(void *dest, const void *src, size_t n){
  for(size_t i=0; i<n; i++)((char*)dest)[i]=((char*)src)[i];
  return dest;
}

void test_oled2();
void test_oled();
void test_accel();


#define ADDR_OLED	0x3C //ssd1306
#define ADDR_MPU6000  0x68

void SystemInit(void){}
void sleep(uint32_t t){while(t--)asm volatile("nop");}

void uart_u32(uint32_t val){
  char buf[11];
  char *ch = &buf[10];
  ch[0] = 0;
  do{
    *(--ch) = (val % 10) + '0';
    val /= 10;
  }while(val);
  while(ch[0] != 0){
    UART_putc(USART, ch[0]);
    ch++;
  }
}
void uart_s32(int32_t val){
  char buf[12];
  char *ch = &buf[11];
  char sign = 0;
  if(val < 0){sign=1; val=-val;}
  ch[0] = 0;
  do{
    *(--ch) = (val % 10) + '0';
    val /= 10;
  }while(val);
  if(sign){
    *(--ch) = '-';
  }
  while(ch[0] != 0){
    UART_putc(USART, ch[0]);
    ch++;
  }
}
void uart_b16(uint16_t val){
  for(uint16_t mask = 0x8000; mask!=0; mask>>=1){
    if(mask == 0x0080)UART_putc(USART, ' ');
    if(val & mask)UART_putc(USART, '1'); else UART_putc(USART, '0');
  }
  //UART_putc(USART, '\r'); UART_putc(USART, '\n');
  UART_putc(USART, '\t');
}
void uart_x8(uint8_t val){
  uint8_t d = val >> 4;
  if(d <= 9)UART_putc(USART, d + '0'); else UART_putc(USART, d - 0x0A + 'A');
  d = val & 0x0F;
  if(d <= 9)UART_putc(USART, d + '0'); else UART_putc(USART, d - 0x0A + 'A');
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////

#define I2C_NUM 2
#define I2C_SPEED I2CM_400k_16_9
//#define I2C_DBG_PIN B,8,1,GPIO_PP50
//#define I2C_DBG 0

#define MODE 1
#if MODE == 0
  #include "i2cm_inline.h"
#elif MODE == 1
  #include "i2cm_intr.h"
#else
  #error
#endif

#define OLED_MODE OLED_128x32
//#define OLED_MIR_X
#define OLED_MIR_Y
#define OLED_CHARS_CP1251
#include "oled_ssd1306.h"

int dev_test(uint8_t dev){
  uint8_t buf[5];
  buf[0] = 0;
  i2cm_start(dev, buf, 1, buf, 3, 10);
  int res = i2cm_wait();
  return res;
}

void display_i2c(){
  for(uint8_t i=0; i<128; i++){
    int res = dev_test(i);
    if((i & 7) == 0)UART_puts(USART, "\r\n");
    uart_x8(i);
    UART_puts(USART, ":");
    if(res == I2CM_READY){
      UART_puts(USART, "OK");
    }else{
      UART_puts(USART, "_");
    }
    UART_puts(USART, "\t");
    
    sleep(1000);
  }
  UART_puts(USART, "\r\n");
}

void test(){
  GPO_OFF(RLED); GPO_OFF(GLED);
  display_i2c();
  test_oled();
  do{
    GPO_T(YLED);
    test_accel();
    sleep(100000000);
  }while(1);
}

int main(){
  clock_HS(1);
  systick_init();
  RCC->APB2PCENR |= RCC_IOPAEN | RCC_IOPBEN | RCC_IOPCEN | RCC_AFIOEN;
  GPIO_config(RLED); GPIO_config(GLED); GPIO_config(YLED);
  GPO_OFF(RLED); GPO_OFF(GLED); GPO_OFF(YLED);
  
  //UART_init(USART, 8000000 / 115200);
  UART_init(USART, 144000000 / 2 / 115200);
  UART_puts(USART, "\r\n\r\n");
  UART_puts(USART, __TIME__ " " __DATE__ "\r\n");
  
  //i2cm_init(8000000);
  i2cm_init(144000000 / 2);
  test();
  
  while(1){
    GPO_T(YLED);
    sleep(1000000);
  }
}

void test_accel(){
  uint8_t data[10];
  data[0] = 0x1B;
  data[1] = 0x08;
  data[2] = 0x18;
  i2cm_start(ADDR_MPU6000, data, 3, NULL, 0, 10000);
  i2cm_wait();

  data[0] = 0x6B;
  data[1] = 1;
  i2cm_start(ADDR_MPU6000, data, 2, NULL, 0, 10000);
  i2cm_wait();

  for(int i=0; i<10; i++)data[i] = 0;
  data[9] = 0x3B;
  i2cm_start(ADDR_MPU6000, &data[9], 1, data, 6, 10000);
  i2cm_wait();

  int16_t res[3];
  res[0] = (data[0]<<8) | data[1];
  res[1] = (data[2]<<8) | data[3];
  res[2] = (data[4]<<8) | data[5];

  data[0] = 0x6B;
  data[1] = 1;
  i2cm_start(ADDR_MPU6000, data, 2, NULL, 0, 10000);
  i2cm_wait();
  
  uart_s32(res[0]); UART_putc(USART, '\t');
  uart_s32(res[1]); UART_putc(USART, '\t');
  uart_s32(res[2]);
  UART_puts(USART, "\r");
}

void test_oled(){
  oled_init();
  oled_str(0, 0, 2, __TIME__);
  oled_update();
  oled_wait();
}