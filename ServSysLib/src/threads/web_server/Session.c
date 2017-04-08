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
#include "Session.h"
#include "TrTimeOutThread.h"

extern char KeyFormSessionId[];
extern char KeySessionId[];
extern char ThrWebServName[];
extern char GenPageDynStatusShowSessIdReq[];
extern char GenPageMain[];
extern char BaseWebDataPath[];
extern char HtmlDataPath[];

extern ListItsTask  UserSessionList;
extern ListItsTask  UserInfoList;
extern STATS_INFO   ServerStats;
extern SESSION_IP_HASH_OCTET_HOP SessionIpHashHop;
extern bool gIsUserDbSaveNeeds;
extern FILE_HASH_CHAR_HOP FileHashHop;
extern HTML_PAGE_HASH_CHAR_HOP GetHtmlPageHashHop;
extern HTML_PAGE_HASH_CHAR_HOP PostHtmlPageHashHop;

static void SessionIndexInit(SESSION_MANAGER *SessionManagerPtr);
//---------------------------------------------------------------------------
void SessionManagerInit(SESSION_MANAGER *SessionManagerPtr, unsigned int UserSessInfoLen)
{
	SessionManagerPtr->LastSetTimerId = MIN_SESSION_TIMER_ID;
	SessionManagerPtr->SysShowHostIP = 0;
	SessionManagerPtr->NewSessionIndex = 0;
	SessionManagerPtr->SysShowUserType = 0;
	SessionManagerPtr->SelConfigEditGrpId = 0;
	SessionManagerPtr->SessionTmrRestartList[0] = 0;
	CreatePool(&SessionManagerPtr->BaseSessionPool, INIT_USER_SESSION_BLK, sizeof(USER_SESSION));
	CreatePool(&SessionManagerPtr->UserSessionPool, INIT_USER_SESSION_BLK, (UserSessInfoLen+1));
	SessionManagerPtr->OnUserSessionOpenCB = NULL;
	SessionManagerPtr->OnUserSessionCloseCB = NULL;
	SessionManagerPtr->OnGetSessionTimeoutCB = NULL;
	SessionManagerPtr->OnSessionTimerResetCB = NULL;
	SessionManagerPtr->OnSessionGrpTimersRestartCB = NULL;
	SessionIndexInit(SessionManagerPtr);
}
//---------------------------------------------------------------------------
void SessionManagerClose(SESSION_MANAGER *SessionManagerPtr)
{
	DestroyPool(&SessionManagerPtr->BaseSessionPool);
	DestroyPool(&SessionManagerPtr->UserSessionPool);
	if (SessionManagerPtr->SessionIndexArrayPtr) FreeMemory(SessionManagerPtr->SessionIndexArrayPtr);
	if (SessionManagerPtr->SessionRefPtr) FreeMemory(SessionManagerPtr->SessionRefPtr);
    SessionManagerPtr->SessionIndexArrayPtr = NULL;
    SessionManagerPtr->SessionRefPtr = NULL; 
}
//---------------------------------------------------------------------------
USER_SESSION* UserSessionCreate(SESSION_MANAGER *SessionManagerPtr, READWEBSOCK *ParReadWeb, 
    SESSION_IP_HASH_RECORD *SessionIpRecPtr)
{
#ifdef WIN32
	SYSTEMTIME      CurrTime;
#else
    struct timeb    hires_cur_time;
    struct tm       *cur_time;
    int             Res;
#endif
	unsigned int    KeyStatus, i;
	bool            isAnonymEnHtml = false;
    USER_SESSION    *SessionPtr = NULL;
	char            *NameOnlyPtr = NULL;
	char            *NewCmdPtr = NULL;
	HTML_PAGE_HASH_RECORD *HtmlHashRecPtr = NULL;
	int             KeyIndex;
	USER_INFO       *SelUserPtr = NULL;
    ObjListTask	    *SelObjPtr = NULL;
	char            NewSessId[128];
	char            HtmlFileName[1024];
	/* New variables */
	POOL_RECORD_STRUCT *BaseSessBlkBufPtr = NULL;
	POOL_RECORD_STRUCT *UserSessBlkBufPtr = NULL;

	BaseSessBlkBufPtr = GetBuffer(&SessionManagerPtr->BaseSessionPool);
	if (!BaseSessBlkBufPtr)
	{
		return NULL;
	}
	SessionPtr = (USER_SESSION*)BaseSessBlkBufPtr->DataPtr;
    SessionPtr->BasePoolPtr = BaseSessBlkBufPtr;
	UserSessBlkBufPtr = GetBuffer(&SessionManagerPtr->UserSessionPool);
	if (!UserSessBlkBufPtr)
	{
		FreeBuffer(&SessionManagerPtr->BaseSessionPool, (POOL_RECORD_STRUCT*)SessionPtr->BasePoolPtr);
		return NULL;
	}
	SessionPtr->UserSessionInfoPtr = (void*)UserSessBlkBufPtr->DataPtr;
	SessionPtr->UserPoolPtr = UserSessBlkBufPtr;

	if (SessionPtr)
	{
#ifdef WIN32
		GetSystemTime(&CurrTime);
#else
        ftime(&hires_cur_time);
        cur_time = localtime(&hires_cur_time.time);
#endif                                                     
		SessionPtr->isActive = true;
		SessionPtr->UserPtr = NULL;        
 		SessionPtr->isMainPageReq = false;       
        if (SessionManagerPtr->SysShowUserType && (ParReadWeb->HttpClientIP == SessionManagerPtr->SysShowHostIP))
        {
            /* HTTP connect from trusted system view host */
	        SelObjPtr = (ObjListTask*)GetFistObjectList(&UserInfoList);
	        while(SelObjPtr)
	        {
	            SelUserPtr = (USER_INFO*)SelObjPtr->UsedTask;
                if (SelUserPtr->UserType == SessionManagerPtr->SysShowUserType)
                {
                    SessionPtr->UserPtr = SelUserPtr;                    
        #ifdef WIN32
	                GetSystemTime(&SelUserPtr->LastVisitTime);
        #else
		            memcpy(&SelUserPtr->LastVisitTime, cur_time, sizeof(struct tm));
        #endif        
                    gIsUserDbSaveNeeds = true;
                    if ((ParReadWeb->FileType == FRT_HTR_PAGE) &&
                        (FindCmdRequest(ParReadWeb->LocalFileName, GenPageDynStatusShowSessIdReq) != -1))
                    {
			            DebugLogPrint(NULL, "%s:Create session for already active trust host (Socet:%d)\r\n",
				            ThrWebServName, ParReadWeb->HttpSocket);
                        isAnonymEnHtml = true;
                    }                    
                    else
                    { 
                        SessionPtr->isMainPageReq = true; 
                    }
                    break;
                }
		        SelObjPtr = (ObjListTask*)GetNextObjectList(&UserInfoList);
	        }
        }

		SessionPtr->ItemsPage = 0;
		SessionPtr->StartItem = 0;
        SessionPtr->LanguageType = SessionManagerPtr->ServCustomCfgPtr->SessionStartLanguage;        
		SessionPtr->SelItemId = 0;
		SessionPtr->SelBadIpId = 0;
		SessionPtr->SelBannerId = 0;
		SessionPtr->SelSmsPhoneId = 0;
		SessionPtr->SearchListPage = 0;
		SessionPtr->SearchListItem = 0;
		SessionPtr->CapchaCode = 0;
		SessionPtr->CapchaCheckRetry = 0;
		SessionPtr->SelConfigEditGrpId = SessionManagerPtr->SelConfigEditGrpId;                       
        SessionPtr->ConfKeyGenTime = 0;
		SessionPtr->SpaseType = 1;
		SessionPtr->UserScreenHeight = 0;
		SessionPtr->UserScreenWidth = 0;
        SessionPtr->RecordsPerPage = 4;
        SessionPtr->StartRecord = 1;
        SessionPtr->FoundRecords = 0;
		if (SessionManagerPtr->OnUserSessionOpenCB)
			(SessionManagerPtr->OnUserSessionOpenCB)(SessionPtr->UserSessionInfoPtr);

        /* Session related public key fillout */
        SessionKeyGen(SessionPtr->SessionEncodeKeyList);

        /* For confirmation key delivery */
        SessionPtr->isConfKeySent = false;
        memset(SessionPtr->ConfirmKey, 0, CONFIRM_KEY_LEN*sizeof(char));        

		if (ParReadWeb->BotType == BOT_NONE)
			  SessionPtr->SessionId = SessionManagerPtr->SessionIndexArrayPtr[SessionManagerPtr->NewSessionIndex++];
		else  SessionPtr->SessionId = SessionManagerPtr->SessionIndexArrayPtr[SESSION_INDEX_LIST_SIZE - 2];

		GenSessionIdKey(SessionPtr->SesionIdKey, SessionPtr->SessionId);
        AddSessionKey(SessionPtr->SesionIdKey, SessionPtr->SessionId);
        UserAuthEncodeGen(SessionPtr->UserAuthEncode);

        KeyIndex = FindCmdRequest(ParReadWeb->StrCmdHTTP, KeySessionId);
		if ((!UserSessionList.Count) && (ParReadWeb->CookieSessionId > 0))
		{
			if (KeyIndex != -1)
			{
				memcpy(&ParReadWeb->StrCmdHTTP[KeyIndex], SessionPtr->SesionIdKey, SESSION_ID_KEY_LEN);
			}
			else
			{
				NewCmdPtr = (char*)AllocateMemory(strlen(ParReadWeb->StrCmdHTTP)+128);
				if (NewCmdPtr)
				{
					strcpy(NewCmdPtr, ParReadWeb->StrCmdHTTP);
					sprintf(NewSessId, "&%s=", KeyFormSessionId);
					strcat(NewCmdPtr, NewSessId);
                    strncat(NewCmdPtr, SessionPtr->SesionIdKey, SESSION_ID_KEY_LEN);
					FreeMemory(ParReadWeb->StrCmdHTTP);
                    ParReadWeb->StrCmdHTTP = NewCmdPtr;
				}
			}
		}
		else
		{
			if ((KeyIndex != -1) && (KeyStatus == 3))
			{
				/* Needs to replace old key with new in command line */
				sprintf(NewSessId, "%d", SessionPtr->SessionId);
				memcpy(&ParReadWeb->StrCmdHTTP[KeyIndex], NewSessId, 5);
			}
			else
			{
				if (KeyStatus != 2)
				{
					NewCmdPtr = (char*)AllocateMemory(strlen(ParReadWeb->StrCmdHTTP)+128);
					if (NewCmdPtr)
					{
						strcpy(NewCmdPtr, ParReadWeb->StrCmdHTTP);
						sprintf(NewSessId, "&%s=", KeyFormSessionId);
						strcat(NewCmdPtr, NewSessId);
						strncat(NewCmdPtr, SessionPtr->SesionIdKey, SESSION_ID_KEY_LEN);
						FreeMemory(ParReadWeb->StrCmdHTTP);
                        ParReadWeb->StrCmdHTTP = NewCmdPtr;
					}
				}
			}
		}

		SessionPtr->SecureKey = SecureKeyGen();
		SessionPtr->UserIpAddr = ParReadWeb->HttpClientIP;
		SessionPtr->LoginTick = GetTickCount();

		SessionManagerPtr->SessionRefPtr[SessionPtr->SessionId - SESSION_INDEX_RANGE_MIN] = (void*)SessionPtr;

	#ifdef WIN32
		GetSystemTime(&SessionPtr->LoginTime);
	#else         
        ftime(&hires_cur_time);
        cur_time = localtime(&hires_cur_time.time);
        memcpy(&SessionPtr->LoginTime, cur_time, sizeof(struct tm));
	#endif                                                     

		SessionPtr->ObjPtr = AddStructListObj(&UserSessionList, SessionPtr);

		if (SessionIpRecPtr)
		{
			/* No first session from same IP is opened */
			SessionIpRecPtr->SessionCount++;
		}
		else
		{
			/* First session is opened from this IP */
			if (!AddSessionIpHash(&SessionIpHashHop, ParReadWeb->HttpClientIP))
			{
				printf("Failed to add %08x IP address to the list of session's IPs\n", (unsigned int)ParReadWeb->HttpClientIP);
			}
		}
		if (ParReadWeb->BotType == BOT_NONE)
		{
            SessionTimerStart(SessionManagerPtr, SessionPtr);
		}
		if (SessionManagerPtr->NewSessionIndex > (SESSION_INDEX_LIST_SIZE - 4)) SessionManagerPtr->NewSessionIndex = 0;
		EventLogPrint(NULL, "User's connect: %d.%d.%d.%d; Session: %d\r\n",
#ifdef WIN32						  
			 (int)(ParReadWeb->HttpClientIP&0x000000ff), 
			(int)((ParReadWeb->HttpClientIP&0x0000ff00)>>8), 
			(int)((ParReadWeb->HttpClientIP&0x00ff0000)>>16),
			(int)((ParReadWeb->HttpClientIP&0xff000000)>>24),
			SessionPtr->SessionId);
#else
  #ifdef _SUN_BUILD_
            (int)((ParReadWeb->HttpClientIP&0xff000000)>>24),
            (int)((ParReadWeb->HttpClientIP&0x00ff0000)>>16),
            (int)((ParReadWeb->HttpClientIP&0x0000ff00)>>8), 
			 (int)(ParReadWeb->HttpClientIP&0x000000ff), 
  #else  
			 (int)(ParReadWeb->HttpClientIP&0x000000ff), 
			(int)((ParReadWeb->HttpClientIP&0x0000ff00)>>8), 
			(int)((ParReadWeb->HttpClientIP&0x00ff0000)>>16),
			(int)((ParReadWeb->HttpClientIP&0xff000000)>>24),             
  #endif            
			SessionPtr->SessionId);
#endif

		if (ParReadWeb->BotType == BOT_NONE)
		{
			ServerStats.IntUserSessions++;
			ServerStats.UserSessions++;
		}
		else
		{
			ServerStats.BotSessions++;
			ServerStats.IntBotSessions++;
		}

		if (UserSessionList.Count > ServerStats.SimultOpenSessions)
			ServerStats.SimultOpenSessions = UserSessionList.Count;

        /* Check for anonym user enabled request */
	    if ((ParReadWeb->ReqestType == HRT_GET) || (ParReadWeb->ReqestType == HTR_HEAD))
		{
			if (ParReadWeb->NoPwdLocalNameShift > MAX_LEN_HTTP_REQ_FILE_NAME)
			{
				printf("NoPwdLocalNameShift is corrupted (%u)\n", ParReadWeb->NoPwdLocalNameShift);
				NameOnlyPtr = NULL;
			}
			else
			{
				NameOnlyPtr = &ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift];
				i = strlen(NameOnlyPtr);
				if (i > 0)
				{
					switch(ParReadWeb->FileType)
					{
						case FRT_HTML_PAGE:
							HtmlHashRecPtr = FindHtmlPageHash(&GetHtmlPageHashHop, NameOnlyPtr, 0x01);
							if (HtmlHashRecPtr)
							{
								/* Requested page is available for anonym user's */
								isAnonymEnHtml = true;
							}
							else
							{
								strcpy(HtmlFileName, HtmlDataPath);
								strcat(HtmlFileName, NameOnlyPtr);
								if (FindFileHash(&FileHashHop, BASE_HASH_LIST, HtmlFileName)) isAnonymEnHtml = true;
							}
							break;

						case FRT_TXT_DATA:
							if (FindCmdRequest(NameOnlyPtr, ".well-known/acme-challenge") != -1)
								isAnonymEnHtml = true;
						default:
							strcpy(HtmlFileName, BaseWebDataPath);
							strcat(HtmlFileName, NameOnlyPtr);
							if (FindFileHash(&FileHashHop, BASE_HASH_LIST, HtmlFileName)) isAnonymEnHtml = true;
							break;
					}
				}
				else
				{
					NameOnlyPtr = NULL;
				}
			}
		}
		else if ((ParReadWeb->ReqestType == HTR_POST) && 
				 (ParReadWeb->FileType == FRT_HTML_PAGE))
		{
			NameOnlyPtr = &ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift];
			i = strlen(NameOnlyPtr);
			if (i > 0)
			{
		        HtmlHashRecPtr = FindHtmlPageHash(&PostHtmlPageHashHop,
					&ParReadWeb->LocalFileName[ParReadWeb->NoPwdLocalNameShift], 0x01);
				if (HtmlHashRecPtr) isAnonymEnHtml = true;
			}
			else
			{
				NameOnlyPtr = NULL;
			}
		}

        if (!isAnonymEnHtml)
		{
            strcpy(ParReadWeb->LocalFileName, SessionManagerPtr->StartPath);
            strcat(ParReadWeb->LocalFileName, BaseWebDataPath);
			ParReadWeb->NoPwdLocalNameShift = strlen(ParReadWeb->LocalFileName);
			strcat(ParReadWeb->LocalFileName, GenPageMain);
			if (ParReadWeb->StrCmdHTTP) FreeMemory(ParReadWeb->StrCmdHTTP);
			ParReadWeb->StrCmdHTTP = (char*)AllocateMemory((strlen(GenPageMain)+16)*sizeof(char));
			ParReadWeb->StrCmdHTTP[0] = 0;
			strcpy(ParReadWeb->StrCmdHTTP, "/");
			strcat(ParReadWeb->StrCmdHTTP, GenPageMain);
			ParReadWeb->FileType = FRT_HTML_PAGE;
            ParReadWeb->ReqestType = HRT_GET;
		}
	}
    return SessionPtr;
}
//---------------------------------------------------------------------------
void HandleSessionActivTmrExp(SESSION_MANAGER *SessionManagerPtr)
{
	ObjListTask  *SelObjPtr;
	USER_SESSION *SessionPtr;
	unsigned int index, RepCnt;
    unsigned char UserType = -1;  
    char         RecordStr[24];
    char         ActiveStr[512];

	DebugLogPrint(NULL, "%s: USER_SESSION_ACTIVITY_TMR_ID timer is expired\r\n", ThrWebServName);
	index = 0;
    RepCnt = 0;
    strcpy(ActiveStr, "Active sessions list: ");   
	SessionManagerPtr->SessionTmrRestartList[0] = 0;
	SelObjPtr = (ObjListTask*)GetFistObjectList(&UserSessionList);
	while(SelObjPtr)
	{
	    SessionPtr = (USER_SESSION*)SelObjPtr->UsedTask;
		if (SessionPtr->isActive)
		{
		    SessionManagerPtr->SessionTmrRestartList[index+1] = SessionPtr->TimerId;
            if (RepCnt < 20)
            {
	            if (SessionPtr->UserPtr) UserType = SessionPtr->UserPtr->UserType;
                sprintf(RecordStr, "%d, %d, %d;", SessionPtr->SessionId, UserType, SessionPtr->TimerId);
                strcat(ActiveStr, RecordStr);
                RepCnt++;
            }
			index++;
			SessionPtr->isActive = false;
		}
		SelObjPtr = (ObjListTask*)GetNextObjectList(&UserSessionList);
	}
	if (index > 0)
	{
		SessionManagerPtr->SessionTmrRestartList[0] = index;
        DebugLogPrint(NULL, "%s: %s\r\n", ThrWebServName, ActiveStr);
		if (SessionManagerPtr->OnSessionGrpTimersRestartCB)
			(SessionManagerPtr->OnSessionGrpTimersRestartCB)(&SessionManagerPtr->SessionTmrRestartList[0], SessionManagerPtr->DataPtr);
	}
}
//---------------------------------------------------------------------------
void HandleSessionTimerExp(SESSION_MANAGER *SessionManagerPtr, unsigned int TimerId)
{
	ObjListTask  *SelObjPtr = NULL;
	USER_SESSION *SessionPtr = NULL;

    DebugLogPrint(NULL, "%s: SESSION_END timer is expired (TimerId: %d)\r\n", 
            ThrWebServName, TimerId);
	SelObjPtr = (ObjListTask*)GetFistObjectList(&UserSessionList);
	while(SelObjPtr)
	{
	    SessionPtr = (USER_SESSION*)SelObjPtr->UsedTask;
		if (SessionPtr->TimerId == TimerId)
		{
			UserSessionDelete(SessionManagerPtr, SessionPtr);
			break;
		}
	    SelObjPtr = (ObjListTask*)GetNextObjectList(&UserSessionList);
    }
}
//---------------------------------------------------------------------------
void UserSessionDelete(SESSION_MANAGER *SessionManagerPtr, USER_SESSION *SessionPtr)
{
	USER_INFO    *UserPtr = NULL;
    ObjListTask	 *SelObjPtr = NULL;
	SESSION_IP_HASH_RECORD *SessionIpRecPtr = NULL;
#ifdef WIN32
	SYSTEMTIME   CurrTime;
#else
    struct timeb hires_cur_time;
    struct tm    *cur_time;
#endif

	if (!SessionPtr) return;

#ifdef WIN32
	GetSystemTime(&CurrTime);
    EventLogPrint(NULL, "User's disconnect: %d.%d.%d.%d; SessionId: %d\r\n",
	    (int)(SessionPtr->UserIpAddr&0x000000ff), 
		(int)((SessionPtr->UserIpAddr&0x0000ff00)>>8), 
	    (int)((SessionPtr->UserIpAddr&0x00ff0000)>>16),
	    (int)((SessionPtr->UserIpAddr&0xff000000)>>24),
	    SessionPtr->SessionId);
#else          
    ftime(&hires_cur_time);
    cur_time = localtime(&hires_cur_time.time);
    EventLogPrint(NULL, "User's disconnect: %d.%d.%d.%d; SessionId: %d\r\n",                        
  #ifdef _SUN_BUILD_ 
        (int)((SessionPtr->UserIpAddr&0xff000000)>>24),
        (int)((SessionPtr->UserIpAddr&0x00ff0000)>>16),
        (int)((SessionPtr->UserIpAddr&0x0000ff00)>>8),
	    (int)(SessionPtr->UserIpAddr&0x000000ff), 
	    SessionPtr->SessionId);
  #else
	    (int)(SessionPtr->UserIpAddr&0x000000ff), 
		(int)((SessionPtr->UserIpAddr&0x0000ff00)>>8), 
	    (int)((SessionPtr->UserIpAddr&0x00ff0000)>>16),
	    (int)((SessionPtr->UserIpAddr&0xff000000)>>24),
	    SessionPtr->SessionId);
  #endif
