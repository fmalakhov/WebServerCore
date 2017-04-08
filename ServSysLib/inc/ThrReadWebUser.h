# if ! defined( ThrReadWebUserH )
#	define ThrReadWebUserH	/* only include me once */

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

#ifdef _WEB_EPOOL_SOCKET_
#include <sys/epoll.h>
#endif

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

#ifndef MobileDeviceDataBaseH
#include "MobileDeviceDataBase.h"
#endif

#ifndef HttpParserH
#include "HttpParser.h"
#endif

#ifndef ClientSocketHashH
#include "ClientSocketHash.h"
#endif

#ifdef WIN32
  #ifndef SysMessagesH
  #include "SysMessages.h"
  #endif
  
  #define TWR_MSG_NOTIFY          (WM_USER + 150)
#endif

#ifndef _SUN_BUILD_
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/x509.h>
#include <openssl/x509_vfy.h>
#endif

#define INIT_WEB_READER_BLK_COUNT 512
#define INIT_WEB_READER_MSG_COUNT 512

#define WEB_READ_HB_CHECK_TMR_ID     0x01
#define HTTP_LD_TIMEOOT_CHECK_TMR_ID 0x02

#define WRC_CLOSE_CHANNEL         0x01
#define WRC_WEB_READ_REQ          0x02
#define WRC_HB_REQ                0x03
#define WRC_SESS_TO_CHECK		  0x04

#ifdef _WEB_EPOOL_SOCKET_
#define MAX_WEB_SESSION_PER_THREAD 1001
#define INIT_WEB_SESSION_BLK_COUNT 1000
#else
#define MAX_WEB_SESSION_PER_THREAD 121
#define INIT_WEB_SESSION_BLK_COUNT 120
#endif

#define READER_NOTIFY_SOCKET_BUF_SIZE 8192

typedef struct {
#ifdef WIN32
	HANDLE              mutex;
	DWORD               ThrReadWeb;
#else
    pthread_mutex_t     mutex;
    pthread_cond_t      cond_var;
#endif
    PoolListItsTask     queue_list;
    bool                destroy;
} ReadWebMsgQueue;

typedef struct {
    unsigned char        MsgTag;
	void                 *DataPtr;
    POOL_RECORD_STRUCT   *BlkPoolPtr;
    ObjPoolListTask      *ObjPtr;
} ReadWebMessage;

typedef struct {
    ReadWebMsgQueue      ReadWebQueue;
#ifdef WIN32
    HANDLE               ReadWebLock;
    HANDLE               ReadWebMsgAccess;
#else
    pthread_mutex_t      ReadWebLock;
    pthread_mutex_t      ReadWebMsgAccess;
#endif
    PoolListItsTask      ListReadWebMsg;
    POOL_RECORD_BASE     ReadWebMsgPool;
    unsigned int         MaxSimultActReadWebs;
    unsigned int         MaxSimultReadWebMsgs;
} ReadWebChannel;

