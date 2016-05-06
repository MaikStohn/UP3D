/*
  up3ddata.h for UP3DTranscoder
  M. Stohn 2016

  This is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _UP3DDATA_H_
#define _UP3DDATA_H_

#include <stdint.h>
#include <stdbool.h>
#include "compat.h"

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
//UP3DPCMD_0 ?
  UP3DPCMD_Stop         = 0x00000001,
  UP3DPCMD_SetState     = 0x00000002,  
  UP3DPCMD_MoveF        = 0x00000003,  //JOGCMD       Speed,Pos,Speed,Pos
  UP3DPCMD_MoveL        = 0x00000004,  //MOVECMD      SegNum,SegPeriod,XSpeed,YSpeed,ASpeed,XAcc,YAcc,AAcc
  UP3DPCMD_Pause        = 0x00000005,  //WAITCMD      Var, Val, Mode
  UP3DPCMD_SetParameter = 0x00000006,  //SYSVARCMD    Var, Val
  UP3DPCMD_WaitIfNot    = 0x00000007,  
  UP3DPCMD_HomeAxis     = 0x00000008,  //HOMECMD      MotorID, HSpeed, HOffDis
  UP3DPCMD_IfNotThenJmp = 0x00000009,  //IFCMD        Var, Val, Mode, Jumpline
//UP3DPCMD_A Change Nozzle ?? (1x LONG 0x05/0x0C/0x0D)
  UP3DPCMD_AddToParam   = 0x0000000B,  //SYSVARADDCMD Var,Val

  //IOCMD        Port,Status
  //COPYRIGHTCMD DwellTime,SerialID,LicenseCount(8),bCountDown(8),CmdVerify1(8),CmdVerify2(8)
  //MATERIALCMD  DwellTime,UsedMaterial,CartridgeID,Type(8),CmdVerify1(8),CmdVerify2(8)
  //FLASHVARCMD  type(8),var(8),res1(8),res2(8),val,jumpline
  
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

typedef enum UP3D_PRINTSTATUS {
  UP3DPRINTSTATUS_NOTPRINTING = 0,
  UP3DPRINTSTATUS_PREPARED = 1,
  UP3DPRINTSTATUS_PRINTING = 2,
  UP3DPRINTSTATUS_PAUSED = 3,
} UP3D_PRINTSTATUS;

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

typedef struct TT_tagPrinterInfoHeader {
  float    flt_unk1;              //1.0                       (info version)
  uint32_t u32_printerid;         //00002777 => UP Mini(M)    (system type)
  uint32_t u32_unk3;              //1                         (nozzle num)
  uint32_t u32_unk4;              //102                       (hardware ver)
  float    f_rom_version;         //6.1                       (software ver)
  uint32_t u32_printerserial;     //208460                    (system sn)
  uint32_t u32_unk7;              //100                       (nozzle type)
} TT_tagPrinterInfoHeader;

typedef struct TT_tagPrinterInfoName {
  char printer_name[63];          //UpMini                    (system type (char))
} TT_tagPrinterInfoName;

typedef struct TT_tagPrinterInfoData {
  float    f_max_x;               //-120.0  maxX and direction (minX)
  float    flt_unk2;              //0.0                        (maxX)
  float    flt_unk3;              //0.0                        (minY)
  float    f_max_y;               //120.0   maxY and direction (maxY)
  float    f_max_z;               //130.0   maxZ and direction (maxZ)
  float    f_steps_mm_x;          //854.0   X steps/mm         (xscale)
  float    f_steps_mm_y;          //854.0   Y steps/mm         (yscale)
  float    f_steps_mm_z;          //854.0   Z steps/mm         (zscale)
  float    f_unknown_a;           //40.0    A? some factor?    (ascale)             BUT in reality is 854.0 steps/mm
  float    flt_unk10;             //0.0                        (xyscale)
  float    flt_unk11;             //0.0                        (zxscale)
  float    flt_unk12;             //0.0                        (zyscale)
  float    flt_unk13;             //1.0                        (paramver)
  uint32_t u32_NumSets;           //4                          (paramsetnum)
} TT_tagPrinterInfoData;

typedef struct TT_tagPrinterInfoSet {
  char  set_name[16];
  float nozzle_diameter;
  float layer_thickness;
  float scan_width;
  float scan_times;
  float hatch_width;
  float hatch_space;
  float hatch_layer;
  float support_width;
  float support_space;
  float support_layer;
  float scan_speed;
  float hatch_speed;
  float support_speed;
  float jump_speed;
  float scan_scale;
  float hatch_scale;
  float support_scale;
  float feed_scale;
  float other_param_1;
  float other_param_2;
  float other_param_3;
  float other_param_4;
  float other_param_5;
  float other_param_6;
  float unused_1;
  float unused_2;
  float unused_3;
  float unused_4;
  float unused_5;
  float unused_6;
} TT_tagPrinterInfoSet;

typedef struct TT_tagPrinterStatus {
  uint8_t  SystemStatus;
  uint8_t  PrintStatus;
  uint16_t ReportLayer;
  int16_t  ReportHeight;
  uint32_t ReportTimeAndPercent;
  int16_t  Nozzle1Temp;
  int16_t  Nozzle2Temp;
  int16_t  BedTemp;
  int16_t  RoomTemp;
  uint8_t  HeaterStatus;
  uint8_t  AxisStatus[4];
  float    AxisPosition[4];
  uint32_t Para_x5E; //outport
  uint32_t Para_x5F; //inport
  uint32_t Para_x56; //outport slave
  uint32_t Para_x57; //inport  slave
  int64_t  UnkMinusOne;  //2x32 reserved1/reserved2
} TT_tagPrinterStatus;

typedef struct UP3D_WIFI_HDR {
  uint16_t u16Marker;   //0xF0F0
  uint8_t  u8Reserved0; //0x00
  uint16_t u16Cmd;      //0x1001 (scan) 0x1002 (connect)   0x4001 (response1) 0x4002 (response2)
  uint16_t u16DataLen;
  uint16_t u16Reserved1;
} UP3D_WIFI_HDR;

typedef struct TT_tagEchoInfo {
  uint32_t serNum1;
  uint32_t serNum2;
  uint32_t systemType;
  uint8_t  accessCtrl;
  uint8_t  workState;
  uint8_t  haveHost;
  uint8_t  reserved;
  struct sockaddr_in udpAddr;
  char hostName[32];
  char printerName[32];
  char printerType[32];
} TT_tagEchoInfo;

#pragma pack()

typedef enum PARA {

//GET_SYSTEM_STATUS 0x00 ==> 0 = System Error, 1 = Jogging, 2 = Running Program, 3 = Idle, 4 = Unknown Status

  PARA_x01                 = 0x01, //program statusx: 0 = Program Stop, 1 = Program Running, 2 = Program Pause, 3 = Program Have Errors

  PARA_x02                 = 0x02, //MOTOR_1: 0=STOP,1=JOG,2=PROG,3=ERRORSTOP,4=HOMEING,5=ERROR (always 0)
  PARA_x03                 = 0x03, //MOTOR_2: 0=STOP,1=JOG,2=PROG,3=ERRORSTOP,4=HOMEING,5=ERROR (always 0)
  PARA_x04                 = 0x04, //MOTOR_3: 0=STOP,1=JOG,2=PROG,3=ERRORSTOP,4=HOMEING,5=ERROR (always 0)
  PARA_x05                 = 0x05, //MOTOR_4: 0=STOP,1=JOG,2=PROG,3=ERRORSTOP,4=HOMEING,5=ERROR (always 0)

//GET_NOZZLE1_TEMP 0x06 (float)
//GET_NOZZLE2_TEMP 0x07 (float)
//GET_BED_TEMP     0x08 (float)
//GET_TEMP4_TEMP   0x09 (float)

  PARA_REPORT_LAYER        = 0x0A, //layer number 1 ..
  PARA_REPORT_HEIGHT       = 0x0B, //height 0.01 ..

  PARA_PAUSE_PROGRAM       = 0x0C, //pause program
  PARA_x0D                 = 0x0D, //stop  program
  PARA_x0E                 = 0x0E, //init  program
  PARA_x0F                 = 0x0F, //SYSTEM INTERNAL STATUS: 0=NO_ERROR, 1=STOP_AT_LIMIT, 2=PROGRAM_STOP, 3=OVER_TIME, 4=UNKNOWN_CMD, 5=SINGLE_JOG_CMD, 6=WAIT_PROG_CMD
  
  PARA_PRINT_STATUS        = 0x10, //STATUS  0=PRINTER ERROR/NOT PRINTING, 1=PREAPRED FOR PRINTING, 2=PRINTING, 3=PAUSED, ...
  PARA_x11                 = 0x11, // 0/1 HAVE SUPPORT / NO SUPPORT ??AFTER EVERY LAYER

  PARA_TEMP_REACHED_N1     = 0x12, // 0/1 N1:  NOT HOT, HOT ENOUGH
  PARA_TEMP_REACHED_N2     = 0x13, // 0/1 N2:  NOT HOT, HOT ENOUGH

  PARA_HEATER_NOZZLE1_ON   = 0x14, //1=ON, >1 n=*2 seconds countdown, 0=OFF
  PARA_HEATER_NOZZLE2_ON   = 0x15, //1=ON, >1 n=*2 seconds countdown, 0=OFF
  PARA_HEATER_BED_ON       = 0x16, //1=ON, >1 n=*2 seconds countdown, 0=OFF

  PARA_x17                 = 0x17, //NOZZLE_OPEN: 0=No Nozzle Open, 1=Nozzle 1 Open, 2=Nozzle 2 Open, 3=2 Nozzles Open

  PARA_x18                 = 0x18, //HF1: %d ?
  PARA_x19                 = 0x19, //HF2: %d ?
  PARA_x1A                 = 0x1A, //HF3: %d ?
  PARA_x1B                 = 0x1B, //HF4: %d ?

  PARA_x1C                 = 0x1C, //PRINTING: 0=Not Printing, 1=Printing

  PARA_x1D                 = 0x1D, //Feed Error Length: %d
  
  PARA_x1E                 = 0x1E, //X: 0=X No Error,1=X+ Limit Error, 2=X- Limit Error
  PARA_x1F                 = 0x1F, //Y: 0=Y No Error,1=Y+ Limit Error, 2=Y- Limit Error
  PARA_x20                 = 0x20, //Z: 0=Z No Error,1=Z+ Limit Error, 2=Z- Limit Error
  PARA_x21                 = 0x21, //A: 0=A No Error,1=A+ Limit Error, 2=A- Limit Error


  PARA_GET_ENDSTOP_Y       = 0x22, //PL1%d (PositiveLimit1)
//0x23                             //PL2%d (PositiveLimit2)
  PARA_GET_ENDSTOP_Z       = 0x24, //PL3%d (PositiveLimit3)
//0x25                             //PL4%d (PositiveLimit4)
//0x26                             //NL1%d (NegativeLimit1)
  PARA_GET_ENDSTOP_X       = 0x27, //NL2%d (NegativeLimit2)
//0x28                             //NL3%d (NegativeLimit3)
//0x29                             //NL4%d (NegativeLimit4)

//0x2A  5000 | /1000  ==> 5.0      //Change Nozzle Time: %d
//0x2B  193  | *0.001 ==> 0.193    //Jump Time: %d
//0x2C                             //Using Nozzle: %d

//0x2D                             //Target Temp 1: %f  (automatic taken after set temp)
//0x2E                             //Target Temp 2: %f  (automatic taken after set temp)
//0x2F                             //Target Temp 3: %f  (automatic taken after set temp)
//0x30                             //Target Temp 4: %f  (automatic taken after set temp)

//0x31 ?? check door %d
  PARA_LIGHT               = 0x31, //light automatic: 0= turn off after some time, 1=slow go on and go off after some time

//0x32 ?? 0/1 ?? check 24V power %d

//0x33 ?? check power key %d
//0x34 ?? check light key %d

//0x35 ??  0 ?? check work room fan

//0x36 ??  103 | *0.001 ==> 0.103  //feedback length

//0x37 ??  save to sd card
//0x38 ??  read from sd card

  PARA_NOZZLE1_TEMP        = 0x39, //%d
  PARA_NOZZLE2_TEMP        = 0x3A, //%d
  PARA_BED_TEMP            = 0x3B, //%d
//0x3C  TEMP4 TEMP &d

//0x3E 334/330/306 | *0.01 = 3.34 / 3.30 / 3.06 ==> VERSION ==> same answer as USB-CMD 01 00

//0x41 ?? 267
//0x42 ?? 242
//0x43 ?? 104
//0x46 ?? 13 
//0x49 ?? 80

//0x4B ?? (default: 0x1312D00 = 20000000 / 50000000) ? timer base for stepper calculation MHZ of CPU in printer ?

  PARA_REPORT_PERCENT      = 0x4C,
  PARA_REPORT_TIME_REMAIN  = 0x4D,
  
//0x54: UP: "SupportNozzleUpDown" (0x03E8B258 / 0x0000B258)


//0x82 ?? FF
//0x83 ?? FF

//? GET POSITION 0x8C

  PARA_RED_BLUE_BLINK      = 0x8E, // RED/BLUE led -1=blue, 0=red, 1=purple, 2-... = blink speed

//0x94: UP:"SetZPrecision" (0/999)
// ACCURACY FOR REPORTING AXIS POSITION 999 = best 
//0x94 ?? 0x03e7 / 0x63 ==> 999, 99 ... 0

  //0xB1 FRONT BUTTON STATE  0=DEPRESSED / !0=PRESSED

//0xBC light countdown timer

  //0xC9, used as counter

} PARA;

void UP3D_PROG_BLK_Stop( UP3D_BLK *pupblk );
void UP3D_PROG_BLK_Power( UP3D_BLK *pupblk, bool on );
void UP3D_PROG_BLK_Beeper( UP3D_BLK *pupblk, bool on );
void UP3D_PROG_BLK_Pause( UP3D_BLK *pupblk, uint32_t msec );
void UP3D_PROG_BLK_SetParameter( UP3D_BLK *pupblk, uint8_t parameter, int32_t value );
void UP3D_PROG_BLK_Home( UP3D_BLK pupblks[2], UP3D_AXIS axis, float direction, float offset, float speed );
void UP3D_PROG_BLK_MoveF( UP3D_BLK pupblks[2], float speedX, float posX, float speedY, float posY, float speedZ, float posZ, float speedA, float posA );
void UP3D_PROG_BLK_MoveL( UP3D_BLK *pupblk, uint16_t p1, uint16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6, int16_t p7, int16_t p8);
void UP3D_PROG_BLK_WaitIfNot( UP3D_BLK *pupblk, uint8_t parameter, int32_t value, char compchar );

#endif //_UP3DDATA_H_
