#ifndef __I2C_master_inline_H__
#define __I2C_master_inline_H__
//v. 2024.05.20

#if 1==0
  
example:

#define I2C_NUM 1
#define I2C_SPEED I2CM_400k_16_9 // I2CM_100k or I2CM_400k_16_9 or I2CM_400k_2_1
#define I2C_REMAP
#include "i2cm_inline.h"

#endif

#define I2CM_100k		0
#define I2CM_400k_2_1	1
#define I2CM_400k_16_9	2


#define I2CM_WAIT		0
#define I2CM_READY		1
#define I2CM_TIMEOUT	2

void i2cm_init(uint32_t f_apb);
void i2cm_start(uint8_t addr, uint8_t *buf_wr, uint16_t cnt_wr, uint8_t *buf_rd, uint16_t cnt_rd, uint32_t timeout_ms);
int i2cm_isready();
int i2cm_wait();

#ifdef I2C_NUM

#ifndef I2C_SPEED
  #error I2C_SPEED not defined
#endif

#define RCC_I2C2EN (1UL<<22) // Спасибо китайцам, которым лень было прописать это в заголовочнике!
#if I2C_NUM == 1
  #ifndef I2C_REMAP
    #define I2C_SDA		B,7,0,GPIO_AOD50
    #define I2C_SCL		B,6,0,GPIO_AOD50
  #else
    #define I2C_SDA		B,9,0,GPIO_AOD50
    #define I2C_SCL		B,8,0,GPIO_AOD50
  #endif
#elif I2C_NUM == 2
  #define I2C_SDA		B,11,0,GPIO_AOD50
  #define I2C_SCL		B,10,0,GPIO_AOD50
#endif

#define _RCC_I2CEN(num) RCC_I2C ## num ## EN
#define RCC_I2CEN(num) _RCC_I2CEN(num)
#define _RCC_I2CRST(num) RCC_I2C ## num ## RST
#define RCC_I2CRST(num) _RCC_I2CRST(num)
#define _I2C(num) I2C ## num
#define I2C(num) _I2C(num)
#define _I2C_vec(num) I2C ## num ## _EV_IRQn
#define I2C_vec(num) _I2C_vec(num)
#define _I2C_evec(num) I2C ## num ## _ER_IRQn
#define I2C_evec(num) _I2C_evec(num)
#define _I2C_handler(num) I2C ## num ## _EV_IRQHandler
#define I2C_handler(num) _I2C_handler(num)
#define _I2C_ehandler(num) I2C ## num ## _ER_IRQHandler
#define I2C_ehandler(num) _I2C_ehandler(num)

#ifndef I2C_DBG
  #define I2C_DBG 0
#endif

#if I2C_DBG > 1
  #define i2c_log(str) UART_puts(USART, str)
  #define i2c_b16(x) uart_b16(x)
#else
  #define i2c_log(str)
  #define i2c_b16(x) 
#endif

static uint32_t i2cm_internal_apbM;
static uint8_t i2cm_internal_status = I2CM_READY;

static void i2cm_sleep(int32_t t){
  t *= i2cm_internal_apbM;
  int32_t t_prev = systick_read32();
  while( systick_read32() - t_prev < t ){}
}

void i2cm_init(uint32_t f_apb){
  RCC->APB1PCENR |= RCC_I2CEN(I2C_NUM);
  I2C(I2C_NUM)->CTLR1 = 0;
  I2C(I2C_NUM)->CTLR2 = 0;
  GPIO_config( I2C_SCL );
  GPIO_config( I2C_SDA );
  
  I2C(I2C_NUM)->CTLR1 = 0;
  I2C(I2C_NUM)->CTLR2 = f_apb/1000000;
  I2C(I2C_NUM)->STAR1 = 0;
  I2C(I2C_NUM)->STAR2 = 0;
  I2C(I2C_NUM)->CKCFGR = 0;
  i2cm_internal_apbM = f_apb / 1000000;
#if I2C_SPEED == I2CM_100k
  uint32_t res = f_apb / (100000*(1+1)); // LO:HI = 1:1
  I2C(I2C_NUM)->RTR = 1000 * (f_apb/1000000) / 1000 + 1;
  I2C(I2C_NUM)->CKCFGR = res;
#elif I2C_SPEED == I2CM_400k_16_9
  uint32_t res = f_apb / (100000*(16+9)); // LO:HI = 16:9
  I2C(I2C_NUM)->RTR = 300 * (f_apb/1000000) / 1000 + 1;
  I2C(I2C_NUM)->CKCFGR = I2C_CKCFGR_FS | I2C_CKCFGR_DUTY | res;
#elif I2C_SPEED == I2CM_400k_2_1
  uint32_t res = f_apb / (100000*(2+1)); // LO:HI = 2:1
  I2C(I2C_NUM)->RTR = 300 * (f_apb/1000000) / 1000 + 1;
  I2C(I2C_NUM)->CKCFGR = I2C_CKCFGR_FS | res;
#else
  #error I2C_SPEED incorrect
#endif
  i2cm_internal_status = I2CM_READY;
  I2C(I2C_NUM)->CTLR1 = I2C_CTLR1_PE;
}

