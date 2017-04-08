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
#include "ForumDataBase.h"

extern bool gIsForumDbSaveNeeds;
extern ListItsTask  UserInfoList;

unsigned int NewForumIndex = 1;
unsigned int NewTopicIndex = 1;
unsigned int NewMessageIndex = 1;

unsigned int ForumCount = 0;
unsigned int TopicCount = 0;
unsigned int ForumMessageCount = 0;
unsigned int ForumUserCount = 0;

ListItsTask  ForumList;
ListItsTask  MsgPublishList;
FORUN_TOPIC_INFO    **TopicListPtr = NULL;
FORUM_MESSAGE_INFO  **MessageListPtr = NULL;
USER_INFO           **ForumUserListPtr = NULL;

extern char ForumDbNamePath[];

#ifdef WIN32
extern HANDLE gFileMutex;
#endif

#define MAX_LEN_FORUM_BASE_LINE 8096

/* List of accepted commands for shop database.*/
	char	*TablForumParsDb[] = {
        "DbForumCreate",       //The creating of new forum record;
		"DbTopicCreate",       //The creating of new topic record;
		"DbMessageCreate",     //The creating of new message record;
		"comment",			   //The identificator of not visibled comment for this line;
	};

    char CmdDbForumId[]       = "ForumId";
	char CmdDbForumName[]     = "ForumName";
	char CmdDbTopicId[]       = "TopicId";
	char CmdDbParTopicId[]    = "ParTopicId";
	char CmdDbModeratorId[]   = "ModeratorId";
    char CmdDbTopicName[]     = "TopicName";
	char CmdDbTopicDescr[]    = "TopicDescr";
	char CmdDbViewCount[]     = "ViewCount";
	char CmdDbCloseFlag[]     = "CloseFlag";
	char CmdDbAnonymChgFlag[] = "AnonymChgFlag";
	char CmdDbBotEnableFlag[] = "BotEnableFlag";
	char CmdDbUserId[]        = "UserId";
	char CmdDbUserName[]      = "UserName";
	char CmdDbSubmitDate[]    = "SubmitDate";
	char CmdDbMessageId[]     = "MessageId";
	char CmdDbFroumMessage[]  = "ForumMessage";
	char CmdDbPubStatus[]     = "PublishStatus";

static void CmdAddNewForumDb(unsigned char *CmdLinePtr);
static void CmdAddNewTopicDb(unsigned char *CmdLinePtr);
static void CmdAddNewMessageDb(unsigned char *CmdLinePtr);
static void TopicDbClear(FORUN_TOPIC_INFO *TopicPtr);
static void SaveDbTopic(FILE *HandleBD, char *CmdLinePtr, FORUN_TOPIC_INFO *SelTopicPtr);
static void SaveDbMessage(FILE *HandleBD, char *CmdLinePtr, FORUM_MESSAGE_INFO *MessagePtr);
static void ParentTopicCountUpdate(FORUN_TOPIC_INFO *TopicPtr, bool isAdd);
static FORUM_MESSAGE_INFO* LastMessageUpdateFind(FORUM_MESSAGE_INFO *LastMsgPtr, FORUN_TOPIC_INFO *TopicPtr);
//---------------------------------------------------------------------------
void ForumDBLoad()
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
	unsigned int Index;
	unsigned int read_blk;
	unsigned int DbTextLine;
	unsigned char *FileData;
	char          *RetCodeCwd = NULL;
	char DbTextItem[MAX_LEN_FORUM_BASE_LINE+1];
	char StartPath[512];
	char BdFileName[1024];

    ForumList.Count = 0;
	ForumList.CurrTask = NULL;
	ForumList.FistTask = NULL;
	MsgPublishList.Count = 0;
	MsgPublishList.CurrTask = NULL;
	MsgPublishList.FistTask = NULL;
#ifdef WIN32
	RetCodeCwd = _getcwd((char*)(&StartPath[0]),512);
#else
	RetCodeCwd = getcwd((char*)(&StartPath[0]),512);
#endif
	strcpy(BdFileName, StartPath);
	strcat(BdFileName, ForumDbNamePath);
