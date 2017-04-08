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

#include "WebServMsgApi.h"
#include "ThrReportMen.h"

extern char ThrWebServName[];

WebServChannel  gWebServMsgChannel;
//---------------------------------------------------------------------------
void WebServMsgQueueCreate(WebServMsgQueue *MsgQueuePtr)
{
	DebugLogPrint(NULL, "%s: Initializing WEB server messages queue.\n", ThrWebServName);
#ifdef WIN32
	MsgQueuePtr->mutex = CreateMutex(NULL, FALSE, NULL);
    if (MsgQueuePtr->mutex == NULL) 
    {
        printf("Create WEB serv. msg queue mutex error: %d\r\n", GetLastError());
	    exit(EXIT_FAILURE);
    }
#else
    pthread_mutex_init(&(MsgQueuePtr->mutex), NULL);
    pthread_cond_init(&(MsgQueuePtr->cond_var), NULL);
#endif
    PoolListInit(&MsgQueuePtr->queue_list, INIT_WEB_SERV_MSG_COUNT);
    MsgQueuePtr->destroy = false;
}
//---------------------------------------------------------------------------
void WebServMsgQueueMarkDestroy(WebServMsgQueue *MsgQueuePtr)
{
	DebugLogPrint(NULL, "%s: Marking Web serv. message queue for destroy.\n", ThrWebServName);
    MsgQueuePtr->destroy = true;
    WebServMsgQueuePost(MsgQueuePtr, NULL);
}
//---------------------------------------------------------------------------
void WebServMsgQueuePost(WebServMsgQueue *MsgQueuePtr, void *p_message)
{
#ifdef WIN32
    unsigned int CurrUsage;

    if (WaitForSingleObject(MsgQueuePtr->mutex, INFINITE) == WAIT_FAILED)
	{
		printf("Fail to get mutex (WebServMsgQueuePost)\r\n");
        return;
	}
	CurrUsage = (unsigned int)MsgQueuePtr->queue_list.Count;
	AddPoolStructList(&MsgQueuePtr->queue_list, p_message);
    if (!ReleaseMutex(MsgQueuePtr->mutex)) 
        printf("Fail to release mutex (WebServMsgQueuePost)\r\n");
	if (!CurrUsage) WinThreadMsgSend(MsgQueuePtr->ThrSendWeb, TWS_MSG_NOTIFY, 0, 0);
#else
    pthread_mutex_lock(&(MsgQueuePtr->mutex));
    AddPoolStructList(&MsgQueuePtr->queue_list, p_message);
    pthread_mutex_unlock(&(MsgQueuePtr->mutex));
    pthread_cond_signal(&(MsgQueuePtr->cond_var));
#endif
}
//---------------------------------------------------------------------------
void WebServMsgQueueDestroy(WebServMsgQueue *MsgQueuePtr)
{
	DebugLogPrint(NULL, "%s: De-initializing WEB serv. messages queue.\n", ThrWebServName);
#ifdef WIN32
	CloseHandle(MsgQueuePtr->mutex);
#else
    pthread_mutex_destroy(&(MsgQueuePtr->mutex));
    pthread_cond_destroy(&(MsgQueuePtr->cond_var));
#endif
    DestroyPoolListStructs(&MsgQueuePtr->queue_list);
}
//---------------------------------------------------------------------------
bool GetWebServMessageQueue(WebServMsgQueue *MsgQueuePtr, void **pp_message)
{
    ObjPoolListTask *ObjPtr = NULL;
    bool status = true;
#ifdef WIN32
    struct tagMSG SysMsg;
#endif

    *pp_message = NULL;
#ifdef WIN32
    if (WaitForSingleObject(MsgQueuePtr->mutex, INFINITE) == WAIT_FAILED)
	{
		printf("Fail to get mutex (1) (GetWebServMessageQueue)\r\n");
        return false;
	}
#else
    pthread_mutex_lock(&(MsgQueuePtr->mutex));
#endif
    ObjPtr = (ObjPoolListTask*)GetFistPoolObjectList(&MsgQueuePtr->queue_list);
    if (ObjPtr)
    {
        *pp_message = ObjPtr->UsedTask;
        RemPoolStructList(&MsgQueuePtr->queue_list, ObjPtr);
    }
    else
    {
        for(;;)
        {
#ifdef WIN32
            if (!ReleaseMutex(MsgQueuePtr->mutex)) 
                printf("Fail to release mutex (GetWebServMessageQueue)\r\n");
			GetMessage(&SysMsg, NULL, TWS_MSG_NOTIFY, TWS_MSG_NOTIFY);
#else
            pthread_cond_wait(&(MsgQueuePtr->cond_var), &(MsgQueuePtr->mutex));
#endif
#ifdef WIN32
            if (WaitForSingleObject(MsgQueuePtr->mutex, INFINITE) == WAIT_FAILED)
	        {
		        printf("Fail to get mutex (2) (GetWebServMessageQueue)\r\n");
				status = false;
                break;
	        }
#endif
            ObjPtr = (ObjPoolListTask*)GetFistPoolObjectList(&MsgQueuePtr->queue_list);
            if (ObjPtr)
            {
                *pp_message = ObjPtr->UsedTask;
                RemPoolStructList(&MsgQueuePtr->queue_list, ObjPtr);
                break;
            }
        }
    }

    if (MsgQueuePtr->destroy)
    {
		DebugLogPrint(NULL, "%s: Wokeup WEB serv. due to destroy flag.\n", ThrWebServName);
        status = false;
    }
#ifdef WIN32
    if (!ReleaseMutex(MsgQueuePtr->mutex)) 
        printf("Fail to release mutex (GetWebServMessageQueue)\r\n");
#else
    pthread_mutex_unlock(&(MsgQueuePtr->mutex));
#endif
    return status;
}
//---------------------------------------------------------------------------
WebServMessage* GetWebServMsgPool(WebServChannel *ChannelPtr, unsigned int MsgTag)
{
    WebServMessage   *WebServMsgPtr = NULL;
    POOL_RECORD_STRUCT *ObjTaskPtr = NULL;

#ifdef WIN32
    if (WaitForSingleObject(ChannelPtr->WebServMsgAccess, INFINITE) == WAIT_FAILED) return NULL;
#else
    pthread_mutex_lock(&ChannelPtr->WebServMsgAccess);
#endif
    ObjTaskPtr = GetBuffer(&ChannelPtr->WebServMsgPool);
    if (!ObjTaskPtr)
    {
		DebugLogPrint(NULL, "%s: No buffers for WEB serv message delivery.\n", ThrWebServName);
#ifdef WIN32
        if (!ReleaseMutex(ChannelPtr->WebServMsgAccess)) 
            printf("Fail to release mutex (GetWebServMsgPool)\r\n");
#else
        pthread_mutex_unlock(&ChannelPtr->WebServMsgAccess);
#endif
        return NULL;
    }
    WebServMsgPtr = (WebServMessage*)ObjTaskPtr->DataPtr;
    WebServMsgPtr->Msg.MsgTag = MsgTag;
    WebServMsgPtr->BlkPoolPtr = ObjTaskPtr;
    WebServMsgPtr->ObjPtr = AddPoolStructListObj(&ChannelPtr->ListWebServMsg, WebServMsgPtr);
    if (ChannelPtr->MaxSimultWebServMsgs < (unsigned int)ChannelPtr->ListWebServMsg.Count)
        ChannelPtr->MaxSimultWebServMsgs = (unsigned int)ChannelPtr->ListWebServMsg.Count;
#ifdef WIN32
    if (!ReleaseMutex(ChannelPtr->WebServMsgAccess)) 
        printf("Fail to release mutex (GetWebServMsgPool)\r\n");
#else
    pthread_mutex_unlock(&ChannelPtr->WebServMsgAccess);
#endif
    return WebServMsgPtr;
}
//---------------------------------------------------------------------------
void FreeWebServMsgPool(WebServChannel *ChannelPtr, WebServMessage *WebServMsgPtr)
{
#ifdef WIN32
    if (WaitForSingleObject(ChannelPtr->WebServMsgAccess, INFINITE) == WAIT_FAILED)
	{
        return;
	}
#else
    pthread_mutex_lock(&ChannelPtr->WebServMsgAccess);
#endif
    FreeBuffer(&ChannelPtr->WebServMsgPool, WebServMsgPtr->BlkPoolPtr);
    RemPoolStructList(&ChannelPtr->ListWebServMsg, WebServMsgPtr->ObjPtr);
#ifdef WIN32
    if (!ReleaseMutex(ChannelPtr->WebServMsgAccess)) 
        printf("Fail to release mutex (FreeWebServMsgPool)\r\n");
#else
    pthread_mutex_unlock(&ChannelPtr->WebServMsgAccess);
#endif
}
//---------------------------------------------------------------------------
void WebServMsgApiInit(unsigned int TimerExpMsgId)
{
	WebServChannel *ChannelPtr = NULL;

	ChannelPtr = &gWebServMsgChannel;
	memset(ChannelPtr, 0, sizeof(WebServChannel));
	ChannelPtr->TimerExpMsgId = TimerExpMsgId;
    PoolListInit(&ChannelPtr->ListWebServMsg, INIT_WEB_SERV_BLK_COUNT);
    CreatePool(&ChannelPtr->WebServMsgPool, INIT_WEB_SERV_BLK_COUNT, sizeof(WebServMessage));
#ifdef _LINUX_X86_
    pthread_mutex_init(&ChannelPtr->WebServMsgAccess, NULL);
#endif
    WebServMsgQueueCreate(&ChannelPtr->WebServQueue);
}
//---------------------------------------------------------------------------
void WebServMsgApiClose()
{
    WebServMessage  *WebServMsgPtr = NULL;
    ObjPoolListTask *PointTask = NULL;
	WebServChannel  *ChannelPtr = NULL;

	ChannelPtr = &gWebServMsgChannel;
#ifdef WIN32
    if (WaitForSingleObject(ChannelPtr->WebServMsgAccess, INFINITE) == WAIT_FAILED)
	{
		printf("WebServMsgApiClose - Fail to get access mutex\r\n");
        return;
	}
#else
    pthread_mutex_lock(&ChannelPtr->WebServMsgAccess);
#endif
    while (ChannelPtr->ListWebServMsg.Count)
    {
        PointTask = (ObjPoolListTask*)ChannelPtr->ListWebServMsg.FistTask;
        WebServMsgPtr = (WebServMessage*)PointTask->UsedTask;
        RemPoolStructList(&ChannelPtr->ListWebServMsg, PointTask);
    }
#ifdef WIN32
    if (!ReleaseMutex(ChannelPtr->WebServMsgAccess)) 
        printf("WebServMsgApiClose - Fail to release memory mutex\r\n");
#else
    pthread_mutex_unlock(&ChannelPtr->WebServMsgAccess);
#endif
    DestroyPoolListStructs(&ChannelPtr->ListWebServMsg);
    DestroyPool(&ChannelPtr->WebServMsgPool);
    WebServMsgQueueDestroy(&ChannelPtr->WebServQueue);
#ifdef WIN32
	CloseHandle(ChannelPtr->WebServMsgAccess);
#else
    pthread_mutex_destroy(&ChannelPtr->WebServMsgAccess);
#endif
}
//---------------------------------------------------------------------------
void OnWebServerTimerExp(unsigned int TimerId)
{
    WebServMessage *WebServMsgPtr = NULL;

	WebServMsgPtr = GetWebServMsgPool(&gWebServMsgChannel, gWebServMsgChannel.TimerExpMsgId);
    if (!WebServMsgPtr) return;
	WebServMsgPtr->Msg.WParam = (void*)(tulong)TimerId;
    WebServMsgQueuePost(&gWebServMsgChannel.WebServQueue, WebServMsgPtr);
}
//---------------------------------------------------------------------------
void WebServerMsgSent(unsigned int MsgTag, void *WParam, void *LParam)
{
    WebServMessage *WebServMsgPtr = NULL;

	WebServMsgPtr = GetWebServMsgPool(&gWebServMsgChannel, MsgTag);
    if (!WebServMsgPtr) return;
	WebServMsgPtr->Msg.WParam = WParam;
	WebServMsgPtr->Msg.LParam = LParam;
    WebServMsgQueuePost(&gWebServMsgChannel.WebServQueue, WebServMsgPtr);
}
//---------------------------------------------------------------------------
