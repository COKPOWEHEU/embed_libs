#ifndef __LCD_N3310_H__
#define __LCD_N3310_H__

#if 1==0
#define N3310_SPI	1
#define N3310_MOSI	A,7,1
#define N3310_SCK	A,5,1
#define N3310_RST	B,13,0, GPIO_PP50
#define N3310_CS	A,4,0, GPIO_PP50
#define N3310_DC	B,10,1, GPIO_PP50
#define N3310_DMA	//may be undefined
#define N3310_CHARS_CP1251 //or N3310_CHARS
#define N3310_3x5_EN
#define N3310_GRAPHICS
#include "lcd_n3310.h"
...
  n3310_init();
  n3310_clear();
  n3310_str(0, 15, 0, "1.2/3-4");
  n3310_str(0, 22, 1, "F1");
  n3310_str(0, 32, 2, "F2");
  n3310_str(10, 5, 1, "Text");
  n3310_hline(10, 34, 4, 2);
  n3310_hline(10, 34, 13, 1);
  n3310_vline(10, 4, 13, 2);
  n3310_vline(34, 4, 13, 1);
  n3310_rect(40, 10, 80, 40, 1);
  n3310_str(45, 15, 2, "#");
  n3310_line(40, 0, 83, 30, 2);
  n3310_hline(50, 60, 35, 0);
  //n3310_disable();
  //n3310_enable();

  while(1){
  ...
    n3310_update();
  }

#endif


#define N3310_W	84
#define N3310_H	48

extern uint8_t n3310_screen[ N3310_W * N3310_H / 8 ];
void n3310_init();
char n3310_ready();
void n3310_contrast( uint8_t contrast );
void n3310_clear();
void n3310_update();
void n3310_disable();
void n3310_enable();
#if defined(N3310_CHARS) || defined(N3310_CHARS_CP1251) || defined(N3310_3x5_EN)
void n3310_str(int x, int y, int font, char *str);
#endif
#ifdef N3310_GRAPHICS
void n3310_rect(int x1, int y1, int x2, int y2, char color); //color=0->clear, color=1->set
void n3310_pix(int x, int y, char color);
void n3310_line(int x1, int y1, int x2, int y2, char color); //color=0->clear, color=1->set, color=2->dotted
void n3310_hline(uint8_t x1, uint8_t x2, uint8_t y, char color); //horizontal line
void n3310_vline(uint8_t x, uint8_t y1, uint8_t y2, char color); //vertical line
#endif

#ifndef N3310_PROTOTYPES

#define SPIn			N3310_SPI
#define SPI_MOSI		N3310_MOSI
#define SPI_SCK			N3310_SCK
#define SPI_SPEED_DIV 	16
#define SPI_MODE		SPI_MASTER
#define SPI_LSBFIRST	0
#define SPI_PHASE		0
#include "spi.h"
#include "dma.h"

uint8_t n3310_screen[ N3310_W * N3310_H / 8 ];

#ifdef N3310_DMA
void dma_register(SPI_DMA_TX(SPIn)){}
uint8_t n3310_dma_run = 0;
char n3310_ready(){
  if(n3310_dma_run){
    if( !dma_flag( SPI_DMA_TX(SPIn), DMA_F_FULL ) )return 0;
    if( !SPI_ready(SPIn) )return 0;
  }
  return 1;
}

#else
char n3310_ready(){return 1;}

#endif

#warning Test n3310_disable, n3310_enable
static void n3310_cmd(uint8_t data){
  SPI_wait(SPIn);
  GPO_OFF(N3310_DC);
  SPI_send(SPIn, data);
  SPI_wait(SPIn);
}

void n3310_disable(){
  while(! n3310_ready() ){}
  GPO_OFF(N3310_CS);
}

void n3310_enable(){
  GPO_ON(N3310_CS);
}

