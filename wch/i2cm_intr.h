#ifndef __I2C_master_interrupt_h__
#define __I2C_master_interrupt_h__
//v. 2024.05.20

#if 1==0
  
example:

#define I2C_NUM 1
#define I2C_SPEED I2CM_400k_16_9 // I2CM_100k or I2CM_400k_16_9 or I2CM_400k_2_1
#define I2C_REMAP
#include "i2cm_intr.h"

#endif

#define I2CM_100k		0
#define I2CM_400k_16_9	1
#define I2CM_400k_2_1	2

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
  void i2c_uart_b16(uint16_t val){
    for(uint16_t mask = 0x8000; mask!=0; mask>>=1){
      if(mask == 0x0080)UART_putc(USART, ' ');
      if(val & mask)UART_putc(USART, '1'); else UART_putc(USART, '0');
    }
    UART_putc(USART, '\t');
  }
  #define i2c_log(str) UART_puts(USART, str)
  #define i2c_b16(x) i2c_uart_b16(x)
#else
  #define i2c_log(str)
  #define i2c_b16(x) 
#endif

static volatile uint8_t i2cm_internal_addr = 0;
static volatile uint8_t *i2cm_internal_wbuf = NULL;
static volatile int16_t i2cm_internal_wcnt = 0;
static volatile uint8_t *i2cm_internal_rbuf = NULL;
static volatile int16_t i2cm_internal_rcnt = 0;
static volatile int32_t i2cm_internal_time = 0;
static volatile int32_t i2cm_internal_timeout = 0;
static volatile uint32_t i2cm_internal_apbM = 1;

static void i2cm_sleep(int32_t t){
  t *= i2cm_internal_apbM;
  int32_t t_prev = systick_read32();
  while( systick_read32() - t_prev < t ){}
}

void i2cm_init(uint32_t f_apb){
  RCC->APB1PCENR |= RCC_I2CEN(I2C_NUM);
  I2C(I2C_NUM)->CTLR1 = 0;
  I2C(I2C_NUM)->CTLR2 = 0;
  
  i2cm_internal_apbM = f_apb / 1000000;
  i2cm_internal_addr = 0xFF;
  i2cm_internal_wcnt = 0;
  i2cm_internal_rcnt = 0;
  
#ifdef I2C_REMAP
  RCC->APB2PCENR |= RCC_AFIOEN;
  AFIO->PCFR1 |= AFIO_PCFR1_I2C1_REMAP;
#endif
  GPIO_config( I2C_SCL );
  GPIO_config( I2C_SDA );
  
  I2C(I2C_NUM)->CTLR2 = f_apb/1000000;
  I2C(I2C_NUM)->STAR1 = 0;
  I2C(I2C_NUM)->STAR2 = 0;
  I2C(I2C_NUM)->CKCFGR = 0;
#if I2C_SPEED == I2CM_100k
  uint32_t res = f_apb / (100000*(1+1)); // LO:HI = 1:1
  I2C(I2C_NUM)->RTR = 1000 * (f_apb/1000000) / 1000 + 1;
  I2C(I2C_NUM)->CKCFGR = res;
#elif I2C_SPEED == I2CM_400k_16_9
  uint32_t res = f_apb / (400000*(16+9)); // LO:HI = 16:9
  I2C(I2C_NUM)->RTR = 300 * (f_apb/1000000) / 1000 + 1;
  I2C(I2C_NUM)->CKCFGR = I2C_CKCFGR_FS | I2C_CKCFGR_DUTY | res;
#elif I2C_SPEED == I2CM_400k_2_1
  uint32_t res = f_apb / (400000*(2+1)); // LO:HI = 2:1
  I2C(I2C_NUM)->RTR = 300 * (f_apb/1000000) / 1000 + 1;
  I2C(I2C_NUM)->CKCFGR = I2C_CKCFGR_FS | res;
#else
  #error I2C_SPEED incorrect
#endif
  I2C(I2C_NUM)->CTLR2 |= I2C_CTLR2_ITEVTEN;// | I2C_CTLR2_ITERREN;
  NVIC_SetPriority(I2C_vec(I2C_NUM), 0xFF);
  NVIC_SetPriority(I2C_evec(I2C_NUM), 0xFF);
  NVIC_EnableIRQ( I2C_vec(I2C_NUM) );
  NVIC_EnableIRQ( I2C_evec(I2C_NUM) );
  I2C(I2C_NUM)->CTLR1 = I2C_CTLR1_PE;
#ifdef I2C_DBG_PIN
  GPIO_config(I2C_DBG_PIN);
  GPO_OFF(I2C_DBG_PIN);
#endif
}

