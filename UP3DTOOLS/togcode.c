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

// translation to the coordinates
#define GCODE_X(x) (-(x))
#define GCODE_Y(y) (+(y))
#define GCODE_Z(z) ((z)+182)

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
  IDLE,
  START
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
long gcode_filter_e = 0;


// determin the type of command by the flags
GCODE get_cmd(bool bx, bool by, bool bz, bool be, GCODE last)
{
  GCODE cmd = last;

  if ( (bx || by) && be)  // if x or y changed together with E we are extruding
  {
    cmd = EXTRUDE;
  }
  else if ( (bx || by) && !be) // if x or y changed but no extrude its a jump
  {
    //    cmd = MOVE_XY;
    cmd = EXTRUDE;
  }
  else if ( be)    // if  e changed its an extrude or retract
  {
    cmd = MOVE_E;
  }
  else if ( bz )   // here we have a layer change or lift operation
  {
    cmd = MOVE_Z;
  }
  return cmd;
}

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
  static GCODE last_command = START;
  
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
  
  //fprintf(stderr,"xy%d z%d e%d\n", bx || by, bz, be);
  
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
  
  bool fcode = false;
  float vz, ve;
  int fz, fe;
  
  GCODE current_command;
  
  current_command = get_cmd(bx, by, bz, be, last_command);
  
//    printf("last: %d  current: %d flags: x%d y%d z%d e%d dir%d\n", last_command, current_command, bx, by, bz, be, direction_changed);
    
    if ( ((current_command != last_command) && (current_command != IDLE))
      || (direction_changed && ((current_command == MOVE_XY) || (current_command == EXTRUDE))) )
    {
      // process direction changed just once
      if (direction_changed)
        direction_changed = false;
      switch (last_command)
      {

        case START:
        case MOVE_XY:
          printf("G1 X%0.4f Y%0.4f E%0.4f F%d ; dt: %0.3f ms speed: %0.2f mm/s angle: %0.2f\n", GCODE_X(x),GCODE_Y(y), e, f, dt * 1000, v, TO_DEG(dir));
          gvx = v;
          gve = 0;
          gvz = 0;
          gcode_move_xy++;
          bx = by = false;
          last_command = get_cmd(bx, by, bz, be, MOVE_XY);
          break;

        case MOVE_Z:
          vz = fabs(gz - z) / dt;
          fz = LIMIT_F(vz * 60);
          printf("G1 Z%0.4f F%d\n", GCODE_Z(hz), fz);
          gvz = v;
          gvx = 0;
          gve = 0;
          gcode_move_z++;
          bz = false;
          last_command = get_cmd(bx, by, bz, be, MOVE_Z);
          break;

        case MOVE_E:
          ve = fabs(ge - e) / dt;
          fe = LIMIT_F(ve * 60);
          printf ("G1 E%0.5f F%d\n", he, f);
          gcode_move_e++;
          be = false;
          last_command = get_cmd(bx, by, bz, be, EXTRUDE);
          break;

//        case START:
//        case MOVE_XY:
        case EXTRUDE:
          if (be)
            printf("G1 X%0.4f Y%0.4f E%0.4f F%d ; dt %0.3f ms speed %0.2f mm/s angle: %0.2f\n", GCODE_X(x),GCODE_Y(y),e,f, dt * 1000, v, TO_DEG(dir));
          else
            printf("G1 X%0.4f Y%0.4f F%d ; dt %0.3f ms speed %0.2f mm/s angle: %0.2f\n", GCODE_X(x),GCODE_Y(y),f, dt * 1000, v, TO_DEG(dir));
          gvx = v;
          gve = fabs(gve - e) / dt ;
          gvz = 0;
          if (be)
            gcode_extrude++;
          else
            gcode_move_xy++;
          bx = by = be = false;
          last_command = get_cmd(bx, by, bz, be, EXTRUDE);
          break;
          
          // not yet implemented
        case HOME:
          gcode_home++;
          last_command = IDLE;
          break;
          
        case IDLE:
          gcode_idle++;
          break;
          
        default:
          break;
      }
      fcode = true;
    }
  //} while (current_command != IDLE);

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
  t1 = time;
  return true;
}


// simple input averaging filter
void filter_all (char * line)
{
  
  static float _x = 0;
  static float _y = 0;
  static float _z = 0;
  static float _e = 0;
  static double _time = 0;;
  static char _state[32] = "delay1";
  
  static float __x = 0;
  static float __y = 0;
  static float __z = 0;
  static float __e = 0;
  static double __time = 0;;
  static char __state[32] = "delay2";
  
  double time;
  float  x, y, z, e;
  char state[32];
  
  if ( sscanf(line, "%lf,%f,%f,%f,%f,%[^,\t\n]", &time,&x,&y,&z,&e, state) != 6)
    return; // just skip a non valid line
  
  // check for invalid samples and replace with previouse one
  x = (x == 0) ? _x : x;
  y = (y == 0) ? _y : y;
  z = (z == 0) ? _z : z;
  e = (e == 0) ? _e : e;
  
  float x_ = (x + _x + __x) / 3;
  float y_ = (y + _y + __y) / 3;
  float z_ = (z + _z + __z) / 3;
  float e_ = (e + _e + __e) / 3;
  double time_ = _time;
  char state_[32]; strcpy(state_, _state);
  
  if (e_ != __e)
    gcode_filter_e++;
  
  __x = _x;
  __y = _y;
  __z = _z;
  __e = _e;
  __time = _time;
  strcpy(__state, _state);
  
  
  _x = x;
  _y = y;
  _z = z;
  _e = e;
  _time = time;
  strcpy(_state, state);
  
  sprintf(line, "%0.2lf,%0.4f,%0.4f,%0.4f,%05f,%s\n", time_, x_, y_, z_, e_, state_);
  return;
}


double approximate (double avg, double input, double n)
{
  avg -= avg/n;
  avg += input/n;
  return avg;
}

// input filter with approximation of a rolling average over n
void filter_approximate (char * line)
{
  static float _x = 0;
  static float _y = 0;
  static float _z = 0;
  static float _e = 0;
  static double _t = 0;
  
  double time;
  float  x, y, z, e;
  char state[32];
  
  if ( sscanf(line, "%lf,%f,%f,%f,%f,%[^,\t\n]", &time,&x,&y,&z,&e, state) != 6)
    return; // just skip a non valid line
  
  // check for invalid samples and replace with previouse one
  x = (x == 0) ? _x : x;
  y = (y == 0) ? _y : y;
  z = (z == 0) ? _z : z;
  e = (e == 0) ? _e : e;
  
  double n = (time - _t) * 3;  // average over 3 ms
  
  n = n < 2.0 ? 2.0: n; //minimum factor 2
 
  _x = approximate(_x, x, n);
  _y = approximate(_y, y, n);
  _z = approximate(_z, z, n);
  _e = (_e == e) ? e : approximate(_e, e, n);
  _t = time;
  
  if (_e != e)
    gcode_filter_e++;
  
  sprintf(line, "%0.2lf,%0.4f,%0.4f,%0.4f,%05f,%s\n", time, _x, _y, _z, _e, state);
  return;
}


void print_usage_and_exit()
{
  printf("Usage: togcode input.capture [average] \n\n");
  printf("         input.capture:  machine capture file\n");
  exit(0);
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
    //static int subsampling = 0;
    //if (subsampling++ % 2)
    {
      //filter_all(line);
      filter_approximate(line);
      if( !process(line) )
      {
        printf("ERROR: line %ld\n", l);
        return (1);
      }
      l++;
    }
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
  printf (";number of filter_e:    %ld \n", gcode_filter_e);
  return 1;
}
