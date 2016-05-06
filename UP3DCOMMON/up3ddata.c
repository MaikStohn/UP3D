/*
  up3ddata.c for UP3DTranscoder
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

#include "up3ddata.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

void UP3D_PROG_BLK_Stop( UP3D_BLK *pupblk )
{
  memset( pupblk, 0, sizeof(UP3D_BLK) );
  pupblk->pcmd=UP3DPCMD_Stop; 
}

void UP3D_PROG_BLK_Power( UP3D_BLK *pupblk, bool on )
{
  memset( pupblk, 0, sizeof(UP3D_BLK) );
  pupblk->pcmd=UP3DPCMD_SetState; 
  pupblk->pdat1.l=UP3DPCMD_SetState_StatePower;
  pupblk->pdat2.l=on?UP3DPCMD_SetState_ValueOn:UP3DPCMD_SetState_ValueOff;
}

void UP3D_PROG_BLK_Beeper( UP3D_BLK *pupblk, bool on )
{
  memset( pupblk, 0, sizeof(UP3D_BLK) );
  pupblk->pcmd=UP3DPCMD_SetState; 
  pupblk->pdat1.l=UP3DPCMD_SetState_StateBeeper;
  pupblk->pdat2.l=on?UP3DPCMD_SetState_ValueOn:UP3DPCMD_SetState_ValueOff;
}

void UP3D_PROG_BLK_Pause( UP3D_BLK *pupblk, uint32_t msec )
{
  memset( pupblk, 0, sizeof(UP3D_BLK) );
  pupblk->pcmd=UP3DPCMD_Pause;
  pupblk->pdat1.l=msec;
}

void UP3D_PROG_BLK_SetParameter( UP3D_BLK *pupblk, uint8_t parameter, int32_t value )
{
  memset( pupblk, 0, sizeof(UP3D_BLK) );
  pupblk->pcmd=UP3DPCMD_SetParameter; 
  pupblk->pdat1.l=parameter;
  pupblk->pdat2.l=value;
}

void UP3D_PROG_BLK_Home( UP3D_BLK *pupblk, UP3D_AXIS axis, float direction, float offset, float speed )
{
  memset( pupblk, 0, sizeof(UP3D_BLK) );
  pupblk->pcmd=UP3DPCMD_HomeAxis; 
  pupblk->pdat1.l=axis;
  pupblk->pdat2.f=speed;
  pupblk->pdat3.f=offset*direction;
}

void UP3D_PROG_BLK_MoveF( UP3D_BLK pupblks[2], float speedX, float posX, float speedY, float posY, float speedZ, float posZ, float speedA, float posA )
{
  memset( pupblks, 0, sizeof(UP3D_BLK)*2 );
  pupblks[0].pcmd=UP3DPCMD_MoveF; pupblks[0].pdat1.f=speedX; pupblks[0].pdat2.f=posX; pupblks[0].pdat3.f=speedY; pupblks[0].pdat4.f=posY;
  pupblks[1].pcmd=UP3DPCMD_MoveF; pupblks[1].pdat1.f=speedZ; pupblks[1].pdat2.f=posZ; pupblks[1].pdat3.f=speedA; pupblks[1].pdat4.f=posA;
}

void UP3D_PROG_BLK_MoveL( UP3D_BLK *pupblk, uint16_t p1, uint16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6, int16_t p7, int16_t p8)
{
  memset( pupblk, 0, sizeof(UP3D_BLK) );
  pupblk->pcmd=UP3DPCMD_MoveL; 
  pupblk->pdat1.s.s1=p1; pupblk->pdat1.s.s2=p2; 
  pupblk->pdat2.s.s1=p3; pupblk->pdat2.s.s2=p4; pupblk->pdat3.s.s1=p5; 
  pupblk->pdat3.s.s2=p6; pupblk->pdat4.s.s1=p7; pupblk->pdat4.s.s2=p8;
}

void UP3D_PROG_BLK_WaitIfNot( UP3D_BLK *pupblk, uint8_t parameter, int32_t value, char compchar )
{
  memset( pupblk, 0, sizeof(UP3D_BLK) );
  pupblk->pcmd=UP3DPCMD_WaitIfNot; 
  pupblk->pdat1.l=parameter;
  pupblk->pdat2.l=value;
  pupblk->pdat3.l=compchar;
}