#ifdef _LINUX_X86_
	FileHandler = fopen(BdFileName,"rb");
	if (!FileHandler) 
	{
        printf("File DB forum list (%s) dos not present\n", BdFileName);
	    return;
	}
        stat(BdFileName, &st);
        if ((st.st_mode & S_IFMT) != S_IFMT)
	{                
            SizeFile = (unsigned long)st.st_size;
        }
        else
        {
            printf("File DB forum list (%s) is not file\n", BdFileName);
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

	if (TopicListPtr == NULL) 
		TopicListPtr = (FORUN_TOPIC_INFO**)AllocateMemory((MAX_TOPICS_FORUM+1)*sizeof(FORUN_TOPIC_INFO*));
    for (Index=0;Index < MAX_TOPICS_FORUM;Index++) TopicListPtr[Index] = NULL;
	if (MessageListPtr == NULL) 
		MessageListPtr = (FORUM_MESSAGE_INFO**)AllocateMemory((MAX_MESSAGES_FORUM+1)*sizeof(FORUM_MESSAGE_INFO*));
    for (Index=0;Index < MAX_MESSAGES_FORUM;Index++) MessageListPtr[Index] = NULL;
	if (ForumUserListPtr == NULL)
		ForumUserListPtr = (USER_INFO**)AllocateMemory((MAX_REGISTER_USERS+1)*sizeof(USER_INFO*));
    for (Index=0;Index < MAX_REGISTER_USERS;Index++) ForumUserListPtr[Index] = NULL;

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
	                switch ( FindCmdArray( DbTextItem, TablForumParsDb, 4 ) )
                    {
                        case  0:
						    CmdAddNewForumDb((unsigned char*)&DbTextItem[0]);
							break;

						case  1:
							CmdAddNewTopicDb((unsigned char*)&DbTextItem[0]);
							break;

						case  2:
							CmdAddNewMessageDb((unsigned char*)&DbTextItem[0]);
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
			   if (DbTextLine < MAX_LEN_FORUM_BASE_LINE)
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
	if (ForumMessageCount) ForumUserListSort();
	printf("Forum DB load is completed (Forums: %d, Topics: %d, Messages: %d)\n", ForumCount, TopicCount, ForumMessageCount);
}
//---------------------------------------------------------------------------
static void CmdAddNewForumDb(unsigned char *CmdLinePtr)
{
	unsigned int  i;
	int           pars_read, ReadValue;
	bool          isCompleate = false;
    char          *FText;
    char          *StrPar;
	char		  *FStrt = NULL;
    FORUM_INFO    *NewForumPtr = NULL;

	for(;;)
	{
	    NewForumPtr = (FORUM_INFO*)AllocateMemory(sizeof(FORUM_INFO));
		if (!NewForumPtr) break;
		memset(NewForumPtr, 0, sizeof(FORUM_INFO));

        StrPar = GetZoneParFunction(CmdLinePtr);
        if ( !StrPar ) break;
        FText = (char*)AllocateMemory( strlen(StrPar)+4 );
		if (!FText) break;
	    FStrt = FText;
        strcpy(FText,StrPar);

        /* Parse forum id field */
        i = FindCmdRequest(FText, CmdDbForumId);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewForumPtr->ForumId = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

		/* Parse forum name field */
        i = FindCmdRequest(FText, CmdDbForumName);
	    if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
		strncpy((char*)&NewForumPtr->ForumName,
			(const char*)FText, MAX_LEN_FORUM_NAME);
		FText = FStrt;
		strcpy(FText,StrPar);
  
		AddStructList( &ForumList, NewForumPtr);
		if (NewForumIndex <= NewForumPtr->ForumId) 
			NewForumIndex = NewForumPtr->ForumId + 1;
		ForumCount++;
	    isCompleate = true;
		break;
	}
	if (!isCompleate)
	{
		if (NewForumPtr) FreeMemory(NewForumPtr);
	}
	if (FStrt) FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
static void CmdAddNewTopicDb(unsigned char *CmdLinePtr)
{
	unsigned int  i;
	int           pars_read, ReadValue;
	bool          isCompleate = false;
    char          *FText;
    char          *StrPar;
	char		  *FStrt = NULL;
    FORUN_TOPIC_INFO    *NewTopicPtr = NULL;
	FORUN_TOPIC_INFO    *ParentTopicPtr = NULL;
	FORUM_INFO    *SelForumPtr = NULL;

	for(;;)
	{
	    NewTopicPtr = (FORUN_TOPIC_INFO*)AllocateMemory(sizeof(FORUN_TOPIC_INFO));
		if (!NewTopicPtr) break;
		memset(NewTopicPtr, 0, sizeof(FORUN_TOPIC_INFO));

        StrPar = GetZoneParFunction(CmdLinePtr);
        if ( !StrPar ) break;
        FText = (char*)AllocateMemory( strlen(StrPar)+4 );
		if (!FText) break;
	    FStrt = FText;
        strcpy(FText,StrPar);

        /* Parse forum id field */
        i = FindCmdRequest(FText, CmdDbForumId);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewTopicPtr->ForumId = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

        /* Parse topic id field */
        i = FindCmdRequest(FText, CmdDbTopicId);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewTopicPtr->TopicId = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

		/* Parse topic name field */
        i = FindCmdRequest(FText, CmdDbTopicName);
	    if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
		strncpy((char*)&NewTopicPtr->TopicName,
			(const char*)FText, MAX_LEN_TOPIC_NAME);
		FText = FStrt;
		strcpy(FText,StrPar);

		/* Parse topic description field */
        i = FindCmdRequest(FText, CmdDbTopicDescr);
	    if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if (!FText) break;
		strncpy((char*)&NewTopicPtr->TopicDescr,
			(const char*)FText, MAX_LEN_TOPIC_DESCR);
		FText = FStrt;
		strcpy(FText,StrPar);

        /* Parse parent topic id field */
        i = FindCmdRequest(FText, CmdDbParTopicId);
        if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewTopicPtr->ParentTopicId = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

        /* Parse moderator id field */
        i = FindCmdRequest(FText, CmdDbModeratorId);
        if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewTopicPtr->ModeratorId = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

        /* Parse closed topic flag field */
        i = FindCmdRequest(FText, CmdDbCloseFlag);
        if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewTopicPtr->isClosed = (bool)ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

        /* Parse anonym modify flag field */
        i = FindCmdRequest(FText, CmdDbAnonymChgFlag);
        if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewTopicPtr->isAnonymModify = (bool)ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

        /* Parse anonym modify flag field */
        i = FindCmdRequest(FText, CmdDbBotEnableFlag);
        if (i != -1)
		{
            FText = ParseParFunction(&FText[i]);
            if ( !FText ) break;
	        pars_read = sscanf(FText, "%d", &ReadValue);
	        if (!pars_read) break;
		    NewTopicPtr->isBotVisible = (bool)ReadValue;
	        FText = FStrt;
            strcpy(FText, StrPar);
		}
		else
		{
			NewTopicPtr->isBotVisible = false;
		}

        /* Parse view count field */
        i = FindCmdRequest(FText, CmdDbViewCount);
        if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewTopicPtr->ViewCount = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

		SelForumPtr = GetForumByForumId(NewTopicPtr->ForumId);
		if (!SelForumPtr) break;

		if (!NewTopicPtr->ParentTopicId)
		{
            AddStructList(&SelForumPtr->RootTopicList, NewTopicPtr);
		}
		else
		{
            ParentTopicPtr = GetTopicByTopicId(NewTopicPtr->ParentTopicId);
			if (!ParentTopicPtr) break;
            AddStructList(&ParentTopicPtr->ChaildTopicList, NewTopicPtr);
		}
		ParentTopicCountUpdate(NewTopicPtr, true);
        TopicListPtr[NewTopicPtr->TopicId] = NewTopicPtr;
		if (NewTopicIndex <= NewTopicPtr->TopicId) 
			NewTopicIndex = NewTopicPtr->TopicId + 1;
		TopicCount++;
	    isCompleate = true;
		break;
	}
	if (!isCompleate)
	{
		if (NewTopicPtr) FreeMemory(NewTopicPtr);
	}
	if (FStrt) FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
static void CmdAddNewMessageDb(unsigned char *CmdLinePtr)
{
	unsigned int  i, msglen;
	int           pars_read, ReadValue, StrLen;
	bool          isCompleate = false;
    char          *FText;
    char          *StrPar;
	char		  *FStrt = NULL;
	unsigned int  rDay, rMonth, rYear, rHour, rMinute, rSecond;
    FORUM_MESSAGE_INFO  *NewMessagePtr = NULL;
	FORUN_TOPIC_INFO    *SelTopicPtr = NULL;
	FORUM_INFO          *SelForumPtr = NULL;
	USER_INFO           *SelUserPtr  = NULL;

	for(;;)
	{
	    NewMessagePtr = (FORUM_MESSAGE_INFO*)AllocateMemory(sizeof(FORUM_MESSAGE_INFO));
		if (!NewMessagePtr) break;
		memset(NewMessagePtr, 0, sizeof(FORUM_MESSAGE_INFO));

        StrPar = GetZoneParFunction(CmdLinePtr);
        if ( !StrPar ) break;
        FText = (char*)AllocateMemory( strlen(StrPar)+4 );
		if (!FText) break;
	    FStrt = FText;
        strcpy(FText,StrPar);

        /* Parse forum id field */
        i = FindCmdRequest(FText, CmdDbForumId);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewMessagePtr->ForumId = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

        /* Parse topic id field */
        i = FindCmdRequest(FText, CmdDbTopicId);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewMessagePtr->TopicId = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

        /* Parse message id field */
        i = FindCmdRequest(FText, CmdDbMessageId);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewMessagePtr->MessageId = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

        /* Parse publish status field */
        i = FindCmdRequest(FText, CmdDbPubStatus);
        if (i != -1)
		{      
            FText = ParseParFunction( &FText[i] );
            if ( !FText ) break;           
	        pars_read = sscanf(FText, "%d", &ReadValue);
	        if (!pars_read) break;      
			if ((ReadValue < MIN_MPS_VALUE)|| (ReadValue > MAX_MPS_VALUE)) break;
		    NewMessagePtr->PublishStatus = ReadValue;
	        FText = FStrt;
            strcpy(FText, StrPar);
		}
		else
		{
            NewMessagePtr->PublishStatus = MPS_WAIT_APPROVAL;
			gIsForumDbSaveNeeds = true;
		}

        /* Parse user id field */
        i = FindCmdRequest(FText, CmdDbUserId);
        if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if ( !FText ) break;
	    pars_read = sscanf(FText, "%d", &ReadValue);
	    if (!pars_read) break;
		NewMessagePtr->UserId = ReadValue;
	    FText = FStrt;
        strcpy(FText, StrPar);

		if (NewMessagePtr->UserId > 0)
		{
		    SelUserPtr = GetUserInfoById(NewMessagePtr->UserId);
			if (!SelUserPtr) break;
		}
		else
		{
		    /* Parse user name field */
            i = FindCmdRequest(FText, CmdDbUserName);
	        if (i == -1) break;
            FText = ParseParFunction( &FText[i] );
            if ( !FText ) break;
		    strncpy((char*)&NewMessagePtr->UserName,
			    (const char*)FText, MAX_LEN_USER_NAME);
		    FText = FStrt;
		    strcpy(FText,StrPar);
		}

		/* Parse submit date field */
        i = FindCmdRequest(FText, CmdDbSubmitDate);
		if (i == -1) break;
        FText = ParseParFunction(&FText[i]);
        if ( !FText ) break;
		pars_read = sscanf(FText, "%d.%d.%d %d:%d:%d", 
			&rDay, &rMonth, &rYear, &rHour, &rMinute, &rSecond);
	    if (pars_read != 6) break;
#ifdef WIN32
		NewMessagePtr->SubmitDate.wDay = rDay;
		NewMessagePtr->SubmitDate.wMonth = rMonth;
		NewMessagePtr->SubmitDate.wYear = rYear;
		NewMessagePtr->SubmitDate.wHour = rHour;
	    NewMessagePtr->SubmitDate.wMinute = rMinute;
		NewMessagePtr->SubmitDate.wSecond = rSecond;
		NewMessagePtr->SubmitDate.wMilliseconds = 0;
#else 
		NewMessagePtr->SubmitDate.tm_mday = rDay;
		NewMessagePtr->SubmitDate.tm_mon = rMonth - 1;
		NewMessagePtr->SubmitDate.tm_year = rYear - 1900;
		NewMessagePtr->SubmitDate.tm_hour = rHour;
	    NewMessagePtr->SubmitDate.tm_min = rMinute;
		NewMessagePtr->SubmitDate.tm_sec = rSecond;
#endif
	    FText = FStrt;
        strcpy(FText,StrPar);

		/* Parse user message field */
        i = FindCmdRequest(FText, CmdDbFroumMessage);
	    if (i == -1) break;
        FText = ParseParFunction( &FText[i] );
        if (!FText) break;
		StrLen = strlen(FText);
		if (!StrLen) break;
		if (StrLen < MAX_FORUM_MESSAGE_LEN) msglen = StrLen;
		else                                msglen = MAX_FORUM_MESSAGE_LEN;
        NewMessagePtr->Message = (char*)AllocateMemory((msglen+2)*sizeof(char));
		NewMessagePtr->Message[msglen] = 0;
		if (!NewMessagePtr->Message) break;
		NewMessagePtr->Message[0] = 0;
		TextAreaConvertBase(FText, (char*)NewMessagePtr->Message, msglen);
		FText = FStrt;
		strcpy(FText,StrPar);

		SelForumPtr = GetForumByForumId(NewMessagePtr->ForumId);
		if (!SelForumPtr) break;

		SelTopicPtr = GetTopicByTopicId(NewMessagePtr->TopicId);
		if (!SelTopicPtr) break;

        NewMessagePtr->BaseObjPtr = AddStructListObj(&SelTopicPtr->MessageList, NewMessagePtr);
		MessageCountInc(SelTopicPtr);
		LastMessageUpdate(NewMessagePtr, SelTopicPtr);
		if (SelUserPtr)  NewMessagePtr->UserListObjPtr = AddStructListObj(&SelUserPtr->MessageList, NewMessagePtr);
        MessageListPtr[NewMessagePtr->MessageId] = NewMessagePtr;
		if (NewMessageIndex <= NewMessagePtr->MessageId) 
			NewMessageIndex = NewMessagePtr->MessageId + 1;

		if (NewMessagePtr->PublishStatus == MPS_WAIT_APPROVAL)
			NewMessagePtr->NewMsgObjPtr = AddStructListObj(&MsgPublishList, NewMessagePtr);
		ForumMessageCount++;
	    isCompleate = true;
		break;
	}
	if (!isCompleate)
	{
		if (NewMessagePtr)
		{
			if (NewMessagePtr->Message) FreeMemory(NewMessagePtr->Message);
			FreeMemory(NewMessagePtr);
		}
	}
	if (FStrt) FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
void ForumDbSave()
{
	FILE            *HandleBD;
	char            *PathFilePtr = NULL;
	char            *CmdLinePtr = NULL;
    ObjListTask	    *SelObjPtr = NULL;
	ObjListTask	    *SelTopicObjPtr = NULL;
	char            *RetCodeCwd = NULL;
	FORUM_INFO      *SelForumRecPtr = NULL;
	FORUN_TOPIC_INFO    *SelTopicPtr = NULL;
	char            BuLine[64];
    DWORD           WaitResult;
#ifdef WIN32
	bool            isFileOperReady = false;
#endif

	PathFilePtr = (char*)AllocateMemory(1024*sizeof(char));
	if (!PathFilePtr) return;
#ifdef WIN32
	RetCodeCwd = _getcwd(PathFilePtr, 512);
#else
	RetCodeCwd = getcwd(PathFilePtr, 512);
#endif
	strcat(PathFilePtr, ForumDbNamePath);
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
			printf("Report manager (forum database) mutex is fialed with error: %d\r\n", GetLastError());
			FreeMemory(PathFilePtr);
			break;
	}
    if (!isFileOperReady) return;
#endif
	HandleBD = fopen(PathFilePtr,"wb");
	if (!HandleBD) 
	{
		FreeMemory(PathFilePtr);
#ifdef WIN32
        if (! ReleaseMutex(gFileMutex)) 
		{ 
           printf("Fail to release mutex (forum database) in report manager task\r\n");
		}
#endif
		return;
	}
	CmdLinePtr = (char*)AllocateMemory(16384*sizeof(char));
	memset(CmdLinePtr, 0, 16384*sizeof(char));
	SelObjPtr = (ObjListTask*)GetFistObjectList(&ForumList);
	while(SelObjPtr)
	{
	    SelForumRecPtr = (FORUM_INFO*)SelObjPtr->UsedTask;
		CmdLinePtr[0] = 0;
		sprintf(CmdLinePtr, "%s(%s=\"%d\" ", 
			TablForumParsDb[0], CmdDbForumId, 
			SelForumRecPtr->ForumId);

		sprintf(BuLine, "%s=\"", CmdDbForumName);
		strcat(CmdLinePtr, BuLine);
		strcat(CmdLinePtr, (const char*)&SelForumRecPtr->ForumName[0]);
		strcat(CmdLinePtr, "\")\r\n");

		fwrite(CmdLinePtr, strlen(CmdLinePtr), 1, HandleBD);

        SelTopicObjPtr = (ObjListTask*)GetFistObjectList(&SelForumRecPtr->RootTopicList);
		while(SelTopicObjPtr)
		{
            SelTopicPtr = (FORUN_TOPIC_INFO*)SelTopicObjPtr->UsedTask;
			SaveDbTopic(HandleBD, CmdLinePtr, SelTopicPtr);
            SelTopicObjPtr = (ObjListTask*)GetNextObjectList(&SelForumRecPtr->RootTopicList);
		}

		SelObjPtr = (ObjListTask*)GetNextObjectList(&ForumList);
	}
	fclose(HandleBD);
#ifdef WIN32
    if (! ReleaseMutex(gFileMutex)) 
	{ 
        printf("Fail to release mutex (forum database) in report manager task\r\n");
	}
#endif
	if (CmdLinePtr) FreeMemory(CmdLinePtr);
	if (PathFilePtr) FreeMemory(PathFilePtr);
}
//---------------------------------------------------------------------------
void AppendForumMessage(FORUM_MESSAGE_INFO  *MessagePtr)
{
	FILE            *HandleBD;
	char            *PathFilePtr = NULL;
	char            *CmdLinePtr = NULL;
	char            *RetCodeCwd = NULL;
    DWORD           WaitResult;
#ifdef WIN32
	bool            isFileOperReady = false;
#endif

	PathFilePtr = (char*)AllocateMemory(1024*sizeof(char));
	if (!PathFilePtr) return;
#ifdef WIN32
	RetCodeCwd = _getcwd(PathFilePtr, 512);
#else
	RetCodeCwd = getcwd(PathFilePtr, 512);
#endif
	strcat(PathFilePtr, ForumDbNamePath);
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
			printf("Report manager (forum database) mutex is fialed with error: %d\r\n", GetLastError());
			FreeMemory(PathFilePtr);
			break;
	}
    if (!isFileOperReady) return;
#endif
	HandleBD = fopen(PathFilePtr,"ab");
	if (!HandleBD) 
	{
		FreeMemory(PathFilePtr);
#ifdef WIN32
        if (! ReleaseMutex(gFileMutex)) 
		{ 
           printf("Fail to release mutex (forum database) in report manager task\r\n");
		}
#endif
		return;
	}
	CmdLinePtr = (char*)AllocateMemory(16384*sizeof(char));
	memset(CmdLinePtr, 0, 16384*sizeof(char));
    SaveDbMessage(HandleBD, CmdLinePtr, MessagePtr);
	fclose(HandleBD);
#ifdef WIN32
    if (! ReleaseMutex(gFileMutex)) 
	{ 
        printf("Fail to release mutex (forum database) in report manager task\r\n");
	}
#endif
	if (CmdLinePtr) FreeMemory(CmdLinePtr);
	if (PathFilePtr) FreeMemory(PathFilePtr);
}
//---------------------------------------------------------------------------
static void SaveDbTopic(FILE *HandleBD, char *CmdLinePtr, FORUN_TOPIC_INFO *TopicPtr)
{
 	ObjListTask *SelTopicObjPtr = NULL;
	ObjListTask *SelMessageObjPtr = NULL;
	FORUN_TOPIC_INFO *SelTopicPtr = NULL;
    FORUM_MESSAGE_INFO *SelMessagePtr = NULL;
	char        BuLine[64];

    sprintf(CmdLinePtr, "%s(%s=\"%d\" %s=\"%d\" %s=\"%d\" %s=\"%d\" %s=\"%d\" %s=\"%d\" %s=\"%d\" %s=\"%d\" ",
		TablForumParsDb[1], 
		CmdDbForumId,       TopicPtr->ForumId,
		CmdDbTopicId,       TopicPtr->TopicId,
		CmdDbParTopicId,    TopicPtr->ParentTopicId,
		CmdDbModeratorId,   TopicPtr->ModeratorId,
		CmdDbViewCount,     TopicPtr->ViewCount,
		CmdDbCloseFlag,     (unsigned char)TopicPtr->isClosed,
        CmdDbAnonymChgFlag, (unsigned char)TopicPtr->isAnonymModify,
		CmdDbBotEnableFlag, (unsigned char)TopicPtr->isBotVisible);

	sprintf(BuLine, "%s=\"", CmdDbTopicName);
	strcat(CmdLinePtr, BuLine);
	strcat(CmdLinePtr, (const char*)&TopicPtr->TopicName[0]);
	sprintf(BuLine, "\" %s=\"", CmdDbTopicDescr);
	strcat(CmdLinePtr, BuLine);
	strcat(CmdLinePtr, (const char*)&TopicPtr->TopicDescr[0]);
	strcat(CmdLinePtr, "\")\r\n");

	fwrite(CmdLinePtr, strlen(CmdLinePtr), 1, HandleBD);

    SelMessageObjPtr = (ObjListTask*)GetFistObjectList(&TopicPtr->MessageList); 
	while (SelMessageObjPtr)
	{
		SelMessagePtr = (FORUM_MESSAGE_INFO*)SelMessageObjPtr->UsedTask;
		SaveDbMessage(HandleBD, CmdLinePtr, SelMessagePtr);
		SelMessageObjPtr = (ObjListTask*)GetNextObjectList(&TopicPtr->MessageList);
	}

	SelTopicObjPtr = (ObjListTask*)GetFistObjectList(&TopicPtr->ChaildTopicList); 
	while (SelTopicObjPtr)
	{
		SelTopicPtr = (FORUN_TOPIC_INFO*)SelTopicObjPtr->UsedTask;
		SaveDbTopic(HandleBD, CmdLinePtr, SelTopicPtr);
		SelTopicObjPtr = (ObjListTask*)GetNextObjectList(&TopicPtr->ChaildTopicList);
	}
}
//---------------------------------------------------------------------------
static void SaveDbMessage(FILE *HandleBD, char *CmdLinePtr, FORUM_MESSAGE_INFO *MessagePtr)
{
	char *ConvBufPtr = NULL;
	char BuLine[256];

    sprintf(CmdLinePtr, "%s(%s=\"%d\" %s=\"%d\" %s=\"%d\" %s=\"%d\" %s=\"%d\" ",
		TablForumParsDb[2], 
		CmdDbForumId,       MessagePtr->ForumId,
		CmdDbTopicId,       MessagePtr->TopicId,
		CmdDbMessageId,     MessagePtr->MessageId,
		CmdDbUserId,        MessagePtr->UserId,
		CmdDbPubStatus,     MessagePtr->PublishStatus);

	sprintf(BuLine, "%s=\"%d.%d.%d %d:%d:%d\" ",
#ifdef WIN32
		CmdDbSubmitDate, MessagePtr->SubmitDate.wDay, MessagePtr->SubmitDate.wMonth,
		MessagePtr->SubmitDate.wYear, MessagePtr->SubmitDate.wHour,
		MessagePtr->SubmitDate.wMinute, MessagePtr->SubmitDate.wSecond);
#else
		CmdDbSubmitDate, MessagePtr->SubmitDate.tm_mday, MessagePtr->SubmitDate.tm_mon+1,
		MessagePtr->SubmitDate.tm_year+1900, MessagePtr->SubmitDate.tm_hour,
		MessagePtr->SubmitDate.tm_min, MessagePtr->SubmitDate.tm_sec);
#endif
	strcat(CmdLinePtr, BuLine);
    if (!MessagePtr->UserId)
	{
        sprintf(BuLine, "%s=\"", CmdDbUserName);
        strcat(CmdLinePtr, BuLine);
	    strcat(CmdLinePtr, (const char*)&MessagePtr->UserName[0]);
        strcat(CmdLinePtr, "\" ");
	}
	sprintf(BuLine, "%s=\"", CmdDbFroumMessage);
    strcat(CmdLinePtr, BuLine);
	fwrite(CmdLinePtr, strlen(CmdLinePtr), 1, HandleBD);
	ConvBufPtr = TextAreaConvertFile(&MessagePtr->Message[0]);
	if (ConvBufPtr)
	{
	    fwrite(ConvBufPtr, strlen((const char*)ConvBufPtr), 1, HandleBD);
        FreeMemory(ConvBufPtr);
    }
	strcpy(CmdLinePtr, "\")\r\n");
	fwrite(CmdLinePtr, strlen(CmdLinePtr), 1, HandleBD);
}
//---------------------------------------------------------------------------
static void TopicDbClear(FORUN_TOPIC_INFO *TopicPtr)
{
 	ObjListTask	    *SelTopicObjPtr = NULL;
	ObjListTask	    *SelMessageObjPtr = NULL;
    FORUM_MESSAGE_INFO  *SelMessagePtr = NULL;
	FORUN_TOPIC_INFO    *SelTopicPtr = NULL;

	if (TopicPtr->ChaildTopicList.Count > 0)
	{
	    SelTopicObjPtr = (ObjListTask*)GetFistObjectList(&TopicPtr->ChaildTopicList); 
		while (SelTopicObjPtr)
		{
			SelTopicPtr = (FORUN_TOPIC_INFO*)SelTopicObjPtr->UsedTask;
			TopicDbClear(SelTopicPtr);
			RemStructList(&TopicPtr->ChaildTopicList, SelTopicObjPtr);
			SelTopicObjPtr = (ObjListTask*)GetFistObjectList(&TopicPtr->ChaildTopicList);
		}
	}

    SelMessageObjPtr = (ObjListTask*)GetFistObjectList(&TopicPtr->MessageList);
	while(SelMessageObjPtr)
	{
         SelMessagePtr = (FORUM_MESSAGE_INFO*)SelMessageObjPtr->UsedTask;
		 if (SelMessagePtr->Message) FreeMemory(SelMessagePtr->Message);
         FreeMemory(SelMessagePtr);
		 RemStructList(&TopicPtr->MessageList, SelMessageObjPtr);
         SelMessageObjPtr = (ObjListTask*)GetFistObjectList(&TopicPtr->MessageList);
	}
	FreeMemory(TopicPtr);
}
//---------------------------------------------------------------------------
void ForumDBClear()
{
    ObjListTask	    *SelObjPtr = NULL;
	ObjListTask	    *SelTopicObjPtr = NULL;
	ObjListTask	    *SelMessageObjPtr = NULL;
	FORUM_INFO      *SelForumRecPtr = NULL;
    FORUM_MESSAGE_INFO  *SelMessagePtr = NULL;
	FORUN_TOPIC_INFO    *SelTopicPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&ForumList);
	while(SelObjPtr)
	{
	    SelForumRecPtr = (FORUM_INFO*)SelObjPtr->UsedTask;
        SelTopicObjPtr = (ObjListTask*)GetFistObjectList(&SelForumRecPtr->RootTopicList);
		while(SelTopicObjPtr)
		{
			SelTopicPtr = (FORUN_TOPIC_INFO*)SelTopicObjPtr->UsedTask;
			TopicDbClear(SelTopicPtr);
            RemStructList(&SelForumRecPtr->RootTopicList, SelTopicObjPtr);
			SelTopicObjPtr = (ObjListTask*)GetFistObjectList(&SelForumRecPtr->RootTopicList);
		}
        FreeMemory(SelForumRecPtr);
		RemStructList(&ForumList, SelObjPtr);
		SelObjPtr = (ObjListTask*)GetFistObjectList(&ForumList);
	}
	SelObjPtr = (ObjListTask*)GetFistObjectList(&MsgPublishList);
	while(SelObjPtr)
	{
		RemStructList(&MsgPublishList, SelObjPtr);
		SelObjPtr = (ObjListTask*)GetFistObjectList(&MsgPublishList);
	}
	if (TopicListPtr)     FreeMemory(TopicListPtr);
	if (MessageListPtr)   FreeMemory(MessageListPtr);
	if (ForumUserListPtr) FreeMemory(ForumUserListPtr);
}
//---------------------------------------------------------------------------
FORUM_INFO* ForumDbAddForum()
{
	FORUM_INFO   *NewForumRecPtr = NULL;

	NewForumRecPtr = (FORUM_INFO*)AllocateMemory(sizeof(FORUM_INFO));
	memset(NewForumRecPtr, 0, sizeof(FORUM_INFO));
	NewForumRecPtr->ForumId = NewForumIndex++;
	AddStructList(&ForumList, NewForumRecPtr);
	return NewForumRecPtr;
}
//---------------------------------------------------------------------------
FORUM_INFO* GetForumByForumId(unsigned int ForumId)
{
	ObjListTask  *SelObjPtr = NULL;
	FORUM_INFO   *ForumPtr = NULL;
	FORUM_INFO   *FindForumPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&ForumList);
	while(SelObjPtr)
	{
	    ForumPtr = (FORUM_INFO*)SelObjPtr->UsedTask;
		if (ForumPtr->ForumId == ForumId)
		{
			FindForumPtr = ForumPtr;
			break;
		}
		SelObjPtr = (ObjListTask*)GetNextObjectList(&ForumList);
	}
	return FindForumPtr;
}
//---------------------------------------------------------------------------
FORUN_TOPIC_INFO* GetTopicByTopicId(unsigned int TopicId)
{
	if (TopicId < MAX_TOPICS_FORUM) return TopicListPtr[TopicId];
	else                            return NULL;
}
//---------------------------------------------------------------------------
FORUM_MESSAGE_INFO* GetMessageByMessageId(unsigned int MessageId)
{
	if (MessageId < MAX_MESSAGES_FORUM) return MessageListPtr[MessageId];
	else                                return NULL;
}
//---------------------------------------------------------------------------
void MessageCountInc(FORUN_TOPIC_INFO *TopicPtr)
{
	FORUN_TOPIC_INFO *ParentTopicPtr = NULL;

	if (!TopicPtr) return;
	TopicPtr->MessageCount++;
	if (TopicPtr->ParentTopicId > 0)
	{
        ParentTopicPtr = GetTopicByTopicId(TopicPtr->ParentTopicId);
        if (ParentTopicPtr) MessageCountInc(ParentTopicPtr);
	}
}
//---------------------------------------------------------------------------
void MessageCountDec(FORUN_TOPIC_INFO *TopicPtr)
{
	FORUN_TOPIC_INFO *ParentTopicPtr = NULL;

	if (!TopicPtr) return;
	if (TopicPtr->MessageCount > 0) TopicPtr->MessageCount--;
	if (TopicPtr->ParentTopicId > 0)
	{
        ParentTopicPtr = GetTopicByTopicId(TopicPtr->ParentTopicId);
        if (ParentTopicPtr) MessageCountDec(ParentTopicPtr);
	}
}
//---------------------------------------------------------------------------
void LastMessageUpdate(FORUM_MESSAGE_INFO *NewMessagePtr, FORUN_TOPIC_INFO *TopicPtr)
{
	FORUN_TOPIC_INFO *ParentTopicPtr = NULL;

	if (!TopicPtr->LastMessagePtr)
	{
        TopicPtr->LastMessagePtr = NewMessagePtr;
	}
	else
	{
        if (isTime1MoreTime2(&NewMessagePtr->SubmitDate, 
			&TopicPtr->LastMessagePtr->SubmitDate))
		{
            TopicPtr->LastMessagePtr = NewMessagePtr;
		}
	}
	if (TopicPtr->ParentTopicId > 0)
	{
        ParentTopicPtr = GetTopicByTopicId(TopicPtr->ParentTopicId);
        if (ParentTopicPtr) LastMessageUpdate(NewMessagePtr, ParentTopicPtr);
	}
}
//---------------------------------------------------------------------------
static FORUM_MESSAGE_INFO* LastMessageUpdateFind(FORUM_MESSAGE_INFO *LastMsgPtr, FORUN_TOPIC_INFO *TopicPtr)
{
	ObjListTask        *SelMsgObjPtr = NULL;
	ObjListTask        *SelTopicObjPtr = NULL;
	FORUM_MESSAGE_INFO *MessagePtr = NULL;
    FORUN_TOPIC_INFO   *ChaildTopicPtr = NULL;

    TopicPtr->LastMessagePtr = NULL;
	SelMsgObjPtr = (ObjListTask*)GetFistObjectList(&TopicPtr->MessageList);
	while(SelMsgObjPtr)
	{
		MessagePtr = (FORUM_MESSAGE_INFO*)SelMsgObjPtr->UsedTask;
		if (!LastMsgPtr) LastMsgPtr = MessagePtr;
		else
		{
            if (isTime1MoreTime2(&MessagePtr->SubmitDate, 
				&LastMsgPtr->SubmitDate))
			{
                LastMsgPtr = MessagePtr;
			}
		}
        SelMsgObjPtr = (ObjListTask*)GetNextObjectList(&TopicPtr->MessageList);
	}
	if (TopicPtr->ChaildTopicList.Count > 0)
	{
        SelTopicObjPtr = (ObjListTask*)GetFistObjectList(&TopicPtr->ChaildTopicList);
		while(SelTopicObjPtr)
		{
			ChaildTopicPtr = (FORUN_TOPIC_INFO*)SelTopicObjPtr->UsedTask;
            LastMsgPtr =  LastMessageUpdateFind(LastMsgPtr, ChaildTopicPtr);
            SelTopicObjPtr = (ObjListTask*)GetNextObjectList(&TopicPtr->ChaildTopicList);
		}
	}
	return LastMsgPtr;
}
//---------------------------------------------------------------------------
void MsgRemLastMessageUpdate(FORUM_MESSAGE_INFO *RemMessagePtr, FORUN_TOPIC_INFO *TopicPtr,
							 FORUM_MESSAGE_INFO *LastMsgPtr, FORUN_TOPIC_INFO *ChdSkipTopicPtr)
{
	FORUN_TOPIC_INFO   *ParentTopicPtr = NULL;
	ObjListTask        *SelMsgObjPtr = NULL;
	ObjListTask        *SelTopicObjPtr = NULL;
	FORUM_MESSAGE_INFO *MessagePtr = NULL;
    FORUN_TOPIC_INFO   *ChaildTopicPtr = NULL;

	if (TopicPtr->LastMessagePtr == RemMessagePtr)
	{
        TopicPtr->LastMessagePtr = NULL;
		SelMsgObjPtr = (ObjListTask*)GetFistObjectList(&TopicPtr->MessageList);
		while(SelMsgObjPtr)
		{
			MessagePtr = (FORUM_MESSAGE_INFO*)SelMsgObjPtr->UsedTask;
			if (MessagePtr != RemMessagePtr)
			{
				if (!LastMsgPtr) LastMsgPtr = MessagePtr;
				else
				{
                    if (isTime1MoreTime2(&MessagePtr->SubmitDate, 
						&LastMsgPtr->SubmitDate))
					{
                        LastMsgPtr = MessagePtr;
					}
				}
			}
            SelMsgObjPtr = (ObjListTask*)GetNextObjectList(&TopicPtr->MessageList);
		}
		if (TopicPtr->ChaildTopicList.Count > 0)
		{
            SelTopicObjPtr = (ObjListTask*)GetFistObjectList(&TopicPtr->ChaildTopicList);
			while(SelTopicObjPtr)
			{
				ChaildTopicPtr = (FORUN_TOPIC_INFO*)SelTopicObjPtr->UsedTask;
                if (ChaildTopicPtr  != ChdSkipTopicPtr)
					LastMsgPtr =  LastMessageUpdateFind(LastMsgPtr, ChaildTopicPtr);
                SelTopicObjPtr = (ObjListTask*)GetNextObjectList(&TopicPtr->ChaildTopicList);
			}
		}
        TopicPtr->LastMessagePtr = LastMsgPtr;
	}
	if (TopicPtr->ParentTopicId > 0)
	{
        ParentTopicPtr = GetTopicByTopicId(TopicPtr->ParentTopicId);
        if (ParentTopicPtr) MsgRemLastMessageUpdate(RemMessagePtr, ParentTopicPtr, LastMsgPtr, TopicPtr);
	}
}
//---------------------------------------------------------------------------
static void ParentTopicCountUpdate(FORUN_TOPIC_INFO *TopicPtr, bool isAdd)
{
	FORUN_TOPIC_INFO *ParentTopicPtr = NULL;

	if (TopicPtr->ParentTopicId > 0)
	{
        ParentTopicPtr = GetTopicByTopicId(TopicPtr->ParentTopicId);
		if (ParentTopicPtr)
		{
		    if (isAdd) ParentTopicPtr->ChaildTopicCount++;
			else
			{
				if (ParentTopicPtr->ChaildTopicCount > 0) ParentTopicPtr->ChaildTopicCount--;
			}
            if (ParentTopicPtr) ParentTopicCountUpdate(ParentTopicPtr, isAdd);
		}
	}
}
//---------------------------------------------------------------------------
FORUM_MESSAGE_INFO* ForumDbAddMessage()
{
#ifdef _LINUX_X86_        
    struct timeb  hires_cur_time;
    struct tm     *CurrTime;
#endif
	FORUM_MESSAGE_INFO  *NewMessagePtr = NULL;

	NewMessagePtr = (FORUM_MESSAGE_INFO*)AllocateMemory(sizeof(FORUM_MESSAGE_INFO));
	if (!NewMessagePtr) return NULL;
	memset(NewMessagePtr, 0, sizeof(FORUM_MESSAGE_INFO));

#ifdef WIN32
	GetSystemTime(&NewMessagePtr->SubmitDate);
#else
    ftime(&hires_cur_time);
    CurrTime = localtime(&hires_cur_time.time);
    memcpy(&NewMessagePtr->SubmitDate, CurrTime, sizeof(struct tm));
#endif
    NewMessagePtr->PublishStatus = MPS_WAIT_APPROVAL;
	NewMessagePtr->NewMsgObjPtr = AddStructListObj(&MsgPublishList, NewMessagePtr);
	return NewMessagePtr;
}
//---------------------------------------------------------------------------
FORUN_TOPIC_INFO* ForumDvAddTopic()
{
	FORUN_TOPIC_INFO *NewTopicPtr = NULL;

	NewTopicPtr = (FORUN_TOPIC_INFO*)AllocateMemory(sizeof(FORUN_TOPIC_INFO));
	if (!NewTopicPtr) return NULL;
	memset(NewTopicPtr, 0, sizeof(FORUN_TOPIC_INFO));
	NewTopicPtr->TopicId = NewTopicIndex++;
	ParentTopicCountUpdate(NewTopicPtr, true);
    TopicListPtr[NewTopicPtr->TopicId] = NewTopicPtr;
	return NewTopicPtr;
}
//---------------------------------------------------------------------------
void AppendForumTopic(FORUN_TOPIC_INFO  *TopicPtr)
{
	FILE            *HandleBD;
	char            *PathFilePtr = NULL;
	char            *CmdLinePtr = NULL;
	char            *RetCodeCwd = NULL;
    DWORD           WaitResult;
#ifdef WIN32
	bool            isFileOperReady = false;
#endif

	PathFilePtr = (char*)AllocateMemory(1024*sizeof(char));
	if (!PathFilePtr) return;
#ifdef WIN32
	RetCodeCwd = _getcwd(PathFilePtr, 512);
#else
	RetCodeCwd = getcwd(PathFilePtr, 512);
#endif
	strcat(PathFilePtr, ForumDbNamePath);
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
			printf("Report manager (forum database) mutex is fialed with error: %d\r\n", GetLastError());
			FreeMemory(PathFilePtr);
			break;
	}
    if (!isFileOperReady) return;
