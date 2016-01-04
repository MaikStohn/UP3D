#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

typedef enum PCMD {
  PCMD_Stop         = 0x00000001,
  PCMD_SetState     = 0x00000002,
  PCMD_MoveF        = 0x00000003,
  PCMD_MoveL        = 0x00000004,
  PCMD_Pause        = 0x00000005,
  PCMD_SetParameter = 0x00000006,
  PCMD_WaitIfNot    = 0x00000007,
  PCMD_HomeAxis     = 0x00000008,
  PCMD_IfNotThenJmp = 0x00000009,
  PCMD_AddToParam   = 0x0000000B,
} PCMD;

typedef enum STATE {
  STATE_MACHINE            = 0x00,
  STATE_BEEPER             = 0x04,
} STATE;

typedef enum AXIS {
  AXIS_Y                   = 0,
  AXIS_X                   = 1,
  AXIS_Z                   = 2,
} AXIS;

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

void _get_block_stop(UPBLOCK *pBlock)
{
  memset( pBlock, 0, sizeof(UPBLOCK) );
  pBlock->pcmd = PCMD_Stop;
}

void _get_block_machine_state(UPBLOCK *pBlock, STATE state, int32_t value)
{
  memset( pBlock, 0, sizeof(UPBLOCK) );
  pBlock->pcmd = PCMD_SetState;
  pBlock->pdat.longs.l1 = state;
  pBlock->pdat.longs.l2 = value;
}

void _get_block_set_parameter(UPBLOCK *pBlock, int32_t param, int32_t value)
{
  memset( pBlock, 0, sizeof(UPBLOCK) );
  pBlock->pcmd = PCMD_SetParameter;
  pBlock->pdat.longs.l1 = param;
  pBlock->pdat.longs.l2 = value;
}

void _get_block_pause(UPBLOCK *pBlock, float value)
{
  memset( pBlock, 0, sizeof(UPBLOCK) );
  pBlock->pcmd = PCMD_Pause;
  pBlock->pdat.longs.l1 = (int32_t)(value*1000);
}

void _get_block_home_axis(UPBLOCK *pBlock, AXIS axis, float speed, float offsetdirection)
{
  memset( pBlock, 0, sizeof(UPBLOCK) );
  pBlock->pcmd = PCMD_HomeAxis;
  pBlock->pdat.longs.l1 = axis;
  pBlock->pdat.floats.f2 = speed;
  pBlock->pdat.floats.f3 = offsetdirection;
}

void _get_block_move_f(UPBLOCK *pBlock, float speed1, float pos1, float speed2, float pos2)
{
  pBlock->pcmd = PCMD_MoveF;
  pBlock->pdat.floats.f1 = speed1;
  pBlock->pdat.floats.f2 = pos1;
  pBlock->pdat.floats.f3 = speed2;
  pBlock->pdat.floats.f4 = pos2;
}

bool _write_block( FILE* f, UPBLOCK* pBlock )
{
  return( sizeof(UPBLOCK) == fwrite( pBlock, 1, sizeof(UPBLOCK), f ) );
}

int main(int argc, char *argv[])
{
  if( argc != 3 )
  {
    printf("Usage: %s input.gcode output.umc\n\n", argv[0]);
    return 0;
  }

  FILE* fgcode = fopen( argv[1], "r" );
  if( !fgcode )
    return -1;

  FILE* fdat = fopen( argv[2], "wb" );
  if( !fdat )
    return -2;


  UPBLOCK block;

  _get_block_machine_state(&block, STATE_BEEPER, 1); _write_block( fdat, &block );
  _get_block_pause(&block, 0.1); _write_block( fdat, &block );
  _get_block_machine_state(&block, STATE_BEEPER, 0); _write_block( fdat, &block );

  _get_block_machine_state(&block, STATE_MACHINE, 1); _write_block( fdat, &block );

  _get_block_set_parameter(&block, 32, 1); _write_block( fdat, &block );

  _get_block_home_axis(&block, AXIS_Z,  5.0, -2.0); _write_block( fdat, &block );
  _get_block_home_axis(&block, AXIS_X, 30.0,  9.0); _write_block( fdat, &block );
  _get_block_home_axis(&block, AXIS_Y, 30.0, -2.0); _write_block( fdat, &block );

  _get_block_pause(&block, 1.0); _write_block( fdat, &block );

  char* line = NULL;
  size_t len = 0;
  while( getline( &line, &len, fgcode ) > 0 )
  {
    float x,y,z,f;
    if( 4 == sscanf( line, "G01 X%f Y%f Z%f F%f", &x,&y,&z,&f ) )
    {
      f/=60;
      _get_block_move_f(&block, -f,-x,-f,y); _write_block( fdat, &block );
      _get_block_move_f(&block, 1000,0,1000,0); _write_block( fdat, &block );
    }
  }
  if( line )
    free( line );

  _get_block_pause(&block, 3.0); _write_block( fdat, &block );
  _get_block_machine_state(&block, STATE_MACHINE, 0); _write_block( fdat, &block );
  _get_block_stop(&block); _write_block( fdat, &block );

  fclose( fgcode );
  fclose( fdat );

  return 0; 
}