/*
  up3dconf.h for UP3DTranscoder
  M. Stohn 2016
  
  This is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License.
  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef up3dconf_h
#define up3dconf_h

#include <stdint.h>
#include <stdbool.h>

#define F_CPU 50000000

#define X_INSERT_STEP_CORRECTIONS
//#define X_WAIT_HEATBED_FACTOR

#define N_AXIS 3 // Number of axes (X,Y,A)

#define X_AXIS 0 // Axis indexing value. 
#define Y_AXIS 1
#define A_AXIS 2

#define DEFAULT_X_STEPS_PER_MM ( 854.0 * 512) // *512 for internal fraction representation in printer
#define DEFAULT_Y_STEPS_PER_MM ( 854.0 * 512)
#define DEFAULT_A_STEPS_PER_MM ( 854.0 * 512) // ??? 40.0*512

//MAX #define DEFAULT_X_MAX_RATE (700.0)    // mm/sec
//MAX #define DEFAULT_Y_MAX_RATE (700.0)    // mm/sec
//MAX #define DEFAULT_A_MAX_RATE (50.0)     // mm/sec
#define DEFAULT_X_MAX_RATE (200.0)    // mm/sec
#define DEFAULT_Y_MAX_RATE (200.0)    // mm/sec
#define DEFAULT_A_MAX_RATE (50.0)     // mm/sec

//MAX #define DEFAULT_X_ACCELERATION (5000) // mm/sec^2
//MAX #define DEFAULT_Y_ACCELERATION (5000) // mm/sec^2
//MAX #define DEFAULT_A_ACCELERATION (5000) // mm/sec^2
#define DEFAULT_X_ACCELERATION (3000) // mm/sec^2
#define DEFAULT_Y_ACCELERATION (3000) // mm/sec^2
#define DEFAULT_A_ACCELERATION (300)  // mm/sec^2

#define DEFAULT_X_MAX_TRAVEL 122.0       // mm
#define DEFAULT_Y_MAX_TRAVEL 122.0       // mm
#define DEFAULT_A_MAX_TRAVEL 100000000.0 // mm

#define DEFAULT_JUNCTION_DEVIATION 0.01  // mm

//global settings
typedef struct {
  // Axis settings
  double steps_per_mm[N_AXIS];
  double max_rate[N_AXIS];
  double acceleration[N_AXIS];
  double max_travel[N_AXIS];
  double junction_deviation;
} settings_t;
extern settings_t settings;

// Minimum planner junction speed. Sets the default minimum junction speed the planner plans to at
// every buffer block junction, except for starting from rest and end of the buffer, which are always
// zero. This value controls how fast the machine moves through junctions with no regard for acceleration
// limits or angle between neighboring block line move directions. This is useful for machines that can't
// tolerate the tool dwelling for a split second, i.e. 3d printers or laser cutters. If used, this value
// should not be much greater than zero or to the minimum value necessary for the machine to work.
#define MINIMUM_JUNCTION_SPEED 0.0 // (mm/min)

// Sets the minimum feed rate the planner will allow. Any value below it will be set to this minimum
// value. This also ensures that a planned motion always completes and accounts for any floating-point
// round-off errors. Although not recommended, a lower value than 1.0 mm/min will likely work in smaller
// machines, perhaps to 0.1mm/min, but your success may vary based on multiple factors.
#define MINIMUM_FEED_RATE 1.0 // (mm/min)

// The temporal resolution of the acceleration management subsystem. A higher number gives smoother
// acceleration, particularly noticeable on machines that run at very high feedrates, but may negatively
// impact performance. The correct value for this parameter is machine dependent, so it's advised to
// set this only as high as needed. Approximate successful values can widely range from 50 to 200 or more.
// NOTE: Changing this value also changes the execution time of a segment in the step segment buffer..
// When increasing this value, this stores less overall time in the segment buffer and vice versa. Make
// certain the step segment buffer is increased/decreased to account for these changes.
//#define ACCELERATION_TICKS_PER_SECOND 100.0
#define ACCELERATION_TICKS_PER_SECOND (0.1/60.0)   //acceleration is handled in machine

// Conversions
#define MM_PER_INCH (25.40)
#define INCH_PER_MM (0.0393701)

// Useful macros
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))

#endif //up3d_h
