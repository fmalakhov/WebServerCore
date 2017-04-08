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

#include "ThrWebMgmt.h"

extern unsigned char gMemoryUtil;

static char ThrWebMgmtName[] = "ThrWebMgmt";
static char TempDataPath[] = "";

static WebMgmtChannel WebMgmtChan;

static void WebMgmtMsgQueueCreate(WebMgmtMsgQueue *p_msg_queue);
static void WebMgmtMsgQueueDestroy(WebMgmtMsgQueue *p_msg_queue);
static void WebMgmtMsgQueuePost(WebMgmtMsgQueue *p_msg_queue, void* p_message);
static bool GetWebMgmtMessageQueue(WebMgmtMsgQueue *p_msg_queue, void **pp_message);
static void WebMgmtMsgQueueMarkDestroy(WebMgmtMsgQueue *p_msg_queue);
static WebMgmtMessage* GetWebMgmtMsgPool(WebMgmtChannel *ChannelPtr, unsigned char MsgTag);
static void FreeWebMgmtMsgPool(WebMgmtChannel *ChannelPtr, WebMgmtMessage *WebMgmtMsgPtr);
static void HandleVoiceBaseCheck();
static void OnWebCpuMeasureTimerExp(unsigned int TimerId);
static void OnIpCntClrTimerExp(unsigned int TimerId);
static void OnIpAccChkTimerExp(unsigned int TimerId);
//---------------------------------------------------------------------------
void WebMgmtsInit(PARAMWEBSERV *ParWebServPtr, TOnGetTimeMarkerCB TimeMarkerCB)
{
	memset(&WebMgmtChan, 0, sizeof(WebMgmtChannel));      
	if (WebMgmtThreadCreate(&WebMgmtChan) == 0)
	{
	    printf("System WEB manager is started\n");
		IpAccessControlImit(ParWebServPtr->ServCustomCfg.DDosDetectTreshold, 
		    ParWebServPtr->ServCustomCfg.DDosIpLockTime,
			ParWebServPtr->ServCustomCfg.DDosIpFreeTime,
			TimeMarkerCB);
    }
}
//---------------------------------------------------------------------------
void WebMgmtsClose()
{
	WebMgmtThreadClose(&WebMgmtChan);
	IpAccessControlClose();
	printf("System WEB manager was stopped\n");
}
//--------------------------------------------------------------------------
unsigned int MaxWebMgmtQueueUseGet()
{
    return WebMgmtChan.MaxSimultWebMgmtMsgs;
}
//---------------------------------------------------------------------------
static void WebMgmtMsgQueueCreate(WebMgmtMsgQueue *MsgQueuePtr)
{
	DebugLogPrint(NULL, "%s: Initializing system WEB manager messages queue.\n", ThrWebMgmtName);
#ifdef WIN32
	MsgQueuePtr->mutex = CreateMutex(NULL, FALSE, NULL);
    if (MsgQueuePtr->mutex == NULL) 
    {
        printf("Create system WEB manager msg queue mutex error: %d\r\n", GetLastError());
	    exit(EXIT_FAILURE);
    }
#else
    pthread_mutex_init(&(MsgQueuePtr->mutex), NULL);
    pthread_cond_init(&(MsgQueuePtr->cond_var), NULL);
#endif
    PoolListInit(&MsgQueuePtr->queue_list, INIT_WEB_MGMT_MSG_COUNT);
    MsgQueuePtr->destroy = false;
}
//---------------------------------------------------------------------------
static void WebMgmtMsgQueueMarkDestroy(WebMgmtMsgQueue *MsgQueuePtr)
{
	DebugLogPrint(NULL, "%s: Marking system WEB manager message queue for destroy.\n", ThrWebMgmtName);
    MsgQueuePtr->destroy = true;
    WebMgmtMsgQueuePost(MsgQueuePtr, NULL);
}
//---------------------------------------------------------------------------
static void WebMgmtMsgQueuePost(WebMgmtMsgQueue *MsgQueuePtr, void *p_message)
{
#ifdef WIN32
    unsigned int CurrUsage;

    if (WaitForSingleObject(MsgQueuePtr->mutex, INFINITE) == WAIT_FAILED)
	{
		printf("Fail to get mutex (WebMgmtMsgQueuePost)\r\n");
        return;
	}
	CurrUsage = (unsigned int)MsgQueuePtr->queue_list.Count;
	AddPoolStructList(&MsgQueuePtr->queue_list, p_message);
    if (!ReleaseMutex(MsgQueuePtr->mutex)) 
        printf("Fail to release mutex (WebMgmtMsgQueuePost)\r\n");
	if (!CurrUsage) WinThreadMsgSend(MsgQueuePtr->ThrWebMgmt, TWM_MSG_NOTIFY, 0, 0);
#else
    pthread_mutex_lock(&(MsgQueuePtr->mutex));
    AddPoolStructList(&MsgQueuePtr->queue_list, p_message);
    pthread_mutex_unlock(&(MsgQueuePtr->mutex));
    pthread_cond_signal(&(MsgQueuePtr->cond_var));
#endif
}
//---------------------------------------------------------------------------
static void WebMgmtMsgQueueDestroy(WebMgmtMsgQueue *MsgQueuePtr)
{
	DebugLogPrint(NULL, "%s: De-initializing system WEB manager messages queue.\n", ThrWebMgmtName);
#ifdef WIN32
	CloseHandle(MsgQueuePtr->mutex);
#else
    pthread_mutex_destroy(&(MsgQueuePtr->mutex));
    pthread_cond_destroy(&(MsgQueuePtr->cond_var));
#endif
    DestroyPoolListStructs(&MsgQueuePtr->queue_list);
}
//---------------------------------------------------------------------------
static bool GetWebMgmtMessageQueue(WebMgmtMsgQueue *MsgQueuePtr, void **pp_message)
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
		printf("Fail to get mutex (1) (GetWebMgmtMessageQueue)\r\n");
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
                printf("Fail to release mutex (GetWebMgmtMessageQueue)\r\n");
			GetMessage(&SysMsg, NULL, TWM_MSG_NOTIFY, TWM_MSG_NOTIFY);
