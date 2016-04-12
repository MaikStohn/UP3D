//////////////////
//Author: M.Stohn/
//////////////////

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <inttypes.h>
#include <unistd.h>
#include <sys/time.h>

#include "up3d.h"
#include "compat.h"

#define upl_error(s) { printf("ERROR: %s\n",s); }

int main(int argc, char const *argv[])
{
  if( !UP3D_Open() )
    return 2;

  if( !UP3D_IsPrinterResponsive() )
  {
    upl_error( "UP printer is not responding\n" );
    UP3D_Close();
    return 3;
  }

  TT_tagPrinterInfoHeader pihdr;
  TT_tagPrinterInfoName   piname;
  TT_tagPrinterInfoData   pidata;
  TT_tagPrinterInfoSet    pisets[8];

  if( !UP3D_GetPrinterInfo( &pihdr, &piname, &pidata, pisets ) )
  {
    upl_error( "UP printer info error\n" );
    UP3D_Close();
    return 4;
  }

  printf("UP 3D PRINTER INFO:\n");
  printf("TypeID:%"PRIx32" ", pihdr.u32_printerid);
  printf("Serial:%"PRIu32" ", pihdr.u32_printerserial);
  printf("ROM:%.4f ", pihdr.f_rom_version);
  printf("Model:%s ", piname.printer_name);
  printf("U1:%.2f U3:%"PRIu32" U4:%"PRIu32" U7:%"PRIu32"",pihdr.flt_unk1,pihdr.u32_unk3,pihdr.u32_unk4,pihdr.u32_unk7 );
  printf("\n");
  
  printf("Max-X:%f ", pidata.f_max_x);
  printf("Max-Y:%f ", pidata.f_max_y);
  printf("Max-Z:%f ", pidata.f_max_z);
  printf("Steps/mm-X:%f ", pidata.f_steps_mm_x);
  printf("Steps/mm-Y:%f ", pidata.f_steps_mm_y);
  printf("Steps/mm-Z:%f ", pidata.f_steps_mm_z);
  printf("Steps--A: %f ", pidata.f_unknown_a);
  printf("Print sets: %"PRIx32"", pidata.u32_NumSets);
  printf("\n");
  printf("U2:%.2f U3:%.2f U10:%.2f U11:%.2f U12:%.2f U13:%.2f",pidata.flt_unk2,pidata.flt_unk3,pidata.flt_unk10,pidata.flt_unk11,pidata.flt_unk12,pidata.flt_unk13 );
  printf("\n");

  uint32_t s;
  for( s=0; s<pidata.u32_NumSets; s++ )
  {
    printf("Set#%"PRIu32"  ", s);
    printf("Name:\"%s\"", pisets[s].set_name);
    printf("\n  ");
    printf("ND:%.2f ", pisets[s].nozzle_diameter );
    printf("LT:%.2f ", pisets[s].layer_thickness );
    printf("SW:%.2f ", pisets[s].scan_width );
    printf("ST:%.2f ", pisets[s].scan_times );
    printf("HW:%.2f ", pisets[s].hatch_width );
    printf("HS:%.2f ", pisets[s].hatch_space );
    printf("HL:%.2f ", pisets[s].hatch_layer );
    printf("XW:%.2f ", pisets[s].support_width );
    printf("XS:%.2f ", pisets[s].support_space );
    printf("XL:%.2f ", pisets[s].support_layer );
    printf("SV:%.2f ", pisets[s].scan_speed );
    printf("\n  ");
    printf("HV:%.2f ", pisets[s].hatch_speed );
    printf("XV:%.2f ", pisets[s].support_speed );
    printf("JS:%.2f ", pisets[s].jump_speed );
    printf("SZ:%.2f ", pisets[s].scan_scale );
    printf("HZ:%.2f ", pisets[s].hatch_scale );
    printf("SZ:%.2f ", pisets[s].support_scale );
    printf("FZ:%.2f ", pisets[s].feed_scale );
    printf("\n  ");
    printf("O1:%.2f ", pisets[s].other_param_1 );
    printf("O2:%.2f ", pisets[s].other_param_2 );
    printf("O3:%.2f ", pisets[s].other_param_3 );
    printf("O4:%.2f ", pisets[s].other_param_4 );
    printf("O5:%.2f ", pisets[s].other_param_5 );
    printf("O6:%.2f ", pisets[s].other_param_6 );
    printf("U1:%.2f ", pisets[s].unused_1 );
    printf("U2:%.2f ", pisets[s].unused_2 );
    printf("U3:%.2f ", pisets[s].unused_3 );
    printf("U4:%.2f ", pisets[s].unused_4 );
    printf("U5:%.2f ", pisets[s].unused_5 );
    printf("U6:%.2f ", pisets[s].unused_6 );
    printf("\n");
  }
  printf("\n");

  UP3D_Close();
  return 0;
}

