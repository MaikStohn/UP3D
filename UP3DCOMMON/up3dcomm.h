//////////////////
//Author: M.Stohn/
//////////////////

#ifndef _UP3DCOMM_H_
#define _UP3DCOMM_H_

#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>

bool UP3DCOMM_Open();
void UP3DCOMM_Close();

int  UP3DCOMM_Read( const uint8_t *data, const size_t maxdatalen );
int  UP3DCOMM_ReadTO( const uint8_t *data, const size_t maxdatalen, const int timeout );
int  UP3DCOMM_Write( const uint8_t *data, const size_t datalen );

#endif //_UP3DCOMM_H_
