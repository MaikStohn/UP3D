/*
  hoststepper.c for UP3DTranscoder
  M. Stohn 2016
  
  Original taken from GRBL:
  =========================

  stepper.c - stepper motor driver: executes motion plans using stepper motors
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

#include "hoststepper.h"
#include "hostplanner.h"

#include "up3dconf.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>

#define get_direction_pin_mask(a) (1<<a)
static segment_up3d_t segment_buffer[SEGMENT_BUFFER_SIZE];

// Step segment ring buffer indices
static volatile uint32_t segment_buffer_tail;
static uint32_t segment_buffer_head;
static uint32_t segment_next_head;

// Pointers for the step segment being prepped from the planner buffer. Accessed only by the
// main program. Pointers may be planning segments or planner blocks ahead of what being executed.
static plan_block_t *pl_block;     // Pointer to the planner block being prepped

// Segment preparation data struct. Contains all the necessary information to compute new segments
// based on the current executing planner block.
typedef struct {
  uint32_t st_block_index;  // Index of stepper common data block being prepped
  double current_speed;    // Current speed at the end of the segment buffer (mm/min)
  double maximum_speed;    // Maximum speed of executing block. Not always nominal speed. (mm/min)
  double exit_speed;       // Exit speed of executing block (mm/min)
  double accelerate_until; // Acceleration ramp end measured from end of block (mm)
  double decelerate_after; // Deceleration ramp start measured from end of block (mm)
} st_prep_t;
static st_prep_t prep;


/*    BLOCK VELOCITY PROFILE DEFINITION 
          __________________________
         /|                        |\     _________________         ^
        / |                        | \   /|               |\        |
       /  |                        |  \ / |               | \       s
      /   |                        |   |  |               |  \      p
     /    |                        |   |  |               |   \     e
    +-----+------------------------+---+--+---------------+----+    e
    |               BLOCK 1            ^      BLOCK 2          |    d
                                       |
                  time ----->      EXAMPLE: Block 2 entry speed is at max junction velocity
  
  The planner block buffer is planned assuming constant acceleration velocity profiles and are
  continuously joined at block junctions as shown above. However, the planner only actively computes
  the block entry speeds for an optimal velocity plan, but does not compute the block internal
  velocity profiles. These velocity profiles are computed ad-hoc as they are executed by the 
  stepper algorithm and consists of only 7 possible types of profiles: cruise-only, cruise-
  deceleration, acceleration-cruise, acceleration-only, deceleration-only, full-trapezoid, and 
  triangle(no cruise).

                                        maximum_speed (< nominal_speed) ->  + 
                    +--------+ <- maximum_speed (= nominal_speed)          /|\                                         
                   /          \                                           / | \                      
 current_speed -> +            \                                         /  |  + <- exit_speed
                  |             + <- exit_speed                         /   |  |                       
                  +-------------+                     current_speed -> +----+--+                   
                   time -->  ^  ^                                           ^  ^                       
                             |  |                                           |  |                       
                decelerate_after(in mm)                             decelerate_after(in mm)
                    ^           ^                                           ^  ^
                    |           |                                           |  |
                accelerate_until(in mm)                             accelerate_until(in mm)
                    
  The step segment buffer computes the executing block velocity profile and tracks the critical
  parameters for the stepper algorithm to accurately trace the profile. These critical parameters 
  are shown and defined in the above illustration.
*/

static int64_t g_ex,g_ey,g_ea;

bool st_get_next_segment_up3d(segment_up3d_t** ppseg)
{
  //auto prepare new segment(s) if segment buffer empty
  if (segment_buffer_head == segment_buffer_tail)
    st_prep_buffer();

  //exit if no segments found
  if (segment_buffer_head == segment_buffer_tail)
    return false;

  *ppseg = &segment_buffer[segment_buffer_tail];

  // Segment is complete. Advance segment indexing.
  if ( ++segment_buffer_tail == SEGMENT_BUFFER_SIZE) { segment_buffer_tail = 0; }

  return true;
}

