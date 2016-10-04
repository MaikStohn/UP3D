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

void print_usage_and_exit()
{
  printf("Usage: up3dtranscode machinetype input.gcode output.umc nozzleheight\n\n");
  printf("          machinetype:  mini / classic / plus / box / cetus\n");
  printf("          input.gcode:  g-code file from slic3r/cura/simplify\n");
  printf("          output.umc:   up machine code file which will be generated\n");
  printf("          nozzleheight: nozzle distance from bed (e.g. 123.45)\n\n");
  exit(0);
}

int main(int argc, char *argv[])
{
  if( 5 != argc )
    print_usage_and_exit();

  switch( argv[1][0] )
  {
    case 'm': //mini
      memcpy( &settings, &settings_mini, sizeof(settings) );
      break;

    case 'c': //classic
      if( 'e' == argv[1][1] ) // cetus
      {
        memcpy( &settings, &settings_cetus, sizeof(settings) );
        break;
      }
      //fall through
    case 'p': //plus
      memcpy( &settings, &settings_classic_plus, sizeof(settings) );
      break;
  
    case 'b': //box
      memcpy( &settings, &settings_box, sizeof(settings) );
      break;

    default:
      printf("ERROR: Uknown machine type: %s\n\n",argv[1] );
      print_usage_and_exit();
  }

  double nozzle_height;
  if( 1 != sscanf(argv[4],"%lf", &nozzle_height) )
  {
    printf("ERROR: Invalid nozzle height: %s\n\n", argv[4]);
    print_usage_and_exit();
  }

  FILE* fgcode = fopen( argv[2], "r" );
  if( !fgcode )
  {
    printf("ERROR: Could not open %s for reading\n\n", argv[2]);
    print_usage_and_exit();
  }

  if( !umcwriter_init( argv[3], nozzle_height, argv[1][0] ) )
  {
    printf("ERROR: Could not open %s for writing\n\n", argv[3]);
    print_usage_and_exit();
  }

  gcp_reset();

  char line[1024];
  while( fgets(line,sizeof(line),fgcode) )
    if( !gcp_process_line(line) )
      return 0;

  umcwriter_finish();
  int32_t print_time = umcwriter_get_print_time();

  fclose( fgcode );

  printf("Height: %5.2fmm / Layer: %3d / Time: ", gcp_get_height(), gcp_get_layer() );
  int h = print_time/3600; if(h){printf("%dh:",h); print_time -= h*3600;}
  int m = print_time/60; printf("%02dm:",m); print_time -= m*60;
  printf("%02ds",print_time);
  printf(" / Nozzle Height: %.2fmm\n", nozzle_height);

  return 0;
}