void n3310_init(){
  GPIO_config( N3310_DC );
  GPIO_config( N3310_CS );	GPO_OFF( N3310_CS );	
  GPIO_config( N3310_RST );	GPO_ON( N3310_RST );
  
  for(uint32_t i=0; i<1000; i++)asm volatile("nop");
  
  GPO_OFF( N3310_RST );
  SPI_init(SPIn);
  GPO_ON( N3310_CS );
  
  n3310_cmd ( 0x20 | (0<<2 | 0<<1 | 1<<0) );// Включаем расширенный набор команд (LCD Extended Commands)
  SPI_send(SPIn,  0x80 | 0x48 );  // Установка контрастности (LCD Vop)
  SPI_send(SPIn,  0x04 | 0b10 );  // Установка температурного коэффициента (Temp coefficent)
  SPI_send(SPIn,  0x10 | 0b011 ); // Настройка питания (LCD bias mode 1:48)
  SPI_send(SPIn,  0x20 ); // Включаем стандартный набор команд и горизонтальную адресацию (LCD Standard Commands,Horizontal addressing mode)
  SPI_send(SPIn,  0x08 | (1<<2 | 0<<0) ); // Нормальный режим (LCD in normal mode)
  // Первичная очистка дисплея
  n3310_clear();
  //адрес начальной точки
  n3310_cmd( 0x80 | 0 );
  SPI_send(SPIn,  0x40 | 0 );
  SPI_wait(SPIn);
  
#ifdef N3310_DMA
  dma_clock( SPI_DMA_TX(SPIn), 1);
  dma_disable( SPI_DMA_TX(SPIn) );
#endif
}

void n3310_contrast( uint8_t contrast ){
  n3310_cmd( 0x21 );              // Расширенный набор команд
  SPI_send(SPIn, 0x80 | contrast );  // Установка уровня контрастности
  SPI_send(SPIn, 0x20 );
  SPI_wait(SPIn);
}

void n3310_clear(){
  for(uint16_t i=0; i<sizeof(n3310_screen); i++)n3310_screen[i] = 0;
}

void n3310_update(){
#ifdef N3310_DMA
  if( !n3310_ready() )return;
  dma_flag_clear( SPI_DMA_TX(SPIn), DMA_F_FULL );
#endif
  
  n3310_cmd( 0x20);
  SPI_send(SPIn, 0x80); //goto(x=0)
  SPI_send(SPIn, 0x40);//goto(y=0)
  while(!SPI_ready(SPIn)){}
  GPO_ON(N3310_DC);

#ifdef N3310_DMA
  dma_cfg_io( SPI_DMA_TX(SPIn), &SPI_DATA(SPIn), n3310_screen, sizeof(n3310_screen) );
  dma_cfg_mem( SPI_DMA_TX(SPIn), 8,0, 8,1, 0, DMA_PRI_LOW);
  dma_enable( SPI_DMA_TX(SPIn) );
  n3310_dma_run = 1;
#else
  for(uint16_t i=0; i<sizeof(n3310_screen); i++)SPI_send(SPIn,  n3310_screen[i] );
  SPI_wait(SPIn);
#endif
}

/////////////////////////////////////////////////////////////////////////////////////
//  Text functions //////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
#ifdef N3310_CHARS_CP1251
  #define LCD_CHARS_FULL_TABLE
  #include "lcd_chars_cp1251.h"
#elif defined N3310_CHARS
  #include "lcd_chars_cp1251.h"
#endif

#ifdef N3310_3x5_EN
static const uint8_t n3310_3x5[][3] = {
  {0b11111, 0b10001, 0b11111}, //0
  {0b00000, 0b00000, 0b11111}, //1
  {0b11101, 0b10101, 0b10111}, //2
  {0b10101, 0b10101, 0b11111}, //3
  {0b00111, 0b00100, 0b11111}, //4
  {0b10111, 0b10101, 0b11101}, //5
  {0b11111, 0b10101, 0b11101}, //6
  {0b00001, 0b00001, 0b11111}, //7
  {0b11111, 0b10101, 0b11111}, //8
  {0b10111, 0b10101, 0b11111}, //9
  {0b00100, 0b00100, 0b00100}, //-
  {0b00000, 0b10000, 0b00000}, //dot
  {0b11000, 0b01110, 0b00011}, //slash
  {0b00000, 0b00000, 0b00000}, //space
};

void n3310_ch0(int x, int y, char ch){
  int dy = y % 8;
  y /= 8;
  switch(ch){
    case '0' ... '9': ch -= '0'; break;
    case '-': ch = 10; break;
    case '.': ch = 11; break;
    case '/': ch = 12; break;
    case ' ': ch = 13; break;
    default: return;
  }
  uint8_t *scr = &n3310_screen[x + N3310_W*y];
  uint16_t mask = ~(0b11111<<dy);
  for(int i=0; i<3; i++){
    uint16_t val = n3310_3x5[(uint8_t)ch][i];
    val <<= dy;
    scr[0] = (scr[0] & mask) | val;
    scr[N3310_W] = (scr[N3310_W] & (mask>>8)) | (val>>8);
    scr++;
  }
  scr[0] = (scr[0] & mask);
  scr[N3310_W] = (scr[N3310_W] & (mask>>8));
}
#else
void n3310_ch0(int x, int y, char ch){}
#endif

