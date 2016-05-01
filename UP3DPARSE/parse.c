#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <math.h>

#define STEPS_X (854.0)
#define STEPS_Y (854.0)
#define STEPS_E (854.0)

typedef enum PCMD {
//PCMD0 ?
  PCMD_Stop         = 0x00000001,
  PCMD_SetState     = 0x00000002,
  PCMD_MoveF        = 0x00000003,
  PCMD_MoveL        = 0x00000004,
  PCMD_Pause        = 0x00000005,
  PCMD_SetParameter = 0x00000006,
  PCMD_WaitIfNot    = 0x00000007,
  PCMD_HomeAxis     = 0x00000008,
  PCMD_IfNotThenJmp = 0x00000009,
//PCMDA Change Nozzle ?? (1x LONG 0x05/0x0C/0x0D)
  PCMD_AddToParam   = 0x0000000B,
} PCMD;

typedef enum STATE {
  STATE_MACHINE            = 0x00,
  STATE_BEEPER             = 0x04,
  STATE_UNK_TESTER         = 0x0A, //??? =2   (TEST / FORMAT SD CARD?) 
} STATE;


/*
//status from 0x10
 0xFFFFFFFF // disconnect
  0      // unknown error
  1      // system ready
  2      // running
  3      // paused
  4      // nozzle error
  5      // motion error
  6      // print finished
  7      // user canceled
  8      // nozzle to hot
  9      // nozzle to cool
 10      // platform to hot
 11      // DIP ERROR
 12      // SWEEP ERROR
 13      // SCAN ERROR
 14      // SCAN CALIB ERROR
 15      // System Power On
 16      // sd card error
 17      // sd card write error
 18      // sd card read error
 19      // save to sd card
 20      // JogCmd Error
 21      // Invalid Copyright
 22      // Over Usage Restriction
*/

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

  PARA_x0C                 = 0x0C, //pause program
  PARA_x0D                 = 0x0D, //stop  program
  PARA_x0E                 = 0x0E, //init  program
  PARA_x0F                 = 0x0F, //SYSTEM INTERNAL STATUS: 0=NO_ERROR, 1=STOP_AT_LIMIT, 2=PROGRAM_STOP, 3=OVER_TIME, 4=UNKNOWN_CMD, 5=SINGLE_JOG_CMD, 6=WAIT_PROG_CMD
  
  PARA_x10                 = 0x10, //STATUS  0=PRINTER ERROR, 1=PREAPRED FOR PRINTING, 2=PRINTING, 3=PAUSED, ...
  PARA_x11                 = 0x11, // 0/1 HAVE SUPPORT / NO SUPPORT ??AFTER EVERY LAYER

  PARA_GET_TEMP_REACHED_N1 = 0x12, // 0/1 N1:  NOT HOT, HOT ENOUGH
  PARA_GET_TEMP_REACHED_N2 = 0x13, // 0/1 N2:  NOT HOT, HOT ENOUGH

  PARA_HEATER_NOZZLE1      = 0x14, //1=ON, >1 n=*2 seconds countdown, 0=OFF
  PARA_HEATER_NOZZLE2      = 0x15, //1=ON, >1 n=*2 seconds countdown, 0=OFF
  PARA_HEATER_BED_TIMER    = 0x16, //1=ON, >1 n=*2 seconds countdown, 0=OFF

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

#pragma pack(1)  
typedef struct UPBLOCK {
  uint32_t pcmd;
  union UPBLOCKDAT {
    struct UPBLOCKDATFLOATS { float f1; float f2; float f3; float f4; } floats;
    struct UPBLOCKDATSHORTS { int16_t s1; int16_t s2; int16_t s3; int16_t s4; int16_t s5; int16_t s6; int16_t s7; int16_t s8; } shorts;
    struct UPBLOCKDATLONGS  { int32_t l1; int32_t l2; int32_t l3; int32_t l4; } longs;
  } pdat;
} UPBLOCK;
#pragma pack()

static double _posX, _posY, _posZ, _posE;
static double _speedX = 0;
static double _speedY = 0;
static double _speedE = 0;

void _dat_cmd_MoveF( float speed1, float pos1, float speed2, float pos2, bool isXY )
{
  if( isXY )
  {
    printf( "Move-F:" );
    if( speed1!=0 )
    {
      printf( " X:%.4f(%.4f)", pos1, speed1 );
      _posX = pos1;
    }
    if( speed2!=0 )
    {
      printf( " Y:%.4f(%.4f)", pos2, speed2 );
      _posY = pos2;
    }
    
    _speedX = 0;
    _speedY = 0;
  }
  else
  {
    if( speed1!=0 )
    {
      printf( " Z:%.4f(%.4f)", pos1, speed1 );
      _posZ = pos1;
    }
    if( speed2!=0 ) 
    {
      printf( " E:%.4f(%.4f)", pos2, speed2 );
      _posE = pos2;
    }
    printf( "\n" );
    
    _speedE = 0;
  }
}

