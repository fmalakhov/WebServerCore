# if ! defined( RemConnMsgApiH )
#	define RemConnMsgApiH	/* only include me once */

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

#define INIT_REM_CONN_BLK_COUNT 8
#define INIT_REM_CONN_MSG_COUNT 8

#define REM_CONN_HB_CHECK_TMR_ID  0x01

#define RCC_CLOSE_CHANNEL         0x01
#define RCC_REM_CONN_REQ          0x02
#define RCC_HB_REQ                0x03

#ifdef WIN32
  #define TRC_MSG_NOTIFY          (WM_USER + 150)
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
} RemConnMsgQueue;

typedef struct {
    unsigned char        MsgTag;
    POOL_RECORD_STRUCT   *BlkPoolPtr;
    ObjPoolListTask      *ObjPtr;
} RemConnMessage;

typedef struct {
    RemConnMsgQueue   RemConnQueue;
#ifdef WIN32
    HANDLE               RemConnLock;
    HANDLE               RemConnMsgAccess;
#else
    pthread_mutex_t      RemConnLock;
    pthread_mutex_t      RemConnMsgAccess;
#endif
    PoolListItsTask      ListRemConnMsg;
    POOL_RECORD_BASE     RemConnMsgPool;
    unsigned int         MaxSimultActRemConns;
    unsigned int         MaxSimultRemConnMsgs;
} RemConnChannel;

void RemConnMsgQueueCreate(RemConnMsgQueue *MsgQueuePtr);
void RemConnMsgQueuePost(RemConnMsgQueue *p_msg_queue, void* p_message);
bool GetRemConnMessageQueue(RemConnMsgQueue *p_msg_queue, void **pp_message);
void RemConnMsgQueueMarkDestroy(RemConnMsgQueue *p_msg_queue);
RemConnMessage* GetRemConnMsgPool(RemConnChannel *ChannelPtr, unsigned char MsgTag);
void FreeRemConnMsgPool(RemConnChannel *ChannelPtr, RemConnMessage *RemoteConnMsgPtr);
void RemConnMsgQueueDestroy(RemConnMsgQueue *MsgQueuePtr);
void RemConnMsgApiInit(RemConnChannel *ChannelPtr);
void RemConnMsgApiClose(RemConnChannel *ChannelPtr);

//---------------------------------------------------------------------------
#endif  /* if ! defined( RemConnMsgApiH ) */