#endif
    SessionIpRecPtr = FindSessionIpHash(&SessionIpHashHop, SessionPtr->UserIpAddr);
	if (SessionIpRecPtr)
	{
        SessionIpRecPtr->SessionCount--;
		if (!SessionIpRecPtr->SessionCount)
		{
			if (!RemSessionIpHash(&SessionIpHashHop, SessionPtr->UserIpAddr))
			{
				printf("Failed to remove Session IP hash record for %08x IP\n", (unsigned int)SessionPtr->UserIpAddr);
			}
		}
	}
	SessionManagerPtr->SessionRefPtr[SessionPtr->SessionId - SESSION_INDEX_RANGE_MIN] = NULL;
	RemStructList(&UserSessionList, SessionPtr->ObjPtr);
	RemSessionKey(SessionPtr->SesionIdKey);

	if (SessionManagerPtr->OnUserSessionCloseCB) 
		(SessionManagerPtr->OnUserSessionCloseCB)(SessionPtr, SessionManagerPtr->DataPtr);

	FreeBuffer(&SessionManagerPtr->BaseSessionPool, (POOL_RECORD_STRUCT*)SessionPtr->BasePoolPtr);
	FreeBuffer(&SessionManagerPtr->UserSessionPool, (POOL_RECORD_STRUCT*)SessionPtr->UserPoolPtr);
}
//---------------------------------------------------------------------------
static void SessionIndexInit(SESSION_MANAGER *SessionManagerPtr)
{
	unsigned int Index, FindId;
	unsigned int RndSessId;
	bool         isSameId = false;

	SessionManagerPtr->SessionIndexArrayPtr = NULL;
	SessionManagerPtr->SessionIndexArrayPtr = (unsigned int*)AllocateMemory((SESSION_INDEX_LIST_SIZE+1)*sizeof(unsigned int));
	SessionManagerPtr->SessionRefPtr = NULL;
	SessionManagerPtr->SessionRefPtr = (void**)AllocateMemory((MAX_SESSION_INDEX+1)*sizeof(void*));
	for (Index=0;Index < SESSION_INDEX_LIST_SIZE;Index++)
	{
		do
		{
	        RndSessId = (unsigned int)((double)(rand()%RAND_GEN_MASK) / 
		        (RAND_GEN_MAX + 1) * (SESSION_INDEX_RANGE_MAX - SESSION_INDEX_RANGE_MIN) + SESSION_INDEX_RANGE_MIN);
		    isSameId = false;
		    for (FindId = 0;FindId < Index;FindId++)
		    {
			    if (SessionManagerPtr->SessionIndexArrayPtr[FindId] == RndSessId)
			    {
				    isSameId = true;
				    break;
			    }
		    }
		} while(isSameId);
		SessionManagerPtr->SessionIndexArrayPtr[Index] = RndSessId;
	}
	for (Index=0;Index < MAX_SESSION_INDEX;Index++)
	{
		SessionManagerPtr->SessionRefPtr[Index] = NULL;
	}
}
//---------------------------------------------------------------------------
unsigned int GetSessionTimeout(SESSION_MANAGER *SessionManagerPtr, USER_SESSION *SessionPtr)
{
	unsigned int Delay;

	Delay = SessionManagerPtr->ServCustomCfgPtr->AnonymSessionTimeout;
	if (SessionPtr->UserPtr)
	{
		switch(SessionPtr->UserPtr->UserType)
		{
	        case UAT_ADMIN:
				Delay = SessionManagerPtr->ServCustomCfgPtr->AdminSessionTimeout;
				break;

			case UAT_GUEST:
				Delay = SessionManagerPtr->ServCustomCfgPtr->UserSessionTimeout;
				break;

			default:
				if (SessionManagerPtr->OnGetSessionTimeoutCB) 
					Delay = (SessionManagerPtr->OnGetSessionTimeoutCB)(SessionPtr->UserPtr->UserType, SessionManagerPtr->DataPtr);
				break;
		}
	}
	return Delay;
}
//---------------------------------------------------------------------------
void SessionTimerStart(SESSION_MANAGER *SessionManagerPtr, USER_SESSION *SessionPtr)
{
    unsigned int Delay;

	SessionManagerPtr->LastSetTimerId++;
	Delay = GetSessionTimeout(SessionManagerPtr, SessionPtr);
    Delay += 60; // Timer restart duration is added
	if (SessionManagerPtr->OnSessionTimerStartCB) 
		(SessionManagerPtr->OnSessionTimerStartCB)(SessionManagerPtr->LastSetTimerId, 
			Delay, SessionManagerPtr->DataPtr);
	SessionPtr->TimerId = SessionManagerPtr->LastSetTimerId;
	if (SessionManagerPtr->LastSetTimerId > MAX_SESSION_TIMER_ID)
		SessionManagerPtr->LastSetTimerId = MIN_SESSION_TIMER_ID;
}
//---------------------------------------------------------------------------
void SessionTimerReset(SESSION_MANAGER *SessionManagerPtr, USER_SESSION *SessionPtr)
{
	if (SessionManagerPtr->OnSessionTimerResetCB) 
		(SessionManagerPtr->OnSessionTimerResetCB)(SessionPtr->TimerId, SessionManagerPtr->DataPtr);
}
//---------------------------------------------------------------------------
USER_SESSION* GetSessionBySessionId(SESSION_MANAGER *SessionManagerPtr, unsigned int ReqSessionId)
{
	if ((ReqSessionId < SESSION_INDEX_RANGE_MIN) || (ReqSessionId > SESSION_INDEX_RANGE_MAX)) return NULL;
	return (USER_SESSION*)(SessionManagerPtr->SessionRefPtr[ReqSessionId - SESSION_INDEX_RANGE_MIN]);
}
//---------------------------------------------------------------------------
