#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <endian.h>

#include "up3dcomm.h"
#include "up3d.h"

#define UP_UDP_PORT 31246

#pragma pack(1)
typedef struct UP3D_WIFI_HDR {
  uint16_t u16Marker;   //0xF0F0
  uint8_t  u8Reserved0; //0x00
  uint16_t u16Cmd;      //0x1001 (scan) 0x1002 (connect)   0x4001 (response1) 0x4002 (response2)
  uint16_t u16DataLen;
  uint16_t u16Reserved1;
} UP3D_WIFI_HDR;
#pragma pack()

static void _UP3DWIFI_printbuf( uint8_t* buf, uint32_t len )
{
 for( ;len>0; len-- )
   printf("%02X ",*buf++);
 printf("\n\n");
}

static int _UP3DWIFI_SendUDPResponse(int sock, struct sockaddr_in* si, uint16_t cmd, uint8_t* dat, uint16_t len)
{
  if( len>1000 )
    return -99;

  uint8_t buf[1024];
  memset( buf, 0, sizeof(buf) );

  UP3D_WIFI_HDR resp_hdr1 = { .u16Marker=htole16(0xF0F0), .u8Reserved0=0, .u16Cmd=htole16(cmd), .u16DataLen=htole16(len), .u16Reserved1=0 };
  memcpy( buf, &resp_hdr1, sizeof(UP3D_WIFI_HDR) );
  memcpy( buf+sizeof(UP3D_WIFI_HDR), dat, len );

  return sendto(sock, buf, sizeof(UP3D_WIFI_HDR)+len, 0, (struct sockaddr*)si, sizeof(struct sockaddr_in));
}

static bool _UP3DUSB_IsPrinterResponsive()
{
  static const uint8_t UP3D_CMD_1[] = { 0x01, 0x00 }; //GetFwVersion ?
  uint8_t resp[2048];
  if( (UP3DCOMM_Write( UP3D_CMD_1, sizeof(UP3D_CMD_1) ) != sizeof(UP3D_CMD_1)) ||
      (5 != UP3DCOMM_Read( resp, sizeof(resp) )) ||
      (6 != resp[4]) )
    return false;

  return true;
}