void _dat_cmd_MoveL( int16_t p1, int16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6, int16_t p7, int16_t p8 )
{
/*
  int32_t v10 = (int32_t)0xFFFFFFF / (int32_t)p1;  //compensate rounding errors

  int32_t v13 = (int32_t)p1*((int32_t)p1 - 1);

  //s = (t^2 * a) / 2        (s travaled during acceleration / decelleration)
  //  = (p1^2 * p6) / 2
  //  = ((v13 * p6) / 2) ---------------------------------vvvvvvvvvvvvvvvvvvvvv   vvv----vvvvvvvvvvvvvv---- compensate rounding errors

  double r1 = (  (( ((int32_t)p3 + v10) * (int32_t)p1 ) + ((v13*(int32_t)p6)/2) - 511 - (int32_t)p1*v10)   /512.0)/STEPS_X;
  double r2 = (  (( ((int32_t)p4 + v10) * (int32_t)p1 ) + ((v13*(int32_t)p7)/2) - 511 - (int32_t)p1*v10)   /512.0)/STEPS_Y;
  double r3 = (  (( ((int32_t)p5 + v10) * (int32_t)p1 ) + ((v13*(int32_t)p8)/2) - 511 - (int32_t)p1*v10)   /512.0)/STEPS_E;


  //s = p3 * p1 ----^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
  //s = v * t        (s travaled during linear speed)

  //512 = divider/factor to compress/expand to/from fixed point

  double r4 = p6/512.0;
  double r5 = p7/512.0;
  double r6 = p8/512.0;
#if 0
  printf( "Move-L: %d,%d,%d,%d,%d,%d,%d,%d : %d  X:%.3f Y:%.3f E:%.3f  %f\t%f\t%f\n", p1,p2,p3,p4,p5,p6,p7,p8,p2,r1,r2,r3,r4,r5,r6 );
#endif

  _posX += r1;
  _posY += r2;
  _posE += r3;
  
  _speedX += r4*p1;
  _speedY += r5*p1;
  _speedE += r6*p1;

*/

//#if 0
  printf( "Move-L: %d,%d,%d,%d,%d,%d,%d,%d\n", p1,p2,p3,p4,p5,p6,p7,p8 );
//#endif

//==> ??? P2 ???

/*
  int32_t t  = p1;
  int32_t vX = p3;
  int32_t vY = p4;
  int32_t vA = p5;
  int32_t aX = p6;
  int32_t aY = p7;
  int32_t aA = p8;

  double sX = ( ( (aX*t*(t-1))/2 + vX*t - 511) / 512.0 ) /STEPS_X;
  double sY = ( ( (aY*t*(t-1))/2 + vY*t - 511) / 512.0 ) /STEPS_Y;
  double sA = ( ( (aA*t*(t-1))/2 + vA*t - 511) / 512.0 ) /STEPS_E;

  _posX += sX;
  _posY += sY;
  _posE += sA;
  
  _speedX = (vX + aX * t)/512.0;
  _speedY = (vY + aY * t)/512.0;
  _speedE = (vA + aA * t)/512.0;
*/

  int32_t sx = floor((float)((p3*p1+p6*p1*p1/2))/512);
  int32_t sy = floor((float)((p4*p1+p7*p1*p1/2))/512);
  int32_t sa = floor((float)((p5*p1+p8*p1*p1/2))/512);
  double r1 = sx/STEPS_X;
  double r2 = sy/STEPS_Y;
  double r3 = sa/STEPS_E;

  _posX += r1;
  _posY += r2;
  _posE += r3;

  _speedX = (p3 + p6 * p1)/512.0;
  _speedY = (p4 + p7 * p1)/512.0;
  _speedE = (p5 + p8 * p1)/512.0;

  printf( "------: X:%.4f(%.4f) Y:%.4f(%.4f) E:%.4f(%.4f) ?:%d\n", _posX, _speedX, _posY, _speedY, _posE, _speedE, p2 );
}


void _dat_cmd_SetParameter( int32_t param, int32_t value )
{
  printf( "Set Parameter: " );
  switch( param )
  {
    case PARA_REPORT_LAYER:
      printf( "Report Layer: %"PRIi32, value );
    break;

    case PARA_REPORT_HEIGHT:
      printf( "Report Height: %.3f", *((float*)&value) );
    break;

    case PARA_NOZZLE1_TEMP:
      printf( "Nozzle 1: %"PRIi32"°C", value );
    break;
    case  PARA_NOZZLE2_TEMP:
      printf( "Nozzle 2: %"PRIi32"°C", value );
    break;
    case PARA_BED_TEMP:
      printf( "Bed: %"PRIi32"°C", value );
    break;

    case PARA_REPORT_PERCENT:
      printf( "Report Percent Done: %"PRIi32"%%", value );
    break;
    case PARA_REPORT_TIME_REMAIN:
      printf( "Report Time Remaining: %"PRIi32" sec", value );
    break;

    case PARA_RED_BLUE_BLINK:
      printf( "Status LED: " );
      switch( value )
      {
        case -1: printf("BLUE");break;
        case  0: printf("RED");break;
        case  1: printf("PURPLE");break;
        default: printf("BLINK Speed: (%d)",value);break;
      }
    break;

    case PARA_HEATER_NOZZLE1:
      printf( "Heater Nozzle1 %s", value?"ON":"OFF" );
    break;

    case PARA_HEATER_NOZZLE2:
      printf( "Heater Nozzle2 %s", value?"ON":"OFF" );
    break;

    case PARA_HEATER_BED_TIMER:
      if( 0 != value )
        printf( "Heater Bed for %"PRIi32" sec", value*2 );
      else
        printf( "Heater Bed OFF" );
    break;

    case PARA_LIGHT:
      printf( "Light: %s", value?"ON":"OFF after 1 minute" );
    break;

    default:
      printf("UNKNOWN(%X) ==> %08X / %"PRIi32, param, value, value );
    break;
  }
  printf( "\n" );
}