void _st_store_up3d_seg(segment_up3d_t* pseg)
{
  if( pseg->p1 )
  {
    memcpy( &segment_buffer[segment_buffer_head], pseg, sizeof(segment_up3d_t) );
    // increment segment buffer indices
    segment_buffer_head = segment_next_head;
    if ( ++segment_next_head == SEGMENT_BUFFER_SIZE ) { segment_next_head = 0; }
  }
}

void _st_subtract_plsteps(segment_up3d_t* pseg)
{
  if( pseg->p1 )
  {
    int32_t p1 = pseg->p1;
    int32_t p3 = pseg->p3;
    int32_t p4 = pseg->p4;
    int32_t p5 = pseg->p5;
    int32_t p6 = pseg->p6;
    int32_t p7 = pseg->p7;
    int32_t p8 = pseg->p8;
    
    //calculate xsteps generated like mcu in printer (THERE IS A BAD *FLOOR* ROUNDING INSIDE!)
    int64_t sx = floor( (float)(p3*p1+p6*(p1-1)*p1/2) / 512.0 );
    int64_t sy = floor( (float)(p4*p1+p7*(p1-1)*p1/2) / 512.0 );
    int64_t sa = floor( (float)(p5*p1+p8*(p1-1)*p1/2) / 512.0 );
    
    pl_block->steps[0] -= llabs(sx);
    pl_block->steps[1] -= llabs(sy);
    pl_block->steps[2] -= llabs(sa);
  }
}

void _st_create_up3d_seg_a(segment_up3d_t* pseg, double t, double v_entry, double v_exit)
{
  pseg->p1 = 0;

  
  //s linear speed
  int64_t s_x = g_ex*512 + (int64_t)(v_entry*t*pl_block->factor[X_AXIS])*512;
  int64_t s_y = g_ey*512 + (int64_t)(v_entry*t*pl_block->factor[Y_AXIS])*512;
  int64_t s_a = g_ea*512 + (int64_t)(v_entry*t*pl_block->factor[A_AXIS])*512;
  
  //s acceleration
  int64_t sa_x = (int64_t)((v_exit-v_entry)*t*pl_block->factor[X_AXIS])*512;
  int64_t sa_y = (int64_t)((v_exit-v_entry)*t*pl_block->factor[Y_AXIS])*512;
  int64_t sa_a = (int64_t)((v_exit-v_entry)*t*pl_block->factor[A_AXIS])*512;
  
  //==> tmax = 65535*65535 / 50000000 = 85.8 sec. per segment
  int64_t p1 = 1+(int64_t)(t*F_CPU)/65535;
  for(;p1<65536;p1++)
  {
    int64_t p2 = (int64_t)(t*F_CPU/p1);
    
    int64_t p6 = (int64_t)(sa_x/(p1*p1));
    int64_t p7 = (int64_t)(sa_y/(p1*p1));
    int64_t p8 = (int64_t)(sa_a/(p1*p1));
    
    int64_t p3 = (int64_t)(s_x/p1) + (int64_t)((sa_x-p6*(p1-1)*p1)/2/p1);
    int64_t p4 = (int64_t)(s_y/p1) + (int64_t)((sa_y-p7*(p1-1)*p1)/2/p1);
    int64_t p5 = (int64_t)(s_a/p1) + (int64_t)((sa_a-p8*(p1-1)*p1)/2/p1);
    
    //test format limits
    if( (p3<-32767) || (p3>32767) || (p4<-32767) || (p4>32767) || (p5<-32767) || (p5>32767) ||
        (p6<-32767) || (p6>32767) || (p7<-32767) || (p7>32767) || (p8<-32767) || (p8>32767) )
      continue; //try again (p1 incremeted)
    
    //check if PWM frequency can be achieved or any output is done at all
    if( (p2>800) && (p3 || p4 || p5 || p6 || p7 || p8) )
    {
      //set segment
      pseg->p1 = p1; pseg->p2 = p2; pseg->p3 = p3; pseg->p4 = p4; pseg->p5 = p5; pseg->p6 = p6; pseg->p7 = p7; pseg->p8 = p8;
    }
   
    //always leave
    break;
  }
}

