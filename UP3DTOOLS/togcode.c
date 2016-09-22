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
#define GCODE_Z(z) ((z)+nozzle_height)

#define MAX_F (200*60)    // mm / min
#define LIMIT_F(f) ( (f) > MAX_F ? MAX_F:(f))

#define MAX_A (150*60)   // mm/s2
#define LIMIT_A(a) ( (a) > MAX_A ? MAX_A:(a))

#define TO_DEG(rad) ((rad)*(180.0/M_PI))

typedef enum  {
  MOVE_XY,
  MOVE_Z,
  MOVE_E,
  EXTRUDE,
  HOME,
  IDLE,
  START
} GCODE;

typedef struct CAP_SETTINGS {
  char      printer_name[63];
  uint32_t  u32_printerid;
  float     f_rom_version;
  float     f_max_x;
  float     f_max_y;
  float     f_max_z;
  float     f_steps_mm_x;
  float     f_steps_mm_y;
  float     f_steps_mm_z;
  float     f_steps_mm_a;
} CAP_SETTINGS;

// default settings
static CAP_SETTINGS cap_settings = {
  "UP Mini(A)", // PrinterName
  10104,        // PrinterID
  6.11,         // ROMVersion
  -120.0,       // max_x
  120.0,        // max_y
  130.0,        // max_z
  854.0,        // steps_mm_x
  854.0,        // steps_mm_y
  854.0,        // steps_mm_z
  854.0         // steps_mm_a
};

static const char cap_settings_keys[10][32] = {
  "; PrinterModel=%[^\t\n]",
  "; PrinterID=%s",
  "; ROMVersion=%s",
  "; max_x=%s",
  "; max_y=%s",
  "; max_z=%s",
  "; steps_mm_x=%s",
  "; steps_mm_y=%s",
  "; steps_mm_z=%s",
  "; steps_mm_a=%s"
};

// globals
float nozzle_height = 0; // used to translate Z to the build plate

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



double approximate (double avg, double input, double n)
{
  avg -= avg/n;
  avg += input/n;
  return avg;
}

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

static bool get_settings(char* line)
{
  int i = 0;
  int ret = 1;
  bool done = false;
  for ( ; i< sizeof(cap_settings_keys); i++)
  {
    char val[1024];
    if (sscanf(line, cap_settings_keys[i], val) == 1)
    {
      switch (i)
      {
        case 0:    // PrinterModel
          strcpy(cap_settings.printer_name, val);
          done = true;
          break;
        case 1:    // PrinterID
          ret = sscanf(val, "%d", &cap_settings.u32_printerid);
          done = true;
          break;
        case 2:    // ROMVersion
          ret = sscanf(val,"%f", &cap_settings.f_rom_version);
          done = true;
          break;
        case 3:    // max_x
          ret = sscanf(val,"%f", &cap_settings.f_max_x);
          done = true;
          break;
        case 4:    // max_y
          ret = sscanf(val,"%f", &cap_settings.f_max_y);
          done = true;
          break;
        case 5:    // max_z
          ret = sscanf(val,"%f", &cap_settings.f_max_z);
          done = true;
          break;
        case 6:    // steps_mm_x
          ret = sscanf(val,"%f", &cap_settings.f_steps_mm_x);
          done = true;
          break;
        case 7:    // steps_mm_y
          ret = sscanf(val,"%f", &cap_settings.f_steps_mm_y);
          done = true;
          break;
        case 8:    // steps_mm_z
          ret = sscanf(val,"%f", &cap_settings.f_steps_mm_z);
          done = true;
          break;
        case 9:    // steps_mm_a
          ret = sscanf(val,"%f", &cap_settings.f_steps_mm_a);
          done = true;
        default:
          break;
      }
    }
    if (done)
      break;
  }
  return ret ? true: false;
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
    return false; // just skip a non valid line
  
  //echo for debugging
  //printf(";%s", line);
  
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
  
  double dir = atan2(sy,sx);
  
  static double _hdir = 0;
  double hdir = atan2(hsy,hsx);
  double n = dt1 * 3;
  n = (n< 2) ? 2 : n;
  hdir = approximate(_hdir, hdir, 2);
  _hdir = dir;
  
  bool direction_changed = ( fabs(dir - hdir) >= atan(1.0l/20/len) ) ? true : false;
  
  bool fcode = false;
  float vz, ve;
  int fz, fe;
  
  GCODE current_command;
  
  current_command = get_cmd(bx, by, bz, be, last_command);
  
//    printf("last: %d  current: %d flags: x%d y%d z%d e%d dir%d\n", last_command, current_command, bx, by, bz, be, direction_changed);
    
    if ( ((current_command != last_command) && (current_command != IDLE))
      || (direction_changed && ((current_command == MOVE_XY) || (current_command == EXTRUDE))) )
    {
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
          {
            printf("G1 X%0.4f Y%0.4f E%0.4f F%d ; dt %0.3f ms speed %0.2f mm/s angle: %0.2f\n", GCODE_X(x),GCODE_Y(y),e,f, dt * 1000, v, TO_DEG(dir));
            if (nozzle_height == 0)
              nozzle_height = -gz;
            //nozzle_height =  gz < -nozzle_height ? -gz : nozzle_height;
          }
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
    filter_approximate(line);
    if( !process(line) )
    {
      if (!get_settings(line))
      {
        printf("ERROR: line %ld\n", l);
        return (1);
      }
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
  printf (";nozzle_height:         %0.2f \n", nozzle_height);

/*
  printf("; PrinterModel=%s\n", cap_settings.printer_name);
  printf("; PrinterID=%d\n", cap_settings.u32_printerid);
  printf("; ROMVersion=%f\n", cap_settings.f_rom_version);
  printf("; max_x=%f\n", cap_settings.f_max_x);
  printf("; max_y=%f\n", cap_settings.f_max_y);
  printf("; max_z=%f\n", cap_settings.f_max_z);
  printf("; steps_mm_x=%f\n", cap_settings.f_steps_mm_x);
  printf("; steps_mm_y=%f\n", cap_settings.f_steps_mm_y);
  printf("; steps_mm_z=%f\n", cap_settings.f_steps_mm_z);
  printf("; steps_mm_a=%f\n", cap_settings.f_steps_mm_a);
*/
  return 1;
}
