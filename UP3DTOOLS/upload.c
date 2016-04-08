//////////////////
//Author: M.Stohn/
//////////////////

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/time.h>

#include "up3d.h"
#include "compat.h"

#define upl_error(s) { printf("ERROR: %s\n",s); }

int main(int argc, char const *argv[])
{
  int block_size = 72; // default number of blocks for USB transfer to SD Card  
  bool bBlock = false;
  
  if (argc == 4)
  	if (strcmp(argv[2], "-b") == 0)
		if (sscanf(argv[3],"%d", &block_size) == 1)
		  argc = 2, bBlock = true; 
		  
  if(argc != 2)
  {
    printf("Usage: %s program.umc [-b block_size]\n\n", argv[0]);
    return 0;
  }

  FILE* fdat = fopen( argv[1], "rb" );
  if( !fdat )
    return 1;

  if( !UP3D_Open() )
    return 2;

  unsigned int version = UP3D_IsPrinterResponsive();
  if( !version )
  {
    upl_error( "UP printer is not responding\n" );
    UP3D_Close();
    return 3;
  }
  
  if (version < 330 & bBlock == false)
  	block_size = 3;  // older firmware cannot handle larger than 64 bytes blocks
  
  UP3D_SetParameter(PARA_RED_BLUE_BLINK,10);

  UP3D_SetParameter(PARA_PRINT_STATUS,0x13); //write to SD

  if( !UP3D_ClearProgramBuf() )
  {
    upl_error( "ClearProgramBuf failed\n" );
    UP3D_Close();
    return 4;
  }
    
  uint8_t program_id = 5;

//  UP3D_SetPrintJobInfo( program_id, 0, 1 );

  if( !UP3D_UseSDProgramBuf( program_id, true ) )
  {
    upl_error( "UseSDProgramBuf failed\n" );
    UP3D_Close();
    return 5;
  }

  long int fpos = 0;
  fseek(fdat, 0, SEEK_END);
  long int flen = ftell(fdat);
  fseek(fdat, 0, SEEK_SET);

  struct timeval tval_start, tval_now, tval_diff;
  gettimeofday(&tval_start, NULL);
  long int usecs = 1;

  uint32_t tblocks = 0;
  for(;;)
  {
    UP3D_BLK blocks[block_size];
    int nblocks = fread( blocks, sizeof(UP3D_BLK), block_size, fdat );
    if( nblocks<=0 )
      break;

    if( !UP3D_WriteBlocks( blocks, nblocks ) )
    {
      upl_error("Write to SD failed");
      UP3D_Close();
      return 6;
    }

    tblocks += nblocks;

    fpos = ftell(fdat);
    gettimeofday(&tval_now, NULL);
    timersub(&tval_now, &tval_start, &tval_diff);
    usecs = (long int)tval_diff.tv_sec*1000000+(long int)tval_diff.tv_usec;

    printf("UPload: %.2f kB sent (%ld%%) [%.2f kB/sec]\r",fpos/1024.0,fpos*100/flen, fpos*(1000000/1024.0)/usecs);
    fflush(stdout);
  }
  printf("\n");

  fclose( fdat );

  uint32_t w;
  for( w=0; w<100; w++ )
  {
    if( UP3D_GetParameter(0xC6) == tblocks )
      break;
    usleep(50*1000); //wait a bit to let printer time to complete write to SD card
  }
  
  if( UP3D_GetParameter(0xC6) != tblocks )
  {
    upl_error("Write to SD not finished");
    UP3D_Close();
    return 7;
  }
  
  UP3D_SetParameter(PARA_RED_BLUE_BLINK,0);

  UP3D_SetParameter(PARA_PRINT_STATUS,0x1); //System ready

//  UP3D_SetPrintJobInfo( program_id, 0, 0 );

  if( !UP3D_ClearProgramBuf() )
  {
    upl_error( "ClearProgramBuf failed\n" );
    UP3D_Close();
    return 8;
  }
  
  if( !UP3D_UseSDProgramBuf( program_id, false ) )
  {
    upl_error( "UseSDProgramBuf failed\n" );
    UP3D_Close();
    return 8;
  }

  if( !UP3D_StartResumeProgram() )
  {
    upl_error( "StartResumeProgram failed\n" );
    UP3D_Close();
    return 9;
  }

  UP3D_Close();
  return 0;
}
