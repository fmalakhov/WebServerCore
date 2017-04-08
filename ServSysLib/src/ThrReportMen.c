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

#ifdef _LINUX_X86_
#include <dirent.h>
#endif
#include <sys/stat.h>

#include "ThrReportMen.h"
#include "TrTimeOutThread.h"

#define REPORT_MSG_RX_PORT        6520

extern char *TablDefNameDey[];
#ifdef WIN32
extern HANDLE gMemoryMutex;
#else
extern sem_t gMemoryMutex;
#endif

#ifdef _REPORT_LOG_WEB_
  extern char GenPageLogMgr[];
  extern char GenPageMain[];
#endif

static PoolListItsTask  gListReportMsg;
static POOL_RECORD_BASE gReportMsgPool;
#ifdef WIN32
static HANDLE           gReportMsgAccess;
#else
static pthread_mutex_t  gReportMsgAccess;
#endif
static ReportMsgQueue    g_report_queue;
static PARINIREPMEN     ParRepMen;
static unsigned int     gMaxSimultReportMsgs = 0;
static unsigned int     gLostReportMsgs = 0;

#define MAX_LOG_MSG_POOL      20000 /* Maximum number of messages in report pool */
#define INIT_REPORT_MSG_COUNT  5000 /* Start number of report messags in pool */
#define MAX_SIZE_BUF_LOG	  32000 /* The maximum size of buffer log file creation. */

#ifdef _REPORT_LOG_WEB_
  #define SIZE_FORM_BUF_HTTP    1024000
#endif

#ifdef _REPORT_LOG_WEB_
char	*TablRepMenCmd[]={"vlj","vle","llf","glf"};
#endif

static void LogDateChangeCheck(REPORT_MANAGER_TASK *Report);
static void OnReportTimerExp(unsigned int TimerId);
static void ReportMsgQueueCreate(ReportMsgQueue *p_msg_queue);
static void ReportMsgQueueDestroy(ReportMsgQueue *p_msg_queue);
static void ReportMsgQueuePost(ReportMsgQueue *p_msg_queue, void* p_message);
static bool GetReportMessageQueue(ReportMsgQueue *p_msg_queue, void **pp_message);
static void ReportMsgQueueMarkDestroy(ReportMsgQueue *p_msg_queue);
static ReportMessage* GetReportMsgPool(unsigned char MsgTag);
static void FreeReportMsgPool(ReportMessage *ReportMsgPtr);
static void ReportDebugLogPrint(const char* format, ... );
static void AddLineEventLog(REPORT_MANAGER_TASK *Report, ReportMessage *ReportMsgPtr);
static char* SetLogTimeStamp(ReportMessage *ReportMsgPtr);
static void HandleStatusReportPrint(REPORT_MANAGER_TASK *Report, ReportMessage *ReportMsgPtr);
static void HandleReportLogReset(REPORT_MANAGER_TASK *Report);

#ifdef WIN32
static DWORD WINAPI THRepMeneger(LPVOID Data);
#else
static void* THRepMeneger(void *Data);
#endif