#if defined(N3310_CHARS) || defined(N3310_CHARS_CP1251) || defined(N3310_3x5_EN)

void n3310_ch1(int x, int y, char ch){
  int dy = y % 8;
  y /= 8;
#ifndef LCD_CHARS_FULL_TABLE
  if(ch < ' ')return;
  if(ch > '~')return;
  ch -= ' ';
#endif
  uint8_t *scr = &n3310_screen[x + N3310_W*y];
  if(dy == 0){
    for(int i=0; i<5; i++){
      scr[0] = char_table[(uint8_t)ch][i];
      scr++;
    }
    scr[0] = 0;
  }else{
    uint16_t mask = ~(0xFF<<dy);
    for(int i=0; i<5; i++){
      uint16_t val = char_table[(uint8_t)ch][i];
      val <<= dy;
      scr[0] = (scr[0] & mask) | val;
      scr[N3310_W] = (scr[N3310_W] & (mask>>8)) | (val >> 8);
      scr++;
    }
    scr[0] = (scr[0] & mask);
    scr[N3310_W] = (scr[N3310_W] & (mask>>8));
  }
}

void n3310_ch2(int x, int y, char ch){
  int dy = y % 8;
  y /= 8;
#ifndef LCD_CHARS_FULL_TABLE
  if(ch < ' ')return;
  if(ch > '~')return;
  ch -= ' ';
#endif
  uint8_t *scr = &n3310_screen[x + N3310_W*y];
  if(dy == 0){
    for(int i=0; i<5; i++){
      uint16_t val = char_table[(uint8_t)ch][i];
      val = (val | (val<<4)) & 0b0000111100001111;//    abcd    efgh
      val = (val | (val<<2)) & 0b0011001100110011;//  ab  cd  ef  gh
      val = (val | (val<<1)) & 0b0101010101010101;// a b c d e f g h
      val = (val | (val<<1));
      scr[0] = val;
      scr[1] = val;
      scr[N3310_W] = val>>8;
      scr[N3310_W+1] = val>>8;
      scr+=2;
    }
    scr[0] = 0; scr[1] = 0;
    scr[N3310_W] = 0; scr[N3310_W+1] = 0;
  }else{
    uint32_t mask = ~(0xFFFF << dy);
    for(int i=0; i<5; i++){
      uint32_t val = char_table[(uint8_t)ch][i];
      val = (val | (val<<4)) & 0b0000111100001111;//    abcd    efgh
      val = (val | (val<<2)) & 0b0011001100110011;//  ab  cd  ef  gh
      val = (val | (val<<1)) & 0b0101010101010101;// a b c d e f g h
      val = (val | (val<<1));
      val = (val << dy);
      scr[0*N3310_W + 0] = (scr[0*N3310_W + 0] & (mask>>0 )) | (val>>0);
      scr[0*N3310_W + 1] = (scr[0*N3310_W + 1] & (mask>>0 )) | (val>>0);
      scr[1*N3310_W + 0] = (scr[1*N3310_W + 0] & (mask>>8 )) | (val>>8);
      scr[1*N3310_W + 1] = (scr[1*N3310_W + 1] & (mask>>8 )) | (val>>8);
      scr[2*N3310_W + 0] = (scr[2*N3310_W + 0] & (mask>>16)) | (val>>16);
      scr[2*N3310_W + 1] = (scr[2*N3310_W + 1] & (mask>>16)) | (val>>16);
      scr+=2;
    }
    scr[0*N3310_W + 0] = (scr[0*N3310_W + 0] & (mask>>0 ));
    scr[0*N3310_W + 1] = (scr[0*N3310_W + 1] & (mask>>0 ));
    scr[1*N3310_W + 0] = (scr[1*N3310_W + 0] & (mask>>8 ));
    scr[1*N3310_W + 1] = (scr[1*N3310_W + 1] & (mask>>8 ));
    scr[2*N3310_W + 0] = (scr[2*N3310_W + 0] & (mask>>16));
    scr[2*N3310_W + 1] = (scr[2*N3310_W + 1] & (mask>>16));
  }
}

void n3310_str(int x, int y, int font, char *str){
  if(font == 0){
    if(y > (N3310_H-5))return;
    while(str[0]){
      if(x >= N3310_W-4)return;
      n3310_ch0(x, y, str[0]);
      str++;
      x += 4;
    }
  }else if(font == 1){
    if(y > (N3310_H-8))return;
    while(str[0]){
      if(x >= N3310_W-6)return;
      n3310_ch1(x, y, str[0]);
      str++;
      x += 6;
    }
  }else if(font == 2){
    if(y > (N3310_H-16))return;
    while(str[0]){
      if(x >= N3310_W-12)return;
      n3310_ch2(x, y, str[0]);
      str++;
      x += 12;
    }
  }
}
#endif
/////////////////////////////////////////////////////////////////////////////////////
//  Graphics functions //////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
#ifdef N3310_GRAPHICS

