/*
  hoststepper.h for UP3DTranscoder
  M. Stohn 2016
  
  Original taken from GRBL:
  =========================

  stepper.h - stepper motor driver: executes motion plans of planner.c using the stepper motors
  Part of Grbl

  Copyright (c) 2011-2015 Sungeun K. Jeon
  Copyright (c) 2009-2011 Simen Svale Skogsrud

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef hoststepper_h
#define hoststepper_h

#define SEGMENT_BUFFER_SIZE 16

#include <stdint.h>
#include <stdbool.h>

// Primary stepper segment ring buffer. Contains small, short line segments for the stepper 
// algorithm to execute, which are "checked-out" incrementally from the first block in the
// planner buffer. Once "checked-out", the steps in the segments buffer cannot be modified by 
// the planner, where the remaining planner block steps still can.
typedef struct {
  uint16_t p1;
  uint16_t p2;

  int16_t p3;
  int16_t p4;
  int16_t p5;

  int16_t p6;
  int16_t p7;
  int16_t p8;
  
} segment_up3d_t;


//MS-->
bool st_get_next_segment_up3d(segment_up3d_t** ppseg);
//<--

// Reset the stepper subsystem variables       
void st_reset();
             
// Reloads step segment buffer. Called continuously by realtime execution system.
void st_prep_buffer();

// Called by planner_recalculate() when the executing block is updated by the new plan.
void st_update_plan_block_parameters();

#endif //hoststepper_h
