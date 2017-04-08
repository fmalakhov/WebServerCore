# if ! defined( SessionH )
#	define SessionH	/* only include me once */

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

#ifndef MemoryPoolH
#include "MemoryPool.h"
#endif

#ifndef ThrReadWebUserH
#include "ThrReadWebUser.h"
#endif

#ifndef SessionKeyHashH
#include "SessionKeyHash.h"
#endif

#ifndef SessionIpHashH
#include "SessionIpHash.h"
#endif

#ifndef ThrReportMenH
#include "ThrReportMen.h"
#endif

#ifndef  UserInfoDataBaseH
#include "UserInfoDataBase.h"
#endif

#ifndef FileDataHashH
#include "FileDataHash.h"
#endif

#ifndef HtmlPageHashH
#include "HtmlPageHash.h"
#endif

#ifndef  CustomConfigDataBaseH
#include "CustomConfigDataBase.h"
#endif

#ifndef SysWebFunctionH
#include "SysWebFunction.h"
#endif

#ifndef WebServInfoH
#include "WebServInfo.h"
#endif

#ifndef BotDataBaseH
#include "BotDataBase.h"
#endif

#define INIT_USER_SESSION_BLK	100
#define SESSION_INDEX_LIST_SIZE 1000

#define	MAX_JOB_WEB_REQ			1000  // The maximum number of open user's sessions.

#define SESSION_INDEX_RANGE_MIN 10000
#define MAX_SESSION_INDEX       10000
#define SESSION_INDEX_RANGE_MAX SESSION_INDEX_RANGE_MIN + MAX_SESSION_INDEX

#define MIN_SESSION_TIMER_ID	1000
#define MAX_SESSION_TIMER_ID	6000

typedef struct {
	bool          isActive;
	bool          isMainPageReq;
    bool          isConfKeySent;
    unsigned char LanguageType;
	unsigned int  SessionId;
	unsigned int  TimerId;
	unsigned long UserIpAddr;
#ifdef WIN32
	SYSTEMTIME    LoginTime;
#else        
    struct tm     LoginTime;
#endif
	DWORD         LoginTick;
	unsigned char   SpaseType;
	unsigned short	UserScreenHeight;
	unsigned short	UserScreenWidth;
	unsigned int  ItemsPage;
	unsigned int  StartItem;
	unsigned int  SelItemId;
	unsigned int  SecureKey;
	unsigned int  SelBadIpId;
	unsigned int  SelBannerId;
	unsigned int  SelSmsPhoneId;
	unsigned int  SelConfigEditGrpId;    
	unsigned int  SearchListPage;
	unsigned int  SearchListItem;
	unsigned int  MobileScreenWidth;
	unsigned int  CapchaCode;
	unsigned char CapchaCheckRetry;
	USER_INFO*    UserPtr;
	ObjListTask*  ObjPtr;
    unsigned long ConfKeyGenTime;
    unsigned int  RecordsPerPage;
    unsigned int  StartRecord;
    unsigned int  FoundRecords;
	void*         UserSessionInfoPtr;
	POOL_RECORD_STRUCT *BasePoolPtr;
	POOL_RECORD_STRUCT *UserPoolPtr;
    unsigned char UserAuthEncode[USER_AUTH_ENCODE_PRIVATE_KEY_LEN+1];
	char          SesionIdKey[SESSION_ID_KEY_LEN+1];
    char          ConfirmKey[CONFIRM_KEY_LEN+1];
    unsigned char SessionEncodeKeyList[256];
} USER_SESSION;

typedef unsigned int (*TOnGetSessionTimeoutCB)(unsigned int UserType, void *DataPtr);
typedef void (*TOnUserSessionOpenCB)(void *UserSessionPtr);
typedef void (*TOnUserSessionCloseCB)(USER_SESSION *SessionPtr, void *DataPtr);
typedef void (*TOnSessionTimerStartCB)(unsigned int TimerId, unsigned int Delay, void *DataPtr);
typedef void (*TOnSessionTimerResetCB)(unsigned int TimerId, void *DataPtr);
typedef void (*TOnSessionGrpTimersRestartCB)(unsigned int *TimerRestartReqArrayPtr, void *DataPtr);

typedef struct {
	unsigned int			SysShowUserType;
	unsigned int			SysShowHostIP;
	unsigned int			SelConfigEditGrpId;
	unsigned int			LastSetTimerId;
	unsigned int			NewSessionIndex;
	unsigned int			SessionTmrRestartList[MAX_JOB_WEB_REQ + 1];
	char                    *StartPath;
	void                    *DataPtr;
	unsigned int			*SessionIndexArrayPtr;
	void					**SessionRefPtr;
	SERVER_CUSTOM_CONFIG    *ServCustomCfgPtr;
	TOnGetSessionTimeoutCB	OnGetSessionTimeoutCB;
	TOnUserSessionOpenCB	OnUserSessionOpenCB;
	TOnUserSessionCloseCB	OnUserSessionCloseCB;
	TOnSessionTimerStartCB	OnSessionTimerStartCB;
	TOnSessionTimerResetCB	OnSessionTimerResetCB;
	TOnSessionGrpTimersRestartCB OnSessionGrpTimersRestartCB;
	POOL_RECORD_BASE		BaseSessionPool;
	POOL_RECORD_BASE		UserSessionPool;
} SESSION_MANAGER;

void SessionManagerInit(SESSION_MANAGER *SessionManagerPtr, unsigned int UserSessInfoLen);
void SessionManagerClose(SESSION_MANAGER *SessionManagerPtr);
USER_SESSION* UserSessionCreate(SESSION_MANAGER *SessionManagerPtr,
	READWEBSOCK *ParReadWeb, SESSION_IP_HASH_RECORD *SessionIpRecPtr);
void HandleSessionActivTmrExp(SESSION_MANAGER *SessionManagerPtr);
void HandleSessionTimerExp(SESSION_MANAGER *SessionManagerPtr, unsigned int TimerId);
void UserSessionDelete(SESSION_MANAGER *SessionManagerPtr, USER_SESSION *SessionPtr);
unsigned int GetSessionTimeout(SESSION_MANAGER *SessionManagerPtr, USER_SESSION *SessionPtr);
void SessionTimerStart(SESSION_MANAGER *SessionManagerPtr, USER_SESSION *SessionPtr);
void SessionTimerReset(SESSION_MANAGER *SessionManagerPtr, USER_SESSION *SessionPtr);
USER_SESSION* GetSessionBySessionId(SESSION_MANAGER *SessionManagerPtr, unsigned int ReqSessionId);

//---------------------------------------------------------------------------
#endif  /* if ! defined( SessionH ) */
