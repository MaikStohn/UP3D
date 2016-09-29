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

//#define X_INSERT_STEP_CORRECTIONS

#define N_AXIS 3 // Number of axes (X,Y,A)

#define X_AXIS 0 // Axis indexing value. 
#define Y_AXIS 1
#define A_AXIS 2

//global settings
typedef struct {
  // Axis settings
  double steps_per_mm[N_AXIS];
  double max_rate[N_AXIS];
  double acceleration[N_AXIS];
  double junction_deviation;
  int    x_axes;
  int    y_axes;
  int    x_dir;
  int    y_dir;
  int    z_dir;
  double x_hspeed_hi;
  double y_hspeed_hi;
  double z_hspeed_hi;
  double x_hofs_hi;
  double y_hofs_hi;
  double z_hofs_hi;
  double x_hspeed_lo;
  double y_hspeed_lo;
  double z_hspeed_lo;
  double x_hofs_lo;
  double y_hofs_lo;
  double z_hofs_lo;
  double heatbed_wait_factor;
} settings_t;

extern settings_t settings;

extern settings_t settings_mini;
extern settings_t settings_classic_plus;
extern settings_t settings_box;
extern settings_t settings_cetus;


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

// Useful macros
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))


#endif //up3d_h