#endif
	HandleBD = fopen(PathFilePtr,"ab");
	if (!HandleBD) 
	{
		FreeMemory(PathFilePtr);
#ifdef WIN32
        if (! ReleaseMutex(gFileMutex)) 
		{ 
           printf("Fail to release mutex (forum database) in report manager task\r\n");
		}
#endif
		return;
	}
	CmdLinePtr = (char*)AllocateMemory(16384*sizeof(char));
	memset(CmdLinePtr, 0, 16384*sizeof(char));
	SaveDbTopic(HandleBD, CmdLinePtr, TopicPtr);
	fclose(HandleBD);
#ifdef WIN32
    if (! ReleaseMutex(gFileMutex)) 
	{ 
        printf("Fail to release mutex (forum database) in report manager task\r\n");
	}
#endif
	if (CmdLinePtr) FreeMemory(CmdLinePtr);
	if (PathFilePtr) FreeMemory(PathFilePtr);
}
//---------------------------------------------------------------------------
void DeleteForumMessage(FORUM_MESSAGE_INFO *RemMsgPtr)
{
	FORUN_TOPIC_INFO   *SelTopicPtr = NULL;
	USER_INFO          *SelUserPtr  = NULL;

    SelTopicPtr = GetTopicByTopicId(RemMsgPtr->TopicId);
	if (!SelTopicPtr) return;
	if (RemMsgPtr->UserId > 0)
	{
		SelUserPtr = GetUserInfoById(RemMsgPtr->UserId);
		if (SelUserPtr) RemStructList(&SelUserPtr->MessageList, RemMsgPtr->UserListObjPtr);
	}
	if (RemMsgPtr->NewMsgObjPtr) RemStructList(&MsgPublishList, RemMsgPtr->NewMsgObjPtr);
    RemStructList(&SelTopicPtr->MessageList, RemMsgPtr->BaseObjPtr);
	MessageListPtr[RemMsgPtr->MessageId] = NULL;
	MessageCountDec(SelTopicPtr);
	MsgRemLastMessageUpdate(RemMsgPtr, SelTopicPtr, NULL, NULL);
    FreeMemory(RemMsgPtr);
}
//---------------------------------------------------------------------------
void DeleteForumTopic(FORUN_TOPIC_INFO  *TopicPtr)
{
	ObjListTask      *SelObjPtr = NULL;
	FORUN_TOPIC_INFO *ChaildTopicPtr = NULL;
	FORUN_TOPIC_INFO *ParentTopicPtr = NULL;
    FORUM_MESSAGE_INFO *MessagePtr = NULL;
    FORUM_INFO       *SelForumPtr = NULL;

	if (TopicPtr->MessageList.Count > 0)
	{
	    SelObjPtr = (ObjListTask*)GetFistObjectList(&TopicPtr->MessageList);
	    while(SelObjPtr)
		{
            MessagePtr = (FORUM_MESSAGE_INFO*)SelObjPtr->UsedTask;
            DeleteForumMessage(MessagePtr);
            SelObjPtr = (ObjListTask*)GetFistObjectList(&TopicPtr->MessageList);
		}
	}

	if (TopicPtr->ParentTopicId > 0)
	{
		/* Delete topic from sub list */
        ParentTopicPtr = GetTopicByTopicId(TopicPtr->ParentTopicId);
		if (ParentTopicPtr)
		{
	        SelObjPtr = (ObjListTask*)GetFistObjectList(&ParentTopicPtr->ChaildTopicList);
	        while(SelObjPtr)
			{
                ChaildTopicPtr = (FORUN_TOPIC_INFO*)SelObjPtr->UsedTask;
                if (ChaildTopicPtr == TopicPtr)
				{
                    RemStructList(&ParentTopicPtr->ChaildTopicList, SelObjPtr);
		            ParentTopicCountUpdate(TopicPtr, false);
                    TopicListPtr[TopicPtr->TopicId] = NULL;
					break;
				}
                SelObjPtr = (ObjListTask*)GetNextObjectList(&ParentTopicPtr->ChaildTopicList);
			}
		}
	}
	else
	{
		/* Delete topic from root list */
        SelForumPtr = GetForumByForumId(TopicPtr->ForumId);
		if (SelForumPtr)
		{
	        SelObjPtr = (ObjListTask*)GetFistObjectList(&SelForumPtr->RootTopicList);
	        while(SelObjPtr)
			{
                ChaildTopicPtr = (FORUN_TOPIC_INFO*)SelObjPtr->UsedTask;
                if (ChaildTopicPtr == TopicPtr)
				{
                    RemStructList(&SelForumPtr->RootTopicList, SelObjPtr);
		            ParentTopicCountUpdate(TopicPtr, false);
                    TopicListPtr[TopicPtr->TopicId] = NULL;
					break;
				}
                SelObjPtr = (ObjListTask*)GetNextObjectList(&SelForumPtr->RootTopicList);
			}
		}
	}

	if (TopicPtr->ChaildTopicList.Count > 0)
	{
	    SelObjPtr = (ObjListTask*)GetFistObjectList(&TopicPtr->ChaildTopicList);
	    while(SelObjPtr)
		{
            ChaildTopicPtr = (FORUN_TOPIC_INFO*)SelObjPtr->UsedTask;
            DeleteForumTopic(ChaildTopicPtr);
            SelObjPtr = (ObjListTask*)GetFistObjectList(&TopicPtr->ChaildTopicList);
		}
	}
	FreeMemory(TopicPtr);
}
//---------------------------------------------------------------------------
void DeleteForumFrt(FORUM_INFO *RemForumPtr)
{
	ObjListTask      *SelObjPtr = NULL;
	ObjListTask      *SelTopicObjPtr = NULL;
    FORUM_INFO       *SelForumPtr = NULL;
	FORUN_TOPIC_INFO *RemTopicPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&ForumList);
	while(SelObjPtr)
	{
        SelForumPtr = (FORUM_INFO*)SelObjPtr->UsedTask;
		if (RemForumPtr == SelForumPtr)
		{
	        SelTopicObjPtr = (ObjListTask*)GetFistObjectList(&RemForumPtr->RootTopicList);
	        while(SelTopicObjPtr)
			{
                RemTopicPtr = (FORUN_TOPIC_INFO*)SelTopicObjPtr->UsedTask;
                DeleteForumTopic(RemTopicPtr);
                SelTopicObjPtr = (ObjListTask*)GetFistObjectList(&RemForumPtr->RootTopicList);
			}
			RemStructList(&ForumList, SelObjPtr);
			FreeMemory(RemForumPtr);
		    break;
		}
        SelObjPtr = (ObjListTask*)GetNextObjectList(&ForumList);
	}
}
//---------------------------------------------------------------------------
void ForumUserListSort()
{
	ObjListTask *SelObjPtr = NULL;
	USER_INFO   *UserPtr = NULL;
 	unsigned int Index, textLen, cmpIndex;
	bool         isChgDetect;

	ForumUserCount = 0;
	SelObjPtr = (ObjListTask*)GetFistObjectList(&UserInfoList);
	while(SelObjPtr)
	{
		UserPtr = (USER_INFO*)SelObjPtr->UsedTask;
		if (UserPtr->MessageList.Count) ForumUserListPtr[ForumUserCount++] = UserPtr;
		SelObjPtr = (ObjListTask*)GetNextObjectList(&UserInfoList);
	}

	if (ForumUserCount > 0)
	{
		do
		{ 			    
			isChgDetect = false;
		    for (Index=1;Index < ForumUserCount;Index++)
		    {
					if (strlen((const char*)&(ForumUserListPtr[Index]->Name)[0]) < 
						strlen((const char*)&(ForumUserListPtr[Index-1]->Name)[0]))
					{
						textLen = strlen((const char*)&(ForumUserListPtr[Index]->Name)[0]);
					}
					else
					{
						textLen = strlen((const char*)&(ForumUserListPtr[Index-1]->Name)[0]);
					}

					for (cmpIndex=0;cmpIndex < textLen;cmpIndex++)
					{
						if (ForumUserListPtr[Index]->Name[cmpIndex] <
							ForumUserListPtr[Index-1]->Name[cmpIndex])
				        {
						    UserPtr = ForumUserListPtr[Index-1];
					        ForumUserListPtr[Index-1] = ForumUserListPtr[Index];
						    ForumUserListPtr[Index] = UserPtr;
					        isChgDetect = true;
							break;
						}
						else if (ForumUserListPtr[Index]->Name[cmpIndex] > 
							     ForumUserListPtr[Index-1]->Name[cmpIndex])
						{
							break;
					    }
				    }
		    }
		}
		while(isChgDetect);
	}
}
//---------------------------------------------------------------------------
