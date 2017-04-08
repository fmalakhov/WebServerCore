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

#include "RemConnMsgApi.h"
#include "ThrReportMen.h"

extern char ThrUserRemConnName[];
//---------------------------------------------------------------------------
void RemConnMsgQueueCreate(RemConnMsgQueue *MsgQueuePtr)
{
	DebugLogPrint(NULL, "%s: Initializing WEB sender channel messages queue.\n", ThrUserRemConnName);
#ifdef WIN32
	MsgQueuePtr->mutex = CreateMutex(NULL, FALSE, NULL);
    if (MsgQueuePtr->mutex == NULL) 
    {
        printf("Create Rem conn. msg queue mutex error: %d\r\n", GetLastError());
	    exit(EXIT_FAILURE);
    }
#else
    pthread_mutex_init(&(MsgQueuePtr->mutex), NULL);
    pthread_cond_init(&(MsgQueuePtr->cond_var), NULL);
#endif
    PoolListInit(&MsgQueuePtr->queue_list, INIT_REM_CONN_MSG_COUNT);
    MsgQueuePtr->destroy = false;
}
//---------------------------------------------------------------------------
void RemConnMsgQueueMarkDestroy(RemConnMsgQueue *MsgQueuePtr)
{
	DebugLogPrint(NULL, "%s: Marking Rem conn. message queue for destroy.\n", ThrUserRemConnName);
    MsgQueuePtr->destroy = true;
    RemConnMsgQueuePost(MsgQueuePtr, NULL);
}
//---------------------------------------------------------------------------
void RemConnMsgQueuePost(RemConnMsgQueue *MsgQueuePtr, void *p_message)
{
#ifdef WIN32
    unsigned int CurrUsage;

    if (WaitForSingleObject(MsgQueuePtr->mutex, INFINITE) == WAIT_FAILED)
	{
		printf("Fail to get mutex (RemConnMsgQueuePost)\r\n");
        return;
	}
	CurrUsage = (unsigned int)MsgQueuePtr->queue_list.Count;
	AddPoolStructList(&MsgQueuePtr->queue_list, p_message);
    if (!ReleaseMutex(MsgQueuePtr->mutex)) 
        printf("Fail to release mutex (RemConnMsgQueuePost)\r\n");
	if (!CurrUsage) WinThreadMsgSend(MsgQueuePtr->ThrSendWeb, TRC_MSG_NOTIFY, 0, 0);
#else
    pthread_mutex_lock(&(MsgQueuePtr->mutex));
    AddPoolStructList(&MsgQueuePtr->queue_list, p_message);
    pthread_mutex_unlock(&(MsgQueuePtr->mutex));
    pthread_cond_signal(&(MsgQueuePtr->cond_var));
#endif
}
//---------------------------------------------------------------------------
void RemConnMsgQueueDestroy(RemConnMsgQueue *MsgQueuePtr)
{
	DebugLogPrint(NULL, "%s: De-initializing Rem conn. messages queue.\n", ThrUserRemConnName);
#ifdef WIN32
	CloseHandle(MsgQueuePtr->mutex);
#else
    pthread_mutex_destroy(&(MsgQueuePtr->mutex));
    pthread_cond_destroy(&(MsgQueuePtr->cond_var));
#endif
    DestroyPoolListStructs(&MsgQueuePtr->queue_list);
}
//---------------------------------------------------------------------------
bool GetRemConnMessageQueue(RemConnMsgQueue *MsgQueuePtr, void **pp_message)
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
		printf("Fail to get mutex (1) (GetRemConnMessageQueue)\r\n");
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
                printf("Fail to release mutex (GetRemConnMessageQueue)\r\n");
			GetMessage(&SysMsg, NULL, TRC_MSG_NOTIFY, TRC_MSG_NOTIFY);
#else
            pthread_cond_wait(&(MsgQueuePtr->cond_var), &(MsgQueuePtr->mutex));
#endif
#ifdef WIN32
            if (WaitForSingleObject(MsgQueuePtr->mutex, INFINITE) == WAIT_FAILED)
	        {
		        printf("Fail to get mutex (2) (GetRemConnMessageQueue)\r\n");
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
		DebugLogPrint(NULL, "%s: Wokeup Rem conn. due to destroy flag.\n", ThrUserRemConnName);
        status = false;
    }
#ifdef WIN32
    if (!ReleaseMutex(MsgQueuePtr->mutex)) 
        printf("Fail to release mutex (GetRemConnMessageQueue)\r\n");
#else
    pthread_mutex_unlock(&(MsgQueuePtr->mutex));
