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

#include "BaseWebServer.h"
#include "TextListDataBase.h"
#include "BaseHtmlConstData.h"
#include "CommonPlatform.h"
#include "ThrReportMen.h"

extern char ServerHttpAddr[];
extern char KeySessionId[];
extern char KeyUserNameId[];
extern char PasswordId[];
extern char SecKeyId[];
extern char KeyUserId[];
extern char KeyOrderId[];
extern char KeyStatusId[];

extern char KeySectionId[];
extern char KeyItemId[];
extern char KeyFormStatusId[];
extern char KeyFormUserId[];
extern char KeyFormOrderId[];

extern READWEBSOCK *ParReadHttpSocketPtr;
extern PARAMWEBSERV *ParWebServPtr;
//---------------------------------------------------------------------------
void AdminLogManager(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
#ifdef _REPORT_LOG_WEB_
	PARUSERWEBTASK *LogReqPrt = NULL;
#endif
	bool       MsgSentResult = false;
	bool       isParseDone = false;
    char*      FText = NULL;
	char*      FStrt = NULL;
	USER_INFO* NewUserPtr = NULL;
	int        i, pars_read, SecKeyForm, UserId;

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
		if ((unsigned int)SecKeyForm != SessionPtr->SecureKey) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

        if (!SessionIdCheck(FText, SessionPtr->SessionId)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, KeyUserId);
		if (i == -1) break;
		pars_read = sscanf(&HttpCmd[i], "%d", &UserId);
		if (!pars_read) break;
		if ((unsigned int)UserId != SessionPtr->UserPtr->UserId) break;

		FText = FStrt;
		strcpy(FText,HttpCmd);
		isParseDone = true;
		break;
	}

    if (isParseDone)
	{
	#ifdef _REPORT_LOG_WEB_
	    LogReqPrt = (PARUSERWEBTASK*)AllocateMemory(sizeof(PARUSERWEBTASK));
	    LogReqPrt->RequestCommand = (char*)AllocateMemory((strlen(HttpCmd)+1)*sizeof(char));
	    LogReqPrt->WorkSocket = ParReadHttpSocketPtr->HttpSocket;
		LogReqPrt->LocalAdress = NULL;
        LogReqPrt->ServerHttpAddr[0] = 0;
		LogReqPrt->SessionId = SessionPtr->SessionId;
        SetServerHttpAddr(LogReqPrt->ServerHttpAddr);
		sprintf(LogReqPrt->PageIdentityStr, "?%s%d&%s%s&%s=%d", 
			KeyUserId, SessionPtr->UserPtr->UserId, 
			KeySessionId, SessionPtr->SesionIdKey, 
			SecKeyId, SessionPtr->SecureKey);
	    strcpy(LogReqPrt->RequestCommand, HttpCmd);
        ReportWebMgtReq(LogReqPrt);
		ParReadHttpSocketPtr->IsExtGen = true;
	#else
	    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		SetRusTextBuf(BufAnsw, SITE_RUS_ITEM_NOT_FOUND_LINE_ID);
		strcat(BufAnsw,"</font></center>\r\n");
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
	#endif
	}
	else
	{
	    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		SetRusTextBuf(BufAnsw, SITE_RUS_ITEM_NOT_FOUND_LINE_ID);
		strcat(BufAnsw,"</font></center>\r\n");
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
	}
	FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
