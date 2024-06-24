#ifndef __I2C_master_interrupt_h__
#define __I2C_master_interrupt_h__
//v. 2024.05.20

#if 1==0
  
example:

#define I2C_NUM 0
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

#if (defined I2C_NUM) && (!defined I2C_PROTOTYPES_ONLY)

#ifndef I2C_SPEED
  #warning I2C_SPEED not defined; use default
  #define I2C_SPEED I2CM_100k
#endif

#if I2C_NUM == 0
  #ifndef I2C_REMAP
    #define I2C_SDA		B,7,0,GPIO_AOD50
    #define I2C_SCL		B,6,0,GPIO_AOD50
  #else
    #define I2C_SDA		B,9,0,GPIO_AOD50
    #define I2C_SCL		B,8,0,GPIO_AOD50
  #endif
#elif I2C_NUM == 1
  #define I2C_SDA		B,11,0,GPIO_AOD50
  #define I2C_SCL		B,10,0,GPIO_AOD50
#endif

#define _RCU_I2CEN(num) RCU_APB1EN_I2C ## num ## EN
#define RCU_I2CEN(num) _RCU_I2CEN(num)
#define __I2Cn(num) I2C ## num
#define _I2Cn(num) __I2Cn(num)
#define I2Cn	_I2Cn(I2C_NUM)

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
static volatile uint32_t i2cm_internal_time = 0;
static volatile uint32_t i2cm_internal_timeout = 0;
static volatile uint32_t i2cm_internal_apbM = 1;

static void i2cm_sleep(int32_t t){
  delay_us(t);
}

void i2cm_init(uint32_t f_apb){
  RCU_APB1EN |= RCU_I2CEN(I2C_NUM);
  I2C_CTL0(I2Cn) = 0;
  I2C_CTL1(I2Cn) = 0;
  
  i2cm_internal_apbM = f_apb / 1000000;
  if(i2cm_internal_apbM < 2)i2cm_internal_apbM = 2;
  i2cm_internal_addr = 0xFF;
  i2cm_internal_wcnt = 0;
  i2cm_internal_rcnt = 0;
  
#ifdef I2C_REMAP
  RCU_APB2EN |= RCU_APB2EN_AFEN;
  AFIO_PCF0 |= AFIO_PCF0_I2C0_REMAP;
#endif
  GPIO_config( I2C_SCL );
  GPIO_config( I2C_SDA );
  
  I2C_CTL1(I2Cn) = f_apb/1000000;
  I2C_STAT0(I2Cn) = 0;
  I2C_STAT1(I2Cn) = 0;
  I2C_CKCFG(I2Cn) = 0;
#if I2C_SPEED == I2CM_100k
  uint32_t res = f_apb / (100000*(1+1)); // LO:HI = 1:1
  I2C_RT(I2Cn) = 1000 * (f_apb/1000000) / 1000 + 1;
  I2C_CKCFG(I2Cn) = res;
#elif I2C_SPEED == I2CM_400k_16_9
  uint32_t res = f_apb / (100000*(16+9)); // LO:HI = 16:9
  I2C_RT(I2Cn) = 300 * (f_apb/1000000) / 1000 + 1;
  I2C_CKCFG(I2Cn) = I2C_CKCFG_FAST | I2C_CKCFG_DTCY | res;
#elif I2C_SPEED == I2CM_400k_2_1
  uint32_t res = f_apb / (100000*(2+1)); // LO:HI = 2:1
  I2C_RT(I2Cn) = 300 * (f_apb/1000000) / 1000 + 1;
  I2C_CKCFG(I2Cn) = I2C_CKCFG_FAST | res;
#else
  #error I2C_SPEED incorrect
#endif
  I2C_CTL1(I2Cn) |= I2C_CTL1_EVIE;// | I2C_CTLR2_ITERREN;
  eclic_set_vmode( I2C_vec(I2C_NUM) );
  //eclic_set_irq_priority( I2C_vec(I2C_NUM), 0x00 );
  eclic_enable_interrupt( I2C_vec(I2C_NUM) );
  eclic_set_vmode( I2C_evec(I2C_NUM) );
  //eclic_set_irq_priority( I2C_evec(I2C_NUM), 0x00 );
  eclic_enable_interrupt( I2C_evec(I2C_NUM) );
  
  I2C_CTL0(I2Cn) = I2C_CTL0_I2CEN;
#ifdef I2C_DBG_PIN
  GPIO_config(I2C_DBG_PIN);
  GPO_OFF(I2C_DBG_PIN);
#endif
}

