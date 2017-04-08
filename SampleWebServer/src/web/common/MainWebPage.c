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
#include "HtmlTemplateParser.h"

extern char OwnerCompany[] ;

extern char KeyUserNameId[];
extern char KeyFormConfirmKey[];
extern char PasswordId[];
extern char SecKeyId[];
extern char ServerHttpAddr[];
extern char ServerVersion[];
extern char KeyFormSessionId[];
extern char KeyFormMenuRefId[];
extern char *EndHtmlPageGenPtr;
extern bool gFileNotFoundFlag;
extern unsigned char gLanguageType;
extern char ThrWebServName[];
extern char WebServerName[];

extern FILE_HASH_CHAR_HOP FileHashHop;
extern READWEBSOCK *ParReadHttpSocketPtr;
extern PARAMWEBSERV *ParWebServPtr;
extern SAMPLE_SERVER_CUSTOM_CONFIG SampleCustomCfg;
//---------------------------------------------------------------------------
void AddBeginPageShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr)
{
	BufAnsw[0] = 0;
	AddPart1ShopWebPage(BufAnsw, SessionPtr);
	AddBeginMiddlePartShopWebPage(BufAnsw, SessionPtr);
}
//---------------------------------------------------------------------------
void CssFileHtmlPageAppend(USER_SESSION *SessionPtr, char *CssFileName)
{
	FILE_HASH_RECORD *FileInfoPtr = NULL;

    FileInfoPtr = FindFileHash(&FileHashHop, BASE_HASH_LIST, CssFileName);
	if (FileInfoPtr)
	{
	    AddStrWebPage("<style type=\"text/css\">\r\n<!--\r\n");
	    SetCssTemlateBody(ParWebServPtr, ParReadHttpSocketPtr, SessionPtr, 
		                  FileInfoPtr, CssFileName);
	    AddStrWebPage("-->\r\n</style>\r\n");
    }
	else
	{
		AddStrWebPage("<link type=\"text/css\" rel=\"stylesheet\" media=\"screen, projection\" href=\"");
	    SetServerHttpAddr(NULL);
        AddStrWebPage(CssFileName);
        AddStrWebPage("\" >\r\n");
    }
}
//---------------------------------------------------------------------------
void CssEnvLoad(USER_SESSION *SessionPtr)
{
	if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)
	{
		if (!ParReadHttpSocketPtr->MobileType)
		{
            CssFileHtmlPageAppend(SessionPtr, "scripts/sample_css_mobile_320.css");
		}
		else
		{
			switch(ParReadHttpSocketPtr->MobileType->AspectWidth)
			{
				case 360:
					CssFileHtmlPageAppend(SessionPtr, "scripts/sample_css_mobile_360.css");
					break;

				case 400:
					CssFileHtmlPageAppend(SessionPtr, "scripts/sample_css_mobile_400.css");
					break;

				default:
					CssFileHtmlPageAppend(SessionPtr, "scripts/sample_css_mobile_320.css");
					break;
			}
		}
	}
	else
	{
		switch(SessionPtr->SpaseType)
		{
			case 2:
				CssFileHtmlPageAppend(SessionPtr, "scripts/sample_css_desc_1280.css");
			    break;

			case 3:
				CssFileHtmlPageAppend(SessionPtr, "scripts/sample_css_desc_1600.css");
			    break;

			case 4:
				CssFileHtmlPageAppend(SessionPtr, "scripts/sample_css_desc_1920.css");
			    break;
				
			default:
			    CssFileHtmlPageAppend(SessionPtr, "scripts/sample_css_desc.css");
				break;
		}
	}
}
//---------------------------------------------------------------------------
void AddPart1ShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr)
{
	char           CmdGenBuf[300];
	BANNER_INFO    *SelBannerPtr = NULL;
	ObjListTask    *SelObjPtr = NULL;

	EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];
	SessionPtr->SecureKey = SecureKeyGen();
    
    ConfirmKeyGen(ParReadHttpSocketPtr->DeviceType, SessionPtr->ConfirmKey);
    SessionPtr->isConfKeySent = false;

    AddStrWebPage("<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.01 Frameset//EN\" \"http://www.w3.org/TR/html4/frameset.dtd\">\r\n");
    AddStrWebPage("<html lang=ru>\r\n<head>\r\n<title>");
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

	if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)
	{
	    AddStrWebPage("<meta name=\"viewport\" content=\"");
		sprintf(CmdGenBuf, "width=%d, ", SessionPtr->MobileScreenWidth);
        AddStrWebPage(CmdGenBuf);
		AddStrWebPage("initial-scale=1.0, maximum-scale=1, user-scalable=1\">\r\n");
	}

	AddStrWebPage("<base href=\"");
	SetServerHttpAddr(NULL);
	AddStrWebPage("\" >\r\n");

	CssEnvLoad(SessionPtr);

	AddStrWebPage("<link rel=\"shortcut icon\" href=\"");
	SetServerHttpAddr(NULL);
    AddStrWebPage("images/favicon.ico\" type=\"image/x-icon\" >\r\n");

    if (ParReadHttpSocketPtr->BotType == BOT_NONE)
    {
	    AddStrWebPage("<script src=\"");
        AddStrWebPage(GetExtFileNameByLocalName("scripts/jquery.js"));
        AddStrWebPage("\" type=\"text/javascript\"></script>\r\n");
    }    
	AddStrWebPage("<script src=\"");    
    AddStrWebPage(GetExtFileNameByLocalName("scripts/sample_lib.js"));
    AddStrWebPage("\" type=\"text/javascript\"></script>\r\n");

	AddStrWebPage("</head>\r\n");

	AddStrWebPage("<body>\r\n");

	if (((ParReadHttpSocketPtr->BotType == BOT_NONE) || 
		 (ParReadHttpSocketPtr->BotType == BOT_YANDEX_METRICA) ||
		 (ParReadHttpSocketPtr->BotType == BOT_YANDEX_DIRECT)) && ((!SessionPtr->UserPtr) ||
		(SessionPtr->UserPtr && SessionPtr->UserPtr->UserType != UAT_ADMIN)))
	{
	    SelObjPtr = (ObjListTask*)GetFistObjectList(&ParWebServPtr->BannerList);
	    while(SelObjPtr)
		{
	        SelBannerPtr = (BANNER_INFO*)SelObjPtr->UsedTask;
			if (SelBannerPtr->Location == BPL_HIDE_AREA_PGBEG)
			{
                AddStrWebPage(SelBannerPtr->BodyPtr);
				AddStrWebPage("\r\n");
			}
		    SelObjPtr = (ObjListTask*)GetNextObjectList(&ParWebServPtr->BannerList);
		}
	}

    if (ParReadHttpSocketPtr->BotType == BOT_NONE)
	{
		if ((SessionPtr->UserPtr) || 
		    (!SessionPtr->UserPtr && 
			 ParWebServPtr->ServCustomCfg.AnonymTimoutExpInfo))
		{
            if (SessionPtr->UserPtr)
            {
                SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr,
		            SessionPtr, "SessionUserTimer.html");
                SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr,
		            SessionPtr, "SysStatusInfo.html");                
            }
            else
            {
                AddStrWebPage("<script type=\"text/javascript\">var UserNotifySessTimeout=true;</script>\r\n");        
                SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr,
		            SessionPtr, "SessionTimer.html");
            }
		}
        else
        {
            AddStrWebPage("<script type=\"text/javascript\">var UserNotifySessTimeout=false;</script>\r\n");
            SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr,
		        SessionPtr, "SessionTimer.html");        
        }
        SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr, SessionPtr, "NumActiveUsers.html");
	}  

	if ((ParReadHttpSocketPtr->DeviceType == SDT_MOBILE) &&
        ((ParReadHttpSocketPtr->MobileType) && 
		 (ParReadHttpSocketPtr->MobileType->DeviceSize == MDT_PHONE)))
	{
	    SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr,
		    SessionPtr, "MenuSelectionMob.html");
	}
	else
	{
	    SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr,
		    SessionPtr, "MenuSelection.html");
	}

	AddStrWebPage("<table width=\"");
	if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)
	{
		switch(SessionPtr->SpaseType)
		{
			case 2:
				AddStrWebPage("1200");
				break;
				
			case 3:
				AddStrWebPage("1500");
				break;
				
			case 4:
				AddStrWebPage("1900");
				break;
				
			default:
				AddStrWebPage("1000");
				break;
		}
	}
	else                                                 
	{
		sprintf(CmdGenBuf, "%d", SessionPtr->MobileScreenWidth);
		AddStrWebPage(CmdGenBuf);
	}
	AddStrWebPage("\" border=\"0\" cellpadding=\"0\" cellspacing=\"0\" align=\"center\">\r\n");
	AddStrWebPage("<tr>\r\n");

	if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)
	{
	    AddStrWebPage("<th class=\"UpLeftHeader\" scope=\"col\">\r\n");
		/* Some information should be shown in up left block */
		AddStrWebPage("</th>\r\n");
	}

	AddStrWebPage("<th scope=\"col\"");
	if (ParReadHttpSocketPtr->DeviceType != SDT_DESCTOP)
	{
		if ((ParReadHttpSocketPtr->MobileType) && 
		    (ParReadHttpSocketPtr->MobileType->DeviceSize == MDT_PAD))
		    AddStrWebPage(" colspan=\"2\"");
	}
    AddStrWebPage(">\r\n");
    
	AddStrWebPage("<div class=\"SiteHeader\"><a href=\"");
	SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageMain);
    SetSessionIdCmdRef(SessionPtr);
	sprintf(CmdGenBuf, ";%s=0", KeyFormMenuRefId);
	AddStrWebPage(CmdGenBuf);
	AddStrWebPage("\"><img src=\"");
	SetServerHttpAddr(NULL);

	if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)
	{
	    if ((ParReadHttpSocketPtr->MobileType) && 
		 (ParReadHttpSocketPtr->MobileType->DeviceSize == MDT_PHONE))
		{
		    switch(ParReadHttpSocketPtr->MobileType->AspectWidth)
			{
			    case 360:
                    AddStrWebPage(ParWebServPtr->ServCustomCfg.ImgBgHeaderM360);
				    break;

			    case 400:
                    AddStrWebPage(ParWebServPtr->ServCustomCfg.ImgBgHeaderM400);
				    break;

			    default:
				    AddStrWebPage(ParWebServPtr->ServCustomCfg.ImgBgHeaderM320);
				    break;
			}
		}
	    else
		{
	        AddStrWebPage(ParWebServPtr->ServCustomCfg.ImgBgHeaderM320);
		}
	}
	else
	{
        AddStrWebPage(ParWebServPtr->ServCustomCfg.ImgBgHeaderDesc);
	}

	AddStrWebPage("\" height=\"140\" width=\"100%\" alt=\"Image\">\r\n");
	AddStrWebPage("<div class=\"header_show_site_name\">");	
	SetRusTextBufName(NULL, (char*)&ParWebServPtr->ShopInfoCfg.Name);
	AddStrWebPage("</div>\r\n");
	AddStrWebPage("<div class=\"header_show_site_descr\">");
    if (gLanguageType == LGT_ENGLISH)	SetRusTextBufName(NULL, "Sample of WEB UI");
    else								SetRusTextBufName(NULL, "Пример WEB ЮИ");
	AddStrWebPage("</div>\r\n");       
	AddStrWebPage("</a></div></th>\r\n");

    if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)
	{
	    AddStrWebPage("<th class=\"UpRightHeader\" scope=\"col\">\r\n");
        SetDateSessionTimeWebPage(BufAnsw, SessionPtr);
		AddStrWebPage("</th>\r\n");
	}

	AddStrWebPage("</tr><tr>\r\n"); /* Go to next grp */

	if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)
	{
        AddStrWebPage("<th class=\"DateAndSessionTime\" align=\"center\" ");
		if ((ParReadHttpSocketPtr->MobileType) && 
		    (ParReadHttpSocketPtr->MobileType->DeviceSize == MDT_PAD))
		    AddStrWebPage("colspan=\"2\"");
		AddStrWebPage("><table cellpadding=\"0\" cellspacing=\"0\">\r\n");
        AddStrWebPage("<tr><td width=\"50%\" align=\"center\">\r\n");
		SetRusTextBuf(NULL, SITE_RUS_TODAY_DATE_LINE_ID);
		AddStrWebPage(":&nbsp;");
		SetDateWebPage();
        AddStrWebPage("</td><td align=\"center\">\r\n");
        if (ParReadHttpSocketPtr->BotType == BOT_NONE)
		{
			if ((SessionPtr->UserPtr) || 
		        (!SessionPtr->UserPtr && 
			     ParWebServPtr->ServCustomCfg.AnonymTimoutExpInfo))
			{
                AddStrWebPage("<div id=\"timer\" align=\"center\"></div>\r\n");
			}
		}        
        AddStrWebPage("</td>\r\n");
        AddStrWebPage("</tr></table>\r\n");        
        AddStrWebPage("</th>\r\n");
        AddStrWebPage("</tr><tr>\r\n");
	}

	AddStrWebPage("<td ");
	if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)
	{
		AddStrWebPage("colspan=\"3\"");
	}
	else if ((ParReadHttpSocketPtr->MobileType) && 
		     (ParReadHttpSocketPtr->MobileType->DeviceSize == MDT_PAD))
	{
	    AddStrWebPage("colspan=\"2\"");
	}
    AddStrWebPage(">");

	AddStrWebPage("<table width=\"100%\" cellpadding=\"0\" cellspacing=\"0\">\r\n");
	AddStrWebPage("<tr><td class=\"NavBarRight\" align=\"center\"><span class=\"pathway\">");
	if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)
	{
	    if (ParReadHttpSocketPtr->MobileType)
		{
		    AddStrWebPage(ParReadHttpSocketPtr->MobileType->DevName);
		}
		else
		{
			AddStrWebPage(" MobileDevice");
		}
	}
	AddStrWebPage("</span></td></tr></table>\r\n");
	AddStrWebPage("</tr><tr>\r\n"); /* Begin left zone */

	if ((ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP) ||
		((SessionPtr->isMainPageReq == true)))
	{
	    AddStrWebPage("<td class=\"MiddleLeft\">\r\n");
		AddStrWebPage("<div class=\"inter_block_spase\"></div>\r\n");
	    SetRegUserExitBtTimeWebPage(BufAnsw, SessionPtr);
        if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)
		{
            if (SessionPtr->UserPtr)
            {
                AddStrWebPage("<div class=\"UpWorkBox\" id=\"state_links_div\">\r\n");
                /* Some information should be shown in Up work box */
			    AddStrWebPage("</div>\r\n<div class=\"inter_block_spase\"></div>\r\n");
            }
			LanguageSelectBlockSet(SessionPtr);
			AddStrWebPage("<div class=\"inter_block_spase\"></div>\r\n");
		}
	    SetMenuWebPage(BufAnsw, SessionPtr);
	    AddRegUserShopWebPage(BufAnsw, SessionPtr);
        AddStrWebPage("</td>\r\n");

	    if ((ParReadHttpSocketPtr->DeviceType == SDT_MOBILE) &&
		    (!ParReadHttpSocketPtr->MobileType) ||
	        ((ParReadHttpSocketPtr->MobileType) && 
		    (ParReadHttpSocketPtr->MobileType->DeviceSize == MDT_PHONE)))
                AddStrWebPage("<td></td></tr><tr>\r\n");
	}
	else
	{
        if ((ParReadHttpSocketPtr->DeviceType == SDT_MOBILE) &&
		    (!ParReadHttpSocketPtr->MobileType) ||
		    ((ParReadHttpSocketPtr->MobileType) && 
		    (ParReadHttpSocketPtr->MobileType->DeviceSize == MDT_PHONE)))
		{

	        AddStrWebPage("<td class=\"MiddleLeft\">\r\n");
			AddStrWebPage("<div class=\"inter_block_spase\"></div>\r\n");
	        SetRegUserExitBtTimeWebPage(BufAnsw, SessionPtr);

			AddStrWebPage("</td>\r\n");
            AddStrWebPage("<td></td></tr><tr>\r\n");
		}
	}
	return;
}
//---------------------------------------------------------------------------
void SetMenuWebPage(char *BufAnsw, USER_SESSION *SessionPtr)
{
	char StrBuf[256];

	if (BufAnsw) EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];

	AddStrWebPage("<div class=\"menu_group\">\r\n");
	AddStrWebPage("<div class=\"title\" id=\"section_2\" >");
	SetRusTextBuf(NULL, SITE_RUS_INFORMATION_LINE_ID);
	AddStrWebPage("</div>\r\n");
	AddStrWebPage("<div class=\"container\" id=\"container_2\">\r\n");

	AddStrWebPage("<a class=\"main_menu\" ");
	sprintf(StrBuf, " id=\"menu_id_%d\" ", 0);
    AddStrWebPage(StrBuf);
	sprintf(StrBuf, "('menu_id_%d', 'menu_text_%d');\" ", 0, 0);
	AddStrWebPage("onMouseOver=\"on_sel_menu_mouseover");
	AddStrWebPage(StrBuf);
	AddStrWebPage("onMouseOut=\"on_sel_menu_mouseout");
	AddStrWebPage(StrBuf);
	AddStrWebPage("onClick=\"on_sel_menu_click");
	AddStrWebPage(StrBuf);
	AddStrWebPage("href=\"");
	SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageMain);
	if ((!SessionPtr->UserPtr) && 
		(ParReadHttpSocketPtr->CookieSessionId > 0) && 
		(ParReadHttpSocketPtr->BotType == BOT_NONE))
	{
	    sprintf(StrBuf, "?%s=0", KeyFormMenuRefId);
	}
	else
	{
	    sprintf(StrBuf, "?%s=%s;%s=0", KeyFormSessionId, 
		    SessionPtr->SesionIdKey, KeyFormMenuRefId);
	}
	AddStrWebPage(StrBuf);
	AddStrWebPage("\"><div class=\"section_name\" ");
	sprintf(StrBuf, " id=\"menu_text_%d\">", 0);
    AddStrWebPage(StrBuf);
    SetRusTextBuf(NULL, SITE_RUS_MAIN_INFO_LINE_ID);
	AddStrWebPage("</div></a>\r\n");

	// Request contact information
	AddComMainMenuItem(GenPageContacts, SITE_RUS_CONTACTS_LINE_ID, SessionPtr, 10);
	
	// Request informatin about server
	AddComMainMenuItem(GenPageAboutServer, SITE_RUS_ABOUT_SERVER_LINE_ID, SessionPtr, 8);

	AddStrWebPage("</div></div>\r\n");

	if (SessionPtr->UserPtr)
	{
		AddStrWebPage("<div class=\"inter_block_spase\"></div>\r\n");
		AddStrWebPage("<div class=\"menu_group\">\r\n");
	    AddStrWebPage("<div class=\"title\" id=\"section_3\" >");
		switch(SessionPtr->UserPtr->UserType)
		{
	        case UAT_ADMIN:
				SetRusTextBuf(NULL, SITE_RUS_ROLE_ADMIN_LINE_ID);
				break;

			default:
				SetRusTextBuf(NULL, SITE_RUS_AUTH_USER_LINE_ID);
				break;
		}
	    AddStrWebPage("</div>\r\n");
	    AddStrWebPage("<div class=\"container\" id=\"container_3\">\r\n");
		switch(SessionPtr->UserPtr->UserType)
		{
			case UAT_ADMIN:
		        /* My contacts - user's menu item */
                if ((ParReadHttpSocketPtr->WebChanId == PRIMARY_WEB_CHAN) || ParWebServPtr->ServCustomCfg.SecondPortInfoEdit)
			        AddSecMainMenuItem(GenPageMyContacts, SITE_RUS_MY_CONTACTS_LINE_ID, SessionPtr, 51);

		        /* Group base request - user's menu item */
			    AddSecMainMenuItem(GenPageGroupDBManage, SITE_RUS_GROUP_BASE_LINE_ID, SessionPtr, 65);

		        /* Bad IPs's database - admin's menu item */
			    AddSecMainMenuItem(GenPageBadIpDBManage, SITE_RUS_BAD_IP_MENU_LINE_ID, SessionPtr, 106);

			    /* List of registered clients - admin's menu item */
			    AddSecMainMenuItem(GenPageRegClientsList, SITE_RUS_SHOP_DB_REG_CLIENTS_LINE_ID, SessionPtr, 107);

		        /* Server's configuration - admin's menu item */
			    AddSecMainMenuItem(GenPageServerConfig, SITE_RUS_SHOP_SERVER_CONFIG_LINE_ID, SessionPtr, 108);

		        /* Server's admin password change - admin's menu item */
			    AddSecMainMenuItem(GenPageAdmPswdChg, SITE_RUS_ADM_PWD_CHG_SEL_LINE_ID, SessionPtr, 109);

		        /* List of active sessions - admin's menu item */
			    AddSecMainMenuItem(GenPageActSessList, SITE_RUS_ACTIVE_SESSIONS_LINE_ID, SessionPtr, 110);

		        /* Server's statistics - admin's menu item */
			    AddSecMainMenuItem(GenPageServStats, SITE_RUS_ADM_STATS_LINE_ID, SessionPtr, 111);
			    break;

            default:
		        /* My contacts - user's menu item */
                if ((ParReadHttpSocketPtr->WebChanId == PRIMARY_WEB_CHAN) || ParWebServPtr->ServCustomCfg.SecondPortInfoEdit)
			         AddSecMainMenuItem(GenPageMyContacts, SITE_RUS_MY_CONTACTS_LINE_ID, SessionPtr, 51);
		}
		AddStrWebPage("</div></div>\r\n");
	}
	AddStrWebPage("<div class=\"inter_block_spase\"></div>\r\n");
	return;
}
//---------------------------------------------------------------------------
void AddBeginMiddlePartShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr)
{
	char *BufPtr = NULL;

	if (BufAnsw) EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];

	AddStrWebPage("<td height=\"150\" align=\"center\" valign=\"top\" bgcolor=\"");
	AddStrWebPage(ParWebServPtr->ServCustomCfg.WorkZoneBgColor);
	AddStrWebPage("\" ");

	if ((ParReadHttpSocketPtr->DeviceType != SDT_DESCTOP) &&
		(SessionPtr->isMainPageReq == false))
	{
		AddStrWebPage("colspan=\"2\" ");
	}

    if ((SessionPtr->UserPtr) && (SessionPtr->UserPtr->UserType == UAT_ADMIN))
	{
        AddStrWebPage(">\r\n");
	}
    else
	{
	    AddStrWebPage("style=\"background-image:url(");
	    SetServerHttpAddr(NULL);
		AddStrWebPage(GetImageNameByKey(WorkZoneBgImageKey));
	    AddStrWebPage(");\">\r\n");
	}
	AddStrWebPage("<table width=\"98%\" border=\"0\" cellspacing=\"0\" cellpadding=\"0\">\r\n");
	AddStrWebPage("<tr><th height=\"10\" colspan=\"3\" valign=\"top\" scope=\"col\"></th></tr>\r\n");
	AddStrWebPage("<tr>\r\n");
	AddStrWebPage("<th width=\"48%\" valign=\"top\" scope=\"col\"></th>\r\n");
	AddStrWebPage("<th width=\"3%\" scope=\"col\">&nbsp;</th>\r\n");
	AddStrWebPage("<th width=\"49%\" valign=\"top\" scope=\"col\"></th>\r\n");
	AddStrWebPage("</tr>\r\n");
 	AddStrWebPage("<tr valign=\"top\">\r\n");
	AddStrWebPage("<td colspan=\"3\">\r\n");
    return;
}
//---------------------------------------------------------------------------
void AddGeneralInfoMainPageShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr)
{
    if (BufAnsw) EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];
	if ((SessionPtr->UserPtr) && 
		(SessionPtr->UserPtr->UserType != UAT_ADMIN) && 
		(SessionPtr->UserPtr->UserType != UAT_GUEST))
    {
        /* Show block of last alarms for Engineer only */
        AddStrWebPage("<div id=\"alarm_list_data_block_id\">\r\n");
        AddStrWebPage("<script language=\"javascript\" type=\"text/javascript\">var AlarmInfoData=[];var LastAlarmWinHeight=");
        switch(ParReadHttpSocketPtr->BrowserType)
        {
            case UBT_CROME:
                AddStrWebPage("360");
                break;
                
            case UBT_FIREFOX:
                AddStrWebPage("404");
                break;
                    
            default:
                AddStrWebPage("374");
                break;
        }        
        AddStrWebPage(";</script>\r\n</div>\r\n");
        AddStrWebPage("<div class=\"last_gen_alarm_list\" id=\"last_gen_alarm_list_div\">Loading...</div>\r\n");    
        
	    SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr,
		    SessionPtr, "LastAlarmsListShow.html");        
        return;
    }
    if (gLanguageType == LGT_ENGLISH)
    {
	    SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr,
            SessionPtr, "GeneralInfoEng.html");
    }
    else
    {
	    SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr,
            SessionPtr, "GeneralInfoRus.html");    
    }
    if (ParWebServPtr->ServCustomCfg.DemoMode)
    {
        AddStrWebPage("<center><span style=\"color: red;font-weight: bold;font-size: 16px;\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_GENINFO_DEMO_MODE_LINE_ID);
        AddStrWebPage("</span></center>\r\n");
    }
	if (ParReadHttpSocketPtr->BotType != BOT_NONE) return;
	return;
}
//---------------------------------------------------------------------------
void AddEndPageShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr)
{
	char BufLine[64];

	if (BufAnsw) EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];

    AddStrWebPage("<br style=\"clear:both\">\r\n");
    AddStrWebPage("</td></tr>\r\n");
	if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)
        AddStrWebPage("<tr><td colspan=\"3\"></td></tr>\r\n");
    else
	{
		if ((ParReadHttpSocketPtr->MobileType) && 
		    (ParReadHttpSocketPtr->MobileType->DeviceSize == MDT_PAD))
            AddStrWebPage("<tr><td colspan=\"2\"></td></tr>\r\n");
		else
			AddStrWebPage("<tr><td></td></tr>\r\n");
	}
    AddStrWebPage("</table></td>\r\n");

	if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)
	{
		AddStrWebPage("<td align=\"center\" valign=\"top\" background=\"");		
		AddStrWebPage(ParWebServPtr->ServCustomCfg.ImgBgSideColumn);
		AddStrWebPage("\">\r\n");
		AddStrWebPage("<div class=\"middle_right_block\">\r\n");
		AddStrWebPage("<div class=\"inter_block_spase\"></div>\r\n");
	    SetRightLocBannerListWebPage(BufAnsw, SessionPtr);
		AddStrWebPage("</div>\r\n");
	} /* End for desctop type */

	AddStrWebPage("</td></tr>\r\n"); /* End middle zone, start small end zone */

	AddStrWebPage("<tr align=\"center\">\r\n");
	AddStrWebPage("<td class=\"down_mail_block\" ");
	if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)
	{
		AddStrWebPage("colspan=\"3\"");
	}
	else
	{
		if ((ParReadHttpSocketPtr->MobileType) && 
		    (ParReadHttpSocketPtr->MobileType->DeviceSize == MDT_PAD))	
		    AddStrWebPage("colspan=\"2\"");
	}
	AddStrWebPage("><a href=\"mailto:");
    AddStrWebPage("sample@mail"); // Mail address for site support communication.
    AddStrWebPage("\">Web-designer</a></td>\r\n");

	AddStrWebPage("</tr><tr>\r\n"); /* End small end zone, start down banner zone */

    if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)
	{
		AddStrWebPage("<td><div class=\"down_left_block\"></div></td>\r\n");
	    AddStrWebPage("<td height=\"70\" ");
	}
	else
	{
        sprintf(BufLine, "<td width=\"%d\" ", SessionPtr->MobileScreenWidth);
	    AddStrWebPage(BufLine);
		AddStrWebPage("height=\"70\" ");
		if ((ParReadHttpSocketPtr->MobileType) && 
		    (ParReadHttpSocketPtr->MobileType->DeviceSize == MDT_PAD))	
		    AddStrWebPage("colspan=\"2\"");
	}
	AddStrWebPage(" align=\"center\" valign=\"top\" bgcolor=\"");
	AddStrWebPage(ParWebServPtr->ServCustomCfg.DownCentrBgColor);
	AddStrWebPage("\" background=\"");
	AddStrWebPage(ParWebServPtr->ServCustomCfg.ImgBgDownCentr);
	AddStrWebPage("\">\r\n");
	SetDownLocBannerListWebPage(BufAnsw, SessionPtr);
	AddStrWebPage("</td>\r\n");

    if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)
		AddStrWebPage("<td><div class=\"down_right_block\"></div></td>\r\n");

	AddStrWebPage("</tr>\r\n");
	AddStrWebPage("</table>\r\n");
	AddStrWebPage("</body>\r\n");
	AddStrWebPage("</html>\r\n");
	return;
}
//---------------------------------------------------------------------------
void SetNumActiveUsersPage()
{
}
//---------------------------------------------------------------------------
