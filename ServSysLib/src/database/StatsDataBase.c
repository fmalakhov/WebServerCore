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
#include "StatsDataBase.h"

ListItsTask  StatsList;

#ifdef WIN32
extern HANDLE gFileMutex;
#endif

extern char StatsDbNamePath[];

/* List of accepted commands for shop database.*/
	char	*TablStatsParsDb[] = {
        "DbStatsCreate",   //The creating of new stats description record;
		"comment"		   //The identificator of not visibled comment for this line;
	};

static char CmdDbStatsId[]      = "StatsId";
static char CmdDbStatsCode[]    = "StatsCode";
static char CmdDbStatsType[]    = "StatsType";
static char CmdDbStatsStyle[]   = "StatsStyle";
static char CmdDbStatsGroup[]   = "StatsGroup";
static char CmdDbStLastUpdate[] = "StatsLastUpdate";
static char CmdDbStatsName[]    = "StatsName";
static char CmdDbStatsDescr[]   = "StatsDescr";

static void CmdAddNewStatsDb(unsigned char *CmdLinePtr);
static STATS_BASE_INFO gStatsBaseInfo;
//---------------------------------------------------------------------------
void StatsDBLoad(unsigned int MinSPGroup, unsigned int MaxSPGroup,
	unsigned int DefSPGroup, unsigned int MinSGStyle,
	unsigned int MaxSGStyle, unsigned int DefSGStyle)
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
	char *CwdRet = NULL;
	unsigned int DbTextLine, read_blk;
	unsigned char *FileData;
	char DbTextItem[MAX_LEN_STATS_BASE_LINE+1];
	char StartPath[512];
	char BdFileName[1024];

	gStatsBaseInfo.MinSPGroup = MinSPGroup;
	gStatsBaseInfo.MaxSPGroup = MaxSPGroup;
	gStatsBaseInfo.DefSPGroup = DefSPGroup;
	gStatsBaseInfo.MinSGStyle = MinSGStyle;
	gStatsBaseInfo.MaxSGStyle = MaxSGStyle;
	gStatsBaseInfo.DefSGStyle = DefSGStyle;
	gStatsBaseInfo.NewStatsIndex = 1;

    StatsList.Count = 0;
	StatsList.CurrTask = NULL;
	StatsList.FistTask = NULL;
#ifdef WIN32
	CwdRet = _getcwd((char*)(&StartPath[0]),512);
#else
	CwdRet = getcwd((char*)(&StartPath[0]),512);
#endif
	strcpy(BdFileName, StartPath);
    strcat(BdFileName, StatsDbNamePath);
#ifdef _LINUX_X86_
	FileHandler = fopen(BdFileName,"rb");
	if (!FileHandler) 
	{
        printf("File DB stats (%s) dos not present\n", BdFileName);
	    return;
	}
        stat(BdFileName, &st);
        if ((st.st_mode & S_IFMT) != S_IFMT)
	{                
            SizeFile = (unsigned long)st.st_size;
        }
        else
        {
            printf("File DB stats (%s) is not file\n", BdFileName);
            fclose(FileHandler);
            return;
        }
