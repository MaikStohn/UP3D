//////////////////
//Author: M.Stohn/
//////////////////

#ifndef _UP3DDATA_H_
#define _UP3DDATA_H_

#include <stdint.h>
#include <stdbool.h>

static char UP3D_STR_MACHINE_STATE[][32] = {"System Error","Jogging","Running Program","Idle","Unknown Status","<UNK>"};

static char UP3D_STR_PROGRAM_STATE[][32] = {"Program Stop","Program Running","Program Pause","Program Have Errors","<UNK>"};

static char UP3D_STR_SYSTEM_STATE[][32]  = {"Unknown","System Ready","Running","Paused","Nozzle Error","Motion Error",
                                           "Print Finished","User Canceled","Nozzle To Hot","Nozzle To Cool","Platform To Hot",
                                           "DIP ERROR","SWEEP ERROR","SCAN ERROR","SCAN CALIB ERROR","System Power On",
                                           "SD Card Error","SD Card Write Error","SD Card Read Error","Save To SD Card",
                                           "JogCmd Error","Invalid Copyright","Over Usage Restriction","<UNK>"};

static char UP3D_STR_MOTOR_STATE[][32]   = {"STOP","JOG","PROG","ERRORSTOP","HOMEING","ERROR","<UNK>"};

static char UP3D_STR_AXIS_NAME[][2]      = {"X","Y","Z","A"};
static char UP3D_STR_AXIS_STATE[][32]    = {"No Error","+Limit Error","-Limit Error","<UNK>"};



typedef enum UP3D_PROG_CMD {
//UP3DPCMD_0 User Pause?
  UP3DPCMD_Stop         = 0x00000001,
  UP3DPCMD_SetState     = 0x00000002,
  UP3DPCMD_MoveF        = 0x00000003,
  UP3DPCMD_MoveL        = 0x00000004,
  UP3DPCMD_Pause        = 0x00000005,
  UP3DPCMD_SetParameter = 0x00000006,
  UP3DPCMD_WaitIfNot    = 0x00000007,
  UP3DPCMD_HomeAxis     = 0x00000008,
  UP3DPCMD_IfNotThenJmp = 0x00000009,
//UP3DPCMD_A Change Nozzle ?? (1x LONG 0x05/0x0C/0x0D)
  UP3DPCMD_AddToParam   = 0x0000000B,
} UP3D_PROG_CMD;

typedef enum UP3D_PROG_CMD_SETSTATE_STATE {
  UP3DPCMD_SetState_StatePower       = 0x00,
  UP3DPCMD_SetState_StateBeeper      = 0x04,
//UP3DPCMD_SetState_StateUnk_Tester  = 0x0A, //(TEST / FORMAT SD CARD?) 
} UP3D_PROG_CMD_SETSTATE;

typedef enum UP3D_PROG_CMD_SETSTATE_VALUE {
  UP3DPCMD_SetState_ValueOff         = 0x00,
  UP3DPCMD_SetState_ValueOn          = 0x01,
//UP3DPCMD_SetState_ValueUnk         = 0x02, //(TEST / FORMAT SD CARD?) 
} UP3D_PROG_CMD_SETSTATE_VALUE;

typedef enum UP3D_AXIS {
  UP3DAXIS_X = 0,
  UP3DAXIS_Y = 1,
  UP3DAXIS_Z = 2,
  UP3DAXIS_A = 3,
} UP3D_AXIS;

#pragma pack(1)  
typedef struct UP3D_BLK {
  UP3D_PROG_CMD pcmd;
  union UPBLOCKDAT1 {
    float f;
    struct UP3DBLKDATSHORTS1 { int16_t s1; int16_t s2; } s;
    int32_t l;
  } pdat1;
  union UPBLOCKDAT2 {
    float f;
    struct UP3DBLKDATSHORTS2 { int16_t s1; int16_t s2; } s;
    int32_t l;
  } pdat2;
  union UPBLOCKDAT3 {
    float f;
    struct UP3DBLKDATSHORTS3 { int16_t s1; int16_t s2; } s;
    int32_t l;
  } pdat3;
  union UPBLOCKDAT4 {
    float f;
    struct UP3DBLKDATSHORTS4 { int16_t s1; int16_t s2; } s;
    int32_t l;
  } pdat4;
} UP3D_BLK;
#pragma pack()

static const UP3D_BLK UP3D_PROG_BLK_Stop          = {.pcmd=UP3DPCMD_Stop};

static const UP3D_BLK UP3D_PROG_BLK_PowerOn       = {.pcmd=UP3DPCMD_SetState,.pdat1.l=UP3DPCMD_SetState_StatePower, .pdat2.l=UP3DPCMD_SetState_ValueOn};
static const UP3D_BLK UP3D_PROG_BLK_PowerOff      = {.pcmd=UP3DPCMD_SetState,.pdat1.l=UP3DPCMD_SetState_StatePower, .pdat2.l=UP3DPCMD_SetState_ValueOff};
static const UP3D_BLK UP3D_PROG_BLK_BeeperOn      = {.pcmd=UP3DPCMD_SetState,.pdat1.l=UP3DPCMD_SetState_StateBeeper,.pdat2.l=UP3DPCMD_SetState_ValueOn};
static const UP3D_BLK UP3D_PROG_BLK_BeeperOff     = {.pcmd=UP3DPCMD_SetState,.pdat1.l=UP3DPCMD_SetState_StateBeeper,.pdat2.l=UP3DPCMD_SetState_ValueOff};

static const UP3D_BLK UP3D_PROG_BLK_Pause100      = {.pcmd=UP3DPCMD_Pause,.pdat1.l=100};
static const UP3D_BLK UP3D_PROG_BLK_Pause500      = {.pcmd=UP3DPCMD_Pause,.pdat1.l=500};
static const UP3D_BLK UP3D_PROG_BLK_Pause1000     = {.pcmd=UP3DPCMD_Pause,.pdat1.l=1000};
static const UP3D_BLK UP3D_PROG_BLK_Pause5000     = {.pcmd=UP3DPCMD_Pause,.pdat1.l=5000};

static const UP3D_BLK UP3D_PROG_BLK_HomeAxisXFast = {.pcmd=UP3DPCMD_HomeAxis,.pdat1.l=UP3DAXIS_X,.pdat2.f=50.0,.pdat3.f=-4.0};
static const UP3D_BLK UP3D_PROG_BLK_HomeAxisX     = {.pcmd=UP3DPCMD_HomeAxis,.pdat1.l=UP3DAXIS_X,.pdat2.f=10.0,.pdat3.f=-2.0};
static const UP3D_BLK UP3D_PROG_BLK_HomeAxisYFast = {.pcmd=UP3DPCMD_HomeAxis,.pdat1.l=UP3DAXIS_Y,.pdat2.f=50.0,.pdat3.f= 4.0};
static const UP3D_BLK UP3D_PROG_BLK_HomeAxisY     = {.pcmd=UP3DPCMD_HomeAxis,.pdat1.l=UP3DAXIS_Y,.pdat2.f=10.0,.pdat3.f= 9.0};
static const UP3D_BLK UP3D_PROG_BLK_HomeAxisZFast = {.pcmd=UP3DPCMD_HomeAxis,.pdat1.l=UP3DAXIS_Z,.pdat2.f=50.0,.pdat3.f=-6.0};
static const UP3D_BLK UP3D_PROG_BLK_HomeAxisZ     = {.pcmd=UP3DPCMD_HomeAxis,.pdat1.l=UP3DAXIS_Z,.pdat2.f= 3.0,.pdat3.f=-2.0};


#endif //_UP3DDATA_H_
