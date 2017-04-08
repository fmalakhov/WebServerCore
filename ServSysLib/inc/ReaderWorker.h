# if ! defined( ReaderWorkerH )
#	define ReaderWorkerH	/* only include me once */

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

#ifndef ThrReadWebUserH
#include "ThrReadWebUser.h"
#endif

#define MAX_OPEN_READ_POOL 10000

typedef struct {
#ifdef WIN32
	    HANDLE           ReadPoolMutex;
#else
	    sem_t            ReadPoolMutex;
#endif
		POOL_RECORD_BASE ReadWebPool;   
		unsigned int     NextReadNotifyPort;             
} WEB_READER_POOL_ACCESS;

typedef struct {
	bool                   isActive;
    bool                   KeepAliveEnable;
	unsigned char		   NumActiveReaders;
#ifdef _LINUX_X86_
	unsigned int           BaseReaderPort;
	unsigned int           WebServMsgPort;  /* Web server rx port */
#endif
	READER_WEB_INFO        *SelReadWebInfoPtr;
	WEB_READER_POOL_ACCESS WebReaderPool;
	ListItsTask            ReaderThrList;
	ListItsTask            ActivHttpReadList;
} READER_WORKER_INFO;

void CreateReaderThreads(READER_WORKER_INFO *ReaderWorkerPtr);
void ReaderActionDone(READER_WORKER_INFO *ReaderWorkerPtr, READWEBSOCK *ParReadWeb);
void StopReadThreads(READER_WORKER_INFO *ReaderWorkerPtr);
void FreeReadPoolBuf(READER_WORKER_INFO *ReaderWorkerPtr, READWEBSOCK *ParReadWeb);
void HttpLoadAddList(READER_WORKER_INFO *ReaderWorkerPtr, READWEBSOCK *ParReadWeb);
void HttpLoadRemList(READER_WORKER_INFO *ReaderWorkerPtr, READWEBSOCK *ParReadWeb);
unsigned int GetNumberActiveReaders(READER_WORKER_INFO *ReaderWorkerPtr);
void CloseHttpReader(READER_WORKER_INFO *ReaderWorkerPtr, READWEBSOCK *ParReadWeb);
void CloseHttpChan(READWEBSOCK *ParReadWeb);
READER_WEB_INFO* GetReaderInstanceBase(READER_WORKER_INFO *ReaderWorkerPtr);
READER_WEB_INFO* GetReaderInstance(READER_WORKER_INFO *ReaderWorkerPtr);
READWEBSOCK* GetWebReaderBlock(READER_WORKER_INFO *ReaderWorkerPtr);
void ReaderInstQueueUsageReport(READER_WORKER_INFO *ReaderWorkerPtr);
#ifdef WIN32
bool ReaderWorkerInit(READER_WORKER_INFO *ReaderWorkerPtr);
#else
void ReaderWorkerInit(READER_WORKER_INFO *ReaderWorkerPtr);
void ReaderWorkerClose(READER_WORKER_INFO *ReaderWorkerPtr);
#endif
//---------------------------------------------------------------------------
#endif  /* if ! defined( ReaderWorkerH ) */
