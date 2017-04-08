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

#include "ReaderWorker.h"
#include "ThrReportMen.h"
//---------------------------------------------------------------------------
void CreateReaderThreads(READER_WORKER_INFO *ReaderWorkerPtr)
{
	unsigned int       index;
	unsigned int       InitCreateCount = 0;
    ObjListTask		   *PointTask = NULL;
	READER_WEB_INFO    *ReaderInfoPtr = NULL;
	READWEBSOCK        *ParReadWeb = NULL;
    POOL_RECORD_STRUCT *ReaderPoolRecPtr[START_SIMULT_WEB_READ_HDR+1];

	InitCreateCount = ReaderWorkerPtr->WebReaderPool.ReadWebPool.m_NumUsedRecords;
    if (InitCreateCount >= START_SIMULT_WEB_READ_HDR) return;
    for(;;)
    {
	    ReaderPoolRecPtr[InitCreateCount] = GetBuffer(&ReaderWorkerPtr->WebReaderPool.ReadWebPool);
        ParReadWeb = (READWEBSOCK*)ReaderPoolRecPtr[InitCreateCount]->DataPtr;
	    ParReadWeb->ReadBufPoolPtr = (void*)ReaderPoolRecPtr[InitCreateCount];
	    ParReadWeb->IDMessReadWeb = WSU_USERDATA;
	    ParReadWeb->HTTPReqLen = 0;
	    ParReadWeb->HttpReqPtr = NULL;
	    ParReadWeb->StrCmdHTTP = NULL;
	    ParReadWeb->BoundInd = NULL;
	    ParReadWeb->PicFileBufPtr = NULL;
        ParReadWeb->XmlFileBufPtr = NULL;
	    ParReadWeb->isKeepAlive = false;
        ParReadWeb->isEncodingAccept = false;
	    ParReadWeb->MozilaMainVer = 0;
	    ParReadWeb->MozilaSubVer = 0;
	    ParReadWeb->MobileType = NULL;
		ParReadWeb->ActListObjPtr = NULL;
        InitCreateCount++;
		if (InitCreateCount == START_SIMULT_WEB_READ_HDR) break;
    }

	for (index=0;index < InitCreateCount;index++)
	{
		if (ReaderPoolRecPtr[index])
	        FreeBuffer(&ReaderWorkerPtr->WebReaderPool.ReadWebPool, ReaderPoolRecPtr[index]);
	}

	/* List of reader worker threads creation */
	for(index=0; index < CREATE_SIMULT_WEB_READ_THR;index++)
	{
		ReaderInfoPtr = (READER_WEB_INFO*)AllocateMemory(sizeof(READER_WEB_INFO));
		if (!ReaderInfoPtr)
		{
			DebugLogPrint(NULL, "ReaderWorker: Fail to memory allocation for reader worker instance\r\n");
			break;
		}
		memset(ReaderInfoPtr, 0, sizeof(READER_WEB_INFO));
#ifdef _LINUX_X86_
        ReaderInfoPtr->NotifyPort = ReaderWorkerPtr->WebReaderPool.NextReadNotifyPort;
        ReaderInfoPtr->WebServPort = ReaderWorkerPtr->WebServMsgPort;
		ReaderWorkerPtr->WebReaderPool.NextReadNotifyPort++;
#endif
	    if (ReadWebThreadCreate(ReaderInfoPtr)) ReaderInfoPtr->isThreadReady = true;
	    else
		{
		    DebugLogPrint(NULL, "ReaderWorker: Fail to create HTTP read thread\r\n");
			break;
		}
		AddStructList(&ReaderWorkerPtr->ReaderThrList, ReaderInfoPtr);
    }

	PointTask = (ObjListTask*)GetFistObjectList(&ReaderWorkerPtr->ReaderThrList);
	ReaderWorkerPtr->SelReadWebInfoPtr = (READER_WEB_INFO*)PointTask->UsedTask;

	Sleep(10);
	printf("%d reader tasks are ready to handle requests\n",  index);
}
//---------------------------------------------------------------------------
void ReaderActionDone(READER_WORKER_INFO *ReaderWorkerPtr, READWEBSOCK *ParReadWeb)
{
	ReaderInstClear(ParReadWeb);
    if ((!ReaderWorkerPtr->KeepAliveEnable) || (!ParReadWeb->isKeepAlive))  
	    FreeReadPoolBuf(ReaderWorkerPtr, ParReadWeb);
}
//---------------------------------------------------------------------------
void FreeReadPoolBuf(READER_WORKER_INFO *ReaderWorkerPtr, READWEBSOCK *ParReadWeb)
{
#ifdef WIN32
	DWORD       WaitResult;
	bool        isPoolReady = false;

    WaitResult = WaitForSingleObject(ReaderWorkerPtr->WebReaderPool.ReadPoolMutex, INFINITE);
    switch(WaitResult)
	{
	    case WAIT_OBJECT_0:
			isPoolReady = true;
		    break;

        case WAIT_ABANDONED: 
			printf("The other thread that using mutex is closed in locked state of mutex\r\n");
            break;

		default:
			printf("Web read pool mutex is fialed with error: %d\r\n", GetLastError());
			break;
	}
    if (!isPoolReady)
	{
        if (! ReleaseMutex(ReaderWorkerPtr->WebReaderPool.ReadPoolMutex)) 
		{ 
            printf("Fail to release mutex (Web read pool)\r\n");
		}
	    return;
	}
#else
    sem_wait(&ReaderWorkerPtr->WebReaderPool.ReadPoolMutex);
#endif
	ReaderInstClear(ParReadWeb);
	//if (ParReadWeb->SslPtr) printf("Reader worker free with active SSL\n");\

	FreeBuffer(&ReaderWorkerPtr->WebReaderPool.ReadWebPool,
		(POOL_RECORD_STRUCT*)ParReadWeb->ReadBufPoolPtr);

#ifdef WIN32
    if (!ReleaseMutex(ReaderWorkerPtr->WebReaderPool.ReadPoolMutex)) 
        printf("Fail to release mutex (Web read pool)\r\n");
#else
    sem_post(&ReaderWorkerPtr->WebReaderPool.ReadPoolMutex);
#endif
}
//---------------------------------------------------------------------------
void StopReadThreads(READER_WORKER_INFO *ReaderWorkerPtr)
{
    ObjListTask		 *PointTask = NULL;
	READER_WEB_INFO  *ReadWebInfoPtr = NULL;
 
	PointTask = (ObjListTask*)GetFistObjectList(&ReaderWorkerPtr->ReaderThrList);
	while(PointTask)
	{
		ReadWebInfoPtr = (READER_WEB_INFO*)PointTask->UsedTask;
		if (ReadWebInfoPtr->isThreadReady) StopReaderThread(ReadWebInfoPtr);
        FreeMemory(ReadWebInfoPtr);
		RemStructList(&ReaderWorkerPtr->ReaderThrList, PointTask);
		PointTask = (ObjListTask*)GetFistObjectList(&ReaderWorkerPtr->ReaderThrList);
	}
}
//---------------------------------------------------------------------------
#ifdef WIN32
bool ReaderWorkerInit(READER_WORKER_INFO *ReaderWorkerPtr)
{
    ReaderWorkerPtr->WebReaderPool.ReadPoolMutex = CreateMutex(NULL, FALSE, NULL);
    if (ReaderWorkerPtr->WebReaderPool.ReadPoolMutex == NULL) 
	{
        printf("Create read pool access mutex error: %d\r\n", GetLastError());
	    return false;
	}

	ReaderWorkerPtr->ActivHttpReadList.Count = 0;
	ReaderWorkerPtr->ActivHttpReadList.CurrTask = NULL;
	ReaderWorkerPtr->ActivHttpReadList.FistTask = NULL;

	ReaderWorkerPtr->ReaderThrList.Count = 0;
	ReaderWorkerPtr->ReaderThrList.CurrTask = NULL;
	ReaderWorkerPtr->ReaderThrList.FistTask = NULL;

	CreatePool(&ReaderWorkerPtr->WebReaderPool.ReadWebPool, START_SIMULT_WEB_READ_HDR, sizeof(READWEBSOCK));
	CreateReaderThreads(ReaderWorkerPtr);
	ReaderWorkerPtr->isActive = true;
	return true;
}
#else
void ReaderWorkerInit(READER_WORKER_INFO *ReaderWorkerPtr)
{
    sem_init(&ReaderWorkerPtr->WebReaderPool.ReadPoolMutex, 0, 0);
	sem_post(&ReaderWorkerPtr->WebReaderPool.ReadPoolMutex);
	ReaderWorkerPtr->ActivHttpReadList.Count = 0;
	ReaderWorkerPtr->ActivHttpReadList.CurrTask = NULL;
	ReaderWorkerPtr->ActivHttpReadList.FistTask = NULL;
	ReaderWorkerPtr->ReaderThrList.Count = 0;
	ReaderWorkerPtr->ReaderThrList.CurrTask = NULL;
	ReaderWorkerPtr->ReaderThrList.FistTask = NULL;
	CreatePool(&ReaderWorkerPtr->WebReaderPool.ReadWebPool, START_SIMULT_WEB_READ_HDR, sizeof(READWEBSOCK));
	ReaderWorkerPtr->WebReaderPool.NextReadNotifyPort = ReaderWorkerPtr->BaseReaderPort;
	CreateReaderThreads(ReaderWorkerPtr);
	ReaderWorkerPtr->isActive = true;
}
//---------------------------------------------------------------------------
void ReaderWorkerClose(READER_WORKER_INFO *ReaderWorkerPtr)
{
	if (!ReaderWorkerPtr->isActive) return;
    StopReadThreads(ReaderWorkerPtr);
	DestroyPool(&ReaderWorkerPtr->WebReaderPool.ReadWebPool);
	sem_destroy(&ReaderWorkerPtr->WebReaderPool.ReadPoolMutex);
}
#endif
//---------------------------------------------------------------------------
void HttpLoadAddList(READER_WORKER_INFO *ReaderWorkerPtr, READWEBSOCK *ParReadWeb)
{
    ParReadWeb->ActListObjPtr = AddStructListObj(&ReaderWorkerPtr->ActivHttpReadList, ParReadWeb);
}
//---------------------------------------------------------------------------
void HttpLoadRemList(READER_WORKER_INFO *ReaderWorkerPtr, READWEBSOCK *ParReadWeb)
{
	if (ParReadWeb->ActListObjPtr) RemStructList(&ReaderWorkerPtr->ActivHttpReadList, ParReadWeb->ActListObjPtr);
	ParReadWeb->ActListObjPtr = NULL;
}
//---------------------------------------------------------------------------
unsigned int GetNumberActiveReaders(READER_WORKER_INFO *ReaderWorkerPtr)
{
    return (unsigned int)ReaderWorkerPtr->ActivHttpReadList.Count;
}
//---------------------------------------------------------------------------
void CloseHttpReader(READER_WORKER_INFO *ReaderWorkerPtr, READWEBSOCK *ParReadWeb)
{
	if (ParReadWeb->SslPtr)
	{
		SSL_set_shutdown(ParReadWeb->SslPtr, SSL_SENT_SHUTDOWN | SSL_RECEIVED_SHUTDOWN);
		SSL_free(ParReadWeb->SslPtr);
		/*
        SSL_shutdown(ParReadWeb->SslPtr);        
        if (ParReadWeb->BioPtr) BIO_free_all(ParReadWeb->BioPtr);
        if (ParReadWeb->AcceptBioPtr) BIO_free_all(ParReadWeb->AcceptBioPtr);
		*/
	}
	CloseHttpSocket(ParReadWeb->HttpSocket);
	ParReadWeb->BioPtr = NULL;
	ParReadWeb->AcceptBioPtr = NULL;
	ParReadWeb->SslPtr = NULL;
	ParReadWeb->HttpSocket = -1;
	if (ReaderWorkerPtr) FreeReadPoolBuf(ReaderWorkerPtr, ParReadWeb);
}
//---------------------------------------------------------------------------
READER_WEB_INFO* GetReaderInstanceBase(READER_WORKER_INFO *ReaderWorkerPtr)
{
	register unsigned int           Usage, MinUsage;
    register ObjListTask		    *PointTask = NULL;
	register READER_WEB_INFO        *ReadWebInfoPtr = NULL;
	register READER_WEB_INFO        *MinReadWebInfoPtr = NULL;
	register ListItsTask            *ListTasks = NULL;

	PointTask = (ObjListTask*)GetFistObjectList(&ReaderWorkerPtr->ReaderThrList);
	while(PointTask)
	{
	    ReadWebInfoPtr = (READER_WEB_INFO*)PointTask->UsedTask;
		if (!MinReadWebInfoPtr)
		{
			MinReadWebInfoPtr = ReadWebInfoPtr;
			MinUsage = GetUsageReaderQueue(ReadWebInfoPtr);
			if (!MinUsage) break;
		}
		else
		{
		    Usage = GetUsageReaderQueue(ReadWebInfoPtr);
		    if (!Usage)
			{
				/* Found reader instance without messages */
				MinReadWebInfoPtr = ReadWebInfoPtr;
				break;
			}
			if (MinUsage > Usage)
			{
				Usage = MinUsage;
				MinReadWebInfoPtr = ReadWebInfoPtr;
			}
		}
		PointTask = (ObjListTask*)GetNextObjectList(&ReaderWorkerPtr->ReaderThrList);
	}
	return MinReadWebInfoPtr;
}
//---------------------------------------------------------------------------
READER_WEB_INFO* GetReaderInstance(READER_WORKER_INFO *ReaderWorkerPtr)
{
	register READER_WEB_INFO        *MinReadWebInfoPtr = NULL;
	register WEB_READER_POOL_ACCESS *ReaderPoolPtr = NULL;

	ReaderPoolPtr = &ReaderWorkerPtr->WebReaderPool;
#ifdef WIN32
	if (WaitForSingleObject(ReaderPoolPtr->ReadPoolMutex, INFINITE) == WAIT_FAILED)
	{
	    printf("Fail to set read pool mutex (MpttCallClose)\r\n");
		return NULL;
	}
#else
	sem_wait(&ReaderPoolPtr->ReadPoolMutex);
#endif
    MinReadWebInfoPtr = GetReaderInstanceBase(ReaderWorkerPtr);
#ifdef WIN32
    if (!ReleaseMutex(ReaderPoolPtr->ReadPoolMutex)) 
       printf("Fail to release read pool mutex (MpttCallClose)\r\n");
#else
	sem_post(&ReaderPoolPtr->ReadPoolMutex);
#endif
	return MinReadWebInfoPtr;
}
//---------------------------------------------------------------------------
READWEBSOCK* GetWebReaderBlock(READER_WORKER_INFO *ReaderWorkerPtr)
{
	READER_WEB_INFO    *ReadWebInfoPtr = NULL;
	READWEBSOCK        *ParReadWeb = NULL;
	POOL_RECORD_STRUCT *ReadWebBufPtr = NULL;
	WEB_READER_POOL_ACCESS *ReaderPoolPtr = NULL;

	ReaderPoolPtr = &ReaderWorkerPtr->WebReaderPool;
#ifdef WIN32
	if (WaitForSingleObject(ReaderPoolPtr->ReadPoolMutex, INFINITE) == WAIT_FAILED)
	{
	    printf("Fail to set read pool mutex (MpttCallClose)\r\n");
		return NULL;
	}
#else
    sem_wait(&ReaderPoolPtr->ReadPoolMutex);
#endif

	if (ReaderPoolPtr->ReadWebPool.m_NumUsedRecords < MAX_OPEN_READ_POOL)
	{
	    ReadWebBufPtr = GetBuffer(&ReaderPoolPtr->ReadWebPool);
	    ParReadWeb = (READWEBSOCK*)ReadWebBufPtr->DataPtr;
        ParReadWeb->ReadBufPoolPtr = (void*)ReadWebBufPtr;
	}
	else
	{
		DebugLogPrint(NULL, "ReaderWorker: Max limit (%d) of open read pools is reached\r\n", MAX_OPEN_READ_POOL);
	}

#ifdef WIN32
    if (!ReleaseMutex(ReaderPoolPtr->ReadPoolMutex)) 
        printf("Fail to release mutex (Web read pool)\r\n");
#else
    sem_post(&ReaderPoolPtr->ReadPoolMutex);
#endif
	return ParReadWeb;
}
//---------------------------------------------------------------------------
void ReaderInstQueueUsageReport(READER_WORKER_INFO *ReaderWorkerPtr)
{
	READER_WEB_INFO *ReadWebInfoPtr = NULL;
    ObjListTask	    *SelObjPtr = NULL;
	char            RecLine[16];
	char            StatusLine[2000];

	strcpy(StatusLine, "Reader queues: ");
    SelObjPtr = (ObjListTask*)GetFistObjectList(&ReaderWorkerPtr->ReaderThrList);
	while(SelObjPtr)
	{
		ReadWebInfoPtr = (READER_WEB_INFO*)SelObjPtr->UsedTask;
#ifdef WIN32
		sprintf(RecLine, "%u %u/%u;", ReadWebInfoPtr->MsgNotifyId, 
			GetMaxUsageReaderQueue(ReadWebInfoPtr), GetUsageReaderQueue(ReadWebInfoPtr));
#else
		sprintf(RecLine, "%u %u/%u;", ReadWebInfoPtr->NotifyPort, 
			GetMaxUsageReaderQueue(ReadWebInfoPtr), GetUsageReaderQueue(ReadWebInfoPtr));
#endif
		strcat(StatusLine, RecLine);
		SelObjPtr = (ObjListTask*)GetNextObjectList(&ReaderWorkerPtr->ReaderThrList);
	}
	DebugLogPrint(NULL, "%s\r\n", StatusLine);
}
//---------------------------------------------------------------------------
