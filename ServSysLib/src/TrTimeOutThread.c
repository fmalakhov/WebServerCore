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

#include "TrTimeOutThread.h"
#include "SysMessages.h"

#ifdef WIN32
extern HANDLE gFileMutex;
#endif

static TimerTaskInfo    gTimerInfo;
static TimerMsgQueue    g_timer_queue;
#ifdef WIN32
static HANDLE           g_timer_lock;
static HANDLE           gTimerMsgAccess;
#else
static pthread_mutex_t  g_timer_lock;
static pthread_mutex_t  gTimerMsgAccess;
#endif
static PoolListItsTask  gListTimerMsg;
static POOL_RECORD_BASE gTimerMsgPool;
static unsigned int     gMaxSimultActTimers = 0;
static unsigned int     gMaxSimultTimerMsgs = 0;

#define INIT_TIMER_BLK_COUNT  512  /* Start number of timers in pool */
#define INIT_TIMER_MSG_COUNT  512  /* Start number of timer messags in pool */

/* Timer related message tags */
#define TMR_TICK_EVENT        0x01
#define TMR_START_TIMER       0x02
#define TMR_RESET_TIMER       0x03
#define TMR_RESTART_TIMER     0x04
#define TMR_RESET_GRP_TIMER   0x05

#define	TICK_SELECT_TIME	      50	/* Time in ms */
#define TIMER_TX_BUFFER_SIZE      128000
#define TIMER_RX_BUFFER_SIZE      16000
#define TIMER_STATUS_CHEC_TIMOUT  (15*TMR_ONE_MIN)
#define TIMER_STATUS_REQ_TIMER_ID 10001

static TimerThrPar TimerPar;

static void TimerMsgQueueCreate(TimerMsgQueue *p_msg_queue);
static void TimerMsgQueueDestroy(TimerMsgQueue *p_msg_queue);
static void TimerMsgQueuePost(TimerMsgQueue *p_msg_queue, void* p_message);
static bool GetTimerMessageQueue(TimerMsgQueue *p_msg_queue, void **pp_message);
static void TimerMsgQueueMarkDestroy(TimerMsgQueue *p_msg_queue);

static int TickThreadInit(TickTaskInfo *TickInfoPtr);
static void TickThreadClose(TickTaskInfo *TickInfoPtr);
static void* TimerTickThread(void* arg);
static void HandleStartTimerReq(TimerThrPar *TimerParPtr, TimerMessage *TimerMsgPtr);
static void HandleResetTimerReq(TimerThrPar *TimerParPtr, TimerMessage *TimerMsgPtr);
static void HandleRestartTimerReq(TimerThrPar *TimerParPtr, TimerMessage *TimerMsgPtr);
static void TimerRecordRemove(TimerThrPar *TimerParPtr, TimerRecord *TimerPtr);
static TimerRecord* GetTimerRecordByTimerId(TimerThrPar *TimerParPtr, TimerMessage *TimerMsgPtr);
static void HandleRestartGrpTimerReq(TimerThrPar *TimerParPtr, TimerMessage *TimerMsgPtr);
static void HandleTimerTick(TimerThrPar *TimerParPtr);
static bool SetTimerActiveList(TimerThrPar *TimerPar, TimerRecord *Timer);
static void TimerStatusShow(TimerThrPar *TimerParPtr);
static void TimerDebugLogPrint(const char* format, ... );

#ifdef WIN32
static DWORD WINAPI TimerHandlerThread(LPVOID Data);
#else
static void* TimerHandlerThread(void *arg);
#endif
//---------------------------------------------------------------------------
static void TimerMsgQueueCreate(TimerMsgQueue *MsgQueuePtr)
{
    TimerDebugLogPrint("Initializing timer messages queue.");
#ifdef WIN32
	MsgQueuePtr->mutex = CreateMutex(NULL, FALSE, NULL);
    if (MsgQueuePtr->mutex == NULL) 
    {
        printf("Create timer msg queue mutex error: %d\r\n", GetLastError());
	    exit(EXIT_FAILURE);
    }
#else
    pthread_mutex_init(&(MsgQueuePtr->mutex), NULL);
    pthread_cond_init(&(MsgQueuePtr->cond_var), NULL);
#endif
    PoolListInit(&MsgQueuePtr->queue_list, INIT_TIMER_MSG_COUNT);
    MsgQueuePtr->destroy = false;
}
//---------------------------------------------------------------------------
static void TimerMsgQueueMarkDestroy(TimerMsgQueue *MsgQueuePtr)
{
    TimerDebugLogPrint("marking timer message queue for destroy.");
    MsgQueuePtr->destroy = true;
    TimerMsgQueuePost(MsgQueuePtr, NULL);
}
//---------------------------------------------------------------------------
static void TimerMsgQueuePost(TimerMsgQueue *MsgQueuePtr, void *p_message)
{
#ifdef WIN32
	bool        isRMFreeReady = false;

    if (WaitForSingleObject(MsgQueuePtr->mutex, INFINITE) == WAIT_FAILED)
	{
		printf("Fail to get mutex (TimerMsgQueuePost)\r\n");
        return;
	}
	AddPoolStructList(&MsgQueuePtr->queue_list, p_message);
    if (!ReleaseMutex(MsgQueuePtr->mutex)) 
        printf("Fail to release mutex (TimerMsgQueuePost)\r\n");
	WinThreadMsgSend(MsgQueuePtr->ThrTimer, TMU_REPORTDATA, 0, 0);
#else
    pthread_mutex_lock(&(MsgQueuePtr->mutex));
    AddPoolStructList(&MsgQueuePtr->queue_list, p_message);
    pthread_mutex_unlock(&(MsgQueuePtr->mutex));
    pthread_cond_signal(&(MsgQueuePtr->cond_var));
#endif
}
//---------------------------------------------------------------------------
static void TimerMsgQueueDestroy(TimerMsgQueue *MsgQueuePtr)
{
    TimerDebugLogPrint("De-initializing timer messages queue.");
#ifdef WIN32
	CloseHandle(MsgQueuePtr->mutex);
#else
    pthread_mutex_destroy(&(MsgQueuePtr->mutex));
    pthread_cond_destroy(&(MsgQueuePtr->cond_var));
#endif
    DestroyPoolListStructs(&MsgQueuePtr->queue_list);
}
//---------------------------------------------------------------------------
static bool GetTimerMessageQueue(TimerMsgQueue *MsgQueuePtr, void **pp_message)
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
		printf("Fail to get mutex (1) (GetTimerMessageQueue)\r\n");
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
                printf("Fail to release mutex (GetTimerMessageQueue)\r\n");
			GetMessage(&SysMsg, NULL, TMU_REPORTDATA, TMU_REPORTDATA);
