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

// Some useful constants.
#define DT_SEGMENT (1.0/(ACCELERATION_TICKS_PER_SECOND*60.0)) // min/segment
#define REQ_MM_INCREMENT_SCALAR 1.25
#define RAMP_ACCEL 0
#define RAMP_CRUISE 1
#define RAMP_DECEL 2

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
  uint32_t flag_partial_block;  // Flag indicating the last block completed. Time to load a new one.

  double steps_remaining;
  double step_per_mm;           // Current planner block step/millimeter conversion scalar
  double req_mm_increment;
  double dt_remainder;
  
  uint32_t ramp_type;     // Current segment ramp state
  double mm_complete;      // End of velocity profile from end of current planner block in (mm).
                          // NOTE: This value must coincide with a step(no mantissa) when converted.
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

//MS-->
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


void st_create_segment_up3d(double t, double v_entry, double v_exit)
{
  //==> tmax = 65535*65535 / 50000000 = 85.8 sec. per segment
  int64_t p1 = 1+(int64_t)(t*F_CPU)/65535;
  for(;;)
  {
    int64_t p2 = (int64_t)(t*F_CPU/p1);

    //minimum timer value UP CPU can use is 100
    if( p2<100 ) p2=100;
    
    //s = v*t
    //s linear speed
    double s_x = v_entry*t*pl_block->factor[X_AXIS];
    double s_y = v_entry*t*pl_block->factor[Y_AXIS];
    double s_a = v_entry*t*pl_block->factor[A_AXIS];
    
    //s acceleration
    double sa_x = (v_exit-v_entry)*t*pl_block->factor[X_AXIS];
    double sa_y = (v_exit-v_entry)*t*pl_block->factor[Y_AXIS];
    double sa_a = (v_exit-v_entry)*t*pl_block->factor[A_AXIS];

    int64_t p6 = (int64_t)(sa_x/(p1*p1));
    int64_t p7 = (int64_t)(sa_y/(p1*p1));
    int64_t p8 = (int64_t)(sa_a/(p1*p1));

    int64_t p3 = (int64_t)(s_x/p1) + (int64_t)((sa_x-p6*p1*p1)/2/p1);
    int64_t p4 = (int64_t)(s_y/p1) + (int64_t)((sa_y-p7*p1*p1)/2/p1);
    int64_t p5 = (int64_t)(s_a/p1) + (int64_t)((sa_a-p8*p1*p1)/2/p1);

    //test format limits
    if( (p3<-32767) || (p3>32767) || (p4<-32767) || (p4>32767) || (p5<-32767) || (p5>32767) ||
        (p6<-32767) || (p6>32767) || (p7<-32767) || (p7>32767) || (p8<-32767) || (p8>32767) )
    {
      p1++;           //increase segment repeat count and try again
      continue;
    }

    //calculate xsteps generated like mcu in printer (THERE IS A BAD *FLOOR* ROUNDING INSIDE!)
    int64_t sx = floor((float)((p3*p1+p6*p1*p1/2))/512)*512;
    int64_t sy = floor((float)((p4*p1+p7*p1*p1/2))/512)*512;
    int64_t sa = floor((float)((p5*p1+p8*p1*p1/2))/512)*512;
    
    //calculate mcu rounding error compared to real xsteps required
    int64_t t_ex = g_ex + s_x + sa_x/2 - sx;
    int64_t t_ey = g_ey + s_y + sa_y/2 - sy;
    int64_t t_ea = g_ea + s_a + sa_a/2 - sa;

    //compensate rounding errors by adding it to initial speed values
    int64_t ex = t_ex/p1; if((p3+ex)>32767){ex=32767-p3;} if((p3+ex)<-32767){ex=-32767-p3;} p3+=ex;
    int64_t ey = t_ey/p1; if((p4+ey)>32767){ey=32767-p4;} if((p4+ey)<-32767){ey=-32767-p4;} p4+=ey;
    int64_t ea = t_ea/p1; if((p5+ea)>32767){ea=32767-p5;} if((p5+ea)<-32767){ea=-32767-p5;} p5+=ea;

    //calculate xsteps generated like mcu (again)
    sx = floor((float)((p3*p1+p6*p1*p1/2))/512)*512;
    sy = floor((float)((p4*p1+p7*p1*p1/2))/512)*512;
    sa = floor((float)((p5*p1+p8*p1*p1/2))/512)*512;

    //calculate remaining mcu rounding error compared to real xsteps required
    g_ex += s_x + sa_x/2 - sx;
    g_ey += s_y + sa_y/2 - sy;
    g_ea += s_a + sa_a/2 - sa;

    // add new segment
    segment_up3d_t *seg = &segment_buffer[segment_buffer_head];
    seg->p1 = p1; seg->p2 = p2; seg->p3 = p3; seg->p4 = p4; seg->p5 = p5; seg->p6 = p6; seg->p7 = p7; seg->p8 = p8;

    // increment segment buffer indices
    segment_buffer_head = segment_next_head;
    if ( ++segment_next_head == SEGMENT_BUFFER_SIZE ) { segment_next_head = 0; }

#ifdef X_INSERT_STEP_CORRECTIONS
    //check if mcu rounding error is big enough to issue fast steps to compensate
    if( (llabs(g_ex)>511) || (llabs(g_ey)>511) || (llabs(g_ea)>511) )
    {
      // add missing steps to a "tiny segment, fast" new segment with almost no time
      segment_up3d_t *seg = &segment_buffer[segment_buffer_head];
      seg->p1 = 1; seg->p2 = 100;
      if( llabs(g_ex)>511 ) {seg->p3 = (g_ex/512)*512; g_ex-=seg->p3;} else seg->p3=0;
      if( llabs(g_ey)>511 ) {seg->p4 = (g_ey/512)*512; g_ey-=seg->p4;} else seg->p4=0;
      if( llabs(g_ea)>511 ) {seg->p5 = (g_ea/512)*512; g_ea-=seg->p5;} else seg->p5=0;
      seg->p6 = 0; seg->p7 = 0; seg->p8 = 0;
      
      // increment segment buffer indices
      segment_buffer_head = segment_next_head;
      if ( ++segment_next_head == SEGMENT_BUFFER_SIZE ) { segment_next_head = 0; }
    }
#endif
    
    //always leave
    break;
  }
}
//MS<--

