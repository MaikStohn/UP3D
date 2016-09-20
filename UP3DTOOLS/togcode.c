///////////////////////////////////////////////////////
// Author: K.Scheffer
//
// Date: 19.Sep.2016
//
// processes capture file from printer
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

typedef enum  {
  MOVE_XY,
  MOVE_Z,
  MOVE_E,
  EXTRUDE,
  HOME,
  IDLE
} GCODE;

// for statistics
long gcode_lines = 0;
long gcode_move_xy = 0;
long gcode_move_z = 0;
long gcode_move_e = 0;
long gcode_extrude = 0;
long gcode_home = 0;
long gcode_idle = 0;
long gcode_timelaps = 0;

static bool process(char* line)
{
  static bool init = false;
  
  // line should be organized as t,x,y,z,e,state
  // the very first line contains a header
  
  double time;
  float  x, y, z, e;
  char state[32];
  
  if ( sscanf(line, "%lf,%f,%f,%f,%f,%[^,\t\n]", &time,&x,&y,&z,&e, state) != 6)
  return true; // just skip a non valid line
  
  //echo for debugging
  printf(";%s", line);
  
  // parameters of last g-code
  static float  gx = 0;
  static float  gy = 0;
  static float  gz = 0;
  static float  ge = 0;
  static float  gvx = 0;  //speed x/y
  static float  gve = 0;  //speed e
  static float  gvz = 0;  //speed z
  
  
  // parameters of last sample
  static float hx = 0;
  static float hy = 0;
  static float hz = 0;
  static float he = 0;
  static float hvx = 0;  //momentary speed
  static GCODE last_command = HOME;
  
  static int32_t gmstat;
  
  static double t0;
  static double t1;
  static double direction = 10; // larger than 2°PI
  
  if (init == false)
  {
    init = true;
    t0 = t1 = time;
    gmstat = (strcmp(state,"Idle") == 0) ? 3: 2;
    gx = x;
    gy = y;
    gz = z;
    ge = e;
    hx = gx;
    hy = gy;
    return true;
  }

#if 0
  // we average the samples here to get a more clean signal
  x = (x + hx) / 2;
  y = (y + hy) / 2;
  z = (z + hz) / 2;
  e = (e + he) / 2;
#endif
  
  float dt = (time - t0) / 1000.0f;    // time in seconds since last g-code command
  float dt1 = (time - t1) / 1000.0f;   // time since last call
  
  if (dt1 > 0.1)
    printf("; inconsistent time lapse of %02.f ms\n", dt1 * 1000), gcode_timelaps++;
  
  // flags for changes in any parameter since last g-command
  bool bx = (gx != x) ? true: false;
  bool by = (gy != y) ? true: false;
  bool bz = (gz != z) ? true: false;
  bool be = (ge != e) ? true: false;
  
  // calculate parameters based on last g-command
  float sx  = gx-x;
  float sy  = gy-y;
  float len = sqrt(sx*sx + sy*sy);    //move len in mm
  float v   = fabs(len) / dt;                       //speed in mm/sec
  int   f   = LIMIT_F(v*60);
  
  // calculate parameters based on last call
  float hsx  = hx - x;
  float hsy  = hy - y;
  float hlen = sqrt(hsx*hsx + hsy*hsy);
  float hv   = fabs(hlen) / dt1;
  
  double dir;
  if (bx || by)
  dir = atan2(sx,sx);
  else
  dir = direction;
  
  double hdir;
  if ( (hx != x) || (hy != y) )
  hdir = atan2(hsx,hsy);
  else
  hdir = direction;
  
  bool direction_changed = ( fabs(direction - hdir) > MIN_ANGLE) ? true : false;
  
  //printf ("                         direction: %0.2f  dir: %0.2f  idir: %0.2f\n", TO_DEG(direction), TO_DEG(dir), TO_DEG(idir));
  
  GCODE current_command = IDLE;
  
  if ( (bx || by) && be )  // if x or y changed together with E we are extruding
  {
    current_command = EXTRUDE;
  }
  else if ( (bx || by) && !be ) // if x or y changed but no extrude its a jump
  {
    current_command = MOVE_XY;
  }
  else if ( !(bx || by) && be ) // if neither x or y changed but e its a extrude or retract
  {
    current_command = MOVE_E;
  }
  else if ( bz  && !( bx || by || be))   // here we have a layer change or lift operation
  {
    current_command = MOVE_Z;
  }
  else if ( bx || by || bz || be )
  {
    current_command = IDLE;
  }
  else
  {
    current_command = IDLE;
  }
  
  bool fcode = false;
  float vz, ve;
  int fz, fe;
  //if ((current_command != last_command) || direction_changed)
  {
    switch (last_command)
    {
        case MOVE_XY:
        printf("G1 X%0.3f Y%0.3f F%d ; dt: %0.3f ms speed: %0.2f mm/s angle: %0.2f\n", x,y,f, dt * 1000, v, TO_DEG(dir));
        gvx = v;
        gve = 0;
        gvz = 0;
        gcode_move_xy++;
        break;
        
        case MOVE_Z:
        vz = fabs(gz - z) / dt;
        fz = LIMIT_F(vz * 60);
        printf("G1 Z%0.3f F%d\n", hz, fz);
        gvz = v;
        gvx = 0;
        gve = 0;
        gcode_move_z++;
        break;
        
        case MOVE_E:
        ve = fabs(ge - e) / dt;
        fe = LIMIT_F(ve * 60);
        printf ("G1 E%0.3f F%d\n", he, f);
        gcode_move_e++;
        break;
        
        case EXTRUDE:
        printf("G1 X%0.3f Y%0.3f E%0.3f F%d ; dt %0.3f ms speed %0.2f mm/s angle: %0.2f\n", x,y,e,f, dt * 1000, v, TO_DEG(dir));
        gvx = v;
        gve = fabs(gve - e) / dt ;
        gvz = 0;
        gcode_extrude++;
        break;
        
        case HOME:
        gcode_home++;
        break;
        
        case IDLE:
        gcode_idle++;
        break;
        
      default:
        break;
    }
    fcode = true;
  }
#if 0
  if ( (bx || by) && be )  // if x or y changed together with E we are extruding
  {
    // now we check for direction
    if ( fabs(direction - hdir) > MIN_ANGLE)
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
#endif
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
  
  hx = x;
  hy = y;
  hz = z;
  he = e;
  hvx = hv;
  last_command = current_command;
  t1 = time;
  return true;
}


void print_usage_and_exit()
{
  printf("Usage: togcode input.capture\n\n");
  printf("          input.capture:  machine capture file\n");
  exit(1);
}

int main(int argc, char *argv[])
{
  if( 2 != argc )
  print_usage_and_exit();
  
  FILE* fcapture = fopen( argv[1], "r" );
  if( !fcapture )
  {
    printf("ERROR: Could not open %s for reading\n\n", argv[1]);
    print_usage_and_exit();
  }
  
  printf("; processing file '%s'\n", argv[1]);
  
  char line[1024];
  long l = 0;;
  while( fgets(line,sizeof(line),fcapture))
  {
    if( !process(line) )
    {
      printf("ERROR: line %ld\n", l);
      return (1);
    }
    l++;
  }
  fclose( fcapture );
  printf (";number of input lines: %ld \n", l);
  printf (";number of g-commands:  %ld \n", gcode_move_xy + gcode_move_z + gcode_move_e + gcode_extrude + gcode_home);
  printf (";number of move_xy:     %ld \n", gcode_move_xy);
  printf (";number of move_z:      %ld \n", gcode_move_z);
  printf (";number of move_e:      %ld \n", gcode_move_e);
  printf (";number of extrude:     %ld \n", gcode_extrude);
  printf (";number of home:        %ld \n", gcode_home);
  printf (";number of idle:        %ld \n", gcode_idle);
  printf (";number of timelaps:    %ld \n", gcode_timelaps);
  return 0;
}
