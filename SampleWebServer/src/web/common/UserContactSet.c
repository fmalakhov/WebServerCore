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
extern READWEBSOCK *ParReadHttpSocketPtr;
extern USER_DB_INFO SampleUserDbIfo;

extern char SecKeyId[];
extern char KeyUserId[];
extern char ServerHttpAddr[];
extern char FormKeyUserId[];
//---------------------------------------------------------------------------
void UserContactSet(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool          isParseDone = false;
    char*         FText = NULL;
	char*         FStrt = NULL;
	USER_INFO*    ChgUserInfoPtr = NULL;
	int           i, pars_read, ReadVal, TextLen;
	unsigned int  SecKeyForm, UserId, TitleId;

	for(;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
	    FStrt = FText;
        strcpy(FText, HttpCmd);
        
        if ((ParReadHttpSocketPtr->WebChanId != PRIMARY_WEB_CHAN) && 
            !ParWebServPtr->ServCustomCfg.SecondPortInfoEdit)
        {
            break;
        }
                
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

	    i = FindCmdRequest(FText, FormKeyUserId);
		if (i == -1) break;
        FText = ParseParForm( &FText[i] );
        if (!FText) break;
	    pars_read = sscanf(FText, "%d", &UserId);
	    if (!pars_read) break;
		if (UserId != SessionPtr->UserPtr->UserId) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);
        
        if (ParWebServPtr->ServCustomCfg.DemoMode)
        {
            isParseDone = true;
            break;
        }        

        ChgUserInfoPtr = NewUserInfoCreate(&SampleUserDbIfo);
        if (!ChgUserInfoPtr) break;

		if (ParWebServPtr->ServCustomCfg.SetCompanyName)
		{
		    if (!StringParParse(FText, (char*)&ChgUserInfoPtr->Contact.CompanyName,
		        "company", MAX_LEN_COMPANY_NAME)) break;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
		}

		if (ParWebServPtr->ServCustomCfg.SetManNamePrefix)
		{
	        i = FindCmdRequest(FText, "title");
		    if (i == -1) break;
            FText = ParseParForm( &FText[i] );
            if (!FText) break;
		    if (!strlen(FText)) break;
		    pars_read = sscanf(FText, "%d", &TitleId);
	        if (!pars_read) break;
		    if ((TitleId < 0)|| (TitleId > 4)) break;
		    ChgUserInfoPtr->Contact.UserTitleId = TitleId;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
		}

		if (!StringParParse(FText, (char*)&ChgUserInfoPtr->Contact.FirstName,
		    "first_name", MAX_LEN_USER_INFO_NAME)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		if (ParWebServPtr->ServCustomCfg.SetLastNameField)
		{
		    if (!StringParParse(FText, (char*)&ChgUserInfoPtr->Contact.LastName,
		        "last_name", MAX_LEN_USER_INFO_NAME)) break;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
		}

		if (ParWebServPtr->ServCustomCfg.SetMiddleNameField)
		{
		    if (!StringParParse(FText, (char*)&ChgUserInfoPtr->Contact.MiddleName,
		        "middle_name", MAX_LEN_USER_INFO_NAME)) break;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
		}
        
        if (ParWebServPtr->ServCustomCfg.SetUserAddr1Field)
        {
		    if (!StringParParse(FText, (char*)&ChgUserInfoPtr->Contact.Address1,
		        "address_1", MAX_LEN_ADDR_1_NAME)) break;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
        }

		if (ParWebServPtr->ServCustomCfg.SetUserAddr2Field)
		{
		    if (!StringParParse(FText, (char*)&ChgUserInfoPtr->Contact.Address2,
		        "address_2", MAX_LEN_ADDR_2_NAME)) break;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
		}

		if (ParWebServPtr->ServCustomCfg.SetUserCityField)
		{
		    if (!StringParParse(FText, (char*)&ChgUserInfoPtr->Contact.City,
		        "city", MAX_LEN_CITY_NAME)) break;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);

		    i = FindCmdRequest(FText, "zip");
		    if (i == -1) break;
            FText = ParseParForm( &FText[i] );
            if (!FText) break;
		    if (!strlen(FText)) break;
		    pars_read = sscanf(FText, "%d", &ChgUserInfoPtr->Contact.ZipCode);
	        if (!pars_read) break;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
		    }

		if (ParWebServPtr->ServCustomCfg.SetUserCountryField)
		{
		    i = FindCmdRequest(FText, "country");
		    if (i == -1) break;
            FText = ParseParForm( &FText[i] );
            if (!FText) break;
		    pars_read = sscanf(FText, "%d", &ChgUserInfoPtr->Contact.CountryId);
	        if (!pars_read) break;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
		}

        if (ParWebServPtr->ServCustomCfg.SetUserLandPhoneField)
        {
		    if (!StringParParse(FText, (char*)&ChgUserInfoPtr->Contact.LandPhone,
		        "land_phone", MAX_LEN_PHONE_NUM)) break;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
        }

		if ((ParWebServPtr->ServCustomCfg.SetUserMobPhoneField) &&
            (SessionPtr->UserPtr) && (SessionPtr->UserPtr->UserType != UAT_GUEST))
		{
		    if (!StringParParse(FText, (char*)&ChgUserInfoPtr->Contact.MobilePhone,
		        "mobile_phone", MAX_LEN_PHONE_NUM)) break;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
		}

        if (ParWebServPtr->ServCustomCfg.SetUserFaxField)
		{
		    if (!StringParParse(FText, (char*)&ChgUserInfoPtr->Contact.FaxPhone,
		        "fax", MAX_LEN_PHONE_NUM)) break;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
		}

		if (!StringParParse(FText, (char*)&ChgUserInfoPtr->Contact.Email,
		    "email", MAX_LEN_USER_INFO_EMAIL)) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

        if ((SessionPtr->UserPtr) && (SessionPtr->UserPtr->UserType != UAT_GUEST))
        {
            /* Parse value of Mail notify about alarm flag */
            i = FindCmdRequest(FText, "mail_etn");
            if (i != -1)
            {
		        FText = ParseParForm(&FText[i]);
                if (!FText) break;
		        TextLen = strlen(FText);
		        if (!TextLen) break;
		        pars_read = sscanf(FText, "%d", &ReadVal);
		        if (!pars_read) break;
		        if (ReadVal == 1)
		        {
                    ((SAMPLE_USER_INFO*)ChgUserInfoPtr->ExtUserInfoPtr)->EventMailNotify = true;
		        }
		        FText = FStrt;
		        strcpy(FText,HttpCmd);            
            }
            else
            {
                ((SAMPLE_USER_INFO*)ChgUserInfoPtr->ExtUserInfoPtr)->EventMailNotify = false;
            }
        }
        else
        {
            ((SAMPLE_USER_INFO*)ChgUserInfoPtr->ExtUserInfoPtr)->EventMailNotify = false;
        }

		isParseDone = true;
		break;
	}
    if (isParseDone)
	{
        if (!ParWebServPtr->ServCustomCfg.DemoMode)
        {
		    memcpy(&SessionPtr->UserPtr->Contact, &ChgUserInfoPtr->Contact, sizeof(CONTACT_INFO));
		    strcpy(SessionPtr->UserPtr->Name, ChgUserInfoPtr->Contact.FirstName);
		    strcpy(SessionPtr->UserPtr->Email, ChgUserInfoPtr->Contact.Email);
            ((SAMPLE_USER_INFO*)SessionPtr->UserPtr->ExtUserInfoPtr)->EventMailNotify = ((SAMPLE_USER_INFO*)ChgUserInfoPtr->ExtUserInfoPtr)->EventMailNotify;
		    UserInfoDBSave(&SampleUserDbIfo);
        }
		MainPageSetShopWebPage(BufAnsw, SessionPtr, HttpCmd);
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
	if (ChgUserInfoPtr)
    {
        if (ChgUserInfoPtr->ExtUserInfoPtr) FreeMemory(ChgUserInfoPtr->ExtUserInfoPtr);
        FreeMemory(ChgUserInfoPtr);
    }
}
//---------------------------------------------------------------------------
