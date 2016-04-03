/*
  umcwriter.cpp for UP3DTranscoder
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

#include "umcwriter.h"
#include "up3ddata.h"
#include "hostplanner.h"
#include "hoststepper.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <math.h>

static FILE* umcwriter_file;
static double umcwriter_Z;
static double umcwriter_Z_height;
static double umcwriter_print_time;
static double umcwriter_print_time_max;

static int _umcwriter_write_file(UP3D_BLK* pblks, uint32_t blks )
{
  if( umcwriter_file )
    return fwrite( pblks, blks, sizeof(UP3D_BLK), umcwriter_file );
  else
    return 0;
}

bool umcwriter_init(const char* filename, double heightZ, double printTimeMax)
{
  umcwriter_Z = 0;
  umcwriter_Z_height = heightZ;
  umcwriter_print_time = 0;
  umcwriter_print_time_max = printTimeMax?printTimeMax:1;
  
  umcwriter_file = NULL;
  if( filename )
  {
    umcwriter_file = fopen(filename,"wb");
    if( !umcwriter_file )
      return false;
  }

  umcwriter_set_report_data( 0, 0 );

  UP3D_BLK blk;
  UP3D_PROG_BLK_Power(&blk,true);
  _umcwriter_write_file(&blk, 1);

  umcwriter_pause(4000); //wait for power on complete and temperature measurement to stabilize

  st_reset();
  plan_reset();

//TODO: move...?
  UP3D_PROG_BLK_SetParameter(&blk,0x41,267);              //? TEMP FOR NOZZLE1 TEMP REACHED
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_SetParameter(&blk,0x42,242);              //? TEMP FOR NOZZLE2 TEMP REACHED
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_SetParameter(&blk,0x43,104);              //? TEMP FOR BED TEMP REACHED
  _umcwriter_write_file(&blk, 1);

// NEEDED?
  UP3D_PROG_BLK_SetParameter(&blk,0x46,13);               //?
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_SetParameter(&blk,0x49,80);               //?
  _umcwriter_write_file(&blk, 1);


  UP3D_PROG_BLK_SetParameter(&blk,PARA_REPORT_LAYER,0);  //layer 0
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_SetParameter(&blk,PARA_REPORT_HEIGHT,0); //height 0
  _umcwriter_write_file(&blk, 1);

  //home all axis (needed for correct print status
  umcwriter_home(0);
  umcwriter_home(1);
  umcwriter_home(2);

  UP3D_PROG_BLK_SetParameter(&blk,PARA_PRINT_STATUS,1); //initialized
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_SetParameter(&blk,0x11,0);              //no support
  _umcwriter_write_file(&blk, 1);

// NEEDED ?
  UP3D_PROG_BLK_SetParameter(&blk,0x35,0);              //check work room fan
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_SetParameter(&blk,0x2A,5000);           //change nozzle time
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_SetParameter(&blk,0x2B,0);              //jump time
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_SetParameter(&blk,0x36,103);            //feedback length ?
  _umcwriter_write_file(&blk, 1);


  UP3D_PROG_BLK_SetParameter(&blk,0x17,0);              //nozzle #1 not open
  _umcwriter_write_file(&blk, 1);

/* PROBLEM...
  UP3D_PROG_BLK_SetParameter(&blk,PARA_PRINT_STATUS,2); //running ==> NOZZLE TO COOL ???
  _umcwriter_write_file(&blk, 1);
*/

// NEEDED?
  UP3D_PROG_BLK_SetParameter(&blk,0x82,255);            //?
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_SetParameter(&blk,0x83,255);            //?
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_SetParameter(&blk,0x1D,100000);         //feed error length
  _umcwriter_write_file(&blk, 1);


/* PROBLEM...
  UP3D_PROG_BLK_SetParameter(&blk,0x1C,1);              //printing...
  _umcwriter_write_file(&blk, 1);
*/
  return true;
}

void umcwriter_finish()
{
  umcwriter_planner_sync();

  umcwriter_print_time += 1.5; //1.5 seconds for end of job

  UP3D_BLK blk;

  umcwriter_beep(200);umcwriter_pause(200);
  umcwriter_beep(200);umcwriter_pause(200);
  umcwriter_beep(200);umcwriter_pause(500);

  UP3D_PROG_BLK_SetParameter(&blk,PARA_REPORT_PERCENT,100);
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_SetParameter(&blk,PARA_REPORT_TIME_REMAIN,0);
  _umcwriter_write_file(&blk, 1);

  UP3D_PROG_BLK_Power(&blk,false);
  _umcwriter_write_file(&blk, 1);

  UP3D_PROG_BLK_SetParameter(&blk,0x1C,0); //not printing...
  _umcwriter_write_file(&blk, 1);

  UP3D_PROG_BLK_SetParameter(&blk,PARA_PRINT_STATUS,0); //not ready
  _umcwriter_write_file(&blk, 1);

  UP3D_PROG_BLK_Stop(&blk);
  _umcwriter_write_file(&blk, 1);

  fclose( umcwriter_file );
  umcwriter_file = NULL;
}

