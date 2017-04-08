# if ! defined( ThrSendWebUserH )
#	define ThrSendWebUserH	/* only include me once */

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

#ifndef SysWebFunctionH
#include "SysWebFunction.h"
#endif

#ifndef MemoryPoolH
#include "MemoryPool.h"
#endif

#ifndef PoolListH
#include "PoolList.h"
#endif

#ifndef WebServInfoH
#include "WebServInfo.h"
#endif

#ifndef GZipH
#include "gzip.h"
#endif

#ifndef ThrReadWebUserH
#include "ThrReadWebUser.h"
#endif

#ifdef WIN32
  #ifndef SysMessagesH
  #include "SysMessages.h"
  #endif
  
  #define TWS_MSG_NOTIFY           (WM_USER + 150)
#endif

#ifndef _SUN_BUILD_
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#endif

#define INIT_WEB_SENDER_BLK_COUNT 4
#define INIT_WEB_SENDER_MSG_COUNT 4

#define WEB_SEND_HB_CHECK_TMR_ID  0x01

#define WSC_CLOSE_CHANNEL         0x01
#define WSC_WEB_SEND_REQ          0x02
#define WSC_HB_REQ                0x03

typedef struct {
#ifdef WIN32
	HANDLE              mutex;
	DWORD               ThrSendWeb;
#else
    pthread_mutex_t     mutex;
    pthread_cond_t      cond_var;
#endif
    PoolListItsTask     queue_list;
    bool                destroy;
} SendWebMsgQueue;

typedef struct {
    unsigned char        MsgTag;
	void                 *DataPtr;
    POOL_RECORD_STRUCT   *BlkPoolPtr;
    ObjPoolListTask      *ObjPtr;
} SendWebMessage;

typedef struct {
    SendWebMsgQueue      SendWebQueue;
#ifdef WIN32
    HANDLE               SendWebLock;
    HANDLE               SendWebMsgAccess;
#else
    pthread_mutex_t      SendWebLock;
    pthread_mutex_t      SendWebMsgAccess;
#endif
    PoolListItsTask      ListSendWebMsg;
    POOL_RECORD_BASE     SendWebMsgPool;
    unsigned int         MaxSimultActSendWebs;
    unsigned int         MaxSimultSendWebMsgs;
} SendWebChannel;

typedef struct {
	SOCKET	       HttpSocket;
	bool           isHashData;
	bool           isBodyCompress;
	bool           isHeaderReq;
	char*          HttpRespPtr;
	char*          HeaderBeginSetLenPtr;
	READWEBSOCK*   ReadWebPtr;
	void*          SendBufPoolPtr;
	unsigned char  WorkPhase;
	unsigned char  FileType;
	unsigned char  isDeliverySuccess;
	unsigned int   HTTPRespLen;
	unsigned long  StartTick;
	unsigned int   SentBytes;
	unsigned long  PageProcessStartTick;
	unsigned int   HeaderLen;
	char           HttpRespHeader[1024];
	char           FileName[512];
    GZIP_TASK_INFO ZipInfo;
	SSL            *SslPtr;
	ObjListTask    *ActListObjPtr;
} SENDWEBSOCK;

typedef struct {
	bool           isThreadReady;
	bool           isThreadStopReq;
    unsigned       IDMessSendWeb;
	unsigned int   ComprBufSize;
	SendWebChannel SendChannel;
	char*          ComprDataBufPtr;
#ifdef WIN32
    unsigned int   MsgNotifyId;
    HANDLE	       HTRSENDDATAWEB;
	DWORD          ParentThrID;
    DWORD	       ThrSendWebID;
#else
	unsigned int   NotifyPort;
    unsigned int   WebServPort;
    pthread_attr_t ThrAttr;
	pthread_t	   SendWebResp_thr;     /* Web response sender thread */
#endif
} SENDER_WEB_INFO;

void OnWebSendHbTimerExp(unsigned int TimerId, void *DataPtr);
void WebDataSentReq(SENDER_WEB_INFO *SendWebInfoPtr, SENDWEBSOCK *SendWebDataPtr);
void StopSenderThread(SENDER_WEB_INFO *SendWebInfoPtr);
bool SendWebThreadCreate(SENDER_WEB_INFO *SendWebInfoPtr);
unsigned int GetMaxUsageSenderQueue(SENDER_WEB_INFO *SendWebInfoPtr);
unsigned int GetUsageSenderQueue(SENDER_WEB_INFO *SendWebInfoPtr);
#ifdef WIN32
DWORD WINAPI THRSendWebUser(LPVOID Data);
#else
void* THRSendWebUser(void *Data);
#endif

//---------------------------------------------------------------------------
#endif  /* if ! defined( ThrSendWebUserH ) */