#endif
    return status;
}
//---------------------------------------------------------------------------
RemConnMessage* GetRemConnMsgPool(RemConnChannel *ChannelPtr, unsigned char MsgTag)
{
    RemConnMessage   *RemConnMsgPtr = NULL;
    POOL_RECORD_STRUCT *ObjTaskPtr = NULL;

#ifdef WIN32
    if (WaitForSingleObject(ChannelPtr->RemConnMsgAccess, INFINITE) == WAIT_FAILED) return NULL;
#else
    pthread_mutex_lock(&ChannelPtr->RemConnMsgAccess);
#endif
    ObjTaskPtr = GetBuffer(&ChannelPtr->RemConnMsgPool);
    if (!ObjTaskPtr)
    {
		DebugLogPrint(NULL, "%s: No buffers for rem. connect message delivery.\n", ThrUserRemConnName);
#ifdef WIN32
        if (!ReleaseMutex(ChannelPtr->RemConnMsgAccess)) 
            printf("Fail to release mutex (GetRemConnMsgPool)\r\n");
#else
        pthread_mutex_unlock(&ChannelPtr->RemConnMsgAccess);
#endif
        return NULL;
    }
    RemConnMsgPtr = (RemConnMessage*)ObjTaskPtr->DataPtr;
    RemConnMsgPtr->MsgTag = MsgTag;
    RemConnMsgPtr->BlkPoolPtr = ObjTaskPtr;
    RemConnMsgPtr->ObjPtr = AddPoolStructListObj(&ChannelPtr->ListRemConnMsg, RemConnMsgPtr);
    if (ChannelPtr->MaxSimultRemConnMsgs < (unsigned int)ChannelPtr->ListRemConnMsg.Count)
        ChannelPtr->MaxSimultRemConnMsgs = (unsigned int)ChannelPtr->ListRemConnMsg.Count;
#ifdef WIN32
    if (!ReleaseMutex(ChannelPtr->RemConnMsgAccess)) 
        printf("Fail to release mutex (GetRemConnMsgPool)\r\n");
#else
    pthread_mutex_unlock(&ChannelPtr->RemConnMsgAccess);
#endif
    return RemConnMsgPtr;
}
//---------------------------------------------------------------------------
void FreeRemConnMsgPool(RemConnChannel *ChannelPtr, RemConnMessage *RemConnMsgPtr)
{
#ifdef WIN32
    if (WaitForSingleObject(ChannelPtr->RemConnMsgAccess, INFINITE) == WAIT_FAILED)
	{
        return;
	}
#else
    pthread_mutex_lock(&ChannelPtr->RemConnMsgAccess);
#endif
    FreeBuffer(&ChannelPtr->RemConnMsgPool, RemConnMsgPtr->BlkPoolPtr);
    RemPoolStructList(&ChannelPtr->ListRemConnMsg, RemConnMsgPtr->ObjPtr);
#ifdef WIN32
    if (!ReleaseMutex(ChannelPtr->RemConnMsgAccess)) 
        printf("Fail to release mutex (FreeRemConnMsgPool)\r\n");
#else
    pthread_mutex_unlock(&ChannelPtr->RemConnMsgAccess);
#endif
}
//---------------------------------------------------------------------------
void RemConnMsgApiInit(RemConnChannel *ChannelPtr)
{
	memset(ChannelPtr, 0, sizeof(RemConnChannel));
    PoolListInit(&ChannelPtr->ListRemConnMsg, INIT_REM_CONN_BLK_COUNT);
    CreatePool(&ChannelPtr->RemConnMsgPool, INIT_REM_CONN_BLK_COUNT, sizeof(RemConnMessage));
#ifdef _LINUX_X86_
    pthread_mutex_init(&ChannelPtr->RemConnMsgAccess, NULL);
#endif
    RemConnMsgQueueCreate(&ChannelPtr->RemConnQueue);
}
//---------------------------------------------------------------------------
void RemConnMsgApiClose(RemConnChannel *ChannelPtr)
{
    RemConnMessage     *RemConnMsgPtr = NULL;
    ObjPoolListTask    *PointTask = NULL;

#ifdef WIN32
    if (WaitForSingleObject(ChannelPtr->RemConnMsgAccess, INFINITE) == WAIT_FAILED)
	{
		printf("RemConnMsgApiClose - Fail to get access mutex\r\n");
        return;
	}
#else
    pthread_mutex_lock(&ChannelPtr->RemConnMsgAccess);
    Sleep(20);
#endif

    while (ChannelPtr->ListRemConnMsg.Count)
    {
        PointTask = (ObjPoolListTask*)ChannelPtr->ListRemConnMsg.FistTask;
        RemConnMsgPtr = (RemConnMessage*)PointTask->UsedTask;
        RemPoolStructList(&ChannelPtr->ListRemConnMsg, PointTask);
    }

#ifdef WIN32
    if (!ReleaseMutex(ChannelPtr->RemConnMsgAccess)) 
        printf("RemConnMsgApiClose - Fail to release memory mutex\r\n");
#else
    pthread_mutex_unlock(&ChannelPtr->RemConnMsgAccess);
#endif
    DestroyPoolListStructs(&ChannelPtr->ListRemConnMsg);
    DestroyPool(&ChannelPtr->RemConnMsgPool);
    RemConnMsgQueueDestroy(&ChannelPtr->RemConnQueue);
#ifdef WIN32
	CloseHandle(ChannelPtr->RemConnMsgAccess);
#else
    pthread_mutex_destroy(&ChannelPtr->RemConnMsgAccess);
#endif
}
//---------------------------------------------------------------------------