#else
            pthread_cond_wait(&(MsgQueuePtr->cond_var), &(MsgQueuePtr->mutex));
#endif
#ifdef WIN32
            if (WaitForSingleObject(MsgQueuePtr->mutex, INFINITE) == WAIT_FAILED)
	        {
		        printf("Fail to get mutex (2) (GetWebMgmtMessageQueue)\r\n");
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
		DebugLogPrint(NULL, "%s: Wokeup system WEB manager due to destroy flag.\n", ThrWebMgmtName);
        status = false;
    }
#ifdef WIN32
    if (!ReleaseMutex(MsgQueuePtr->mutex)) 
        printf("Fail to release mutex (GetWebMgmtMessageQueue)\r\n");
#else
    pthread_mutex_unlock(&(MsgQueuePtr->mutex));
#endif
    return status;
}
//---------------------------------------------------------------------------
static WebMgmtMessage* GetWebMgmtMsgPool(WebMgmtChannel *ChannelPtr, unsigned char MsgTag)
{
    WebMgmtMessage   *WebMgmtMsgPtr = NULL;
    POOL_RECORD_STRUCT *ObjTaskPtr = NULL;

#ifdef WIN32
    if (WaitForSingleObject(ChannelPtr->WebMgmtMsgAccess, INFINITE) == WAIT_FAILED) return NULL;
#else
    pthread_mutex_lock(&ChannelPtr->WebMgmtMsgAccess);
#endif
    ObjTaskPtr = GetBuffer(&ChannelPtr->WebMgmtMsgPool);
    if (!ObjTaskPtr)
    {
		DebugLogPrint(NULL, "%s: No buffers for system WEB manageer message delivery.\n", ThrWebMgmtName);
#ifdef WIN32
        if (!ReleaseMutex(ChannelPtr->WebMgmtMsgAccess)) 
            printf("Fail to release mutex (GetWebMgmtMsgPool)\r\n");
#else
        pthread_mutex_unlock(&ChannelPtr->WebMgmtMsgAccess);
#endif
        return NULL;
    }
    WebMgmtMsgPtr = (WebMgmtMessage*)ObjTaskPtr->DataPtr;
    WebMgmtMsgPtr->MsgTag = MsgTag;
    WebMgmtMsgPtr->BlkPoolPtr = ObjTaskPtr;
    WebMgmtMsgPtr->ObjPtr = AddPoolStructListObj(&ChannelPtr->ListWebMgmtMsg, WebMgmtMsgPtr);
    if (ChannelPtr->MaxSimultWebMgmtMsgs < (unsigned int)ChannelPtr->ListWebMgmtMsg.Count)
        ChannelPtr->MaxSimultWebMgmtMsgs = (unsigned int)ChannelPtr->ListWebMgmtMsg.Count;
#ifdef WIN32
    if (!ReleaseMutex(ChannelPtr->WebMgmtMsgAccess)) 
        printf("Fail to release mutex (GetWebMgmtMsgPool)\r\n");
#else
    pthread_mutex_unlock(&ChannelPtr->WebMgmtMsgAccess);
#endif
    return WebMgmtMsgPtr;
}
//---------------------------------------------------------------------------
static void FreeWebMgmtMsgPool(WebMgmtChannel *ChannelPtr, WebMgmtMessage *WebMgmtMsgPtr)
{
#ifdef WIN32
    if (WaitForSingleObject(ChannelPtr->WebMgmtMsgAccess, INFINITE) == WAIT_FAILED)
	{
        return;
	}
#else
    pthread_mutex_lock(&ChannelPtr->WebMgmtMsgAccess);
#endif
    FreeBuffer(&ChannelPtr->WebMgmtMsgPool, WebMgmtMsgPtr->BlkPoolPtr);
    RemPoolStructList(&ChannelPtr->ListWebMgmtMsg, WebMgmtMsgPtr->ObjPtr);
#ifdef WIN32
    if (!ReleaseMutex(ChannelPtr->WebMgmtMsgAccess)) 
        printf("Fail to release mutex (FreeWebMgmtMsgPool)\r\n");
#else
    pthread_mutex_unlock(&ChannelPtr->WebMgmtMsgAccess);
#endif
}
//---------------------------------------------------------------------------
int WebMgmtThreadCreate(WebMgmtChannel *ChannelPtr)
{
    int                Res = 0;
#ifdef WIN32
	struct tagMSG      SysMsg;
#else
    pthread_attr_t     attr, *attrPtr = &attr;
    struct sched_param sched;
#endif

    PoolListInit(&ChannelPtr->ListWebMgmtMsg, INIT_WEB_MGMT_BLK_COUNT);
    CreatePool(&ChannelPtr->WebMgmtMsgPool, INIT_WEB_MGMT_BLK_COUNT, sizeof(WebMgmtMessage));
#ifdef WIN32
    ChannelPtr->WebMgmtMsgAccess = CreateMutex(NULL, FALSE, NULL);
    if (ChannelPtr->WebMgmtMsgAccess == NULL) 
    {
        printf("Create WebMgmt manager access mutex error: %d\r\n", GetLastError());
	    Res = -1;
    }
	else
	{
        WebMgmtMsgQueueCreate(&ChannelPtr->WebMgmtQueue);
		ChannelPtr->WebMgmtInfo.StopWebMgmtFlag = false;
		ChannelPtr->WebMgmtInfo.isStartDone = false;
	    ChannelPtr->WebMgmtInfo.ThrWEBMGR = 0;
        ChannelPtr->WebMgmtInfo.HTRWEBMGR = 0;
        ChannelPtr->WebMgmtInfo.ThrAnswStart = GetCurrentThreadId();
        ChannelPtr->WebMgmtInfo.IDAnswStart = TWM_WEBMGR_RESULT_START;
        ChannelPtr->WebMgmtInfo.HTRWEBMGR = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)WebMgmtHandlerThread,
                            (LPVOID)ChannelPtr, 0, (LPDWORD)&ChannelPtr->WebMgmtInfo.ThrWEBMGR);
		ChannelPtr->WebMgmtQueue.ThrWebMgmt = (DWORD)ChannelPtr->WebMgmtInfo.ThrWEBMGR;
        GetMessage(&SysMsg, NULL, TWM_WEBMGR_RESULT_START, TWM_WEBMGR_RESULT_START);
        if (SysMsg.wParam != 1)
        {
	        printf("Error start TLS worker thread. Error:[%d]\r\n",(unsigned)SysMsg.lParam);
		    Res = -1;
        }
	    if (!ChannelPtr->WebMgmtInfo.isStartDone) Res = -1;
	}
