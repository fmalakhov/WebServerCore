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
extern char SecKeyId[];
extern PARAMWEBSERV *ParWebServPtr;
extern READWEBSOCK  *ParReadHttpSocketPtr;
//---------------------------------------------------------------------------
void ExistUserExit(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool         isParseDone = false;
    char*        FText = NULL;
	char*        FStrt = NULL;
	USER_INFO*   UserPtr = NULL;
	int          i, pars_read;
	unsigned int SecKeyForm;

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
		i = FindCmdRequest(FText, KeyUserNameId);
		if (i == -1) break;
        FText = ParseParForm( &FText[i] );
        if (!FText) break;
		isParseDone = true;
		break;
	}
    if (isParseDone)
	{
		UserPtr = SessionPtr->UserPtr;
		if (UserPtr)
        {
			/* An User's related action related to user's exit shoud be placed here */
        }
		SessionPtr->UserPtr = NULL;
        SessionPtr->ConfKeyGenTime = 0;
		SessionPtr->isMainPageReq = true;
		MainPageSetShopWebPage(BufAnsw, SessionPtr, HttpCmd);
		if (ParReadHttpSocketPtr->BotType == BOT_NONE)
		{
			SessionTimerReset(&ParWebServPtr->SessionManager, SessionPtr);
			SessionTimerStart(&ParWebServPtr->SessionManager, SessionPtr);
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
	FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
