#ifndef __SPI_H__
#define __SPI_H__


#if 0==1
#define SPIn			1 //if macro SPIn not defined -> software SPI
#define SPI_MISO		B,4,1 //may be undefined
#define SPI_MOSI		B,5,1 //may be undefined
#define SPI_SCK			B,3,1
#define SPI_SPEED_DIV	2 // F_APBx / 2..256
//SPI_soft_speed 123  //(software SPI only!) Delay between bits. For super-slow SPI
#define SPI_LSBFIRST	0
#define SPI_PHASE		0

static inline void SPI_init();
static uint16_t SPI_send(uint16_t data);
uint8_t SPI_ready();
void SPI_disable();
void SPI_enable();
static inline void SPI_size_8();
static inline void SPI_size_16();

#endif

#ifndef SPI_SCK
  #error not found '#define SPI_SCK P,n,a' (P=port, n=pin number, a=active level)
#endif
#ifndef SPI_SPEED_DIV
  #warning SPI_SPEED_DIV not defined; using slowest (1:256)
  #define SPI_SPEED_DIV 256
#endif
#ifndef SPI_LSBFIRST
  #warning SPI_LSBFIRST not defined; using default (0)
  #define SPI_LSBFIRST 0
#endif
#ifndef SPI_PHASE
  #warning SPI_PHASE not defined; using default (0)
  #define SPI_PHASE 0
#endif

#if SPIn == 1
  #define SPI_DMA_TX	1,3,do{SPI1->CTLR2 |= SPI_CTLR2_TXDMAEN;}while(0),do{SPI1->CTLR2 &=~ SPI_CTLR2_TXDMAEN;}while(0)
  #define SPI_DMA_RX	1,2,do{SPI1->CTLR2 |= SPI_CTLR2_RXDMAEN;}while(0),do{SPI1->CTLR2 &=~ SPI_CTLR2_RXDMAEN;}while(0)
#elif SPIn == 2
  #define SPI_DMA_TX	1,5,do{SPI2->CTLR2 |= SPI_CTLR2_TXDMAEN;}while(0),do{SPI2->CTLR2 &=~ SPI_CTLR2_TXDMAEN;}while(0)
  #define SPI_DMA_RX	1,4,do{SPI2->CTLR2 |= SPI_CTLR2_RXDMAEN;}while(0),do{SPI2->CTLR2 &=~ SPI_CTLR2_RXDMAEN;}while(0)
#elif SPIn == 3
  #define SPI_DMA_TX	1,2,do{SPI3->CTLR2 |= SPI_CTLR2_TXDMAEN;}while(0),do{SPI3->CTLR2 &=~ SPI_CTLR2_TXDMAEN;}while(0)
  #define SPI_DMA_RX	1,1,do{SPI3->CTLR2 |= SPI_CTLR2_RXDMAEN;}while(0),do{SPI3->CTLR2 &=~ SPI_CTLR2_RXDMAEN;}while(0)
#else
  #ifdef SPIn
    #error wrong SPIn (SPI number)
  #endif
#endif

#ifndef SPI_DECLARATIONS
////////////// Hardware SPI /////////////////////////////////////////////////
#if defined(SPIn)

#if (SPI_SPEED_DIV == 2)
  #define SPI_BRR (0*SPI_CTLR1_BR_0)
#elif (SPI_SPEED_DIV == 4)
  #define SPI_BRR (1*SPI_CTLR1_BR_0)
#elif (SPI_SPEED_DIV == 8)
  #define SPI_BRR (2*SPI_CTLR1_BR_0)
#elif (SPI_SPEED_DIV == 16)
  #define SPI_BRR (3*SPI_CTLR1_BR_0)
#elif (SPI_SPEED_DIV == 32)
  #define SPI_BRR (4*SPI_CTLR1_BR_0)
#elif (SPI_SPEED_DIV == 64)
  #define SPI_BRR (5*SPI_CTLR1_BR_0)
#elif (SPI_SPEED_DIV == 128)
  #define SPI_BRR (6*SPI_CTLR1_BR_0)
#elif (SPI_SPEED_DIV == 256)
  #define SPI_BRR (7*SPI_CTLR1_BR_0)
#else
  #error SPI: wrong SPI_SPEED_DIV macro
#endif

#define __SPI_NAME(num) SPI ## num
#define _SPI_NAME(num) __SPI_NAME(num)
#define CurSPI _SPI_NAME(SPIn)

