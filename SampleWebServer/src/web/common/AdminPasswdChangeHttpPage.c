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

extern char KeyNameId[];
extern char KeyUserNameId[];
extern char EmailId[];
extern char PasswordId[];
extern char SecKeyId[];
extern char KeyUserId[];
extern char FormKeySessionId[];
extern char FormKeyUserId[];
extern USER_DB_INFO SampleUserDbIfo;

char KeyFormChgPwdReq[] = "chg_passwd_req";

void AdminPasswdChgpWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
//---------------------------------------------------------------------------
void AdminPasswdChange(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool         isParseDone = false;
	unsigned int TextLen;
	int          i, pars_read;
	unsigned int PwdSetReq, SecKeyForm, UserId;
    char*        FText = NULL;
	char*        FStrt = NULL;
	USER_INFO    *AdmUserPtr = NULL;
	char         EditPasswd[MAX_LEN_USER_INFO_PASSWD+1];

	for(;;)
	{
		EditPasswd[0] = 0;
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

		PwdSetReq = 0;
		i = FindCmdRequest(FText, KeyFormChgPwdReq);
		if (i != -1)
		{
		    FText = ParseParForm( &FText[i] );
            if (!FText) break;
		    TextLen = strlen(FText);
		    if (!TextLen) break;
		    pars_read = sscanf(FText, "%d", &PwdSetReq);
		    if (!pars_read) break;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);

			if (!StringParParse(FText, (char*)&EditPasswd[0],
		        PasswordId, MAX_LEN_USER_INFO_PASSWD)) break;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
		}
		isParseDone = true;
		break;
	}
    if (isParseDone)
	{
		if (PwdSetReq > 0)
		{
			AdmUserPtr = GetUserInfoById(1);
			strcpy(AdmUserPtr->Passwd, EditPasswd);
			UserInfoDBSave(&SampleUserDbIfo);

	        AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		    strcat(BufAnsw,"<center><font size=\"3\" color=\"green\">");
		    SetRusTextBuf(BufAnsw, SITE_RUS_SUCC_ADM_PWD_CHG_LINE_ID);
		    strcat(BufAnsw,"</font></center>\r\n");
		    AddEndPageShopWebPage(BufAnsw, SessionPtr);
		}
		else
		{
			AdminPasswdChgpWebPage(BufAnsw, SessionPtr, HttpCmd);
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
void AdminPasswdChgpWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	char StrBuf[64];
	USER_INFO  *AdmUserPtr = NULL;

	AddBeginPageShopWebPage(BufAnsw, SessionPtr);
	AdmUserPtr = GetUserInfoById(1);

    strcat(BufAnsw,"<script language=\"javascript\" type=\"text/javascript\">\r\n");
    strcat(BufAnsw,"function on_adm_pwd_change_req() {\r\n");
    strcat(BufAnsw,"var form = document.AdmPwdChgForm;\r\n");
    strcat(BufAnsw,"var r = new RegExp(\"[\\<|\\>|\\\"|\\'|\\%|\\;|\\(|\\)|\\&|\\+|\\-]\", \"i\");\r\n");
    strcat(BufAnsw,"if (form.password.value.length < 6) {\r\n");
    strcat(BufAnsw,   "alert( \"");
	SetOriginalRusTextBuf(BufAnsw, SITE_RUS_USER_REG_5_LINE_ID);
    strcat(BufAnsw,"\" );\r\n");
    strcat(BufAnsw,"} else if (form.password2.value == \"\") {\r\n");
    strcat(BufAnsw,"   alert( \"");
	SetOriginalRusTextBuf(BufAnsw, SITE_RUS_USER_REG_6_LINE_ID);
    strcat(BufAnsw,"\" );\r\n");
    strcat(BufAnsw,"} else if ((form.password.value != \"\") && (form.password.value != form.password2.value)){\r\n");
    strcat(BufAnsw,"   alert( \"");
	SetOriginalRusTextBuf(BufAnsw, SITE_RUS_USER_REG_7_LINE_ID);
    strcat(BufAnsw,"\" );\r\n");
    strcat(BufAnsw,"} else if (r.exec(form.password.value)) {\r\n");
    strcat(BufAnsw,"   alert( \"");
	SetOriginalRusTextBuf(BufAnsw, SITE_RUS_USER_REG_8_LINE_ID);
    strcat(BufAnsw,"\" );\r\n");
    strcat(BufAnsw,"} else {\r\n");
	strcat(BufAnsw,"form.submit();}\r\n");
    strcat(BufAnsw,"}\r\n</script>\r\n");

    strcat(BufAnsw,"<form action=\"");
	strcat(BufAnsw, GenPageAdmPswdChg);
	strcat(BufAnsw,"\" method=\"post\" name=\"AdmPwdChgForm\">\r\n");
	strcat(BufAnsw,"<div class=\"componentheading\">");
    SetRusTextBuf(BufAnsw, SITE_RUS_ADM_PWD_CHG_LINE_ID);
	strcat(BufAnsw,"</div>\r\n");
	strcat(BufAnsw,"<table cellpadding=\"0\" cellspacing=\"0\" border=\"0\" width=\"100%\" class=\"contentpane\">\r\n");
	strcat(BufAnsw,"<tr><td colspan=\"2\">\r\n");
	SetRusTextBuf(BufAnsw, SITE_RUS_USER_REG_9_LINE_ID);
	strcat(BufAnsw,"</td></tr>\r\n");

	strcat(BufAnsw,"<tr><td>\r\n");
    SetRusTextBuf(BufAnsw, SITE_RUS_USER_REG_12_LINE_ID);
	strcat(BufAnsw,"</td>\r\n");
	strcat(BufAnsw,"<td><input class=\"inputbox\" type=\"password\" name=\"");
	strcat(BufAnsw,PasswordId);
	strcat(BufAnsw,"\" size=\"40\" value=\"");
	strcat(BufAnsw, AdmUserPtr->Passwd);
	strcat(BufAnsw,"\" maxlength=\"");
	sprintf(StrBuf, "%d\">\r\n", MAX_LEN_USER_INFO_PASSWD);
	strcat(BufAnsw,StrBuf);
	strcat(BufAnsw,"</td></tr>\r\n");

	strcat(BufAnsw,"<tr><td>\r\n");
    SetRusTextBuf(BufAnsw, SITE_RUS_USER_REG_13_LINE_ID);
	strcat(BufAnsw,"</td>\r\n");
	strcat(BufAnsw,"<td><input class=\"inputbox\" type=\"password\" name=\"password2\" size=\"40\" value=\"\" maxlength=\"");
	sprintf(StrBuf, "%d\">\r\n", MAX_LEN_USER_INFO_PASSWD);
	strcat(BufAnsw,StrBuf);
	strcat(BufAnsw,"</td></tr>\r\n");

	strcat(BufAnsw,"<tr><td colspan=\"2\"></td></tr>\r\n");
	strcat(BufAnsw,"<tr><td colspan=\"2\"></td></tr>\r\n");
	strcat(BufAnsw,"</table>\r\n");
	strcat(BufAnsw,"<input type=\"button\" value=\"");
	SetRusTextBuf(BufAnsw, SITE_RUS_SENT_REQ_LINE_ID);
    strcat(BufAnsw,"\" class=\"button\" onclick=\"on_adm_pwd_change_req()\">\r\n");

	SetHiddenIntParForm(BufAnsw, SecKeyId, SessionPtr->SecureKey);
    SetHiddenStrParForm(BufAnsw, FormKeySessionId, SessionPtr->SesionIdKey);
	SetHiddenIntParForm(BufAnsw, FormKeyUserId, SessionPtr->UserPtr->UserId);
	SetHiddenIntParForm(BufAnsw, KeyFormChgPwdReq, 1);

	strcat(BufAnsw,"</form>\r\n");
	AddEndPageShopWebPage(BufAnsw, SessionPtr);
}
//---------------------------------------------------------------------------
