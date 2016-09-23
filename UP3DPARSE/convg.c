//UP program to gcode converter
//Author M.Stohn

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <math.h>

#define STEPS_X (854.0)
#define STEPS_Y (854.0)
#define STEPS_E (854.0)

#define ADD_X   (0.0)
#define ADD_Y   (120.0)
#define ADD_Z   (123.1)

typedef enum PCMD {
  PCMD_Stop         = 0x00000001,
  PCMD_2 = 2, //PAUSE 4/1 4/0
  PCMD_MoveF        = 0x00000003,
  PCMD_MoveL        = 0x00000004,
  PCMD_5 = 5, //DELAY
  PCMD_SetParameter = 0x00000006
} PCMD;

typedef enum PARA {
  PARA_REPORT_LAYER        = 0x0A,
  PARA_REPORT_HEIGHT       = 0x0B,

  PARA_x0C                 = 0x0C, //PAUSE (1)
  PARA_x10                 = 0x10, //PAUSE (3)
  PARA_x11                 = 0x11, //AFTER EVERY LAYER (1)
  PARA_x16                 = 0x16, //(0)AT END ==> RUN END PROGRAM

  PARA_NOZZLE1_TEMP        = 0x39,
  PARA_NOZZLE2_TEMP        = 0x3A,
  PARA_BED_TEMP            = 0x3B,

  PARA_REPORT_PERCENT      = 0x4C,
  PARA_REPORT_TIME_REMAIN  = 0x4D,
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
static int    _lastFeed = -1;
static bool   _Xchange, _Ychange, _Zchange, _Echange;
static int    _minFeed;
static int    _maxFeed;

void _dat_cmd_MoveF( float speed1, float pos1, float speed2, float pos2, bool isXY )
{
  if( isXY )
  {
    _Xchange = false;
    _Ychange = false;
    _Zchange = false;
    _Echange = false;
    _minFeed = 1000000;
    _maxFeed = 0;
  
    if( speed1!=0 ) 
    {
      _posX = pos1;
      if( _minFeed>fabs(speed1) ) _minFeed = fabs(speed1);
      if( _maxFeed<fabs(speed1) ) _maxFeed = fabs(speed1);
      _Xchange = true;
    }
    if( speed2!=0 ) 
    {
      _posY = pos2;
      if( _minFeed>fabs(speed2) ) _minFeed = fabs(speed2);
      if( _maxFeed<fabs(speed2) ) _maxFeed = fabs(speed2);
      _Ychange = true;
    }
  }
  else
  {
    if( speed1!=0 )
    {
      _posZ = pos1;
      if( _minFeed>fabs(speed1) ) _minFeed = fabs(speed1);
      if( _maxFeed<fabs(speed1) ) _maxFeed = fabs(speed1);
      _Zchange = true;
    }
    if( speed2!=0 ) 
    {
      if( speed2<0 )
        _posE = pos2;
      else
        _posE += pos2;
      
      if( _minFeed>fabs(speed2) ) _minFeed = fabs(speed2);
      if( _maxFeed<fabs(speed2) ) _maxFeed = fabs(speed2);
      _Echange = true;
    }
    
    if( _Xchange || _Ychange || _Zchange || _Echange )
    {
      printf("G1");
      if( _Xchange ) printf(" X%.4f", _posY);
      if( _Ychange ) printf(" Y%.4f", -_posX);
      if( _Zchange ) printf(" Z%.4f", ADD_Z+_posZ);
      if( _Echange ) printf(" E%.5f", _posE);
/*
      if( _lastFeed != _minFeed )
      {
        printf(" F%d", _minFeed);
        _lastFeed = _minFeed;
      }
*/
      if( _lastFeed != _maxFeed )
      {
        printf(" F%d", _maxFeed);
        _lastFeed = _maxFeed;
      }
      printf("\n");

//      if( _Echange ) printf("G92 E0\n");
    }
  }
}

void _dat_cmd_MoveL( uint16_t p1, uint16_t p2, int16_t p3, int16_t p4, int16_t p5, int16_t p6, int16_t p7, int16_t p8 )
{
  int32_t sx = floor((float)((p3*p1+p6*(p1-1)*p1/2))/512);
  int32_t sy = floor((float)((p4*p1+p7*(p1-1)*p1/2))/512);
  int32_t sa = floor((float)((p5*p1+p8*(p1-1)*p1/2))/512);
  double r1 = sx/STEPS_X;
  double r2 = sy/STEPS_Y;
  double r3 = sa/STEPS_E;

  _posX += r1;
  _posY += r2;
  _posE += r3;
 
  if( (r1!=0) || (r2!=0) || (r3!=0) )
  {
    printf("G1");
    printf(" X%.4f",_posY);
    printf(" Y%.4f",-_posX);
    printf(" E%.5f",_posE);

    if( p1>1 )
    {
      double t = ((double)p2*(double)p1)/50000000.0;
    
      double fx = fabs(r1/t);
      double fy = fabs(r2/t);
      double fa = fabs(r3/t);
    
      double f = fx;
      if( fy>f ) f = fy;
      if( fa>f ) f = fa;
    
      int32_t feed = f*60.0;
    
      if( _lastFeed != feed )
      {
        printf(" F%d", feed);
        _lastFeed = feed;
      }
    }  
    printf("\n");
  }
}


void _dat_cmd_SetParameter( int32_t param, int32_t value )
{
  printf( ";Set Parameter: " );
  switch( param )
  {
    case PARA_REPORT_LAYER:
      printf( "Report Layer: %"PRIi32, value );
    break;

    case PARA_REPORT_HEIGHT:
      printf( "Report Height: %.3f", *((float*)&value) );
    break;

    case PARA_NOZZLE1_TEMP:
      printf( "Nozzle 1: %"PRIi32" C", value );
    break;
    case  PARA_NOZZLE2_TEMP:
      printf( "Nozzle 2: %"PRIi32" C", value );
    break;
    case PARA_BED_TEMP:
      printf( "Bed: %"PRIi32" C", value );
    break;

    case PARA_REPORT_PERCENT:
      printf( "Report Percent Done: %"PRIi32"%%", value );
    break;
    case PARA_REPORT_TIME_REMAIN:
      printf( "Report Time Remaining: %"PRIi32" sec", value );
    break;
  
    default:
    break;
  }
  printf( "\n" );

  switch( param )
  {
    case PARA_NOZZLE1_TEMP:
      printf( "M104 S%"PRIi32" ;set extruder temp\n", value );
    break;
    case PARA_BED_TEMP:
      printf( "M140 S%"PRIi32" ;set bed temp\n", value );
    break;

    default:
    break;
  }
}

void _parse_dat_block( UPBLOCK* pBlock )
{
  static bool _cmd3_XY = true;

  switch( pBlock->pcmd )
  {
    case PCMD_Stop:
      printf( ";STOP\n");
      break;

    case PCMD_2: //PAUSE - USER INTERACTION 4/1 4/0
      printf( ";CMD_2: %08X / %08X\n", pBlock->pdat.longs.l1, pBlock->pdat.longs.l2 );
      break;
      
    case PCMD_MoveF:
      _dat_cmd_MoveF( pBlock->pdat.floats.f1, pBlock->pdat.floats.f2, pBlock->pdat.floats.f3, pBlock->pdat.floats.f4, _cmd3_XY );
      _cmd3_XY = !_cmd3_XY;
      break;
 
    case PCMD_MoveL:
      _dat_cmd_MoveL( pBlock->pdat.shorts.s1, pBlock->pdat.shorts.s2, pBlock->pdat.shorts.s3, pBlock->pdat.shorts.s4,
                      pBlock->pdat.shorts.s5, pBlock->pdat.shorts.s6, pBlock->pdat.shorts.s7, pBlock->pdat.shorts.s8 );
      break;

    case PCMD_5:
      printf( "G4 P%"PRIi32" ;delay %f sec\n", pBlock->pdat.longs.l1, (double)pBlock->pdat.longs.l1 / 1000.0);
      break;

    case PCMD_SetParameter:
      _dat_cmd_SetParameter( pBlock->pdat.longs.l1, pBlock->pdat.longs.l2 ); //l3 and l4 is junk
      break;
      
    default:
      printf( ";UNKNOWN CMD: %08X : %08X %08X %08X %08X\n", 
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

  printf( ";UP converted GCODE\n\n" );
  printf( "G21 ;set units to millimeter\n" );
  printf( "G90 ;absoulte coordinates\n" );
  printf( "G92 ;reset X,Y,Z\n" );
  printf( "M82 ;use absolute distances for extrusion\n" );
  printf( "G1 Z0;make gcode viewer happy --> layer 0 = Z0\n" );
  printf( "\n" );

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
