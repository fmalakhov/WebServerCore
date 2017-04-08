# if ! defined( ClientSocketHashH )
#	define ClientSocketHashH	/* only include me once */

/*
Copyright (c) 2012-2017 MFBS, Fedor Malakhov

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef CommonPlatformH
#include "CommonPlatform.h"
#endif

#ifndef vistypesH
#include "vistypes.h"
#endif

#ifndef SysLibToolH
#include "SysLibTool.h"
#endif

#ifndef MemoryPoolH
#include "MemoryPool.h"
#endif

#define MAX_SOCKET_INDEX_OCTET        5
#define MAX_SOCKET_OCTET_HASH_INDEX   16

#define SUCCES_ADD_SOCKET             1
#define FAIL_ADD_SOCKET_ALREADY_EXIST 2
#define FAIL_ADD_SOCKET_MEMORY_ALLOC  3

#define INIT_CLSOCKET_OCTET_POOL_SIZE    100000
#define INIT_CLSOCKET_REC_POOL_SIZE      1000

typedef struct {
    void           *ClientPtr;
	void           *HashOctetHopPtr;
	POOL_RECORD_STRUCT *PoolRecordPtr;
} CLIENT_SOCKET_HASH_RECORD;

typedef struct {
    unsigned int     UsedOctetCount;
    unsigned int     ParentHashOctetIndex;
    void             *ParentHashOctetPtr;
    CLIENT_SOCKET_HASH_RECORD *Record;
    POOL_RECORD_STRUCT  *PoolRecordPtr;
    void             *HashOctetHop[MAX_SOCKET_OCTET_HASH_INDEX];
} CLIENT_SOCKET_HASH_OCTET_HOP;

typedef struct {
    POOL_RECORD_BASE SocketHashOctetPool;
    POOL_RECORD_BASE SocketHashRecordPool;
    CLIENT_SOCKET_HASH_OCTET_HOP RootSocketHop;
} CLIENT_SOCKET_HASH_CHAN;

void ClientSocketHashChanCreate(CLIENT_SOCKET_HASH_CHAN *ChanPtr);
void ClientSocketHashChanClose(CLIENT_SOCKET_HASH_CHAN *ChanPtr);
void* ClientInfoFindBySocket(CLIENT_SOCKET_HASH_CHAN *ChanPtr, unsigned int ClientSocket);
unsigned char AddClientInfoSocketHash(CLIENT_SOCKET_HASH_CHAN *ChanPtr, 
	void *ClientPtr, unsigned int ClientSocket);
bool RemClientInfoSocketHash(CLIENT_SOCKET_HASH_CHAN *ChanPtr, unsigned int ClientSocket);
//---------------------------------------------------------------------------
#endif  /* if ! defined( ClientSocketHashH ) */