void i2cm_start(uint8_t addr, uint8_t *buf_wr, uint16_t cnt_wr, uint8_t *buf_rd, uint16_t cnt_rd, uint32_t timeout_ms){
  i2cm_internal_time = systick_read32();
  
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
  
  i2cm_internal_wbuf = buf_wr;
  i2cm_internal_wcnt = cnt_wr;
  i2cm_internal_rbuf = buf_rd;
  i2cm_internal_rcnt = cnt_rd;
  i2cm_internal_addr = addr;
  
  if(systick_read32() == i2cm_internal_time){
    systick_init();
  }
  i2cm_internal_time = systick_read32();
  if(timeout_ms < 2)timeout_ms = 2;
  i2cm_internal_timeout = timeout_ms * 1000 * i2cm_internal_apbM;
  
  I2C(I2C_NUM)->CTLR1 |= I2C_CTLR1_START;
  I2C(I2C_NUM)->CTLR2 |= I2C_CTLR2_ITEVTEN;
}

int i2cm_isready(){
  if((systick_read32() - i2cm_internal_time) > i2cm_internal_timeout )return I2CM_TIMEOUT;
  if(i2cm_internal_addr != 0xFF)return I2CM_WAIT;
  if(I2C(I2C_NUM)->STAR2 & I2C_STAR2_BUSY)return I2CM_WAIT;
  return I2CM_READY;
}

int i2cm_wait(){
  while(i2cm_isready() == I2CM_WAIT){}
  int res = i2cm_isready();
#if I2C_DBG > 0
  if(res == I2CM_TIMEOUT){
    UART_puts(USART, "Timeout ");
    i2c_b16(I2C(I2C_NUM)->STAR1);
    i2c_b16(I2C(I2C_NUM)->STAR2);
    UART_puts(USART, "\r\n");
  }
#endif
  return res;
}

__attribute__((interrupt))void I2C_handler(I2C_NUM)(void){
#ifdef I2C_DBG_PIN
  GPO_ON(I2C_DBG_PIN);
#endif
  uint32_t stat = I2C(I2C_NUM)->STAR1;
  i2c_log("i"); i2c_b16(stat);
  
  if( I2C(I2C_NUM)->CTLR2 & I2C_CTLR2_ITBUFEN ){
    if(stat & I2C_STAR1_TXE){
      i2cm_internal_time = systick_read32(); //reset timeout
      if(i2cm_internal_wcnt > 0){
        i2c_log("W");
        I2C(I2C_NUM)->DATAR = i2cm_internal_wbuf[0];
        i2cm_internal_wbuf++;
        i2cm_internal_wcnt--;
      }else{
        I2C(I2C_NUM)->CTLR2 &=~ I2C_CTLR2_ITBUFEN;
        if(i2cm_internal_rcnt > 0){
          i2c_log("r");
          I2C(I2C_NUM)->CTLR1 |= I2C_CTLR1_STOP | I2C_CTLR1_START;
        }else{
          i2c_log("P");
        }
      }
    }else if(stat & I2C_STAR1_RXNE){
      i2cm_internal_time = systick_read32()-1; //reset timeout
      i2c_log("R");
      i2cm_internal_rbuf[0] = I2C(I2C_NUM)->DATAR;
      i2cm_internal_rbuf++;
      i2cm_internal_rcnt--;
      if(i2cm_internal_rcnt == 1){
        I2C(I2C_NUM)->CTLR1 &=~ I2C_CTLR1_ACK;
        i2c_log("n");
      }else if(i2cm_internal_rcnt == 0){
        i2c_log("p\r\n");
        I2C(I2C_NUM)->CTLR2 &=~ I2C_CTLR2_ITEVTEN | I2C_CTLR2_ITBUFEN;
        I2C(I2C_NUM)->CTLR1 |= I2C_CTLR1_STOP;
        i2cm_internal_addr = 0xFF;
      }
    }
  }else if(stat & I2C_STAR1_BTF){ 
    i2c_log("b\r\n");
    I2C(I2C_NUM)->CTLR2 &=~ I2C_CTLR2_ITEVTEN | I2C_CTLR2_ITBUFEN;
    I2C(I2C_NUM)->CTLR1 |= I2C_CTLR1_STOP;
    i2cm_internal_addr = 0xFF;
  }//I2C_CTLR2_ITBUFEN

  if(stat & I2C_STAR1_SB){
    i2c_log("S");
    I2C(I2C_NUM)->DATAR = (i2cm_internal_addr<<1) | (i2cm_internal_wcnt == 0);
  }else if(stat & I2C_STAR1_ADDR){
    (void)I2C(I2C_NUM)->STAR2;
    i2c_log("A");
    I2C(I2C_NUM)->CTLR2 |= I2C_CTLR2_ITBUFEN;
    I2C(I2C_NUM)->CTLR1 |= I2C_CTLR1_ACK;
  }
  
  
  i2c_log("\r\n");
#ifdef I2C_DBG_PIN
  GPO_OFF(I2C_DBG_PIN);
#endif
}

__attribute__((interrupt))void I2C_ehandler(I2C_NUM)(void){
  i2cm_sleep(1000000);
  UART_puts(USART, "Err");
  i2c_b16(I2C(I2C_NUM)->STAR1);
  i2c_b16(I2C(I2C_NUM)->STAR2);
  UART_puts(USART, "\r\n");
  i2cm_sleep(10000000);
}

#undef i2c_log
#undef i2c_b16

#endif

#endif