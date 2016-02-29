/*
  gcodeparser.h for UP3DTranscoder
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

#ifndef gcodeparser_h
#define gcodeparser_h

#include <stdint.h>
#include <stdbool.h>

void   gcp_reset();
bool   gcp_process_line(const char* gcodeline);
int    gcp_get_layer();
double gcp_get_height();

#endif //gcodeparser_h
