# if ! defined( ThrWebServerH )
#	define ThrWebServerH /* only include me once */

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

#ifndef BaseWebServerH
#include "BaseWebServer.h"
#endif

#ifndef HtmlConstDataH
#include "HtmlConstData.h"
#endif

#ifndef ThrConnWebH
#include "ThrConnWeb.h"
#endif

#ifndef InterfaceH
#include "Interface.h"
#endif

#ifndef SysSwVersionH
#include "SysSwVersion.h"
#endif

#ifndef SampleCustomConfigDataBaseH
#include "SampleCustomConfigDataBase.h"
#endif

#define LATEST_USER_DB_VERSION     0x10
#define START_SIMULT_REM_CONN_THR  5
#define CREATE_SIMULT_REM_CON_THR  20

#define BASE_SENDWEB_NOTIFY_MESS WM_USER+500
#define USER_SESSION_ACTIVITY_TMR_ID 1
#define INTER_MAIL_DELAY_TMR_ID      2
#define STATS_CHECK_TMR_ID           3
#define SYS_TIMESTAMP_TMR_ID         4

#define CONFIRM_KEY_LIFE_TIME   300000
#define MAX_LOAD_CMD_FILE_SIZE  2048*1024

#define GEC_GENERAL     1
#define GEC_MAIL        2
#define GEC_SHOP        3
#define GEC_BANNER      4
#define GEC_SMS         5
#define MIN_GEC_TYPE_ID GEC_GENERAL
#define MAX_GEC_TYPE_ID GEC_SMS

#define SERV_CFG_CMD_SAVE_PAGE   1
#define SERV_CFG_FILE_HASH_UPD   2
#define SERV_CFG_CMD_CHG_PAGE    3
#define SERV_CFG_CMD_ADD_BANNER  4
#define SERV_CFG_CMD_SEL_EDIT    5
#define SERV_CFG_CMD_REM_BANNER  6
#define SERV_CFG_CMD_ADD_PHONE   7
#define SERV_CFG_CMD_SEL_PHED    8
#define SERV_CFG_CMD_CHG_SMS_NUM 9
#define SERV_CFG_CMD_REM_SMS_NUM 10
#define SERV_CFG_CUSTOM_UPD      11
#define SERV_OFFER_SALES_GEN     12
#define WEB_SERV_SHUTDOWN_REQ    13
#define MIN_CFG_CMD_ID           SERV_CFG_CMD_SAVE_PAGE
#define MAX_CFG_CMD_ID           WEB_SERV_SHUTDOWN_REQ

/* Customised user acess types */
#define UAT_SYSSHOW 3
#define MIN_UAT_ID  UAT_ADMIN
#define MAX_UAT_ID  UAT_SYSSHOW

#define DB_GROUP_SEL_EDIT_GROUP_REQ 1
#define DB_GROUP_SAVE_GROUP_REQ     2
#define DB_GROUP_BD_RELOAD_REQ      3
#define DB_GROUP_ADD_GROUP_REQ      4
#define DB_GROUP_REM_GROUP_REQ      5
#define DB_GROUP_MIN_REQ            DB_GROUP_SEL_EDIT_GROUP_REQ
#define DB_GROUP_MAX_REQ            DB_GROUP_REM_GROUP_REQ

#define SHOW_ALL_AVAIL_GRP          255

typedef struct {
    bool         EventMailNotify;
} SAMPLE_USER_INFO;

typedef struct {
    unsigned int  UserSelGrpId;
    unsigned int  SelGroupId;
    unsigned int  StartGroupRec;
	unsigned int  GroupPerPage;
} SAMPLE_SPECIFIC_USER_SESSION;

void RusTextListLoad(PARAMWEBSERV *ParWebServPtr);
void EngTextListLoad(PARAMWEBSERV *ParWebServPtr);
void MenuTreeRefresh();
void HandleTimeoutNextMailSent(PARAMWEBSERV *ParWebServ);
void ServerConfigLoad(PARAMWEBSERV *ParServPtr);
void ServerConfigSave(PARAMWEBSERV *ParServPtr);

void NewOrderEventSmsSent(PARAMWEBSERV *ParWebServ);
void CssEnvLoad(USER_SESSION *SessionPtr);
void CssFileHtmlPageAppend(USER_SESSION *SessionPtr, char *CssFileName);
void SetCssTemlateBody(PARAMWEBSERV *ParWebServ, READWEBSOCK *ParReadWeb, USER_SESSION *SessionPtr, FILE_HASH_RECORD *FileInfoPtr, char *CssTemplateFileName);
void SetCapchaCodeRequestBody(PARAMWEBSERV *ParWebServPtr, USER_SESSION *SessionPtr);
void SetCapchaCodeRequestIntro();
bool isPrimaryGroupAccessUser(USER_INFO *UserPtr);
void HandleNewUserConnect(PARAMWEBSERV *ParWebServPtr, READWEBSOCK *ParReadWeb);

//---------------------------------------------------------------------------
#endif  /* if ! defined( ThrWebServerH ) */
