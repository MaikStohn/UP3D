/*
  UP3D G-Code transcoder
  M. Stohn 2016

  This is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License.
  If not, see <http://www.gnu.org/licenses/>.
*/

#include "hostplanner.h"
#include "hoststepper.h"
#include "gcodeparser.h"
#include "umcwriter.h"

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>


int main(int argc, char *argv[])
{
  if( argc != 3 )
  {
    printf("Usage: %s input.gcode output.umc\n\n", argv[0]);
    return 0;
  }

  FILE* fgcode = fopen( argv[1], "r" );
  if( !fgcode )
    return -1;

  char line[1024];

  //gcode parsing pass 1 (determine print time)
  umcwriter_init( NULL, 0, 0 );

  gcp_reset();
  while( fgets(line,sizeof(line),fgcode) )
    gcp_process_line(line);

  umcwriter_finish();
  int32_t print_time = umcwriter_get_print_time();

  //gcode parsing pass 2 (write output)
  rewind( fgcode );
  if( !umcwriter_init( argv[2], 123.45, print_time ) )
    return -2;

  gcp_reset();
  while( fgets(line,sizeof(line),fgcode) )
    gcp_process_line(line);

  umcwriter_finish();

  fclose( fgcode );

  printf("Height: %5.2fmm / Layer: %3d / Time: ", gcp_get_height(), gcp_get_layer() );
  int h = print_time/3600; if(h){printf("%dh:",h); print_time -= h*3600;}
  int m = print_time/60; printf("%02dm:",m); print_time -= m*60;
  printf("%02ds\n",print_time);

  return 0; 
}