void n3310_rect(int x1, int y1, int x2, int y2, char color){
  int dy1 = y1 % 8;
  y1 /= 8;
  int dy2 = y2 % 8;
  y2 /= 8;
  int dx = x2-x1;
  uint8_t *scr = &n3310_screen[x1 + y1*N3310_W];
  for(int y=y1; y<=y2; y++){
    uint8_t mask = 0xFF;
    if(y == y1)mask <<= dy1;
    if(y == y2)mask &=~(0xFF00>>(7-dy2));
    if(color){
      for(int x=0; x<=dx; x++){
        scr[x] |= mask;
      }
    }else{
      for(int x=0; x<=dx; x++){
        scr[x] &=~ mask;
      }
    }
    scr += N3310_W;
  }
}

void n3310_pix(int x, int y, char color){
  int dy = y % 8;
  y /= 8;
  uint8_t *scr = &n3310_screen[x + y*N3310_W];
  if(color)scr[0] |= (1<<dy); else scr[0] &=~ (1<<dy);
}

void n3310_hline(uint8_t x1, uint8_t x2, uint8_t y, char color){
  if(x1 > x2){uint8_t x=x1; x1=x2; x2=x;}
  if(x1 >= N3310_W)return;
  if(x2 >= N3310_W)x2=N3310_W-1;
  if(y >= N3310_H)return;
  int dy = y % 8;
  y /= 8;
  uint8_t mask = (1<<dy);
  uint8_t nmask = ~mask;
  char ccol = color;
  uint8_t *scr = &n3310_screen[x1 + y*N3310_W];
  for(; x1<=x2; x1++){
    if(color == 2)ccol = (x1 & 1);
    if(ccol)scr[0] |= mask; else scr[0] &= nmask;
    scr++;
  }
}

void n3310_vline(uint8_t x, uint8_t y1, uint8_t y2, char color){
  if(y1 > y2){uint8_t y=y1; y1=y2; y2=y;}
  if(y1 >= N3310_H)return;
  if(y2 >= N3310_H)y2=N3310_H-1;
  if(x >= N3310_W)return;
  int dy1 = y1 % 8;
  y1 /= 8;
  int dy2 = y2 % 8;
  y2 /= 8;
  uint8_t *scr = &n3310_screen[x + y1*N3310_W];
  uint8_t c2mask = 0x55;
  if(!(x & 1))c2mask<<=1;
  for(int y=y1; y<=y2; y++){
    uint8_t mask = 0xFF;
    if(y == y1)mask <<= dy1;
    if(y == y2)mask &=~(0xFF00>>(7-dy2));
    if(color == 0){
      scr[0] &=~mask;
    }else if(color == 1){
      scr[0] |= mask;
    }else{
      scr[0] = (scr[0] &~ mask) | (c2mask & mask);
    }
    scr += N3310_W;
  }
}

void n3310_line(int x1, int y1, int x2, int y2, char color){
  int dx = x2-x1;
  int dy = y2-y1;
  int dd;
  if(dx < 0){dx=-dx;}
  if(dy < 0){dy=-dy;}
  if(dx > dy){
    if(x1 > x2){dd=x1; x1=x2; x2=dd; dd=y1; y1=y2; y2=dd;}
    dy=y2-y1;
    dd=1;
    if(dy < 0){dd=-1; dy=-dy;}
    int err = 0;
    int y=y1;
    for(int x=x1; x<=x2; x++){
      err+=dy;
      if(err > dx){
        y+=dd;
        err -= dx;
      }
      if(color == 2){
        n3310_pix(x, y, x & 1);
      }else{
        n3310_pix(x, y, color);
      }
    }
  }else{
    if(y1 > y2){dd=x1; x1=x2; x2=dd; dd=y1; y1=y2; y2=dd;}
    dx=x2-x1;
    dd=1;
    if(dx < 0){dd=-1; dx=-dx;}
    int err = 0;
    int x=x1;
    for(int y=y1; y<=y2; y++){
      err+=dx;
      if(err > dy){
        x+=dd;
        err -= dy;
      }
      if(color == 2){
        n3310_pix(x, y, y & 1);
      }else{
        n3310_pix(x, y, color);
      }
    }
  }
}

#endif

#endif
#endif