#endif
#ifdef WIN32
	HFSndMess = CreateFile((LPCWSTR)BdFileName,0,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (HFSndMess == INVALID_HANDLE_VALUE)
	{
	   return;
	}
	SizeFile = GetFileSize( HFSndMess, NULL );
	CloseHandle( HFSndMess );
        FileHandler = fopen(BdFileName,"rb");
#endif
	FileData = (unsigned char*)AllocateMemory( SizeFile+2 );
	read_blk = fread((unsigned char*)FileData, 1, SizeFile, FileHandler);
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
	                switch ( FindCmdArray( DbTextItem, TablStatsParsDb, 2 ) )
                    {
                        case  0:
						    CmdAddNewStatsDb((unsigned char*)&DbTextItem[0]);
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
			   if (DbTextLine < MAX_LEN_STATS_BASE_LINE)
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
    printf("Stats DB load is done (Records in base: %d)\n", (unsigned int)StatsList.Count);
}
//---------------------------------------------------------------------------
static void CmdAddNewStatsDb(unsigned char *CmdLinePtr)
{
	unsigned int i;
	int          pars_read, ReadValue;
	unsigned int rDay, rMonth, rYear, rHour, rMinute, rSecond;    
	bool         isCompleate = false;
    char         *FText;
    char         *StrPar;
	char		 *FStrt = NULL;
    DEV_STATS_INFO   *NewStatsPtr = NULL;

	for(;;)
	{
	    NewStatsPtr = (DEV_STATS_INFO*)AllocateMemory(sizeof(DEV_STATS_INFO));
		if (!NewStatsPtr) break;
        StrPar = GetZoneParFunction(CmdLinePtr);
        if ( !StrPar ) break;
        FText = (char*)AllocateMemory(strlen(StrPar)+4);
		if (!FText) break;
	    FStrt = FText;
        strcpy(FText,StrPar);

        /* Parse stats id field */
        i = FindCmdRequest(FText, CmdDbStatsId);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewStatsPtr->StatsId = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

        /* Parse stats type field */
        i = FindCmdRequest(FText, CmdDbStatsType);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewStatsPtr->StatsType = (unsigned char)(ReadValue & 0xf);
	    FText = FStrt;
        strcpy(FText, StrPar);

        /* Parse stats style field */
        i = FindCmdRequest(FText, CmdDbStatsStyle);
        if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		if ((ReadValue < (int)gStatsBaseInfo.MinSGStyle) ||(ReadValue > (int)gStatsBaseInfo.MaxSGStyle)) break;
		NewStatsPtr->StatsStyle = (unsigned char)(ReadValue & 0xff);
	    FText = FStrt;
        strcpy(FText, StrPar);

        /* Parse stats group field */
        i = FindCmdRequest(FText, CmdDbStatsGroup);
        if (i != -1)
        {
            FText = ParseParFunction( &FText[i] );
            if ( !FText ) break;
	        pars_read = sscanf(FText, "%d", &ReadValue);
	        if (!pars_read) break;
			if ((ReadValue < (int)gStatsBaseInfo.MinSPGroup) ||(ReadValue > (int)gStatsBaseInfo.MaxSPGroup)) break;
		    NewStatsPtr->StatsGroup = (unsigned char)(ReadValue & 0xff);
	        FText = FStrt;
            strcpy(FText, StrPar);
        }
        else
        {
			NewStatsPtr->StatsGroup = gStatsBaseInfo.DefSPGroup;
        }

        /* Parse stats code field */
        i = FindCmdRequest(FText, CmdDbStatsCode);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewStatsPtr->StatsCode = (unsigned short)(ReadValue & 0xffff);
	    FText = FStrt;
        strcpy(FText, StrPar);

        i = FindCmdRequest(FText, CmdDbStLastUpdate);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
		pars_read = sscanf(FText, "%d.%d.%d %d:%d:%d", 
			&rDay, &rMonth, &rYear, &rHour, &rMinute, &rSecond);
	    if (pars_read != 6) break;
#ifdef WIN32
		NewStatsPtr->LastUpdateDate.wDay = rDay;
		NewStatsPtr->LastUpdateDate.wMonth = rMonth;
		NewStatsPtr->LastUpdateDate.wYear = rYear;
		NewStatsPtr->LastUpdateDate.wHour = rHour;
	    NewStatsPtr->LastUpdateDate.wMinute = rMinute;
		NewStatsPtr->LastUpdateDate.wSecond = rSecond;
		NewStatsPtr->LastUpdateDate.wMilliseconds = 0;
#endif
#ifdef _LINUX_X86_ 
		NewStatsPtr->LastUpdateDate.tm_mday = rDay;
		NewStatsPtr->LastUpdateDate.tm_mon = rMonth - 1;
		NewStatsPtr->LastUpdateDate.tm_year = rYear - 1900;
		NewStatsPtr->LastUpdateDate.tm_hour = rHour;
	    NewStatsPtr->LastUpdateDate.tm_min = rMinute;
		NewStatsPtr->LastUpdateDate.tm_sec = rSecond;
#endif
	    FText = FStrt;
        strcpy(FText,StrPar);

		/* Parse stats name field */
        i = FindCmdRequest(FText, CmdDbStatsName);
	    if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
		strncpy((char*)&NewStatsPtr->StatsName,
			(const char*)FText, MAX_LEN_STATS_NAME);
		FText = FStrt;
		strcpy(FText,StrPar);

		/* Parse stats description field */
        i = FindCmdRequest(FText, CmdDbStatsDescr);
	    if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if ( !FText ) break;
        TextAreaConvertBase(FText, (char*)&NewStatsPtr->StatsDescr, 
            MAX_LEN_STATS_DESCR);
		FText = FStrt;
		strcpy(FText,StrPar);
  
		NewStatsPtr->ObjPtr = AddStructListObj(&StatsList, NewStatsPtr);
		if (gStatsBaseInfo.NewStatsIndex <= NewStatsPtr->StatsId) 
			gStatsBaseInfo.NewStatsIndex = NewStatsPtr->StatsId + 1;
	    isCompleate = true;
		break;
	}
	if (!isCompleate)
	{
		if (NewStatsPtr) FreeMemory(NewStatsPtr);
	}
	if (FStrt) FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
void StatsDbSave()
{
	FILE            *HandleBD;
	char            *PathFilePtr = NULL;
	char            *CmdLinePtr = NULL;
    ObjListTask	    *SelObjPtr = NULL;
	DEV_STATS_INFO      *SelStatsRecPtr = NULL;
	char            *RetCodeCwd = NULL;
	char            *ConvBufPtr = NULL;    
	char            BuLine[128];
#ifdef WIN32
    DWORD           WaitResult;
	bool            isFileOperReady = false;
#endif

	PathFilePtr = (char*)AllocateMemory(1024*sizeof(char));
	if (!PathFilePtr) return;
#ifdef WIN32
	RetCodeCwd = _getcwd(PathFilePtr, 512);
#else
	RetCodeCwd = getcwd(PathFilePtr, 512);
#endif
    strcat(PathFilePtr, StatsDbNamePath);
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
			printf("WebServer (Stats database) mutex is fialed with error: %d\r\n", GetLastError());
			FreeMemory(PathFilePtr);
			break;
	}
    if (!isFileOperReady) return;
#endif
	HandleBD = fopen(PathFilePtr,"wb");
	if (!HandleBD) 
	{
		FreeMemory(PathFilePtr);
		return;
	}
	CmdLinePtr = (char*)AllocateMemory(8192*sizeof(char));
	memset(CmdLinePtr, 0, 8192*sizeof(char));
	SelObjPtr = (ObjListTask*)GetFistObjectList(&StatsList);
	while(SelObjPtr)
	{
	    SelStatsRecPtr = (DEV_STATS_INFO*)SelObjPtr->UsedTask;
		CmdLinePtr[0] = 0;
		sprintf(CmdLinePtr, "%s(%s=\"%d\" ", 
			TablStatsParsDb[0], CmdDbStatsId, 
			SelStatsRecPtr->StatsId);
            
		sprintf(BuLine, "%s=\"%d\" %s=\"%d\" %s=\"%d\" %s=\"%d\" ",
            CmdDbStatsType,   SelStatsRecPtr->StatsType,
            CmdDbStatsStyle,  SelStatsRecPtr->StatsStyle,
            CmdDbStatsGroup,  SelStatsRecPtr->StatsGroup,
            CmdDbStatsCode,   SelStatsRecPtr->StatsCode);
		strcat(CmdLinePtr, BuLine);

		sprintf(BuLine, "%s=\"%d.%d.%d %d:%d:%d\" ",
#ifdef WIN32
		    CmdDbStLastUpdate, SelStatsRecPtr->LastUpdateDate.wDay, SelStatsRecPtr->LastUpdateDate.wMonth,
			SelStatsRecPtr->LastUpdateDate.wYear, SelStatsRecPtr->LastUpdateDate.wHour,
			SelStatsRecPtr->LastUpdateDate.wMinute, SelStatsRecPtr->LastUpdateDate.wSecond);
#else
			CmdDbStLastUpdate, SelStatsRecPtr->LastUpdateDate.tm_mday, SelStatsRecPtr->LastUpdateDate.tm_mon+1,
			SelStatsRecPtr->LastUpdateDate.tm_year+1900, SelStatsRecPtr->LastUpdateDate.tm_hour,
			SelStatsRecPtr->LastUpdateDate.tm_min, SelStatsRecPtr->LastUpdateDate.tm_sec);
#endif
        strcat(CmdLinePtr, BuLine);

		sprintf(BuLine, "%s=\"", CmdDbStatsName);
		strcat(CmdLinePtr, BuLine);
		strcat(CmdLinePtr, (const char*)&SelStatsRecPtr->StatsName[0]);

		sprintf(BuLine, "\" %s=\"", CmdDbStatsDescr);
		strcat(CmdLinePtr, BuLine);
        fwrite(CmdLinePtr, strlen(CmdLinePtr), 1, HandleBD);
                
		ConvBufPtr = TextAreaConvertFile(&SelStatsRecPtr->StatsDescr[0]);
		if (ConvBufPtr)
		{
			fwrite(ConvBufPtr, strlen((const char*)ConvBufPtr), 1, HandleBD);
            FreeMemory(ConvBufPtr);
        }
        
		strcpy(CmdLinePtr, "\")\r\n");
		fwrite(CmdLinePtr, strlen(CmdLinePtr), 1, HandleBD);
		SelObjPtr = (ObjListTask*)GetNextObjectList(&StatsList);
	}
	fclose(HandleBD);
#ifdef WIN32
    if (! ReleaseMutex(gFileMutex)) 
	{ 
        printf("Fail to release mutex (stats database) in web server task\r\n");
	}
#endif    
	if (CmdLinePtr) FreeMemory(CmdLinePtr);
	if (PathFilePtr) FreeMemory(PathFilePtr);
}
//---------------------------------------------------------------------------
void StatsDBClear()
{
    ObjListTask	   *SelObjPtr = NULL;
	DEV_STATS_INFO      *SelStatsRecPtr = NULL;
        
	SelObjPtr = (ObjListTask*)GetFistObjectList(&StatsList);
	while(SelObjPtr)
	{
	    SelStatsRecPtr = (DEV_STATS_INFO*)SelObjPtr->UsedTask;
		FreeMemory(SelStatsRecPtr);
		RemStructList(&StatsList, SelObjPtr);
		SelObjPtr = (ObjListTask*)GetFistObjectList(&StatsList);
	}
}
//---------------------------------------------------------------------------
DEV_STATS_INFO* StatsDbAddItem()
{
	DEV_STATS_INFO   *NewStatsRecPtr = NULL;

	NewStatsRecPtr = (DEV_STATS_INFO*)AllocateMemory(sizeof(DEV_STATS_INFO));
    if (!NewStatsRecPtr) return NULL;
	memset(&NewStatsRecPtr->StatsName, 0, MAX_LEN_STATS_NAME);
    memset(&NewStatsRecPtr->StatsDescr, 0, MAX_LEN_STATS_DESCR);
	NewStatsRecPtr->StatsId = gStatsBaseInfo.NewStatsIndex++;
    NewStatsRecPtr->StatsCode  = 0;
    NewStatsRecPtr->StatsType  = SGT_GENERAL;
	NewStatsRecPtr->StatsStyle = gStatsBaseInfo.DefSGStyle;
	NewStatsRecPtr->StatsGroup = gStatsBaseInfo.DefSPGroup;
    SetStatsLastUpdateTime(NewStatsRecPtr);
    NewStatsRecPtr->ObjPtr = AddStructListObj(&StatsList, NewStatsRecPtr);
	return NewStatsRecPtr;
}
//---------------------------------------------------------------------------
void StatsDbRemItem(DEV_STATS_INFO *RemStatsPtr)
{
    RemStructList(&StatsList, RemStatsPtr->ObjPtr);
    FreeMemory(RemStatsPtr);
    StatsDbSave();
}
//---------------------------------------------------------------------------
void SetStatsLastUpdateTime(DEV_STATS_INFO *StatsPtr)
{
#ifdef _LINUX_X86_        
    struct timeb  hires_cur_time;
    struct tm     *CurrTime;
#endif

#ifdef WIN32
	GetSystemTime(&StatsPtr->LastUpdateDate);
#else
    ftime(&hires_cur_time);
    CurrTime = localtime(&hires_cur_time.time);
    memcpy(&StatsPtr->LastUpdateDate, CurrTime, sizeof(struct tm));
#endif
}
//---------------------------------------------------------------------------
DEV_STATS_INFO* GetStatsByStatsId(unsigned int StatsId)
{
	ObjListTask  *SelObjPtr = NULL;
	DEV_STATS_INFO   *StatsPtr = NULL;
	DEV_STATS_INFO   *FindStatsPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&StatsList);
	while(SelObjPtr)
	{
	    StatsPtr = (DEV_STATS_INFO*)SelObjPtr->UsedTask;
		if ( StatsPtr->StatsId == StatsId)
		{
			FindStatsPtr = StatsPtr;
			break;
		}
		SelObjPtr = (ObjListTask*)GetNextObjectList(&StatsList);
	}
	return FindStatsPtr;
}
//---------------------------------------------------------------------------
DEV_STATS_INFO* GetStatsByStatsCode(unsigned short StatsCode)
{
	ObjListTask  *SelObjPtr = NULL;
	DEV_STATS_INFO   *StatsPtr = NULL;
	DEV_STATS_INFO   *FindStatsPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&StatsList);
	while(SelObjPtr)
	{
	    StatsPtr = (DEV_STATS_INFO*)SelObjPtr->UsedTask;
		if ( StatsPtr->StatsCode == StatsCode)
		{
			FindStatsPtr = StatsPtr;
			break;
		}
		SelObjPtr = (ObjListTask*)GetNextObjectList(&StatsList);
	}
	return FindStatsPtr;
}
//---------------------------------------------------------------------------
