# if ! defined( SessionIpHashH )
#	define SessionIpHashH	/* only include me once */

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

#define MAX_OCTET_HASH_INDEX    16

typedef struct {
    unsigned long   SessionCount;
	void            *HashOctetHopPtr;
} SESSION_IP_HASH_RECORD;

typedef struct {
   unsigned int     UsedOctetCount;
   unsigned int     ParentHashOctetIndex;
   void             *ParentHashOctetPtr;
   SESSION_IP_HASH_RECORD *Record;
   void             *HashOctetHop[MAX_OCTET_HASH_INDEX];
} SESSION_IP_HASH_OCTET_HOP;

void InitSessionIpHash(SESSION_IP_HASH_OCTET_HOP *NameHop);
void CloseSessionIpHash(SESSION_IP_HASH_OCTET_HOP *RootNameHop);
bool AddSessionIpHash(SESSION_IP_HASH_OCTET_HOP *RootNameHop, unsigned long SessionIp);
SESSION_IP_HASH_RECORD* FindSessionIpHash(SESSION_IP_HASH_OCTET_HOP *RootNameHop, unsigned long SessionIp);
bool RemSessionIpHash(SESSION_IP_HASH_OCTET_HOP *RootNameHop, unsigned long SessionIp);
void CloseSessionIpHashHop(SESSION_IP_HASH_OCTET_HOP *SelHashHopCharPtr);
//---------------------------------------------------------------------------
#endif  /* if ! defined( SessionIpHashH ) */