#ifdef _REPORT_LOG_WEB_
static void ParseWebReqLog(REPORT_MANAGER_TASK	*Report, PARUSERWEBTASK *WebTask);
static void FormNameLogRequest( char *TestName,REQGETCURRLOG *ReqGetNameLog );
static void CreateWebPageLog( char *StrHTM, char *RepFile, PARUSERWEBTASK* WebTask );
static void PrepareBlockLogFile( PROCFORMWEBLOG *Load );
static void CreateWebPageListLog( char *StrHTM, char *UserLogPath, PARUSERWEBTASK* WebTask );
#endif
//---------------------------------------------------------------------------
static void ReportMsgQueueCreate(ReportMsgQueue *MsgQueuePtr)
{
    ReportDebugLogPrint("Initializing report messages queue.");
#ifdef WIN32
	MsgQueuePtr->mutex = CreateMutex(NULL, FALSE, NULL);
    if (MsgQueuePtr->mutex == NULL) 
    {
        printf("Create report msg queue mutex error: %d\r\n", GetLastError());
	    exit(EXIT_FAILURE);
    }
#else
    pthread_mutex_init(&(MsgQueuePtr->mutex), NULL);
    pthread_cond_init(&(MsgQueuePtr->cond_var), NULL);
#endif
    PoolListInit(&MsgQueuePtr->queue_list, INIT_REPORT_MSG_COUNT);
    MsgQueuePtr->destroy = false;
}
//---------------------------------------------------------------------------
static void ReportMsgQueueMarkDestroy(ReportMsgQueue *MsgQueuePtr)
{
    ReportDebugLogPrint("marking report message queue for destroy.");
    MsgQueuePtr->destroy = true;
    ReportMsgQueuePost(MsgQueuePtr, NULL);
}
//---------------------------------------------------------------------------
static void ReportMsgQueuePost(ReportMsgQueue *MsgQueuePtr, void *p_message)
{
#ifdef WIN32
	DWORD       WaitResult;
	bool        isRMFreeReady = false;

    if ((WaitResult = WaitForSingleObject(MsgQueuePtr->mutex, INFINITE)) == WAIT_FAILED)
	{
		printf("Fail to set mutex (ReportMsgQueuePost)\r\n");
        return;
	}
	AddPoolStructList(&MsgQueuePtr->queue_list, p_message);
    if (!ReleaseMutex(MsgQueuePtr->mutex)) 
        printf("Fail to release mutex (ReportMsgQueuePost)\r\n");
    WinThreadMsgSend(MsgQueuePtr->ThrReportMen, RMU_REPORTDATA, 0, 0);
#else
    pthread_mutex_lock(&(MsgQueuePtr->mutex));
    AddPoolStructList(&MsgQueuePtr->queue_list, p_message);
    pthread_mutex_unlock(&(MsgQueuePtr->mutex));
    pthread_cond_signal(&(MsgQueuePtr->cond_var));
#endif
}
//---------------------------------------------------------------------------
static void ReportMsgQueueDestroy(ReportMsgQueue *MsgQueuePtr)
{
    ReportDebugLogPrint("De-initializing report messages queue.");
#ifdef WIN32
	CloseHandle(MsgQueuePtr->mutex);
#else
    pthread_mutex_destroy(&(MsgQueuePtr->mutex));
    pthread_cond_destroy(&(MsgQueuePtr->cond_var));
#endif
    DestroyPoolListStructs(&MsgQueuePtr->queue_list);
}
//---------------------------------------------------------------------------
static bool GetReportMessageQueue(ReportMsgQueue *MsgQueuePtr, void **pp_message)
{
    ObjPoolListTask *ObjPtr = NULL;
    bool status = true;
#ifdef WIN32
	DWORD         rc;
    struct tagMSG SysMsg;
#endif

    *pp_message = NULL;
#ifdef WIN32
    if ((rc = WaitForSingleObject(MsgQueuePtr->mutex, INFINITE)) == WAIT_FAILED)
	{
		printf("Fail to set mutex (GetReportMessageQueue)\r\n");
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
                printf("Fail to release mutex (GetReportMessageQueue)\r\n");
			GetMessage(&SysMsg, NULL, RMU_REPORTDATA, RMU_REPORTDATA);
#else
            pthread_cond_wait(&(MsgQueuePtr->cond_var), &(MsgQueuePtr->mutex));
#endif
#ifdef WIN32
            if ((rc = WaitForSingleObject(MsgQueuePtr->mutex, INFINITE)) == WAIT_FAILED)
	        {
		        printf("Fail to set mutex (GetReportMessageQueue)\r\n");
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
        ReportDebugLogPrint("wokeup due to destroy flag.");
        status = false;
    }
#ifdef WIN32
    if (!ReleaseMutex(MsgQueuePtr->mutex)) 
        printf("Fail to release mutex (GetReportMessageQueue)\r\n");
#else
    pthread_mutex_unlock(&(MsgQueuePtr->mutex));
#endif
    return status;
}
//---------------------------------------------------------------------------
static ReportMessage* GetReportMsgPool(unsigned char MsgTag)
{
    ReportMessage      *ReportMsgPtr = NULL;
    POOL_RECORD_STRUCT *ObjTaskPtr = NULL;
#ifdef WIN32
	DWORD              rc;
#endif

#ifdef WIN32
    if ((rc = WaitForSingleObject(gReportMsgAccess, INFINITE)) == WAIT_FAILED)
	{
		printf("Fail to set Report access mutex (GetReportMsgPool)\r\n");
        return NULL;
	}
#else
    pthread_mutex_lock(&gReportMsgAccess);
#endif
    if (gListReportMsg.Count < MAX_LOG_MSG_POOL)
    {
        ObjTaskPtr = GetBuffer(&gReportMsgPool);
        if (!ObjTaskPtr)
        {
            ReportDebugLogPrint("No buffers for report message delivery.");
            gLostReportMsgs++;
#ifdef WIN32
            if (!ReleaseMutex(gReportMsgAccess)) 
                printf("Fail to release Report access mutex (GetReportMsgPool)\r\n");
#else
            pthread_mutex_unlock(&gReportMsgAccess);            
#endif
            return NULL;
        }
    }
    else
    {
        ReportDebugLogPrint("Max number of report messages in queuq is reached.");
        gLostReportMsgs++;
#ifdef WIN32
        if (!ReleaseMutex(gReportMsgAccess)) 
            printf("Fail to release mutex (GetReportMsgPool)\r\n");
#else
        pthread_mutex_unlock(&gReportMsgAccess);
#endif
        return NULL;    
    }
    ReportMsgPtr = (ReportMessage*)ObjTaskPtr->DataPtr;
    ReportMsgPtr->MsgTag = MsgTag;
    ReportMsgPtr->BlkPoolPtr = ObjTaskPtr;
    ReportMsgPtr->ObjPtr = AddPoolStructListObj(&gListReportMsg, ReportMsgPtr);
    if (gMaxSimultReportMsgs < (unsigned int)gListReportMsg.Count)
        gMaxSimultReportMsgs = (unsigned int)gListReportMsg.Count;
#ifdef WIN32
    if (!ReleaseMutex(gReportMsgAccess)) 
        printf("Fail to release mutex (GetReportMsgPool)\r\n");
#else
    pthread_mutex_unlock(&gReportMsgAccess);
#endif
    return ReportMsgPtr;
}
//---------------------------------------------------------------------------
static void FreeReportMsgPool(ReportMessage *ReportMsgPtr)
{
#ifdef WIN32
	DWORD  rc;

    if ((rc = WaitForSingleObject(gReportMsgAccess, INFINITE)) == WAIT_FAILED)
	{
		printf("Fail to set Report Access mutex (GetReportMsgPool)\r\n");
        return;
	}
#else
    pthread_mutex_lock(&gReportMsgAccess);
#endif
    FreeBuffer(&gReportMsgPool, ReportMsgPtr->BlkPoolPtr);
    RemPoolStructList(&gListReportMsg, ReportMsgPtr->ObjPtr);
#ifdef WIN32
    if (!ReleaseMutex(gReportMsgAccess)) 
        printf("Fail to release Report Access mutex (GetReportMsgPool)\r\n");
#else
    pthread_mutex_unlock(&gReportMsgAccess);
#endif
}
//---------------------------------------------------------------------------
#ifdef WIN32
static DWORD WINAPI THRepMeneger(LPVOID Data)
#else
static void* THRepMeneger(void *Data)
#endif
{
	REPORT_MANAGER_TASK	Report;	   
    ReportMessage       *ReportMsgPtr = NULL;
	char                *CwdRet = NULL;
#ifdef WIN32
    struct _SYSTEMTIME  CurrTime;
#else
    struct timeb        hires_cur_time;
    struct tm           *cur_time; 
#endif

	Report.MainParRep = (PARINIREPMEN*)Data;
#ifdef WIN32
	CwdRet = _getcwd(Report.StartDir,512);
	strcat(Report.StartDir,"\\LogFiles\\");
    GetLocalTime(&CurrTime);
	Report.DbgLastDayOfWeek = (unsigned char)CurrTime.wDayOfWeek;
#else
	CwdRet = getcwd(Report.StartDir,512);
	strcat(Report.StartDir,"/LogFiles/");
    ftime(&hires_cur_time);
    cur_time = localtime(&hires_cur_time.time);
	Report.DbgLastDayOfWeek = (unsigned char)cur_time->tm_wday;
#endif
	Report.EvtLastDayOfWeek = Report.DbgLastDayOfWeek;
	strcpy(Report.PathRepFiles, Report.StartDir);
	strcat(Report.PathRepFiles, "ServerDebug_");
	strcat(Report.PathRepFiles, TablDefNameDey[Report.DbgLastDayOfWeek]);
    strcat(Report.PathRepFiles, ".log");
    strcpy(Report.PathRepEventLog,Report.StartDir);
	strcat(Report.PathRepEventLog,"ServerEvent_");
	strcat(Report.PathRepEventLog, TablDefNameDey[Report.EvtLastDayOfWeek]);
	strcat(Report.PathRepEventLog,".log");

    Report.HandleRep = fopen(Report.PathRepFiles,"ab");
	if (Report.HandleRep == NULL)
        Report.HandleRep = fopen(Report.PathRepFiles,"wb");

  #ifdef WIN32
    CreateThreadTimerCB(OnReportTimerExp, GetCurrentThreadId(), 10*TMR_ONE_SEC, DATE_CHANGE_CHECK_TMR_ID, true);
  #else
    CreateThreadTimerCB(OnReportTimerExp, REPORT_MSG_RX_PORT, 10*TMR_ONE_SEC, DATE_CHANGE_CHECK_TMR_ID, true);
  #endif

    printf("%s Report manager is ready to handle requests\n", SetTimeStampLine());

    /* Read the data from the report messages queue. */
    while (GetReportMessageQueue(&g_report_queue, (void**)&ReportMsgPtr))
    {
	    if(Report.MainParRep->StopReportFlag) break;        
        switch(ReportMsgPtr->MsgTag)
        {
	        case RPMN_ADD_STR_REQ:
                if (Report.HandleRep) fwrite(ReportMsgPtr->LogLine, strlen(ReportMsgPtr->LogLine), 1, Report.HandleRep);
			    break;

	        case RPMN_ADD_STR_EVENT_REQ:
                AddLineEventLog(&Report, ReportMsgPtr);
			    break;
            
            case RPMN_PRINT_STATUS:
                HandleStatusReportPrint(&Report, ReportMsgPtr);
                break;

#ifdef _REPORT_LOG_WEB_
		    case RPMN_WEB_REQ_LOG:
			    ParseWebReqLog(&Report, (PARUSERWEBTASK*)ReportMsgPtr->WebReq);
			    break;
#endif
               
            case RPMN_DATE_CHG_TMR:
                LogDateChangeCheck(&Report);
                break;

			case RPMN_RESET_LOG_REQ:
				HandleReportLogReset(&Report);
				break;

            default:
                ReportDebugLogPrint("Unexpected(0x%02X) message tag is received", ReportMsgPtr->MsgTag);
                break;
        }
        FreeReportMsgPool(ReportMsgPtr);
    }
  #ifdef WIN32
	CloseThreadTimer(GetCurrentThreadId(), DATE_CHANGE_CHECK_TMR_ID);
  #else
	CloseThreadTimer(REPORT_MSG_RX_PORT, DATE_CHANGE_CHECK_TMR_ID);
  #endif
    fflush(Report.HandleRep);
    fclose(Report.HandleRep);
    printf("%s Report manager thread was stopped\n", SetTimeStampLine());    
#ifdef WIN32
    ExitThread(0);
    return 0;
#else
    pthread_exit((void *)0);
#endif
}
//---------------------------------------------------------------------------
static void OnReportTimerExp(unsigned int TimerId)
{
    ReportMessage *ReportMsgPtr = NULL;

    ReportMsgPtr = GetReportMsgPool(RPMN_DATE_CHG_TMR);
    if (!ReportMsgPtr) return;
    ReportMsgQueuePost(&g_report_queue, ReportMsgPtr);
}
//---------------------------------------------------------------------------
static void LogDateChangeCheck(REPORT_MANAGER_TASK *Report)
{
	unsigned char CurrDayOfWeek;
#ifdef WIN32
    SYSTEMTIME    hires_cur_time;
#else
    struct timeb  hires_cur_time;
    struct tm     *cur_time; 
#endif

	if (Report->HandleRep) fflush(Report->HandleRep);
#ifdef WIN32
    if (WaitForSingleObject(gMemoryMutex, INFINITE) == WAIT_FAILED)
	{
		printf("LogDateChangeCheck - Fail to get memory mutex\r\n");
        return;
	}
	GetSystemTime(&hires_cur_time);
	CurrDayOfWeek = (unsigned char)(hires_cur_time.wDayOfWeek);
    if (!ReleaseMutex(gMemoryMutex)) 
        printf("LogDateChangeCheck - Fail to release memory mutex\r\n");
#else
    sem_wait(&gMemoryMutex);
    ftime(&hires_cur_time);
    cur_time = localtime(&hires_cur_time.time);
	CurrDayOfWeek = (unsigned char)(cur_time->tm_wday);
    sem_post(&gMemoryMutex);
#endif
	if (CurrDayOfWeek == Report->DbgLastDayOfWeek) return;
    
    /* Date was changed - needs to open new file */
    fclose(Report->HandleRep);
    strcpy(Report->PathRepFiles, Report->StartDir);
	strcat(Report->PathRepFiles, "ServerDebug_");
	strcat(Report->PathRepFiles, TablDefNameDey[CurrDayOfWeek]);
    strcat(Report->PathRepFiles, ".log");
	Report->HandleRep = fopen(Report->PathRepFiles,"wb");
	Report->DbgLastDayOfWeek = CurrDayOfWeek;
}
//---------------------------------------------------------------------------
static void AddLineEventLog(REPORT_MANAGER_TASK *Report, ReportMessage *ReportMsgPtr)
{
	unsigned char CurrDayOfWeek;
	FILE		  *HandleRep;
#ifdef WIN32
	DWORD         rc;
    SYSTEMTIME    hires_cur_time;
#else
    struct timeb  hires_cur_time;
    struct tm     *cur_time; 
#endif

#ifdef WIN32
    if ((rc = WaitForSingleObject(gMemoryMutex, INFINITE)) == WAIT_FAILED)
	{
		printf("AddLineEventLog - Fail to get memory mutex\r\n");
        return;
	}
	GetSystemTime(&hires_cur_time);
	CurrDayOfWeek = (unsigned char)(hires_cur_time.wDayOfWeek);
    if (!ReleaseMutex(gMemoryMutex)) 
        printf("AddLineEventLog - Fail to release memory mutex\r\n");
#else
    sem_wait(&gMemoryMutex);
    ftime(&hires_cur_time);
    cur_time = localtime(&hires_cur_time.time);
	CurrDayOfWeek = (unsigned char)(cur_time->tm_wday);
    sem_post(&gMemoryMutex);
#endif
	if (CurrDayOfWeek != Report->EvtLastDayOfWeek)
	{
        strcpy(Report->PathRepEventLog,Report->StartDir);
	    strcat(Report->PathRepEventLog,"ServerEvent_");
	    strcat(Report->PathRepEventLog, TablDefNameDey[CurrDayOfWeek]);
	    strcat(Report->PathRepEventLog,".log");
		HandleRep = fopen(Report->PathRepEventLog,"wb");
		Report->EvtLastDayOfWeek = CurrDayOfWeek;
	}
	else
	{
        HandleRep = fopen(Report->PathRepEventLog,"ab");
		if (HandleRep == NULL)
            HandleRep = fopen(Report->PathRepEventLog,"wb");
    }

	if (HandleRep)
	{
		fwrite(ReportMsgPtr->LogLine, strlen(ReportMsgPtr->LogLine), 1, HandleRep);
	    fclose(HandleRep);
	}
}
//---------------------------------------------------------------------------
static char* SetLogTimeStamp(ReportMessage *ReportMsgPtr)
{
#ifdef WIN32
	SYSTEMTIME   CurrTime;
	DWORD        rc;
#else       
    struct timeb hires_cur_time;
    struct tm    *cur_time;     
#endif

	*ReportMsgPtr->LogLine = 0;
#ifdef WIN32
    if ((rc = WaitForSingleObject(gMemoryMutex, INFINITE)) == WAIT_FAILED)
	{
		printf("SetLogTimeStamp - Fail to get memory mutex\r\n");
        return ReportMsgPtr->LogLine;
	}
    GetSystemTime(&CurrTime);
    sprintf(ReportMsgPtr->LogLine, "%02d/%02d/%04d-%02d:%02d:%02d.%03d | ",
		CurrTime.wDay, CurrTime.wMonth, CurrTime.wYear,
		CurrTime.wHour, CurrTime.wMinute, 
		CurrTime.wSecond, CurrTime.wMilliseconds);  
    if (!ReleaseMutex(gMemoryMutex)) 
        printf("SetLogTimeStamp - Fail to release memory mutex\r\n");
#else
    sem_wait(&gMemoryMutex);
    ftime(&hires_cur_time);
    cur_time = localtime(&hires_cur_time.time);
    sprintf(ReportMsgPtr->LogLine, "%02d/%02d/%04d-%02d:%02d:%02d.%03d | ",
           cur_time->tm_mday, (cur_time->tm_mon+1),
           (cur_time->tm_year+1900), cur_time->tm_hour,
           cur_time->tm_min, cur_time->tm_sec, hires_cur_time.millitm); 
    sem_post(&gMemoryMutex);
#endif   
    return &ReportMsgPtr->LogLine[strlen(ReportMsgPtr->LogLine)]; 
}
//---------------------------------------------------------------------------
static void HandleReportLogReset(REPORT_MANAGER_TASK *Report)
{
	if (Report->HandleRep) fclose(Report->HandleRep);
    Report->HandleRep = fopen(Report->PathRepFiles,"wb");
}
//---------------------------------------------------------------------------
static void HandleStatusReportPrint(REPORT_MANAGER_TASK *Report, ReportMessage *ReportMsgPtr)
{
    char *LogBodyPtr = NULL;
    
    LogBodyPtr = SetLogTimeStamp(ReportMsgPtr);
    sprintf(LogBodyPtr, "Report status: Max RMQ size: %u, Lost RMQ: %u, Curr RMQ: %u\n", 
        gMaxSimultReportMsgs, gLostReportMsgs, (unsigned int)gListReportMsg.Count);  
    if (Report->HandleRep) fwrite(LogBodyPtr, strlen(LogBodyPtr), 1, Report->HandleRep);   
}
//---------------------------------------------------------------------------
void EventLogPrint(ThrMsgChanInfo *ReqThrChan, const char* format, ... )
{
    ReportMessage *ReportMsgPtr = NULL;
    char          *LogBodyPtr = NULL;
    va_list       args;

    ReportMsgPtr = GetReportMsgPool(RPMN_ADD_STR_EVENT_REQ);
    if (!ReportMsgPtr) return;
    *ReportMsgPtr->LogLine = 0;
    va_start( args, format );
    LogBodyPtr = SetLogTimeStamp(ReportMsgPtr);
	vsprintf(LogBodyPtr, format, args);
    va_end( args );
    ReportMsgQueuePost(&g_report_queue, ReportMsgPtr);
}
//---------------------------------------------------------------------------
void DebugLogPrint(ThrMsgChanInfo *ReqThrChan, const char* format, ... )
{
    ReportMessage *ReportMsgPtr = NULL;
    char          *LogBodyPtr = NULL;
    va_list       args;

    ReportMsgPtr = GetReportMsgPool(RPMN_ADD_STR_REQ);
    if (!ReportMsgPtr) return;
    *ReportMsgPtr->LogLine = 0;
    
    va_start(args, format);
    LogBodyPtr = SetLogTimeStamp(ReportMsgPtr);
	vsprintf(LogBodyPtr, format, args);
    va_end( args );
    ReportMsgQueuePost(&g_report_queue, ReportMsgPtr);
}
//---------------------------------------------------------------------------
void DebugLogStrPrint(char *StrPtr)
{
    ReportMessage *ReportMsgPtr = NULL;

	if (!StrPtr) return;
    ReportMsgPtr = GetReportMsgPool(RPMN_ADD_STR_REQ);
    if (!ReportMsgPtr) return;
	if (strlen(StrPtr) < MAX_LOG_LINE_LEN) strcpy(ReportMsgPtr->LogLine, StrPtr);
	else
	{
		memcpy(ReportMsgPtr->LogLine, StrPtr, MAX_LOG_LINE_LEN);
		ReportMsgPtr->LogLine[MAX_LOG_LINE_LEN] = 0;
    }
    ReportMsgQueuePost(&g_report_queue, ReportMsgPtr);
}
//---------------------------------------------------------------------------
void DebugLogStrBufPrint(ThrMsgChanInfo *ReqThrChan, char *StrBuf, const char* format, ... )
{        
    ReportMessage *ReportMsgPtr = NULL;
    char          *LogBodyPtr = NULL;
	unsigned int  MaxAddStrLen = 0;
	unsigned int  FirstPartLen = 0;
	unsigned int  AddTextLen = 0;
    va_list       args;

    ReportMsgPtr = GetReportMsgPool(RPMN_ADD_STR_REQ);
    if (!ReportMsgPtr) return;
    *ReportMsgPtr->LogLine = 0;

    va_start( args, format );
    LogBodyPtr = SetLogTimeStamp(ReportMsgPtr);
	vsprintf((char*)LogBodyPtr, format, args);
	FirstPartLen = strlen(ReportMsgPtr->LogLine);
    MaxAddStrLen = (MAX_LOG_LINE_LEN+24) - (FirstPartLen + 8);
    AddTextLen = strlen(StrBuf);
	ReportMsgPtr->LogLine[FirstPartLen] = '{';
	if (AddTextLen < MaxAddStrLen)
	{
		strcpy(&ReportMsgPtr->LogLine[FirstPartLen+1], StrBuf);
		strcpy(&ReportMsgPtr->LogLine[FirstPartLen + AddTextLen + 1], "}\r\n");
	}
	else
	{
		memcpy(&ReportMsgPtr->LogLine[FirstPartLen+1], StrBuf, MaxAddStrLen);
		strcpy(&ReportMsgPtr->LogLine[FirstPartLen + MaxAddStrLen + 1], "...}\r\n");
	}
    va_end( args );
    ReportMsgQueuePost(&g_report_queue, ReportMsgPtr); 
}
//---------------------------------------------------------------------------
void ReportStatusPrint()
{
    ReportMessage *ReportMsgPtr = NULL;

    ReportMsgPtr = GetReportMsgPool(RPMN_PRINT_STATUS);
    if (!ReportMsgPtr) return;
    *ReportMsgPtr->LogLine = 0;
    ReportMsgQueuePost(&g_report_queue, ReportMsgPtr);
}
//---------------------------------------------------------------------------
void ReportLogReset()
{
    ReportMessage *ReportMsgPtr = NULL;

    ReportMsgPtr = GetReportMsgPool(RPMN_RESET_LOG_REQ);
    if (!ReportMsgPtr) return;
    *ReportMsgPtr->LogLine = 0;
    ReportMsgQueuePost(&g_report_queue, ReportMsgPtr);
}
//---------------------------------------------------------------------------
int ReportThreadCreate()
{
    int Res = 0;
#ifdef _LINUX_X86_
    pthread_attr_t	attr, *attrPtr = &attr;
    struct sched_param	sched;
    size_t StackSize = 1024*1024;
#endif

    PoolListInit(&gListReportMsg, INIT_REPORT_MSG_COUNT);
    CreatePool(&gReportMsgPool, INIT_REPORT_MSG_COUNT, sizeof(ReportMessage));
#ifdef WIN32
    gReportMsgAccess = CreateMutex(NULL, FALSE, NULL);
    if (gReportMsgAccess == NULL) 
    {
        printf("Create Report manager access mutex error: %d\r\n", GetLastError());
	    Res = -1;
    }
	else
	{
        ReportMsgQueueCreate(&g_report_queue);
        ParRepMen.StopReportFlag = false;
        ParRepMen.HtrReportMen = 0;
        ParRepMen.ThrReportMen = 0;
        ParRepMen.HtrReportMen = CreateThread( NULL, 0, (LPTHREAD_START_ROUTINE)THRepMeneger,
                            (LPVOID)(&ParRepMen), 0, (LPDWORD)&ParRepMen.ThrReportMen);
		g_report_queue.ThrReportMen = ParRepMen.ThrReportMen;
	}
#else    
    pthread_mutex_init(&gReportMsgAccess, NULL);
    ReportMsgQueueCreate(&g_report_queue);
    ParRepMen.StopReportFlag = false;
    pthread_attr_init(attrPtr);
    (void)pthread_attr_setstacksize (attrPtr, StackSize);
    pthread_attr_setdetachstate(attrPtr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(attrPtr, PTHREAD_SCOPE_SYSTEM);
    if (pthread_attr_getschedparam(attrPtr, &sched) == 0)
    {
	    sched.sched_priority = 0;
	    pthread_attr_setschedparam(attrPtr, &sched);
    }	
    printf("Report manager sturtup\n");	
    if (pthread_create(&ParRepMen.report_thr, &attr, &THRepMeneger, &ParRepMen) != 0)
    {
	    printf("Report manager thread create with %d error!\n", errno);
        Res = -1;
    }
#endif
    return Res;
}
//---------------------------------------------------------------------------
void ReportThreadClose()
{
    ReportMessage    *ReportMsgPtr = NULL;
    ObjPoolListTask  *PointTask = NULL;
#ifdef WIN32
	DWORD            rc;
#endif

    ParRepMen.StopReportFlag = true;
    ReportMsgQueueMarkDestroy(&g_report_queue);
#ifdef WIN32    
    if (ParRepMen.ThrReportMen)
        WaitCloseProcess(ParRepMen.HtrReportMen);
    CloseHandle(ParRepMen.HtrReportMen);
    ParRepMen.ThrReportMen = 0;
    ParRepMen.HtrReportMen = 0;
    if ((rc = WaitForSingleObject(gReportMsgAccess, INFINITE)) == WAIT_FAILED)
	{
		printf("AddLineDebugLog - Fail to get Report Msg Access mutex\r\n");
        return;
	}
#else
    pthread_join(ParRepMen.report_thr, NULL);
    Sleep(100);
    pthread_mutex_lock(&gReportMsgAccess);
#endif
    while (gListReportMsg.Count)
    {
        PointTask = (ObjPoolListTask*)gListReportMsg.FistTask;
        ReportMsgPtr = (ReportMessage*)PointTask->UsedTask;
        RemPoolStructList(&gListReportMsg, PointTask);
    }
#ifdef WIN32
    if (!ReleaseMutex(gReportMsgAccess)) 
        printf("ReportThreadClose - Fail to release Report Msg Access mutex\r\n");
#else
    pthread_mutex_unlock(&gReportMsgAccess);
#endif
    DestroyPoolListStructs(&gListReportMsg);
    DestroyPool(&gReportMsgPool);    
#ifdef WIN32
	CloseHandle(gReportMsgAccess);
#else
    pthread_mutex_destroy(&gReportMsgAccess);
#endif
    printf("Report manager close is completed\r\n");
    return;
}
//---------------------------------------------------------------------------
static void ReportDebugLogPrint(const char* format, ... )
{
#ifdef _REPORT_DEBUG_
    char         *CwdRet = NULL;
  #ifdef WIN32
	SYSTEMTIME   CurrTime;
	DWORD        WaitResult;
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
	strcat(FileName, "\\LogFiles\\ReportThread.log");
  #endif
  #ifdef _LINUX_X86_
	strcat(FileName, "/LogFiles/ReportThread.log");
  #endif

  #ifdef WIN32
    if ((WaitResult = WaitForSingleObject(gFileMutex, INFINITE)) == WAIT_FAILED)
	{
		printf("Fail to set file access mutex (ReportDebugLogPrint)\r\n");
        return;
	}
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
        printf("Fail to release file access mutex (ReportDebugLogPrint)\r\n");
  #endif
    va_end( args );   
#endif
}
//-----------------------------------------------------------------------
#ifdef _REPORT_LOG_WEB_
void ReportWebMgtReq(PARUSERWEBTASK *WebReq)
{
    ReportMessage *ReportMsgPtr = NULL;

    ReportMsgPtr = GetReportMsgPool(RPMN_WEB_REQ_LOG);
    if (!ReportMsgPtr) return;
    *ReportMsgPtr->LogLine = 0;
    ReportMsgPtr->WebReq = WebReq;
    ReportMsgQueuePost(&g_report_queue, ReportMsgPtr);
}
//---------------------------------------------------------------------------
static void ParseWebReqLog(REPORT_MANAGER_TASK	*Report, PARUSERWEBTASK *WebTask)
{
	char	*StrHTM;
	char	*UserLogFile;
    char    *FileNamePtr = NULL;
	unsigned int	l,k;

	StrHTM = (char*)AllocateMemory(2*SIZE_FORM_BUF_HTTP);
	FormHeaderGoodAnswerWWW( StrHTM );
	k = FindCmdRequest( (char*)WebTask->RequestCommand, "logcmd=");
	if ( k != -1 )
      {switch ( FindCmdArray( &WebTask->RequestCommand[k], TablRepMenCmd, 4 ) )
         {case -1:	break;
          case  0:
			CreateWebPageLog( StrHTM, Report->PathRepFiles, WebTask );
			break;
		  case  1:
			CreateWebPageLog( StrHTM, Report->PathRepEventLog, WebTask );
			break;
		  case  2:
			CreateWebPageListLog( StrHTM, Report->StartDir, WebTask );
			break;
		  case  3:
			l = FindCmdRequest( &WebTask->RequestCommand[k], "logname=");
			if ( k != -1 )
			  {UserLogFile = (char*)AllocateMemory( 1024 );
			   strcpy(UserLogFile, Report->StartDir);
#ifdef WIN32               
			   strcat(UserLogFile,"\\");	
#endif               
#ifdef _LINUX_X86_               
			   strcat(UserLogFile,"/");	
#endif             
               FileNamePtr = &UserLogFile[strlen(UserLogFile)];
			   ConverStrFormRequest(FileNamePtr, &WebTask->RequestCommand[k+l], strlen(&WebTask->RequestCommand[k+l]) );
#ifdef WIN32 
               if (FindCmdRequest(FileNamePtr, ".\\") != -1)
#endif
#ifdef _LINUX_X86_ 
               if (FindCmdRequest(FileNamePtr, "./") != -1)
#endif            
                   CreateWebPageLog(StrHTM, Report->PathRepEventLog, WebTask);
               else
			       CreateWebPageLog( StrHTM, UserLogFile, WebTask );               
			   FreeMemory(UserLogFile );
			  }	
			break;
		 }
	  }
	else  CreateWebPageLog( StrHTM, Report->PathRepFiles, WebTask );
	l = strlen(StrHTM);
	if ( l ) send( WebTask->WorkSocket, StrHTM, l, 0 );
#ifdef WIN32
	closesocket( WebTask->WorkSocket );
#endif
#ifdef _LINUX_X86_
	shutdown(WebTask->WorkSocket, SHUT_RDWR);
	close(WebTask->WorkSocket);
#endif
	if ( WebTask->LocalAdress )		FreeMemory( WebTask->LocalAdress );
	if ( WebTask->RequestCommand )	FreeMemory( WebTask->RequestCommand );
	FreeMemory( WebTask );
	FreeMemory( StrHTM );
	return;
}
//---------------------------------------------------------------------------
static void FormNameLogRequest( char *TestName,REQGETCURRLOG *ReqGetNameLog )
{
	int	i,l;

	if (!TestName ) {ReqGetNameLog->LogName = 0;return;}
	l = strlen( TestName );
	for (i=l;i > 0;i--)
	   {if ( TestName[i-1] == '\\')  
		  break;
	   }
	l = strlen( &TestName[i] );
	ReqGetNameLog->LogName = (char*)AllocateMemory(l+4);
	strcpy(ReqGetNameLog->LogName,&TestName[i]);
	for (i=0;i < (int)strlen(ReqGetNameLog->LogName);i++)
	   {if ( ReqGetNameLog->LogName[i] == '.')
		  {ReqGetNameLog->LogName[i] = 0;
		   break;
		  }
	   }
	return;
}
//---------------------------------------------------------------------------
static void CreateWebPageLog( char *StrHTM, char *RepFile, PARUSERWEBTASK* WebTask )
{
	FILE	*HandleRep;
	char	*BufLoad;
	unsigned int Readed;
	PROCFORMWEBLOG LoadLog;
#ifdef WIN32
	bool    isMutexReady = false;
    DWORD   WaitResult;
#endif

	strcat(StrHTM,"<HTML><HEAD><TITLE>LMS server system logs.</TITLE>\r\n");
	strcat(StrHTM,"<meta HTTP-EQUIV=\"Cache-Control\" CONTENT=\"no-cache\">\r\n");
    strcat(StrHTM,"</HEAD>\r\n");
	strcat(StrHTM,"<BODY BACKGROUND=\"images/groundLink.gif\" bgcolor=#FFFFFF TEXT=\"000000\"><CENTER>\r\n");
	strcat(StrHTM,"<a href=\"");
	strcat(StrHTM, WebTask->ServerHttpAddr);
    strcat(StrHTM,GenPageLogMgr);
	strcat(StrHTM,WebTask->PageIdentityStr);
	strcat(StrHTM,"&logcmd=vlj\">Current debug log.</a>&nbsp;|&nbsp;\r\n");
	strcat(StrHTM,"<a href=\"");
	strcat(StrHTM, WebTask->ServerHttpAddr);
    strcat(StrHTM,GenPageLogMgr);
	strcat(StrHTM,WebTask->PageIdentityStr);
	strcat(StrHTM,"&logcmd=vle\">Current event log.</a>&nbsp;|&nbsp;\r\n");
	strcat(StrHTM,"<a href=\"");
	strcat(StrHTM, WebTask->ServerHttpAddr);
    strcat(StrHTM,GenPageLogMgr);
	strcat(StrHTM,WebTask->PageIdentityStr);
	strcat(StrHTM,"&logcmd=llf\">List of log files.</a>&nbsp;|&nbsp;\r\n");
	strcat(StrHTM,"<a href=\"");
	strcat(StrHTM, WebTask->ServerHttpAddr);
	strcat(StrHTM,GenPageMain);
    strcat(StrHTM,WebTask->PageIdentityStr);
	strcat(StrHTM,"\">Back to main menu.</a></CENTER>\r\n");
	strcat(StrHTM,"<hr width=100%>\r\n");
	strcat(StrHTM,"<b>Src: </b>");
	strcat(StrHTM,RepFile);
	strcat(StrHTM,"\r\n");
	strcat(StrHTM,"<hr width=100%><TT>\r\n");

#ifdef WIN32
    WaitResult = WaitForSingleObject(gFileMutex, INFINITE);
    switch(WaitResult)
	{
        case WAIT_OBJECT_0:
			isMutexReady = true;
			break;

        case WAIT_ABANDONED: 
			printf("The other thread that using mutex is closed in locked state of mutex\r\n");
            break;

		default:
			printf("Report manager (web req) mutex is fialed with error: %d\r\n", GetLastError());
			break;
	}
	if (isMutexReady)
    {
#endif
	HandleRep = fopen(RepFile,"rb");
	if ( HandleRep != NULL )
	{
	   BufLoad = (char*)AllocateMemory(4098);
       Readed = fread( BufLoad, 1, 4000, HandleRep);
	   LoadLog.Data = BufLoad;
	   LoadLog.FormStr = (char*)AllocateMemory(8200);
	   LoadLog.HTTPStr = StrHTM;
	   LoadLog.LineSrc = Readed;
	   LoadLog.LineLogStr = 0;
	   LoadLog.LastRot = false;
	   while ( 1 )
         {LoadLog.LineSrc = Readed;
		  PrepareBlockLogFile( &LoadLog );
		  send( WebTask->WorkSocket, StrHTM, strlen(StrHTM), 0 );
		  StrHTM[0] = 0;	
		  if ( Readed < 4000) break;
		  Readed = fread( BufLoad, 1, 4000, HandleRep);
		 }		
	   fclose(HandleRep);
	   FreeMemory( LoadLog.FormStr );	
	   FreeMemory( BufLoad );
	}
#ifdef WIN32
	}
    if (! ReleaseMutex(gFileMutex)) 
	{ 
        printf("Fail to release mutex (web req) in report manager task\r\n");
	}
#endif
	strcat(StrHTM,"</TT><hr width=100%><CENTER>\r\n");
	strcat(StrHTM,"<a href=\"");
	strcat(StrHTM, WebTask->ServerHttpAddr);
	strcat(StrHTM,GenPageLogMgr);
	strcat(StrHTM,WebTask->PageIdentityStr);
	strcat(StrHTM,"&logcmd=vlj\">Current debug log.</a>&nbsp;|&nbsp;\r\n");
	strcat(StrHTM,"<a href=\"");
	strcat(StrHTM, WebTask->ServerHttpAddr);
	strcat(StrHTM,GenPageLogMgr);
	strcat(StrHTM,WebTask->PageIdentityStr);
	strcat(StrHTM,"&logcmd=vle\">Current event log.</a>&nbsp;|&nbsp;\r\n");
	strcat(StrHTM,"<a href=\"");
	strcat(StrHTM, WebTask->ServerHttpAddr);
	strcat(StrHTM,GenPageLogMgr);
	strcat(StrHTM,WebTask->PageIdentityStr);
	strcat(StrHTM,"&logcmd=llf\">List of log files.</a>&nbsp;|&nbsp;\r\n");
	strcat(StrHTM,"<a href=\"");
	strcat(StrHTM, WebTask->ServerHttpAddr);
	strcat(StrHTM,GenPageMain);
    strcat(StrHTM,WebTask->PageIdentityStr);
	strcat(StrHTM,"\">Back to main menu.</a>\r\n");
	strcat(StrHTM,"</CENTER></BODY></HTML>");
	return;
}
//---------------------------------------------------------------------------
static void PrepareBlockLogFile( PROCFORMWEBLOG *Load )
{
	unsigned        PointFind,i;

	PointFind = 0;
    while ( PointFind < Load->LineSrc )
     {if ( Load->Data[PointFind] =='\r' || Load->Data[PointFind] =='\n')
        {if ( Load->LastRot ) goto NoMove;
         Load->LastRot = true;
         Load->FormStr[Load->LineLogStr] = 0;
		 strcat(Load->HTTPStr,Load->FormStr);
		 strcat(Load->HTTPStr,"<br>");
		 Load->LineLogStr = 0;
		 PointFind++;
	    }
       else
        {
   NoMove:
         if (Load->Data[PointFind] !='\r' && Load->Data[PointFind] !='\n')
           {switch ( Load->Data[PointFind] )
				{case 0x09:
					Load->FormStr[Load->LineLogStr] = 0;
					for (i=0;i < 8;i++) strcat( &Load->FormStr[Load->LineLogStr], "&nbsp;" );
					Load->LineLogStr += 48;
					break;

				 case ' ':
					Load->FormStr[Load->LineLogStr] = 0;
					strcat( &Load->FormStr[Load->LineLogStr], "&nbsp;" );
					Load->LineLogStr += 6;
					break;

				 default:
					Load->FormStr[Load->LineLogStr] = Load->Data[PointFind];
					Load->LineLogStr++;
				}
           }
         PointFind++;
		 Load->LastRot = false;
        }
      }
    return;
}
//---------------------------------------------------------------------------
static void CreateWebPageListLog( char *StrHTM, char *UserLogPath, PARUSERWEBTASK* WebTask )
{
	unsigned	Num;
	unsigned long	FileSize;
	char		FormNum[16];
	char		*PathSearth = NULL;
    char        *FileNamePtr = NULL;
#ifdef WIN32
	HANDLE		HDIRFILE;
	WIN32_FIND_DATA Data_File;
#endif
#ifdef _LINUX_X86_
        DIR            *DirPtr;
        struct dirent  *DirEntPtr;
        struct stat    st;
#endif

	Num = 0;
	PathSearth = (char*)AllocateMemory(1024*sizeof(char));
	strcpy(PathSearth, UserLogPath);
#ifdef WIN32     
	strcat(PathSearth,"\\*.log");
#endif
	strcat(StrHTM,"<HTML><HEAD><TITLE>List log files.</TITLE>");
	strcat(StrHTM,"<meta HTTP-EQUIV=\"Cache-Control\" CONTENT=\"no-cache\">\r\n");
	strcat(StrHTM,"</HEAD>\r\n");
	strcat(StrHTM,"<BODY BACKGROUND=\"images/groundLink.gif\" bgcolor=#FFFFFF TEXT=\"000000\">\r\n");
	strcat(StrHTM,"<TABLE BORDER=0 CELLSPACING=0 CELLPADDING=0 align=center valign=center>\r\n");
	strcat(StrHTM,"<TR><td><TABLE BORDER=1 CELLSPACING=1 CELLPADDING=1 ALIGN=\"CENTER\">\r\n");
	strcat(StrHTM,"<TR valign=top bgcolor=#00ccff ALIGN=\"CENTER\" VALIGN=\"CENTER\">\r\n");
	strcat(StrHTM,"<TH  bgcolor=#C0C0C0>Nn</th><TH bgcolor=#C0C0C0>Key</th>\r\n<TH bgcolor=#C0C0C0 width=200>Log file</th>\r\n<TH bgcolor=#C0C0C0 width=200>Size file</th></TR>\r\n");
#ifdef WIN32        
	HDIRFILE = FindFirstFile(PathSearth,&Data_File);
	if (HDIRFILE != INVALID_HANDLE_VALUE)
        {
            while ( 1 )
	    {
                if ( !(Data_File.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) )
		{
                    FileSize = ((unsigned long)Data_File.nFileSizeHigh)<<16;
		    FileSize += (unsigned long)Data_File.nFileSizeLow;
		    strcat(StrHTM,"<tr><td><font size=\"-1\">");
		    sprintf(FormNum,"%d",Num+1);
		    strcat(StrHTM,FormNum);
		    strcat(StrHTM,"</font></td>\r\n");
		    strcat(StrHTM,"<th bgcolor=#00ff00>\r\n");
		    strcat(StrHTM,"<map name=\"SelScrFile");
		    sprintf(FormNum,"%d",Num);
		    strcat(StrHTM,FormNum);
		    strcat(StrHTM,"\"><area shape=rect coords=\"2,2,17,17\" href=\"");
		    strcat(StrHTM, WebTask->ServerHttpAddr);
		    strcat(StrHTM,GenPageLogMgr);
            strcat(StrHTM,WebTask->PageIdentityStr);
		    strcat(StrHTM,"&logcmd=glf&logname=");
		    strcat(StrHTM,(char*)&Data_File.cFileName );
		    strcat(StrHTM,"\" alt=\"View this log!\"></area></map>\r\n");
		    strcat(StrHTM,"<img src=\"images/SelFile.gif\" width=20 height=20 border=0 align=left alt=\"Load this log file!\" usemap=\"#SelScrFile");
		    sprintf(FormNum,"%d",Num);
		    strcat(StrHTM,FormNum);
		    strcat(StrHTM,"\"></th>\r\n<td bgcolor=#ffa8d3 align=left><font size=\"-1\">");
		    strcat(StrHTM,(char*)&Data_File.cFileName );
		    strcat(StrHTM,"</font></td>\r\n");
		    strcat(StrHTM,"<td><font size=\"-1\">");
		    sprintf(FormNum,"%d",FileSize);
		    strcat(StrHTM,FormNum);
		    strcat(StrHTM,"</font></td></tr>\r\n");
		    Num++;
		    if ( strlen(StrHTM) > SIZE_FORM_BUF_HTTP-1000 )
		    {
                        send( WebTask->WorkSocket, StrHTM, strlen(StrHTM), 0 );
		        StrHTM[0] = 0;
		    }
		}
		if ( !FindNextFile(HDIRFILE,&Data_File) &&
		     GetLastError() == ERROR_NO_MORE_FILES) break;
	    }
            FindClose(HDIRFILE);
        }
#endif

#ifdef _LINUX_X86_     
    DirPtr = opendir(PathSearth);
	if (DirPtr)
    {
        FileNamePtr = &PathSearth[strlen(PathSearth)];
        while ((DirEntPtr = readdir(DirPtr)) != NULL)
	    {
            strcpy(FileNamePtr, DirEntPtr->d_name);           
            stat(PathSearth, &st);            
            if (((st.st_mode & S_IFMT) != S_IFMT) &&
                (strcmp(DirEntPtr->d_name, ".") != 0) &&
                (strcmp(DirEntPtr->d_name, "..") != 0) &&
                (FindCmdRequest(DirEntPtr->d_name, ".log") != -1))
		    {                
                FileSize = (unsigned long)st.st_size;            
		        strcat(StrHTM,"<tr><td><font size=\"-1\">");
		        sprintf(FormNum,"%d",Num+1);
		        strcat(StrHTM,FormNum);
		        strcat(StrHTM,"</font></td>\r\n");
		        strcat(StrHTM,"<th bgcolor=#00ff00>\r\n");
		        strcat(StrHTM,"<map name=\"SelScrFile");
		        sprintf(FormNum,"%d",Num);
		        strcat(StrHTM,FormNum);
		        strcat(StrHTM,"\"><area shape=rect coords=\"2,2,17,17\" href=\"");
		        strcat(StrHTM, WebTask->ServerHttpAddr);
		        strcat(StrHTM, GenPageLogMgr);
                strcat(StrHTM,WebTask->PageIdentityStr);
		        strcat(StrHTM,"&logcmd=glf&logname=");
		        strcat(StrHTM,(char*)DirEntPtr->d_name);
		        strcat(StrHTM,"\" alt=\"View this log!\"></area></map>\r\n");
		        strcat(StrHTM,"<img src=\"images/SelFile.gif\" width=20 height=20 border=0 align=left alt=\"Load this log file!\" usemap=\"#SelScrFile");
		        sprintf(FormNum,"%d",Num);
		        strcat(StrHTM,FormNum);
		        strcat(StrHTM,"\"></th>\r\n<td bgcolor=#ffa8d3 align=left><font size=\"-1\">");
		        strcat(StrHTM,(char*)DirEntPtr->d_name);
		        strcat(StrHTM,"</font></td>\r\n");
		        strcat(StrHTM,"<td><font size=\"-1\">");
		        sprintf(FormNum,"%d", (unsigned int)FileSize);
		        strcat(StrHTM,FormNum);
		        strcat(StrHTM,"</font></td></tr>\r\n");
		        Num++;
		        if ( strlen(StrHTM) > SIZE_FORM_BUF_HTTP-1000 )
		        {
                    send( WebTask->WorkSocket, StrHTM, strlen(StrHTM), 0 );
		            StrHTM[0] = 0;
		        }
		    }
	    }
        closedir(DirPtr);
    }
#endif

	strcat(StrHTM,"</table></td></tr></TABLE>\r\n");
	strcat(StrHTM,"<hr width=100%><CENTER>\r\n");
	strcat(StrHTM,"<a href=\"");
	strcat(StrHTM, WebTask->ServerHttpAddr);
	strcat(StrHTM,GenPageLogMgr);
    strcat(StrHTM,WebTask->PageIdentityStr);
	strcat(StrHTM,"&logcmd=vlj\">Current Terminal log.</a>&nbsp;|&nbsp;\r\n");
	strcat(StrHTM,"<a href=\"");
	strcat(StrHTM, WebTask->ServerHttpAddr);
	strcat(StrHTM,GenPageLogMgr);
	strcat(StrHTM,WebTask->PageIdentityStr);
	strcat(StrHTM,"&logcmd=vle\">Current Event log.</a>&nbsp;|&nbsp;\r\n");
	strcat(StrHTM,"<a href=\"");
	strcat(StrHTM, WebTask->ServerHttpAddr);
	strcat(StrHTM,GenPageLogMgr);
	strcat(StrHTM,WebTask->PageIdentityStr);
	strcat(StrHTM,"&logcmd=llf\">List of log files.</a>&nbsp;|&nbsp;\r\n");
	strcat(StrHTM,"<a href=\"");
	strcat(StrHTM, WebTask->ServerHttpAddr);
	strcat(StrHTM,GenPageMain);
	strcat(StrHTM,WebTask->PageIdentityStr);   
	strcat(StrHTM,"\">Back to main menu.</a>\r\n");
	strcat(StrHTM,"</CENTER></BODY></HTML>\r\n");
	FreeMemory(PathSearth);
	return;
}
#endif
//---------------------------------------------------------------------------
