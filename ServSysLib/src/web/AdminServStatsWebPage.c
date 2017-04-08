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

extern char ServerHttpAddr[];
extern char KeySessionId[];
extern char KeyUserNameId[];
extern char PasswordId[];
extern char SecKeyId[];
extern char KeyUserId[];
extern char KeyOrderId[];
extern char KeyStatusId[];
extern STATS_INFO ServerStats;
extern char *EndHtmlPageGenPtr;

extern char KeySectionId[];
extern char KeyItemId[];
extern char KeyFormStatusId[];
extern char KeyFormUserId[];
extern char KeyFormOrderId[];
extern unsigned int LastPageGenTime[];
extern unsigned int LastReqLoadTime[];

static void AdminServStatsPageGen(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
static void AddStatsToPrintList(unsigned int StatsTxtId, unsigned int StatsValue);
//---------------------------------------------------------------------------
void AdminServStatsWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool       isParseDone = false;
    char*      FText = NULL;
	char*      FStrt = NULL;
	USER_INFO* NewUserPtr = NULL;
	int        i, pars_read;
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
		AdminServStatsPageGen(BufAnsw, SessionPtr, HttpCmd);
	}
	else
	{
	    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];
		AddStrWebPage("<center><font size=\"3\" color=\"red\">");
		SetRusTextBuf(NULL, SITE_RUS_ITEM_NOT_FOUND_LINE_ID);
		AddStrWebPage("</font></center>\r\n");
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
	}
	FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
static void AdminServStatsPageGen(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	ObjListTask  *SelObjPtr = NULL;
	char         CmdGenBuf[64];
	USER_INFO    *UserPtr = NULL;
	int          index = 1;

    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
    EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];

	// Form header
    AddStrWebPage("<h3><center>\r\n");
    SetRusTextBuf(NULL, SITE_RUS_ADM_STATS_LINE_ID);
    AddStrWebPage("</center></h3><br>\r\n");

    AddStrWebPage("<table width=\"100%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");
    AddStrWebPage("<tr align=\"left\" class=\"sectiontableheader\">\r\n");
    AddStrWebPage("<th with=\"50%\">");
	SetRusTextBuf(NULL, SITE_RUS_STATS_NAME_LINE_ID);
	AddStrWebPage("</th>\r\n");
    AddStrWebPage("<th with=\"50%\">");
	SetRusTextBuf(NULL, SITE_RUS_STATS_VALUE_LINE_ID);
	AddStrWebPage("</th></tr>\r\n");

	// Show number of simult. read sessions
	AddStatsToPrintList(SITE_RUS_STATS_SIM_READ_LINE_ID, 
		ServerStats.SumultReadSessions);

	// Show number of simult. send sessions
	AddStatsToPrintList(SITE_RUS_STATS_SIM_SEND_LINE_ID, 
		ServerStats.SimultSendSessions);

    // Show number of simult. open sessions
	AddStatsToPrintList(SITE_RUS_STATS_SIM_SESS_LINE_ID, 
		ServerStats.SimultOpenSessions);

    // Show number of user's connections
	AddStatsToPrintList(SITE_RUS_STATS_USER_CON_LINE_ID, 
		ServerStats.UserSessions);

    // Show number of bot's connections
	AddStatsToPrintList(SITE_RUS_STATS_BOT_CON_LINE_ID, 
		ServerStats.BotSessions);

    // Show number of successfully sent mails
	AddStatsToPrintList(SITE_RUS_STATS_MAIL_OK_LINE_ID, 
		ServerStats.SuccMailDelivery);

    // Show number of failed for sent mails
	AddStatsToPrintList(SITE_RUS_STATS_MAIL_FAIL_LINE_ID, 
		ServerStats.FailMailDelivery);

    // Show number of successfully sent sms
	AddStatsToPrintList(SITE_RUS_STATS_SMS_OK_LINE_ID, 
		ServerStats.SuccSmsDelivery);

    // Show number of failed for sent sms
	AddStatsToPrintList(SITE_RUS_STATS_SMS_FAIL_LINE_ID, 
		ServerStats.FailSmsDelivery);

    // Show number of sent HTML pages
	AddStatsToPrintList(SITE_RUS_STATS_SENT_HTML_LINE_ID, 
		ServerStats.SendHtmlPages);

    // Show number of sent data files
	AddStatsToPrintList(SITE_RUS_STATS_SENT_FILE_LINE_ID, 
		ServerStats.SendFiles);

    // Show average time of HTML page generation
	AddStatsToPrintList(SITE_RUS_STATS_AVG_HTML_TIME_LINE_ID, 
		ServerStats.AvgPageGenTime);

    // Show max time of HTML page generation
	AddStatsToPrintList(SITE_RUS_STATS_MAX_HTML_TIME__LINE_ID, 
		ServerStats.MaxPageGenTime);

    // Show average time of HTML page generation
	AddStatsToPrintList(SITE_RUS_STATS_AVG_HTM_REQ_TIME_LINE_ID, 
		ServerStats.AvgReqLoadTime);

    // Show max time of HTML page generation
	AddStatsToPrintList(SITE_RUS_STATS_MAX_HTM_REQ_TIME_LINE_ID, 
		ServerStats.MaxReqLoadTime);

	AddStrWebPage("<tr valign=\"top\" class=\"sectiontableentry1\">\r\n");
	AddStrWebPage( "<td with=\"50%\"><strong>");
	SetRusTextBuf(NULL, SITE_RUS_STATS_AVG_HTML_TIME_LINE_ID);
	sprintf(CmdGenBuf, "</strong></td>\r\n");
	AddStrWebPage(CmdGenBuf);
	sprintf(CmdGenBuf, "<td with=\"50%%\">%d %d %d %d</td></tr>\r\n", 
		LastPageGenTime[0], LastPageGenTime[1], LastPageGenTime[2], LastPageGenTime[3]);
	AddStrWebPage(CmdGenBuf);

	AddStrWebPage("<tr valign=\"top\" class=\"sectiontableentry1\">\r\n");
	AddStrWebPage( "<td with=\"50%\"><strong>");
	SetRusTextBuf(NULL, SITE_RUS_STATS_AVG_HTM_REQ_TIME_LINE_ID);
	sprintf(CmdGenBuf, "</strong></td>\r\n");
    AddStrWebPage(CmdGenBuf);
	sprintf(CmdGenBuf, "<td with=\"50%%\">%d %d %d %d</td></tr>\r\n", 
		LastReqLoadTime[0], LastReqLoadTime[1], LastReqLoadTime[2], LastReqLoadTime[3]);
	AddStrWebPage(CmdGenBuf);

    AddStrWebPage("</table>\r\n");
	AddEndPageShopWebPage(BufAnsw, SessionPtr);
}
//---------------------------------------------------------------------------
static void AddStatsToPrintList(unsigned int StatsTxtId, unsigned int StatsValue)
{
    char CmdGenBuf[64];

	AddStrWebPage("<tr valign=\"top\" class=\"sectiontableentry1\">\r\n");
	AddStrWebPage("<td with=\"50%\"><strong>");
	SetRusTextBuf(NULL, StatsTxtId);
	sprintf(CmdGenBuf, "</strong></td>\r\n");
	AddStrWebPage( CmdGenBuf);
	sprintf(CmdGenBuf, "<td with=\"50%%\">%d</td></tr>\r\n", StatsValue);
	AddStrWebPage( CmdGenBuf);
}
//---------------------------------------------------------------------------
