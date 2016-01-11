//////////////////
//Author: M.Stohn/
//////////////////

#ifndef _UP3D_H_
#define _UP3D_H_

#include "up3ddata.h"

#include <stdint.h>
#include <stdbool.h>

bool     UP3D_Open();
void     UP3D_Close();

bool     UP3D_IsPrinterResponsive();

uint32_t UP3D_GetParameter(uint8_t param);
bool     UP3D_SetParameter(uint8_t param, uint32_t value);

bool     UP3D_BeginWrite();
bool     UP3D_SetProgramID( uint8_t progID, bool enableWrite );
bool     UP3D_InsertRomProgram( uint8_t prog );
uint32_t UP3D_GetFreeBlocks();
bool     UP3D_WriteBlocks( const UP3D_BLK *data, uint8_t blocks );
bool     UP3D_WriteBlock( const UP3D_BLK *data );
bool     UP3D_Execute();

int32_t  UP3D_GetMachineState();
int32_t  UP3D_GetProgramState();
int32_t  UP3D_GetSystemState();
int32_t  UP3D_GetMotorState(int motor);
uint32_t UP3D_GetPositiveLimitState(int motor);
uint32_t UP3D_GetNegativeLimitState(int motor);
int32_t  UP3D_GetLayer();
float    UP3D_GetHeight();
int32_t  UP3D_GetPercent();
int32_t  UP3D_GetTimeRemaining();
int32_t  UP3D_GetPrintState();
int32_t  UP3D_GetHeaterTempReached(int heater);
int32_t  UP3D_GetHeaterState(int heater);
int32_t  UP3D_GetHeaterSetTemp(int heater);
float    UP3D_GetHeaterTemp(int heater);
int32_t  UP3D_GetHeaterTargetTemp(int heater);
int32_t  UP3D_GetHF(int heater);
int32_t  UP3D_GetSupportState();
int32_t  UP3D_GetFeedErrorLength();
float    UP3D_GetFeedBackLength();
int32_t  UP3D_GetAxisState(int axis);
float    UP3D_GetChangeNozzleTime();
float    UP3D_GetJumpTime();
int32_t  UP3D_GetUsedNozzle();
int32_t  UP3D_GetCheckDoor();
int32_t  UP3D_GetCheck24V();
int32_t  UP3D_GetCheckPowerKey();
int32_t  UP3D_GetCheckLightKey();
int32_t  UP3D_GetCheckWorkRoomFan();
int32_t  UP3D_GetReadFromSD();
int32_t  UP3D_GetWriteToSD();

int32_t  UP3D_GetAxisPosition(int axis);

bool     UP3D_SendRandom(); //not used from MAC, no change in printer status...

///////
void     UP3D_CreateMoveF( UP3D_BLK upblks[2], float speedX, float posX, float speedY, float posY, float speedZ, float posZ, float speedA, float posA );
void     UP3D_CreateMoveL_( UP3D_BLK *pupblk, short p1,  short p2,  short p3,  short p4,  short p5,  short p6,  short p7,  short p8);
void     UP3D_CreateMoveL_A( UP3D_BLK *pupblk, int32_t p1,  short p2,  float XA, float YA, float AA, float XB, float YB, float AB);


#endif //_UP3D_H_
