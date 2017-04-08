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
#include "FileNameMapBase.h"
#include "FileDataHash.h"
#include "ImageNameHash.h"
#include "Interface.h"
#include "HtmlTemplateParser.h"

extern char KeyUserNameId[];
extern char KeyFormSessionId[];
extern char PasswordId[];
extern char SecKeyId[];
extern char ServerHttpAddr[];
extern char ServerVersion[];
extern char *EndHtmlPageGenPtr;
extern unsigned char gLanguageType;
extern char FormKeySessionId[];
extern char FormKeyUserId[];
extern char KeyFormReversMode[];
extern char KeyFormSortType[];
extern char ThrWebServName[];
extern char WebServerName[];
extern READWEBSOCK *ParReadHttpSocketPtr;
extern PARAMWEBSERV *ParWebServPtr;

extern char OwnerCompany[];
extern char SystemName[];
//---------------------------------------------------------------------------
void MainSystemShowWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
    SessionPtr->isMainPageReq = true;
    BufAnsw[0] = 0;
	EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];
	SessionPtr->SecureKey = SecureKeyGen();
    
    AddStrWebPage("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\" \"http://www.w3.org/TR/html4/frameset.dtd\">\r\n");
    AddStrWebPage("<html lang=ru>\r\n");
	AddStrWebPage("<head>\r\n");
	AddStrWebPage("<title>");
	if (strlen(ParWebServPtr->UserTitle) > 0)
	{
		AddStrWebPage(ParWebServPtr->UserTitle);
        AddStrWebPage(" | ");
	}
    AddStrWebPage(ParWebServPtr->ShopInfoCfg.Name);
	AddStrWebPage("</title>\r\n");
    AddStrWebPage("<meta HTTP-EQUIV=\"Cache-Control\" CONTENT=\"no-cache\">\r\n");
	AddStrWebPage("<meta http-equiv=\"Content-Type\" content=\"text/html; charset=windows-1251\" >\r\n");
	if (strlen(ParWebServPtr->UserMetaData) > 0)
	{
		AddStrWebPage(ParWebServPtr->UserMetaData);
	}
	else
	{
	    AddStrWebPage("<meta name=\"description\" content=\"");
	    SetOriginalRusTextBuf(NULL, SITE_RUS_HEAD_2_LINE_ID);
	    AddStrWebPage("\" >\r\n");
	    AddStrWebPage("<meta name=\"keywords\" content=\"");
	    SetOriginalRusTextBuf(NULL, SITE_RUS_HEAD_3_LINE_ID);
	    AddStrWebPage("\" >\r\n");
	}
	AddStrWebPage("<meta name=\"author\" content=\"");
    AddStrWebPage(OwnerCompany);
    AddStrWebPage("\">\r\n");
    AddStrWebPage("<meta name=\"copyright\" content=\"Copyright &copy; 2017 ");
    AddStrWebPage(OwnerCompany);
    AddStrWebPage(" All rights reserved.\">\r\n");
	AddStrWebPage("<meta name=\"Generator\" content=\"");
    AddStrWebPage(WebServerName);
    AddStrWebPage("\">\r\n");
	AddStrWebPage("<meta name=\"robots\" content=\"index, follow\">\r\n");
	AddStrWebPage("<meta name=\"revisit-after\" content=\"4 days\">\r\n");

	AddStrWebPage("<base href=\"");
	SetServerHttpAddr(NULL);
	AddStrWebPage("\" >\r\n");

	CssFileHtmlPageAppend(SessionPtr, "scripts/PublicStatusShow.css");

	AddStrWebPage("<link rel=\"shortcut icon\" href=\"");
	SetServerHttpAddr(NULL);
    AddStrWebPage("images/favicon.ico\" type=\"image/x-icon\" >\r\n");

	AddStrWebPage("<script src=\"");
    AddStrWebPage(GetExtFileNameByLocalName("scripts/jquery.js"));
    AddStrWebPage("\" type=\"text/javascript\"></script>\r\n");
	AddStrWebPage("<script src=\"");    
    AddStrWebPage(GetExtFileNameByLocalName("scripts/sample_lib.js"));
    AddStrWebPage("\" type=\"text/javascript\"></script>\r\n");
    AddStrWebPage("<script language=\"javascript\" type=\"text/javascript\" src=\"scripts/jquery.flot.js\"></script>\r\n");
	AddStrWebPage("</head>\r\n<body>\r\n");


    AddStrWebPage("<div class=\"status_show_main_block\">\r\n");

    /* Add Date & Session time */
	AddStrWebPage("<div class=\"date_time_lang_sel_box\">\r\n");
	AddStrWebPage("<div class=\"ServDateInfZone\" id=\"serv_date_inf_zone_div\">Loading...</div>\r\n");
    LanguageSelectBlockSet(SessionPtr);          
	AddStrWebPage("</div>\r\n");

    /* Add logo information */
    AddStrWebPage("<div class=\"company_logo_info_block\">\r\n");
    AddStrWebPage("<div class=\"company_logo_pic_block\"></div>\r\n");
    AddStrWebPage("<div class=\"company_logo_name_block\">");
    AddStrWebPage(OwnerCompany);
    AddStrWebPage("&nbsp;");
    AddStrWebPage(SystemName);
    AddStrWebPage("</div>\r\n</div>\r\n");
    
    /* Add sample server version */
    AddStrWebPage("<div class=\"system_version_info\" id=\"system_show_version_id\">\r\n");
    SystemVersionSet();
    AddStrWebPage("</div>\r\n");
        
    /* Add exit from view show mode block */    
	AddStrWebPage("<div class=\"user_exit_container\" id=\"div_user_exit_cont\">\r\n");
    AddStrWebPage("<div class=\"user_exit_name\">\r\n");
	SetRusTextBuf(NULL, SITE_RUS_AUTH_USER2_LINE_ID);
	AddStrWebPage(":&nbsp;");
	SetRusTextBufName(NULL, (unsigned char*)&SessionPtr->UserPtr->Name[0]);
    AddStrWebPage("</div>\r\n");
    AddStrWebPage("<div class=\"user_exit_bt_box\">\r\n");
	AddStrWebPage("<form action=\"");
	AddStrWebPage(GenPageUserExitReq);
	AddStrWebPage("\" method=\"post\" name=\"PublicStatusExit\" >\r\n");
    AddStrWebPage("<input type=\"submit\" name=\"Submit\" class=\"button\" value=\"");
	SetRusTextBuf(NULL, SITE_RUS_AUTH_EXIT_LINE_ID);
	AddStrWebPage("\" onclick=\"PublicStatusShowStop();\">\r\n");

	SetHiddenStrParForm(NULL, KeyFormSessionId, SessionPtr->SesionIdKey);
    SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
	SetHiddenStrParForm(NULL, KeyUserNameId, SessionPtr->UserPtr->UserName);

	AddStrWebPage("</form></div>\r\n");
	AddStrWebPage("</div>\r\n");    

    AddStrWebPage("</div>\r\n");

    SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr, 
        SessionPtr, "PublicStatusShow.html");

    AddStrWebPage("<script type=\"text/javascript\">\r\n");
    AddStrWebPage("PublicStatusShowStart();\r\n");
    AddStrWebPage("</script>\r\n");  
             
	AddStrWebPage("</body>\r\n");
	AddStrWebPage("</html>\r\n");    
}
//---------------------------------------------------------------------------
