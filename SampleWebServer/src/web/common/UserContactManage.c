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

extern char *EndHtmlPageGenPtr;
extern PARAMWEBSERV *ParWebServPtr;
extern READWEBSOCK *ParReadHttpSocketPtr;
extern USER_DB_INFO SampleUserDbIfo;

extern char KeySessionId[];
extern char SecKeyId[];
extern char KeyUserId[];
extern char ServerHttpAddr[];

char FormKeyUserId[]    = "user_id";
char FormKeySessionId[] = "session_id";

unsigned int UserPreffixArray[] = {
SITE_RUS_ORDER_REQ_TYPE_1_LINE_ID, SITE_RUS_ORDER_REQ_TYPE_2_LINE_ID,
SITE_RUS_ORDER_REQ_TYPE_3_LINE_ID, SITE_RUS_ORDER_REQ_TYPE_4_LINE_ID,
SITE_RUS_ORDER_REQ_TYPE_5_LINE_ID};
//---------------------------------------------------------------------------
void UserContactManage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool         isParseDone = false;
    char*        FText = NULL;
	char*        FStrt = NULL;
	int          i, pars_read;
	unsigned int SecKeyForm, UserId;

	for(;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
		if (!FText) break;
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
		UserContactPageGen(BufAnsw, SessionPtr, HttpCmd);
	}
	else
	{
	    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
		SetRusTextBuf(BufAnsw, SITE_RUS_ITEM_NOT_FOUND_LINE_ID);
		strcat(BufAnsw,"</font></center>\r\n");
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
	}
	if (FStrt) FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
