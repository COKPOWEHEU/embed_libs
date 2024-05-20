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
#define I2C_DBG_PIN B,8,1,GPIO_PP50
//#define I2C_DBG 0

#define MODE 1
#if MODE == 0
  #include "i2cm_inline.h"
#elif MODE == 1
  #include "i2cm_intr.h"
#else
  #error
#endif

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

void oled_send(uint8_t *buf, uint16_t count){
  i2cm_start(ADDR_OLED, buf, count, NULL, 0, 100);
  i2cm_wait();
}

void test_oled(){
  uint8_t data[5];
  // Set display off
  data[0] = 0x00; //OLED command flag
  data[1] = 0xAE;
  oled_send(data, 2);
  if(i2cm_isready() != I2CM_READY){
    UART_puts(USART, "OLED not found\r\n");
    return;
  }
  
  // Set oscillator frequency
  data[0] = 0x00; //OLED command flag
  data[1] = 0xD5;
  data[2] = 0x80;
  oled_send(data, 3);
  
  // Enable charge pump regulator
  data[0] = 0x00; //OLED command flag
  data[1] = 0x8D;
  data[2] = 0x14;
  oled_send(data, 3);
  // Set display start line
  data[0] = 0x00; //OLED command flag
  data[1] = 0x40 | 0;
  oled_send(data, 2);
  
  // Set segment remap (поворот дисплея?)
  data[0] = 0x00; //OLED command flag
  data[1] = 0xA1;
  oled_send(data, 2);
  
  // Set COM output scan direction (поворот дисплея?)
  data[0] = 0x00; //OLED command flag
  data[1] = 0xC8;
  oled_send(data, 2);
  
  // Set COM pins hardware configuration
  data[0] = 0x00; //OLED command flag
  data[1] = 0xDA;
  data[2] = 0x20;//0x12; //TODO найти правильное значение!
  oled_send(data, 3);
  
  // Set MUX ratio
  data[0] = 0x00; //OLED command flag
  data[1] = 0xA8;
  data[2] = 63;
  oled_send(data, 3);
  
  // Set display offset
  data[0] = 0x00; //OLED command flag
  data[1] = 0xD3;
  data[2] = 0;
  oled_send(data, 3);
  
  // Set horizontal addressing mode
  data[0] = 0x00; //OLED command flag
  data[1] = 0x20;
  data[2] = 0x00; //00-horizontal, 01-vertical, 10-page(?)
  oled_send(data, 3);
  
  // Set column address
  data[0] = 0x00; //OLED command flag
  data[1] = 0x21;
  data[2] = 0;
  data[3] = 127;
  oled_send(data, 4);
  
  // Set page address
  data[0] = 0x00; //OLED command flag
  data[1] = 0x22;
  data[2] = 0; //0
  data[3] = 3;//3;//7;
  oled_send(data, 4);
  
  // Set contrast
  data[0] = 0x00; //OLED command flag
  data[1] = 0x81;
  data[2] = 0x7F;
  oled_send(data, 3);
  // Entire display on
  data[0] = 0x00; //OLED command flag
  data[1] = 0xA4;
  oled_send(data, 2);
  //Set normal display
  data[0] = 0x00; //OLED command flag
  data[1] = 0xA6;
  oled_send(data, 2);
  // Set display on
  data[0] = 0x00; //OLED command flag
  data[1] = 0xAF;
  oled_send(data, 2);
  
  struct{
    uint8_t cmdflag;
    uint8_t buf[128*32/8];
  }screen;
  for(int i=0; i<sizeof(screen.buf); i++){
    screen.buf[i] = i + __TIME_S__;
  }
  screen.cmdflag = 0x40;
  oled_send((uint8_t*)&screen, sizeof(screen));
}