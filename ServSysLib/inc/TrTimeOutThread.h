# if ! defined( TrTimeOutThreadH )
#	define TrTimeOutThreadH /* only include me once */

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

#define TMR_ONE_MSEC        1
#define TMR_ONE_TENTH_SEC   (TMR_ONE_MSEC * 100)
#define TMR_ONE_SEC         (TMR_ONE_MSEC * 1000)
#define TMR_ONE_MIN         (TMR_ONE_SEC * 60)
#define TMR_ONE_HOUR        (TMR_ONE_MIN * 60)
#define TMR_ONE_DAY         (TMR_ONE_HOUR * 24)

#ifdef WIN32
  #define TMU_TIMER_RESULT_START  2400
  #define TMU_REPORTDATA          2401
  #define TIMER_NOTIFY_TARGET (TimerPtr->ThreadID)
  #define MSG_NOTIFY_TARGET   (TimerMsgPtr->ThreadID)
#else
  #define TIMER_NOTIFY_TARGET (TimerPtr->NotifyPort)
  #define MSG_NOTIFY_TARGET   (TimerMsgPtr->NotifyPort)
#endif

typedef void (*TOnTimerCB)(unsigned int TimerId);
typedef void (*TOnTimerCBD)(unsigned int TimerId, void *DataPtr);

typedef struct {
#ifdef WIN32
	HANDLE              mutex;
	DWORD               ThrTimer;
#else
    pthread_mutex_t     mutex;
    pthread_cond_t      cond_var;
#endif
    PoolListItsTask     queue_list;
    bool                destroy;
} TimerMsgQueue;

typedef struct {
    unsigned int         TimerId;
    unsigned long        TimeStart;
    unsigned int         TimeOutVal;
    bool                 Replay;
#ifdef WIN32
    int	                 ThreadID;
#else
    unsigned short       NotifyPort;
#endif
	void                 *DataPtr;
	TOnTimerCB           OnTimerExp;
	TOnTimerCBD          OnTimerExpData;
    POOL_RECORD_STRUCT   *BlkPoolPtr;
    ObjPoolListTask      *ObjPtr;
} TimerRecord;

typedef struct {
    unsigned char        MsgTag;
    unsigned int         TimerId;
    unsigned int         TimeOutVal;
    bool                 Replay;
	void                 *DataPtr;
	TOnTimerCB           OnTimerExp;
	TOnTimerCBD          OnTimerExpData;
    unsigned int         *TimerRestartReqArray;
#ifdef WIN32
    int	                 ThreadID;
#else
    unsigned short       NotifyPort;
#endif
    POOL_RECORD_STRUCT   *BlkPoolPtr;
    ObjPoolListTask      *ObjPtr;
} TimerMessage;

typedef struct {
    bool                 StopReq;
	bool	             EnOnTimer;
    unsigned long        Tick;
    PoolListItsTask      ListTimers;
    POOL_RECORD_BASE     TimerBlockPool;
    int                  TimerMsgfd;
} TimerThrPar;

typedef struct {      
    bool                 StopTickFlag; /* Flag stop of tick thread */
#ifdef WIN32
    HANDLE               HTRTICKER;
    unsigned long        ThrTICKER;
#else
    pthread_t            ThreadId;
#endif
} TickTaskInfo;

typedef struct {
	bool                 isStartDone;
    bool                 StopTimerFlag; /* Flag stop of timer thread */
#ifdef WIN32
    DWORD		         ThrAnswStart;
	unsigned	         IDAnswStart;	
    HANDLE               HTRSYSTIMER;
    unsigned long        ThrSYSTEMTIMER;
#else
    pthread_t            ThreadId;
    sem_t	             TimerSem;
#endif
} TimerTaskInfo;

#ifdef WIN32
void CreateThreadTimer(int AnswThread, unsigned int TimeOut, unsigned int TimerId, bool Replay);
void CreateThreadTimerCB(TOnTimerCB Callback, int AnswThread, unsigned int TimeOut, unsigned int TimerId, bool Replay);
void CreateThreadTimerCBD(TOnTimerCBD Callback, int AnswThread, unsigned int TimeOut, unsigned int TimerId, bool Replay, void *DataPtr);
void ResetThreadTimer(int AnswThread, unsigned int TimerId);
void CloseThreadTimer(int AnswThread, unsigned int TimerId);
void RestartGrpThreadTimer(int AnswThread, unsigned int *TimerRestartReqArray);
#else
void CreateThreadTimer(unsigned short NotifyPort, unsigned int TimeOut, unsigned int TimerId, bool Replay);
void CreateThreadTimerCB(TOnTimerCB Callback, unsigned short NotifyPort, unsigned int TimeOut, unsigned int TimerId, bool Replay);
void CreateThreadTimerCBD(TOnTimerCBD Callback, unsigned short NotifyPort, unsigned int TimeOut, unsigned int TimerId, bool Replay, void *DataPtr);
void ResetThreadTimer(unsigned short NotifyPort, unsigned int TimerId);
void CloseThreadTimer(unsigned short NotifyPort, unsigned int TimerId);
void RestartGrpThreadTimer(unsigned short NotifyPort, unsigned int *TimerRestartReqArray);
#endif
int TimerThreadCreate();
void TimerThreadClose();

//---------------------------------------------------------------------------
#endif  /* if ! defined( TrTimeOutThreadH ) */
