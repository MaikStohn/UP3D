///////////////////////////////////////////////////////
// Author: K.Scheffer based on upshell from M. Stohn //
//
// Date: 18.Sep.2016
//
// live captures the position reporting of the printer
// and translates it to g-code
//
//////////////////////////////////////////////////////

#include <stdlib.h>
#ifdef _WIN32
#include <ncurses/curses.h>
#else
#include <curses.h>
#endif
#include <signal.h>
#include <string.h>
#include <sys/time.h>

#include <stdbool.h>
#include <stdint.h>

#include <stdio.h>
#include <math.h>
#include <unistd.h>

#include "compat.h"
#include "up3d.h"

#define upl_error(s) { printf("ERROR: %s\n",s); }


#define MAX_F (200*60)    // mm / min
#define LIMIT_F(f) ( (f) > MAX_F ? MAX_F:(f))

#define MAX_A (150*60)   // mm/s2
#define LIMIT_A(a) ( (a) > MAX_A ? MAX_A:(a))

// smales angle we can discriminate
// is defined by the max travel speed of the printer (200mm/s)
// and the sample periode (1ms)
// and the step resolution (1/854 or 1/160 on the cetus printer)
#define MIN_ANGLE (M_PI/3600)  // we give it a try with this
#define TO_DEG(rad) (((rad) > 0 ? (rad) : (2*M_PI + (rad))) * 360 / (2*M_PI))

float steps[4]; // steps per mm for each axis from printer info


float timedifference_msec(struct timeval t0, struct timeval t1)
{
    return (t1.tv_sec - t0.tv_sec) * 1000.0f + (t1.tv_usec - t0.tv_usec) / 1000.0f;
} 


#define CAPTURE_STATES

/*
 * captures the position and state information from the printer
 * and prints it out to stdout.
 *
 * input   : continues time counter in milliseconds
 * returns : machine state
 */  
static int32_t capture(double time)
{
  static bool init = false;
  static double t0;
    
  if (init == false)
  {
    init = true;
    t0 = time;
#ifdef CAPTURE_STATES
    printf("t,x,y,z,e,mstat,pstat,sstat\n"); 
#else
    printf("t,x,y,z,e\n"); 
#endif
  }
  double interval_ms = (time - t0);
  t0 = time;
  
  int32_t xpos = UP3D_GetAxisPosition(1);
  float x = (float)xpos / steps[0];

  int32_t ypos = UP3D_GetAxisPosition(2);
  float y = (float)ypos / steps[1];

  int32_t zpos = UP3D_GetAxisPosition(3);
  float z = (float)zpos / steps[2];

  int32_t epos = UP3D_GetAxisPosition(4);
  float e = (float)epos / steps[3];
  
#ifdef CAPTURE_STATES
  int32_t mstat = UP3D_GetMachineState();
  int32_t pstat = UP3D_GetProgramState();
  int32_t sstat = UP3D_GetSystemState();
  printf("%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%s,%s,%s\n",
    time, x,y,z,e,
    UP3D_STR_MACHINE_STATE[mstat],
    UP3D_STR_PROGRAM_STATE[pstat],
    UP3D_STR_SYSTEM_STATE[sstat] );
#else
  printf("%0.3f,%0.3f,%0.3f,%0.3f,%0.3f\n",
    time, x,y,z,e );
#endif

#ifdef CAPTURE_STATES
  return mstat;
#else
  return 2;  // return "Running Program"
#endif
}

