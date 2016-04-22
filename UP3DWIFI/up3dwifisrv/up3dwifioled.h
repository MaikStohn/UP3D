/*
  up3dwifioled.h for UP3DWIFI
  M. Stohn 2016
  
  Parts derived from Daniel Eichhorn: http://github.com/squix78/esp8266-oled-ssd1306

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

#include <inttypes.h>

int  oled_init();
void oled_deinit();
void oled_send_fb(const uint8_t *frame);

void oled_fb_clear();
void oled_fb_update();
void oled_fb_setpixel(const int x, const int y, const int mode);
void oled_fb_setfont(const uint8_t* font);
int  oled_fb_getstringwidth(char* text);
void oled_fb_writestring(const int x, const int y, char* text, const int mode);
