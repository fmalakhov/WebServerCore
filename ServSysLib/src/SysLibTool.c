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

#include "CommonPlatform.h"
#include "SysLibTool.h"
#include "vistypes.h"

#define MAX_LEN_ENCODE_SRC_LINE 1024

char g_TimeStampLine[128];

#ifdef WIN32
HANDLE gMemoryMutex;
HANDLE gFileMutex;
#else
sem_t  gMemoryMutex;
sem_t  gFileMutex;
#endif

static char DrdInfoFile[]  = "serv_info.log";
static char BrdCheckCmd[]  = "dmidecode";
static char BrdCheckPars[] = "system";
static char LibVersionInfo[] = "\nServer System Library version: 4.0.2 11.22.2017\nCopyright (c) 2012-2017 MFBS.\nThis product includes software developed by MFBS\n\n";

unsigned char AsciiCharToHexOctet[] = {
 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
 0x80, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
 0x80, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80 };

//---------------------------------------------------------------------------
void ServSysLibVersionShow()
{
	printf("%s", LibVersionInfo);
}
//---------------------------------------------------------------------------
ObjListTask* AddStructListObj(ListItsTask *ListTasks, void *UserTask)
{
	ObjListTask     *NewPT;
    ObjListTask     *PrevPT;
    ObjListTask     *NextPT;

	NewPT = (ObjListTask*)AllocateMemory(sizeof(ObjListTask));
	NewPT->UsedTask = UserTask;
	if (!ListTasks->Count || !ListTasks->FistTask)
	{
		ListTasks->FistTask = NewPT;
	    NewPT->NextTask = NewPT;
	    NewPT->PrevTask = NewPT;
        ListTasks->Count = 0;
	}
	else
    {
		NextPT = (ObjListTask*)ListTasks->FistTask;
        PrevPT = (ObjListTask*)NextPT->PrevTask;
        NewPT->NextTask = (void*)NextPT;
        NewPT->PrevTask = (void*)PrevPT;
        if (PrevPT) PrevPT->NextTask = (void*)NewPT;
        if (NextPT) NextPT->PrevTask = (void*)NewPT;
    }
	ListTasks->Count++;
	return NewPT;
}
//---------------------------------------------------------------------------
void AddStructList( ListItsTask *ListTasks, void *UserTask )
{
	ObjListTask     *NewPT;

	NewPT = AddStructListObj(ListTasks, UserTask);
	return;
}
//---------------------------------------------------------------------------
void RemStructList( ListItsTask *ListTasks, ObjListTask *PointTask )
{
	ObjListTask     *NextPT;
	ObjListTask     *PrevPT;

	if ( !ListTasks->Count || !PointTask ) return;
	PrevPT = (ObjListTask*)PointTask->PrevTask;
	NextPT = (ObjListTask*)PointTask->NextTask;
    if (PrevPT) PrevPT->NextTask = (void*)NextPT;
    if (NextPT) NextPT->PrevTask = (void*)PrevPT;
    if (PointTask == ListTasks->FistTask) ListTasks->FistTask = NextPT;
	FreeMemory( (ObjListTask*)PointTask );
	ListTasks->Count--;
	return;
}
//---------------------------------------------------------------------------
void* GetFistObjectList( ListItsTask *ListTasks )
{
	if (!ListTasks) return NULL;
    if (!ListTasks->Count) return NULL;
    ListTasks->CurrTask = (ObjListTask*)ListTasks->FistTask;
    ListTasks->Select = 0;
    return ListTasks->CurrTask;
}
//---------------------------------------------------------------------------
void* GetLastObjectList( ListItsTask *ListTasks )
{
	if (!ListTasks) return NULL;
    if (!ListTasks->Count) return NULL;
	if (!ListTasks->FistTask) return NULL;
    ListTasks->CurrTask = (ObjListTask*)(ListTasks->FistTask->PrevTask);
    ListTasks->Select = ListTasks->Count-1;
    return ListTasks->CurrTask;
}
//---------------------------------------------------------------------------
void* GetNextObjectList( ListItsTask *ListTasks )
{
	if (!ListTasks) return NULL;
	if (!ListTasks->Count) return NULL;
	if (!ListTasks->CurrTask) return NULL;
	if (!ListTasks->FistTask) return NULL;
    if (ListTasks->FistTask == (ObjListTask*)(ListTasks->CurrTask->NextTask) ) return NULL;
    ListTasks->CurrTask = (ObjListTask*)(ListTasks->CurrTask->NextTask);
    ListTasks->Select++;
    return ListTasks->CurrTask;
}
//---------------------------------------------------------------------------
void* GetPrevObjectList( ListItsTask *ListTasks )
{
	if (!ListTasks) return NULL;
	if (!ListTasks->Count) return NULL;
	if (!ListTasks->CurrTask) return NULL;
	if (!ListTasks->FistTask) return NULL;
    if (ListTasks->FistTask == (ObjListTask*)(ListTasks->CurrTask)) return NULL;
    ListTasks->CurrTask = (ObjListTask*)(ListTasks->CurrTask->PrevTask);
    ListTasks->Select--;
    return ListTasks->CurrTask;
}
//---------------------------------------------------------------------------
void ClearListStructs(ListItsTask *List)
{
ObjListTask     *PointTask;

if ( !List->Count ) return;
while ( List->Count )
   {PointTask = List->FistTask;
    RemStructList( List, PointTask );
   }
}
//---------------------------------------------------------------------------
void* GetUserTaskByPoint( ListItsTask *List, unsigned Point )
{
    unsigned    i;
    ObjListTask *PointTask;

    PointTask = (ObjListTask*)List->FistTask;
    for (i=0;i < Point;i++) PointTask = (ObjListTask*)PointTask->NextTask;
    return PointTask->UsedTask;
}
//--------------------------------------------------------------------------
char* FindCharStrScript(char TestChar,char *SrcText)
{
unsigned    i;
char *PointChar;

if ( !SrcText )	return NULL;
PointChar = 0;
for (i=0;i < strlen(SrcText);i++)
   {if (SrcText[i] == TestChar)
      {PointChar = &SrcText[i];
       break;
      }
   }
return PointChar;
}
//--------------------------------------------------------------------------
char* MoveTabStrScript(char *SrcText)
{
unsigned    i;
char *PointChar;

if ( !SrcText )	return NULL;
PointChar = NULL;
for (i=0;i < strlen(SrcText);i++)
   {if (SrcText[i] == 0 || SrcText[i] == 0x0d || SrcText[i] == 0x0a ) {PointChar = 0;break;}
	if (SrcText[i] != ' ' && SrcText[i] != 0x09)
      {PointChar = &SrcText[i];
       break;
      }
   }
return PointChar;
}
//--------------------------------------------------------------------------
char* ParseSetDefine( char *SrcText )
{
    char    *FindLine;

	if ( !SrcText )	return NULL;
    if ( !strlen(SrcText) ) return NULL;
	FindLine = MoveTabStrScript( SrcText );
    if ( !FindLine ) return NULL;
    if ( FindLine[0] != '=' ) return NULL;
    if ( !strlen(FindLine) ) return NULL;
    FindLine = MoveTabStrScript( &FindLine[1] );
    if ( !FindLine ) return NULL;
    if ( !strlen(FindLine) ) return NULL;
	return FindLine;
}
//--------------------------------------------------------------------------
char* ParseParFunction( char *SrcText )
{
    char    *FindLine;
    char    *StartPar;


    if ( !SrcText || !strlen(SrcText) ) return NULL;
    FindLine = MoveTabStrScript( SrcText );
    if ( !FindLine ) return NULL;
    if ( FindLine[0] != '=' ) return NULL;
    if ( !strlen(FindLine) ) return NULL;
    FindLine = MoveTabStrScript( &FindLine[1] );
    if ( !FindLine ) return NULL;
    if ( !strlen(FindLine) ) return NULL;
    FindLine = FindCharStrScript( '"',FindLine );
    if ( !FindLine ) return NULL;
    StartPar = &FindLine[1];
    if ( !strlen(StartPar) ) return NULL;
    FindLine = FindCharStrScript( '"',StartPar );
    if ( !FindLine ) return NULL;
    FindLine[0] = 0;
    return StartPar;
}
//--------------------------------------------------------------------------
char* ParseParForm(char *SrcText)
{
    char    *FindLine;
    char    *StartPar;

    if ( !SrcText || !strlen(SrcText) ) return NULL;
    FindLine = MoveTabStrScript( SrcText );
    if ( !FindLine ) return NULL;
    if ( FindLine[0] != '=' ) return NULL;
    if ( !strlen(FindLine) ) return NULL;
    FindLine = MoveTabStrScript( &FindLine[1] );
    if ( !FindLine ) return NULL;
    if ( !strlen(FindLine) ) return NULL;
	StartPar = FindLine;
    FindLine = FindCharStrScript( '&',FindLine);
    if ( !FindLine ) return StartPar;
	FindLine[0] = 0;
    return StartPar;
}
//---------------------------------------------------------------------------
int ConvHexToInt( char *Src )
{
	DWORD			Value;
	int				Poz;
	unsigned char	DSim;

	if ( !Src || !strlen(Src) ) return -1;
	Poz = 0;
	Value  = 0;
	while ( 1 )
	  {if ( Poz > 7 )	return -1;
	   if ( !Src[Poz] )	return (int)Value;
	   if ( Src[Poz] >= '0' && Src[Poz] <= '9' )		DSim = (unsigned char)(Src[Poz]-'0');
	   else
		 {if ( Src[Poz] >= 'a' && Src[Poz] <= 'f' )		DSim = (unsigned char)(Src[Poz]-'a');
		  else						
			{if ( Src[Poz] >= 'A' && Src[Poz] <= 'F' )	DSim = (unsigned char)(Src[Poz]-'A');
			 else	
			   {if ( MoveTabStrScript( &Src[Poz] ) ) return -1;
			    break;
			   }
			}
		 }
	   Value = (Value << 4) | ((DWORD)DSim&0x0f);
	   Poz++;
	  }
	return (int)Value;
}
//---------------------------------------------------------------------------
void* AllocateMemory( unsigned int MemBlockSize )
{
	void		*NewMem;
	unsigned	Count;
#ifdef WIN32
	DWORD       WaitResult;
	bool        isMemAllocReady = false;

    WaitResult = WaitForSingleObject(gMemoryMutex, INFINITE);
    switch(WaitResult)
	{
	    case WAIT_OBJECT_0:
			isMemAllocReady = true;
		    break;

        case WAIT_ABANDONED: 
			printf("The other thread that using mutex is closed in locked state of mutex\r\n");
            break;

		default:
			printf("Memory allocation mutex is fialed with error: %d\r\n", GetLastError());
			break;
	}
    if (!isMemAllocReady)
	{
        if (! ReleaseMutex(gMemoryMutex)) 
		{ 
            printf("Fail to release mutex (memory allocation)\r\n");
		}
	    return NULL;
	}
#endif
#ifdef _LINUX_X86_
    sem_wait(&gMemoryMutex);
#endif
	Count = 0;
    NewMem = malloc(MemBlockSize);
	while(!NewMem)
	{
	    Sleep(5);
	    Count++;
	    if (Count > 10) break;
	    NewMem = malloc(MemBlockSize);
	}
#ifdef WIN32
    if (! ReleaseMutex(gMemoryMutex)) 
	{ 
        printf("Fail to release mutex (memory allocation)\r\n");
	}
#endif
#ifdef _LINUX_X86_
    sem_post(&gMemoryMutex);
#endif
	return NewMem;
}
//---------------------------------------------------------------------------
void FreeMemory(void* FreeBlkPtr)
{
#ifdef WIN32
	DWORD       WaitResult;
	bool        isMemFreeReady = false;

    WaitResult = WaitForSingleObject(gMemoryMutex, INFINITE);
    switch(WaitResult)
	{
	    case WAIT_OBJECT_0:
			isMemFreeReady = true;
		    break;

        case WAIT_ABANDONED: 
			printf("The other thread that using mutex is closed in locked state of mutex\r\n");
            break;

		default:
			printf("Memory free mutex is failed with error: %d\r\n", GetLastError());
			break;
	}
    if (!isMemFreeReady)
	{
        if (! ReleaseMutex(gMemoryMutex)) 
		{ 
            printf("Fail to release mutex (memory clean)\r\n");
		}
	}
#endif
#ifdef _LINUX_X86_
    sem_wait(&gMemoryMutex);
#endif
	free(FreeBlkPtr);
#ifdef WIN32
    if (! ReleaseMutex(gMemoryMutex)) 
	{ 
        printf("Fail to release mutex (memory allocation)\r\n");
	}
#endif
#ifdef _LINUX_X86_
    sem_post(&gMemoryMutex);
#endif
}
//---------------------------------------------------------------------------
#ifdef _LINUX_X86_
void AppThreadMsgSend(ThrMsgChanInfo *DestPtr, unsigned int MsgTag, void *WParam, void *LParam)
{
    InterThreadInfoMsg Msg;
    struct sockaddr_in DestAddr;
    int                sent_bytes;

    Msg.MsgTag = MsgTag;
    Msg.WParam = WParam;
    Msg.LParam = LParam;

    DestAddr.sin_family      = AF_INET;

  #ifdef _SUN_BUILD_
    DestAddr.sin_addr.s_addr = htonl(DestPtr->DestThrAddr);
    DestAddr.sin_port        = htons(DestPtr->DestThrPortNum);    
  #else
    DestAddr.sin_addr.s_addr = DestPtr->DestThrAddr;
    DestAddr.sin_port        = DestPtr->DestThrPortNum;    
  #endif

    sent_bytes = sendto(DestPtr->ThrSockfd, (void *)(&Msg), 
		    sizeof(InterThreadInfoMsg), 0, (struct sockaddr *) &DestAddr,
			sizeof(DestAddr));      
    if ( sent_bytes < 0 )
    {
        printf("\n%s Failed to sent packet to thread tag: %d, port: %d (errno: %d)",
			SetTimeStampLine(), MsgTag, DestPtr->DestThrPortNum, errno);
    }
}
#endif
//---------------------------------------------------------------------------
void SocketMsgSendThread(SOCKET ThrSockfd, unsigned short DestPortNum, unsigned int MsgTag, void *WParam, void *LParam)
{
    InterThreadInfoMsg Msg;
    struct sockaddr_in DestAddr;
    int                sent_bytes;

    Msg.MsgTag = MsgTag;
    Msg.WParam = WParam;
    Msg.LParam = LParam;

    DestAddr.sin_family      = AF_INET;

  #ifndef _SUN_BUILD_
    DestAddr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    DestAddr.sin_port        = htons(DestPortNum);    
  #else
    DestAddr.sin_addr.s_addr = INADDR_LOOPBACK;
    DestAddr.sin_port        = DestPortNum;    
  #endif

    sent_bytes = sendto(ThrSockfd, (const char*)(&Msg), 
		    sizeof(InterThreadInfoMsg), 0, (struct sockaddr *) &DestAddr,
			sizeof(DestAddr));      
    if ( sent_bytes < 0 )
    {
        printf("\n%s Failed to sent packet to thread tag: %d, port: %d (errno: %d)",
			SetTimeStampLine(), MsgTag, DestPortNum, errno);
    }
}
//---------------------------------------------------------------------------
#ifdef WIN32
void WinThreadMsgSend(DWORD ThreadId, unsigned int MsgTag, unsigned int WParam, LPARAM LParam)
{
	bool MsgSentResult = false;

    for(;;)
	{
        MsgSentResult = PostThreadMessage(ThreadId,
						   MsgTag,(WPARAM)WParam,LParam);
		if (!MsgSentResult)
		{
			Sleep(1);
		    continue;
		}
		else
		{
		    break;
		}
	}
}
#endif
//---------------------------------------------------------------------------
#ifdef _LINUX_X86_
unsigned char *SetTimeStampLine()
{
    struct timeb hires_cur_time;
    struct tm *cur_time;

    ftime(&hires_cur_time);
    cur_time = localtime(&hires_cur_time.time);

    sprintf(g_TimeStampLine, "%2.2d/%2.2d/%4.4d %2.2d:%2.2d:%2.2d.%3.3d|",
           (cur_time->tm_mon+1), cur_time->tm_mday,
            (cur_time->tm_year+1900), cur_time->tm_hour,
             cur_time->tm_min, cur_time->tm_sec,
            hires_cur_time.millitm);
    return &g_TimeStampLine[0];
}
#endif
#ifdef WIN32
unsigned char *SetTimeStampLine()
{
	struct _SYSTEMTIME RealTime;

    GetLocalTime(&RealTime);
    sprintf(g_TimeStampLine, "%2.2d/%2.2d/%4.4d %2.2d:%2.2d:%2.2d.%3.3d|",
           RealTime.wMonth, RealTime.wDay, RealTime.wYear, RealTime.wHour,
             RealTime.wMinute, RealTime.wSecond, RealTime.wMilliseconds);
    return &g_TimeStampLine[0];
}
#endif
//---------------------------------------------------------------------------
#ifdef _LINUX_X86_
unsigned long GetTickCount()
{
    struct timeval tv;
  
    if( gettimeofday(&tv, NULL) != 0 ) return 0;
    return (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
}
//---------------------------------------------------------------------------
void Sleep(unsigned short Delay)
{
    struct timespec sleep_time;
    
    sleep_time.tv_sec = 0;
    sleep_time.tv_nsec = (Delay * 1000000);
    nanosleep(&sleep_time, 0);
}
#endif
//---------------------------------------------------------------------------
unsigned char* Uint16Pack(unsigned char* BufPtr, unsigned int Value)
{
	*BufPtr++ = (unsigned char)((Value & 0x0000ff00) >> 8);
	*BufPtr++ = (unsigned char)(Value & 0x000000ff);
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint16Unpack(unsigned char* BufPtr, unsigned int* Value)
{
    *Value = (unsigned int)(*BufPtr++) << 8;
	*Value |= (unsigned int)(*BufPtr++);
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint24Pack(unsigned char* BufPtr, unsigned int Value)
{
    *BufPtr++ = (unsigned char)((Value & 0x00ff0000) >> 16);
	*BufPtr++ = (unsigned char)((Value & 0x0000ff00) >> 8);
	*BufPtr++ = (unsigned char)(Value & 0x000000ff);
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint24Unpack(unsigned char* BufPtr, unsigned int* Value)
{
    *Value = (unsigned int)(*BufPtr++) << 16;
	*Value |= (unsigned int)(*BufPtr++) << 8;
	*Value |= (unsigned int)(*BufPtr++);
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint32Pack(unsigned char* BufPtr, unsigned int Value)
{
	*BufPtr++ = (unsigned char)((Value & 0xff000000) >> 24);
    *BufPtr++ = (unsigned char)((Value & 0x00ff0000) >> 16);
	*BufPtr++ = (unsigned char)((Value & 0x0000ff00) >> 8);
	*BufPtr++ = (unsigned char)(Value & 0x000000ff);
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint32Unpack(unsigned char* BufPtr, unsigned int* Value)
{
    *Value = (unsigned int)(*BufPtr++) << 24;
	*Value |= (unsigned int)(*BufPtr++) << 16;
	*Value |= (unsigned int)(*BufPtr++) << 8;
	*Value |= (unsigned int)(*BufPtr++);
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint16PackBE(unsigned char* BufPtr, unsigned int Value)
{
	*BufPtr++ = (unsigned char)(Value & 0x000000ff);
	*BufPtr++ = (unsigned char)((Value & 0x0000ff00) >> 8);
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint16UnpackBE(unsigned char* BufPtr, unsigned int* Value)
{
	*Value = (unsigned int)(*BufPtr++);
	*Value |= (unsigned int)(*BufPtr++) << 8;
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint24PackBE(unsigned char* BufPtr, unsigned int Value)
{
	*BufPtr++ = (unsigned char)(Value & 0x000000ff);
    *BufPtr++ = (unsigned char)((Value & 0x0000ff00) >> 8);
    *BufPtr++ = (unsigned char)((Value & 0x00ff0000) >> 16);
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint24UnpackBE(unsigned char* BufPtr, unsigned int* Value)
{
	*Value = (unsigned int)(*BufPtr++);
	*Value |= (unsigned int)(*BufPtr++) << 8;
	*Value |= (unsigned int)(*BufPtr++) << 16;
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint32PackBE(unsigned char* BufPtr, unsigned int Value)
{
	*BufPtr++ = (unsigned char)(Value & 0x000000ff);
	*BufPtr++ = (unsigned char)((Value & 0x0000ff00) >> 8);
    *BufPtr++ = (unsigned char)((Value & 0x00ff0000) >> 16);
	*BufPtr++ = (unsigned char)((Value & 0xff000000) >> 24);
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint32UnpackBE(unsigned char* BufPtr, unsigned int* Value)
{
	*Value = (unsigned int)(*BufPtr++);
	*Value |= (unsigned int)(*BufPtr++) << 8;
	*Value |= (unsigned int)(*BufPtr++) << 16;
    *Value |= (unsigned int)(*BufPtr++) << 24;
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint16PackES(unsigned char* BufPtr, unsigned int Value, bool isLE)
{
	if (isLE)
	{
	    *BufPtr++ = (unsigned char)((Value & 0x0000ff00) >> 8);
	    *BufPtr++ = (unsigned char)(Value & 0x000000ff);
	}
	else
	{
	    *BufPtr++ = (unsigned char)(Value & 0x000000ff);
	    *BufPtr++ = (unsigned char)((Value & 0x0000ff00) >> 8);
	}
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint16UnpackES(unsigned char* BufPtr, unsigned int* Value, bool isLE)
{
	if (isLE)
	{
        *Value = (unsigned int)(*BufPtr++) << 8;
	    *Value |= (unsigned int)(*BufPtr++);
	}
	else
	{
	    *Value = (unsigned int)(*BufPtr++);
		*Value |= (unsigned int)(*BufPtr++) << 8;
	}
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint24PackES(unsigned char* BufPtr, unsigned int Value, bool isLE)
{
	if (isLE)
	{
        *BufPtr++ = (unsigned char)((Value & 0x00ff0000) >> 16);
	    *BufPtr++ = (unsigned char)((Value & 0x0000ff00) >> 8);
	    *BufPtr++ = (unsigned char)(Value & 0x000000ff);
	}
	else
	{
		*BufPtr++ = (unsigned char)(Value & 0x000000ff);
		*BufPtr++ = (unsigned char)((Value & 0x0000ff00) >> 8);
        *BufPtr++ = (unsigned char)((Value & 0x00ff0000) >> 16);
	}
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint24UnpackES(unsigned char* BufPtr, unsigned int* Value, bool isLE)
{
	if (isLE)
	{
        *Value = (unsigned int)(*BufPtr++) << 16;
	    *Value |= (unsigned int)(*BufPtr++) << 8;
	    *Value |= (unsigned int)(*BufPtr++);
	}
	else
	{
		*Value = (unsigned int)(*BufPtr++);
		*Value |= (unsigned int)(*BufPtr++) << 8;
		*Value |= (unsigned int)(*BufPtr++) << 16;
	}
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint32PackES(unsigned char* BufPtr, unsigned int Value, bool isLE)
{
	if (isLE)
	{
	    *BufPtr++ = (unsigned char)((Value & 0xff000000) >> 24);
        *BufPtr++ = (unsigned char)((Value & 0x00ff0000) >> 16);
	    *BufPtr++ = (unsigned char)((Value & 0x0000ff00) >> 8);
	    *BufPtr++ = (unsigned char)(Value & 0x000000ff);
	}
	else
	{
	    *BufPtr++ = (unsigned char)(Value & 0x000000ff);
	    *BufPtr++ = (unsigned char)((Value & 0x0000ff00) >> 8);
        *BufPtr++ = (unsigned char)((Value & 0x00ff0000) >> 16);
	    *BufPtr++ = (unsigned char)((Value & 0xff000000) >> 24);
	}
	return BufPtr;
}
//---------------------------------------------------------------------------
unsigned char* Uint32UnpackES(unsigned char* BufPtr, unsigned int* Value, bool isLE)
{
	if (isLE)
	{
        *Value = (unsigned int)(*BufPtr++) << 24;
	    *Value |= (unsigned int)(*BufPtr++) << 16;
	    *Value |= (unsigned int)(*BufPtr++) << 8;
	    *Value |= (unsigned int)(*BufPtr++);
	}
	else
	{
		*Value = (unsigned int)(*BufPtr++);
		*Value |= (unsigned int)(*BufPtr++) << 8;
		*Value |= (unsigned int)(*BufPtr++) << 16;
        *Value |= (unsigned int)(*BufPtr++) << 24;
	}
	return BufPtr;
}
//---------------------------------------------------------------------------
void SetIpAddrToString(char *BufPtr, unsigned int IpAddr)
{
    sprintf(BufPtr, "%d.%d.%d.%d",   
#ifdef WIN32    
        (int)((IpAddr&0xff000000)>>24),
        (int)((IpAddr&0x00ff0000)>>16),
        (int)((IpAddr&0x0000ff00)>>8),
	    (int)(IpAddr&0x000000ff));	
#else 
        (int)(IpAddr&0x000000ff), 
	    (int)((IpAddr&0x0000ff00)>>8), 
	    (int)((IpAddr&0x00ff0000)>>16),
	    (int)((IpAddr&0xff000000)>>24));
#endif
}
//---------------------------------------------------------------------------
int FindCmdRequest(char *BufSourse, char *TestText)
{
int i,j,k,rs,ReturnFind;

i = 0;ReturnFind = -1;
k = strlen(TestText);
if ( (int)strlen(BufSourse) < k) return ReturnFind;
j = strlen(BufSourse)+1-k;
while ( i < j )
{
	rs = strncmp(&BufSourse[i], TestText, k );
    if (rs == 0)
	{
		ReturnFind = i+k;
        break;
    }
    i++;
}
return ReturnFind;
}
//---------------------------------------------------------------------------
int FindCmdRequestLine(char *BufSourse, char *TestText)
{
	register int i = 0, j, k, l, ReturnFind = -1;

	k = strlen(TestText);
	l = strlen(BufSourse);
	if (l < k) return ReturnFind;
	j = l + 1 - k;
	while (i < j)
	{
		if ((*BufSourse == '\r') || (*BufSourse == '\n')) break;
		if (strncmp(BufSourse, TestText, k) == 0)
		{
			ReturnFind = i + k;
			break;
		}
		i++;
		BufSourse++;
	}
	return ReturnFind;
}
//---------------------------------------------------------------------------
int FindCmdArray(char *BufRequest,char **TablArrCMD,int AllTestCMD)
{
	bool			CmpPar;
	unsigned int	i,j,m,n;
	unsigned int	*LenCmds;

	if ( !AllTestCMD || !BufRequest || !TablArrCMD )	return -1;
	LenCmds = (unsigned int*)AllocateMemory(sizeof(unsigned int)*AllTestCMD);
	for (i=0;i < (unsigned int)AllTestCMD;i++)	LenCmds[i] = strlen( TablArrCMD[i] );
	m = n = strlen(BufRequest);
	for (i=0;i < m;i++)
		{for (CmpPar=false, j=0;j < (unsigned int)AllTestCMD;j++)
			{if ( LenCmds[j] <= n )
			   {if ( strncmp( &BufRequest[i], TablArrCMD[j], LenCmds[j] ) == 0)
			      {FreeMemory( LenCmds );return j;}
				CmpPar = true;
			   }
			}
		 if ( !CmpPar )	break;
		 n--;
		}
	FreeMemory( LenCmds );
	return -1;  
}
//---------------------------------------------------------------------------
void CmdStrConvert(char *DestPtr, char *SrcPtr, unsigned int len)
{
	register unsigned char charVal1, charVal2;
	register unsigned int i;

	for(i=0;i < len;i++)
	{
		if (*SrcPtr == '%')
		{
			if ((len - i) < 3)
			{
				*DestPtr++ = *SrcPtr++;
				continue;
			}
			SrcPtr++;
			i++;
			charVal1 = AsciiCharToHexOctet[*SrcPtr];
			if (charVal1 & 0x80)
			{
				*DestPtr++ = *SrcPtr++;
			    continue;
			}
			SrcPtr++;
			i++;
			charVal2 = AsciiCharToHexOctet[*SrcPtr];
			if (charVal2 & 0x80)
			{
				*DestPtr++ = *SrcPtr++;
			    continue;
			}
            *DestPtr++ = (charVal1 << 4) + charVal2;
			SrcPtr++;
		}
		else
		{
			*DestPtr++ = *SrcPtr++;
		}
	}
	*DestPtr = 0;
}
//---------------------------------------------------------------------------
#ifdef _ESXI_PLATFORM_
unsigned long int __fdelt_chk (unsigned long int d)
{
  if (d >= FD_SETSIZE)
    __chk_fail ();

  return d / __NFDBITS;
}
#endif
//---------------------------------------------------------------------------
#ifdef WIN32
void MicroSleep(unsigned long usec)
{
    struct timeval tv;
	int            Res;

    fd_set dummy;
    SOCKET s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    FD_ZERO(&dummy);
    FD_SET(s, &dummy);
    tv.tv_sec = 0;
    tv.tv_usec = usec % 1000000ul;
    Res = select(0, 0, 0, &dummy, &tv);
    closesocket(s);
}
//---------------------------------------------------------------------------
void WaitCloseProcess(HANDLE ClosedProcHandle)
{
    DWORD   ExitCode;
    DWORD   CountClose;

    CountClose = 0;
    GetExitCodeThread( ClosedProcHandle, &ExitCode );
    while ( ExitCode == STILL_ACTIVE )
    {
		if (CountClose > 100)
        {
			TerminateThread(ClosedProcHandle,0);
			break;
		}
        Sleep(100);
        CountClose++;
        GetExitCodeThread( ClosedProcHandle, &ExitCode );
    }
    return;
}
#endif
//---------------------------------------------------------------------------
void SleepMs(unsigned int SleepTime)
{
#ifdef WIN32
	Sleep(SleepTime);
#else
    usleep(SleepTime * 1000);
#endif
}
//---------------------------------------------------------------------------
#ifdef _LINUX_X86_
void SetServUUIDInfo(char *UuIdPtr)
{
    unsigned int  i;  
    int           c, pos, SysStatus;
    int           size = 128;
	char          *RetCodeCwd = NULL;
	FILE          *DataFilePtr = NULL;
    char          SudoStr[16];
    char          StartPath[514];
    char          SystemCmd[1024];
    char          Buffer[129];

    *UuIdPtr = 0;
    *SudoStr = 0;
    strcpy(SudoStr, "sudo");
	RetCodeCwd = getcwd((char*)(&StartPath[0]), 512);
    strcpy(SystemCmd, StartPath);
    strcat(SystemCmd, DrdInfoFile);
	if (access(SystemCmd, F_OK) == 0)
	{
        sprintf(SystemCmd, "%s rm %s%s\n", SudoStr, StartPath, DrdInfoFile);
        SysStatus = system(SystemCmd);    
    }

    sprintf(SystemCmd, "%s %s -t %s > %s%s 2>/dev/null\n", 
        SudoStr, BrdCheckCmd, BrdCheckPars,
        StartPath, DrdInfoFile);
    SysStatus = system(SystemCmd);

    strcpy(SystemCmd, StartPath);
    strcat(SystemCmd, DrdInfoFile);
    DataFilePtr = fopen(SystemCmd, "r");
    do 
    { 
        /* read all lines in file */
        pos = 0;
        do
		{ 
            /* read one line */
            c = fgetc(DataFilePtr);
            if((c != EOF) && (pos < size)) Buffer[pos++] = (char)c;
		} while((c != EOF) && (c != '\n'));
        Buffer[pos] = 0;
        i = FindCmdRequest(Buffer, "UUID: ");
        if (i != -1)
        {
            if (strlen(&Buffer[i]) > MAX_LEN_SERV_BOARD_ID)
                 Buffer[i+ MAX_LEN_SERV_BOARD_ID] = 0;
            strcpy(UuIdPtr, &Buffer[i]);
            break;
        }
	} while(c != EOF); 
    fclose(DataFilePtr);
    sprintf(SystemCmd, "%s rm %s%s\n", SudoStr, StartPath, DrdInfoFile);
    SysStatus = system(SystemCmd);
}
#endif
//---------------------------------------------------------------------------
bool ServSysLibInit()
{
#ifdef WIN32
    gMemoryMutex = CreateMutex(NULL, FALSE, NULL);
    if (gMemoryMutex == NULL) 
    {
        printf("Create memory mutex error: %d\r\n", GetLastError());
	    return false;
    }

    gFileMutex = CreateMutex(NULL, FALSE, NULL);
    if (gFileMutex == NULL) 
    {
        printf("Create file's access mutex error: %d\r\n", GetLastError());
		CloseHandle(gMemoryMutex);
	    return false;
    }
#else
    sem_init(&gMemoryMutex, 0, 0);
	sem_post(&gMemoryMutex);

    sem_init(&gFileMutex, 0, 0);
	sem_post(&gFileMutex);
#endif
	return true;
}
//---------------------------------------------------------------------------
void ServSysLibClose()
{
#ifdef WIN32
	CloseHandle(gFileMutex);
	CloseHandle(gMemoryMutex);
#else
	sem_destroy(&gFileMutex);
	sem_destroy(&gMemoryMutex);
#endif
}
//---------------------------------------------------------------------------
int updateSocketSize(int sock, int attribute, unsigned long bufSize)
{
    int            optionLen;
    unsigned long  curSize;
    unsigned long  newSize;

    /* Get the current size of the buffer */
    optionLen = sizeof(curSize);
    if (getsockopt(sock, SOL_SOCKET, attribute,
                   (char *)(&curSize), &optionLen) < 0)
    {
        printf("\nERROR: getsockopt");
        return (-1);
    }

    /* Check if the current size is already big enough */
    if (curSize < bufSize)
    {
        /* Set the new size */
        optionLen = sizeof(bufSize);
        if (setsockopt(sock, SOL_SOCKET, attribute,
                       (char *)(&bufSize), optionLen) < 0)
        {
            printf("\nERROR: setsockopt");
            return (-1);
        }

        /* Retrive the new size and verify it took */
        optionLen = sizeof(newSize);
        if (getsockopt(sock, SOL_SOCKET, attribute,
                       (char *)(&newSize), &optionLen) < 0)
        {
            printf("\nERROR: getsockopt");
            return (-1);
        }

        /* Check the sizes match */
        if (newSize < bufSize)
        {
            printf("\nFailed to set socket size to %ld, size now %ld\n", bufSize, newSize);
            return (-1);
        }
    }
    /* All good */
    return (0);
}
//---------------------------------------------------------------------------
unsigned GetLocalIPServer(char *SrcCmd, char *Param, char *LocalHostName)
{
    int		i, j;
    int		Value;
    unsigned	LocalAddr;
    char*	NextData;
    char*   DataStartPtr;
    int		rs;
    bool	CorrectAddr;
#ifdef WIN32
    WORD	wVersionRequested;
    WSADATA	wsaData;
#endif

	struct in_addr  LocalSAddr = { 0 };
    struct hostent  *LocalHost;

	LocalAddr = 0;
    NextData = SrcCmd;

	DataStartPtr = NextData;
	for (i=0;i < 4;i++)
    {
        if (!sscanf(NextData, "%d", &Value ))
        {
	        LocalAddr = 0;
            break;
        }
	    if (Value > 255)
        {
            LocalAddr = 0;
		    break;
	    }
	    LocalAddr = LocalAddr |(((unsigned)Value)<<(i<<3));
		if ( i < 3 ) 
        {
            for ( CorrectAddr=false, j=0; j < (int)strlen(NextData);j++)
		    {
                if ( (char)NextData[j] == '.' ) 
				{
                    CorrectAddr=true;
			        break;
				}
		    }	
		    if ( CorrectAddr ) NextData = &NextData[j+1];
		    else				 
			{
			    LocalAddr = 0;
			    break;
			}
        }
	}
	if ( !LocalAddr ) return 0;

    //Initialisation socket at TCP protocol.
#ifdef WIN32
    if (Param)
    {
        wVersionRequested = MAKEWORD( 1, 1 );
        rs = WSAStartup( wVersionRequested, &wsaData );
        if ( rs != 0 ) return 0; //No network support.
        if ( LOBYTE( wsaData.wVersion ) != 1 || HIBYTE( wsaData.wVersion ) != 1 ) return 0; //No network support.
    }
#endif
    LocalSAddr.s_addr = inet_addr(DataStartPtr);
    if (LocalSAddr.s_addr == INADDR_NONE)
    {
		return 0;
    }
    else
    {
#ifdef WIN32
		LocalHost = gethostbyaddr((char *) &LocalSAddr, 4, AF_INET);
#else
        LocalHost = gethostbyaddr((const void*)&LocalSAddr, (socklen_t)4, (int)AF_INET);
#endif
        if (LocalHost != NULL)
        {
			strcpy(LocalHostName, LocalHost->h_name);
	    }
	    else
	    {
#ifdef WIN32
	        sprintf(LocalHostName,"%d.%d.%d.%d",
                (int)(LocalAddr&0x000000ff), 
				(int)((LocalAddr&0x0000ff00)>>8), 
				(int)((LocalAddr&0x00ff0000)>>16),
				(int)((LocalAddr&0xff000000)>>24) );
#else    
	        sprintf(LocalHostName,"%d.%d.%d.%d",
    #ifdef _SUN_BUILD_
                (int)((LocalAddr&0xff000000)>>24),
                (int)((LocalAddr&0x00ff0000)>>16),
			    (int)((LocalAddr&0x0000ff00)>>8),
                (int)(LocalAddr&0x000000ff) );
    #else
                (int)(LocalAddr&0x000000ff), 
				(int)((LocalAddr&0x0000ff00)>>8), 
				(int)((LocalAddr&0x00ff0000)>>16),
				(int)((LocalAddr&0xff000000)>>24) );
    #endif
			printf("LocalHostName: %s\n", LocalHostName);
#endif
	    }
    }
	return LocalAddr;
}
//---------------------------------------------------------------------------
