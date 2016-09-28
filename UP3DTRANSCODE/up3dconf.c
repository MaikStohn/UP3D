/*
  up3dconf.c for UP3DTranscoder
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

#include "up3dconf.h"

settings_t settings;

settings_t settings_mini = { 
  .steps_per_mm = { 854.0, 854.0, 854.0 },
  .max_rate = { 200, 200, 50 },
  .acceleration = { 1500, 1500, 300 },
  .junction_deviation = 0.05,
  .x_axes =  1, .y_axes =  0,
  .x_dir  =  1, .y_dir  = -1, .z_dir  = -1,
  .x_hspeed_hi = 50.0, .y_hspeed_hi = 50.0, .z_hspeed_hi = 50.0, .x_hofs_hi =  4.0, .y_hofs_hi =  4.0, .z_hofs_hi =  6.0,
  .x_hspeed_lo = 10.0, .y_hspeed_lo = 10.0, .z_hspeed_lo =  3.0, .x_hofs_lo =  9.0, .y_hofs_lo =  2.0, .z_hofs_lo =  2.0,
  .heatbed_wait_factor = 20.0,
};

settings_t settings_classic_plus = { 
  .steps_per_mm = { 644.0, 644.0, 854.0 },
  .max_rate = { 200, 200, 50 },
  .acceleration = { 1500, 1500, 300 },
  .junction_deviation = 0.05,
  .x_axes =  1, .y_axes =  0,
  .x_dir  =  1, .y_dir  = -1, .z_dir  = -1,
  .x_hspeed_hi = 50.0, .y_hspeed_hi = 50.0, .z_hspeed_hi = 50.0, .x_hofs_hi =  4.0, .y_hofs_hi =  4.0, .z_hofs_hi =  6.0,
  .x_hspeed_lo = 10.0, .y_hspeed_lo = 10.0, .z_hspeed_lo =  3.0, .x_hofs_lo =  2.0, .y_hofs_lo =  2.0, .z_hofs_lo =  2.0,
  .heatbed_wait_factor = 30.0,
};

settings_t settings_box = { 
  .steps_per_mm = { 644.0, 644.0, 854.0 },
  .max_rate = { 200, 200, 50 },
  .acceleration = { 1500, 1500, 300 },
  .junction_deviation = 0.05,
  .x_axes =  1, .y_axes =  0,
  .x_dir  = -1, .y_dir  =  1, .z_dir  = -1,
  .x_hspeed_hi = 50.0, .y_hspeed_hi = 30.0, .z_hspeed_hi = 30.0, .x_hofs_hi =  4.0, .y_hofs_hi =  4.0, .z_hofs_hi =  6.0,
  .x_hspeed_lo = 10.0, .y_hspeed_lo = 10.0, .z_hspeed_lo =  3.0, .x_hofs_lo =  2.0, .y_hofs_lo =  2.0, .z_hofs_lo =  2.0,
  .heatbed_wait_factor = 30.0,
};

// experimental settings for the Cetus3D
// the steps_per_mm for the extruder has been measured on a prototype
settings_t settings_cetus = { 
  .steps_per_mm = { 160.0, 160.0, 236.0 },
  .max_rate = { 200, 200, 50 },
  .acceleration = { 1500, 1500, 300 },
  .junction_deviation = 0.05,
  .x_axes =  1, .y_axes =  0,
  .x_dir  = 1, .y_dir  =  -1, .z_dir  = -1,
  .x_hspeed_hi = 50.0, .y_hspeed_hi = 30.0, .z_hspeed_hi = 30.0, .x_hofs_hi =  4.0, .y_hofs_hi =  4.0, .z_hofs_hi =  6.0,
  .x_hspeed_lo = 10.0, .y_hspeed_lo = 10.0, .z_hspeed_lo =  3.0, .x_hofs_lo =  2.0, .y_hofs_lo =  2.0, .z_hofs_lo =  2.0,
  .heatbed_wait_factor = 0.0,  // no heatbed
};
