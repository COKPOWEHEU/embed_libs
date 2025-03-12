#ifndef __OLED_SSD1306__
#define __OLED_SSD1306__

#if 0==1

#define OLED_MODE OLED_128x32
//#define OLED_MIR_X
#define OLED_MIR_Y
#define OLED_CHARS_CP1251 //<nothing> or OLED_CHARS_ALNUM or OLED_CHARS_CP1251
//#define OLED_GRAPHICS //enable rectangles and lines (variable angles)
#define OLED_LINES //enable rectangles (Y in units of 8) and horizontal/vertical lines
#define OLED_CHARS_ABS_POS //enable various place of chars
#include "oled_ssd1306.h"
...
oled_init();
oled_clear();
oled_str(x, y, font, str);
oled_rect(x1, y1, x2, y2, color); //color=0->clear, color=1->set
oled_pix(x, y, color);
oled_line(x1, y1, x2, y2, color); //color=0->clear, color=1->set, color=2->dotted
oled_hline(x1, x2, y, color); //horizontal line
oled_vline(x, y1, y2, color); //vertical line
...
while(1){
  ...
  oled_update();
  if(oled_test_leak())error();
}
#endif

#define OLED_128x32	1
#define OLED_96x16	2

#ifndef OLED_MODE
  #warning 'OLED_MODE' not defined. Coorect values are: 'OLED_128x32', 'OLED_96x16'
  #define OLED_MODE OLED_128x32
#endif

#if OLED_MODE == OLED_128x32
  #define OLED_SCR_W	128
  #define OLED_SCR_H	32
  #define OLED_22_PAGE_START	0
#elif OLED_MODE == OLED_96x16
  #define OLED_SCR_W	96
  #define OLED_SCR_H	16
  #define OLED_22_PAGE_START	2
#else
  #error 'OLED_MODE' contains incorrect value
#endif

#ifdef OLED_MIR_X
  #define OLED_CMD_SEG_REMAP_X (OLED_CMD_SEG_REMAP | 1)
#else
  #define OLED_CMD_SEG_REMAP_X (OLED_CMD_SEG_REMAP | 0)
#endif
#ifdef OLED_MIR_Y
  #define OLED_CMD_STARTLINE_Y (OLED_CMD_STARTLINE | 32)
  #define OLED_CMD_COM_SCAN_Y (OLED_CMD_COM_SCAN | 0)
#else
  #define OLED_CMD_STARTLINE_Y (OLED_CMD_STARTLINE | 0)
  #define OLED_CMD_COM_SCAN_Y (OLED_CMD_COM_SCAN | 8)
#endif


#define I2C_ADDR_OLED	0x3C

#define OLED_CMD_CLOCK	0xD5
#define OLED_CMD_CHARGEPUMP	0x8D
#define OLED_CMD_STARTLINE	0x40
#define OLED_CMD_SEG_REMAP	0xA0 //MIR_X?
#define OLED_CMD_COM_SCAN	0xC0 // |8
#define OLED_CMD_COM_HW		0xDA //next data=(bit4=, bit5=), TODO
#define OLED_CMD_MUX_RATIO	0xA8
#define OLED_CMD_DISP_OFFSET	0xD3
#define OLED_CMD_ADDR_MODE	0x20 //data=0-horizontal, 1=vertical, 2-page
#define OLED_CMD_XAREA		0x21 //data0=startX, data1=endX
#define OLED_CMD_YAREA		0x22 //data0=startY, data1=endY
#define OLED_CMD_CONTRAST	0x81 //data0=contrast(0-255)
#define OLED_CMD_ENT_DISP_ON	0xA4 // |0-off, |1-on
#define OLED_CMD_INVERSE	0xA6 // |0-normal, |1-inverse
#define OLED_CMD_DISP_ON	0xAE

//#define OLED_LEAK_TEST
//OLED_CMD_STARTLINE | 32 + OLED_CMD_COM_SCAN | 0 -> mir_Y

static struct{
  uint8_t cmd;
  uint8_t buf[ OLED_SCR_W*OLED_SCR_H/8 ];
#ifdef OLED_LEAK_TEST
  uint8_t reserved[100];
#endif
}oled_scrcmd;
#define oled_screen (oled_scrcmd.buf)

