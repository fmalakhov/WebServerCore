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

#include <sys/stat.h>
#include "BackRequestDataBase.h"

unsigned int NewBackReqIndex = 1;

ListItsTask  BackReqList;

extern char BackReqDbNamePath[];

#ifdef WIN32
extern HANDLE gFileMutex;
#endif

#define MAX_LEN_BACKREQ_BASE_LINE 3000

/* List of accepted commands for shop database.*/
	char	*TablBackReqParsDb[] = {
        "DbBackReqCreate",  /* The creating of new back request ; */
		"comment"			/* The identificator of not visibled comment for this line; */
	};

	char CmdDbBackReqSubmit[] = "BackReqSubmit";
    char CmdDbBackReqId[]     = "BackReqId";
	char CmdDbBackReqOper[]   = "BackReqOper";
	char CmdDbBackReqCmplt[]  = "BackReqCmplt";
	char CmdDbBackReqMail[]   = "BackReqMail";
	char CmdDbBackReqPhone[]  = "BackReqPhone";
	char CmdDbBackReqName[]   = "BackReqName";
	char CmdDbBackReqMsg[]    = "BackReqMsg";
    char CmdDbBackReqResp[]   = "BackReqShopResp";

static void CmdAddNewBackReqDb(unsigned char *CmdLinePtr);
//---------------------------------------------------------------------------
void BackReqDBLoad()
{
	DWORD PointFind;
	bool     LastRot;
#ifdef WIN32
	DWORD  SizeFile;
	HANDLE HFSndMess;
#else
	unsigned long SizeFile;
    struct stat st;
#endif
	FILE   *FileHandler;
	int pars_read = 0;
	unsigned int DbTextLine;
	unsigned int ReadRes;
	unsigned char *FileData;
	char *CwdRet = NULL;
	char DbTextItem[MAX_LEN_BACKREQ_BASE_LINE+1];
	char StartPath[512];
	char BdFileName[1024];

    BackReqList.Count = 0;
	BackReqList.CurrTask = NULL;
	BackReqList.FistTask = NULL;

#ifdef WIN32
	CwdRet = _getcwd((char*)(&StartPath[0]),512);
#else
	CwdRet = getcwd((char*)(&StartPath[0]),512);
#endif
	strcpy(BdFileName, StartPath);
	strcat(BdFileName, BackReqDbNamePath);
#ifdef _LINUX_X86_
	FileHandler = fopen(BdFileName,"rb");
	if (!FileHandler) 
	{
        printf("No records in DB back requests.\n");
	    return;
	}
        stat(BdFileName, &st);
        if ((st.st_mode & S_IFMT) != S_IFMT)
	{                
            SizeFile = (unsigned long)st.st_size;
        }
        else
        {
            printf("File DB back requests (%s) is not file\n", BdFileName);
            fclose(FileHandler);
            return;
        }
#endif
#ifdef WIN32
	HFSndMess = CreateFile((LPCWSTR)BdFileName,0,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (HFSndMess == INVALID_HANDLE_VALUE)
	{
	   printf("No records in DB back requests.\n");
	   return;
	}
	SizeFile = GetFileSize(HFSndMess, NULL);
	CloseHandle( HFSndMess );
	FileHandler = fopen(BdFileName,"rb");
#endif
	FileData = (unsigned char*)AllocateMemory(SizeFile+2);
	ReadRes = fread((unsigned char*)FileData, 1, SizeFile, FileHandler);
	LastRot = false;
    PointFind = 0;
	DbTextLine = 0;
	while ( PointFind < SizeFile )
    {
		if (FileData[PointFind] == 0x00) break;
		if ( FileData[PointFind] =='\r' || FileData[PointFind] =='\n')
        {
			if ( LastRot ) goto NoMove;
            if ( FileData[PointFind] =='\r' ) LastRot = true;
            DbTextItem[DbTextLine] = 0;
            if (FindCmdRequest(DbTextItem, "comment") == -1)
            {
				if ( DbTextItem[0] !='\r' && DbTextItem[0] !='\n')
		        {
					LastRot = true;
			        DbTextItem[DbTextLine] = 0;
	                switch ( FindCmdArray( DbTextItem, TablBackReqParsDb, 2 ) )
                    {
                        case  0:
						    CmdAddNewBackReqDb((unsigned char*)&DbTextItem[0]);
							break;

		                default:
							break;
	                }					
					DbTextLine = 0;
			        PointFind++;
		        }
		    }
		    else
		    {
				DbTextLine = 0;
			    FileData[DbTextLine] = 0;
		    }
		    PointFind++;
	   }
       else
       {
   NoMove:
           if ((FileData[PointFind] !='\r') &&
			   (FileData[PointFind] !='\n') &&
			   (FileData[PointFind] != 0))
           {
			   if (DbTextLine < MAX_LEN_BACKREQ_BASE_LINE)
			   {
			       DbTextItem[DbTextLine] = FileData[PointFind];
                   DbTextLine++;
			   }
           }
           PointFind++;
		   LastRot = false;
        }
    }
	fclose(FileHandler);
	FreeMemory(FileData);
	printf("Back requests databade load is done: (Load back requests=%u)\n",
        (unsigned int)BackReqList.Count);
}
//---------------------------------------------------------------------------
static void CmdAddNewBackReqDb(unsigned char *CmdLinePtr)
{
	unsigned int  i;
	unsigned int  rDay, rMonth, rYear, rHour, rMinute, rSecond;
	int           pars_read, ReadValue;
	bool          isCompleate = false;
    char          *FText;
    char          *StrPar;
	char		  *FStrt = NULL;
    BACK_USER_INFO_REQ *NewBackReqPtr = NULL;

	for(;;)
	{
	    NewBackReqPtr = (BACK_USER_INFO_REQ*)AllocateMemory(sizeof(BACK_USER_INFO_REQ));
		if (!NewBackReqPtr) break;
		memset(NewBackReqPtr, 0, sizeof(BACK_USER_INFO_REQ));

        StrPar = GetZoneParFunction(CmdLinePtr);
        if ( !StrPar ) break;
        FText = (char*)AllocateMemory( strlen(StrPar)+4 );
		if (!FText) break;
	    FStrt = FText;
        strcpy(FText,StrPar);

        i = FindCmdRequest(FText, CmdDbBackReqId);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewBackReqPtr->BackReqId = ReadValue;
	    FText = FStrt;
        strcpy(FText,StrPar);

        i = FindCmdRequest(FText, CmdDbBackReqCmplt);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 0) || (ReadValue > 1)) break;
		NewBackReqPtr->IsCompleted = (bool)ReadValue;
	    FText = FStrt;
        strcpy(FText,StrPar);

        i = FindCmdRequest(FText, CmdDbBackReqSubmit);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
		pars_read = sscanf(FText, "%d.%d.%d %d:%d:%d", 
			&rDay, &rMonth, &rYear, &rHour, &rMinute, &rSecond);
	    if (pars_read != 6) break;
#ifdef WIN32
		NewBackReqPtr->SubmitDate.wDay = rDay;
		NewBackReqPtr->SubmitDate.wMonth = rMonth;
		NewBackReqPtr->SubmitDate.wYear = rYear;
		NewBackReqPtr->SubmitDate.wHour = rHour;
	    NewBackReqPtr->SubmitDate.wMinute = rMinute;
		NewBackReqPtr->SubmitDate.wSecond = rSecond;
		NewBackReqPtr->SubmitDate.wMilliseconds = 0;
#else 
		NewBackReqPtr->SubmitDate.tm_mday = rDay;
		NewBackReqPtr->SubmitDate.tm_mon = rMonth - 1;
		NewBackReqPtr->SubmitDate.tm_year = rYear - 1900;
		NewBackReqPtr->SubmitDate.tm_hour = rHour;
	    NewBackReqPtr->SubmitDate.tm_min = rMinute;
		NewBackReqPtr->SubmitDate.tm_sec = rSecond;
#endif
	    FText = FStrt;
        strcpy(FText,StrPar);

        i = FindCmdRequest(FText, CmdDbBackReqName);
	    if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
		TextAreaConvertBase(FText, (char*)&NewBackReqPtr->UserName, 
			MAX_LEN_USER_INFO_NAME);
		FText = FStrt;
		strcpy(FText,StrPar);

        i = FindCmdRequest(FText, CmdDbBackReqOper);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < 1) || (ReadValue > 2)) break;
		NewBackReqPtr->BackOperId = ReadValue;
	    FText = FStrt;
        strcpy(FText,StrPar);

		if (NewBackReqPtr->BackOperId == 1)
		{
            i = FindCmdRequest(FText, CmdDbBackReqPhone);
	        if (i == -1) break;
            FText = ParseParFunction( &FText[i] );
            if ( !FText ) break;
		    TextAreaConvertBase(FText, (char*)&NewBackReqPtr->UserPhone, 
			    MAX_LEN_PHONE_NUM);
		    FText = FStrt;
		    strcpy(FText,StrPar);
		}
		else
		{
            i = FindCmdRequest(FText, CmdDbBackReqMail);
	        if (i == -1) break;
            FText = ParseParFunction( &FText[i] );
            if (!FText) break;
		    TextAreaConvertBase(FText, (char*)&NewBackReqPtr->UserMail,
			    MAX_LEN_USER_INFO_EMAIL);
		    FText = FStrt;
		    strcpy(FText,StrPar);

            i = FindCmdRequest(FText, CmdDbBackReqPhone);
	        if (i == -1) break;
            FText = ParseParFunction( &FText[i] );
            if ( !FText ) break;
		    TextAreaConvertBase(FText, (char*)&NewBackReqPtr->UserPhone, 
			    MAX_LEN_PHONE_NUM);
		    FText = FStrt;
		    strcpy(FText,StrPar);
		}

        i = FindCmdRequest(FText, CmdDbBackReqMsg);
	    if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		TextAreaConvertBase(FText, (char*)&NewBackReqPtr->UserMessage, 
			MAX_LEN_USER_BACK_MSG);
		FText = FStrt;
		strcpy(FText,StrPar);

        i = FindCmdRequest(FText, CmdDbBackReqResp);
	    if (i != -1)
		{
            FText = ParseParFunction(&FText[i]);
            if (!FText) break;
		    TextAreaConvertBase(FText, (char*)&NewBackReqPtr->ShopRespMsg, 
			    MAX_LEN_USER_BACK_MSG);
		    FText = FStrt;
		    strcpy(FText,StrPar);
		}

		AddStructList(&BackReqList, NewBackReqPtr);
		if (NewBackReqIndex <= NewBackReqPtr->BackReqId) 
			NewBackReqIndex = NewBackReqPtr->BackReqId + 1;
	    isCompleate = true;
		break;
	}
	if (!isCompleate)
	{
		if (NewBackReqPtr) FreeMemory(NewBackReqPtr);
	}
	if (FStrt) FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