#else
            pthread_cond_wait(&(MsgQueuePtr->cond_var), &(MsgQueuePtr->mutex));
#endif
#ifdef WIN32
            if (WaitForSingleObject(MsgQueuePtr->mutex, INFINITE) == WAIT_FAILED)
	        {
		        printf("Fail to get mutex (2) (GetTimerMessageQueue)\r\n");
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
        TimerDebugLogPrint("wokeup due to destroy flag.");
        status = false;
    }
#ifdef WIN32
    if (!ReleaseMutex(MsgQueuePtr->mutex)) 
        printf("Fail to release mutex (GetTimerMessageQueue)\r\n");
#else
    pthread_mutex_unlock(&(MsgQueuePtr->mutex));
#endif
    return status;
}
//---------------------------------------------------------------------------
static TimerMessage* GetTimerMsgPool(unsigned int TimerId, unsigned char MsgTag)
{
    TimerMessage       *TimerMsgPtr = NULL;
    POOL_RECORD_STRUCT *ObjTaskPtr = NULL;
#ifdef WIN32
	DWORD              rc;
#endif

#ifdef WIN32
    if ((rc = WaitForSingleObject(gTimerMsgAccess, INFINITE)) == WAIT_FAILED)
	{
        return NULL;
	}
#else
    pthread_mutex_lock(&gTimerMsgAccess);
#endif
    ObjTaskPtr = GetBuffer(&gTimerMsgPool);
    if (!ObjTaskPtr)
    {
        TimerDebugLogPrint("No buffers for timer message delivery.");
#ifdef WIN32
        if (!ReleaseMutex(gTimerMsgAccess)) 
            printf("Fail to release mutex (GetTimerMsgPool)\r\n");
#else
        pthread_mutex_unlock(&gTimerMsgAccess);
#endif
        return NULL;
    }
    TimerMsgPtr = (TimerMessage*)ObjTaskPtr->DataPtr;
    TimerMsgPtr->MsgTag = MsgTag;
    TimerMsgPtr->TimerId = TimerId;
    TimerMsgPtr->BlkPoolPtr = ObjTaskPtr;
    TimerMsgPtr->ObjPtr = AddPoolStructListObj(&gListTimerMsg, TimerMsgPtr);
    if (gMaxSimultTimerMsgs < (unsigned int)gListTimerMsg.Count)
        gMaxSimultTimerMsgs = (unsigned int)gListTimerMsg.Count;
#ifdef WIN32
    if (!ReleaseMutex(gTimerMsgAccess)) 
        printf("Fail to release mutex (GetTimerMsgPool)\r\n");
#else
    pthread_mutex_unlock(&gTimerMsgAccess);
#endif
    return TimerMsgPtr;
}
//---------------------------------------------------------------------------
static void FreeTimerMsgPool(TimerMessage *TimerMsgPtr)
{
#ifdef WIN32
	DWORD  rc;

    if ((rc = WaitForSingleObject(gTimerMsgAccess, INFINITE)) == WAIT_FAILED)
	{
        return;
	}
#else
    pthread_mutex_lock(&gTimerMsgAccess);
#endif
    FreeBuffer(&gTimerMsgPool, TimerMsgPtr->BlkPoolPtr);
    RemPoolStructList(&gListTimerMsg, TimerMsgPtr->ObjPtr);
#ifdef WIN32
    if (!ReleaseMutex(gTimerMsgAccess)) 
        printf("Fail to release mutex (FreeTimerMsgPool)\r\n");
#else
    pthread_mutex_unlock(&gTimerMsgAccess);
#endif
}
//---------------------------------------------------------------------------
int TimerThreadCreate()
{
    int                Res = 0;
#ifdef WIN32
	struct tagMSG      SysMsg;
#else
    pthread_attr_t     attr, *attrPtr = &attr;
    struct sched_param sched;
#endif

    PoolListInit(&gListTimerMsg, INIT_TIMER_BLK_COUNT);
    CreatePool(&gTimerMsgPool, INIT_TIMER_BLK_COUNT, sizeof(TimerMessage));
#ifdef WIN32
    gTimerMsgAccess = CreateMutex(NULL, FALSE, NULL);
    if (gTimerMsgAccess == NULL) 
    {
        printf("Create Timer manager access mutex error: %d\r\n", GetLastError());
	    Res = -1;
    }
	else
	{
        TimerMsgQueueCreate(&g_timer_queue);
		gTimerInfo.StopTimerFlag = false;
		gTimerInfo.isStartDone = false;
	    gTimerInfo.ThrSYSTEMTIMER = 0;
        gTimerInfo.HTRSYSTIMER = 0;
        gTimerInfo.ThrAnswStart = GetCurrentThreadId();
        gTimerInfo.IDAnswStart = TMU_TIMER_RESULT_START;
        gTimerInfo.HTRSYSTIMER = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)TimerHandlerThread,
                            (LPVOID)(&gTimerInfo), 0, (LPDWORD)&gTimerInfo.ThrSYSTEMTIMER);
		g_timer_queue.ThrTimer = (DWORD)gTimerInfo.ThrSYSTEMTIMER;
        GetMessage(&SysMsg, NULL, TMU_TIMER_RESULT_START, TMU_TIMER_RESULT_START);
        if (SysMsg.wParam != 1)
        {
	        printf("Error start system timer thread. Error:[%d]\r\n",(unsigned)SysMsg.lParam);
		    Res = -1;
        }
	    if (!gTimerInfo.isStartDone) Res = -1;
		else
		{
            CreateThreadTimer(0, TIMER_STATUS_CHEC_TIMOUT, TIMER_STATUS_REQ_TIMER_ID, true);
		}
	}