void _st_create_up3d_seg_c(segment_up3d_t* pseg, double v)
{
  pseg->p1 = 0;

  //calc xsteps
  int64_t s_x = g_ex*512 + pl_block->steps[0]*512*((pl_block->direction_bits&get_direction_pin_mask(0))?-1:1);
  int64_t s_y = g_ey*512 + pl_block->steps[1]*512*((pl_block->direction_bits&get_direction_pin_mask(1))?-1:1);
  int64_t s_a = g_ea*512 + pl_block->steps[2]*512*((pl_block->direction_bits&get_direction_pin_mask(2))?-1:1);
  
  //calc time based on corrected xsteps
  double tx = fabs(s_x / (512*settings.steps_per_mm[0]) / v);
  double ty = fabs(s_y / (512*settings.steps_per_mm[1]) / v);
  double ta = fabs(s_a / (512*settings.steps_per_mm[2]) / v);
  
  double t = max(max(tx,ty),ta);
 
  //==> tmax = 65535*65535 / 50000000 = 85.8 sec. per segment
  int64_t p1 = 1+(int64_t)(t*F_CPU)/65535;
  for(;p1<65536;p1++)
  {
    int64_t p2 = (int64_t)(t*F_CPU/p1);
    
    int64_t p3 = (int64_t)(s_x/p1); if(pl_block->steps[0]<0) p3=0; //take care not to insert REVERSE steps
    int64_t p4 = (int64_t)(s_y/p1); if(pl_block->steps[1]<0) p4=0; //take care not to insert REVERSE steps
    int64_t p5 = (int64_t)(s_a/p1); if(pl_block->steps[2]<0) p5=0; //take care not to insert REVERSE steps
    
    //test format limits
    if( (p3<-32767) || (p3>32767) || (p4<-32767) || (p4>32767) || (p5<-32767) || (p5>32767) )
      continue; //try again (p1 incremeted)
    
    //check if PWM frequency can be achieved or any output is done at all
    if( (p2>800) && (p3 || p4 || p5) )
    {
      //set segment
      pseg->p1 = p1; pseg->p2 = p2; pseg->p3 = p3; pseg->p4 = p4; pseg->p5 = p5; pseg->p6 = 0; pseg->p7 = 0; pseg->p8 = 0;
    }
    else
      p1 = 0;

    //calculate xsteps generated like mcu in printer (THERE IS A BAD *FLOOR* ROUNDING INSIDE!)
    int64_t sx = floor( (float)(p3*p1) / 512.0 );
    int64_t sy = floor( (float)(p4*p1) / 512.0 );
    int64_t sa = floor( (float)(p5*p1) / 512.0 );
    
    //track global error
    g_ex = (s_x/512 - sx);
    g_ey = (s_y/512 - sy);
    g_ea = (s_a/512 - sa);
    
    //always leave
    break;
  }
}

// Reset and clear stepper subsystem variables
void st_reset()
{
  // Initialize stepper algorithm variables.
  memset(&prep, 0, sizeof(st_prep_t));
  pl_block = NULL;  // Planner block pointer used by segment buffer
  segment_buffer_tail = 0;
  segment_buffer_head = 0; // empty = tail
  segment_next_head = 1;
  g_ex = g_ey = g_ea = 0;
}

// Called by planner_recalculate() when the executing block is updated by the new plan.
void st_update_plan_block_parameters()
{ 
  if (pl_block != NULL) { // Ignore if at start of a new block.
    pl_block->entry_speed_sqr = prep.current_speed*prep.current_speed; // Update entry speed.
    pl_block = NULL; // Flag st_prep_segment() to load new velocity profile.
  }
}