int main(void)
{
  //daemonize
  pid_t result = fork();

  if( -1 == result )
  {
    printf( "[ERROR] Failed to fork: %s.", strerror(errno) );
    return -1;
  }
  else if (result != 0)    //parent
    return 0;
  setsid();

  if( !UP3DCOMM_Open() )
    return -1;

  if( !_UP3DUSB_IsPrinterResponsive() )
    return -2;

  int udpsock;
  if( (udpsock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))<0 )
  {
    printf("[ERROR]: UDP socket create\n");
    return -1;
  }
  struct sockaddr_in si_self;
  memset(&si_self, 0, sizeof(si_self));
  si_self.sin_family = AF_INET;
  si_self.sin_port = htons(UP_UDP_PORT);
  si_self.sin_addr.s_addr = htonl(INADDR_ANY);
  if( bind(udpsock, (struct sockaddr*)&si_self, sizeof(si_self))<0 )
  {
    printf("[ERROR]: UDP socket bind\n");
    return -1;
  }

  for( ;; )
  {
    struct sockaddr_in si_host_tcp;
    bool connectTCP = false;

    struct sockaddr_in si_host_udp;
    socklen_t slen = sizeof(si_host_udp);

    uint8_t buf[2048];
    int rlen;
    if( (rlen = recvfrom(udpsock, buf, sizeof(buf), 0, (struct sockaddr*)&si_host_udp, &slen))>0 ) //receive UDP messages
    {
      printf("Received UDP packet from %s:%d\n", inet_ntoa(si_host_udp.sin_addr), ntohs(si_host_udp.sin_port));

      UP3D_WIFI_HDR *hdr = (UP3D_WIFI_HDR*)buf;
      if( ( rlen>=sizeof(UP3D_WIFI_HDR) ) &&
          ( le16toh(hdr->u16Marker) == 0xF0F0 ) &&
          ( hdr->u8Reserved0 == 0 ) &&
          ( le16toh(hdr->u16DataLen) <= (rlen-sizeof(UP3D_WIFI_HDR)) )
        )
      {
        if( (le16toh(hdr->u16Cmd) == 0x1001) && (le16toh(hdr->u16DataLen) == 0x12) ) //SCAN
        {
          printf("Search UP... ");
          memcpy(&si_host_tcp, buf+sizeof(UP3D_WIFI_HDR), sizeof(si_host_tcp));

          si_host_udp.sin_port = *((uint16_t*)&buf[25]);
          printf("sending ACK to: %s:%d\n", inet_ntoa(si_host_udp.sin_addr), ntohs(si_host_udp.sin_port));

          uint8_t dat[128]; //MUST BE 128
          uint16_t datlen = sizeof(dat);
          memset(dat,0,sizeof(dat));

          //NAME - wchar_t
          dat[64]='T';dat[66]='E';dat[68]='S';dat[70]='T';dat[72]='1';dat[74]='2';

          if( _UP3DWIFI_SendUDPResponse(udpsock, &si_host_udp, 0x4001, dat, datlen)<0 )
            printf("[ERROR]: Could not send UDP response\n");
        }
        else
        if( (le16toh(hdr->u16Cmd) == 0x1002) && (le16toh(hdr->u16DataLen) == 0x4C) ) //CONNECT
        {
          printf("Connect UP... ");
          memcpy(&si_host_tcp, buf+sizeof(UP3D_WIFI_HDR), sizeof(si_host_tcp));

          si_host_tcp.sin_family = AF_INET;
          si_host_udp.sin_port = *((uint16_t*)&buf[25]);
          printf("sending ACK to: %s:%d\n", inet_ntoa(si_host_udp.sin_addr), ntohs(si_host_udp.sin_port));

          uint8_t dat[128];
          uint16_t datlen = sizeof(dat);
          memset(dat,0,sizeof(dat));

          if( _UP3DWIFI_SendUDPResponse(udpsock, &si_host_udp, 0x4002, dat, datlen)<0 )
            printf("[ERROR]: Could not send UDP response\n");

          connectTCP = true;
        }
        else
        {
          printf("UNKNOWN-C\n");
          _UP3DWIFI_printbuf( buf, rlen );
        }
      }
      else
      {
        printf("UNKNOWN-D\n");
        _UP3DWIFI_printbuf( buf, rlen );
      }
    }

    fflush(stdout);

    if( connectTCP )
    {
      printf("Connect address: %s:%d\n", inet_ntoa(si_host_tcp.sin_addr), ntohs(si_host_tcp.sin_port));

      int tcpsock;
      if( (tcpsock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
      {
        printf("[ERROR]: Could not create tcp socket\n");
        continue;
      }

      if( connect(tcpsock, (struct sockaddr *)&si_host_tcp, sizeof(si_host_tcp)) < 0 )
      {
        printf("[ERROR]: Could not connect to host: %d\n", errno);
        close(tcpsock);
        continue;
      }

      int flag = 1;
      if( setsockopt(tcpsock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0 )
      {
        printf("[ERROR]: Could not set socket no delay\n");
        close(tcpsock);
        continue;
      }

      if( fcntl(tcpsock, F_SETFL, fcntl(tcpsock, F_GETFL) | O_NONBLOCK) < 0 )
      {
        printf("[ERROR]: Could not set socket non blocking\n");
        close(tcpsock);
        continue;
      }

      printf("Connected\n");

      int rlen = 0;
      bool bWaitUSBResponse = false;
      uint8_t bufin[4096];
      UP3D_WIFI_HDR *hdr = (UP3D_WIFI_HDR*)bufin;
      uint8_t bufout[2048];

      for( ;; )
      {
        if( !bWaitUSBResponse && (rlen<2048) )
        {
          int nlen = recv( tcpsock, &bufin[rlen], 2048, 0 );

          if( (-1==nlen) && (EAGAIN != errno) && (EWOULDBLOCK != errno) )
          {
            printf("[ERROR] TCP recv: %d\n",rlen);
            break;
          }

          if( nlen>0 )
            rlen += nlen;
        }

        if( rlen>0 )
        {
          if( !bWaitUSBResponse &&
              ( rlen>=sizeof(UP3D_WIFI_HDR) ) &&
              ( le16toh(hdr->u16Marker) == 0xF0F0 ) &&
              ( hdr->u8Reserved0 == 0 ) &&
              ( le16toh(hdr->u16DataLen) <= (rlen-sizeof(UP3D_WIFI_HDR)) )
            )
          {
            int tlen = sizeof(UP3D_WIFI_HDR) +  le16toh(hdr->u16DataLen);
            int nlen = rlen - tlen;

            if( (le16toh(hdr->u16Cmd) == 0x200E) && (le16toh(hdr->u16DataLen) > 0) )
            {
              //special case "0x7201" ==> get printer status
              if( (0x72 == bufin[sizeof(UP3D_WIFI_HDR)]) && (0x01==bufin[sizeof(UP3D_WIFI_HDR)+1]) )
              {
                TT_tagPrinterStatus status;
                if( UP3D_GetPrinterStatus(&status) )
                {
                  memcpy( bufout, &status, sizeof(status) );
                  int ulen = sizeof(status);
                  bufout[ulen]=0x06; ulen++;
                  if( send( tcpsock, bufout, ulen, 0 ) != ulen )
                  {
                    printf("[ERROR] TCP send\n");
                    break;
                  }
                }
              }
              else
              {
                if( UP3DCOMM_Write( bufin+sizeof(UP3D_WIFI_HDR), le16toh(hdr->u16DataLen) ) != le16toh(hdr->u16DataLen) )
                {
                  printf("[ERROR] USB write error\n");
                  break;
                }
                bWaitUSBResponse = true;
              }
            }
            else
            if( (le16toh(hdr->u16Cmd) == 0x2010) && (le16toh(hdr->u16DataLen) > 0) )
            {
              //TODO: do we really have to check the 32bit packet counter at end of buffer ??? TCP is reliable !

              //wait for empty buffer writes???
              int w;
              for( w=0; w<10*5; w++ )
              {
                if( UP3D_GetFreeBlocks()>=72 )
                  break;
                 usleep( 100000 );
              }

              if( UP3DCOMM_Write( bufin+sizeof(UP3D_WIFI_HDR), le16toh(hdr->u16DataLen) - 4 ) != (le16toh(hdr->u16DataLen) - 4) )
              {
                printf("[ERROR] USB write error\n");
                break;
              }
            }
            else
            {
              printf("\n\n*****************UNKNOWN CMD:");
              _UP3DWIFI_printbuf( bufin, rlen );
            }

            if( nlen>0 )
              memmove( &bufin[0], &bufin[tlen], nlen );
            rlen = nlen;
            if( rlen<0 )
            {
              printf("\n\n****************LENGTH MISMATCH\n\n");
              rlen = 0;
            }
          }
        }

        int ulen = UP3DCOMM_ReadTO( bufout, sizeof(bufout), 10 );
        if( ulen>0 )
        {
          bWaitUSBResponse = false; //TODO check for ending 0x06 ???
          if( send( tcpsock, bufout, ulen, 0 ) != ulen )
          {
            printf("[ERROR] TCP send\n");
            break;
          }
        }
      }
      printf("Connection closed\n");
      close(tcpsock);
    }
  }

  close(udpsock);

  UP3DCOMM_Close();

  return 0;
}
