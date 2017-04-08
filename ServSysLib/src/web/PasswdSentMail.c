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

extern ListItsTask  UserInfoList;
extern unsigned int UserPreffixArray[];
extern char KeyNameId[];
extern char EmailId[];
extern PARAMWEBSERV *ParWebServPtr;
//---------------------------------------------------------------------------
void PasswdSentMail(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool       isParseDone = false;
    bool       MailStartResult;
    char*      FText = NULL;
	char*      FStrt = NULL;
	int          i, pars_read, TextLen, EnterCapchaCode, LastNameLen, FirstNameLen;
	char       *MailBodyOrderConfPtr = NULL;
	USER_INFO* SelUserPtr = NULL;
	char       UserName[MAX_LEN_USER_INFO_NAME+1];
	char       UserMail[MAX_LEN_USER_INFO_EMAIL+1];
	char       TextLineBuf[128];
	char       MailSubject[128];

	for(;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
	    FStrt = FText;
        strcpy(FText, HttpCmd);
		
		/* User's name parameter parse */
		if (!StringParParse(FText, UserName,
				KeyNameId, MAX_LEN_USER_INFO_NAME)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		/* User's email parameter parse */
		if (!StringParParse(FText, UserMail,
				EmailId, MAX_LEN_USER_INFO_EMAIL)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);
        
	    /* Capcha code check */
	    i = FindCmdRequest(FText, KeyFormCapchaCode);
	    if (i == -1) break;
	    FText = ParseParForm(&FText[i]);
        if (!FText) break;
	    TextLen = strlen(FText);
	    pars_read = sscanf(FText, "%d", &EnterCapchaCode);
	    if (!pars_read) break;        
	    FText = FStrt;
	    strcpy(FText,HttpCmd);               
		isParseDone = true;
		break;
	}
	AddBeginPageShopWebPage(BufAnsw, SessionPtr);
    if (isParseDone)
	{
        if (UserCapchaCodeValidate(SessionPtr->CapchaCode, EnterCapchaCode))
        {
            strcat(BufAnsw,"<div class=\"componentheading\">\r\n");
	        SetRusTextBuf(BufAnsw, SITE_RUS_AUTH_INFO_USR_REC_LINE_ID);
            strcat(BufAnsw,"</div>\r\n");
		    SelUserPtr = GetUserInfoDbByNameEmail(UserName, UserMail);
		    if (SelUserPtr)
		    {
		        strcat(BufAnsw,"<center><font size=\"3\" color=\"green\">");
		        SetRusTextBuf(BufAnsw, SITE_RUS_USER_INF_FIND_SUCC_LINE_ID);
		        strcat(BufAnsw,"</font></center>\r\n");

	            MailBodyOrderConfPtr = (char*)AllocateMemory(8000*sizeof(char));
	            MailBodyOrderConfPtr[0] = 0;
	            SetOriginalRusTextBuf(MailBodyOrderConfPtr, SITE_RUS_ORDER_CONF_BODY_1_LINE_ID);
	            strcat(MailBodyOrderConfPtr, ", ");
	            if (SelUserPtr->Contact.UserTitleId > 0)
	            {
                    SetOriginalRusTextBuf(MailBodyOrderConfPtr, 
			            UserPreffixArray[SelUserPtr->Contact.UserTitleId]);
		            strcat(MailBodyOrderConfPtr," ");
	            }
                FirstNameLen = strlen(SelUserPtr->Contact.FirstName);
                LastNameLen = strlen(SelUserPtr->Contact.LastName);
                if (LastNameLen > 0) strcat(MailBodyOrderConfPtr, SelUserPtr->Contact.LastName);
                if (FirstNameLen > 0)
                {
                    if (LastNameLen > 0) strcat(MailBodyOrderConfPtr, " ");
                    strcat(MailBodyOrderConfPtr, SelUserPtr->Contact.FirstName);
                }
                if (strlen(SelUserPtr->Contact.MiddleName) > 0)
                {
                    if ((FirstNameLen > 0) || (LastNameLen > 0)) strcat(MailBodyOrderConfPtr, " ");
                    strcat(MailBodyOrderConfPtr, SelUserPtr->Contact.MiddleName);
                }
                strcat(MailBodyOrderConfPtr, ".\n\n");
                
			    SetOriginalRusTextBuf(MailBodyOrderConfPtr, SITE_RUS_AUTH_INFO_PERS_PSWD_LINE_ID);
			    sprintf(TextLineBuf, ": <%s>\n\n", SelUserPtr->Passwd);
			    strcat(MailBodyOrderConfPtr, TextLineBuf);
			    MailSubject[0] = 0;
                strcat(MailSubject, ParWebServPtr->ShopInfoCfg.Name);
		        strcat(MailSubject, ": ");
			    SetOriginalRusTextBuf(MailSubject, SITE_RUS_AUTH_INFO_MSG_TITLE_LINE_ID);
		        SetOriginalRusTextBuf(MailBodyOrderConfPtr, SITE_RUS_ORDER_CONF_BODY_11_LINE_ID);
		        strcat(MailBodyOrderConfPtr, "\n\n");
		        SetOriginalRusTextBuf(MailBodyOrderConfPtr, SITE_RUS_ORDER_CONF_BODY_8_LINE_ID);
		        strcat(MailBodyOrderConfPtr, "\n\n");
		        SetOriginalRusTextBuf(MailBodyOrderConfPtr, SITE_RUS_ORDER_CONF_BODY_9_LINE_ID);
		        strcat(MailBodyOrderConfPtr, "\n\n");
		        SetOriginalRusTextBuf(MailBodyOrderConfPtr, SITE_RUS_ORDER_CONF_BODY_10_LINE_ID);
		        strcat(MailBodyOrderConfPtr,"\"");
		        strcat(MailBodyOrderConfPtr, ParWebServPtr->ShopInfoCfg.Name);
		        strcat(MailBodyOrderConfPtr,"\" (");
		        strcat(MailBodyOrderConfPtr, ParWebServPtr->ShopInfoCfg.URL);
		        strcat(MailBodyOrderConfPtr,")\n");

		        MailStartResult = SendMailSmtpServer(&ParWebServPtr->MailWorker,
			        SelUserPtr->Contact.Email, MailSubject, MailBodyOrderConfPtr);
		    }
		    else
		    {
		        strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		        SetRusTextBuf(BufAnsw, SITE_RUS_USER_NOT_FOUND_LINE_ID);
		        strcat(BufAnsw,"</font></center>\r\n");
		    }            
        }
        else
        {
            /* Wrong code from capcha picture was entered */
            strcat(BufAnsw,"<div class=\"componentheading\">\r\n");
	        SetRusTextBuf(BufAnsw, SITE_RUS_AUTH_INFO_USR_REC_LINE_ID);
            strcat(BufAnsw,"</div>\r\n");
		    strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		    SetRusTextBuf(BufAnsw, SITE_RUS_WRONG_CAPCHA_LINE_ID);
		    strcat(BufAnsw,"</font></center>\r\n");                               
        }
	}
	else
	{
		strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		SetRusTextBuf(BufAnsw, SITE_RUS_ITEM_NOT_FOUND_LINE_ID);
		strcat(BufAnsw,"</font></center>\r\n");
	}
	AddEndPageShopWebPage(BufAnsw, SessionPtr);
	FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