typedef struct {
	/* Data load related info */
	bool           isInitDone;
	bool           HeadrCheck;
	bool		   HeaderLoadDone;
	unsigned int   DelayCount;
	unsigned int   ContentLdSize;
	char		   ContrEnd[5];
	PoolListItsTask ListHTTPData;
	
	SOCKET	       HttpSocket;  
    unsigned       IDMessReadWeb;
	char*          HttpReqPtr;
	char*          BoundInd;
	char*          ServRootDir;
	char*          StrCmdHTTP;
    bool           IsExtGen;       /* Flag of HTML page generation by extenal thread */
    bool           RespDelay;      /* Flag of delay of HTML page generation upon action done */
	bool           isUserAuthReq; /* Flag of user's authentification request is received */
    unsigned char  WebChanId;
	unsigned char  ReqestType;
	unsigned char  FileType;
	unsigned char  Status;
	unsigned char  MozilaMainVer;
    unsigned char  MozilaSubVer;
	unsigned char  BotType;
	unsigned char  DeviceType;
    unsigned char  BrowserType;
	unsigned long  LastDataRxTime;
	unsigned long  RxTimeOut;
	bool           isEncodingAccept;
	bool           isKeepAlive;
    bool           isCompressRequired;
    bool           isSslAccept;
    bool           isCapchaFile;
    bool           isFileRedirect;
	bool           isContentDelivery; /* Flag of content delivery mode */
	char           *SessioIdOffset;
	unsigned char  *PicFileBufPtr;
	unsigned int   PicFileLen;
	unsigned char  *XmlFileBufPtr;
	unsigned int   XmlFileLen;    
	unsigned int   ShiftBeginContent;
	unsigned int   ContentLen;
	unsigned int   HTTPReqLen;
	unsigned int   NoPwdLocalNameShift;
	unsigned long  HttpClientIP;
	unsigned int   KeepAliveTime;
	unsigned int   KeepAliveFlag;
	unsigned int   CookieSessionId;
	unsigned long  StartReqHandleTick;
	time_t         IfModifyedSince;
	void           *ReadBufPoolPtr;
	SSL_CTX        *CtxPtr;
	SSL            *SslPtr;
	BIO            *AcceptBioPtr;
	BIO            *BioPtr;
	ObjListTask    *ActListObjPtr;
	ListItsTask    *MobDevListPtr;
	MOBILE_DEV_TYPE *MobileType;
#ifdef _READER_PERF_MEASURE_
    unsigned long long int StartTime;
#endif
    ObjPoolListTask *ObjPtr;	
	char           LocalFileName[MAX_LEN_HTTP_REQ_FILE_NAME+1];
	char           RequestInfoStr[MAX_LEN_HTTP_REQ_FILE_NAME+128];
    char           HtmlFileName[1024];
} READWEBSOCK;

typedef struct {
	bool           isThreadReady;
	bool           isThreadStopReq;
	int	           ReaderSocket;
	ReadWebChannel ReadChannel;
	POOL_RECORD_BASE ReadWebPool;
	POOL_RECORD_BASE ReadBlkPool;
#ifdef WIN32
    unsigned int   MsgNotifyId;
    HANDLE	       HTRREADDATAWEB;
    DWORD	       ThrReadWebID;
#else
  #ifdef _WEB_EPOOL_SOCKET_
    int            epollfd;
    struct epoll_event Ev;
    struct epoll_event Events[MAX_WEB_SESSION_PER_THREAD];   
  #endif
	unsigned int   NotifyPort;
    unsigned int   WebServPort;
    pthread_attr_t ThrAttr;
	pthread_t	   ReadWebReq_thr;     /* Web request reader thread */
#endif
    unsigned char	*RecieveCMD;
	unsigned long   CurrTimeTick;
	CLIENT_SOCKET_HASH_CHAN SocketHash;
	PoolListItsTask         ActiveHttpSessionList;
	/* HTTP stack parsers */
    MPTT_PARSER_MESSAGE UserAgentParserMessage;
	MPTT_PARSER_MESSAGE HttpCmdParserMessage;
	MPTT_PARSER_MESSAGE HttpCotentCmdParserMessage;
	MPTT_PARSER_MESSAGE HttpFileLineParserMessage;
} READER_WEB_INFO;

unsigned int GetMaxUsageReaderQueue(READER_WEB_INFO *ReadWebInfoPtr);
unsigned int GetUsageReaderQueue(READER_WEB_INFO *ReadWebInfoPtr);
void WebDataReadReq(READER_WEB_INFO *ReadWebInfoPtr, READWEBSOCK *ReaderPtr);
void StopReaderThread(READER_WEB_INFO *ReadWebInfo);
int SendHttpPage(READWEBSOCK *ParReadWeb, unsigned char *HtmlPagePtr);
bool ReadWebThreadCreate(READER_WEB_INFO *ReadWebInfoPtr);
void ReaderInstClear(READWEBSOCK *ParReadWeb);
#ifdef WIN32
DWORD WINAPI THRReadWebUser(LPVOID Data);
#else
void* THRReadWebUser(void *arg);
#endif

//---------------------------------------------------------------------------
#endif  /* if ! defined( ThrReadWebUserH ) */
