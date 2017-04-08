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

extern char ServerHttpAddr[];
extern char KeyUserNameId[];
extern char PasswordId[];
extern char SecKeyId[];
extern char KeyUserId[];
extern char KeyOrderId[];
extern char KeyStatusId[];
extern ListItsTask  UserSessionList;
extern unsigned int UserPreffixArray[];

extern char KeySectionId[];
extern char KeyItemId[];
extern char KeyFormStatusId[];
extern char KeyFormUserId[];
extern char KeyFormOrderId[];

void ActSessionsListPageGen(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
//---------------------------------------------------------------------------
void ActiveSessionsList(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool         isParseDone = false;
    char*        FText = NULL;
	char*        FStrt = NULL;
	USER_INFO*   NewUserPtr = NULL;
	int          i, pars_read;
	unsigned int SecKeyForm, UserId;
	ObjListTask  *SelObjPtr = NULL;

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

		if (!SessionIdCheck(FText, SessionPtr->SessionId)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, KeyUserId);
		if (i == -1) break;
		pars_read = sscanf(&HttpCmd[i], "%d", &UserId);
		if (!pars_read) break;
		if (UserId != SessionPtr->UserPtr->UserId) break;

		FText = FStrt;
		strcpy(FText,HttpCmd);
		isParseDone = true;
		break;
	}
    if (isParseDone)
	{
		ActSessionsListPageGen(BufAnsw, SessionPtr, HttpCmd);
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
void ActSessionsListPageGen(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	ObjListTask  *SelObjPtr = NULL;
	char         CmdGenBuf[512];
	USER_SESSION *SelSessionPtr = NULL;
	int          index = 1;
	unsigned int SessDurSec, SessDurMin;
	CONTACT_INFO *ContactPtr = NULL;

    AddBeginPageShopWebPage(BufAnsw, SessionPtr);

	// Form header
    strcat(BufAnsw,"<h3><center>\r\n");
    SetRusTextBuf(BufAnsw, SITE_RUS_ACTIVE_SESSIONS_LINE_ID);

    strcat(BufAnsw,"<table width=\"100%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");
    strcat(BufAnsw,"<tr align=\"left\" class=\"sectiontableheader\">\r\n");
	strcat(BufAnsw,"<th>Nn</th>\r\n");
    strcat(BufAnsw,"<th>");
	SetRusTextBuf(BufAnsw, SITE_RUS_SESSIONS_INFO_CODE_LINE_ID);
	strcat(BufAnsw,"</th>\r\n");
    strcat(BufAnsw,"<th>");
	SetRusTextBuf(BufAnsw, SITE_RUS_USERS_LIST_CLIENT_LINE_ID);
	strcat(BufAnsw,"</th>\r\n");
    strcat(BufAnsw,"<th>");
	SetRusTextBuf(BufAnsw, SITE_RUS_SESSIONS_IP_ADDR_LINE_ID);
	strcat(BufAnsw,"</th>\r\n");
	strcat(BufAnsw,"<th>");
	SetRusTextBuf(BufAnsw, SITE_RUS_SESSIONS_DURAT_LINE_ID);
	strcat(BufAnsw,"</th>\r\n");

	SelObjPtr = (ObjListTask*)GetFistObjectList(&UserSessionList);
	while(SelObjPtr)
	{
	    SelSessionPtr = (USER_SESSION*)SelObjPtr->UsedTask;
        strcat(BufAnsw,"<tr valign=\"top\" class=\"sectiontableentry1\">\r\n");

	    // Show number of order in backet
		sprintf(CmdGenBuf, "<td>%d</td>\r\n", index);
		strcat(BufAnsw, CmdGenBuf);
			
		// Show session ID in backet
		sprintf(CmdGenBuf, "<td>%d</td>\r\n", SelSessionPtr->SessionId);
		strcat(BufAnsw, CmdGenBuf);

	    // Show client full name
        strcat(BufAnsw,"<td><strong>\r\n");		
		if (SelSessionPtr->UserPtr)
		{
			switch (SelSessionPtr->UserPtr->UserType)
			{
			    case UAT_ADMIN:
				    SetRusTextBuf(BufAnsw, SITE_RUS_SERV_ADMIN_INFO_LINE_ID);
					break;

			    case UAT_SYSSHOW:
				    SetRusTextBuf(BufAnsw, SITE_RUS_ROLE_STATUS_SHOW_LINE_ID);
					break;

				default:
			        ContactPtr = &SelSessionPtr->UserPtr->Contact;
		            if (ContactPtr->UserTitleId > SITE_RUS_ORDER_REQ_TYPE_1_LINE_ID)
					{
		                SetRusTextBuf(BufAnsw, UserPreffixArray[ContactPtr->UserTitleId]);
			            strcat(BufAnsw," ");
					}
		            sprintf(CmdGenBuf, "%s %s %s", 
			            ContactPtr->LastName, ContactPtr->FirstName, ContactPtr->MiddleName);
	                strcat(BufAnsw,CmdGenBuf);
					break;
			}
		}
		else
		{
			SetRusTextBuf(BufAnsw, SITE_RUS_SESSIONS_ANON_USR_LINE_ID);
		}
		strcat(BufAnsw, "</strong></td>\r\n");

		// Show session IP address in backet
		sprintf(CmdGenBuf, "<td>%d.%d.%d.%d</td>", 
			 (int)(SelSessionPtr->UserIpAddr&0x000000ff), 
		    (int)((SelSessionPtr->UserIpAddr&0x0000ff00)>>8), 
		    (int)((SelSessionPtr->UserIpAddr&0x00ff0000)>>16),
		    (int)((SelSessionPtr->UserIpAddr&0xff000000)>>24) );
		strcat(BufAnsw,CmdGenBuf);

		// Show session duration in backet
		SessDurSec = (unsigned int)(GetTickCount() - SelSessionPtr->LoginTick)/1000;
		SessDurMin = SessDurSec/60;
		sprintf(CmdGenBuf, "<td>%02d:%02d</td>\r\n", 
			SessDurMin, SessDurSec - SessDurMin*60);
		strcat(BufAnsw, CmdGenBuf);

		strcat(BufAnsw, "</tr>\r\n");
		index++;
		SelObjPtr = (ObjListTask*)GetNextObjectList(&UserSessionList);
	}
    strcat(BufAnsw,"</table>\r\n");
	AddEndPageShopWebPage(BufAnsw, SessionPtr);
}
//---------------------------------------------------------------------------
