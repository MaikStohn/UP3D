//////////////////
//Author: M.Stohn/
//////////////////

#include "up3d.h"
#include "up3dcomm.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


//???

/*
U 0x2E GetCurMaterial ==> (usageinfo/cartridgeserial)
U 0x43 ClearProgramBuf
U 0x49 ('0'+port) GetInPort
U 0x4C ('0'+prg) LoadRomProg  (8=extrude nozzle 1, 9=extrude nozzle 2)
U 0x4A ('0'+AXIS,FLOAT,FLOAT) MotorJogTo
U 0x6A ('0'+AXIS,FLOAT,FLOAT) MotorJog
U 0x53 StopProgram
U SetSystemVar(0x38,1) ReadCMDFromSD
U 0x58 Start/ResumeProgram
U 0x70 ('0'+AXIS) GetMotorPos ==> (int32 steps)

U 0x6C (x,y) UseSDProgramBuf

 //63 was used in older versions
*/


//CMD 20 ==> ...

//?CMD 4A MOVE  00 = X 01 = Y (FLOAT,FLOAT)
//CMD 6A MOVE  00 = X, 01 = Y, 02 = Z (FLOAT,FLOAT)
//CMD 53 = STOP?

//CMD 70 ('0'+AXIS) ==> INT32 = STEPS
//CMD 73 ('0'+AXIS,FLOAT,?)

//CMD 22 WRITE SD
//CMD 23 READ SD (int8_t index, int8_t len(<60)) : 23 0A 10 ==> GetNozzelHeightSD ==> 123456.0 | height

//CMD 2B SEND RANDOM (8byte)

//CMD 52 GET PRINTER PARAMS ==> long answer

//CMD 77 ERASE 01 = ROMPROG 02 = CONFIG
//CMD 75 SENDBLK 01 = 1ST BLOCK 02 = FOLLOW BLOCKS
//CMD 55 COMMIT 00 = ? 01 = ? 02 = OUTPORTBLK 03 = INPORTBLK 04 = ? 05 = SETBLK 06 = IDBLK

bool UP3D_Open()
{
  return UP3DCOMM_Open();
}

void UP3D_Close()
{
  UP3DCOMM_Close();
}

unsigned int UP3D_IsPrinterResponsive()
{
  static const uint8_t UP3D_CMD_1[] = { 0x01, 0x00 }; //GetFwVersion ?
  uint8_t resp[2048];  
  if( (UP3DCOMM_Write( UP3D_CMD_1, sizeof(UP3D_CMD_1) ) != sizeof(UP3D_CMD_1)) ||
      (5 != UP3DCOMM_Read( resp, sizeof(resp) )) ||
      (6 != resp[4]) )
    return false;
  unsigned int version = (unsigned int)resp[1]<<8 | resp[0];
  printf("Up Printer Version: V%d.%02d\n", version / 100, version % 100); 
  return version;
}

bool UP3D_ClearProgramBuf()
{
  static const uint8_t UP3D_CMD_43[] = { 0x43 };
  uint8_t resp[2048];  

  if( (UP3DCOMM_Write( UP3D_CMD_43, sizeof(UP3D_CMD_43) ) != sizeof(UP3D_CMD_43)) ||
      (1 != UP3DCOMM_Read( resp, sizeof(resp) )) ||
      (6 != resp[0]) )
    return false;

  return true;
}

bool UP3D_UseSDProgramBuf( uint8_t progID, bool enableWrite )
{
  if( progID>9 )
    return false;
    
  uint8_t UP3D_CMD_6C_X_Y[3] = { 0x6C, progID, enableWrite?0x01:0x00 };
  uint8_t resp[2048];  

  if( (UP3DCOMM_Write( UP3D_CMD_6C_X_Y, sizeof(UP3D_CMD_6C_X_Y) ) != sizeof(UP3D_CMD_6C_X_Y)) ||
      (1 != UP3DCOMM_Read( resp, sizeof(resp) )) ||
      (6 != resp[0]) )
    return false;

  return true;
}

bool UP3D_SetPrintJobInfo( uint8_t progID, uint8_t x, uint32_t y )
{
  if( progID>9 )
    return false;
    
  uint8_t UP3D_CMD_63_X_Y[7] = { 0x63, progID, x, (y>>0)&0xFF, (y>>8)&0xFF, (y>>16)&0xFF, (y>>24)&0xFF };
  uint8_t resp[2048];  

  if( (UP3DCOMM_Write( UP3D_CMD_63_X_Y, sizeof(UP3D_CMD_63_X_Y) ) != sizeof(UP3D_CMD_63_X_Y)) ||
      (1 != UP3DCOMM_Read( resp, sizeof(resp) )) ||
      (6 != resp[0]) )
    return false;

  return true;
}