#else
	sem_init(&ChannelPtr->WebMgmtInfo.WebMgmtSem, 0, 0);
    pthread_mutex_init(&ChannelPtr->WebMgmtMsgAccess, NULL);
    WebMgmtMsgQueueCreate(&ChannelPtr->WebMgmtQueue);
	ChannelPtr->WebMgmtInfo.isStartDone = false;
    ChannelPtr->WebMgmtInfo.StopWebMgmtFlag = false;
    pthread_attr_init(attrPtr);
    pthread_attr_setdetachstate(attrPtr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(attrPtr, PTHREAD_SCOPE_SYSTEM);
    if (pthread_attr_getschedparam(attrPtr, &sched) == 0)
    {
        sched.sched_priority = 0;
        pthread_attr_setschedparam(attrPtr, &sched);
    }
    DebugLogPrint(NULL, "%s: WebMgmt thread sturtup\n", ThrWebMgmtName);
    if (pthread_create(&ChannelPtr->WebMgmtInfo.ThreadId, &attr, &WebMgmtHandlerThread, ChannelPtr) == -1)
    {
        DebugLogPrint(NULL, "%s: WebMgmt thread create error\n", ThrWebMgmtName);
        Res = -1;
    }
    else
    {
        sem_wait(&ChannelPtr->WebMgmtInfo.WebMgmtSem);
		if (!ChannelPtr->WebMgmtInfo.isStartDone) Res = -1;
		sem_post(&ChannelPtr->WebMgmtInfo.WebMgmtSem);
    }
#endif
    return Res;
}
//---------------------------------------------------------------------------
void WebMgmtThreadClose(WebMgmtChannel *ChannelPtr)
{
    WebMgmtMessage *WebMgmtMsgPtr = NULL;
    ObjPoolListTask  *PointTask = NULL;

    ChannelPtr->WebMgmtInfo.StopWebMgmtFlag = true;
    WebMgmtMsgQueueMarkDestroy(&ChannelPtr->WebMgmtQueue);
#ifdef WIN32
    if (ChannelPtr->WebMgmtInfo.ThrWEBMGR)
    {
        WaitCloseProcess(ChannelPtr->WebMgmtInfo.HTRWEBMGR);
        CloseHandle(ChannelPtr->WebMgmtInfo.HTRWEBMGR);
    }
    ChannelPtr->WebMgmtInfo.ThrWEBMGR = 0;
    ChannelPtr->WebMgmtInfo.HTRWEBMGR = 0;
    SleepMs(100);
    if (WaitForSingleObject(ChannelPtr->WebMgmtMsgAccess, INFINITE) == WAIT_FAILED)
	{
		printf("WebMgmtThreadClose - Fail to get access mutex\r\n");
        return;
	}
#else        
    pthread_join(ChannelPtr->WebMgmtInfo.ThreadId, NULL);
    SleepMs(100);
    pthread_mutex_lock(&ChannelPtr->WebMgmtMsgAccess);
#endif
    while (ChannelPtr->ListWebMgmtMsg.Count)
    {
        PointTask = (ObjPoolListTask*)ChannelPtr->ListWebMgmtMsg.FistTask;
        WebMgmtMsgPtr = (WebMgmtMessage*)PointTask->UsedTask;
        RemPoolStructList(&ChannelPtr->ListWebMgmtMsg, PointTask);
    }
#ifdef WIN32
    if (!ReleaseMutex(ChannelPtr->WebMgmtMsgAccess)) 
        printf("WebMgmtThreadClose - Fail to release memory mutex\r\n");
#else
    pthread_mutex_unlock(&ChannelPtr->WebMgmtMsgAccess);
#endif
    DestroyPoolListStructs(&ChannelPtr->ListWebMgmtMsg);
    DestroyPool(&ChannelPtr->WebMgmtMsgPool);
    WebMgmtMsgQueueDestroy(&ChannelPtr->WebMgmtQueue);
#ifdef WIN32
	CloseHandle(ChannelPtr->WebMgmtMsgAccess);
#else
    pthread_mutex_destroy(&ChannelPtr->WebMgmtMsgAccess);
	sem_destroy(&ChannelPtr->WebMgmtInfo.WebMgmtSem);
#endif
    DebugLogPrint(NULL, "%s: WebMgmt close is completed\n", ThrWebMgmtName);
    return;   
}
//---------------------------------------------------------------------------
#ifdef WIN32
static DWORD WINAPI WebMgmtHandlerThread(LPVOID Data)
#else
static void* WebMgmtHandlerThread(void *Data)
#endif
{
    WebMgmtMessage    *WebMgmtMsgPtr = NULL;
	bool              MsgSentResult = false;	
	unsigned char     NewMemUsage = 0;
	WebMgmtTaskInfo   *ThrInfoPtr;    
	WebMgmtChannel    *ChannelPtr;

	ChannelPtr = (WebMgmtChannel*)Data;
	ThrInfoPtr = &ChannelPtr->WebMgmtInfo;
    ThrInfoPtr->StopWebMgmtFlag = false; 
	ThrInfoPtr->isStartDone = true;
	CreateThreadTimerCB(OnWebCpuMeasureTimerExp, WEB_MGMT_ACCESS_ID, 15*TMR_ONE_SEC, CPU_UTIL_MEASURE_TMR_ID, true);
    CreateThreadTimerCB(OnIpCntClrTimerExp, WEB_MGMT_ACCESS_ID, TMR_ONE_SEC, IP_CNT_CLR_TMR_ID, true);
    CreateThreadTimerCB(OnIpAccChkTimerExp, WEB_MGMT_ACCESS_ID, 30*TMR_ONE_SEC, IP_ACC_CHECK_TMR_ID, true);
	ServerCpuUtilInit(TempDataPath);
#ifdef WIN32
	WinThreadMsgSend(ThrInfoPtr->ThrAnswStart, ThrInfoPtr->IDAnswStart, 1, 0);
#else
	sem_post(&ThrInfoPtr->WebMgmtSem);
#endif
	DebugLogPrint(NULL, "%s: WebMgmt is ready to process requests\n", ThrWebMgmtName);
    /* Read the data from the system manager messages queue. */
    while (GetWebMgmtMessageQueue(&ChannelPtr->WebMgmtQueue, (void**)&WebMgmtMsgPtr))
    {    
	    if(ThrInfoPtr->StopWebMgmtFlag)
        {
            printf("%s WebMgmt's thread stop was requested\n", SetTimeStampLine());
            break;
        }   
	
        switch(WebMgmtMsgPtr->MsgTag)
        {
			case WMP_CPU_MEASURE_REQ:
				HandleCpuUtilMeasureTimerExp();
				if (!GetServMemUsage(&NewMemUsage, TempDataPath, true))
					 DebugLogPrint(NULL, "%s: Memory usage measure is failed\n", ThrWebMgmtName);
				else gMemoryUtil = NewMemUsage;
				break;

			case WMP_IP_CNT_CLR_REQ:
			    OnIpAccessCountClearTmrExp();
			    break;
			
			case WMP_IP_ACC_CHECK_REQ:
			    OnActiveIpAccessListCheckTmrExp();
			    break;
			
            default:
                DebugLogPrint(NULL, "%s: Unexpected(0x%02X) message tag is received by system manager\n", 
					ThrWebMgmtName, WebMgmtMsgPtr->MsgTag);
                break;
        }
        FreeWebMgmtMsgPool(ChannelPtr, WebMgmtMsgPtr);
    }   
	CloseThreadTimer(WEB_MGMT_ACCESS_ID, CPU_UTIL_MEASURE_TMR_ID);
    CloseThreadTimer(WEB_MGMT_ACCESS_ID, IP_CNT_CLR_TMR_ID);
    CloseThreadTimer(WEB_MGMT_ACCESS_ID, IP_ACC_CHECK_TMR_ID);
    printf("WebMgmt thread was stoped\n");      
#ifdef WIN32
    ExitThread(0);
#else
    pthread_exit((void *)0);
#endif
}
//---------------------------------------------------------------------------
static void OnWebCpuMeasureTimerExp(unsigned int TimerId)
{
    WebMgmtMessage *WebMgmtMsgPtr = NULL;

    WebMgmtMsgPtr = GetWebMgmtMsgPool(&WebMgmtChan, WMP_CPU_MEASURE_REQ);
    if (!WebMgmtMsgPtr) return;         
    WebMgmtMsgQueuePost(&WebMgmtChan.WebMgmtQueue, WebMgmtMsgPtr);
}
//---------------------------------------------------------------------------
static void OnIpCntClrTimerExp(unsigned int TimerId)
{
    WebMgmtMessage *WebMgmtMsgPtr = NULL;

    WebMgmtMsgPtr = GetWebMgmtMsgPool(&WebMgmtChan, WMP_IP_CNT_CLR_REQ);
    if (!WebMgmtMsgPtr) return;         
    WebMgmtMsgQueuePost(&WebMgmtChan.WebMgmtQueue, WebMgmtMsgPtr);
}
//---------------------------------------------------------------------------
static void OnIpAccChkTimerExp(unsigned int TimerId)
{
    WebMgmtMessage *WebMgmtMsgPtr = NULL;

    WebMgmtMsgPtr = GetWebMgmtMsgPool(&WebMgmtChan, WMP_IP_ACC_CHECK_REQ);
    if (!WebMgmtMsgPtr) return;         
    WebMgmtMsgQueuePost(&WebMgmtChan.WebMgmtQueue, WebMgmtMsgPtr);
}
//---------------------------------------------------------------------------