int32_t umcwriter_get_print_time()
{
  return (int32_t)umcwriter_print_time;
}

void umcwriter_home(int32_t axes)
{
  umcwriter_planner_sync();

  double pos[3];
  plan_get_position(pos);

  umcwriter_print_time += 7; //apx. 7 seconds for homeing
  
  UP3D_BLK blks[2];
  if( 0==axes ) {UP3D_PROG_BLK_Home( blks, UP3DAXIS_X ); pos[1] = 0; }
  if( 1==axes ) {UP3D_PROG_BLK_Home( blks, UP3DAXIS_Y ); pos[0] = 0; }
  if( 2==axes ) {UP3D_PROG_BLK_Home( blks, UP3DAXIS_Z ); umcwriter_Z = 0; }
  _umcwriter_write_file(blks, 2);

  umcwriter_planner_set_position( pos[1], pos[0], umcwriter_Z );
}

void umcwriter_virtual_home(double speedX, double speedY, double speedZ)
{
  umcwriter_planner_sync();

  umcwriter_print_time += 5; //apx. 5 seconds for virtual homeing

  UP3D_BLK blks[2];
  UP3D_PROG_BLK_MoveF( blks, -speedY,0, -speedX,0, -speedZ,0, 0,0);
  _umcwriter_write_file( blks, 2);
  umcwriter_planner_set_position(0,0,0);
}

void umcwriter_move_direct(double X, double Y, double Z, double A, double F)
{
  umcwriter_planner_sync();

  //TODO: ??? calc X/Y/Z/E speeds to make linear move
  double feedX = F;// /60;
  double feedY = F;// /60;
  double feedZ = F;// /60;
  double feedA = F;// /60;

  double pos[3];
  plan_get_position(pos);
  double relA = A-pos[A_AXIS];

  double tX = fabs(X+pos[1])/feedX;
  double tY = fabs(Y-pos[0])/feedY;
  double tZ = fabs(umcwriter_Z_height-Z-umcwriter_Z)/feedZ;
  //double tA = fabs(relA)/feedA;

  double t=0;
  if(tX>t) t=tX; if(tY>t) t=tY; if(tZ>t) t=tZ;
  umcwriter_print_time += t;

  UP3D_BLK blks[2];
  UP3D_PROG_BLK_MoveF( blks,-feedY,-Y,-feedX,X,-feedZ,-(umcwriter_Z_height-Z),feedA,relA);
  _umcwriter_write_file(blks, 2);

  umcwriter_planner_set_position(X,Y,A);
  umcwriter_Z = Z;
}

void umcwriter_planner_set_position(double X, double Y, double A)
{
  umcwriter_planner_sync();
  double pos[]={-Y,X,A};
  plan_set_position(pos);
  st_reset();
}

void umcwriter_planner_add(double X, double Y, double A, double F)
{
  while( plan_check_full_buffer() )
  {
    segment_up3d_t *pseg;
    if( !st_get_next_segment_up3d(&pseg) )
      break;
    UP3D_BLK blk;
    UP3D_PROG_BLK_MoveL(&blk,pseg->p1,pseg->p2,pseg->p3,pseg->p4,pseg->p5,pseg->p6,pseg->p7,pseg->p8);
    _umcwriter_write_file(&blk, 1);
  }

  double pos[] = {-Y,X,A};
  double feed = F/60;
  plan_buffer_line( pos, feed, false);
}

void umcwriter_planner_sync()
{
  segment_up3d_t *pseg;
  while( st_get_next_segment_up3d(&pseg) )
  {
    umcwriter_print_time += ((double)pseg->p2*(double)pseg->p1)/F_CPU;

    UP3D_BLK blk;
    UP3D_PROG_BLK_MoveL(&blk,pseg->p1,pseg->p2,pseg->p3,pseg->p4,pseg->p5,pseg->p6,pseg->p7,pseg->p8);
    _umcwriter_write_file(&blk, 1);
  }
}