#define CR1_def (SPI_CTLR1_MSTR | SPI_BRR | SPI_CTLR1_SPE | SPI_CTLR1_SSI | SPI_CTLR1_SSM | \
                (SPI_CTLR1_LSBFIRST * (SPI_LSBFIRST)) |\
                (SPI_CTLR1_CPHA * (SPI_PHASE)) | \
                (SPI_CTLR1_CPOL * (!marg3(SPI_SCK))))

uint8_t SPI_ready();
void SPI_wait();

static inline void SPI_size_8(){
  SPI_wait();
  CurSPI->CTLR1 &=~ SPI_CTLR1_SPE;
  CurSPI->CTLR1 &=~ SPI_CTLR1_DFF;
  CurSPI->CTLR1 |= SPI_CTLR1_SPE;
}

static inline void SPI_size_16(){
  SPI_wait();
  CurSPI->CTLR1 &=~ SPI_CTLR1_SPE;
  CurSPI->CTLR1 |= SPI_CTLR1_DFF;
  CurSPI->CTLR1 |= SPI_CTLR1_SPE;
}

static inline void SPI_init(){
#if SPIn == 1
  RCC->APB2PCENR |= RCC_SPI1EN;
#elif SPIn == 2
  RCC->APB1PCENR |= RCC_SPI2EN;
#elif SPIn == 3
  RCC->APB1PCENR |= RCC_SPI3EN;
#endif
#ifdef SPI_MISO
  GPIO_manual(SPI_MISO, GPIO_HIZ);
#endif
#ifdef SPI_MOSI
  GPIO_manual(SPI_MOSI, GPIO_APP50);
  GPO_OFF(SPI_MOSI);
#endif
  GPIO_manual(SPI_SCK, GPIO_APP50);
  GPO_OFF(SPI_SCK);

  CurSPI->CTLR1 = 0;
  CurSPI->CTLR1 = CR1_def;
#if (SPI_SPEED_DIV == 2) && (defined SPI_MISO)
  CurSPI->HSCR = 1;
#endif
}

void SPI_disable(){
  SPI_wait();
  CurSPI->CTLR1 &=~ SPI_CTLR1_SPE;
}

void SPI_enable(){
  SPI_wait();
  CurSPI->CTLR1 |= SPI_CTLR1_SPE;
}

uint16_t SPI_send(uint16_t data){
  uint16_t res;
  SPI_wait();
  res = CurSPI->DATAR;
  CurSPI->DATAR = data;
  return res;
}

uint8_t SPI_ready(){
  return !(CurSPI->STATR & SPI_STATR_BSY);
}

void SPI_wait(){
  while( CurSPI->STATR & SPI_STATR_BSY ){}
}

#undef SPI_BRR
#undef GPIO_MODE
//#undef __SPI_NAME
//#undef _SPI_NAME
//#undef CurSPI
#undef CR1_def

#else
//////////////// Software SPI /////////////////////////////////////////
#warning Software SPI

static uint8_t SPI_data_size = 8;

#ifndef SPI_soft_speed
  #define SPI_soft_speed (SPI_SPEED_DIV*4)
#endif

static inline void SPI_size_8(){SPI_data_size = 8;}
static inline void SPI_size_16(){SPI_data_size=16;}

static void SPI_sleep(){
  uint32_t time = SPI_soft_speed;
  while(time--)asm volatile("nop");
}

static inline void SPI_init(){
#ifdef SPI_MISO
  GPIO_manual(SPI_MISO, GPIO_HIZ);
#endif
#ifdef SPI_MOSI
  GPIO_manual(SPI_MOSI, GPIO_PP50);
  GPO_OFF(SPI_MOSI);
#endif
  GPIO_manual(SPI_SCK, GPIO_PP50);
  GPO_OFF(SPI_SCK);
}

static uint16_t SPI_send(uint16_t data){
  if(SPI_data_size == 8)data <<= 8;
  for(uint8_t i=0; i<SPI_data_size; i++){
    #ifdef SPI_MOSI
    if(data & 0x8000)GPO_ON(SPI_MOSI); else GPO_OFF(SPI_MOSI);
    #endif
    SPI_sleep();
    GPO_ON(SPI_SCK);
    data<<=1;
    #ifdef SPI_MISO
    if(GPI_ON(SPI_MISO))data |= (1<<0);
    #endif
    SPI_sleep();
    GPO_OFF(SPI_SCK);
  }
  return data;
}

uint8_t SPI_ready(){
  return 1;
}
void SPI_wait(){}
void SPI_disable(){}
void SPI_enable(){}

#endif //hardware/software SPI

#else //SPI_DECLARATIONS

uint8_t SPI_ready();
void SPI_wait();
void SPI_disable();
void SPI_enable();

#endif //SPI_DECLARATIONS

#endif
