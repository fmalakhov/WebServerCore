# if ! defined( ThrReportMenH )
#	define ThrReportMenH	/* only include me once */

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

#ifndef PoolListH
#include "PoolList.h"
#endif

#define MAX_LOG_LINE_LEN 2048

#ifdef WIN32
#define RMU_STARTRESULT         2300
#define RMU_REPORTDATA          2301
#endif

#define DATE_CHANGE_CHECK_TMR_ID 1

/* Report manager command identifiers */
#define	RPMN_ADD_STR_REQ		1
#define RPMN_ADD_STR_EVENT_REQ	2
#define RPMN_PRINT_STATUS       3
#ifdef _REPORT_LOG_WEB_
#define RPMN_WEB_REQ_LOG        4
#endif
#define RPMN_DATE_CHG_TMR       5
#define RPMN_RESET_LOG_REQ      6

#ifdef _REPORT_LOG_WEB_
typedef struct {
	DWORD		    ThrCernelID;
	DWORD			ThrTimerID;   //
	SOCKET			WorkSocket;
	unsigned int    SessionId;
	int				LocalPortServ;
	char			*RequestCommand;
	char            PageIdentityStr[128];
	char            ServerHttpAddr[512];
	char			*LocalAdress;
	unsigned		IDAnswEndJob;
    DWORD			ThrAnswEndJob;
	unsigned int	Req_ID;
} PARUSERWEBTASK;

typedef struct {
	char			*LogName;
	unsigned char	TypeLog;
	unsigned		IDAnswMess;
	DWORD			ThrAnsMess;
    unsigned		IDError;
    char			*ErrParam;
} REQGETCURRLOG;

typedef struct {
	bool	    LastRot;
	unsigned	LineSrc;
	char	    *Data;
	char	    *HTTPStr;
	char	    *FormStr;
	unsigned	LineLogStr;
} PROCFORMWEBLOG;
#endif

typedef struct {
#ifdef WIN32
	HANDLE              mutex;
	unsigned long       ThrReportMen;
#else
    pthread_mutex_t     mutex;
    pthread_cond_t      cond_var;
#endif
    PoolListItsTask     queue_list;
    bool                destroy;
} ReportMsgQueue;

typedef struct {
    unsigned char        MsgTag;
#ifdef _REPORT_LOG_WEB_
    PARUSERWEBTASK       *WebReq;
#endif
    POOL_RECORD_STRUCT   *BlkPoolPtr;
    ObjPoolListTask      *ObjPtr;
    char                 LogLine[MAX_LOG_LINE_LEN+24];
} ReportMessage;

typedef struct {
#ifdef WIN32
	unsigned long ThrReportMen;
    HANDLE        HtrReportMen;
#else
	pthread_t	  report_thr;     /* Report manager thread */
#endif
	bool          StopReportFlag; /* Flag stop of report thread */
} PARINIREPMEN;

typedef struct {
	FILE		  *HandleRep;
	unsigned char DbgLastDayOfWeek;
    unsigned char EvtLastDayOfWeek;
	PARINIREPMEN  *MainParRep;
	char		  StartDir[1024];
	char		  PathRepFiles[1024];
    char		  PathRepEventLog[1024];
} REPORT_MANAGER_TASK;

void EventLogPrint(ThrMsgChanInfo *ReqThrChan, const char* format, ... );
void DebugLogPrint(ThrMsgChanInfo *ReqThrChan, const char* format, ... );
void DebugLogStrBufPrint(ThrMsgChanInfo *ReqThrChan, char *StrBuf, const char* format, ... );
int ReportThreadCreate();
void ReportThreadClose();
void ReportStatusPrint();
void ReportLogReset();
void DebugLogStrPrint(char *StrPtr);

#ifdef _REPORT_LOG_WEB_
void ReportWebMgtReq(PARUSERWEBTASK *WebReq);
#endif
//---------------------------------------------------------------------------
#endif  /* if ! defined( ThrReportMenH ) */
