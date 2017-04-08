# if ! defined( SenderWorkerH )
#	define SenderWorkerH	/* only include me once */

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

#ifndef ThrSendWebUserH
#include "ThrSendWebUser.h"
#endif

typedef void (*TOnAddPageSendCB)(void *DataPtr);

typedef struct {
	bool                   isActive;
    bool                   KeepAliveEnable;
	void                   *DataPtr;
	TOnAddPageSendCB       OnAddPageSendCB;
	SENDER_WEB_INFO        *SelSendWebInfoPtr;
#ifdef WIN32
	unsigned int           NextSendNotifyId;
#else
	unsigned int           NextSendNotifyPort;
	unsigned int           WebServMsgPort;  /* Web server rx port */
#endif
	POOL_RECORD_BASE       SendWebPool;
	ListItsTask            HttpSendThrList;
	ListItsTask            SenderThrList;
} SENDER_WORKER_INFO;

void StopSendThreads(SENDER_WORKER_INFO *SenderWorkerPtr);
void CreateSenderThreads(SENDER_WORKER_INFO *SenderWorkerPtr);
void SenderWorkerInit(SENDER_WORKER_INFO *SenderWorkerPtr);
void SenderWorkerClose(SENDER_WORKER_INFO *SenderWorkerPtr);
void SenderInstQueueUsageReport(SENDER_WORKER_INFO *SenderWorkerPtr);
bool SendHttpFileHost(SENDER_WORKER_INFO *SenderWorkerPtr, READWEBSOCK *ReadWeb,
	char *HttpRespPtr, unsigned int HeaderLen, unsigned int DataLen);
bool SendHttpRespUser(SENDER_WORKER_INFO *SenderWorkerPtr, READWEBSOCK *ReadWeb,
	char *HttpRespPtr, unsigned int DataLen);
bool SendHtmlDataUser(SENDER_WORKER_INFO *SenderWorkerPtr, SENDWEBSOCK *ParSendWeb);
void HttpSentRemList(SENDER_WORKER_INFO *SenderWorkerPtr, SENDWEBSOCK *ParSendWeb);
SENDER_WEB_INFO* GetSenderInstance(SENDER_WORKER_INFO *SenderWorkerPtr);

//---------------------------------------------------------------------------
#endif  /* if ! defined( ReaderSenderH ) */