// Reset and clear stepper subsystem variables
void st_reset()
{
  // Initialize stepper algorithm variables.
  memset(&prep, 0, sizeof(prep));
  pl_block = NULL;  // Planner block pointer used by segment buffer
  segment_buffer_tail = 0;
  segment_buffer_head = 0; // empty = tail
  segment_next_head = 1;

//MS-->
  g_ex = g_ey = g_ea = 0;
//MS<--
}

// Called by planner_recalculate() when the executing block is updated by the new plan.
void st_update_plan_block_parameters()
{ 
  if (pl_block != NULL) { // Ignore if at start of a new block.
    prep.flag_partial_block = true;
    pl_block->entry_speed_sqr = prep.current_speed*prep.current_speed; // Update entry speed.
    pl_block = NULL; // Flag st_prep_segment() to load new velocity profile.
  }
}

/* Prepares step segment buffer. Continuously called from main program. 

   The segment buffer is an intermediary buffer interface between the execution of steps
   by the stepper algorithm and the velocity profiles generated by the planner. The stepper
   algorithm only executes steps within the segment buffer and is filled by the main program
   when steps are "checked-out" from the first block in the planner buffer. This keeps the
   step execution and planning optimization processes atomic and protected from each other.
   The number of steps "checked-out" from the planner buffer and the number of segments in
   the segment buffer is sized and computed such that no operation in the main program takes
   longer than the time it takes the stepper algorithm to empty it before refilling it. 
   Currently, the segment buffer conservatively holds roughly up to 40-50 msec of steps.
   NOTE: Computation units are in steps, millimeters, and minutes.
*/
void st_prep_buffer()
{
  if (segment_buffer_tail != segment_next_head) { // Check if we need to fill the buffer.

    // Determine if we need to load a new planner block or if the block has been replanned. 
    if (pl_block == NULL) {
      pl_block = plan_get_current_block(); // Query planner for a queued block
      if (pl_block == NULL) { return; } // No planner blocks. Exit.
                      
      // Check if the segment buffer completed the last planner block. If so, load the Bresenham
      // data for the block. If not, we are still mid-block and the velocity profile was updated. 
      if (prep.flag_partial_block) {
        prep.flag_partial_block = false; // Reset flag
      } else {
        // Increment stepper common data index to store new planner block data. 
        if ( ++prep.st_block_index == (SEGMENT_BUFFER_SIZE-1) ) { prep.st_block_index = 0; }

        // Initialize segment buffer data for generating the segments.
        prep.steps_remaining = pl_block->step_event_count;
        prep.step_per_mm = prep.steps_remaining/pl_block->millimeters;
        prep.req_mm_increment = REQ_MM_INCREMENT_SCALAR/prep.step_per_mm;
        prep.dt_remainder = 0.0; // Reset for new planner block

        prep.current_speed = sqrt(pl_block->entry_speed_sqr);
      }

      /* --------------------------------------------------------------------------------- 
         Compute the velocity profile of a new planner block based on its entry and exit
         speeds, or recompute the profile of a partially-completed planner block if the 
         planner has updated it. For a commanded forced-deceleration, such as from a feed 
         hold, override the planner velocities and decelerate to the target exit speed.
      */
      prep.mm_complete = 0.0; // Default velocity profile complete at 0.0mm from end of block.
      double inv_2_accel = 0.5/pl_block->acceleration;

      // Compute or recompute velocity profile parameters of the prepped planner block.
      prep.ramp_type = RAMP_ACCEL; // Initialize as acceleration ramp.
      prep.accelerate_until = pl_block->millimeters; 
      prep.exit_speed = plan_get_exec_block_exit_speed();   
      double exit_speed_sqr = prep.exit_speed*prep.exit_speed;
      double intersect_distance =
              0.5*(pl_block->millimeters+inv_2_accel*(pl_block->entry_speed_sqr-exit_speed_sqr));
      if (intersect_distance > 0.0) {
        if (intersect_distance < pl_block->millimeters) { // Either trapezoid or triangle types
          // NOTE: For acceleration-cruise and cruise-only types, following calculation will be 0.0.
          prep.decelerate_after = inv_2_accel*(pl_block->nominal_speed_sqr-exit_speed_sqr);
          if (prep.decelerate_after < intersect_distance) { // Trapezoid type
            prep.maximum_speed = sqrt(pl_block->nominal_speed_sqr);
            if (pl_block->entry_speed_sqr == pl_block->nominal_speed_sqr) { 
              // Cruise-deceleration or cruise-only type.
              prep.ramp_type = RAMP_CRUISE;
            } else {
              // Full-trapezoid or acceleration-cruise types
              prep.accelerate_until -= inv_2_accel*(pl_block->nominal_speed_sqr-pl_block->entry_speed_sqr); 
            }
          } else { // Triangle type
            prep.accelerate_until = intersect_distance;
            prep.decelerate_after = intersect_distance;
            prep.maximum_speed = sqrt(2.0*pl_block->acceleration*intersect_distance+exit_speed_sqr);
          }          
        } else { // Deceleration-only type
          prep.ramp_type = RAMP_DECEL;
          // prep.decelerate_after = pl_block->millimeters;
          prep.maximum_speed = prep.current_speed;
        }
      } else { // Acceleration-only type
        prep.accelerate_until = 0.0;
        // prep.decelerate_after = 0.0;
        prep.maximum_speed = prep.exit_speed;
      }
    }

    /*------------------------------------------------------------------------------------
        Compute the average velocity of this new segment by determining the total distance
      traveled over the segment time DT_SEGMENT. The following code first attempts to create 
      a full segment based on the current ramp conditions. If the segment time is incomplete 
      when terminating at a ramp state change, the code will continue to loop through the
      progressing ramp states to fill the remaining segment execution time. However, if 
      an incomplete segment terminates at the end of the velocity profile, the segment is 
      considered completed despite having a truncated execution time less than DT_SEGMENT.
        The velocity profile is always assumed to progress through the ramp sequence:
      acceleration ramp, cruising state, and deceleration ramp. Each ramp's travel distance
      may range from zero to the length of the block. Velocity profiles can end either at 
      the end of planner block (typical) or mid-block at the end of a forced deceleration, 
      such as from a feed hold.
    */
    double dt_max = DT_SEGMENT; // Maximum segment time
    double dt = 0.0; // Initialize segment time
    double time_var = dt_max; // Time worker variable
    double mm_var; // mm-Distance worker variable
    double speed_var; // Speed worker variable   
    double mm_remaining = pl_block->millimeters; // New segment distance from end of block.
    double minimum_mm = mm_remaining-prep.req_mm_increment; // Guarantee at least one step.

    if (minimum_mm < 0.0) { minimum_mm = 0.0; }

    do {
      //MS-->
      time_var = dt_max; //restart a new segment time with each iteration
      //<--MS
      
      switch (prep.ramp_type) {
        case RAMP_ACCEL: 
          // NOTE: Acceleration ramp only computes during first do-while loop.
          speed_var = pl_block->acceleration*time_var;
          mm_remaining -= time_var*(prep.current_speed + 0.5*speed_var);

          if (mm_remaining < prep.accelerate_until) 
          { // End of acceleration ramp.
            // Acceleration-cruise, acceleration-deceleration ramp junction, or end of block.
            mm_remaining = prep.accelerate_until; // NOTE: 0.0 at EOB
            time_var = 2.0*(pl_block->millimeters-mm_remaining)/(prep.current_speed+prep.maximum_speed);

            //MS-->
            st_create_segment_up3d(time_var, prep.current_speed, prep.maximum_speed);
            //<--MS

            if (mm_remaining == prep.decelerate_after) { prep.ramp_type = RAMP_DECEL; }
            else { prep.ramp_type = RAMP_CRUISE; }
            prep.current_speed = prep.maximum_speed;
          } 
          else 
          { // Acceleration only. 

            //MS-->
            st_create_segment_up3d(time_var, prep.current_speed, prep.current_speed + speed_var);
            //<--MS

            prep.current_speed += speed_var;
          }


          break;
        case RAMP_CRUISE: 
          // NOTE: mm_var used to retain the last mm_remaining for incomplete segment time_var calculations.
          // NOTE: If maximum_speed*time_var value is too low, round-off can cause mm_var to not change. To 
          //   prevent this, simply enforce a minimum speed threshold in the planner.
          mm_var = mm_remaining - prep.maximum_speed*time_var;

          if (mm_var < prep.decelerate_after) 
          { // End of cruise. 
            // Cruise-deceleration junction or end of block.
            time_var = (mm_remaining - prep.decelerate_after)/prep.maximum_speed;

            mm_remaining = prep.decelerate_after; // NOTE: 0.0 at EOB

            //MS-->
            st_create_segment_up3d(time_var, prep.maximum_speed, prep.maximum_speed);
            //<--MS

            prep.ramp_type = RAMP_DECEL;
          } 
          else 
          { // Cruising only.
            mm_remaining = mm_var; 

            //MS-->
            st_create_segment_up3d(time_var, prep.maximum_speed, prep.maximum_speed);
            //<--MS
          } 

          break;
        default: // case RAMP_DECEL:
          // NOTE: mm_var used as a misc worker variable to prevent errors when near zero speed.
          speed_var = pl_block->acceleration*time_var; // Used as delta speed (mm/min)

          if (prep.current_speed > speed_var) { // Check if at or below zero speed.
            // Compute distance from end of segment to end of block.
            mm_var = mm_remaining - time_var*(prep.current_speed - 0.5*speed_var); // (mm)
            if (mm_var > prep.mm_complete) { // Deceleration only.
              mm_remaining = mm_var;

            //MS-->
            st_create_segment_up3d(time_var, prep.current_speed, prep.current_speed - speed_var);
            //<--MS

              prep.current_speed -= speed_var;
              break; // Segment complete. Exit switch-case statement. Continue do-while loop.
            }
          } // End of block or end of forced-deceleration.

          time_var = 2.0*(mm_remaining-prep.mm_complete)/(prep.current_speed+prep.exit_speed);
          mm_remaining = prep.mm_complete; 

          //MS-->
          st_create_segment_up3d(time_var, prep.current_speed, prep.exit_speed);
          //<--MS
      }
      dt += time_var; // Add computed ramp time to total segment time.
      if (dt < dt_max) { time_var = dt_max - dt; } // **Incomplete** At ramp junction.
      else {
        if (mm_remaining > minimum_mm) { // Check for very slow segments with zero steps.
          // Increase segment time to ensure at least one step in segment. Override and loop
          // through distance calculations until minimum_mm or mm_complete.
          dt_max += DT_SEGMENT;
          time_var = dt_max - dt;
        } else { 
          break; // **Complete** Exit loop. Segment execution time maxed.
        }
      }

    } while (mm_remaining > prep.mm_complete); // **Complete** Exit loop. Profile complete.

    /* -----------------------------------------------------------------------------------
       Compute segment step rate, steps to execute, and apply necessary rate corrections.
       NOTE: Steps are computed by direct scalar conversion of the millimeter distance 
       remaining in the block, rather than incrementally tallying the steps executed per
       segment. This helps in removing floating point round-off issues of several additions. 
       However, since floats have only 7.2 significant digits, long moves with extremely 
       high step counts can exceed the precision of floats, which can lead to lost steps.
       Fortunately, this scenario is highly unlikely and unrealistic in CNC machines
       supported by Grbl (i.e. exceeding 10 meters axis travel at 200 step/mm).
    */
    double steps_remaining = prep.step_per_mm*mm_remaining; // Convert mm_remaining to steps
    double n_steps_remaining = ceil(steps_remaining); // Round-up current steps remaining
    double last_n_steps_remaining = ceil(prep.steps_remaining); // Round-up last steps remaining

    // Compute segment step rate. Since steps are integers and mm distances traveled are not,
    // the end of every segment can have a partial step of varying magnitudes that are not 
    // executed, because the stepper ISR requires whole steps due to the AMASS algorithm. To
    // compensate, we track the time to execute the previous segment's partial step and simply
    // apply it with the partial step distance to the current segment, so that it minutely
    // adjusts the whole segment rate to keep step output exact. These rate adjustments are 
    // typically very small and do not adversely effect performance, but ensures that Grbl
    // outputs the exact acceleration and velocity profiles as computed by the planner.
    dt += prep.dt_remainder; // Apply previous segment partial step execute time
    double inv_rate = dt/(last_n_steps_remaining - steps_remaining); // Compute adjusted step rate inverse
    prep.dt_remainder = (n_steps_remaining - steps_remaining)*inv_rate; // Update segment partial step time

    // Setup initial conditions for next segment.
    if (mm_remaining > prep.mm_complete) { 
      // Normal operation. Block incomplete. Distance remaining in block to be executed.
      pl_block->millimeters = mm_remaining;      
      prep.steps_remaining = steps_remaining;  
    } else { 
      // End of planner block or forced-termination. No more distance to be executed.
      if (mm_remaining > 0.0) { // At end of forced-termination.
        // Reset prep parameters for resuming and then bail. Allow the stepper ISR to complete
        // the segment queue, where realtime protocol will set new state upon receiving the 
        // cycle stop flag from the ISR. Prep_segment is blocked until then.
        prep.current_speed = 0.0; // NOTE: (=0.0) Used to indicate completed segment calcs for hold.
        prep.dt_remainder = 0.0;
        prep.steps_remaining = ceil(steps_remaining);
        pl_block->millimeters = prep.steps_remaining/prep.step_per_mm; // Update with full steps.
        plan_cycle_reinitialize(); 
        return; // Bail!
      } else { // End of planner block
        // The planner block is complete. All steps are set to be executed in the segment buffer.
        pl_block = NULL; // Set pointer to indicate check and load next planner block.
        plan_discard_current_block();
      }
    }

  }
}
