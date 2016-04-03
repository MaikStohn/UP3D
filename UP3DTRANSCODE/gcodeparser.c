/*
  gcodeparser.cpp for UP3DTranscoder
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

#include "gcodeparser.h"
#include "umcwriter.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

static unsigned int gcp_line_number;
static unsigned int gcp_file_line_number;

static bool gcp_use_absoulte;
static bool gcp_use_extruder_absoulte;
static double gcp_X,gcp_Y,gcp_Z,gcp_E,gcp_F;

static double gcp_Z_max_used;
static unsigned int gcp_layer;

#define gcp_error(e,s) { printf("GCP ERROR at line %d: %s\n>>%s\n",gcp_file_line_number,e,s); }

void gcp_reset()
{
  gcp_line_number = 0;
  gcp_file_line_number = 0;

  gcp_use_absoulte = true;
  gcp_use_extruder_absoulte = true;
  
  gcp_X = gcp_Y = gcp_Z = gcp_E = 0;
  gcp_F = 100;

  gcp_Z_max_used = 0;
  gcp_layer = 0;
}

bool gcp_process_line(const char* gcodeline)
{
  if( !gcodeline )
    return false;

  char _line[256];
  strncpy( _line, gcodeline, sizeof(_line) );
  char *line = _line;
  char *tok;

  gcp_file_line_number++;
  
  if( (0==*line) || ('\n'==*line) || ('\r'==*line) )
    return true; //empty line

  if( 'N'==*line )
  {
    double lineno;
    if( 1 != sscanf( line,"N%lf", &lineno ) )
    {
      gcp_error("Invalid line number format",gcodeline);
      return false;
    }
    if( gcp_line_number+1 != lineno )
    {
      gcp_error("Invalid line number sequence",gcodeline);
      return false;
    }
    gcp_line_number = lineno;

    tok = strchr(line,' '); //jump over line number
    if( !tok )
      return true; //empty line
    line = tok+1;
  }

  if( (tok = strchr(line,'*')) )
  {
    double checksum;
    if( 1 != sscanf( tok,"*%lf", &checksum ) )
    {
      gcp_error("Invalid checksum format",gcodeline);
      return false;
    }
    uint8_t csx=0;
    for( tok--; tok!=line; tok-- )
      csx^=*tok;
    if( csx != checksum )
    {
      gcp_error("Invalid checksum",gcodeline);
      return false;
    }
    *tok=0; //cut line before checksum
  }

  if( (tok = strchr(line,';')) )
  {
    if( tok == line )
      return true; //complete line is a comment

    *tok=0; //cut line before first comment
  }

  bool  xp=false,yp=false,zp=false,ep=false,fp=false,tp=false,sp=false,pp=false,rp=false;
  double xv,yv,zv,ev,fv,tv,sv,pv,rv;
  bool  xb=false,yb=false,zb=false,eb=false,fb=false,tb=false,sb=false,pb=false,rb=false;

  for( tok=line; tok; tok=strchr(tok,' ') ) //white spaces ???
  {
    if( !tok || !*(++tok) )
      break;
    if('X'==*tok){xp=true;if(1==sscanf(tok,"X%lf",&xv))xb=true;}
    if('Y'==*tok){yp=true;if(1==sscanf(tok,"Y%lf",&yv))yb=true;}
    if('Z'==*tok){zp=true;if(1==sscanf(tok,"Z%lf",&zv))zb=true;}
    if('E'==*tok){ep=true;if(1==sscanf(tok,"E%lf",&ev))eb=true;}
    if('F'==*tok){fp=true;if(1==sscanf(tok,"F%lf",&fv))fb=true;}

    if('T'==*tok){tp=true;if(1==sscanf(tok,"T%lf",&tv))tb=true;}
    if('S'==*tok){sp=true;if(1==sscanf(tok,"S%lf",&sv))sb=true;}
    if('P'==*tok){pp=true;if(1==sscanf(tok,"P%lf",&pv))pb=true;}
    if('R'==*tok){rp=true;if(1==sscanf(tok,"R%lf",&rv))rb=true;}
  }
  
  double code;
  if( 1 == sscanf( line,"G%lf", &code ) )
  {
    switch( (int)code )
    {
      case 0:
      case 1: //move
       {
        if(fb) gcp_F=fv;
        if(eb) gcp_E=(gcp_use_absoulte||gcp_use_extruder_absoulte)?ev:gcp_E+ev;
        if(xb) gcp_X=(gcp_use_absoulte)?xv:gcp_X+xv;
        if(yb) gcp_Y=(gcp_use_absoulte)?yv:gcp_Y+yv;
        if(zb) gcp_Z=(gcp_use_absoulte)?zv:gcp_Z+zv;
        if(zb)
        {
          umcwriter_move_direct(gcp_X,gcp_Y,gcp_Z,gcp_E,gcp_F);

          if( gcp_Z > gcp_Z_max_used )
          {
            gcp_Z_max_used = gcp_Z;
            gcp_layer++;
          }
          umcwriter_set_report_data( gcp_layer, gcp_Z );
        }
        else
          umcwriter_planner_add(gcp_X,gcp_Y,gcp_E,gcp_F);
       }
       break;

      case 2:
      case 3:
        gcp_error("G2/G3 ARC not suppported",gcodeline);
        return false;

      case 4: //pause
        {
          uint32_t msec = 0;
          if(sb) msec=sv*1000;
          if(pb) msec=pv;
          if(msec)
            umcwriter_pause(msec);
        }
        break;

      case 21: //metric valus (default)
        break;

      case 28: //home
        {
          if(!xp && !yp && !zp) {xp=true;yp=true;zp=true;} //if no parameter given home all axis
          if(xp){ gcp_X=0; }
          if(yp){ gcp_Y=0; }
          if(zp){ gcp_Z=0; }
          umcwriter_virtual_home(xp?10000:0,yp?10000:0,zp?10000:0);
        }
        break;

      case 90: //set absolute positioning
        gcp_use_absoulte = true;
        break;

      case 91: //set relative positioning
        gcp_use_absoulte = false;
        break;

      case 92: //set position
       {
         if(xb) gcp_X=xv;
         if(yb) gcp_Y=yv;
         if(zb) gcp_Z=zv;
         if(eb) gcp_E=ev;
         umcwriter_planner_set_position(gcp_X,gcp_Y,gcp_E);
       }
       break;
 
      default:
        gcp_error("UNSUPPORTED command",gcodeline);
        return false;
    }
  }
  else
  if( 1 == sscanf( line,"M%lf", &code ) )
  {
    switch( (int)code )
    {
      case 82: //set extruder absolute mode - default
        gcp_use_extruder_absoulte = true;
        break;
      case 83: //set extruder relative mode
        gcp_use_extruder_absoulte = false;
        break;

      case 84: //disable motors
        break;

      case 104://set extruder target temp
        if( sb ) umcwriter_set_extruder_temp(sv,false);
        break;

      case 106: //fan
      case 107: //fan off
        //ignore
        break;

      case 117: //set message
        //ignore
        break;

      case 109: //set extrduder target temp and wait
        if( sb ) umcwriter_set_extruder_temp(sv,true);
        if( rb ) umcwriter_set_extruder_temp(rv,true);
        break;

      case 140: //set bed target temp
        if( sb ) umcwriter_set_bed_temp(sv,false);
        break;

      case 190: //set bed target temp and wait
        if( sb ) umcwriter_set_bed_temp(sv,true);
        if( rb ) umcwriter_set_bed_temp(rv,true);
        break;

      case 300: //play beep sound
        if( pb ) umcwriter_beep(pv);
        break;

      default:
        gcp_error("UNSUPPORTED command",gcodeline);
        return false;
    }
  }
  else
  if( 1 == sscanf( line,"T%lf", &code ) )
  {
    umcwriter_planner_sync();
    //ignore any T (tool change) commands
  }
  else
  {
    gcp_error("Unknown command",gcodeline);
    return false;
  }

  return true;
}

int gcp_get_layer()
{
  return gcp_layer;
}

double gcp_get_height()
{
  return gcp_Z_max_used;
}
