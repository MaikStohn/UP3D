//////////////////
//Author: M.Stohn/
//////////////////

#include "up3dcomm.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <libusb-1.0/libusb.h>

//#define _DEBUG_IN_OUT_

#define VID (0x4745)
#define PID_MINI_A (0x0001)
#define PID_MINI_M (0x2777)
#define EP_OUT 1
#define EP_IN  1

static libusb_context* _libusb_ctx              = NULL;
static libusb_device_handle* _libusb_dev_handle = NULL;

#ifdef _DEBUG_IN_OUT_
static void _print_buffer( const uint8_t* data, const size_t datalen )
{
  unsigned int pos;
  for( pos=0; pos<datalen; pos++ )
    printf("%02X ", data[pos]);
  printf("\n");
}
#endif

bool UP3DCOMM_Open()
{
  int r;

  r = libusb_init( &_libusb_ctx );
  if( r < 0 ) 
  {
    printf( "[ERROR] USB Init: %d\n", r );
    return false;
  }

  libusb_set_debug( _libusb_ctx, 0 ); //set verbosity level to 0

  _libusb_dev_handle = libusb_open_device_with_vid_pid( _libusb_ctx, VID, PID_MINI_A );
  if( !_libusb_dev_handle )
    _libusb_dev_handle = libusb_open_device_with_vid_pid( _libusb_ctx, VID, PID_MINI_M );
  
  
  if( !_libusb_dev_handle )
  {
    fprintf(stderr, "[ERROR] USB Open Device (%04X:%04X/%04X) not found\n", VID, PID_MINI_A, PID_MINI_M );
    UP3DCOMM_Close();
    return false;
  }

  if( 1 == libusb_kernel_driver_active( _libusb_dev_handle, 0 ) )
    libusb_detach_kernel_driver( _libusb_dev_handle, 0 );

  if( libusb_claim_interface( _libusb_dev_handle, 0 ) < 0 ) 
  {
    fprintf(stderr,"[ERROR] USB Claim Interface\n");
    UP3DCOMM_Close();
    return false;
  }
  
  return true;
}

void UP3DCOMM_Close()
{
  if( _libusb_dev_handle )
    libusb_close( _libusb_dev_handle );

  _libusb_dev_handle = NULL;

  if( _libusb_ctx )
    libusb_exit( _libusb_ctx );
  _libusb_ctx = NULL;
}

int UP3DCOMM_Read( const uint8_t *data, const size_t maxdatalen )
{
  int read;
  int ret;
  ret = libusb_bulk_transfer( _libusb_dev_handle, (EP_IN | LIBUSB_ENDPOINT_IN), (uint8_t*)data, maxdatalen, &read, 5000);
  if (ret != 0)
  {
//#ifdef _DEBUG_IN_OUT_
	fprintf(stderr,"UP3DCOMM_Read failed with error code %d. Bytes %d of %d read.", ret, read, (int)maxdatalen);
//#endif  
    return -1;
  }

#ifdef _DEBUG_IN_OUT_
  fprintf(stderr,"<"); _print_buffer( data, read );
#endif

  return read;
}

int UP3DCOMM_Write( const uint8_t *data, const size_t datalen )
{
#ifdef _DEBUG_IN_OUT_
  fprintf(stderr,">"); _print_buffer( data, datalen );
#endif

  int written;
  int ret;
  ret = libusb_bulk_transfer( _libusb_dev_handle, (EP_OUT | LIBUSB_ENDPOINT_OUT), (uint8_t*)data, datalen, &written, 5000);
  if (ret != 0)
  {
//#ifdef _DEBUG_IN_OUT_
	fprintf(stderr,"UP3DCOMM_Write failed with error code %d. Bytes %d of %d written", ret, written, (int)datalen );
//#endif  
    return -1;
  }

  return written;
}