#if 0
static void process(double time)
{
  static bool init = false;
  
  // parameters of last g-code 
  static float  gx = 0;
  static float  gy = 0;
  static float  gz = 0;
  static float  ge = 0;
  static float  gvx = 0;  //speed x/y
  static float  gve = 0;  //speed e
  static float  gvz = 0;  //speed z
  static double gt  = 0;  //time
  
  // parameters of last sample
  static float hx = 0; 
  static float hy = 0;
  static float hvx = 0;  //momentary speed  
  
  static int32_t gmstat;
  static int32_t gpstat;
  static int32_t gsstat;
  
  static bool bx,by, bz, be;
  static double t0;
  static double t1;
  static double direction = 10; // larger than 2°PI
  
  if (init == false)
  {
    init = true;
    t0 = time;
    gmstat = UP3D_GetMachineState();
    gpstat = UP3D_GetProgramState();
    gsstat = UP3D_GetSystemState();
    gx = (float)UP3D_GetAxisPosition(1) / steps[0];
    gy = (float)UP3D_GetAxisPosition(1) / steps[1];
    gz = (float)UP3D_GetAxisPosition(1) / steps[2];
    ge = (float)UP3D_GetAxisPosition(1) / steps[3];
    hx = gx;
    hy = gy;
    return;
  }
  
  float dt = (time - t0) / 1000.0f;    // time in seconds since last g-code command
  float dt1 = (time - t1) / 1000.0f;   // time since last call
  
  //printf("dt : %f\n", dt);

  // first we get the positions, since they are time critical

  int32_t xpos = UP3D_GetAxisPosition(1);
  float x = (float)xpos / steps[0];

  int32_t ypos = UP3D_GetAxisPosition(2);
  float y = (float)ypos / steps[1];

  int32_t zpos = UP3D_GetAxisPosition(3);
  float z = (float)zpos / steps[2];

  int32_t epos = UP3D_GetAxisPosition(4);
  float e = (float)epos / steps[3];
  
  // flags for changes in any parameter since last g-command
  bx = (gx != x) ? true: false;
  by = (gy != y) ? true: false;
  bz = (gz != z) ? true: false;
  be = (ge != e) ? true: false;
  
  // calculate parameters based on last g-command
  float sx  = gx-x;
  float sy  = gy-y;
  float len = sqrt(sx*sx + sy*sy);    //move len in mm
  float v   = fabs(len) / dt;                       //speed in mm/sec
  int   f   = LIMIT_F(v*60);
  
  // calculate parameters based on last call
  float isx  = ix - x;
  float isy  = iy - y;
  float ilen = sqrt(isx*isx + isy*isy);
  float iv   = fabs(ilen) / dt1;
  
  double dir;
  if (bx || by)
    dir = atan2(sx,sx);
  else
    dir = direction;
  
  double idir;
  if ( (ix != x) || (iy != y) )
    idir = atan2(isx,isy);
  else
    idir = direction;
    
  printf ("                         direction: %0.2f  dir: %0.2f  idir: %0.2f\n", TO_DEG(direction), TO_DEG(dir), TO_DEG(idir));  

  bool fcode = false; 
  if ( (bx || by) && be )  // if x or y changed together with E we are extruding
  {
    // now we check for direction
    if ( fabs(direction - idir) > MIN_ANGLE)
    {  
      printf("G1 X%0.3f Y%0.3f E%0.3f F%d ; dt %0.3f ms speed %0.2f mm/s angle: %0.2f\n", x,y,e,f, dt * 1000, v, TO_DEG(dir));
      fcode = true;
      gvx = v; 
      gve = fabs(gve - e) / dt ; 
      gvz = 0;
    }
  }
  else if ( (bx || by) && !be ) // if x or y changed but no extrude its a jump
  {   
    // now we check for direction
    if ( fabs(direction - dir) > MIN_ANGLE)
    {
      printf("G1 X%0.3f Y%0.3f F%d ; dt: %0.3f ms speed: %0.2f mm/s angle: %0.2f\n", x,y,f, dt * 1000, v, TO_DEG(dir));
      fcode = true;
      gvx = v;
      gve = 0;
      gvz = 0;
    }   
  }
  else if ( !(bx || by) && be ) // if neither x or y changed but e its a extrude or retract
  {
    float ve = fabs(ge-e) / dt;
    int   f = LIMIT_F(ve * 60);
    
    printf("G1 E%0.3f F%d ; dt: %0.3f ms speed: %0.2f mm/s\n", e, f, dt * 1000, v);
    fcode = true;
    gve = (gve + v)/2;
    gvx = 0;
    gvz = 0;
  }
  else if ( bz  && !( bx || by || be))   // here we have a layer change or lift operation
  {
    float v = fabs(gz - z) / dt;
    int   f = LIMIT_F(v * 60);
    
    printf("G1 Z%0.3f F%d ; dt: %0.3f ms speed: %0.2f mm/s\n", z, f, dt * 1000, v);
    fcode = true;
    
    gvz = (gvz + v)/2;
    gvx = 0;
    gve = 0;
  }
  else if ( bx || by || bz || be )
  {
    printf("; unknown move\n");
  }
  else
  {
    // nothing here - we just idle
  }

  // check if a g-code was spit out, if so we set a new global reference 
  if (fcode)
  {
    gx = x;
    gy = y;
    gz = z;
    ge = e;
    t0 = time;
    direction = dir;
  }
  
  ix = x;
  iy = y;
  ivx = iv;

  // next we get all the rest of the stuff
#if 0
  int32_t mstat = UP3D_GetMachineState();
  if (gmstat != mstat)
  {
    gmstat = mstat;
    printf(";Machine-State (%1d) %-16.16s\n", mstat, UP3D_STR_MACHINE_STATE[mstat]);
  }

  int32_t pstat = UP3D_GetProgramState();
  if (gpstat != pstat)
  {
    gpstat = pstat;
    printf(";Program-State (%1d) %-19.19s\n", pstat, UP3D_STR_PROGRAM_STATE[pstat] );
  }

  int32_t sstat = UP3D_GetSystemState();
  if (gsstat != sstat)
  {
    gsstat = sstat;
    printf(";System-State (%02d) %-22.22s\n", sstat, UP3D_STR_SYSTEM_STATE[sstat] );
  }
#endif
}
#endif