void i2cm_start(uint8_t addr, uint8_t *buf_wr, uint16_t cnt_wr, uint8_t *buf_rd, uint16_t cnt_rd, uint32_t timeout_ms){
  uint32_t timeout = timeout_ms * 100;
  i2cm_internal_status = I2CM_READY;
  
  if(I2C(I2C_NUM)->STAR2 & I2C_STAR2_BUSY){
    //UART_puts(USART, "busy\r\n");
    NVIC_DisableIRQ( I2C_vec(I2C_NUM) );
    NVIC_DisableIRQ( I2C_evec(I2C_NUM) );
    I2C(I2C_NUM)->CTLR1 = I2C_CTLR1_SWRST;
    i2cm_sleep(100);
    I2C(I2C_NUM)->CTLR1 = 0;
    GPIO_manual( I2C_SCL, GPIO_OD50 );
    GPIO_manual( I2C_SDA, GPIO_OD50 );
    GPO_ON( I2C_SDA );
    for(int i=0; i<10; i++){
      GPO_T( I2C_SCL );
      i2cm_sleep(10);
    }
    GPO_OFF( I2C_SCL );
    GPO_OFF( I2C_SDA );
    for(int i=0; i<10; i++){
      GPO_T( I2C_SCL );
      i2cm_sleep(10);
    }
    GPO_OFF( I2C_SCL );
    i2cm_sleep(100);
    i2cm_init( i2cm_internal_apbM * 1000000);
  }
  
  I2C(I2C_NUM)->CTLR1 |= I2C_CTLR1_START;
  while(!(I2C(I2C_NUM)->STAR1 & I2C_STAR1_SB)){
    if(! timeout--){i2cm_internal_status = I2CM_TIMEOUT; return;}
  }
  (void)I2C(I2C_NUM)->STAR1;
  
  i2c_log("A");
  I2C(I2C_NUM)->DATAR = (addr<<1) | 0;
  while(!(I2C(I2C_NUM)->STAR1 & I2C_STAR1_ADDR)){
    if(! timeout--){i2cm_internal_status = I2CM_TIMEOUT; return;}
  }
  (void)I2C(I2C_NUM)->STAR1;
  (void)I2C(I2C_NUM)->STAR2;
  
  i2c_log("R");
  for(uint32_t i=0; i<cnt_wr; i++){
    //uart_u32(i); UART_putc(USART, ' ');
    I2C(I2C_NUM)->DATAR = buf_wr[i];
    while(!(I2C(I2C_NUM)->STAR1 & I2C_STAR1_TXE)){
      if(! timeout--){i2cm_internal_status = I2CM_TIMEOUT; return;}
    }
  }
  
  if(cnt_rd != 0){
    i2c_log("s");
    I2C(I2C_NUM)->CTLR1 |= I2C_CTLR1_STOP | I2C_CTLR1_START;
    while(!(I2C(I2C_NUM)->STAR1 & I2C_STAR1_SB)){
      if(! timeout--){i2cm_internal_status = I2CM_TIMEOUT; return;}
    }
    (void)I2C(I2C_NUM)->STAR1;
    
    i2c_log("a");
    I2C(I2C_NUM)->DATAR = (addr<<1) | 1;
    while(!(I2C(I2C_NUM)->STAR1 & I2C_STAR1_ADDR)){
      if(! timeout--){i2cm_internal_status = I2CM_TIMEOUT; return;}
    }
    (void)I2C(I2C_NUM)->STAR1;
    (void)I2C(I2C_NUM)->STAR2;
    
    for(int i=0; i<cnt_rd-1; i++){
      I2C(I2C_NUM)->CTLR1 |= I2C_CTLR1_ACK;
      while(!(I2C(I2C_NUM)->STAR1 & I2C_STAR1_RXNE)){
        if(! timeout--){i2cm_internal_status = I2CM_TIMEOUT; return;}
      }
      buf_rd[i] = I2C(I2C_NUM)->DATAR;
      //uart_u32(i); UART_putc(USART, ' ');
    }
    I2C(I2C_NUM)->CTLR1 &=~ I2C_CTLR1_ACK;
    while(!(I2C(I2C_NUM)->STAR1 & I2C_STAR1_RXNE)){
      if(! timeout--){i2cm_internal_status = I2CM_TIMEOUT; return;}
    }
    buf_rd[cnt_rd-1] = I2C(I2C_NUM)->DATAR;
    //uart_u32(count-1); UART_putc(USART, ' ');
  }
  
  i2c_log("P");
  I2C(I2C_NUM)->CTLR1 |= I2C_CTLR1_STOP;
  
  while(I2C(I2C_NUM)->STAR2 & I2C_STAR2_BUSY){
    if(! timeout--){i2cm_internal_status = I2CM_TIMEOUT; return;}
  }
  i2c_log("_\r\n");
}

int i2cm_isready(){return i2cm_internal_status;}
int i2cm_wait(){return i2cm_internal_status;}

#endif

#endif