/*
  up3dwifilogo.c for UP3DWIFI
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
#include "up3dwifioled.h"
#include "up3dwifilogo.h"

#include "up3dwififont.h"

int main(int argc, char* argv[])
{
  if( !oled_init() )
  {
    printf("could not find oled\n");
    return 1;
  }

  //send logo
  oled_send_fb( up3dwifilogo );


//TEST
  oled_fb_clear();
  oled_fb_writestring( 0, 0, "Hello World!", 1 );
  oled_fb_writestring( 0,16, "Hello World!", 1 );
  oled_fb_writestring( 0,32, "Hello World!", 1 );
  oled_fb_writestring( 0,48, "Hello World!", 1 );
/*
  oled_fb_setfont(ArialMT_Plain_10);
  oled_fb_writestring( 0,0, "Hello World!", 1 );
  oled_fb_setfont(ArialMT_Plain_16);
  oled_fb_writestring( 0,11, "Hello World!", 1 );
  oled_fb_setfont(DejaVu_Sans_Condensed_Plain_12);
  oled_fb_writestring( 0,30, "Hello World!", 1 );
*/
  oled_fb_update();

  //oled_deinit();
  return 0;
}