void oled_send(uint8_t *buf, uint16_t count){
  i2cm_start(I2C_ADDR_OLED, buf, count, NULL, 0, 1*count);
  i2cm_wait();
  //delay_ms(10);
}

char oled_init(){
  uint8_t data[7];
  // Set display off
  data[0] = 0x00; //OLED command flag
  data[1] = 0xAE;
  oled_send(data, 2);
  if(i2cm_isready() != I2CM_READY)return 1;
  
  // Set oscillator frequency
  data[1] = OLED_CMD_CLOCK;
  data[2] = 0x80;
  oled_send(data, 3);
  
  // Enable charge pump regulator
  data[1] = OLED_CMD_CHARGEPUMP;
  data[2] = 0x14;
  oled_send(data, 3);
  // Set display start line
  data[1] = OLED_CMD_STARTLINE_Y;
  oled_send(data, 2);
  
  // Set segment remap (поворот дисплея?)
  data[1] = OLED_CMD_SEG_REMAP_X;
  oled_send(data, 2);
  
  // Set COM output scan direction (поворот дисплея?)
  data[1] = OLED_CMD_COM_SCAN_Y;
  oled_send(data, 2);
  
  // Set COM pins hardware configuration
  data[1] = OLED_CMD_COM_HW;
  data[2] = 0x02 | (1<<5)|(0<<4);
  oled_send(data, 3);
  
  // Set MUX ratio
  data[1] = OLED_CMD_MUX_RATIO;
  data[2] = 63;
  oled_send(data, 3);
  
  // Set display offset
  data[1] = OLED_CMD_DISP_OFFSET;
  data[2] = 0;
  oled_send(data, 3);
  
  // Set horizontal addressing mode
  data[1] = OLED_CMD_ADDR_MODE;
  data[2] = 0x00; //00-horizontal, 01-vertical, 10-page(?)
  oled_send(data, 3);
  
  // Set column address
  data[0] = 0x00; //OLED command flag
  data[1] = 0x21;
  data[2] = 0;
  data[3] = (OLED_SCR_W-1);
  oled_send(data, 4);
  
  // Set page address
  data[1] = 0x22;
  data[2] = OLED_22_PAGE_START;
  data[3] = data[2] + (OLED_SCR_H/8-1);
  oled_send(data, 4);
  
  // Set contrast
  data[1] = OLED_CMD_CONTRAST;
  data[2] = 0x7F;
  oled_send(data, 3);
  // Entire display on
  data[1] = OLED_CMD_ENT_DISP_ON | 0;
  oled_send(data, 2);
  //Set normal display
  data[1] = OLED_CMD_INVERSE | 0;
  oled_send(data, 2);
  
  
  // Set display on
  data[1] = OLED_CMD_DISP_ON | 1;
  oled_send(data, 2);
  
#ifdef OLED_LEAK_TEST
  for(int i=0; i<sizeof(oled_scrcmd.reserved); i++)oled_scrcmd.reserved[i] = 0;
#endif
  
  return 0;
}

void oled_clear(){
  for(uint32_t i=0; i<sizeof(oled_screen); i++)oled_screen[i] = 0;
}

void oled_update(){
  i2cm_wait();
  uint8_t data[4];
  // Set column address
  data[0] = 0x00; //OLED command flag
  data[1] = OLED_CMD_XAREA;
  data[2] = 0;
  data[3] = (OLED_SCR_W-1);
  oled_send(data, 4);
  
  // Set page address
  data[1] = OLED_CMD_YAREA;
  data[2] = OLED_22_PAGE_START;
  data[3] = data[2] + (OLED_SCR_H/8-1);//CMD22_3;
  oled_send(data, 4);
  oled_scrcmd.cmd = 0x40;
#ifdef OLED_LEAK_TEST
  i2cm_start(I2C_ADDR_OLED, (uint8_t*)&oled_scrcmd, sizeof(oled_scrcmd)-sizeof(oled_scrcmd.reserved), NULL, 0, 100);
#else
  i2cm_start(I2C_ADDR_OLED, (uint8_t*)&oled_scrcmd, sizeof(oled_scrcmd), NULL, 0, 100);
#endif
}

