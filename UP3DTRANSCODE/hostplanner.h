/*
  hostplanner.h for UP3DTranscoder
  M. Stohn 2016
  
  Original taken from GRBL:
  =========================

  planner.h - buffers movement commands and manages the acceleration profile plan
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

#ifndef hostplanner_h
#define hostplanner_h

#include "up3dconf.h"
#include <stdint.h>
#include <stdbool.h>

// The number of linear motions that can be in the plan at any give time
#ifndef BLOCK_BUFFER_SIZE
  #define BLOCK_BUFFER_SIZE 8192
#endif

// This struct stores a linear movement of a g-code block motion with its critical "nominal" values
// are as specified in the source g-code. 
typedef struct {
  // Fields used by the bresenham algorithm for tracing the line
  // NOTE: Used by stepper algorithm to execute the block correctly. Do not alter these values.
  uint8_t direction_bits;    // The direction bit set for this block (refers to *_DIRECTION_BIT in config.h)
//-->MS
  int32_t steps[N_AXIS];    // Step count along each axis
//<--MS
  uint32_t step_event_count; // The maximum step axis count and number of steps required to complete this block. 

  // Fields used by the motion planner to manage acceleration
  double entry_speed_sqr;         // The current planned entry speed at block junction in (mm/min)^2
  double max_entry_speed_sqr;     // Maximum allowable entry speed based on the minimum of junction limit and 
                                 //   neighboring nominal speeds with overrides in (mm/min)^2
  double max_junction_speed_sqr;  // Junction entry speed limit based on direction vectors in (mm/min)^2
  double nominal_speed_sqr;       // Axis-limit adjusted nominal speed for this block in (mm/min)^2
  double acceleration;            // Axis-limit adjusted line acceleration in (mm/min^2)
  double millimeters;             // The remaining distance for this block to be executed in (mm)
  // uint8_t max_override;       // Maximum override value based on axis speed limits

 // int32_t line_number;

//-->MS
  double factor[N_AXIS];
//<--MS

} plan_block_t;

      
// Initialize and reset the motion plan subsystem
void plan_reset();

// Add a new linear movement to the buffer. target[N_AXIS] is the signed, absolute target position 
// in millimeters. Feed rate specifies the speed of the motion. If feed rate is inverted, the feed
// rate is taken to mean "frequency" and would complete the operation in 1/feed_rate minutes.
void plan_buffer_line(double *target, double feed_rate, bool invert_feed_rate);

// Called when the current block is no longer needed. Discards the block and makes the memory
// availible for new blocks.
void plan_discard_current_block();

// Gets the current block. Returns NULL if buffer empty
plan_block_t *plan_get_current_block();

// Called periodically by step segment buffer. Mostly used internally by planner.
uint32_t plan_next_block_index(uint32_t block_index);

// Called by step segment buffer when computing executing block velocity profile.
double plan_get_exec_block_exit_speed();

// Reinitialize plan with a partially completed block
void plan_cycle_reinitialize();

// Returns the number of active blocks are in the planner buffer.
uint32_t plan_get_block_buffer_count();

// Returns the status of the block ring buffer. True, if buffer is full.
bool plan_check_full_buffer();

//-->MS
void plan_set_position(double *pos);
void plan_set_e_position(double epos);
void plan_get_position(double *pos);
//<--MS

#endif //hostplanner_h