bool UP3D_InsertRomProgram( uint8_t prog )
{
  uint8_t UP3D_CMD_4C_X[] = { 0x4C, '0'+prog };
  uint8_t resp[2048];  

  if( (UP3DCOMM_Write( UP3D_CMD_4C_X, sizeof(UP3D_CMD_4C_X) ) != sizeof(UP3D_CMD_4C_X)) ||
      (5 != UP3DCOMM_Read( resp, sizeof(resp) )) ||
      (6 != resp[4]) ||
      (1 != resp[0])
    )
    return false;
    
  return true;
}

uint32_t UP3D_GetFreeBlocks()
{
  static const uint8_t UP3D_CMD_46[] = { 0x46 };
  uint8_t resp[2048];

  if( (UP3DCOMM_Write( UP3D_CMD_46, sizeof(UP3D_CMD_46) ) != sizeof(UP3D_CMD_46)) ||
      (5 != UP3DCOMM_Read( resp, sizeof(resp) )) ||
      (6 != resp[4]) )
    return 0;

  uint32_t r;
  memcpy( &r, resp, sizeof(r) );
  return r;
}

bool UP3D_WriteBlocks( const UP3D_BLK *data, uint8_t blocks )
{
  uint8_t send[2048];
  for( ;blocks>0; )
  {
    uint8_t b = blocks;
    if( b>72 ) b = 72; //??? needed ???

    uint32_t fblocks= UP3D_GetFreeBlocks();
    if( b>fblocks ) b = fblocks;

//    for( ;out+2+20*b>2048; b--) //??? needed ???
//      ;

    if( b>0 )
    {
      uint8_t UP3D_CMD_2F_X[] = { 0x2F, b };
      memcpy( &send[0], UP3D_CMD_2F_X, 2 );
      memcpy( &send[2], data, 20*b );
      data++;
      if( UP3DCOMM_Write( send, 2+20*b ) != 2+20*b )
        return false;

      blocks -= b;
      data += b;
    }
    else
      usleep( 100000 );
  }

  return true;
}

bool UP3D_WriteBlock( const UP3D_BLK *data ) 
{
  return UP3D_WriteBlocks(data,1);
}

bool UP3D_StartResumeProgram()
{
  static const uint8_t UP3D_CMD_58[] = { 0x58 };
  uint8_t resp[2048];  

  if( (UP3DCOMM_Write( UP3D_CMD_58, sizeof(UP3D_CMD_58) ) != sizeof(UP3D_CMD_58)) ||
      (0 == UP3DCOMM_Read( resp, sizeof(resp) )) /*||
      (6 != resp[0])*/ )                                //returns "Exe Count: 0" 06
    return false;
  return true;
}

bool UP3D_SetParameter(uint8_t param, uint32_t value)
{
  uint8_t UP3D_CMD_56_X_Y[6] = { 0x56, param, 0x00,0x00,0x00,0x00 };
  memcpy( &UP3D_CMD_56_X_Y[2], &value, sizeof(value) );
  uint8_t resp[2048];

  if( (UP3DCOMM_Write( UP3D_CMD_56_X_Y, sizeof(UP3D_CMD_56_X_Y) ) != sizeof(UP3D_CMD_56_X_Y)) ||
      (1 != UP3DCOMM_Read( resp, sizeof(resp) )) ||
      (6 != resp[0]) )
    return false;
    
  return true;
}

uint32_t UP3D_GetParameter(uint8_t param)
{
  uint8_t UP3D_CMD_76_X[] = { 0x76, param };
  uint8_t resp[2048];

  if( (UP3DCOMM_Write( UP3D_CMD_76_X, sizeof(UP3D_CMD_76_X) ) != sizeof(UP3D_CMD_76_X)) ||
      (5 != UP3DCOMM_Read( resp, sizeof(resp) )) ||
      (6 != resp[4]) )
    return 0;

  uint32_t r;
  memcpy( &r, resp, sizeof(r) );
  return r;
}

int32_t UP3D_GetAxisPosition(int axis)
{
  if( (axis<1) || (axis>4) )
    return -1;
  uint8_t UP3D_CMD_70_X[] = { 0x70, '0'+axis-1 };
  uint8_t resp[2048];

  if( (UP3DCOMM_Write( UP3D_CMD_70_X, sizeof(UP3D_CMD_70_X) ) != sizeof(UP3D_CMD_70_X)) ||
      (5 != UP3DCOMM_Read( resp, sizeof(resp) )) ||
      (6 != resp[4]) )
    return 0;

  int32_t i;
  memcpy( &i, resp, sizeof(i) );
  return i;
}

bool UP3D_SendRandom()
{
  uint8_t UP3D_CMD_2B_RRR[9] = { 0x2B, 0xCA,0xFE,0x00,0x00,0x00,0x00,0x12,0x34 };
  uint8_t resp[2048];

  if( (UP3DCOMM_Write( UP3D_CMD_2B_RRR, sizeof(UP3D_CMD_2B_RRR) ) != sizeof(UP3D_CMD_2B_RRR)) ||
      (1 != UP3DCOMM_Read( resp, sizeof(resp) )) ||
      (6 != resp[0]) )
    return false;
    
  return true;
}