#define oled_wait() i2cm_wait()
#define oled_ready() (i2cm_isready() != I2CM_WAIT)

#ifdef OLED_LEAK_TEST
  char oled_test_leak(){
    for(int i=0; i<sizeof(oled_scrcmd.reserved); i++)if(oled_scrcmd.reserved[i] != 0)return 1;
    return 0;
  }
#else
  #define oled_test_leak() (0)
#endif

#if (defined OLED_CHARS_ALNUM) || (defined OLED_CHARS_CP1251)

#ifdef OLED_CHARS_CP1251
  #define LCD_CHARS_FULL_TABLE
  #define _char_table(i) char_table[i]
#else
  #define _char_table(i) char_table[i-32]
#endif
#include "lcd_chars_cp1251.h"

#ifndef OLED_CHARS_ABS_POS
  #define oled_scr_mod(pos, ch) oled_screen[pos] = ch
#else
  #define oled_scr_mod(pos, ch) do{\
    oled_screen[pos] = (oled_screen[pos] & m1) | (ch << dy); \
    oled_screen[pos+OLED_SCR_W] = (oled_screen[pos+OLED_SCR_W] & m2) | (ch >> (8-dy)); \
  }while(0)
#endif

void oled_ch1(uint32_t pos, uint8_t dy, char _ch){
  uint8_t ch = _ch;
  #ifndef OLED_CHARS_FAST
    uint8_t m1 = ~(0xFF << dy);
    uint8_t m2 = ~(0xFF >> (8-dy));
  #endif
  for(int i=0; i<CHAR_W; i++, pos++){
    char c = _char_table(ch)[i];
    oled_scr_mod(pos, c);
  }
  oled_scr_mod(pos, 0);
}
uint32_t sym_dup2(uint8_t ch){
  uint32_t sym = ch;                          //        abcdefgh
  sym = (sym | (sym<<4)) & 0b0000111100001111;//    abcd    efgh
  sym = (sym | (sym<<2)) & 0b0011001100110011;//  ab  cd  ef  gh
  sym = (sym | (sym<<1)) & 0b0101010101010101;// a b c d e f g h
  sym = (sym | (sym<<1));
  return sym;
}
void oled_ch2(uint32_t pos, uint8_t dy, char _ch){
  uint8_t ch = _ch;
  #ifndef OLED_CHARS_FAST
    uint8_t m1 = ~(0xFF << dy);
    uint8_t m2 = ~(0xFF >> (8-dy));
  #endif
  uint16_t sym;
  for(int i=0; i<CHAR_W; i++){
    sym = sym_dup2(_char_table(ch)[i]);
    for(int j=0; j<2; j++){
      oled_scr_mod(pos+OLED_SCR_W*0, (sym & 0xFF));
      oled_scr_mod(pos+OLED_SCR_W*1, ((sym >> 8)&0xFF));
      pos++;
    }
  }
  for(int j=0; j<2; j++){
    oled_scr_mod(pos+OLED_SCR_W*0, 0);
    oled_scr_mod(pos+OLED_SCR_W*1, 0);
    pos++;
  }
}
uint32_t sym_dup3(uint8_t ch){
  uint32_t sym = ch;                                          //                        abcdefgh
  sym = (sym | (sym<<8)) & 0b00000000000000001111000000001111;//                abcd        efgh
  sym = (sym | (sym<<4)) & 0b00000000000011000011000011000011;//            ab    cd    ef    gh
  sym = (sym | (sym<<2)) & 0b00000000001001001001001001001001;//          a  b  c  d  e  f  g  h
  sym = (sym | (sym<<1) | (sym<<2));
  return sym;
}
void oled_ch3(uint32_t pos, uint8_t dy, char _ch){
  uint8_t ch = _ch;
  #ifndef OLED_CHARS_FAST
    uint8_t m1 = ~(0xFF << dy);
    uint8_t m2 = ~(0xFF >> (8-dy));
  #endif
  uint32_t sym;
  for(int i=0; i<CHAR_W; i++){
    sym = sym_dup3(_char_table(ch)[i]);
    for(int j=0; j<3; j++){
      oled_scr_mod(pos+OLED_SCR_W*0, ((sym >> 0)&0xFF));
      oled_scr_mod(pos+OLED_SCR_W*1, ((sym >> 8)&0xFF));
      oled_scr_mod(pos+OLED_SCR_W*2, ((sym >>16)&0xFF));
      pos++;
    }
  }
  for(int j=0; j<3; j++){
    oled_scr_mod(pos+OLED_SCR_W*0, 0);
    oled_scr_mod(pos+OLED_SCR_W*1, 0);
    oled_scr_mod(pos+OLED_SCR_W*2, 0);
    pos++;
  }
}
uint32_t sym_dup4(uint8_t ch){
  uint32_t sym = ch;                                          //                        abcdefgh
  sym = (sym | (sym<<12))& 0b00000000000011110000000000001111;//            abcd            efgh
  sym = (sym | (sym<<6)) & 0b00000011000000110000001100000011;//      ab      cd      ef      gh
  sym = (sym | (sym<<3)) & 0b00010001000100010001000100010001;//   a   b   c   d   e   f   g   h
  sym = (sym | (sym<<1) | (sym<<2) | (sym<<3));
  return sym;
}
void oled_ch4(uint32_t pos, uint8_t dy, char _ch){
  uint8_t ch = _ch;
  #ifndef OLED_CHARS_FAST
    uint8_t m1 = ~(0xFF << dy);
    uint8_t m2 = ~(0xFF >> (8-dy));
  #endif
  uint32_t sym;
  for(int i=0; i<CHAR_W; i++){
    sym = sym_dup4(_char_table(ch)[i]);
    for(int j=0; j<4; j++){
      oled_scr_mod(pos+OLED_SCR_W*0, ((sym >> 0)&0xFF));
      oled_scr_mod(pos+OLED_SCR_W*1, ((sym >> 8)&0xFF));
      oled_scr_mod(pos+OLED_SCR_W*2, ((sym >>16)&0xFF));
      oled_scr_mod(pos+OLED_SCR_W*3, ((sym >>24)&0xFF));
      pos++;
    }
  }
  for(int j=0; j<4; j++){
    oled_scr_mod(pos+OLED_SCR_W*0, 0);
    oled_scr_mod(pos+OLED_SCR_W*1, 0);
    oled_scr_mod(pos+OLED_SCR_W*2, 0);
    oled_scr_mod(pos+OLED_SCR_W*3, 0);
    pos++;
  }
}
typedef void (*oled_ch_func)(uint32_t pos, uint8_t dy, char _ch);
uint32_t oled_strpos = 0;

