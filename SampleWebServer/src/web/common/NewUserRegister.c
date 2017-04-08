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

extern PARAMWEBSERV *ParWebServPtr;
extern READWEBSOCK  *ParReadHttpSocketPtr;
extern USER_DB_INFO SampleUserDbIfo;

char KeyNameId[]     = "basename";
char KeyUserNameId[] = "username";
char EmailId[]       = "email";
char PasswordId[]    = "password";
char SecKeyId[]      = "secure_key";
char ds_login_1[]    = "admin";
char ds_login_2[]    = "root";
//---------------------------------------------------------------------------
void NewUserRegister(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool         isParseDone = false;
    char*        FText = NULL;
	char*        FStrt = NULL;
	USER_INFO*   NewUserPtr = NULL;
	int          i, pars_read, TextLen, EnterCapchaCode;
	unsigned int SecKeyForm;
	char         *MailBodyOrderConfPtr = NULL;
    bool         MailStartResult;
	char         TextLineBuf[128];
	char         MailSubject[128];
#ifdef _LINUX_X86_        
    struct timeb hires_cur_time;
    struct tm    *CurrTime;
#endif

	for(;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
	    FStrt = FText;
        strcpy(FText, HttpCmd);
        
		i = FindCmdRequest(FText, SecKeyId);
		if (i == -1) break;        
        FText = ParseParForm(&FText[i]);
        if (!FText) break;
	    pars_read = sscanf(FText, "%d", &SecKeyForm);
	    if (!pars_read) break;
		if (SecKeyForm != SessionPtr->SecureKey) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

        if ((ParReadHttpSocketPtr->WebChanId == SECONDARY_WEB_CHAN) && 
            !ParWebServPtr->ServCustomCfg.SecondPortInfoEdit) break;

	    NewUserPtr = NewUserInfoCreate(&SampleUserDbIfo);
        if (!NewUserPtr) break;
        
        if (!StringParParse(FText, (char*)&NewUserPtr->Name[0], 
            KeyNameId, MAX_LEN_USER_INFO_NAME)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);
        
        if (!StringParParse(FText, (char*)&NewUserPtr->UserName[0], 
            KeyUserNameId, MAX_LEN_USER_INFO_USER_NAME)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

        if (!StringParParse(FText, (char*)&NewUserPtr->Email[0], 
            EmailId, MAX_LEN_USER_INFO_EMAIL)) break;            
		FText = FStrt;
		strcpy(FText,HttpCmd);

        if (!StringParParse(FText, (char*)&NewUserPtr->Passwd[0], 
            PasswordId, MAX_LEN_USER_INFO_PASSWD)) break;
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
    
    if (isParseDone)
	{
        if (UserCapchaCodeValidate(SessionPtr->CapchaCode, EnterCapchaCode))
        {
		    if ((strcmp(NewUserPtr->UserName,  ds_login_1) != 0) && 
			    (strcmp(NewUserPtr->UserName,  ds_login_2) != 0) &&
			    (!CheckLoginUserInfoDb(NewUserPtr->UserName, NewUserPtr->Passwd)))
		    {
			    SessionPtr->UserPtr = NewUserPtr;
			    strncpy(&NewUserPtr->Contact.FirstName[0], &NewUserPtr->Name[0], MAX_LEN_USER_INFO_NAME);
			    strncpy(&NewUserPtr->Contact.Email[0], &NewUserPtr->Email[0], MAX_LEN_USER_INFO_EMAIL);
                NewUserPtr->UserType = UAT_GUEST;
                NewUserInfoAddDb(&SampleUserDbIfo, NewUserPtr);
			    if (strlen(NewUserPtr->Contact.Email) > 0)
			    {
	                MailBodyOrderConfPtr = (char*)AllocateMemory(8000*sizeof(char));
	                MailBodyOrderConfPtr[0] = 0;
			        MailSubject[0] = 0;
                    strcat(MailSubject, ParWebServPtr->ShopInfoCfg.Name);
		            strcat(MailSubject, ": ");
			        SetOriginalRusTextBuf(MailSubject, SITE_RUS_NEW_USER_INFO_TITLE_LINE_ID);
		            SetOriginalRusTextBuf(MailBodyOrderConfPtr, SITE_RUS_NEW_USER_INFO_1_LINE_ID);
		            strcat(MailBodyOrderConfPtr, "\n");
		            SetOriginalRusTextBuf(MailBodyOrderConfPtr, SITE_RUS_NEW_USER_INFO_2_LINE_ID);
		            strcat(MailBodyOrderConfPtr," \"");
		            strcat(MailBodyOrderConfPtr, ParWebServPtr->ShopInfoCfg.Name);
		            strcat(MailBodyOrderConfPtr,"\" (");
		            strcat(MailBodyOrderConfPtr, ParWebServPtr->ShopInfoCfg.URL);
		            strcat(MailBodyOrderConfPtr,")\n");
		            strcat(MailBodyOrderConfPtr, "\n\n");
		            SetOriginalRusTextBuf(MailBodyOrderConfPtr, SITE_RUS_NEW_USER_INFO_3_LINE_ID);
				    strcat(MailBodyOrderConfPtr, " ");
		            strcat(MailBodyOrderConfPtr, ParWebServPtr->ShopInfoCfg.URL);
                    strcat(MailBodyOrderConfPtr, ".\n\n");
		            SetOriginalRusTextBuf(MailBodyOrderConfPtr, SITE_RUS_NEW_USER_INFO_4_LINE_ID);
		            strcat(MailBodyOrderConfPtr, "\n");
                    SetOriginalRusTextBuf(MailBodyOrderConfPtr, SITE_RUS_NEW_USER_INFO_5_LINE_ID);
			        sprintf(TextLineBuf, " %s\n", NewUserPtr->UserName);
			        strcat(MailBodyOrderConfPtr, TextLineBuf);
			        SetOriginalRusTextBuf(MailBodyOrderConfPtr, SITE_RUS_NEW_USER_INFO_6_LINE_ID);
			        sprintf(TextLineBuf, " %s\n", NewUserPtr->Passwd);
			        strcat(MailBodyOrderConfPtr, TextLineBuf);
			        SetOriginalRusTextBuf(MailBodyOrderConfPtr, SITE_RUS_NEW_USER_INFO_7_LINE_ID);
				    strcat(MailBodyOrderConfPtr, " ");
		            strcat(MailBodyOrderConfPtr, ParWebServPtr->ShopInfoCfg.URL);
                    strcat(MailBodyOrderConfPtr, ". ");
				    SetOriginalRusTextBuf(MailBodyOrderConfPtr, SITE_RUS_NEW_USER_INFO_8_LINE_ID);
                    strcat(MailBodyOrderConfPtr, "\n");

		            MailStartResult = SendMailSmtpServer(&ParWebServPtr->MailWorker,
			            NewUserPtr->Contact.Email, MailSubject, MailBodyOrderConfPtr);
			    }
			    UserContactPageGen(BufAnsw, SessionPtr, HttpCmd);
		    }
		    else
		    {
                if (NewUserPtr->ExtUserInfoPtr) FreeMemory(NewUserPtr->ExtUserInfoPtr);
		        FreeMemory(NewUserPtr);
                AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		        strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		        SetRusTextBuf(BufAnsw, SITE_RUS_USER_ALREADY_EXIST_LINE_ID);
		        strcat(BufAnsw,"</font></center>\r\n");                
		        AddEndPageShopWebPage(BufAnsw, SessionPtr);
		    }
        }
        else
        {
            /* Wrong code from capcha picture was entered */
            if (NewUserPtr->ExtUserInfoPtr) FreeMemory(NewUserPtr->ExtUserInfoPtr);
		    FreeMemory(NewUserPtr);
            AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		    strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		    SetRusTextBuf(BufAnsw, SITE_RUS_WRONG_CAPCHA_LINE_ID);
		    strcat(BufAnsw,"</font></center>\r\n");            
		    AddEndPageShopWebPage(BufAnsw, SessionPtr);            
        }
	}
	else
	{
        if (NewUserPtr)
        {
            if (NewUserPtr->ExtUserInfoPtr) FreeMemory(NewUserPtr->ExtUserInfoPtr);
		    FreeMemory(NewUserPtr);
        }
	    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		SetRusTextBuf(BufAnsw, SITE_RUS_ITEM_NOT_FOUND_LINE_ID);
		strcat(BufAnsw,"</font></center>\r\n");        
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
	}
	FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