void umcwriter_set_extruder_temp(double temp, bool wait)
{
  umcwriter_planner_sync();

  UP3D_BLK blk;

  if( temp )
  {
    //TODO: always ???
    UP3D_PROG_BLK_SetParameter(&blk,0x17,1);              //nozzle #1 open
    _umcwriter_write_file(&blk, 1);
  }

  UP3D_PROG_BLK_SetParameter(&blk,PARA_NOZZLE1_TEMP,temp);
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_SetParameter(&blk,PARA_HEATER_NOZZLE1_ON,(temp)?1:0);
  _umcwriter_write_file(&blk, 1);

  if(wait)
  {
    umcwriter_print_time += 120; //apx. 2 minutes
  
    UP3D_PROG_BLK_SetParameter(&blk,PARA_RED_BLUE_BLINK,100);
    _umcwriter_write_file(&blk, 1);

    UP3D_PROG_BLK_WaitIfNot( &blk, PARA_TEMP_REACHED_N1, 1, '=' );
    _umcwriter_write_file(&blk, 1);

    UP3D_PROG_BLK_SetParameter(&blk,PARA_RED_BLUE_BLINK,200);
    _umcwriter_write_file(&blk, 1);
  }
}

void umcwriter_set_bed_temp(int32_t temp, bool wait)
{
  umcwriter_planner_sync();

  UP3D_BLK blk;
  UP3D_PROG_BLK_SetParameter(&blk,PARA_BED_TEMP,temp);
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_SetParameter(&blk,PARA_HEATER_BED_ON,(temp)?1:0);
  _umcwriter_write_file(&blk, 1);

  if(wait)
  {
    UP3D_PROG_BLK_SetParameter(&blk,PARA_RED_BLUE_BLINK,400);
    _umcwriter_write_file(&blk, 1);

#ifdef X_WAIT_HEATBED_FACTOR
    //TODO: better wait... idea: move nozzle to bed and use thermistor of nozzle
    if(temp>50)
    {
      umcwriter_pause(temp/10*60*1000); //wait 5,6,7,8,9,10 (50,60,70,80,90C) minutes
      umcwriter_print_time += temp/10*60;
    }
#else
    umcwriter_pause(3*60*1000); //wait 3 minute
    umcwriter_print_time += 3*60;
#endif

    UP3D_PROG_BLK_SetParameter(&blk,PARA_RED_BLUE_BLINK,200);
    _umcwriter_write_file(&blk, 1);
  }
}

void umcwriter_set_report_data(int32_t layer, double height)
{
  int32_t percent = (umcwriter_print_time*100)/umcwriter_print_time_max;
  int32_t seconds_remaining = umcwriter_print_time_max - umcwriter_print_time;

  UP3D_BLK blk;
  UP3D_PROG_BLK_SetParameter(&blk,PARA_REPORT_LAYER,layer);
  _umcwriter_write_file(&blk, 1);
  float h = (float)height;
  UP3D_PROG_BLK_SetParameter(&blk,PARA_REPORT_HEIGHT,*((int32_t*)&h));
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_SetParameter(&blk,PARA_REPORT_PERCENT,percent);
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_SetParameter(&blk,PARA_REPORT_TIME_REMAIN,seconds_remaining);
  _umcwriter_write_file(&blk, 1);
}
 
void umcwriter_pause(uint32_t msec)
{
  umcwriter_planner_sync();

  umcwriter_print_time += msec/1000.0;

  UP3D_BLK blk;
  UP3D_PROG_BLK_Pause(&blk,msec);
  _umcwriter_write_file(&blk, 1);
}

void umcwriter_beep(uint32_t msec)
{
  umcwriter_planner_sync();

  UP3D_BLK blk;
  UP3D_PROG_BLK_Beeper(&blk,true);
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_Pause(&blk,msec);
  _umcwriter_write_file(&blk, 1);
  UP3D_PROG_BLK_Beeper(&blk,false);
  _umcwriter_write_file(&blk, 1);
}

void umcwriter_user_pause()
{
  umcwriter_planner_sync();

  umcwriter_print_time += 2; //2 seconds for processing

  UP3D_BLK blks[2];
  UP3D_PROG_BLK_MoveF( blks,150,0,150,0,10000,30,10000,0 );
  _umcwriter_write_file(blks, 2);

  umcwriter_beep(500);

  UP3D_BLK blk;
  
  UP3D_PROG_BLK_SetParameter(&blk,PARA_PAUSE_PROGRAM,1);
  _umcwriter_write_file(&blk, 1);

  UP3D_PROG_BLK_SetParameter(&blk,PARA_PRINT_STATUS,3);
  _umcwriter_write_file(&blk, 1);

  umcwriter_beep(500);

  UP3D_PROG_BLK_MoveF( blks,150,0,150,0,10000,-30,10000,0 );
  _umcwriter_write_file(blks, 2);
}
