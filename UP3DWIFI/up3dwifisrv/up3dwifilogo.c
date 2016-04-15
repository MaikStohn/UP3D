#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>
#include <inttypes.h>

#include "up3dwifilogo.h"

int oled_cmd(int fd, uint8_t cmd)
{
  uint8_t buf[2] = {0x80,cmd};
  return write( fd, buf, sizeof(buf) );
}

int oled_dat(int fd, uint8_t* dat, uint8_t len )
{
  if( len>255 )
    return -1;

  uint8_t buf[256] = {0x40};
  memcpy( buf+1, dat, len );
    return write( fd, buf, 1+len );
}

void oled_send_fb( int fd, uint8_t *frame )
{
  uint32_t p;
  for( p=0; p<8; p++ )
  {
    oled_cmd(fd, 0xB0 | p); //set page
    oled_cmd(fd, 0x02 );    //set write address lo
    oled_cmd(fd, 0x10 );    //set write address high
    oled_dat(fd, &frame[p<<7], 128); //send second 64 bytes
  }
}

int main(int argc, char* argv[])
{
  int i;
  int fdoled = open( "/dev/i2c-0", O_RDWR );
  if( fdoled<0 )
  {
    printf("error opening bus: /dev/i2c-0\n");
    return 1;
  }

  if (ioctl(fdoled, I2C_SLAVE, 0x3C) < 0)
  {
    printf("error set i2c slave address");
    return 1;
  }

  //init
  oled_cmd(fdoled,0x8D); oled_cmd(fdoled,0x14); //charge pump on
  oled_cmd(fdoled,0x20), oled_cmd(fdoled,0x02); //memory address mode page
  oled_cmd(fdoled,0xC8);                        //com scan dir (flip y)
  oled_cmd(fdoled,0xA1);                        //seg scan dir (flip x)

  //send frame
  oled_send_fb( fdoled, up3dwifilogo );

  oled_cmd(fdoled,0xAF);                        //display on

  close(fdoled);
  return 0;
}