void st_prep_buffer()
{
  if (segment_buffer_tail != segment_next_head) // Check if we need to fill the buffer.
  {
    // Determine if we need to load a new planner block or if the block has been replanned. 
    if (pl_block == NULL)
    {
      if( !(pl_block = plan_get_current_block()) ) // Query planner for a queued block
        return; // No planner blocks. Exit.
                      
      // Check if the segment buffer completed the last planner block. If so, load the Bresenham
      // data for the block. If not, we are still mid-block and the velocity profile was updated. 
      // Increment stepper common data index to store new planner block data.
      if ( ++prep.st_block_index == (SEGMENT_BUFFER_SIZE-1) ) { prep.st_block_index = 0; }
      
      // Initialize segment buffer data for generating the segments.
      prep.current_speed = sqrt(pl_block->entry_speed_sqr);

      //---------------------------------------------------------------------------------------
      // Compute the velocity profile of a new planner block based on its entry and exit speeds
      double inv_2_accel = 0.5/pl_block->acceleration;

      // Compute or recompute velocity profile parameters of the prepped planner block.
      prep.accelerate_until = pl_block->millimeters;
      prep.exit_speed = plan_get_exec_block_exit_speed();   
      double exit_speed_sqr = prep.exit_speed*prep.exit_speed;
      double intersect_distance = 0.5*(pl_block->millimeters+inv_2_accel*(pl_block->entry_speed_sqr-exit_speed_sqr));
      if (intersect_distance > 0.0)
      {
        if (intersect_distance < pl_block->millimeters) // Either trapezoid or triangle types
        {
          // NOTE: For acceleration-cruise and cruise-only types, following calculation will be 0.0.
          prep.decelerate_after = inv_2_accel*(pl_block->nominal_speed_sqr-exit_speed_sqr);
          if (prep.decelerate_after < intersect_distance) // Trapezoid type
          {
            prep.maximum_speed = sqrt(pl_block->nominal_speed_sqr);
            if (pl_block->entry_speed_sqr == pl_block->nominal_speed_sqr)
            {
              // Cruise-deceleration or cruise-only type.
            }
            else
            {
              // Full-trapezoid or acceleration-cruise types
              prep.accelerate_until -= inv_2_accel*(pl_block->nominal_speed_sqr-pl_block->entry_speed_sqr); 
            }
          }
          else
          {
            // Triangle type
            prep.accelerate_until = intersect_distance;
            prep.decelerate_after = intersect_distance;
            prep.maximum_speed = sqrt(2.0*pl_block->acceleration*intersect_distance+exit_speed_sqr);
          }          
        }
        else
        {
          // Deceleration-only type
          prep.decelerate_after = pl_block->millimeters;
          prep.maximum_speed = prep.current_speed;
        }
      }
      else
      {
        // Acceleration-only type
        prep.accelerate_until = 0.0;
        prep.decelerate_after = 0.0;
        prep.maximum_speed = prep.exit_speed;
      }
    }
    
    if( pl_block->millimeters-prep.accelerate_until )
    {
      //calc A
      segment_up3d_t a_seg;
      // Acceleration-cruise, acceleration-deceleration ramp junction, or end of block.
      double time_var = 2.0*(pl_block->millimeters-prep.accelerate_until)/(prep.current_speed+prep.maximum_speed);
      _st_create_up3d_seg_a( &a_seg, time_var, prep.current_speed, prep.maximum_speed);
      //subtract A block distance
      _st_subtract_plsteps( &a_seg );
      //emit A block
      _st_store_up3d_seg( &a_seg );
    }

    segment_up3d_t d_seg = {0};
    if( prep.decelerate_after )
    {
      //calc D
      double time_var = 2.0*(prep.decelerate_after)/(prep.maximum_speed+prep.exit_speed);
      _st_create_up3d_seg_a( &d_seg, time_var, prep.maximum_speed, prep.exit_speed);
      //subtract D block distance
      _st_subtract_plsteps( &d_seg );
    }

    if( pl_block->steps[0] || pl_block->steps[1] || pl_block->steps[2] )
    {
      //calc C
      segment_up3d_t c_seg;
      _st_create_up3d_seg_c( &c_seg, prep.maximum_speed );
      //emit C
      _st_store_up3d_seg( &c_seg );
    }
    
    //emit D block
    _st_store_up3d_seg( &d_seg );

    pl_block = NULL; // Set pointer to indicate check and load next planner block.
    plan_discard_current_block();
  }
}
