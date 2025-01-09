/*
Не помню что это за версия. Даже если рабочая, то одна из первых
*/
#if 1==0
//1: clock from MASTER-SPI
  #define SPIOSCI		3,1,2 // SPI3=master, SPI1=slave, SPI2=slave
  #define SPIOSCI_SPEED 2 // F_APB( SPI3 ) / 2
  #define SPIOSCI_SZ	1000
  #include "spi_osci.h"
  //WARNING: inputs: SPI3-MISO, other-MOSI
//2: clock from Timer
  #define SPIOSCI			3,1,2 // all SPI = slaves
  #define SPIOSCI_TIMER		10,1,TIMO_PWM_NINV | TIMO_NEG | TIMO_REMAP1 //TIM10.1N, remap1
  #define SPIOSCI_TIMOUT	A,5,1
  #define SPIOSCI_SPEED 	2 // F_APB(Tim10) / 2
  #define SPIOSCI_SZ		1000
  #include "spi_osci.h"
  //WARNING: inputs: all - MOSI
//Common:
  void spiosci_uart(uint8_t chan1, uint8_t chan2, uint8_t chan3){
    for(int i=0; i<8; i++){
      while(UART_avaible(USART) < 16){}
      uint8_t a = chan1 & 1;
      uint8_t b = chan2 & 1;
      uint8_t c = chan3 & 1;
      UART_putc(USART, '@' | a | (b<<1) | (c<<2));
      chan1 >>= 1;
      chan2 >>= 1;
      chan3 >>= 1;
    }
  }
...
  spiosci_init();
  spiosci_start();
  //Do smth
  spiosci_stop();
  spiosci_out(spiosci_uart);
#endif


#ifdef __SPIOSCI_PINS__

#if __SPIOSCI_PINS__ == 1
  #define SPIn			1
  #define SPI_MISO		A,6,1
  #define SPI_MOSI		A,7,1
  #define SPI_SCK		A,5,1
#elif __SPIOSCI_PINS__ == 2
  #define SPIn			2
  #define SPI_MISO		B,14,1
  #define SPI_MOSI		B,15,1
  #define SPI_SCK		B,13,1
#elif __SPIOSCI_PINS__ == 3
  #define SPIn			3
  #define SPI_MISO		B,4,1
  #define SPI_MOSI		B,5,1
  #define SPI_SCK		B,3,1
#error
#else
  #define SPIn			0
#endif
#undef __SPIOSCI_PINS__
#else



#ifndef __SPI_OSCI_H__
#define __SPI_OSCI_H__

#include "dma.h"

#ifdef SPIOSCI_TIMER
  #ifndef SPIOSCI_TIMOUT
    #error SPIOSCI_TIMOUT (timer output GPIO) undefined
  #endif
  #ifndef SPIOSCI_SPEED
    #warning SPIOSCI_SPEED undefined. Use default value (1:256)
    #define SPIOSCI_SPEED 256
  #endif
  #define SPIOSCI_SPEED_DIV 2
  #include "timer.h"
#else //SPI(ch1) = master, others = slaves
  #ifdef SPIOSCI_SPEED
    #define SPIOSCI_SPEED_DIV SPIOSCI_SPEED
  #else
    #warning SPIOSCI_SPEED undefined. Use default value (1:256)
    #define SPIOSCI_SPEED_DIV 256
  #endif
#endif

#define nums SPIOSCI,0,0,0
#define ch1	marg1(nums)
#define ch2 marg2(nums)
#define ch3 marg3(nums)

#define __SPIOSCI_PINS__ ch1
#include "spi_osci.h"
#define SPI_SPEED_DIV	SPIOSCI_SPEED_DIV
#define SPI_LSBFIRST	1
#define SPI_PHASE		0
  #ifndef SPIOSCI_TIMER
    #undef SPI_MOSI
    #define SPI_MODE		SPI_MASTER
  #else
    #undef SPI_MISO
    #define SPI_MODE		SPI_SLAVE
  #endif
#include "spi.h"
#undef SPIn
#define chbuf1(i) spiosci_buf0[i]
void dma_register(SPI_DMA_RX(ch1)){}
#ifndef SPIOSCI_TIMER
  void dma_register(SPI_DMA_TX(ch1)){}
#endif

#if ch2 != 0
  #define __SPIOSCI_PINS__ ch2
  #include "spi_osci.h"
  #undef SPI_MISO
  #define SPI_SPEED_DIV	SPIOSCI_SPEED_DIV
  #define SPI_LSBFIRST	1
  #define SPI_PHASE		0
  #define SPI_MODE		SPI_SLAVE
  #include "spi.h"
  #undef SPIn
  #define chbuf2(i) spiosci_buf1[i]
  void dma_register(SPI_DMA_RX(ch2)){}
  
  #if ch3 != 0
    #define __SPIOSCI_PINS__ ch3
    #include "spi_osci.h"
    #undef SPI_MISO
    #define SPI_SPEED_DIV	SPIOSCI_SPEED_DIV
    #define SPI_LSBFIRST	1
    #define SPI_PHASE		0
    #define SPI_MODE		SPI_SLAVE
    #include "spi.h"
    #undef SPIn
    #define chbuf3(i) spiosci_buf2[i]
    void dma_register(SPI_DMA_RX(ch3)){}
    #define SPIOSCI_channels	3
  #else  //marg3 == 0
    #define SPIOSCI_channels	2
    #define chbuf3(i) 0
  #endif //marg3 ?= 0