void BackReqDbSave()
{
	FILE            *HandleBD;
	char            *PathFilePtr = NULL;
	char            *CmdLinePtr = NULL;
    ObjListTask	    *SelObjPtr = NULL;
	char            *CwdRet = NULL;
	BACK_USER_INFO_REQ *SelBackReqPtr = NULL;
	char            *ConvBufPtr = NULL;
	char            BuLine[64];
    DWORD           WaitResult;
#ifdef WIN32
	bool            isFileOperReady = false;
#endif

	PathFilePtr = (char*)AllocateMemory(1024*sizeof(char));
	if (!PathFilePtr) return;
#ifdef WIN32
	CwdRet = _getcwd(PathFilePtr, 512);
#else
	CwdRet = getcwd(PathFilePtr, 512);
#endif
	strcat(PathFilePtr, BackReqDbNamePath);
#ifdef WIN32
    WaitResult = WaitForSingleObject(gFileMutex, INFINITE);
    switch(WaitResult)
	{
	    case WAIT_OBJECT_0:
			isFileOperReady = true;
		    break;

        case WAIT_ABANDONED: 
			printf("The other thread that using mutex is closed in locked state of mutex\r\n");
            FreeMemory(PathFilePtr);
            break;

		default:
			printf("Report manager (back requests database) mutex is fialed with error: %d\r\n", GetLastError());
			FreeMemory(PathFilePtr);
			break;
	}
	if (!isFileOperReady) return;
#endif
	HandleBD = fopen(PathFilePtr,"wb");
	if ( !HandleBD ) 
	{
		FreeMemory(PathFilePtr);
#ifdef WIN32
        if (! ReleaseMutex(gFileMutex)) 
		{ 
            printf("Fail to release mutex (back requests database) in report manager task\r\n");
		}
#endif
		return;
	}
	CmdLinePtr = (char*)AllocateMemory(8192*sizeof(char));
	memset(CmdLinePtr, 0, 8192*sizeof(char));
	SelObjPtr = (ObjListTask*)GetFistObjectList(&BackReqList);
	while(SelObjPtr)
	{
	    SelBackReqPtr = (BACK_USER_INFO_REQ*)SelObjPtr->UsedTask;
		CmdLinePtr[0] = 0;
		sprintf(CmdLinePtr, "%s(%s=\"%d\" %s=\"%d\" %s=\"%d\" %s=\"", TablBackReqParsDb[0], 
			CmdDbBackReqId, SelBackReqPtr->BackReqId,
			CmdDbBackReqOper, SelBackReqPtr->BackOperId,
			CmdDbBackReqCmplt, SelBackReqPtr->IsCompleted, CmdDbBackReqName);
 
		strcat(CmdLinePtr, (const char*)&SelBackReqPtr->UserName[0]);
		strcat(CmdLinePtr, "\" ");
		if (SelBackReqPtr->BackOperId == 1)
		{
            sprintf(BuLine, " %s=\"", CmdDbBackReqPhone);
			strcat(CmdLinePtr, BuLine);
			strcat(CmdLinePtr, (const char*)&SelBackReqPtr->UserPhone[0]);
			strcat(CmdLinePtr, "\" ");
		}
		else
		{
            sprintf(BuLine, " %s=\"", CmdDbBackReqMail);
			strcat(CmdLinePtr, BuLine);
			strcat(CmdLinePtr, (const char*)&SelBackReqPtr->UserMail[0]);
			strcat(CmdLinePtr, "\" ");

            sprintf(BuLine, " %s=\"", CmdDbBackReqPhone);
			strcat(CmdLinePtr, BuLine);
			strcat(CmdLinePtr, (const char*)&SelBackReqPtr->UserPhone[0]);
			strcat(CmdLinePtr, "\" ");
		}

#ifdef WIN32
		sprintf(BuLine, "%s=\"%d.%d.%d %d:%d:%d\" %s=\"",
			CmdDbBackReqSubmit, SelBackReqPtr->SubmitDate.wDay, SelBackReqPtr->SubmitDate.wMonth,
			SelBackReqPtr->SubmitDate.wYear, SelBackReqPtr->SubmitDate.wHour,
			SelBackReqPtr->SubmitDate.wMinute, SelBackReqPtr->SubmitDate.wSecond, CmdDbBackReqMsg);
	    strcat(CmdLinePtr, BuLine);
#else 
		sprintf(BuLine, "%s=\"%d.%d.%d %d:%d:%d\" %s=\"",
			CmdDbBackReqSubmit, SelBackReqPtr->SubmitDate.tm_mday, SelBackReqPtr->SubmitDate.tm_mon+1,
			SelBackReqPtr->SubmitDate.tm_year+1900, SelBackReqPtr->SubmitDate.tm_hour,
			SelBackReqPtr->SubmitDate.tm_min, SelBackReqPtr->SubmitDate.tm_sec, CmdDbBackReqMsg);
		strcat(CmdLinePtr, BuLine);
#endif
		fwrite(CmdLinePtr, strlen(CmdLinePtr), 1, HandleBD);

		CmdLinePtr[0] = 0;
		ConvBufPtr = TextAreaConvertFile(&SelBackReqPtr->UserMessage[0]);
		if (ConvBufPtr)
		{
		    fwrite(ConvBufPtr, strlen((const char*)ConvBufPtr), 1, HandleBD);
            FreeMemory(ConvBufPtr);
			strcpy(CmdLinePtr, "\" ");
        }

        if (strlen(SelBackReqPtr->ShopRespMsg) > 0)
		{
			strcat(CmdLinePtr, CmdDbBackReqResp);
            strcat(CmdLinePtr, "=\"");
			fwrite(CmdLinePtr, strlen(CmdLinePtr), 1, HandleBD);
			CmdLinePtr[0] = 0;
		    ConvBufPtr = TextAreaConvertFile(&SelBackReqPtr->ShopRespMsg[0]);
		    if (ConvBufPtr)
			{
		        fwrite(ConvBufPtr, strlen((const char*)ConvBufPtr), 1, HandleBD);
                FreeMemory(ConvBufPtr);
				strcpy(CmdLinePtr, "\" ");
			}
		}
		strcat(CmdLinePtr, ")\r\n");
		fwrite(CmdLinePtr, strlen(CmdLinePtr), 1, HandleBD);

		SelObjPtr = (ObjListTask*)GetNextObjectList(&BackReqList);
	}
	fclose(HandleBD);
#ifdef WIN32
    if (! ReleaseMutex(gFileMutex)) 
	{ 
        printf("Fail to release mutex (back requests database) in report manager task\r\n");
	}
#endif
	if (CmdLinePtr) FreeMemory(CmdLinePtr);
	if (PathFilePtr) FreeMemory(PathFilePtr);
}
//---------------------------------------------------------------------------
void BackReqDBClear()
{
    ObjListTask	         *SelObjPtr = NULL;
	BACK_USER_INFO_REQ   *SelBackReqRecPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&BackReqList);
	while(SelObjPtr)
	{
	    SelBackReqRecPtr = (BACK_USER_INFO_REQ*)SelObjPtr->UsedTask;
		RemStructList(&BackReqList, SelObjPtr);
		FreeMemory(SelBackReqRecPtr);
		SelObjPtr = (ObjListTask*)GetFistObjectList(&BackReqList);
	}
}
//---------------------------------------------------------------------------
BACK_USER_INFO_REQ* BackReqDbAddBackReq()
{
	BACK_USER_INFO_REQ*   NewReqPtr = NULL;
#ifdef _LINUX_X86_        
    struct timeb  hires_cur_time;
    struct tm     *CurrTime;
#endif

	NewReqPtr = (BACK_USER_INFO_REQ*)AllocateMemory(sizeof(BACK_USER_INFO_REQ));
	if (!NewReqPtr) return NULL;
	memset(NewReqPtr, 0, sizeof(BACK_USER_INFO_REQ));

#ifdef WIN32
	GetSystemTime(&NewReqPtr->SubmitDate);
#endif
#ifdef _LINUX_X86_
    ftime(&hires_cur_time);
    CurrTime = localtime(&hires_cur_time.time);
    memcpy(&NewReqPtr->SubmitDate, CurrTime, sizeof(struct tm));
#endif
	NewReqPtr->BackReqId = NewBackReqIndex++;
	return NewReqPtr;
}
//---------------------------------------------------------------------------
