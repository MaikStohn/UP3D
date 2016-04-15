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
  .steps_per_mm = { 854.0 * 512, 854.0 * 512, 854.0 * 512 },
  .max_rate = { 200, 200, 50 },
  .acceleration = { 3000, 3000, 300 },
  .max_travel = { 122.0, 122.0, 100000000.0 },
  .junction_deviation = 0.01,
  .x_axes = 1,
  .y_axes = 0,
};

settings_t settings_classic_plus = { 
  .steps_per_mm = { 644.0 * 512, 644.0 * 512, 854.0 * 512 },
  .max_rate = { 200, 200, 50 },
  .acceleration = { 3000, 3000, 300 },
  .max_travel = { 142.0, 142.0, 100000000.0 },
  .junction_deviation = 0.01,
  .x_axes = 1,
  .y_axes = 0,
};

settings_t settings_box = { 
  .steps_per_mm = { 644.0 * 512, 644.0 * 512, 854.0 * 512 },
  .max_rate = { 200, 200, 50 },
  .acceleration = { 3000, 3000, 300 },
  .max_travel = { 264.0, 212.0, 100000000.0 },
  .junction_deviation = 0.01,
  .x_axes = 0,
  .y_axes = 1,
};