///////////////////////////////////////////


int32_t UP3D_GetMachineState()
{
  int32_t state = UP3D_GetParameter(0x00);
  if( (state<0) || (state>4) )
    state = 5;
  return state;
}

int32_t UP3D_GetProgramState()
{
  int32_t state = UP3D_GetParameter(0x01);
  if( (state<0) || (state>3) )
    state = 4;
  return state;
}

int32_t UP3D_GetSystemState()
{
  int32_t state = UP3D_GetParameter(0x10);
  if( (state<0) || (state>22) )
    state = 23;
  return state;
}

int32_t UP3D_GetMotorState(int motor)
{
  if( (motor<1) || (motor>4) )
    return -1;
  int32_t state = UP3D_GetParameter(0x02 + motor - 1);
  if( (state<0) || (state>5) )
    state = 6;
  return state;
}

uint32_t UP3D_GetPositiveLimitState(int motor)
{
  if( (motor<1) || (motor>4) )
    return -1;
  return UP3D_GetParameter(0x22 + motor - 1);
}

uint32_t UP3D_GetNegativeLimitState(int motor)
{
  if( (motor<1) || (motor>4) )
    return -1;
  return UP3D_GetParameter(0x26 + motor - 1);
}

int32_t UP3D_GetLayer()
{
  return UP3D_GetParameter(0x0A);
}

float UP3D_GetHeight()
{
  uint32_t f = UP3D_GetParameter(0x0B);
  return *((float*)&f);
}

int32_t UP3D_GetPercent()
{
  return UP3D_GetParameter(0x4C);
}

int32_t UP3D_GetTimeRemaining()
{
  return UP3D_GetParameter(0x4D);
}

int32_t UP3D_GetPrintState()
{
  return UP3D_GetParameter(0x1C);
}

int32_t UP3D_GetHeaterTempReached(int heater)
{
  if( (heater<1) || (heater>2) )
    return -1;
  return UP3D_GetParameter(0x12 + heater - 1);
}

int32_t UP3D_GetHeaterState(int heater)
{
  if( (heater<1) || (heater>3) )
    return -1;
  return UP3D_GetParameter(0x14 + heater - 1);
}

int32_t UP3D_GetHeaterSetTemp(int heater)
{
  if( (heater<1) || (heater>4) )
    return -1;
  return UP3D_GetParameter(0x39 + heater - 1);
}

float UP3D_GetHeaterTemp(int heater)
{
  if( (heater<1) || (heater>4) )
    return -1;

  uint32_t f = UP3D_GetParameter(0x06 + heater -1);
  return *((float*)&f);
}

int32_t UP3D_GetHeaterTargetTemp(int heater)
{
  if( (heater<1) || (heater>4) )
    return -1;
  return UP3D_GetParameter(0x2D + heater -1);
}

int32_t UP3D_GetHF(int heater)
{
  if( (heater<1) || (heater>4) )
    return -1;
  return UP3D_GetParameter(0x18 + heater - 1);
}

int32_t UP3D_GetSupportState()
{
  return UP3D_GetParameter(0x11);
}

int32_t UP3D_GetFeedErrorLength()
{
  return UP3D_GetParameter(0x1D); //QQQ /1000 ?
}

float UP3D_GetFeedBackLength()
{
  return ((float)UP3D_GetParameter(0x36))/1000;
}

int32_t UP3D_GetAxisState(int axis)
{
  if( (axis<1) || (axis>4) )
    return -1;
  int32_t state = UP3D_GetParameter(0x1E + axis - 1);
  if( state>2 )
    state = 3;
  return state;
}

float UP3D_GetChangeNozzleTime()
{
  return ((float)UP3D_GetParameter(0x2A))/1000;
}

float UP3D_GetJumpTime()
{
  return ((float)UP3D_GetParameter(0x2B))/1000;
}

int32_t UP3D_GetUsedNozzle()
{
  return UP3D_GetParameter(0x2C);
}

int32_t UP3D_GetCheckDoor()
{
  return UP3D_GetParameter(0x31);
}

int32_t UP3D_GetCheck24V()
{
  return UP3D_GetParameter(0x32);
}

int32_t UP3D_GetCheckPowerKey()
{
  return UP3D_GetParameter(0x33);
}

int32_t UP3D_GetCheckLightKey()
{
  return UP3D_GetParameter(0x34);
}

int32_t UP3D_GetCheckWorkRoomFan()
{
  return UP3D_GetParameter(0x35);
}

int32_t UP3D_GetReadFromSD()
{
  return UP3D_GetParameter(0x36);
}

int32_t UP3D_GetWriteToSD()
{
  return UP3D_GetParameter(0x37);
}


