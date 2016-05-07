/*
  umcwriter.h for UP3DTranscoder
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

#ifndef umcwriter_h
#define umcwriter_h

#include <stdint.h>
#include <stdbool.h>

bool    umcwriter_init(const char* filename, const double heightZ, const char machine_type);
void    umcwriter_finish();
int32_t umcwriter_get_print_time();
void    umcwriter_home(int32_t axes);
void    umcwriter_virtual_home(double speedX, double speedY,double speedZ);
void    umcwriter_move_direct(double X, double Y, double Z, double A, double F);
void    umcwriter_planner_set_position(double X, double Y, double A);
void    umcwriter_planner_set_a_position(double A);
void    umcwriter_planner_add(double X, double Y, double A, double F);
void    umcwriter_planner_sync();
void    umcwriter_set_extruder_temp(double temp, bool wait);
void    umcwriter_set_bed_temp(int32_t temp, bool wait);
void    umcwriter_set_report_data(int32_t layer, double height);
void    umcwriter_pause(uint32_t msec);
void    umcwriter_beep(uint32_t msec);
void    umcwriter_user_pause();

#endif //umcwriter_h