#else
	sem_init(&gTimerInfo.TimerSem, 0, 0);
    pthread_mutex_init(&gTimerMsgAccess, NULL);
    TimerMsgQueueCreate(&g_timer_queue);
	gTimerInfo.isStartDone = false;
    gTimerInfo.StopTimerFlag = false;
    pthread_attr_init(attrPtr);
    pthread_attr_setdetachstate(attrPtr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(attrPtr, PTHREAD_SCOPE_SYSTEM);
    if (pthread_attr_getschedparam(attrPtr, &sched) == 0)
    {
        sched.sched_priority = 0;
        pthread_attr_setschedparam(attrPtr, &sched);
    }
    TimerDebugLogPrint("Timer mgr thread sturtup");
    if (pthread_create(&gTimerInfo.ThreadId, &attr, &TimerHandlerThread,
        &gTimerInfo) == -1)
    {
        TimerDebugLogPrint("Timer manager thread create error");
        Res = -1;
    }
    else
    {
        sem_wait(&gTimerInfo.TimerSem);
		if (!gTimerInfo.isStartDone) Res = -1;
		sem_post(&gTimerInfo.TimerSem);
        if (Res == 0) CreateThreadTimer(0, TIMER_STATUS_CHEC_TIMOUT, TIMER_STATUS_REQ_TIMER_ID, true);
    }
#endif
    return Res;
}
//---------------------------------------------------------------------------
void TimerThreadClose()
{
    TimerMessage    *TimerMsgPtr = NULL;
    ObjPoolListTask *PointTask = NULL;
#ifdef WIN32
	DWORD            rc;
#endif

    gTimerInfo.StopTimerFlag = true;
    TimerMsgQueueMarkDestroy(&g_timer_queue);
#ifdef WIN32
    if (gTimerInfo.ThrSYSTEMTIMER)
    {
        WaitCloseProcess(gTimerInfo.HTRSYSTIMER);
        CloseHandle(gTimerInfo.HTRSYSTIMER);
    }
    gTimerInfo.ThrSYSTEMTIMER = 0;
    gTimerInfo.HTRSYSTIMER = 0;
    Sleep(2*TICK_SELECT_TIME);
    if ((rc = WaitForSingleObject(gTimerMsgAccess, INFINITE)) == WAIT_FAILED)
	{
		printf("TimerThreadClose - Fail to get access mutex\r\n");
        return;
	}
#else        
    pthread_join(gTimerInfo.ThreadId, NULL);
    Sleep(2*TICK_SELECT_TIME);
    pthread_mutex_lock(&gTimerMsgAccess);
#endif
    while (gListTimerMsg.Count)
    {
        PointTask = (ObjPoolListTask*)gListTimerMsg.FistTask;
        TimerMsgPtr = (TimerMessage*)PointTask->UsedTask;
        RemPoolStructList(&gListTimerMsg, PointTask);
    }
#ifdef WIN32
    if (!ReleaseMutex(gTimerMsgAccess)) 
        printf("TimerThreadClose - Fail to release memory mutex\r\n");
#else
    pthread_mutex_unlock(&gTimerMsgAccess);
#endif
    DestroyPoolListStructs(&gListTimerMsg);
    DestroyPool(&gTimerMsgPool);
    TimerMsgQueueDestroy(&g_timer_queue);
#ifdef WIN32
	CloseHandle(gTimerMsgAccess);
#else
    pthread_mutex_destroy(&gTimerMsgAccess);
	sem_destroy(&gTimerInfo.TimerSem);
#endif
    TimerDebugLogPrint("Timer close is completed");
    return;   
}
//---------------------------------------------------------------------------
#ifdef WIN32
static DWORD WINAPI TimerHandlerThread(LPVOID Data)
#else
static void* TimerHandlerThread(void *Data)
#endif
{
    TimerMessage    *TimerMsgPtr = NULL;
	TimerRecord		*Timer;
	bool            MsgSentResult = false;	
	ObjPoolListTask	*PointTask = NULL;
	TimerTaskInfo   *ThrInfoPtr;
    TickTaskInfo    TickInfo;
#ifdef _LINUX_X86_
    struct timeval  tv;
    int             timerlen;
    struct sockaddr_in timer_addr;
    unsigned int    index;
#endif    

    ThrInfoPtr = (TimerTaskInfo*)Data;
    ThrInfoPtr->StopTimerFlag = false;

    TimerPar.EnOnTimer  = false;
    TimerPar.StopReq    = false;    
    PoolListInit(&TimerPar.ListTimers, INIT_TIMER_BLK_COUNT);
    CreatePool(&TimerPar.TimerBlockPool, INIT_TIMER_BLK_COUNT, sizeof(TimerRecord));

#ifdef WIN32
	if (TickThreadInit(&TickInfo) == -1)
    {
        printf("\n%s Failed to init tick generator", SetTimeStampLine());
		WinThreadMsgSend(ThrInfoPtr->ThrAnswStart, ThrInfoPtr->IDAnswStart, 0, 0);
        ExitThread(0);
    }
#else
    /* Create the socket for requests handle from other threads */  
    TimerPar.TimerMsgfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (TimerPar.TimerMsgfd < 0)
    {
        printf("%s Failed to open site socket (errno %d)\n",
            SetTimeStampLine(), errno);
		sem_post(&ThrInfoPtr->TimerSem);
        pthread_exit((void *)0);
    }
    
    /* Double the size of the socket send buffer */
    if (updateSocketSize(TimerPar.TimerMsgfd, SO_SNDBUF, TIMER_TX_BUFFER_SIZE) != 0)
    {
        printf("%s Failed to update TX buffer size for site socket (errno %d)\n",
            SetTimeStampLine(), errno);
        close(TimerPar.TimerMsgfd);
		sem_post(&ThrInfoPtr->TimerSem);
        pthread_exit((void *)0);
    }

    /* Double the size of the socket receive buffer */
    if (updateSocketSize(TimerPar.TimerMsgfd, SO_RCVBUF, TIMER_RX_BUFFER_SIZE) != 0)
    {
        printf("%s Failed to update RX buffer size for site socket (errno %d)\n",
            SetTimeStampLine(), errno);
        close(TimerPar.TimerMsgfd);
		sem_post(&ThrInfoPtr->TimerSem);
        pthread_exit((void *)0);
    }
  
    if (TickThreadInit(&TickInfo) == -1)
    {
        printf("\n%s Failed to init tick generator", SetTimeStampLine());
        close(TimerPar.TimerMsgfd);
		sem_post(&ThrInfoPtr->TimerSem);
        pthread_exit((void *)0);
    }

#endif

	ThrInfoPtr->isStartDone = true;
#ifdef WIN32
	WinThreadMsgSend(ThrInfoPtr->ThrAnswStart, ThrInfoPtr->IDAnswStart, 1, 0);
#else
	sem_post(&ThrInfoPtr->TimerSem);
#endif
    printf("%s Tick generator was successfuly started\n", SetTimeStampLine());    

    /* Read the data from the timer messages queue. */
    while (GetTimerMessageQueue(&g_timer_queue, (void**)&TimerMsgPtr))
    {    
	    if(ThrInfoPtr->StopTimerFlag)
        {
            printf("%s Timer's thread stop was requested\n", SetTimeStampLine());
            break;
        }
        if (TimerMsgPtr->MsgTag != TMR_TICK_EVENT)
            TimerDebugLogPrint("Received ext message (Msg:0x%02X, Id:%d, MP:%p)",
                TimerMsgPtr->MsgTag, TimerMsgPtr->TimerId, TimerMsgPtr);
#ifdef WIN32
		TimerPar.Tick = GetTickCount();
#else
        if(gettimeofday(&tv, NULL) != 0 ) TimerPar.Tick = 1;
        else TimerPar.Tick = (unsigned long)((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
        if (!TimerPar.Tick) TimerPar.Tick = 1;
#endif        
        switch(TimerMsgPtr->MsgTag)
        {
            case TMR_TICK_EVENT:
                HandleTimerTick(&TimerPar);
                break;

            case TMR_START_TIMER:
                HandleStartTimerReq(&TimerPar, TimerMsgPtr);
                break;

            case TMR_RESET_TIMER:
                HandleResetTimerReq(&TimerPar, TimerMsgPtr);
                break;

            case TMR_RESTART_TIMER:
                HandleRestartTimerReq(&TimerPar, TimerMsgPtr);
                break;
                
            case TMR_RESET_GRP_TIMER:
                HandleRestartGrpTimerReq(&TimerPar, TimerMsgPtr);
                break;
                
            default:
                TimerDebugLogPrint("Unexpected(0x%02X) message tag is received", TimerMsgPtr->MsgTag);
                break;
        }
        FreeTimerMsgPool(TimerMsgPtr);
    }
    
    TimerDebugLogPrint("Timer thread stop is initiated");
    TickThreadClose(&TickInfo);       
    while (TimerPar.ListTimers.Count)
    {
	    PointTask = (ObjPoolListTask*)TimerPar.ListTimers.FistTask;
	    Timer = (TimerRecord*)PointTask->UsedTask;
	    FreeMemory(Timer);
	    RemPoolStructList(&TimerPar.ListTimers, PointTask);
    }    
#ifdef _LINUX_X86_
    close(TimerPar.TimerMsgfd);
#endif
    DestroyPoolListStructs(&TimerPar.ListTimers);
    DestroyPool(&TimerPar.TimerBlockPool);
    printf("\n%s Timer thread was stopped\n", SetTimeStampLine());
#ifdef WIN32
    ExitThread(0);
#else
    pthread_exit((void *)0);
#endif
}
//---------------------------------------------------------------------------
static void HandleStartTimerReq(TimerThrPar *TimerParPtr, TimerMessage *TimerMsgPtr)
{
    TimerRecord        *TimerPtr = NULL;
    POOL_RECORD_STRUCT *ObjTaskPtr = NULL;
    int                ret = 0;

    TimerPtr = GetTimerRecordByTimerId(TimerParPtr, TimerMsgPtr);
    if (TimerPtr)
    {
        TimerDebugLogPrint("Fail to add timer - already exist 1 (restart) (NP: %d, Id: %d, TO: %d, RF: %d)",
            TIMER_NOTIFY_TARGET, TimerPtr->TimerId,
            TimerPtr->TimeOutVal, (unsigned char)TimerPtr->Replay);
            
        RemPoolStructList(&TimerParPtr->ListTimers, TimerPtr->ObjPtr);
        TimerPtr->TimeStart = TimerParPtr->Tick;
        TimerPtr->TimeOutVal = TimerMsgPtr->TimeOutVal;
        SetTimerActiveList(TimerParPtr, TimerPtr);
            
    }
    else
    {
        TimerDebugLogPrint("Timer no exist needs to add (NP: %d, Id: %d, TO: %d, RF: %d, CB:%p, CBD:%p)",
            MSG_NOTIFY_TARGET, TimerMsgPtr->TimerId, 
			TimerMsgPtr->TimeOutVal, (unsigned char)TimerMsgPtr->Replay, 
			TimerMsgPtr->OnTimerExp, TimerMsgPtr->OnTimerExpData);
                    
        ObjTaskPtr = GetBuffer(&TimerParPtr->TimerBlockPool);
        if (!ObjTaskPtr)
        {
            TimerDebugLogPrint("Fail to get free timer record (NP: %d, Id: %d, TO: %d, RF: %d)",
                MSG_NOTIFY_TARGET, TimerMsgPtr->TimerId,
                TimerMsgPtr->TimeOutVal, (unsigned char)TimerMsgPtr->Replay);        
        }
        else
        {
            TimerPtr = (TimerRecord*)ObjTaskPtr->DataPtr;
            TimerPtr->TimeOutVal = TimerMsgPtr->TimeOutVal;
            TimerPtr->TimerId    = TimerMsgPtr->TimerId;
            TimerPtr->TimeStart  = TimerParPtr->Tick;
            TimerPtr->Replay     = TimerMsgPtr->Replay;
			TimerPtr->OnTimerExp = TimerMsgPtr->OnTimerExp;
			TimerPtr->OnTimerExpData = TimerMsgPtr->OnTimerExpData;
			TimerPtr->DataPtr    = TimerMsgPtr->DataPtr;
            TIMER_NOTIFY_TARGET  = MSG_NOTIFY_TARGET;
            TimerPtr->BlkPoolPtr = ObjTaskPtr;
            if (SetTimerActiveList(TimerParPtr, TimerPtr))
            {
                TimerDebugLogPrint("Fail to add timer - already exist 2 (NP: %d, Id: %d, TO: %d, RF: %d)",
                    TIMER_NOTIFY_TARGET, TimerPtr->TimerId,
                    TimerPtr->TimeOutVal, (unsigned char)TimerPtr->Replay);
                FreeBuffer(&TimerParPtr->TimerBlockPool, TimerPtr->BlkPoolPtr);
            }
            else
            {
                TimerDebugLogPrint("Add timer (NP: %d, Id: %d, TO: %d, RF: %d, CB: %p, CBD:%p)",
                    TIMER_NOTIFY_TARGET, TimerPtr->TimerId,
                    TimerPtr->TimeOutVal, (unsigned char)TimerPtr->Replay,
					TimerMsgPtr->OnTimerExp, TimerMsgPtr->OnTimerExpData);
            }
        }
    }
}
//---------------------------------------------------------------------------
static void HandleResetTimerReq(TimerThrPar *TimerParPtr, TimerMessage *TimerMsgPtr)
{
    TimerRecord       *TimerPtr = NULL;

    TimerDebugLogPrint("Reset timer (NP: %d, Id: %d)",
        MSG_NOTIFY_TARGET, TimerMsgPtr->TimerId);
    TimerPtr = GetTimerRecordByTimerId(TimerParPtr, TimerMsgPtr);
    if (TimerPtr) TimerRecordRemove(TimerParPtr, TimerPtr);
    else          TimerDebugLogPrint("Timer (NP: %d, Id: %d) not found", 
                        MSG_NOTIFY_TARGET, TimerMsgPtr->TimerId);
}
//---------------------------------------------------------------------------
static void HandleRestartTimerReq(TimerThrPar *TimerParPtr, TimerMessage *TimerMsgPtr)
{
    TimerRecord       *TimerPtr = NULL;

    TimerDebugLogPrint("Restart timer (NP: %d, Id: %d)",
        MSG_NOTIFY_TARGET, TimerMsgPtr->TimerId);
    TimerPtr = GetTimerRecordByTimerId(TimerParPtr, TimerMsgPtr);
    if (TimerPtr)
    {
        RemPoolStructList(&TimerParPtr->ListTimers, TimerPtr->ObjPtr);
        TimerPtr->TimeStart = TimerParPtr->Tick;
        SetTimerActiveList(TimerParPtr, TimerPtr);
    }
    else
    {
        TimerDebugLogPrint("Timer (NP: %d, Id: %d) not found", 
            MSG_NOTIFY_TARGET, TimerMsgPtr->TimerId);
    }
}
//---------------------------------------------------------------------------
static void HandleRestartGrpTimerReq(TimerThrPar *TimerParPtr, TimerMessage *TimerMsgPtr)
{
    bool              isRestartDone = false;
    unsigned int      i;
    TimerRecord       *TimerPtr = NULL;
	ObjPoolListTask	  *PointTask = NULL;
    ObjListTask       *SelReqObjPtr = NULL;
    ListItsTask       RestartReqList;
    
    TimerDebugLogPrint("Restart group of timer's (NP: %d, GS: %d)", 
        MSG_NOTIFY_TARGET, TimerMsgPtr->TimerRestartReqArray[0]);
	if (TimerMsgPtr->TimerRestartReqArray[0] == 0) return;

	RestartReqList.Count = 0;
	RestartReqList.CurrTask = NULL;
	RestartReqList.FistTask = NULL;
    
    for (i=0;i < TimerMsgPtr->TimerRestartReqArray[0];i++)
        AddStructList(&RestartReqList, (void*)(tulong)TimerMsgPtr->TimerRestartReqArray[i+1]);
   
	PointTask = (ObjPoolListTask*)GetFistPoolObjectList(&TimerParPtr->ListTimers);
	while(PointTask && (RestartReqList.Count > 0))
	{
		TimerPtr = (TimerRecord*)PointTask->UsedTask;
		if (TimerPtr)
        {            
            if (TIMER_NOTIFY_TARGET == MSG_NOTIFY_TARGET)
			{
                SelReqObjPtr = (ObjListTask*)GetFistObjectList(&RestartReqList);
                while(SelReqObjPtr)
				{
					if (TimerPtr->TimerId == (unsigned int)(tulong)SelReqObjPtr->UsedTask)
					{
                        TimerDebugLogPrint("Restart timer in group (NP: %d, TI: %d)", 
                            TIMER_NOTIFY_TARGET, TimerPtr->TimerId);
                        RemPoolStructList(&TimerParPtr->ListTimers, TimerPtr->ObjPtr);
					    TimerPtr->TimeStart = TimerParPtr->Tick;
                        SetTimerActiveList(TimerParPtr, TimerPtr);
                        PointTask = (ObjPoolListTask*)GetFistPoolObjectList(&TimerParPtr->ListTimers);  
                        RemStructList(&RestartReqList, SelReqObjPtr); 
                        isRestartDone = true;
						break;
					}
                    SelReqObjPtr = (ObjListTask*)GetNextObjectList(&RestartReqList);
				}
            }
            if (!isRestartDone) PointTask = (ObjPoolListTask*)GetNextPoolObjectList(&TimerParPtr->ListTimers);
            isRestartDone = false;
		}
		else
		{
			RemPoolStructList(&TimerParPtr->ListTimers, PointTask);
			PointTask = (ObjPoolListTask*)GetFistPoolObjectList(&TimerParPtr->ListTimers);
		}
	}
    
	SelReqObjPtr = (ObjListTask*)GetFistObjectList(&RestartReqList);
	while(SelReqObjPtr)
	{
		RemStructList(&RestartReqList, SelReqObjPtr);
	    SelReqObjPtr = (ObjListTask*)GetFistObjectList(&RestartReqList);
	} 
}
//---------------------------------------------------------------------------
static TimerRecord* GetTimerRecordByTimerId(TimerThrPar *TimerParPtr, TimerMessage *TimerMsgPtr)
{
    bool            EnOnTimer = true;
    TimerRecord     *TimerPtr = NULL;
    TimerRecord     *FindTimerPtr = NULL;
    ObjPoolListTask *PointTask = NULL;

    while (EnOnTimer)
    {
        EnOnTimer = false;
        PointTask = (ObjPoolListTask*)GetFistPoolObjectList(&TimerParPtr->ListTimers);
        while(PointTask)
        {
            TimerPtr = (TimerRecord*)PointTask->UsedTask;
            if (TimerPtr)
            {
                if ((TIMER_NOTIFY_TARGET == MSG_NOTIFY_TARGET) && 
                    (TimerPtr->TimerId == TimerMsgPtr->TimerId))
                {
                    FindTimerPtr = TimerPtr;
                    break;
                }
            }
            else
            {
                RemPoolStructList(&TimerParPtr->ListTimers, PointTask);
                FreeBuffer(&TimerParPtr->TimerBlockPool, TimerPtr->BlkPoolPtr);
                EnOnTimer = true;
                break;
            }
            PointTask = (ObjPoolListTask*)GetNextPoolObjectList(&TimerParPtr->ListTimers);
        }
    }
    return FindTimerPtr;
}
//---------------------------------------------------------------------------
static void TimerStatusShow(TimerThrPar *TimerParPtr)
{
    ObjPoolListTask *PointTask = NULL;
    TimerRecord     *TimerPtr = NULL;

    TimerDebugLogPrint("Status - AT:%d, MT:%d, MPUR:%d, MPFR:%d, ML:%d, MTM:%d",
        (unsigned int)TimerParPtr->ListTimers.Count, gMaxSimultActTimers,
        gTimerMsgPool.m_NumUsedRecords, gTimerMsgPool.m_NumFreeRecords,
        (unsigned int)gListTimerMsg.Count, gMaxSimultTimerMsgs);

    PointTask = (ObjPoolListTask*)GetFistPoolObjectList(&TimerParPtr->ListTimers);
    while(PointTask)
    {
        TimerPtr = (TimerRecord*)PointTask->UsedTask;
        if (TimerPtr)
        {
            TimerDebugLogPrint("Status - Active timer (NP: %d, TI: %d, TO: %d, RF: %d)",
                TIMER_NOTIFY_TARGET, TimerPtr->TimerId,
                TimerPtr->TimeOutVal, (unsigned char)TimerPtr->Replay);
        }
        PointTask = (ObjPoolListTask*)GetNextPoolObjectList(&TimerParPtr->ListTimers);
    }
}
//---------------------------------------------------------------------------
static void TimerRecordRemove(TimerThrPar *TimerParPtr, TimerRecord *TimerPtr)
{
    TimerDebugLogPrint("Remove timer record (NP: %d, Id: %d)",
        TIMER_NOTIFY_TARGET, TimerPtr->TimerId);
    RemPoolStructList(&TimerParPtr->ListTimers, TimerPtr->ObjPtr);
    FreeBuffer(&TimerParPtr->TimerBlockPool, TimerPtr->BlkPoolPtr);
}
//---------------------------------------------------------------------------
static void HandleTimerTick(TimerThrPar *TimerParPtr)
{
   unsigned int    Wait;
   TimerRecord     *Timer = NULL;
   ObjPoolListTask *PointTask = NULL;

   TimerParPtr->EnOnTimer = true;
   while (TimerParPtr->EnOnTimer)
   {
      TimerParPtr->EnOnTimer = false;
	  PointTask = (ObjPoolListTask*)GetFistPoolObjectList(&TimerParPtr->ListTimers);
	  while(PointTask)
      {
		  Timer = (TimerRecord*)PointTask->UsedTask;
		  if (Timer)	
          {
		      Wait = (unsigned int)(TimerParPtr->Tick - Timer->TimeStart);
              if (Wait < Timer->TimeOutVal)
              {
                  /* All other timers are not expired */
                  break;
              }
              else
              {
#ifdef _LINUX_X86_
				  if (Timer->OnTimerExp)
				  {
                      TimerDebugLogPrint("Timer is expired (CB): TI=%d, TO=%d(ms), RP=%d, CB: %p", 
                          Timer->TimerId, Timer->TimeOutVal, Timer->Replay, Timer->OnTimerExp);
					  (Timer->OnTimerExp)((unsigned int)Timer->TimerId);
				  }
				  else if (Timer->OnTimerExpData)
				  {
                      TimerDebugLogPrint("Timer is expired (CBD): TI=%d, TO=%d(ms), RP=%d, CBD: %p", 
                          Timer->TimerId, Timer->TimeOutVal, Timer->Replay, Timer->OnTimerExpData);
					  (Timer->OnTimerExpData)((unsigned int)Timer->TimerId, Timer->DataPtr);
				  }
				  else if (Timer->NotifyPort > 0)
                  {              
                      TimerDebugLogPrint("Timer is expired: NP=%d, TI=%d, TO=%d(ms), RP=%d", 
                          Timer->NotifyPort, Timer->TimerId, Timer->TimeOutVal, Timer->Replay);                           
					  SocketMsgSendThread(TimerParPtr->TimerMsgfd, Timer->NotifyPort,
					      TM_ONTIMEOUT, (void*)(tulong)((unsigned int)Timer->TimerId), 0);
                  }
#endif
#ifdef WIN32
				  if (Timer->OnTimerExp)
				  {
                      TimerDebugLogPrint("Timer is expired (CB): TI=%d, TO=%d(ms), RP=%d, CB: %p", 
                          Timer->TimerId, Timer->TimeOutVal, Timer->Replay, Timer->OnTimerExp);
					  (Timer->OnTimerExp)((unsigned int)Timer->TimerId);
				  }
				  else if (Timer->OnTimerExp)
				  {
                      TimerDebugLogPrint("Timer is expired (CBD): TI=%d, TO=%d(ms), RP=%d, CBD: %p", 
                          Timer->TimerId, Timer->TimeOutVal, Timer->Replay, Timer->OnTimerExpData);
					  (Timer->OnTimerExpData)((unsigned int)Timer->TimerId, Timer->DataPtr);
				  }
				  else if (Timer->ThreadID > 0)
                  {
                      TimerDebugLogPrint("Timer is expired: NP=%d, TI=%d, TO=%d(ms), RP=%d", 
                          Timer->ThreadID, Timer->TimerId, Timer->TimeOutVal, Timer->Replay);

                      WinThreadMsgSend(Timer->ThreadID, TM_ONTIMEOUT, (unsigned int)Timer->TimerId, 0);
                  }
#endif                      
                  else if (Timer->TimerId == TIMER_STATUS_REQ_TIMER_ID)
                  {
                      TimerStatusShow(TimerParPtr);
                  }
                  else
                  {
                      TimerDebugLogPrint("Unexpected internal timer ID (%d) is received", Timer->TimerId);
                  }

                  if (!Timer->Replay)
				  {
                      FreeBuffer(&TimerParPtr->TimerBlockPool, Timer->BlkPoolPtr);
					  RemPoolStructList(&TimerParPtr->ListTimers, PointTask);
					  TimerParPtr->EnOnTimer = true;
					  break;
				  }
				  else
				  {
                      RemPoolStructList(&TimerParPtr->ListTimers, PointTask);
					  Timer->TimeStart = TimerParPtr->Tick;
                      SetTimerActiveList(TimerParPtr, Timer);
					  TimerParPtr->EnOnTimer = true;
					  break;                      
				  }
             }
			 PointTask = (ObjPoolListTask*)GetNextPoolObjectList(&TimerParPtr->ListTimers);
		  }
		  else
		  {
			  RemPoolStructList(&TimerParPtr->ListTimers, PointTask);
			  PointTask = (ObjPoolListTask*)GetFistPoolObjectList(&TimerParPtr->ListTimers);
		  } 
	  }
   }
}
//---------------------------------------------------------------------------
#ifdef WIN32
void CreateThreadTimer(int AnswThread, unsigned int TimeOut, 
    unsigned int TimerId, bool Replay)
#else
void CreateThreadTimer(unsigned short NotifyPort, unsigned int TimeOut,
    unsigned int TimerId, bool Replay)
#endif
{
    TimerMessage *TimerMsgPtr = NULL;

    TimerMsgPtr = GetTimerMsgPool(TimerId, TMR_START_TIMER);
    if (!TimerMsgPtr) return;
    TimerMsgPtr->Replay = Replay;
    TimerMsgPtr->TimeOutVal = TimeOut;
	TimerMsgPtr->OnTimerExp = NULL;
	TimerMsgPtr->OnTimerExpData = NULL;
	TimerMsgPtr->DataPtr = NULL;
#ifdef WIN32
    MSG_NOTIFY_TARGET = AnswThread;
    TimerDebugLogPrint("Timer start request (AT: %d, TI: %d)",
               MSG_NOTIFY_TARGET, TimerMsgPtr->TimerId);
#else
    MSG_NOTIFY_TARGET = NotifyPort;
    TimerDebugLogPrint("Timer start request (TAP: %d, TI: %d)",
               MSG_NOTIFY_TARGET, TimerMsgPtr->TimerId);
#endif               
    TimerMsgQueuePost(&g_timer_queue, TimerMsgPtr);
}

#ifdef WIN32
void CreateThreadTimerCB(TOnTimerCB OnTimerExp, int AnswThread, unsigned int TimeOut,
    unsigned int TimerId, bool Replay)
#else
void CreateThreadTimerCB(TOnTimerCB OnTimerExp, unsigned short NotifyPort, unsigned int TimeOut,
    unsigned int TimerId, bool Replay)
#endif
{
    TimerMessage *TimerMsgPtr = NULL;

    TimerMsgPtr = GetTimerMsgPool(TimerId, TMR_START_TIMER);
    if (!TimerMsgPtr) return;
    TimerMsgPtr->Replay = Replay;
    TimerMsgPtr->TimeOutVal = TimeOut;
	TimerMsgPtr->OnTimerExp = OnTimerExp;
	TimerMsgPtr->OnTimerExpData = NULL;
	TimerMsgPtr->DataPtr = NULL;
#ifdef WIN32
	MSG_NOTIFY_TARGET = AnswThread;
#else
	MSG_NOTIFY_TARGET = NotifyPort;
#endif
    TimerDebugLogPrint("Timer start request (CP: %p, TI: %d)",
               OnTimerExp, TimerMsgPtr->TimerId);             
    TimerMsgQueuePost(&g_timer_queue, TimerMsgPtr);
}

#ifdef WIN32
void CreateThreadTimerCBD(TOnTimerCBD OnTimerExp, int AnswThread, unsigned int TimeOut,
    unsigned int TimerId, bool Replay, void *DataPtr)
#else
void CreateThreadTimerCBD(TOnTimerCBD OnTimerExp, unsigned short NotifyPort, unsigned int TimeOut,
    unsigned int TimerId, bool Replay, void *DataPtr)
#endif
{
    TimerMessage *TimerMsgPtr = NULL;

    TimerMsgPtr = GetTimerMsgPool(TimerId, TMR_START_TIMER);
    if (!TimerMsgPtr) return;
    TimerMsgPtr->Replay = Replay;
    TimerMsgPtr->TimeOutVal = TimeOut;
	TimerMsgPtr->OnTimerExp = NULL;
	TimerMsgPtr->OnTimerExpData = OnTimerExp;
	TimerMsgPtr->DataPtr = DataPtr;
#ifdef WIN32
	MSG_NOTIFY_TARGET = AnswThread;
#else
	MSG_NOTIFY_TARGET = NotifyPort;
#endif
    TimerDebugLogPrint("Timer start request (CP: %p, TI: %d)",
               OnTimerExp, TimerMsgPtr->TimerId);             
    TimerMsgQueuePost(&g_timer_queue, TimerMsgPtr);
}
//---------------------------------------------------------------------------
#ifdef WIN32
void ResetThreadTimer(int AnswThread, unsigned int TimerId)
#else
void ResetThreadTimer(unsigned short NotifyPort, unsigned int TimerId)
#endif
{
    TimerMessage *TimerMsgPtr = NULL;

    TimerMsgPtr = GetTimerMsgPool(TimerId, TMR_RESTART_TIMER);
    if (!TimerMsgPtr) return;
#ifdef WIN32
    MSG_NOTIFY_TARGET = AnswThread;
    TimerDebugLogPrint("Timer restart request (AT: %d, TI: %d)",
               MSG_NOTIFY_TARGET, TimerMsgPtr->TimerId);
#else
    MSG_NOTIFY_TARGET = NotifyPort;
    TimerDebugLogPrint("Timer restart request (TAP: %d, TI: %d)",
               MSG_NOTIFY_TARGET, TimerMsgPtr->TimerId);
#endif               
    TimerMsgQueuePost(&g_timer_queue, TimerMsgPtr);
}
//---------------------------------------------------------------------------
#ifdef WIN32
void CloseThreadTimer(int AnswThread, unsigned int TimerId)
#else
void CloseThreadTimer(unsigned short NotifyPort, unsigned int TimerId)
#endif
{
    TimerMessage *TimerMsgPtr = NULL;

    TimerMsgPtr = GetTimerMsgPool(TimerId, TMR_RESET_TIMER);
    if (!TimerMsgPtr) return;
#ifdef WIN32
    MSG_NOTIFY_TARGET = AnswThread;
    TimerDebugLogPrint("Timer reset request (AT: %d, TI: %d)",
               MSG_NOTIFY_TARGET, TimerMsgPtr->TimerId);
#else
    MSG_NOTIFY_TARGET = NotifyPort;
    TimerDebugLogPrint("Timer reset request (TAP: %d, TI: %d)",
               MSG_NOTIFY_TARGET, TimerMsgPtr->TimerId);
#endif               
    TimerMsgQueuePost(&g_timer_queue, TimerMsgPtr);
}
//---------------------------------------------------------------------------
#ifdef WIN32
void  RestartGrpThreadTimer(int AnswThread, unsigned int *TimerRestartReqArray)
#else
void RestartGrpThreadTimer(unsigned short NotifyPort, unsigned int *TimerRestartReqArray)
#endif
{
    TimerMessage *TimerMsgPtr = NULL;

    TimerMsgPtr = GetTimerMsgPool(0, TMR_RESET_GRP_TIMER);
    if (!TimerMsgPtr) return;
#ifdef WIN32
    MSG_NOTIFY_TARGET = AnswThread;
#else
    MSG_NOTIFY_TARGET = NotifyPort;
#endif               
    TimerMsgPtr->TimerRestartReqArray = TimerRestartReqArray;
    TimerDebugLogPrint("Timer's group restart request (NP: %d)",
               MSG_NOTIFY_TARGET);
    TimerMsgQueuePost(&g_timer_queue, TimerMsgPtr);
}
//---------------------------------------------------------------------------
static int TickThreadInit(TickTaskInfo *TickInfoPtr)
{
    int                 Res = 0;
#ifdef _LINUX_X86_ 
    pthread_attr_t      attr, *attrPtr = &attr;
    struct sched_param  sched;
#endif

    TickInfoPtr->StopTickFlag = false;
#ifdef WIN32
    TickInfoPtr->HTRTICKER = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)TimerTickThread,
                            (LPVOID)TickInfoPtr, 0, (LPDWORD)&TickInfoPtr->ThrTICKER);
#else
    pthread_attr_init(attrPtr);
    pthread_attr_setdetachstate(attrPtr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(attrPtr, PTHREAD_SCOPE_SYSTEM);
    if (pthread_attr_getschedparam(attrPtr, &sched) == 0)
    {
        sched.sched_priority = 0;
        pthread_attr_setschedparam(attrPtr, &sched);
    }
    printf("Timer tick thread sturtup\n");
    if (pthread_create(&TickInfoPtr->ThreadId, &attr, &TimerTickThread,
        TickInfoPtr) == -1)
    {
        printf("Timer tick thread create error\n");
        Res = -1;
    }
#endif
    return Res;
}
//---------------------------------------------------------------------------
static void TickThreadClose(TickTaskInfo *TickInfoPtr)
{
    TickInfoPtr->StopTickFlag = true;
    TimerDebugLogPrint("Tick thread close initiate");
#ifdef WIN32
    if (TickInfoPtr->ThrTICKER)
    {
        WaitCloseProcess(TickInfoPtr->HTRTICKER);
        CloseHandle(TickInfoPtr->HTRTICKER);
    }
    TickInfoPtr->ThrTICKER = 0;
    TickInfoPtr->HTRTICKER = 0;
#else    
    pthread_join(TickInfoPtr->ThreadId, NULL);
#endif
    Sleep((TICK_SELECT_TIME*3)/2);
    return;
}
//---------------------------------------------------------------------------
static void* TimerTickThread(void* arg)
{
#ifdef _LINUX_X86_
    struct timespec    tim;
#endif
    TickTaskInfo       *TickInfoPtr = NULL;
    TimerMessage       *TimerMsgPtr = NULL;

    TickInfoPtr = (TickTaskInfo*)arg;
#ifdef WIN32
	TimerDebugLogPrint("Timer tick thread [%u] is started.", GetCurrentThreadId());
#else
    TimerDebugLogPrint("Timer tick thread [%p] is started.", (void*)pthread_self());
#endif
    for(;;)
    {
        if (TickInfoPtr->StopTickFlag) break;
#ifdef WIN32
		Sleep(TICK_SELECT_TIME);
#else
        tim.tv_sec = 0;
        tim.tv_nsec = TICK_SELECT_TIME*1000000L;
        if (nanosleep(&tim, (struct timespec*)NULL) < 0)
            TimerDebugLogPrint("Nano sleep system call processing error.");
#endif
        TimerMsgPtr = GetTimerMsgPool(0, TMR_TICK_EVENT);
        if (TimerMsgPtr) TimerMsgQueuePost(&g_timer_queue, TimerMsgPtr);
    }
    TimerDebugLogPrint("Tick thread is closed");
#ifdef WIN32
    ExitThread(0);
#else
    pthread_exit((void *)0);
#endif
}
//---------------------------------------------------------------------------
static bool SetTimerActiveList(TimerThrPar *TimerPar, TimerRecord *Timer)
{
	bool            Present = false;
    unsigned int    TestTimerExp;    
	ObjPoolListTask	*PointTask = NULL;
    ObjPoolListTask	*PointTimeMoreTask = NULL;    
	TimerRecord        *TestTimer = NULL;

	PointTask = (ObjPoolListTask*)GetFistPoolObjectList(&TimerPar->ListTimers);
	while(PointTask)
	{
		TestTimer = (TimerRecord*)PointTask->UsedTask;
		if (TestTimer)
		{
            TestTimerExp = (unsigned int)(TimerPar->Tick - TestTimer->TimeStart);
            if (TestTimer->TimeOutVal < TestTimerExp) TestTimerExp = 0;
            else                                      TestTimerExp = TestTimer->TimeOutVal - TestTimerExp;

            if (!PointTimeMoreTask && (Timer->TimeOutVal < TestTimerExp))
                PointTimeMoreTask = PointTask;
#ifdef WIN32
            if ((Timer->ThreadID == TestTimer->ThreadID) &&
#else
            if ((Timer->NotifyPort == TestTimer->NotifyPort) &&
#endif
		        (Timer->TimerId == TestTimer->TimerId))
			{
				Present = true;
                break;
			}
		}
        PointTask = (ObjPoolListTask*)GetNextPoolObjectList(&TimerPar->ListTimers);
    }
	if ( !Present ) 
    {
        if (PointTimeMoreTask)
        {
            /* Timer with more exp. delay was found */
            Timer->ObjPtr = AddPoolStructListAboveObj(&TimerPar->ListTimers, PointTimeMoreTask, Timer);
        }
        else
        {
            /* Timers with more expiration delay was not found */                
            Timer->ObjPtr = AddPoolStructListObj(&TimerPar->ListTimers, Timer);
        }
        if ((unsigned int)TimerPar->ListTimers.Count > gMaxSimultActTimers)
            gMaxSimultActTimers = (unsigned int)TimerPar->ListTimers.Count;        
    }
    return Present;
}
//---------------------------------------------------------------------------
static void TimerDebugLogPrint(const char* format, ... )
{
#ifdef _TIMER_DEBUG_
    char         *CwdRet = NULL;
  #ifdef WIN32
	SYSTEMTIME   CurrTime;
	DWORD        WaitResult;
	bool         isFileOperReady = false;
  #endif
  #ifdef _LINUX_X86_
    struct timeb hires_cur_time;
    struct tm    *cur_time;
  #endif
	FILE         *FileLogPtr = NULL;
    va_list      args;
    char         LogBuffer[256];
	char         FileName[1024];

    va_start( args, format );
	*LogBuffer = 0;
	CwdRet = getcwd(FileName, 512);
  #ifdef WIN32
	strcat(FileName, "\\LogFiles\\TimerThread.log");
  #endif
  #ifdef _LINUX_X86_
	strcat(FileName, "/LogFiles/TimerThread.log");
  #endif

  #ifdef WIN32
    WaitResult = WaitForSingleObject(gFileMutex, INFINITE);
    switch(WaitResult)
	{
	    case WAIT_OBJECT_0:
			isFileOperReady = true;
		    break;

        case WAIT_ABANDONED:
			printf("The other thread that using mutex is closed in locked state of mutex\r\n");
            break;

		default:
			printf("Timer thread mutex is fialed with error: %d\r\n", GetLastError());
			break;
	}
    if (!isFileOperReady) return;
  #endif

	FileLogPtr = fopen(FileName,"ab");

  #ifdef WIN32
    GetSystemTime(&CurrTime);
    sprintf(LogBuffer, "%02d/%02d/%04d-%02d:%02d:%02d.%03d | ",
		CurrTime.wDay, CurrTime.wMonth, CurrTime.wYear,
		CurrTime.wHour, CurrTime.wMinute, 
		CurrTime.wSecond, CurrTime.wMilliseconds);
  #endif
  #ifdef _LINUX_X86_
    ftime(&hires_cur_time);
    cur_time = localtime(&hires_cur_time.time);
    sprintf(LogBuffer, "%02d/%02d/%04d-%02d:%02d:%02d.%03d | ",
           cur_time->tm_mday, (cur_time->tm_mon+1),
           (cur_time->tm_year+1900), cur_time->tm_hour,
           cur_time->tm_min, cur_time->tm_sec,
           hires_cur_time.millitm);
  #endif
	vsprintf(&LogBuffer[strlen(LogBuffer)], format, args);
    strcat(LogBuffer, "\n");
    fwrite((const void*)&LogBuffer[0], strlen(LogBuffer), 1, FileLogPtr);
	fclose(FileLogPtr);
  #ifdef WIN32
        if (! ReleaseMutex(gFileMutex))
            printf("Fail to release mutex Timer thr\r\n");
  #endif
    va_end( args );   
#endif
}
//-----------------------------------------------------------------------
