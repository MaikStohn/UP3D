/*
  up3dwifisrv.c for UP3DWIFI
  M. Stohn 2016

  This is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This software is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  If not, see <http://www.gnu.org/licenses/>.
*/

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
#include <pthread.h>

#include "up3d.h"
#include "up3dcomm.h"
#include "compat.h"

#define UP_UDP_PORT 31246

/*
0x1001 Scan              ==> resp: 4001(EchoInfo) 4002(EchoMsg)
0x1002 Connect
0x200A GetMultiParam     ==> resp: 2003 2004 2005 2006 4002
0x200D CommTest
0x200D KeepAlive
0x200E GetWifiResponse
0x2010 GetWifiResponse


//P_UP             = 0
//P_UP_PLUS_2      = 1
//P_UP_Plus_3      = 2
//P_UP_MINI        = 3
//P_UP_MINI2       = 4
//P_UP_BOX_3D      = 5
//P_UP_MARS        = 6
*/

static void _UP3DWIFI_printbuf( uint8_t* buf, uint32_t len )
{
  for( ;len>0; len-- )
    printf("%02X ",*buf++);
  printf("\n\n");
}

static bool _UP3DTCPConnected = false;

static void* _UP3DTCPHandler(void* arg)
{
  if( !arg )
    return (void*)-1;

  struct sockaddr_in si_host_tcp;
  memcpy( &si_host_tcp, (struct sockaddr_in*)arg, sizeof(si_host_tcp) );

  printf("Connect address: %s:%d\n", inet_ntoa(si_host_tcp.sin_addr), ntohs(si_host_tcp.sin_port));

  int tcpsock;
  if( (tcpsock = socket(AF_INET, SOCK_STREAM, 0)) < 0 )
  {
    //printf("[ERROR]: Could not create tcp socket\n");
    return (void*)-2;
  }

  if( connect(tcpsock, (struct sockaddr *)&si_host_tcp, sizeof(si_host_tcp)) < 0 )
  {
    //printf("[ERROR]: Could not connect to host: %d\n", errno);
    close(tcpsock);
    return (void*)-3;
  }

  int flag = 1;
  if( setsockopt(tcpsock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag)) < 0 )
  {
    //printf("[ERROR]: Could not set socket no delay\n");
    close(tcpsock);
    return (void*)-4;
      }

  if( fcntl(tcpsock, F_SETFL, fcntl(tcpsock, F_GETFL) | O_NONBLOCK) < 0 )
  {
    //printf("[ERROR]: Could not set socket non blocking\n");
    close(tcpsock);
    return (void*)-5;
  }

  _UP3DTCPConnected = true;
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
        //printf("[ERROR] TCP recv: %d\n",rlen);
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
                //printf("[ERROR] TCP send\n");
                break;
              }
            }
          }
          else
          {
            if( UP3DCOMM_Write( bufin+sizeof(UP3D_WIFI_HDR), le16toh(hdr->u16DataLen) ) != le16toh(hdr->u16DataLen) )
            {
              //printf("[ERROR] USB write error\n");
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
            //printf("[ERROR] USB write error\n");
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
        //printf("[ERROR] TCP send\n");
        break;
      }
    }
  }

  printf("Connection closed\n");
  close(tcpsock);
  _UP3DTCPConnected = false;

  return (void*)0;
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

static void* _UP3DUDPResponder(void* args)
{
  int udpsock;
  if( (udpsock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP))<0 )
  {
    //printf("[ERROR]: UDP socket create\n");
    return (void*)-1;
  }
  struct sockaddr_in si_self;
  memset(&si_self, 0, sizeof(si_self));
  si_self.sin_family = AF_INET;
  si_self.sin_port = htons(UP_UDP_PORT);
  si_self.sin_addr.s_addr = htonl(INADDR_ANY);
  if( bind(udpsock, (struct sockaddr*)&si_self, sizeof(si_self))<0 )
  {
    //printf("[ERROR]: UDP socket bind\n");
    return (void*)-2;
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
          printf("Search UP...\n");
          memcpy(&si_host_tcp, buf+sizeof(UP3D_WIFI_HDR), sizeof(si_host_tcp));

          si_host_udp.sin_port = *((uint16_t*)&buf[25]);
          //printf("sending ACK to: %s:%d\n", inet_ntoa(si_host_udp.sin_addr), ntohs(si_host_udp.sin_port));

          TT_tagEchoInfo echoinfo = {
            .serNum1=111,
            .serNum2=0,
            .systemType=0x2777, //UpMini, ...
            .accessCtrl=0,
            .workState=0,
            .haveHost=0,
            .hostName={'H',0,'o',0,'s',0,'t',0,0,0},
            .printerName={'U',0,'p',0,'M',0,'i',0,'n',0,'i',0,'(',0,'A',0,')',0,0,0},
            .printerType={'T',0,'y',0,'p',0,'e',0,' ',0,' ',0,' ',0,' ',0,' ',0,0,0},
          };
          memcpy( &echoinfo.udpAddr, &si_self, sizeof(si_self) );

          if( _UP3DWIFI_SendUDPResponse(udpsock, &si_host_udp, 0x4001, (uint8_t*)&echoinfo, sizeof(echoinfo))<0 )
          {
            //printf("[ERROR]: Could not send UDP response\n");
          }
        }
        else
        if( (le16toh(hdr->u16Cmd) == 0x1002) && (le16toh(hdr->u16DataLen) == 0x4C) ) //CONNECT
        {
          printf("Connect UP...\n");
          memcpy(&si_host_tcp, buf+sizeof(UP3D_WIFI_HDR), sizeof(si_host_tcp));

          si_host_tcp.sin_family = AF_INET;
          si_host_udp.sin_port = *((uint16_t*)&buf[25]);
          //printf("sending ACK to: %s:%d\n", inet_ntoa(si_host_udp.sin_addr), ntohs(si_host_udp.sin_port));

          uint8_t dat[128];
          uint16_t datlen = sizeof(dat);
          memset(dat,0,sizeof(dat));

          if( _UP3DWIFI_SendUDPResponse(udpsock, &si_host_udp, 0x4002, dat, datlen)<0 )
          {
            //printf("[ERROR]: Could not send UDP response\n");
          }

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

    if( connectTCP && !_UP3DTCPConnected )
    {
      pthread_t tid;
      pthread_create( &tid, NULL, _UP3DTCPHandler, (void*)&si_host_tcp );
    }
  }

  close(udpsock);
}

int main(int argc, char* argv[])
{
  if( argc<2 )
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
  }

  if( !UP3DCOMM_Open() )
    return -1;

  if( !UP3D_IsPrinterResponsive() )
    return -2;

  pthread_t tid;
  pthread_create( &tid, NULL, _UP3DUDPResponder, NULL );
  int status;
  pthread_join( tid, (void**)&status );

  UP3DCOMM_Close();

  return status;
}
