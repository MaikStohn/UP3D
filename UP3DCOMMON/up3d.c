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
#include <math.h>

#include "compat.h"

static uint32_t _up3d_connected_fw_version;

bool UP3D_Open()
{
  return UP3DCOMM_Open();
}

void UP3D_Close()
{
  UP3DCOMM_Close();
}

bool UP3D_IsPrinterResponsive()
{
  static const uint8_t UP3D_CMD_1[] = { 0x01, 0x00 }; //GetFwVersion
  uint8_t resp[2048];
  if( (UP3DCOMM_Write( UP3D_CMD_1, sizeof(UP3D_CMD_1) ) != sizeof(UP3D_CMD_1)) ||
      (5 != UP3DCOMM_Read( resp, sizeof(resp) )) ||
      (6 != resp[4]) )
    return false;

  _up3d_connected_fw_version = le32toh(*((uint32_t*)resp));

  return true;
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

  return le32toh( *((uint32_t*)resp) );
}

bool UP3D_WriteBlocks( const UP3D_BLK *data, uint8_t blocks )
{
  uint8_t send[2048];
  for( ;blocks>0; )
  {
    uint8_t b = blocks;

    if( _up3d_connected_fw_version<330 )
      if (b > 3) b = 3; //older firmware versions support max 64 bytes (3 blocks) per write only

    if( b>72 ) b = 72;                          //limit to max. 2048 bytes per logical packet

    uint32_t fblocks= UP3D_GetFreeBlocks();
    if( b>fblocks ) b = fblocks;

    if( b>0 )
    {
      uint8_t UP3D_CMD_2F_X[] = { 0x2F, b };
      memcpy( &send[0], UP3D_CMD_2F_X, 2 );
      memcpy( &send[2], data, 20*b );
      //data++;
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

  return le32toh( *((uint32_t*)resp) );
}

bool UP3D_GetSystemVar(uint8_t param, int32_t *var)
{
  uint8_t UP3D_CMD_76_X[] = { 0x76, param };
  uint8_t resp[2048];

  if( (UP3DCOMM_Write( UP3D_CMD_76_X, sizeof(UP3D_CMD_76_X) ) != sizeof(UP3D_CMD_76_X)) ||
      (5 != UP3DCOMM_Read( resp, sizeof(resp) )) ||
      (6 != resp[4]) )
    return false;

  *var = le32toh( *((uint32_t*)resp) );
  return true;
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

  return le32toh( *((uint32_t*)resp) );
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

bool UP3D_GetPrinterInfo(TT_tagPrinterInfoHeader *pihdr, TT_tagPrinterInfoName *piname, TT_tagPrinterInfoData *pidata, TT_tagPrinterInfoSet pisets[8])
{
  static const uint8_t UP3D_CMD_52[] = { 0x52, 0x00 }; //GetPrinterInfo
  uint8_t resp[2048];
  if( (UP3DCOMM_Write( UP3D_CMD_52, sizeof(UP3D_CMD_52) ) != sizeof(UP3D_CMD_52)) )
    return false;
  
  if( (1 != UP3DCOMM_Read( resp, 1 )) || (7 != resp[0]) )
    return false;


  if( (sizeof(TT_tagPrinterInfoHeader) != UP3DCOMM_Read( (uint8_t*)pihdr,  sizeof(TT_tagPrinterInfoHeader) )) ||
      (sizeof(TT_tagPrinterInfoName  ) != UP3DCOMM_Read( (uint8_t*)piname, sizeof(TT_tagPrinterInfoName) )) ||
      (sizeof(TT_tagPrinterInfoData)   != UP3DCOMM_Read( (uint8_t*)pidata, sizeof(TT_tagPrinterInfoData) ))
    )
    return false;

  if( pidata->u32_NumSets>8 )
    return false;

  uint32_t s;
  for( s=0; s<pidata->u32_NumSets; s++ )
  {
    if( (16 != UP3DCOMM_Read( (uint8_t*)(&pisets[s])+ 0, 16 )) ||
        (60 != UP3DCOMM_Read( (uint8_t*)(&pisets[s])+16, 60 )) ||
        (60 != UP3DCOMM_Read( (uint8_t*)(&pisets[s])+76, 60 ))
      )
      return false;
  }

  if( (1 != UP3DCOMM_Read( resp, 1 )) || (6 != resp[0]) )
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

bool UP3D_GetPrinterStatus(TT_tagPrinterStatus *status)
{
  int32_t sysvar, sysvar2 = 0;
  if( UP3D_GetSystemVar( 0, &sysvar) )     //PARA_SYSTEM_STATUS
    status->SystemStatus = (uint8_t)sysvar;

  if( !UP3D_GetSystemVar( 0x10, &sysvar) ) //PARA_PRINT_STATUS
    return false;
  status->PrintStatus = (uint8_t)sysvar;

  if( UP3D_GetSystemVar( 0x0A, &sysvar) )  //PARA_REPORT_LAYER
    status->ReportLayer = htole16((uint16_t)sysvar);

  if( UP3D_GetSystemVar( 0x0B, &sysvar) )  //PARA_REPORT_HEIGHT
    status->ReportHeight = htole16((int16_t)floor( (*((float*)&sysvar))*10.0 ) );

  if( UP3D_GetSystemVar( 0x4D, &sysvar) && UP3D_GetSystemVar( 0x4C, &sysvar2) )  //PARA_REPORT_TIME_REMAIN PARA_REPORT_PERCENT 
    status->ReportTimeAndPercent = htole32(sysvar2 + (sysvar<<8));

  if( UP3D_GetSystemVar( 6, &sysvar) )     //NOZZLE1 TEMP
    status->Nozzle1Temp = htole16((int16_t)floor( (*((float*)&sysvar))*50.0 ));
  if( UP3D_GetSystemVar( 7, &sysvar) )     //NOZZLE2 TEMP
    status->Nozzle2Temp = htole16((int16_t)floor( (*((float*)&sysvar))*50.0 ));
  if( UP3D_GetSystemVar( 8, &sysvar) )     //BED TEMP
    status->BedTemp = htole16((int16_t)floor( (*((float*)&sysvar))*50.0 ));
  if( UP3D_GetSystemVar( 9, &sysvar) )     //ROOM TEMP
    status->RoomTemp = htole16((int16_t)floor( (*((float*)&sysvar))*50.0 ));

  status->HeaterStatus = 0;
  if( UP3D_GetSystemVar( 0x14, &sysvar) )     //HEATER NOZZLE1 STATUS
    status->HeaterStatus |= sysvar?1:0;
  if( UP3D_GetSystemVar( 0x15, &sysvar) )     //HEATER NOZZLE2 STATUS
    status->HeaterStatus |= sysvar?2:0;
  if( UP3D_GetSystemVar( 0x16, &sysvar) )     //HEATER BED STATUS
    status->HeaterStatus |= sysvar?4:0;

  UP3D_SetParameter( 0x94, 999 );             //SET_Z_PRECISION

  uint32_t v;
  for( v=0; v<4; v++ )
  {
    status->AxisStatus[v] = 0;

    if( UP3D_GetSystemVar( v+2, &sysvar) )    //MOTOR STATUS
      status->AxisStatus[v] |= sysvar & 7;

    if( UP3D_GetSystemVar( v+0x18, &sysvar) ) //HF?
      status->AxisStatus[v] |= sysvar?8:0;

    if( UP3D_GetSystemVar( v+0x26, &sysvar) ) //NL?
      status->AxisStatus[v] |= sysvar?16:0;

    if( UP3D_GetSystemVar( v+0x22, &sysvar) ) //ENDSTOP
      status->AxisStatus[v] |= sysvar?32:0;

    if( UP3D_GetSystemVar( v+0x1E, &sysvar) ) //ERROR
      status->AxisStatus[v] |= sysvar<<6;

    sysvar = UP3D_GetAxisPosition(v);
    float f = ((float)sysvar) / 854.0;        //FIXED VALUE FOR NOW
      status->AxisPosition[v] = htole32( *((uint32_t*)&f) );
  }

  UP3D_SetParameter( 0x94, 0 );             //SET_Z_PRECISION

  if( UP3D_GetSystemVar( 0x5E, &sysvar) )     //?
    status->Para_x5E = htole32(sysvar);
  if( UP3D_GetSystemVar( 0x5F, &sysvar) )     //?
    status->Para_x5F = htole32(sysvar);
  if( UP3D_GetSystemVar( 0x56, &sysvar) )     //?
    status->Para_x56 = htole32(sysvar);
  if( UP3D_GetSystemVar( 0x57, &sysvar) )     //?
    status->Para_x57 = htole32(sysvar);

  status->UnkMinusOne = htole64((int64_t)-1);
  return true;
}
