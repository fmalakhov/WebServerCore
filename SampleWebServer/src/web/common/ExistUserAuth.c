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
#include "vistypes.h"
#include "ThrCernel.h"
#include "SysWebFunction.h"
#include "HttpPageGen.h"

extern ListItsTask  UserInfoList;

extern char KeyUserNameId[];
extern char PasswordId[];
extern char SecKeyId[];
extern char AdminName[];
extern char AdminPassword[];
extern char KeyFormConfirmKey[];
extern bool gIsUserDbSaveNeeds;
extern PARAMWEBSERV *ParWebServPtr;
extern READWEBSOCK  *ParReadHttpSocketPtr;
extern USER_DB_INFO SampleUserDbIfo;
//---------------------------------------------------------------------------
void ExistUserAuth(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool         isParseDone = false;
    bool         isKeyCheck = false;
    char*        FText = NULL;
	char*        FStrt = NULL;
	USER_INFO*   ChkUserPtr = NULL;
	USER_INFO*   GetUserPtr = NULL;
	int          i, j, pars_read, TextLen;
	unsigned int SecKeyForm;
#ifdef _LINUX_X86_        
    struct timeb  hires_cur_time;
    struct tm     *CurrTime;
#endif

	for(;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
	    FStrt = FText;
        strcpy(FText, HttpCmd);
        
		i = FindCmdRequest(FText, SecKeyId);
		if (i == -1) break;
        FText = ParseParForm( &FText[i] );
        if (!FText) break;
	    pars_read = sscanf(FText, "%d", &SecKeyForm);
	    if (!pars_read) break;
		if (SecKeyForm != SessionPtr->SecureKey) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

        ChkUserPtr = NewUserInfoCreate(&SampleUserDbIfo);
        if (!ChkUserPtr) break;
                               
        i = FindCmdRequest(FText, "usr_auth");
		if (i == -1) break;
        FText = ParseParForm(&FText[i]);
        if (!FText) break;
        j = strlen(FText);
        if (!j || (j > (MAX_LEN_USER_INFO_USER_NAME + MAX_LEN_USER_INFO_PASSWD + 2))) break;
        if (!UserAuthDecode(SessionPtr, FText, ChkUserPtr->UserName, ChkUserPtr->Passwd)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

        if (((ParReadHttpSocketPtr->WebChanId == SECONDARY_WEB_CHAN) && 
             (ParWebServPtr->ServCustomCfg.SecondPortKeyAccess)) ||
             ((ParReadHttpSocketPtr->WebChanId == PRIMARY_WEB_CHAN) && 
             (ParWebServPtr->ServCustomCfg.PrimPortKeyAccess)))
        {
		    /* Action body parameter parse */
	        i = FindCmdRequest(FText, KeyFormConfirmKey);
		    if (i == -1) break;
	        FText = ParseParForm(&FText[i]);
            if (!FText) break;
	        TextLen = strlen(FText);
            if ((TextLen == strlen(SessionPtr->ConfirmKey)) &&
                (strcmp(SessionPtr->ConfirmKey, FText) == 0) &&
                ((GetTickCount() - SessionPtr->ConfKeyGenTime) < CONFIRM_KEY_LIFE_TIME))
            {
                isKeyCheck = true;
            }
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
        }
        else
        {
            isKeyCheck = true;
        }
		isParseDone = true;
		break;
	}
    if (isParseDone)
	{
		GetUserPtr = CheckLoginUserInfoDb(ChkUserPtr->UserName, ChkUserPtr->Passwd);
		if (isKeyCheck && GetUserPtr)
		{
			SessionPtr->UserPtr = GetUserPtr;
            ftime(&hires_cur_time);
            CurrTime = localtime(&hires_cur_time.time);
		    memcpy(&GetUserPtr->LastVisitTime, CurrTime, sizeof(struct tm));   
            gIsUserDbSaveNeeds = true;
			SessionPtr->isMainPageReq = true;
            if ((ParWebServPtr->SessionManager.SysShowUserType > 0) &&
				(GetUserPtr->UserType == ParWebServPtr->SessionManager.SysShowUserType))
            {
                MainSystemShowWebPage(BufAnsw, SessionPtr, HttpCmd);
				SessionTimerReset(&ParWebServPtr->SessionManager, SessionPtr);
			    SessionTimerStart(&ParWebServPtr->SessionManager, SessionPtr);
            }
            else
            {
		        MainPageSetShopWebPage(BufAnsw, SessionPtr, HttpCmd);
			    if (ParReadHttpSocketPtr->BotType == BOT_NONE)
			    {
				    SessionTimerReset(&ParWebServPtr->SessionManager, SessionPtr);
			        SessionTimerStart(&ParWebServPtr->SessionManager, SessionPtr);
			    }
            }
		}
		else
		{
	        AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		    strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		    SetRusTextBuf(BufAnsw, SITE_RUS_AUTH_ERROR_LINE_ID);
		    strcat(BufAnsw,"</font></center>\r\n");
		    AddEndPageShopWebPage(BufAnsw, SessionPtr);
		}
	}
	else
	{
	    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		SetRusTextBuf(BufAnsw, SITE_RUS_ITEM_NOT_FOUND_LINE_ID);
		strcat(BufAnsw,"</font></center>\r\n");
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
	}
    ConfirmKeyGen(ParReadHttpSocketPtr->DeviceType, SessionPtr->ConfirmKey);
    SessionPtr->isConfKeySent = false;
    if (ChkUserPtr)
    {
        if (ChkUserPtr->ExtUserInfoPtr) FreeMemory(ChkUserPtr->ExtUserInfoPtr);
	    FreeMemory(ChkUserPtr);
    }
	FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