void _dat_cmd_WaitIfNot( uint32_t param, int32_t value, uint8_t condition )
{
  printf( "Wait If Not $%X %c %d\n", param, condition, value );
}

void _dat_cmd_SetState( uint32_t state, uint32_t value )
{
  printf( "Set State: " );
  switch( state )
  {
    case STATE_MACHINE:
      printf( "Machine = %s", value?"On":"Off" );
    break;
    case STATE_BEEPER:
      printf( "Beeper = %s", value?"On":"Off" );
    break;
    
    default:
      printf("UNKNOWN(%X) ==> %08X / %"PRIi32, state, value, value );
    break;
  }
  printf( "\n" );
}

void _dat_cmd_HomeAxis( uint32_t axis, float f1, float f2 )
{
  static const char axisname[] = {'Y','X','Z'};
  printf( "Home-Axis: %c speed:%.3f direction:%.3f)\n", axisname[axis], f1, f2 );
}

void _dat_cmd_IfNotThenJmp( uint32_t param, int32_t value, uint8_t condition, int32_t rel_target )
{
  printf( "If Not $%X %c %d Then Goto %d\n", param, condition, value, rel_target );
}

void _dat_cmd_AddToParam( uint32_t param, int32_t value )
{
  printf( "Change Param: $%X += %d\n", param, value );
}

void _parse_dat_block( UPBLOCK* pBlock )
{
  static bool _cmd3_XY = true;

  switch( pBlock->pcmd )
  {
    case PCMD_Stop:
      printf( "STOP\n");
      break;

    case PCMD_SetState:
      _dat_cmd_SetState( pBlock->pdat.longs.l1, pBlock->pdat.longs.l2 );
      break;

    case PCMD_MoveF:
      _dat_cmd_MoveF( pBlock->pdat.floats.f1, pBlock->pdat.floats.f2, pBlock->pdat.floats.f3, pBlock->pdat.floats.f4, _cmd3_XY );
      _cmd3_XY = !_cmd3_XY;
      break;
 
    case PCMD_MoveL:
      _dat_cmd_MoveL( pBlock->pdat.shorts.s1, pBlock->pdat.shorts.s2, pBlock->pdat.shorts.s3, pBlock->pdat.shorts.s4,
                      pBlock->pdat.shorts.s5, pBlock->pdat.shorts.s6, pBlock->pdat.shorts.s7, pBlock->pdat.shorts.s8 );
      break;

    case PCMD_Pause:
      printf( "Pause: %f sec\n", (double)pBlock->pdat.longs.l1 / 1000.0 );
      break;

    case PCMD_SetParameter:
      _dat_cmd_SetParameter( pBlock->pdat.longs.l1, pBlock->pdat.longs.l2 ); //l3 and l4 is junk
      break;

    case PCMD_WaitIfNot:
      _dat_cmd_WaitIfNot( pBlock->pdat.longs.l1, pBlock->pdat.longs.l2, pBlock->pdat.longs.l3 );
      break;

    case PCMD_HomeAxis:
      _dat_cmd_HomeAxis( pBlock->pdat.longs.l1, pBlock->pdat.floats.f2, pBlock->pdat.floats.f3 );
      break;


    case PCMD_IfNotThenJmp:
      _dat_cmd_IfNotThenJmp( pBlock->pdat.longs.l1, pBlock->pdat.longs.l2, pBlock->pdat.longs.l3, pBlock->pdat.longs.l4 );
      break;

    case PCMD_AddToParam:
      _dat_cmd_AddToParam( pBlock->pdat.longs.l1, pBlock->pdat.longs.l2 );
      break;

    default:
      printf( "UNKNOWN CMD: %08X : %08X %08X %08X %08X\n", 
              pBlock->pcmd,
              pBlock->pdat.longs.l1, pBlock->pdat.longs.l2, pBlock->pdat.longs.l3, pBlock->pdat.longs.l4
            );

    break;
  }
}

int main(int argc, char *argv[])
{
  if( argc != 2 )
  {
    printf("Usage: %s program.dat\n\n", argv[0]);
    return 0;
  }

  FILE* fdat = fopen( argv[1], "rb" );
  if( !fdat )
    return -1;

  for( ;; )
  {
    UPBLOCK block;
    
    if( sizeof(UPBLOCK) != fread( &block, 1, sizeof(UPBLOCK), fdat ) )
      break;
      
    _parse_dat_block( &block );
  }

  fclose( fdat );

  return 0; 
}