void i2cm_start(uint8_t addr, uint8_t *buf_wr, uint16_t cnt_wr, uint8_t *buf_rd, uint16_t cnt_rd, uint32_t timeout_ms){
  if(I2C_STAT1(I2Cn) & I2C_STAT1_I2CBSY){
    //UART_puts(USART, "busy\r\n");
    eclic_disable_interrupt( I2C_vec(I2C_NUM) );
    eclic_disable_interrupt( I2C_evec(I2C_NUM) );
    I2C_CTL0(I2Cn) = I2C_CTL0_SRESET;
    i2cm_sleep(100);
    I2C_CTL0(I2Cn) = 0;
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
  
  i2cm_internal_time = read_mcycles();
  if(timeout_ms < 2)timeout_ms = 2;
  i2cm_internal_timeout = timeout_ms * 1000 * i2cm_internal_apbM;
  
  I2C_CTL0(I2Cn) |= I2C_CTL0_START;
  I2C_CTL1(I2Cn) |= I2C_CTL1_EVIE;
}

int i2cm_isready(){
  if((read_mcycles() - i2cm_internal_time) > i2cm_internal_timeout )return I2CM_TIMEOUT;
  if(i2cm_internal_addr != 0xFF)return I2CM_WAIT;
  if(I2C_STAT1(I2Cn) & I2C_STAT1_I2CBSY)return I2CM_WAIT;
  return I2CM_READY;
}

int i2cm_wait(){
  while(i2cm_isready() == I2CM_WAIT){}
  int res = i2cm_isready();
#if I2C_DBG > 0
  if(res == I2CM_TIMEOUT){
    UART_puts(USART, "Timeout ");
    i2c_uart_b16(I2C_STAT0(I2Cn));
    i2c_uart_b16(I2C_STAT1(I2Cn));
    UART_puts(USART, "\r\n");
  }
#endif
  return res;
}

void I2C_handler(I2C_NUM)(void){
#ifdef I2C_DBG_PIN
  GPO_ON(I2C_DBG_PIN);
#endif
  uint32_t stat = I2C_STAT0(I2Cn);
  i2c_log("i"); i2c_b16(stat);
  
  if( I2C_CTL1(I2Cn) & I2C_CTL1_BUFIE ){
    if(stat & I2C_STAT0_TBE){
      i2cm_internal_time = read_mcycles(); //reset timeout
      if(i2cm_internal_wcnt > 0){
        i2c_log("W");
        I2C_DATA(I2Cn) = i2cm_internal_wbuf[0];
        i2cm_internal_wbuf++;
        i2cm_internal_wcnt--;
      }else{
        I2C_CTL1(I2Cn) &=~ I2C_CTL1_BUFIE;
        if(i2cm_internal_rcnt > 0){
          i2c_log("r");
          I2C_CTL0(I2Cn) |= I2C_CTL0_STOP | I2C_CTL0_START;
        }else{
          i2c_log("P");
        }
      }
    }else if(stat & I2C_STAT0_RBNE){
      i2cm_internal_time = read_mcycles()-1; //reset timeout
      i2c_log("R");
      i2cm_internal_rbuf[0] = I2C_DATA(I2Cn);
      i2cm_internal_rbuf++;
      i2cm_internal_rcnt--;
      if(i2cm_internal_rcnt == 1){
        I2C_CTL0(I2Cn) &=~ I2C_CTL0_ACKEN;
        i2c_log("n");
      }else if(i2cm_internal_rcnt == 0){
        i2c_log("p\r\n");
        I2C_CTL1(I2Cn) &=~ I2C_CTL1_EVIE | I2C_CTL1_BUFIE;
        I2C_CTL0(I2Cn) |= I2C_CTL0_STOP;
        i2cm_internal_addr = 0xFF;
      }
    }
  }else if(stat & I2C_STAT0_BTC){ 
    i2c_log("b\r\n");
    I2C_CTL1(I2Cn) &=~ I2C_CTL1_EVIE | I2C_CTL1_BUFIE;
    I2C_CTL0(I2Cn) |= I2C_CTL0_STOP;
    i2cm_internal_addr = 0xFF;
  }//I2C_CTL1_BUFIE

  if(stat & I2C_STAT0_SBSEND){
    i2c_log("S");
    I2C_DATA(I2Cn) = (i2cm_internal_addr<<1) | (i2cm_internal_wcnt == 0);
  }else if(stat & I2C_STAT0_ADDSEND){
    (void)I2C_STAT1(I2Cn);
    i2c_log("A");
    I2C_CTL1(I2Cn) |= I2C_CTL1_BUFIE;
    I2C_CTL0(I2Cn) |= I2C_CTL0_ACKEN;
  }
  
  
  i2c_log("\r\n");
#ifdef I2C_DBG_PIN
  GPO_OFF(I2C_DBG_PIN);
#endif
}

void I2C_ehandler(I2C_NUM)(void){
  i2cm_sleep(1000000);
  UART_puts(USART, "Err");
  i2c_b16(I2C_STAT0(I2Cn));
  i2c_b16(I2C_STAT1(I2Cn));
  UART_puts(USART, "\r\n");
  i2cm_sleep(10000000);
}

#undef i2c_log
#undef i2c_b16

#endif

#endif