void oled_str(int x, int y, uint8_t font, char *str){
  const oled_ch_func chn[] = {oled_ch1, oled_ch2, oled_ch3, oled_ch4};
  oled_ch_func chx = chn[font-1];
  if((x>=0)&&(y>=0)){
    oled_strpos = x*8 + (y & ~7)*OLED_SCR_W + (y&7);
  }
  uint32_t pos = oled_strpos / 8;
  uint8_t dy = oled_strpos % 8;
  uint32_t dpos = (CHAR_W+1)*font;
  if( (pos + ((font-1)+(dy!=0))*OLED_SCR_W) > sizeof(oled_screen))return;
  while(str[0]){
    if( (x+dpos) >= OLED_SCR_W)return;
    chx(pos, dy, str[0]);
    pos += dpos;
    x += dpos;
    str++;
  }
  oled_strpos = pos*8 + dy;
}

#endif //OLED_CHARS

#ifdef OLED_GRAPHICS
#define OLED_LINES
void oled_rect(int x1, int y1, int x2, int y2, char color){
  int y = y1 & ~7;
  int yend = (y2+8) & ~7;
  if(x1 < 0)x1=0;
  if(x2 >= OLED_SCR_W)x2=(OLED_SCR_W-1);
  if(y < 0)y=0;
  if(yend >= OLED_SCR_H)yend=(OLED_SCR_H);
  for(; y<yend; y+=8){
    int ys = y1-y; if(ys<0)ys=0;
    int ye = y+8-y2; if(ye<0)ye=0;
    uint32_t mask = 0xFF;
    mask >>= (ys+ye);
    mask <<= ys;
    if(color == 0)mask = ~mask;
    int pos = x1 + (y/8)*OLED_SCR_W;
    for(int x=x1; x<=x2; x++){
      if(color == 1){
        oled_screen[pos] |= mask;
      }else{
        oled_screen[pos] &= mask;
      }
      pos++;
    }
  }
}

