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
#include "GroupDataBase.h"

ListItsTask  GroupList;

#ifdef WIN32
extern HANDLE gFileMutex;
#endif

extern char GroupDbNamePath[];

/* List of accepted commands for shop database.*/
	char	*TablGroupParsDb[] = {
        "DbGroupCreate",   // The creating of new group description record;
		"comment",		   // The identificator of not visibled comment for this line;
	};

char CmdDbGroupId[]   = "GroupId";
char CmdDbGroupName[] = "GroupName";

void CmdAddNewGroupDb(unsigned char *CmdLinePtr);
//---------------------------------------------------------------------------
void GroupDBLoad()
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
	char DbTextItem[MAX_LEN_GROUP_BASE_LINE+1];
	char StartPath[512];
	char BdFileName[1024];

    GroupList.Count = 0;
	GroupList.CurrTask = NULL;
	GroupList.FistTask = NULL;

#ifdef WIN32
	CwdRet = _getcwd((char*)(&StartPath[0]),512);
#else
	CwdRet = getcwd((char*)(&StartPath[0]),512);
#endif
	strcpy(BdFileName, StartPath);
    strcat(BdFileName, GroupDbNamePath);
#ifdef _LINUX_X86_
	FileHandler = fopen(BdFileName,"rb");
	if (!FileHandler) 
	{
        printf("File DB groups (%s) dos not present\n", BdFileName);
	    return;
	}
        stat(BdFileName, &st);
        if ((st.st_mode & S_IFMT) != S_IFMT)
	{                
            SizeFile = (unsigned long)st.st_size;
        }
        else
        {
            printf("File DB groups (%s) is not file\n", BdFileName);
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
	                switch ( FindCmdArray( DbTextItem, TablGroupParsDb, 2 ) )
                    {
                        case  0:
						    CmdAddNewGroupDb((unsigned char*)&DbTextItem[0]);
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
			   if (DbTextLine < MAX_LEN_GROUP_BASE_LINE)
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
    printf("Group DB load is done (Records in base: %d)\n", (unsigned int)GroupList.Count);
}
//---------------------------------------------------------------------------
void CmdAddNewGroupDb(unsigned char *CmdLinePtr)
{
	unsigned int i;
	int          pars_read, ReadValue; 
	bool         isCompleate = false;
    char         *FText;
    char         *StrPar;
	char		 *FStrt = NULL;
    GROUP_INFO_TYPE   *NewGroupPtr = NULL;

	for(;;)
	{
	    NewGroupPtr = (GROUP_INFO_TYPE*)AllocateMemory(sizeof(GROUP_INFO_TYPE));
		if (!NewGroupPtr) break;
        StrPar = GetZoneParFunction(CmdLinePtr);
        if ( !StrPar ) break;
        FText = (char*)AllocateMemory(strlen(StrPar)+4);
		if (!FText) break;
	    FStrt = FText;
        strcpy(FText,StrPar);

        /* Parse group id field */
        i = FindCmdRequest(FText, CmdDbGroupId);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
        if ((ReadValue < MIN_USER_GRP_ID) || (ReadValue > MAX_USER_GRP_ID)) break;
		NewGroupPtr->GroupId = (unsigned char)ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

		/* Parse room name field */
        i = FindCmdRequest(FText, CmdDbGroupName);
	    if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if ( !FText ) break;
		strncpy((char*)&NewGroupPtr->GroupName,
			(const char*)FText, MAX_LEN_GROUP_BASE_NAME);
		FText = FStrt;
		strcpy(FText,StrPar);

		NewGroupPtr->ObjPtr = AddStructListObj(&GroupList, NewGroupPtr);
	    isCompleate = true;
		break;
	}
	if (!isCompleate)
	{
		if (NewGroupPtr) FreeMemory(NewGroupPtr);
	}
	if (FStrt) FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
void GroupDbSave()
{
	FILE            *HandleBD;
	char            *PathFilePtr = NULL;
	char            *CmdLinePtr = NULL;
    ObjListTask	    *SelObjPtr = NULL;
	GROUP_INFO_TYPE      *SelGroupRecPtr = NULL;
	char            *RetCodeCwd = NULL;
	char            *ConvBufPtr = NULL;    
	char            BuLine[512];
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
    strcat(PathFilePtr, GroupDbNamePath);
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
			printf("WebServer (Group database) mutex is fialed with error: %d\r\n", GetLastError());
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
	CmdLinePtr = (char*)AllocateMemory(4096*sizeof(char));
	memset(CmdLinePtr, 0, 4096*sizeof(char));
	SelObjPtr = (ObjListTask*)GetFistObjectList(&GroupList);
	while(SelObjPtr)
	{
	    SelGroupRecPtr = (GROUP_INFO_TYPE*)SelObjPtr->UsedTask;
		CmdLinePtr[0] = 0;
		sprintf(CmdLinePtr, "%s(%s=\"%d\" ", 
			TablGroupParsDb[0], CmdDbGroupId, SelGroupRecPtr->GroupId);

		sprintf(BuLine, "%s=\"", CmdDbGroupName);
		strcat(CmdLinePtr, BuLine);
		strcat(CmdLinePtr, (const char*)&SelGroupRecPtr->GroupName[0]);
		strcat(CmdLinePtr, "\")\r\n");
        
		fwrite(CmdLinePtr, strlen(CmdLinePtr), 1, HandleBD);
		SelObjPtr = (ObjListTask*)GetNextObjectList(&GroupList);
	}
	fclose(HandleBD);
#ifdef WIN32
    if (! ReleaseMutex(gFileMutex)) 
	{ 
        printf("Fail to release mutex (Groups database) in WEB server\r\n");
	}
#endif    
	if (CmdLinePtr) FreeMemory(CmdLinePtr);
	if (PathFilePtr) FreeMemory(PathFilePtr);
}
//---------------------------------------------------------------------------
void GroupDBClear()
{
    ObjListTask	    *SelObjPtr = NULL;
	GROUP_INFO_TYPE  *SelGroupRecPtr = NULL;
        
	SelObjPtr = (ObjListTask*)GetFistObjectList(&GroupList);
	while(SelObjPtr)
	{
	    SelGroupRecPtr = (GROUP_INFO_TYPE*)SelObjPtr->UsedTask;
		FreeMemory(SelGroupRecPtr);
		RemStructList(&GroupList, SelObjPtr);
		SelObjPtr = (ObjListTask*)GetFistObjectList(&GroupList);
	}
}
//---------------------------------------------------------------------------
GROUP_INFO_TYPE* GroupDbAddItem()
{
    unsigned char   NewGroupId = 1;
    unsigned int    AvaiGrplMask = 0;
    unsigned int    i;
	GROUP_INFO_TYPE *NewGroupRecPtr = NULL;
    ObjListTask	    *SelObjPtr = NULL;
	GROUP_INFO_TYPE *SelGroupRecPtr = NULL;

    if ((unsigned int)GroupList.Count == MAX_USER_GRP_ID) return NULL;
    
	SelObjPtr = (ObjListTask*)GetFistObjectList(&GroupList);
	while(SelObjPtr)
	{
	    SelGroupRecPtr = (GROUP_INFO_TYPE*)SelObjPtr->UsedTask;
        AvaiGrplMask |= (0x01 << (SelGroupRecPtr->GroupId-1));
		SelObjPtr = (ObjListTask*)GetNextObjectList(&GroupList);
	}
    
    for(i=0;i < MAX_USER_GRP_ID;i++)
    {
        if (!(AvaiGrplMask & (0x01 << i))) break;
        NewGroupId++;
    }
    
	NewGroupRecPtr = (GROUP_INFO_TYPE*)AllocateMemory(sizeof(GROUP_INFO_TYPE));
    if (!NewGroupRecPtr) return NULL;

	memset(&NewGroupRecPtr->GroupName, 0, MAX_LEN_GROUP_BASE_NAME);
	NewGroupRecPtr->GroupId = NewGroupId;
    NewGroupRecPtr->ObjPtr = AddStructListObj(&GroupList, NewGroupRecPtr);
	return NewGroupRecPtr;
}
//---------------------------------------------------------------------------
void GroupDbRemItem(GROUP_INFO_TYPE *RemGroupPtr)
{
    RemStructList(&GroupList, RemGroupPtr->ObjPtr);
    FreeMemory(RemGroupPtr);
    GroupDbSave();
}
//---------------------------------------------------------------------------
GROUP_INFO_TYPE* GetGroupByGroupId(unsigned char GroupId)
{
    ObjListTask	    *SelObjPtr = NULL;
	GROUP_INFO_TYPE *SelGroupRecPtr = NULL;
    GROUP_INFO_TYPE *FindGroupPtr = NULL;
            
	SelObjPtr = (ObjListTask*)GetFistObjectList(&GroupList);
	while(SelObjPtr)
	{
	    SelGroupRecPtr = (GROUP_INFO_TYPE*)SelObjPtr->UsedTask;
        if (SelGroupRecPtr->GroupId == GroupId)
        {
            FindGroupPtr = SelGroupRecPtr;
            break;
        }
		SelObjPtr = (ObjListTask*)GetNextObjectList(&GroupList);
	}
    return FindGroupPtr;
}
//---------------------------------------------------------------------------
unsigned long long int GetAllGroupAccessMask()
{
    unsigned long long int FullAccessMask = 0x01;
    ObjListTask	    *SelObjPtr = NULL;
	GROUP_INFO_TYPE *SelGroupRecPtr = NULL;
            
	SelObjPtr = (ObjListTask*)GetFistObjectList(&GroupList);
	while(SelObjPtr)
	{
	    SelGroupRecPtr = (GROUP_INFO_TYPE*)SelObjPtr->UsedTask;
        FullAccessMask |= (unsigned long long int)(1 <<  (unsigned long long int)SelGroupRecPtr->GroupId);
		SelObjPtr = (ObjListTask*)GetNextObjectList(&GroupList);
	}
    return FullAccessMask;
}
//---------------------------------------------------------------------------
