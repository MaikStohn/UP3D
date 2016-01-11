//////////////////
//Author: M.Stohn/
//////////////////

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "up3d.h"

int main(int argc, char const *argv[])
{
  if( argc != 2 )
  {
    printf("Usage: %s program.umc\n\n", argv[0]);
    return 0;
  }

  FILE* fdat = fopen( argv[1], "rb" );
  if( !fdat )
    return 1;

  if( !UP3D_Open() )
    return 2;

  if( !UP3D_IsPrinterResponsive() )
  {
    printf( "UP printer is not responding\n" );
    UP3D_Close();
    return 3;
  }

  if( !UP3D_BeginWrite() )
    return 4;

//  if( !UP3D_SetProgramID( 9, true ) )
//    return 5;

  for(;;)
  {
    UP3D_BLK block;
    if( sizeof(block) != fread( &block, 1, sizeof(block), fdat ) )
      break;

    if( !UP3D_WriteBlocks( &block, 1 ) )
      return 6;
  }

  fclose( fdat );

//  UP3D_GetFreeBlocks(); //??
//  if( !UP3D_BeginWrite() )
//    return 7;
//  if( !UP3D_SetProgramID( 9, false ) )
//    return 8;

  if( !UP3D_Execute() )
    return 9;

  UP3D_Close();
  return 0;
}