void UserContactPageGen(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
    int  Index;
	unsigned int EditFiledSize = 30;
	bool isFindSession = false;
	char StrBuf[64];
	USER_INFO *UserPtr;

    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
	EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];

	UserPtr = SessionPtr->UserPtr;
    AddStrWebPage("<div class=\"componentheading\">\r\n");
	SetRusTextBuf(NULL, SITE_RUS_MY_CONTACTS_LINE_ID);
    AddStrWebPage("</div>\r\n");

    if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)
	{
		EditFiledSize = 30;
	}
	else
	{
		EditFiledSize = 25;
	}
	// Fill user's contact information
	if (!strlen(UserPtr->Contact.LandPhone) &&
		!strlen(UserPtr->Contact.MobilePhone))
	{
	    AddStrWebPage("<br ><br >\r\n");
        AddStrWebPage("<div class=\"sectiontableheader\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_ORDER_2_LINE_ID);
        AddStrWebPage("</div><br >\r\n");
	}
    AddStrWebPage("<script language=\"javascript\" type=\"text/javascript\">//<![CDATA[\r\n");
    AddStrWebPage("function submitregistration() {\r\n");
    AddStrWebPage("var form = document.adminForm;\r\n");

	/* Check first name filed */
	AddStrWebPage("var isvalid = true;\r\n");
    AddStrWebPage("if (form.first_name.value == \"\") {\r\n");
	AddStrWebPage("document.getElementById('first_name_div').style.color = \"red\";\r\n");
	AddStrWebPage("isvalid = false;}\r\n");
	AddStrWebPage("else {document.getElementById('first_name_div').style.color = \"white\";}\r\n");

	/* Check last name filed */
	if (ParWebServPtr->ServCustomCfg.ReqLastNameField)
	{
        AddStrWebPage("if (form.last_name.value == \"\") {\r\n");
	    AddStrWebPage("document.getElementById('last_name_div').style.color = \"red\";\r\n");
	    AddStrWebPage("isvalid = false;}\r\n");
	    AddStrWebPage("else {document.getElementById('last_name_div').style.color = \"white\";}\r\n");
    }

	/* Check middle name filed */
	if (ParWebServPtr->ServCustomCfg.ReqMiddleNameField)
	{
	    AddStrWebPage("if (form.middle_name.value == \"\") {\r\n");
	    AddStrWebPage("document.getElementById('middle_name_div').style.color = \"red\";\r\n");
	    AddStrWebPage("isvalid = false;}\r\n");
	    AddStrWebPage("else {document.getElementById('middle_name_div').style.color = \"white\";}\r\n");
	}

	/* Check address 1 filed */
	if (ParWebServPtr->ServCustomCfg.ReqUserAddr1Field)
	{
        AddStrWebPage("if (form.address_1.value == \"\") {\r\n");
	    AddStrWebPage("document.getElementById('address_1_div').style.color = \"red\";\r\n");
	    AddStrWebPage("isvalid = false;}\r\n");
	    AddStrWebPage("else {document.getElementById('address_1_div').style.color = \"white\";}\r\n");
	}

	/* Check address 2 filed */
	if (ParWebServPtr->ServCustomCfg.ReqUserAddr2Field)
	{
        AddStrWebPage("if (form.address_2.value == \"\") {\r\n");
	    AddStrWebPage("document.getElementById('address_2_div').style.color = \"red\";\r\n");
	    AddStrWebPage("isvalid = false;}\r\n");
	    AddStrWebPage("else {document.getElementById('address_1_div').style.color = \"white\";}\r\n");
	}

	/* Check city name filed */
	if (ParWebServPtr->ServCustomCfg.ReqUserCityField)
	{
	    AddStrWebPage("if (form.city.value == \"\") {\r\n");
	    AddStrWebPage("document.getElementById('city_div').style.color = \"red\";\r\n");
	    AddStrWebPage("isvalid = false;}\r\n");
	    AddStrWebPage("else {document.getElementById('city_div').style.color = \"white\";}\r\n");
	}

	/* Check zip code filed */
	if (ParWebServPtr->ServCustomCfg.ReqUserZipField)
	{
	    AddStrWebPage("if (form.zip.value == \"\") {\r\n");
	    AddStrWebPage("document.getElementById('zip_div').style.color = \"red\";\r\n");
	    AddStrWebPage("isvalid = false;}\r\n");
	    AddStrWebPage("else {document.getElementById('zip_div').style.color = \"white\";}\r\n");
	}

	/* Check country name filed */
    if (ParWebServPtr->ServCustomCfg.ReqUserCountryField)
	{
        AddStrWebPage("if (form.country.value == \"\") {\r\n");
	    AddStrWebPage("document.getElementById('country_div').style.color = \"red\";\r\n");
	    AddStrWebPage("isvalid = false;}\r\n");
	    AddStrWebPage("else {document.getElementById('country_div').style.color = \"white\";}\r\n");
	}

	/* Check mobile phone number filed */
	if (ParWebServPtr->ServCustomCfg.ReqUserMobPhoneField)
	{
	    AddStrWebPage("if (form.mobile_phone.value == \"\") {\r\n");
	    AddStrWebPage("document.getElementById('mobile_phone_div').style.color = \"red\";\r\n");
	    AddStrWebPage("isvalid = false;}\r\n");
	    AddStrWebPage("else {document.getElementById('mobile_phone_div').style.color = \"white\";}\r\n");
	}

	/* Check land phone number filed */
    if (ParWebServPtr->ServCustomCfg.ReqUserLandPhoneField)
    {
	    AddStrWebPage("if (form.land_phone.value == \"\") {\r\n");
	    AddStrWebPage("document.getElementById('land_phone_div').style.color = \"red\";\r\n");
	    AddStrWebPage("isvalid = false;}\r\n");
	    AddStrWebPage("else {document.getElementById('land_phone_div').style.color = \"white\";}\r\n");
    }
	/* Check email filed */
	AddStrWebPage("if (form.email.value == \"\") {\r\n");
    AddStrWebPage("isvalid = false;\r\n");
	AddStrWebPage("document.getElementById('email_div').style.color = \"red\";\r\n");
    AddStrWebPage("} else if (r.exec(form.email.value)) {\r\n");
	AddStrWebPage("document.getElementById('email_div').style.color = \"red\";\r\n");
	AddStrWebPage("alert( \"");
	SetOriginalRusTextBuf(NULL, SITE_RUS_ORDER_3_LINE_ID);
	AddStrWebPage("\");\r\n");
	AddStrWebPage("isvalid = false;}\r\n");
	AddStrWebPage("else {document.getElementById('email_div').style.color = \"white\";}\r\n");

	/* Check of failed case */
	AddStrWebPage("if(!isvalid) {\r\n");
    AddStrWebPage("alert( \"");
    SetOriginalRusTextBuf(NULL, SITE_RUS_ORDER_5_LINE_ID);
    AddStrWebPage("\");\r\n");
	AddStrWebPage("return false;\r\n");
    AddStrWebPage("} else {\r\n");
	AddStrWebPage("form.submit();}\r\n");
    AddStrWebPage("}\r\n");
    AddStrWebPage("</script>\r\n");

    AddStrWebPage("<form action=\"");
    SetServerHttpAddr(NULL);
	AddStrWebPage("UserContactInfoSet.html\" method=\"post\" name=\"adminForm\">\r\n");
    AddStrWebPage("<div style=\"width:90%;\">\r\n");
    AddStrWebPage("<div style=\"padding:5px;text-align:center;\"><strong>(* = \r\n");
	SetRusTextBuf(NULL, SITE_RUS_ORDER_MUST_LINE_ID);
    AddStrWebPage(")</strong></div>\r\n");
    AddStrWebPage("<div style=\"width:90%;align:center;\">\r\n");
    AddStrWebPage("<fieldset><legend class=\"5\">\r\n");
    SetRusTextBuf(NULL, SITE_RUS_ORDER_6_LINE_ID);
    AddStrWebPage("</legend>\r\n");

	AddStrWebPage("<table width=\"90%\" cellspacing=\"2\" cellpadding=\"2\" align=\"center\" >\r\n");

	if (ParWebServPtr->ServCustomCfg.SetCompanyName)
	{
        AddStrWebPage("<tr><td width=\"40%\" align=\"left\">\r\n");
        AddStrWebPage("<div id=\"company_div\" >\r\n");
        AddStrWebPage("<label for=\"company_field\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_ORDER_COMP_NAME_LINE_ID);
        AddStrWebPage("</label></div>\r\n");
	    AddStrWebPage("</td><td align=\"left\" valign=\"top\">\r\n");
        AddStrWebPage("<div style=\"float:left;\">\r\n");
        AddStrWebPage("<input type=\"text\" id=\"company_field\" name=\"company\" ");
	    sprintf(StrBuf, "size=\"%d\"", EditFiledSize);
        AddStrWebPage(StrBuf);
	    AddStrWebPage(" value=\"");
	    AddStrWebPage( UserPtr->Contact.CompanyName);
	    sprintf(StrBuf, "\" maxlength=\"%d\" ", MAX_LEN_COMPANY_NAME);
	    AddStrWebPage(StrBuf);	
	    AddStrWebPage("class=\"inputbox\" >\r\n");
        AddStrWebPage("</div>\r\n");
        AddStrWebPage("</td></tr>\r\n");
	}

	if (ParWebServPtr->ServCustomCfg.SetManNamePrefix)
	{
	    AddStrWebPage("<tr><td width=\"40%\" align=\"left\">\r\n");
        AddStrWebPage("<div id=\"title_div\" >\r\n");
        AddStrWebPage("<label for=\"title_field\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_ORDER_REQ_TYPE_LINE_ID);
        AddStrWebPage("</label> </div>\r\n");
	    AddStrWebPage("</td><td align=\"left\" valign=\"top\">\r\n");
        AddStrWebPage("<div style=\"float:left;\">\r\n");
        AddStrWebPage("<select class=\"inputbox\" name=\"title\" id=\"user_title\">\r\n");
	    for(Index=0;Index < sizeof(UserPreffixArray)/sizeof(unsigned int);Index++)
	    {
		    if (Index == UserPtr->Contact.UserTitleId)
		    {
			    sprintf(StrBuf, "<option value=\"%d\" selected=\"selected\">", Index);
		    }
		    else
		    {
                sprintf(StrBuf, "<option value=\"%d\">", Index);
	            AddStrWebPage(StrBuf);
		    }
		    SetRusTextBuf(NULL, UserPreffixArray[Index]);
		    AddStrWebPage("</option>\r\n");
	    }
        AddStrWebPage("</select>\r\n");
        AddStrWebPage("</div>\r\n");
        AddStrWebPage("</td></tr>\r\n");
	}

	AddStrWebPage("<tr><td width=\"40%\" align=\"left\">\r\n");
    AddStrWebPage("<div id=\"first_name_div\">\r\n");
    AddStrWebPage("<label for=\"first_name_field\">\r\n");
    SetRusTextBuf(NULL, SITE_RUS_ORDER_NAME_LINE_ID);
    AddStrWebPage("</label><strong>* </strong> </div>\r\n");
	AddStrWebPage("</td><td align=\"left\" valign=\"top\">\r\n");
    AddStrWebPage("<div style=\"float:left;\">\r\n");
    AddStrWebPage("<input type=\"text\" id=\"first_name_field\" name=\"first_name\" ");
	sprintf(StrBuf, "size=\"%d\"", EditFiledSize);
    AddStrWebPage(StrBuf);
	AddStrWebPage(" value=\"");
    if (ParWebServPtr->ServCustomCfg.DemoMode)   AddStrWebPage("Ivan Ivanov");
    else	                                     AddStrWebPage(UserPtr->Name);
	sprintf(StrBuf, "\" maxlength=\"%d\" ", MAX_LEN_USER_INFO_NAME);
	AddStrWebPage(StrBuf);
	AddStrWebPage("class=\"inputbox\" >\r\n");
    AddStrWebPage("</div>\r\n");
    AddStrWebPage("</td></tr>\r\n");

	if (ParWebServPtr->ServCustomCfg.SetMiddleNameField)
	{
	    AddStrWebPage("<tr><td width=\"40%\" align=\"left\">\r\n");
	    AddStrWebPage("<div id=\"middle_name_div\">\r\n");
        AddStrWebPage("<label for=\"middle_name_field\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_ORDER_MID_NAME_LINE_ID);
        AddStrWebPage("</label>");
	    if (ParWebServPtr->ServCustomCfg.ReqMiddleNameField) 
		    AddStrWebPage(" <strong>*</strong>");
	    AddStrWebPage(" </div>\r\n");
	    AddStrWebPage("</td><td align=\"left\" valign=\"top\">\r\n");
        AddStrWebPage("<div style=\"float:left;\">\r\n");
        AddStrWebPage("<input type=\"text\" id=\"middle_name_field\" name=\"middle_name\" ");
	    sprintf(StrBuf, "size=\"%d\"", EditFiledSize);
        AddStrWebPage(StrBuf);
	    AddStrWebPage(" value=\"");
	    AddStrWebPage( UserPtr->Contact.MiddleName);
	    sprintf(StrBuf, "\" maxlength=\"%d\" ", MAX_LEN_USER_INFO_NAME);
	    AddStrWebPage(StrBuf);	
	    AddStrWebPage("class=\"inputbox\" >\r\n");
        AddStrWebPage("</div>\r\n");
        AddStrWebPage("</td></tr>\r\n");
	}

	if (ParWebServPtr->ServCustomCfg.SetLastNameField)
	{
	    AddStrWebPage("<tr><td width=\"40%\" align=\"left\">\r\n");
        AddStrWebPage("<div id=\"last_name_div\" >\r\n");
        AddStrWebPage("<label for=\"last_name_field\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_ORDER_LAST_NAME_LINE_ID);
        AddStrWebPage("</label>");
	    if (ParWebServPtr->ServCustomCfg.ReqLastNameField) 
		    AddStrWebPage(" <strong>*</strong>");	
	    AddStrWebPage("</div>\r\n");
	    AddStrWebPage("</td><td align=\"left\" valign=\"top\">\r\n");
        AddStrWebPage("<div style=\"float:left;\">\r\n");
        AddStrWebPage("<input type=\"text\" id=\"last_name_field\" name=\"last_name\" ");
	    sprintf(StrBuf, "size=\"%d\"", EditFiledSize);
        AddStrWebPage(StrBuf);
	    AddStrWebPage(" value=\"");
	    AddStrWebPage( UserPtr->Contact.LastName);
	    sprintf(StrBuf, "\" maxlength=\"%d\" ", MAX_LEN_USER_INFO_NAME);
	    AddStrWebPage(StrBuf);	
	    AddStrWebPage("class=\"inputbox\" >\r\n");
        AddStrWebPage("</div>\r\n");
        AddStrWebPage("</td></tr>\r\n");
	}

	if (ParWebServPtr->ServCustomCfg.SetUserAddr1Field)
	{
	    AddStrWebPage("<tr><td width=\"40%\" align=\"left\">\r\n");
        AddStrWebPage("<div id=\"address_1_div\" >\r\n");
        AddStrWebPage("<label for=\"address_1_field\">\r\n");
	    if (ParWebServPtr->ServCustomCfg.SetUserAddr2Field)
               SetRusTextBuf(NULL, SITE_RUS_ORDER_ADDR_1_LINE_ID);
	    else   SetRusTextBuf(NULL, SITE_RUS_ORDER_ADDR_3_LINE_ID);
        AddStrWebPage("</label>");
	    if (ParWebServPtr->ServCustomCfg.ReqUserAddr1Field) 
		    AddStrWebPage(" <strong>*</strong>");
	    AddStrWebPage(" </div>\r\n");
	    AddStrWebPage("</td><td align=\"left\" valign=\"top\">\r\n");
        AddStrWebPage("<div style=\"float:left;\">\r\n");
        AddStrWebPage("<input type=\"text\" id=\"address_1_field\" name=\"address_1\" ");
	    sprintf(StrBuf, "size=\"%d\"", EditFiledSize);
        AddStrWebPage(StrBuf);
	    AddStrWebPage(" value=\"");
	    AddStrWebPage(UserPtr->Contact.Address1);
	    sprintf(StrBuf, "\" maxlength=\"%d\" ", MAX_LEN_ADDR_1_NAME);
	    AddStrWebPage(StrBuf);	
	    AddStrWebPage("class=\"inputbox\" >\r\n");
        AddStrWebPage("</div>\r\n");
        AddStrWebPage("</td></tr>\r\n");
    }

	if (ParWebServPtr->ServCustomCfg.SetUserAddr2Field)
	{
	    AddStrWebPage("<tr><td width=\"40%\" align=\"left\">\r\n");
        AddStrWebPage("<div id=\"address_2_div\" >\r\n");
        AddStrWebPage("<label for=\"address_2_field\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_ORDER_ADDR_2_LINE_ID);
        AddStrWebPage("</label>");
	    if (ParWebServPtr->ServCustomCfg.ReqUserAddr2Field) 
		    AddStrWebPage(" <strong>*</strong>");	
	    AddStrWebPage(" </div>\r\n");
	    AddStrWebPage("</td><td align=\"left\" valign=\"top\">\r\n");
        AddStrWebPage("<div style=\"float:left;\">\r\n");
        AddStrWebPage("<input type=\"text\" id=\"address_2_field\" name=\"address_2\" ");
	    sprintf(StrBuf, "size=\"%d\"", EditFiledSize);
        AddStrWebPage(StrBuf);
	    AddStrWebPage(" value=\"");
	    AddStrWebPage( UserPtr->Contact.Address2);
	    sprintf(StrBuf, "\" maxlength=\"%d\" ", MAX_LEN_ADDR_2_NAME);
	    AddStrWebPage(StrBuf);	
	    AddStrWebPage("class=\"inputbox\" >\r\n");
        AddStrWebPage("</div>\r\n");
        AddStrWebPage("</td></tr>\r\n");
	}

	if (ParWebServPtr->ServCustomCfg.SetUserCityField)
	{
	    AddStrWebPage("<tr><td width=\"40%\" align=\"left\">\r\n");
        AddStrWebPage("<div id=\"city_div\" >\r\n");
        AddStrWebPage("<label for=\"city_field\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_ORDER_CITY_LINE_ID);
        AddStrWebPage("</label>");
	    if (ParWebServPtr->ServCustomCfg.ReqUserCityField) 
		    AddStrWebPage(" <strong>*</strong>");
	    AddStrWebPage(" </div>\r\n");
	    AddStrWebPage("</td><td align=\"left\" valign=\"top\">\r\n");
        AddStrWebPage("<div style=\"float:left;\">\r\n");
        AddStrWebPage("<input type=\"text\" id=\"city_field\" name=\"city\" ");
	    sprintf(StrBuf, "size=\"%d\"", EditFiledSize);
        AddStrWebPage(StrBuf);
	    AddStrWebPage(" value=\"");
	    AddStrWebPage( UserPtr->Contact.City);
	    sprintf(StrBuf, "\" maxlength=\"%d\" ",MAX_LEN_CITY_NAME);
	    AddStrWebPage(StrBuf);	
	    AddStrWebPage("class=\"inputbox\" >\r\n");
        AddStrWebPage("</div>\r\n");
        AddStrWebPage("</td></tr>\r\n");
	}

	if (ParWebServPtr->ServCustomCfg.SetUserZipField)
	{
	    AddStrWebPage("<tr><td width=\"40%\" align=\"left\">\r\n");
        AddStrWebPage("<div id=\"zip_div\" >\r\n");
        AddStrWebPage("<label for=\"zip_field\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_ORDER_INDEX_LINE_ID);
        AddStrWebPage("</label>");
	    if (ParWebServPtr->ServCustomCfg.ReqUserZipField) 
		    AddStrWebPage(" <strong>*</strong>");
	    AddStrWebPage(" </div>\r\n");
	    AddStrWebPage("</td><td align=\"left\" valign=\"top\">\r\n");
        AddStrWebPage("<div style=\"float:left;\">\r\n");
        AddStrWebPage("<input type=\"text\" id=\"zip_field\" name=\"zip\" ");
	    sprintf(StrBuf, "size=\"%d\"", EditFiledSize);
        AddStrWebPage(StrBuf);
	    AddStrWebPage(" value=\"");
	    sprintf(StrBuf, "%d\" maxlength=\"%d\" ", UserPtr->Contact.ZipCode, MAX_LEN_ZIP_CODE);
	    AddStrWebPage(StrBuf);	
	    AddStrWebPage("class=\"inputbox\" >\r\n");
        AddStrWebPage("</div>\r\n");
        AddStrWebPage("</td></tr>\r\n");
	}

	if (ParWebServPtr->ServCustomCfg.SetUserCountryField)
	{
	    AddStrWebPage("<tr><td width=\"40%\" align=\"left\">\r\n");
        AddStrWebPage("<div id=\"country_div\" >\r\n");
        AddStrWebPage("<label for=\"country_field\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_ORDER_COUNTRY_LINE_ID);
        AddStrWebPage("</label>");
	    if (ParWebServPtr->ServCustomCfg.ReqUserCountryField) 
		    AddStrWebPage(" <strong>*</strong>");
	    AddStrWebPage(" </div>\r\n");
	    AddStrWebPage("</td><td align=\"left\" valign=\"top\">\r\n");
        AddStrWebPage("<div style=\"float:left;\">\r\n");
        AddStrWebPage("<select class=\"inputbox\" name=\"country\" size=\"1\"  id=\"country_field\" >\r\n");
        AddStrWebPage("<option value="" >\r\n");
        SetRusTextBuf(NULL, SITE_RUS_SELECT_LINE_ID);
        AddStrWebPage("</option>\r\n");
        AddStrWebPage("<option value=\"0\" selected=\"selected\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_ORDER_RUS_LINE_ID);
        AddStrWebPage("</option>\r\n");
        AddStrWebPage("</select></div>\r\n");
        AddStrWebPage("</td></tr>\r\n");
	}

	if (ParWebServPtr->ServCustomCfg.SetUserLandPhoneField)
	{
	    AddStrWebPage("<tr><td width=\"40%\" align=\"left\">\r\n");
        AddStrWebPage("<div id=\"land_phone_div\" >\r\n");
        AddStrWebPage("<label for=\"land_phone_field\">\r\n");
	    SetRusTextBuf(NULL, SITE_RUS_PSTN_LINE_ID);
        AddStrWebPage("</label>&nbsp;");
        if (ParWebServPtr->ServCustomCfg.ReqUserLandPhoneField) 
            AddStrWebPage("<strong>*</strong>");
        AddStrWebPage("</div>\r\n");
	    AddStrWebPage("</td><td align=\"left\" valign=\"top\">\r\n");
        AddStrWebPage("<div style=\"float:left;\">\r\n");
        AddStrWebPage("<input type=\"text\" id=\"land_phone_field\" name=\"land_phone\" ");
	    sprintf(StrBuf, "size=\"%d\"", EditFiledSize);
        AddStrWebPage(StrBuf);
	    AddStrWebPage(" value=\"");
	    AddStrWebPage( UserPtr->Contact.LandPhone);
	    sprintf(StrBuf, "\" maxlength=\"%d\" ", MAX_LEN_PHONE_NUM);
	    AddStrWebPage(StrBuf);
	    AddStrWebPage("class=\"inputbox\" ></div>\r\n");
        AddStrWebPage("</td></tr>\r\n");
    }

	if ((ParWebServPtr->ServCustomCfg.SetUserMobPhoneField) &&
        (SessionPtr->UserPtr) && (SessionPtr->UserPtr->UserType != UAT_GUEST))
	{
	    AddStrWebPage("<tr><td width=\"40%\" align=\"left\">\r\n");
        AddStrWebPage("<div id=\"mobile_phone_div\" >\r\n");
        AddStrWebPage("<label for=\"mobile_phone_field\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_ORDER_MOB_PHONE_LINE_ID);
        AddStrWebPage("</label>");
	    if (ParWebServPtr->ServCustomCfg.ReqUserMobPhoneField) 
		    AddStrWebPage(" <strong>*</strong>");
	    AddStrWebPage(" </div>\r\n");
	    AddStrWebPage("</td><td align=\"left\" valign=\"top\">\r\n");
        AddStrWebPage("<div style=\"float:left;\">\r\n");
        AddStrWebPage("<input type=\"text\" id=\"mobile_phone_field\" name=\"mobile_phone\" ");
	    sprintf(StrBuf, "size=\"%d\"", EditFiledSize);
        AddStrWebPage(StrBuf);
	    AddStrWebPage(" value=\"");
        if (ParWebServPtr->ServCustomCfg.DemoMode) AddStrWebPage("89211112233");
        else                                       AddStrWebPage( UserPtr->Contact.MobilePhone);
	    sprintf(StrBuf, "\" maxlength=\"%d\" ", MAX_LEN_PHONE_NUM);
	    AddStrWebPage(StrBuf);
	    AddStrWebPage("class=\"inputbox\" ></div>\r\n");
        AddStrWebPage("</td></tr>\r\n");
	}

	if (ParWebServPtr->ServCustomCfg.SetUserFaxField)
	{
	    AddStrWebPage("<tr><td width=\"40%\" align=\"left\">\r\n");
        AddStrWebPage("<div id=\"fax_div\" >\r\n");
        AddStrWebPage("<label for=\"fax_field\">\r\n");
	    SetRusTextBuf(NULL, SITE_RUS_FAX_LINE_ID);
        AddStrWebPage("</label>");
        if (ParWebServPtr->ServCustomCfg.ReqUserFaxField) AddStrWebPage(" <strong>*</strong>");
	    AddStrWebPage(" </div>\r\n");
	    AddStrWebPage("</td><td align=\"left\" valign=\"top\">\r\n");
        AddStrWebPage("<div style=\"float:left;\">\r\n");
        AddStrWebPage("<input type=\"text\" id=\"fax_field\" name=\"fax\" ");
	    sprintf(StrBuf, "size=\"%d\"", EditFiledSize);
        AddStrWebPage(StrBuf);
	    AddStrWebPage(" value=\"");
	    AddStrWebPage( UserPtr->Contact.FaxPhone);
	    sprintf(StrBuf, "\" maxlength=\"%d\" ", MAX_LEN_PHONE_NUM);
	    AddStrWebPage(StrBuf);
	    AddStrWebPage("class=\"inputbox\" ></div>\r\n");
        AddStrWebPage("</td></tr>\r\n");
	}

	AddStrWebPage("<tr><td width=\"40%\" align=\"left\">\r\n");
    AddStrWebPage("<div id=\"email_div\" >\r\n");
    AddStrWebPage("<label for=\"email_field\">E-mail</label>");
	if (ParWebServPtr->ServCustomCfg.ReqUserEmailFiled)
	    AddStrWebPage(" <strong>*</strong>");
	AddStrWebPage("</div>\r\n");
	AddStrWebPage("</td><td align=\"left\" valign=\"top\">\r\n");
    AddStrWebPage("<div style=\"float:left;\">\r\n");
    AddStrWebPage("<input type=\"text\" id=\"email_field\" name=\"email\" ");
	sprintf(StrBuf, "size=\"%d\"", EditFiledSize);
    AddStrWebPage(StrBuf);
	AddStrWebPage(" value=\"");
    if (ParWebServPtr->ServCustomCfg.DemoMode)  AddStrWebPage("ivan.ivanov@gmail.com");
	else                                        AddStrWebPage( UserPtr->Email);
	sprintf(StrBuf, "\" maxlength=\"%d\" ", MAX_LEN_USER_INFO_EMAIL);
	AddStrWebPage(StrBuf);	
	AddStrWebPage("class=\"inputbox\" ></div>\r\n");
    AddStrWebPage("</td></tr>\r\n");

    if (SessionPtr->UserPtr)
    {
        /* Edit flag of enable send MAIL upon alarms */
	    AddStrWebPage("<tr><td width=\"40%\" align=\"left\">\r\n");
        AddStrWebPage("<div id=\"mail_etn_div\" >\r\n");
        AddStrWebPage("<label for=\"mail_etn_field\">\r\n");
	    SetRusTextBuf(NULL, SITE_RUS_SEND_MAIL_ALARM_LINE_ID);
        AddStrWebPage("</label>");
	    AddStrWebPage("</div>\r\n");
	    AddStrWebPage("</td><td align=\"left\" valign=\"top\">\r\n");
        AddStrWebPage("<div style=\"float:left;\">\r\n");
        AddStrWebPage("<input type=\"checkbox\" id=\"mail_etn_field\" name=\"mail_etn\" value=\"1\" ");
        if (((SAMPLE_USER_INFO*)UserPtr->ExtUserInfoPtr)->EventMailNotify) AddStrWebPage(" checked ");
	    AddStrWebPage("class=\"inputbox\" ></div>\r\n");
        AddStrWebPage("</td></tr>\r\n");  
    }

    AddStrWebPage("</table></fieldset></div><br>\r\n");

	SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
	SetHiddenStrParForm(NULL, FormKeySessionId, SessionPtr->SesionIdKey);
	SetHiddenIntParForm(NULL, FormKeyUserId, SessionPtr->UserPtr->UserId);;

	AddStrWebPage("<input type=\"submit\" value=\"");
    SetRusTextBuf(NULL, SITE_RUS_CONTACT_INFO_CHG_LINE_ID);
    AddStrWebPage("\" class=\"button\" onclick=\"return(submitregistration());\" ></div>\r\n");
    AddStrWebPage("</form></div><br>\r\n");
	AddEndPageShopWebPage(BufAnsw, SessionPtr);
}
//---------------------------------------------------------------------------
