# if ! defined( WebServMsgApiH )
#	define WebServMsgApiH	/* only include me once */

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

#ifndef PoolListH
#include "PoolList.h"
#endif

#define INIT_WEB_SERV_BLK_COUNT 2048
#define INIT_WEB_SERV_MSG_COUNT 2048

#define RCC_CLOSE_CHANNEL         0x01
#define RCC_REM_CONN_REQ          0x02
#define RCC_HB_REQ                0x03

#define WHN_READER_IND            0x01
#define WHN_SENDER_IND            0x02
#define WHN_REMCONN_IND           0x03

#ifdef WIN32
  #define TWS_MSG_NOTIFY          (WM_USER + 150)
#endif

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
} WebServMsgQueue;

typedef struct {
	InterThreadInfoMsg   Msg;
    POOL_RECORD_STRUCT   *BlkPoolPtr;
    ObjPoolListTask      *ObjPtr;
} WebServMessage;

typedef struct {
	unsigned int         TimerExpMsgId;
    WebServMsgQueue      WebServQueue;
#ifdef WIN32
    HANDLE               WebServLock;
    HANDLE               WebServMsgAccess;
#else
    pthread_mutex_t      WebServLock;
    pthread_mutex_t      WebServMsgAccess;
#endif
    PoolListItsTask      ListWebServMsg;
    POOL_RECORD_BASE     WebServMsgPool;
    unsigned int         MaxSimultActWebServs;
    unsigned int         MaxSimultWebServMsgs;
} WebServChannel;

void WebServMsgQueuePost(WebServMsgQueue *p_msg_queue, void* p_message);
void WebServMsgQueueMarkDestroy(WebServMsgQueue *p_msg_queue);
WebServMessage* GetWebServMsgPool(WebServChannel *ChannelPtr, unsigned int MsgTag);
bool GetWebServMessageQueue(WebServMsgQueue *p_msg_queue, void **pp_message);
void FreeWebServMsgPool(WebServChannel *ChannelPtr, WebServMessage *WebServMsgPtr);
void WebServMsgApiInit();
void WebServMsgApiClose();
void OnWebServerTimerExp(unsigned int TimerId);
void WebServerMsgSent(unsigned int MsgTag, void *WParam, void *LParam);

//---------------------------------------------------------------------------
#endif  /* if ! defined( WebServMsgApiH ) */
