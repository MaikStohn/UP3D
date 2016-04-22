/*
  up3dwifioled.c for UP3DWIFI
  M. Stohn 2016

  This is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  If not, see <http://www.gnu.org/licenses/>.
*/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <inttypes.h>

#ifdef __linux__
 #include <fcntl.h>
 #include <linux/i2c-dev.h>
 #include <errno.h>
 #include <sys/ioctl.h>
#endif

#include "up3dwifioled.h"
#include "up3dwififont.h"

static int      _fd_oled;
static uint8_t  _framebuffer[128*64/8];
static uint8_t* _oled_fb_font;

#define WIDTH_POS 0
#define HEIGHT_POS 1
#define FIRST_CHAR_POS 2
#define CHAR_NUM_POS 3
#define CHAR_WIDTH_START_POS 4

static int _oled_cmd(const int fd, const uint8_t cmd)
{
#ifdef __linux__
  uint8_t buf[2] = {0x80,cmd};
  return write( fd, buf, sizeof(buf) );
#else
  return -1;
#endif
}

static int _oled_dat(const int fd, const uint8_t* dat, const uint8_t len )
{
#ifdef __linux__
  if( len>255 )
    return -1;

  uint8_t buf[256] = {0x40};
  memcpy( buf+1, dat, len );
    return write( fd, buf, 1+len );
#else
  return -1;
#endif
}

int oled_init()
{
#ifdef __linux__
  _fd_oled = open( "/dev/i2c-0", O_RDWR );
  if( _fd_oled<0 )
    return 0;

  if( ioctl(_fd_oled, I2C_SLAVE, 0x3C) < 0 )
    return 0;

  //init sequence
  _oled_cmd(_fd_oled,0x8D); _oled_cmd(_fd_oled,0x14); //charge pump on
  _oled_cmd(_fd_oled,0x20), _oled_cmd(_fd_oled,0x00); //memory address mode page
  _oled_cmd(_fd_oled,0xC8);                           //com scan dir (flip y)
  _oled_cmd(_fd_oled,0xA1);                           //seg scan dir (flip x)
  oled_fb_clear();                                    //clear frambuffer
  oled_fb_update();                                   //send framebuffer to screen
  _oled_cmd(_fd_oled,0xAF);                           //display on
  return 1;
#else
  return 0;
#endif
}

void oled_deinit()
{
#ifdef __linux__
  _oled_cmd(_fd_oled,0xAE);                           //display off
  _oled_cmd(_fd_oled,0x8D); _oled_cmd(_fd_oled,0x10); //charge pump off

  close(_fd_oled);
#endif
}

void oled_send_fb(const uint8_t *frame)
{
  uint32_t p;
  for( p=0; p<8; p++ )
  {
    _oled_cmd(_fd_oled, 0xB0 | p);                   //set page
    _oled_cmd(_fd_oled, 0x02 );                      //set write address lo (0 for SSD1306 / 2 for SH1106)
    _oled_cmd(_fd_oled, 0x10 );                      //set write address high
    _oled_dat(_fd_oled, &frame[p<<7], 64);           //send first 64 bytes (tiny-usb-i2c can only handle 64 bytes)
    _oled_dat(_fd_oled, &frame[(p<<7)+64], 64);      //send second 64 bytes
  }
}

void oled_fb_update()
{
  oled_send_fb( _framebuffer );
}

void oled_fb_clear()
{
  memset( _framebuffer, 0, sizeof(_framebuffer) );
}

void oled_fb_setpixel(const int x, const int y, const int mode)
{
  if( (x>=0) && (x<128) && (y>=0) && (y<64) )
  {
    switch( mode )
    {
      case 0: //BLACK (UNSET)
        _framebuffer[((y<<4)&~0x7f)|x] &= (1<<(y&7)); break;
      case 2: //INVERT
        _framebuffer[((y<<4)&~0x7f)|x] ^= (1<<(y&7)); break;
      default://WHITE (SET)
        _framebuffer[((y<<4)&~0x7f)|x] |= (1<<(y&7)); break;
    }
  }
}

void oled_fb_setfont(const uint8_t* font)
{
  _oled_fb_font = (uint8_t*)font;
}

int oled_fb_getstringwidth(char* text)
{
  int width = 0;
  for( ;*text; text++ )
    width += _oled_fb_font[CHAR_WIDTH_START_POS + (*text - _oled_fb_font[FIRST_CHAR_POS])];
  return width;
}

int oled_fb_writechar(const int x, const int y, const uint8_t ch, const int mode)
{
  int i, j, width, astart, aend;
  unsigned int index, dat;
  uint8_t* pData;
  uint8_t c = ch;

  if( (c<FONT_MYFONT_FIRST_CH) || (c>=FONT_MYFONT_LAST_CH) )
    return 0;
  c -= FONT_MYFONT_FIRST_CH;

  index = 0;
  pData = (uint8_t*)FONT_MYFONT_WIDTHS;

  for( i=0; i<c; i++)
  {
    dat = *pData++;
    if(dat&0xF)
      index += (dat&0xF)*(FONT_MYFONT_HEIGHT-(dat>>6)-((dat>>4)&3));
    else
      i += (dat&0xf0)?*pData++:dat>>4;
  }

  width = (*pData)&0xF;
  astart = (*pData)>>6;
  aend = ((*pData)>>4)&3;
  pData = (uint8_t*)FONT_MYFONT_BITS + (index>>3);
  dat = ((*pData++)|0x100)<<(index&7);

  for( j=x; j<x+width; j++)
  {
    for( i=y+astart; i<y+FONT_MYFONT_HEIGHT-aend; i++) 
    {
      if( dat&0x80 )
        oled_fb_setpixel( j, i, mode );
      dat <<= 1;
      if( dat & 0x10000 )
        dat = (*pData++)|0x100;
    }
  }
  return width+1;
}

void oled_fb_writestring(const int x, const int y, char* text, const int mode)
{
  int sx = x;
  for( ;*text; text++ )
    sx += oled_fb_writechar( sx, y, *text, mode );
}

