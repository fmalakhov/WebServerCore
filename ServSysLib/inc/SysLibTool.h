# if ! defined( SysLibToolH )
#	define SysLibToolH /* only include me once */

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

#define WM_USER 0x0400
#define MAX_IMAGE_ZOOM 2  /* Maximum zoom for small images */

#define UINT_PACK_SIZE 4
#define DATE_PACK_SIZE 7

// begin for web
#define NOTIFY_SOCKET_FUF_SIZE 512
// end

#define MAX_LEN_SERV_BOARD_ID   36
#define MAX_LEN_LICENSE_INFO    (25 + MAX_LEN_SERV_BOARD_ID)

/* Statistic type */
#define SGT_GENERAL 0

typedef struct  {
    void	*UsedTask;
    void	*NextTask;
    void	*PrevTask;
} ObjListTask;

typedef struct  {
    unsigned long   Select;
    unsigned long   Count;
    ObjListTask   *FistTask;
    ObjListTask   *CurrTask;
} ListItsTask;

typedef struct {
    unsigned int MsgTag;
    void         *WParam;
    void         *LParam;
} InterThreadInfoMsg;

#define SOCKET_BUFFER_SIZE 64000

#ifdef _LINUX_X86_
typedef struct {
    int ThrSockfd;
    in_port_t DestThrPortNum;
    in_addr_t DestThrAddr;
} ThrMsgChanInfo;

typedef struct {
    pthread_t     event_thr;     /* Event timer thread */
    sem_t	      semx;
    unsigned char StopTimerFlag; /* Flag stop of timer thread */
    unsigned int  TimerMsgRxPort;
    unsigned int  TimerTickPort;
} TimerThrInfo;
#endif

#ifdef WIN32
typedef struct {
    unsigned int MsgTag;
	DWORD		 ThrReceiverId;
} ThrMsgChanInfo;
#endif

void ServSysLibVersionShow();
ObjListTask* AddStructListObj(ListItsTask *ListTasks, void *UserTask);
void AddStructList( ListItsTask *ListTasks, void *UserTask );
void RemStructList( ListItsTask *ListTasks, ObjListTask *PointTask );
void ClearListStructs(ListItsTask *List);
void* GetUserTaskByPoint( ListItsTask *List, unsigned Point );
void* GetFistObjectList( ListItsTask *ListTasks );
void* GetLastObjectList( ListItsTask *ListTasks );
void* GetNextObjectList( ListItsTask *ListTasks );
void* GetPrevObjectList( ListItsTask *ListTasks );
char* ParseParFunction( char *SrcText );
char* ParseParForm( char *SrcText );
char* FindCharStrScript( char TestChar,char *SrcText );
char* MoveTabStrScript(char *SrcText);
char* ParseSetDefine( char *SrcText );
void* AllocateMemory( unsigned int MemBlockSize );
void FreeMemory(void* FreeBlkPtr);
int ConvHexToInt( char *Src );

unsigned char* Uint16Pack(unsigned char* BufPtr, unsigned int Value);
unsigned char* Uint16Unpack(unsigned char* BufPtr, unsigned int* Value);
unsigned char* Uint24Pack(unsigned char* BufPtr, unsigned int Value);
unsigned char* Uint24Unpack(unsigned char* BufPtr, unsigned int* Value);
unsigned char* Uint32Pack(unsigned char* BufPtr, unsigned int Value);
unsigned char* Uint32Unpack(unsigned char* BufPtr, unsigned int* Value);

unsigned char* Uint16PackBE(unsigned char* BufPtr, unsigned int Value);
unsigned char* Uint16UnpackBE(unsigned char* BufPtr, unsigned int* Value);
unsigned char* Uint24PackBE(unsigned char* BufPtr, unsigned int Value);
unsigned char* Uint24UnpackBE(unsigned char* BufPtr, unsigned int* Value);
unsigned char* Uint32PackBE(unsigned char* BufPtr, unsigned int Value);
unsigned char* Uint32UnpackBE(unsigned char* BufPtr, unsigned int* Value);

unsigned char* Uint16PackES(unsigned char* BufPtr, unsigned int Value, bool isLE);
unsigned char* Uint16UnpackES(unsigned char* BufPtr, unsigned int* Value, bool isLE);
unsigned char* Uint24PackES(unsigned char* BufPtr, unsigned int Value, bool isLE);
unsigned char* Uint24UnpackES(unsigned char* BufPtr, unsigned int* Value, bool isLE);
unsigned char* Uint32PackES(unsigned char* BufPtr, unsigned int Value, bool isLE);
unsigned char* Uint32UnpackES(unsigned char* BufPtr, unsigned int* Value, bool isLE);
void SleepMs(unsigned int SleepTime);
void SocketMsgSendThread(SOCKET ThrSockfd, unsigned short DestPortNum,
    unsigned int MsgTag, void *WParam, void *LParam);
bool ServSysLibInit();
void ServSysLibClose();
#ifdef _LINUX_X86_
void AppThreadMsgSend(ThrMsgChanInfo *DestPtr, unsigned int MsgTag,
                      void *WParam, void *LParam);
unsigned long GetTickCount();
void Sleep(unsigned short Delay);
void SetServUUIDInfo(char *UuIdPtr);
#endif
#ifdef WIN32
void WinThreadMsgSend(DWORD ThreadId, unsigned int MsgTag, unsigned int WParam, LPARAM LParam);
void WaitCloseProcess(HANDLE ClosedProcHandle);
void MicroSleep(unsigned long usec);
#endif
unsigned char *SetTimeStampLine();
void SetIpAddrToString(char *BufPtr, unsigned int IpAddr);
int FindCmdRequest(char *BufSourse, char *TestText);
int FindCmdRequestLine(char *BufSourse, char *TestText);
int FindCmdArray(char *BufRequest,char **TablArrCMD,int AllTestCMD);
void CmdStrConvert(char *DestPtr, char *SrcPtr, unsigned int len);
unsigned int crc32(unsigned char *beg, int len);
void BufferSecure(unsigned char *Key, unsigned int KeyLen, char *DecBuf, 
				  char *EncBuf, unsigned int len);
int updateSocketSize(int sock, int attribute, unsigned long bufSize);
unsigned GetLocalIPServer(char *SrcCmd, char *Param, char *LocalHostName);

#ifdef _ESXI_PLATFORM_
unsigned long int __fdelt_chk (unsigned long int d);
#endif
//---------------------------------------------------------------------------
#endif  /* if ! defined( SysLibToolH ) */