static void sigwinch(int sig)
{
}

static void sigfinish(int sig)
{
  UP3D_Close();
  exit(0);
}

int main(int argc, char *argv[])
{
  static struct timeval t0;
  static struct timeval t1;
  static double elapsed = 0;

  if( !UP3D_Open() )
    return -1;

  TT_tagPrinterInfoHeader pihdr;
  TT_tagPrinterInfoName   piname;
  TT_tagPrinterInfoData   pidata;
  TT_tagPrinterInfoSet    pisets[8];
    
  if( !UP3D_GetPrinterInfo( &pihdr, &piname, &pidata, pisets ) )
  {
    upl_error( "UP printer info error\n" );
    UP3D_Close();
    return -1;
  }
  
  steps[0] = pidata.f_steps_mm_x;
  steps[1] = pidata.f_steps_mm_y;
  steps[2] = pidata.f_steps_mm_z;
  steps[3] = pidata.f_steps_mm_x == 160.0 ? 236.0 : 854.0; // fix display for Cetus3D
  
  UP3D_SetParameter(0x94,999); //set best accuracy for reporting position

  signal(SIGINT, sigfinish);   // set sigint handler
#ifdef SIGWINCH
  signal(SIGWINCH, sigwinch);  // set sigint handler
#endif

  gettimeofday(&t0,0);

  //the loop
  uint32_t state = 0;
  for(;state !=3 ;)    // run until state is idle
  {
#if 1    
    switch( getch() )
    {
      case 0x12:    // CRTL-R
        fprintf(stderr,"Machine State: %1d %s\n", state, UP3D_STR_MACHINE_STATE[state]);
        break; // CTRL-R

      case 'p':
       {
         UP3D_BLK blk;
         UP3D_ClearProgramBuf();
         UP3D_PROG_BLK_Power(&blk,true);UP3D_WriteBlock(&blk);
         UP3D_PROG_BLK_Stop(&blk);UP3D_WriteBlock(&blk);
         UP3D_StartResumeProgram();
         UP3D_SetParameter(0x94,999); //set best accuracy for reporting position
       }
       break;
      case 'q':
       {
         UP3D_BLK blk;
         UP3D_ClearProgramBuf();
         UP3D_PROG_BLK_Power(&blk,false);UP3D_WriteBlock(&blk);
         UP3D_PROG_BLK_Stop(&blk);UP3D_WriteBlock(&blk);
         UP3D_StartResumeProgram();
         sigfinish(0);
       }
       break;

      case '0':
       {
         UP3D_ClearProgramBuf();
         UP3D_InsertRomProgram(0);
         UP3D_StartResumeProgram();
       }
       break;
    }
#endif    
    gettimeofday(&t1, 0);
    elapsed += timedifference_msec(t0, t1);
    t0 = t1;
    state = capture(elapsed);
    
    static int32_t old_state = -1;
    if (old_state != state) 
    {
      // information on state changes is piped to stderr so the main capture can be
      // redirected from stdout without this info
      old_state = state;
      fprintf(stderr,"Machine State: %1d %s\n", state, UP3D_STR_MACHINE_STATE[state]);
    }
    //usleep(10000);
  }

  sigfinish(0);
  return 0;
}
