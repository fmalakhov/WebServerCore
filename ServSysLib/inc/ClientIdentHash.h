# if ! defined( ClientIdentHashH )
#	define ClientIdentHashH	/* only include me once */

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

#ifndef StatsCodeHashH
#include "StatsCodeHash.h"
#endif

#define MAX_CLIENT_OCTET_HASH_INDEX    16

typedef struct {
    ListItsTask  StatsList;
    STATS_CODE_HASH_OCTET_HOP StatsCodeHash;
	void         *HashOctetHopPtr;    
} CLIENT_IDENT_HASH_RECORD;

typedef struct {
   unsigned int     UsedOctetCount;
   unsigned int     ParentHashOctetIndex;
   void             *ParentHashOctetPtr;
   CLIENT_IDENT_HASH_RECORD *Record;
   void             *HashOctetHop[MAX_CLIENT_OCTET_HASH_INDEX];
} CLIENT_IDENT_HASH_OCTET_HOP;

void InitClientIdentHash(CLIENT_IDENT_HASH_OCTET_HOP *NameHop);
void CloseClientIdentHash(CLIENT_IDENT_HASH_OCTET_HOP *RootNameHop);
bool AddClientIdentHash(CLIENT_IDENT_HASH_OCTET_HOP *RootNameHop, 
    unsigned char ClientType, unsigned int ClientId, unsigned short DevId, STATS_COLLECT_RECORD* RecordPtr);
CLIENT_IDENT_HASH_RECORD* FindClientIdentHash(CLIENT_IDENT_HASH_OCTET_HOP *RootNameHop,
    unsigned char ClientType, unsigned int ClientId, unsigned short DevId);
STATS_COLLECT_RECORD* FindStatsRecordHash(CLIENT_IDENT_HASH_OCTET_HOP *RootNameHop,
    unsigned char ClientType, unsigned int ClientId, unsigned short DevId, unsigned short StatsCode);
bool RemClientIdentHash(CLIENT_IDENT_HASH_OCTET_HOP *RootNameHop, unsigned char ClientType,
     unsigned int ClientId, unsigned short DevId);
void CloseClientIdentHashHop(CLIENT_IDENT_HASH_OCTET_HOP *SelHashHopCharPtr);
//---------------------------------------------------------------------------
#endif  /* if ! defined( ClientIdentHashH ) */