#else  //marg2 == 0
  #define SPIOSCI_channels	1
  #define chbuf2(i) 0
  #define chbuf3(i) 0
#endif //marg2 ?= 0

//__attribute__ ((aligned (4096)))
uint8_t spiosci_buf0[SPIOSCI_SZ]; 
#if ch2 != 0 
//__attribute__ ((aligned (32768)))
uint8_t spiosci_buf1[SPIOSCI_SZ];
#endif
#if ch3 != 0 
//__attribute__ ((aligned (32768)))
uint8_t spiosci_buf2[SPIOSCI_SZ];
#endif

void spiosci_init(){
  SPI_init(ch1);
  SPI_size_16(ch1);
  dma_clock(SPI_DMA_RX(ch1), 1);
  dma_disable(SPI_DMA_RX(ch1));
  
#ifndef SPIOSCI_TIMER
  dma_clock(SPI_DMA_TX(ch1), 1);
  dma_disable(SPI_DMA_TX(ch1));
  dma_cfg_io(SPI_DMA_TX(ch1), &SPI_DATA(ch1), spiosci_buf0, SPIOSCI_SZ/2);
  dma_cfg_mem(SPI_DMA_TX(ch1), 16,0, 16,0, 0, DMA_PRI_VHIGH);
#else
  GPIO_manual(SPIOSCI_TIMOUT, GPIO_APP50);
  timer_init(SPIOSCI_TIMER, SPIOSCI_SPEED/2-1, 2-1);
  timer_chval(SPIOSCI_TIMER) = 1;
  timer_chcfg(SPIOSCI_TIMER);
  TIMO_OFF(SPIOSCI_TIMER);
  timer_enable(SPIOSCI_TIMER);
#endif
  
  dma_cfg_io(SPI_DMA_RX(ch1), &spiosci_buf0, &SPI_DATA(ch1), SPIOSCI_SZ/2);
  dma_cfg_mem(SPI_DMA_RX(ch1), 16,1, 16,0, 0, DMA_PRI_VHIGH);
  dma_enable(SPI_DMA_RX(ch1));
  
#if ch2 != 0
  SPI_init(ch2);
  SPI_size_16(ch2);
  dma_clock(SPI_DMA_RX(ch2), 1);
  dma_disable(SPI_DMA_RX(ch2));
  
  dma_cfg_io(SPI_DMA_RX(ch2), &spiosci_buf1, &SPI_DATA(ch2), SPIOSCI_SZ/2);
  dma_cfg_mem(SPI_DMA_RX(ch2), 16,1, 16,0, 0, DMA_PRI_VHIGH);
  dma_enable(SPI_DMA_RX(ch2));
#endif
  
#if ch3 != 0
  SPI_init(ch3);
  SPI_size_16(ch3);
  dma_clock(SPI_DMA_RX(ch3), 1);
  dma_disable(SPI_DMA_RX(ch3));
  
  dma_cfg_io(SPI_DMA_RX(ch3), &spiosci_buf2, &SPI_DATA(ch3), SPIOSCI_SZ/2);
  dma_cfg_mem(SPI_DMA_RX(ch3), 16,1, 16,0, 0, DMA_PRI_VHIGH);
  dma_enable(SPI_DMA_RX(ch3));
#endif
  

}

#ifndef SPIOSCI_TIMER
  void spiosci_start(){
    dma_enable(SPI_DMA_TX(ch1));
  }
  void spiosci_stop(){
    dma_disable(SPI_DMA_TX(ch1));
  }
#else
  void spiosci_start(){
    TIMO_DEF(SPIOSCI_TIMER);
  }
  void spiosci_stop(){
    TIMO_OFF(SPIOSCI_TIMER);
  }
#endif

uint16_t spiosci_count(){
  uint32_t sz = SPIOSCI_SZ - 2*(DMA_CH(SPI_DMA_RX(ch1))->CNTR);
/*#if ch2 != 0
  uint32_t sz2 = SPIOSCI_SZ - DMA_CH(SPI_DMA_RX(ch2))->CNTR;
  if(sz2 < sz)sz = sz2;
  #if ch3 != 0
    uint32_t sz3 = SPIOSCI_SZ - DMA_CH(SPI_DMA_RX(ch3))->CNTR;
    if(sz3 < sz)sz = sz3;
  #endif
#endif*/
  return sz;
//  return SPIOSCI_SZ - DMA_CH(SPI_DMA_RX(ch1))->CNTR;
}

typedef void (*spiosci_outfunc_t)(uint8_t chan1, uint8_t chan2, uint8_t chan3);

void spiosci_out(spiosci_outfunc_t outfunc){
  //uint16_t cnt = SPIOSCI_SZ - DMA_CH(SPI_DMA_RX(ch1))->CNTR;
  uint32_t cnt = spiosci_count();
  for(uint32_t i=0; i<cnt; i++){
    outfunc( chbuf1(i), chbuf2(i), chbuf3(i) );
    //outfunc( spiosci_buf[0][i], 0, 0 );
  }
}

#undef ch1
#undef ch2
#undef ch3
#undef chbuf1
#undef chbuf2
#undef chbuf3
#undef nums

#endif //__SPI_OSCI_H__

#endif //__SPIOSCI_PINS__