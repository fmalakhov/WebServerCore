# if ! defined( ThrWebMgmtH )
#	define ThrWebMgmtH /* only include me once */

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

#ifndef BaseWebServerH
#include "BaseWebServer.h"
#endif

#ifndef TrTimeOutThreadH
#include "TrTimeOutThread.h"
#endif

#ifndef ThrReportMenH
#include "ThrReportMen.h"
#endif

#ifndef PoolListH
#include "PoolList.h"
#endif

#ifndef MemoryUtilizationH
#include "MemoryUtilization.h"
#endif

#ifdef _LINUX_X86_
  #ifndef ServerPortsH
  #include "ServerPorts.h"
  #endif
  
  #define WEBMGR_MSG_RX_PORT (WEBSERV_MSG_RX_PORT + 2)
#else
  #ifndef SysMessagesH
  #include "SysMessages.h"
  #endif
  
  #define TWM_WEBMGR_RESULT_START (WM_USER + 250)
  #define TWM_MSG_NOTIFY          (WM_USER + 151)
#endif

#ifndef IpAccessControlH
#include "IpAccessControl.h"
#endif

/* WebMgmt related message tags */
#define WMP_CPU_MEASURE_REQ        0x01
#define WMP_IP_CNT_CLR_REQ         0x02
#define WMP_IP_ACC_CHECK_REQ       0x03

/* WebMgmt timer identifiers */
#define CPU_UTIL_MEASURE_TMR_ID    0x01
#define IP_CNT_CLR_TMR_ID          0x02
#define IP_ACC_CHECK_TMR_ID        0x03

#define INIT_WEB_MGMT_BLK_COUNT    50   /* Start number of in pool */
#define INIT_WEB_MGMT_MSG_COUNT    50   /* Start number of system nanager messags in pool */

#ifdef WIN32
#define WEB_MGMT_ACCESS_ID (GetCurrentThreadId())
#else
#define WEB_MGMT_ACCESS_ID (WEBMGR_MSG_RX_PORT)
#endif

typedef struct {
#ifdef WIN32
	HANDLE              mutex;
	DWORD               ThrWebMgmt;
#else
    pthread_mutex_t     mutex;
    pthread_cond_t      cond_var;
#endif
    PoolListItsTask     queue_list;
    bool                destroy;
} WebMgmtMsgQueue;

typedef struct {
    unsigned char        MsgTag;
    POOL_RECORD_STRUCT   *BlkPoolPtr;
    ObjPoolListTask      *ObjPtr;
} WebMgmtMessage;

typedef struct {
	bool                 isStartDone;
    bool                 StopWebMgmtFlag; /* Flag stop of TLS worker thread */
#ifdef WIN32
    DWORD		         ThrAnswStart;
	unsigned	         IDAnswStart;	
    HANDLE               HTRWEBMGR;
    unsigned long        ThrWEBMGR;
#else
    pthread_t            ThreadId;
    sem_t	             WebMgmtSem;
#endif
} WebMgmtTaskInfo;

typedef struct {
    WebMgmtTaskInfo      WebMgmtInfo;
    WebMgmtMsgQueue      WebMgmtQueue;
#ifdef WIN32
    HANDLE               WebMgmtLock;
    HANDLE               WebMgmtMsgAccess;
#else
    pthread_mutex_t      WebMgmtLock;
    pthread_mutex_t      WebMgmtMsgAccess;
#endif
    PoolListItsTask      ListWebMgmtMsg;
    POOL_RECORD_BASE     WebMgmtMsgPool;
    unsigned int         MaxSimultActWebMgmts;
    unsigned int         MaxSimultWebMgmtMsgs;
} WebMgmtChannel;

#ifdef WIN32
static DWORD WINAPI WebMgmtHandlerThread(LPVOID Data);
#else
static void* WebMgmtHandlerThread(void *arg);
#endif

void WebMgmtsInit(PARAMWEBSERV *ParWebServPtr, TOnGetTimeMarkerCB TimeMarkerCB);
void WebMgmtsClose();
int WebMgmtThreadCreate(WebMgmtChannel *ChannelPtr);
void WebMgmtThreadClose(WebMgmtChannel *ChannelPtr);
unsigned int MaxWebMgmtQueueUseGet();

//---------------------------------------------------------------------------
#endif  /* if ! defined( ThrWebMgmtH ) */