//0-clear, 1-set, 2-dots
void oled_pix(int x, int y, int col){
  uint8_t ym = (1<<y%8);
  int pos = x + (y/8)*OLED_SCR_W;
  if((x < 0)||(x>=OLED_SCR_W)||(y<0)||(y>OLED_SCR_H))return;
  if(col){
    oled_screen[pos] |= ym;
  }else{
    oled_screen[pos] &=~ ym;
  }
}
void oled_line(int x1, int y1, int x2, int y2, int color){
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
        oled_pix(x, y, x & 1);
      }else{
        oled_pix(x, y, color);
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
        oled_pix(x, y, y & 1);
      }else{
        oled_pix(x, y, color);
      }
    }
  }
}
#endif //OLED_GRAPHICS

#ifdef OLED_LINES
void oled_hline(int x1, int x2, int y, char color){
  if((y<0)||(y>=OLED_SCR_H))return;
  if(x1 < 0)x1=0;
  if(x2 >= OLED_SCR_W)x2=OLED_SCR_W-1;
  int pos = x1 + (y/8)*OLED_SCR_W;
  uint32_t mask = (1<<(y%8));
  uint32_t nmask = ~mask;
  for(;x1<=x2; x1++, pos++){
    if(color & 1){
      oled_screen[pos] |= mask;
    }else{
      oled_screen[pos] &= nmask;
    }
    color ^= (color >> 1);
  }
}
#ifdef OLED_GRAPHICS
void oled_vline(int x, int y1, int y2, char color){
  if((x<0)||(x>=OLED_SCR_W))return;
  int y = y1 & ~7;
  int yend = (y2+8) & ~7;
  if(y < 0)y=0;
  if(yend >= OLED_SCR_H)yend=(OLED_SCR_H);
  int pos = x + y*(OLED_SCR_W/8);
  for(; y<yend; y+=8, pos+=OLED_SCR_W){
    int ys = y1-y; if(ys<0)ys=0;
    int ye = y+8-y2; if(ye<0)ye=0;
    uint32_t mask = 0xFF;
    mask >>= (ys+ye);
    mask <<= ys;
    int pos = x + (y/8)*OLED_SCR_W;
    if(color == 1){
      oled_screen[pos] |= mask;
    }else if(color == 0){
      oled_screen[pos] &=~mask;
    }else if(color == 2){
      oled_screen[pos] |= (mask & 0xAA);
      oled_screen[pos] &=~(mask & 0x55);
    }
  }
}
#else//OLED_GRAPHICS
void oled_vline(int x, int y1, int y2, char color){
  if((x<0)||(x>=OLED_SCR_W))return;
  int y = y1/8;
  int yend = y2/8+1;
  if(y < 0)y=0;
  if(yend >= OLED_SCR_H/8)yend=(OLED_SCR_H/8);
  int pos = x + y*OLED_SCR_W;
  uint8_t mask = 0;
  if(color==1){
    mask = 0xFF;
  }else if(color == 2){
    mask = 0xAA;
  }
  for(; y<yend; y++, pos+=OLED_SCR_W){
    oled_screen[pos] = mask;
  }
}
void oled_rect(int x1, int y1, int x2, int y2, char color){
  int y = y1/8;
  int yend = y2/8+1;
  if(x1 < 0)x1=0;
  if(x2 >= OLED_SCR_W)x2=(OLED_SCR_W-1);
  if(y < 0)y=0;
  if(yend >= OLED_SCR_H/8)yend=(OLED_SCR_H/8);
  int pos = x1 + y*(OLED_SCR_W/8);
  uint8_t mask = 0;
  if(color){
    mask = 0xFF;
  }
  for(; y<yend; y++){
    pos = x1 + y*OLED_SCR_W;
    for(int x=x1; x<=x2; x++, pos++){
      oled_screen[pos] = mask;
    }
  }
}
#endif//OLED_GRAPHICS

#endif //OLED_LINES

#endif