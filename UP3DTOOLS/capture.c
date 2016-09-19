///////////////////////////////////////////////////////
// Author: K.Scheffer based on upshell from M. Stohn //
//
// Date: 18.Sep.2016
//
// live captures the position reporting of the printer
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
//  int32_t pstat = UP3D_GetProgramState();
//  int32_t sstat = UP3D_GetSystemState();
  printf("%0.3f,%0.3f,%0.3f,%0.3f,%0.3f,%s\n"
    ,time, x,y,z,e
    ,UP3D_STR_MACHINE_STATE[mstat]
/*    ,UP3D_STR_PROGRAM_STATE[pstat] */
/*    ,UP3D_STR_SYSTEM_STATE[sstat] */
    );
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
  uint32_t samples = 0;
  for(;state !=3 ; samples++)    // run until state is idle
  {
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
      fprintf(stderr, "Machine State: %1d %s\n", state, UP3D_STR_MACHINE_STATE[state]);
    }
    //usleep(10000);
  }
  fprintf(stderr, "samples recorded: %d,  total time: %0.3f s, %d samples/s\n",
    samples, elapsed / 1000, (int)(samples/elapsed*1000));    
  sigfinish(0);
  return 0;
}
