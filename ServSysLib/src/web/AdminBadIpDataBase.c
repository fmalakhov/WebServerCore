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
#include "BadIpHash.h"
#include "HtmlTemplateParser.h"

extern char ServerHttpAddr[];
extern char KeySessionId[];
extern char KeyUserNameId[];
extern char PasswordId[];
extern char SecKeyId[];
extern char KeyUserId[];

extern READWEBSOCK  *ParReadHttpSocketPtr;
extern PARAMWEBSERV *ParWebServPtr;
extern char *EndHtmlPageGenPtr;
extern ListItsTask  BadIpList;
extern BAD_IP_HASH_OCTET_HOP BadIpHashHop;

extern char KeyFormUserId[];
extern char FormKeySessionId[];
extern char FormKeyUserId[];

#define DB_BADIP_SEL_EDIT_REQ  1
#define DB_BADIP_SAVE_REQ      2
#define DB_BADIP_BD_RELOAD_REQ 3
#define DB_BADIP_ADD_REQ       4
#define DB_BADIP_DEL_REQ       5
#define DB_BADIP_MIN_REQ       DB_BADIP_SEL_EDIT_REQ
#define DB_BADIP_MAX_REQ       DB_BADIP_DEL_REQ

char KeyFormBadIpDbOper[]      = "bdip_oper";
char KeyFormBadIpId[]          = "bdip_id";

char BadIpDbOper[]             = "bdip_oper=";

static void AdminBadIpBasePageGen(char *BufAnsw, USER_SESSION *SessionPtr,
						   char *HttpCmd, bool isIpExist);
//---------------------------------------------------------------------------
void AdminBadIpDataBase(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool       isParseDone = false;
	bool       isIpExist = false; 
	unsigned int TextLen;
    char*      FText = NULL;
	char*      FStrt = NULL;
	BAD_IP_RECORD* SelBadIpRecPtr = NULL;
	BAD_IP_HASH_RECORD* FindBadIpRecPtr = NULL;
	unsigned int i, pars_read, SecKeyForm, UserId, 
		         ParValue, BadIpId, IpAddr1, IpAddr2, IpAddr3,
				 IpAddr4, IPAddrToList, DbOperId;
	ObjListTask  *SelObjPtr = NULL;
	char         *TextConvertBufPtr = NULL;
	BAD_IP_RECORD EditBadIp;
	unsigned char *PictureDirPtr = NULL;
	char          *PrevPicPtr = NULL;

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
		
		i = FindCmdRequest(FText, BadIpDbOper);
		if (i != -1)
		{
		    pars_read = sscanf(&HttpCmd[i], "%d", &DbOperId);
		    if (!pars_read) break;
			if ((DbOperId < DB_BADIP_MIN_REQ) || (DbOperId > DB_BADIP_MAX_REQ)) break;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
            switch(DbOperId)
		    {
				case DB_BADIP_SEL_EDIT_REQ:
		            i = FindCmdRequest(FText, KeyFormBadIpId);
					if (i == -1) break;
					FText = ParseParForm( &FText[i] );
                    if (!FText) break;
					TextLen = strlen(FText);
					if (!TextLen) break;
		            pars_read = sscanf(FText, "%d", &BadIpId);
		            if (!pars_read) break;
		            FText = FStrt;
		            strcpy(FText,HttpCmd);
					SessionPtr->SelBadIpId = BadIpId;
					isParseDone = true;
					break;

				case DB_BADIP_SAVE_REQ:
					SelBadIpRecPtr = GetBadIpRecById(SessionPtr->SelBadIpId);
					if (!SelBadIpRecPtr) break;
					memcpy(&EditBadIp, SelBadIpRecPtr, sizeof(BAD_IP_RECORD));

					/* parse of first digit bad IP address */
					i = FindCmdRequest(FText, "ipaddr_1");
		            if (i == -1) break;
					FText = ParseParForm( &FText[i] );
                    if (!FText) break;
					TextLen = strlen(FText);
					if (!TextLen) break;
		            pars_read = sscanf(FText, "%d", &ParValue);
		            if (!pars_read) break;
					if ((ParValue < 0) || (ParValue > 255)) break;
					IpAddr1 = (unsigned int)ParValue;
		            FText = FStrt;
		            strcpy(FText,HttpCmd);

					/* parse of second digit bad IP address */
					i = FindCmdRequest(FText, "ipaddr_2");
		            if (i == -1) break;
					FText = ParseParForm( &FText[i] );
                    if (!FText) break;
					TextLen = strlen(FText);
					if (!TextLen) break;
		            pars_read = sscanf(FText, "%d", &ParValue);
		            if (!pars_read) break;
					if ((ParValue < 0) || (ParValue > 255)) break;
					IpAddr2 = (unsigned int)ParValue;
		            FText = FStrt;
		            strcpy(FText,HttpCmd);

					/* parse of third digit bad IP address */
					i = FindCmdRequest(FText, "ipaddr_3");
		            if (i == -1) break;
					FText = ParseParForm( &FText[i] );
                    if (!FText) break;
					TextLen = strlen(FText);
					if (!TextLen) break;
		            pars_read = sscanf(FText, "%d", &ParValue);
		            if (!pars_read) break;
					if ((ParValue < 0) || (ParValue > 255)) break;
					IpAddr3 = (unsigned int)ParValue;
		            FText = FStrt;
		            strcpy(FText,HttpCmd);

					/* parse of fourth digit bad IP address */
					i = FindCmdRequest(FText, "ipaddr_4");
		            if (i == -1) break;
					FText = ParseParForm( &FText[i] );
                    if (!FText) break;
					TextLen = strlen(FText);
					if (!TextLen) break;
		            pars_read = sscanf(FText, "%d", &ParValue);
		            if (!pars_read) break;
					if ((ParValue < 0) || (ParValue > 255)) break;
					IpAddr4 = (unsigned int)ParValue;
		            FText = FStrt;
		            strcpy(FText,HttpCmd);

                    #ifdef WIN32
                        IPAddrToList = IpAddr4;
                        IPAddrToList |= (unsigned int)(IpAddr3 & 0xff) << 8;
						IPAddrToList |= (unsigned int)(IpAddr2 & 0xff) << 16;
						IPAddrToList |= (unsigned int)(IpAddr1 & 0xff) << 24;
				    #else
                      #ifdef _SUN_BUILD_
                        IPAddrToList = IpAddr4;
                        IPAddrToList |= (unsigned int)(IpAddr3 & 0xff) << 8;
						IPAddrToList |= (unsigned int)(IpAddr2 & 0xff) << 16;
						IPAddrToList |= (unsigned int)(IpAddr1 & 0xff) << 24;
                      #else
                        IPAddrToList = IpAddr1;
                        IPAddrToList |= (unsigned int)(IpAddr2 & 0xff) << 8;
						IPAddrToList |= (unsigned int)(IpAddr3 & 0xff) << 16;
						IPAddrToList |= (unsigned int)(IpAddr4 & 0xff) << 24;   
                      #endif
                    #endif
					FindBadIpRecPtr = FindBadIpHash(&BadIpHashHop, IPAddrToList);
					if (!FindBadIpRecPtr)
					{
                        if (RemBadIpHash(&BadIpHashHop, SelBadIpRecPtr->IpAddr))
						{
					        EditBadIp.IpAddr = IPAddrToList;
					        memcpy(SelBadIpRecPtr, &EditBadIp, sizeof(BAD_IP_RECORD));
							if (!AddBadIpHash(&BadIpHashHop, SelBadIpRecPtr))
							{
								printf("Fail to add new IP to list of bad IPs\n");
							}
					        BadIpDbSave();
						}
						else
						{
							printf("Failed to remove IP address from list of bad IPs\n");
						}
					    SessionPtr->SelBadIpId = 0;
					}
					else
					{
						isIpExist = true;
					}
					isParseDone = true;
					break;

				case DB_BADIP_BD_RELOAD_REQ:
					BadIPListClear();
					BadIPListLoad();
					SessionPtr->SelBadIpId = 0;
					isParseDone = true;
					break;

				case DB_BADIP_ADD_REQ:
					SelBadIpRecPtr = BadIpDbAddItem();
					if (SelBadIpRecPtr)
					{
						BadIpDbSave();
						SessionPtr->SelBadIpId = SelBadIpRecPtr->IpAddrId;
					    isParseDone = true;
					}
					break;

				case DB_BADIP_DEL_REQ:
					SelBadIpRecPtr = GetBadIpRecById(SessionPtr->SelBadIpId);
					if (!SelBadIpRecPtr) break;
                    if (RemBadIpHash(&BadIpHashHop, SelBadIpRecPtr->IpAddr))
					{
                        RemStructList(&BadIpList, (ObjListTask*)SelBadIpRecPtr->ObjPtr);
						FreeMemory(SelBadIpRecPtr);
					    BadIpDbSave();
					}
                    else
					{
	                    printf("Fail to remove IP from list of bad IPs\n");
					}
					SessionPtr->SelBadIpId = 0;
                    isParseDone = true;
					break;

				default:
					break;
		    }
		}
		else
		{
			SessionPtr->SelBadIpId = 0;
		    isParseDone = true;
		}
		break;
	}
    if (isParseDone)
	{
		AdminBadIpBasePageGen(BufAnsw, SessionPtr, HttpCmd, isIpExist);
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
static void AdminBadIpBasePageGen(char *BufAnsw, USER_SESSION *SessionPtr,
						   char *HttpCmd, bool isIpExist)
{
	ObjListTask   *SelObjPtr = NULL;
	BAD_IP_RECORD *BadIpPtr = NULL;
	BAD_IP_RECORD *EditBadIpPtr = NULL;
	char         StrBuf[128];
	unsigned int NumItems = 0;
	unsigned int IpAddr1, IpAddr2, IpAddr3, IpAddr4;
	int          index = 1;

    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
    EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];
    
	// Script for item for edit selection support
	SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr,
        SessionPtr, "SelButtonScript.html");

    AddStrWebPage("<script language=\"javascript\" type=\"text/javascript\">//<![CDATA[\r\n");
    AddStrWebPage("function on_submit_badip_record() {\r\n");
    AddStrWebPage("var form = document.BadIpEditForm;\r\n");
    AddStrWebPage("var r = new RegExp(\"[\\<|\\>|\\\"|'|\\%|\\;|\\(|\\)|\\&|\\+|\\-]\", \"i\");\r\n");

	/* Check first digit of IP address */
	AddStrWebPage("var isvalid = true;\r\n");
    AddStrWebPage("if (form.ip_addr_name_1.value == \"\") {\r\n");
	AddStrWebPage("document.getElementById('ip_addr_div').style.color = \"red\";\r\n");
	AddStrWebPage("isvalid = false;}\r\n");
	AddStrWebPage("else {document.getElementById('ip_addr_div').style.color = \"white\";}\r\n");

	/* Check second digit of IP address */
	AddStrWebPage("if (isvalid == true) {\r\n");
    AddStrWebPage("if (form.ip_addr_name_2.value == \"\") {\r\n");
	AddStrWebPage("document.getElementById('ip_addr_div').style.color = \"red\";\r\n");
	AddStrWebPage("isvalid = false;}\r\n");
	AddStrWebPage("else {document.getElementById('ip_addr_div').style.color = \"white\";}\r\n}\r\n");

	/* Check third digit of IP address */
	AddStrWebPage("if (isvalid == true) {\r\n");
    AddStrWebPage("if (form.ip_addr_name_3.value == \"\") {\r\n");
	AddStrWebPage("document.getElementById('ip_addr_div').style.color = \"red\";\r\n");
	AddStrWebPage("isvalid = false;}\r\n");
	AddStrWebPage("else {document.getElementById('ip_addr_div').style.color = \"white\";}\r\n}\r\n");

	/* Check forth digit of IP address */
	AddStrWebPage("if (isvalid == true) {\r\n");
    AddStrWebPage("if (form.ip_addr_name_4.value == \"\") {\r\n");
	AddStrWebPage("document.getElementById('ip_addr_div').style.color = \"red\";\r\n");
	AddStrWebPage("isvalid = false;}\r\n");
	AddStrWebPage("else {document.getElementById('ip_addr_div').style.color = \"white\";}\r\n}\r\n");

	AddStrWebPage("if(!isvalid) {\r\n");
    AddStrWebPage("alert( \"");
    SetOriginalRusTextBuf(NULL, SITE_RUS_ORDER_5_LINE_ID);
    AddStrWebPage("\");\r\n");
	AddStrWebPage("return false;\r\n");
    AddStrWebPage("} else {\r\n");
	AddStrWebPage("return true;}\r\n");
    AddStrWebPage("}\r\n");
    AddStrWebPage("</script>\r\n");

	// Form header
    AddStrWebPage("<h3><center>\r\n");
    SetRusTextBuf(NULL, SITE_RUS_BADIP_BASE_LINE_ID);
    AddStrWebPage("</center></h3><br >\r\n");

	if (isIpExist)
	{
        AddStrWebPage("<h4><center>\r\n");
        SetRusTextBuf(NULL, SITE_RUS_IP_ALREADY_EXIST_BASE_LINE_ID);
        AddStrWebPage("</center></h4><br >\r\n");
	}

    AddStrWebPage("<table width=\"100%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");
    AddStrWebPage("<tr align=\"center\">\r\n");

    AddStrWebPage("<td></td>\r\n");

	// Database of bad IPs reload request button
    AddStrWebPage("<td><form action=\"");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageBadIpDBManage);
	AddStrWebPage("\" method=\"post\" name=\"BaseBadIPReloadForm\">\r\n");

	SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
	SetHiddenStrParForm(NULL, FormKeySessionId, SessionPtr->SesionIdKey);
	SetHiddenIntParForm(NULL, FormKeyUserId, SessionPtr->UserPtr->UserId);
	SetHiddenIntParForm(NULL, KeyFormBadIpDbOper, DB_BADIP_BD_RELOAD_REQ);

	AddStrWebPage("<input type=\"submit\" value=\"");
    SetRusTextBuf(NULL, SITE_RUS_SHOP_DB_RELOAD_LINE_ID);
    AddStrWebPage("\" class=\"button\" onclick=\"\" >\r\n");
	AddStrWebPage("</form></td>\r\n");

	// New bad IP record create request button
    AddStrWebPage("<td><form action=\"");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageBadIpDBManage);
	AddStrWebPage("\" method=\"post\" name=\"BadIpCreateForm\">\r\n");

	SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
	SetHiddenStrParForm(NULL, FormKeySessionId, SessionPtr->SesionIdKey);
	SetHiddenIntParForm(NULL, FormKeyUserId, SessionPtr->UserPtr->UserId);
	SetHiddenIntParForm(NULL, KeyFormBadIpDbOper, DB_BADIP_ADD_REQ);

	AddStrWebPage("<input type=\"submit\" value=\"");
    SetRusTextBuf(NULL, SITE_RUS_BADIP_DB_NEW_BR_LINE_ID);
    AddStrWebPage("\" class=\"button\" onclick=\"\" >\r\n");
	AddStrWebPage("</form></td>\r\n");
	AddStrWebPage("</tr></table>\r\n");

	if (!BadIpList.Count)
	{
		AddStrWebPage("<center><h2>\r\n");
		SetRusTextBuf(NULL, SITE_RUS_BADIP_DB_EMPTY_LINE_ID);
		AddStrWebPage("</h2></center>\r\n");
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
		return;
	}

    AddStrWebPage("<table width=\"90%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\" align=\"center\">\r\n");
    AddStrWebPage("<tr align=\"left\" class=\"sectiontableheader\">\r\n");
	AddStrWebPage("<th>Nn</th>\r\n");
    AddStrWebPage("<th>");
	SetRusTextBuf(NULL, SITE_RUS_IP_ADDRESS_LINE_ID);
    AddStrWebPage("</th>\r\n");
    AddStrWebPage("<th>");
	SetRusTextBuf(NULL, SITE_RUS_TRY_ACCESS_LINE_ID);
    AddStrWebPage("</th>\r\n");
	AddStrWebPage("<th>");
	SetRusTextBuf(NULL, SITE_RUS_SERV_DB_OPER_LINE_ID);
	AddStrWebPage("</th></tr>\r\n");
	index = 1;
	SelObjPtr = (ObjListTask*)GetFistObjectList(&BadIpList);
	while(SelObjPtr)
	{
	    BadIpPtr = (BAD_IP_RECORD*)SelObjPtr->UsedTask;
        AddStrWebPage("<tr valign=\"top\" class=\"sectiontableentry1\">\r\n");

	    /* Show number of IP address in list */
		sprintf(StrBuf, "<td>%d</td>\r\n", index);
		AddStrWebPage(StrBuf);

		/* Show blocked IP address */
		AddStrWebPage("<td>");
		SetIpAddrToString(StrBuf, BadIpPtr->IpAddr);
		AddStrWebPage(StrBuf);
		AddStrWebPage("</td>\r\n");

		/* Show number of trys from blocked IP address */
		sprintf(StrBuf, "<td>%d</td>\r\n", BadIpPtr->TryAccessCount);
		AddStrWebPage( StrBuf);

	    // Show button for record edit
		AddStrWebPage("<td align=\"center\" valign=\"center\"><a href=\"");
		AddStrWebPage(GenPageBadIpDBManage);
	    sprintf(StrBuf, "?%s=%s;", FormKeySessionId, SessionPtr->SesionIdKey);
	    AddStrWebPage(StrBuf);
		sprintf(StrBuf, "%s=%d;", KeyFormBadIpDbOper, DB_BADIP_SEL_EDIT_REQ);
		AddStrWebPage(StrBuf);
	    sprintf(StrBuf, "%s=%d;", SecKeyId, SessionPtr->SecureKey);
	    AddStrWebPage(StrBuf);
	    sprintf(StrBuf, "%s=%d;", FormKeyUserId, SessionPtr->UserPtr->UserId);
	    AddStrWebPage(StrBuf);
		sprintf(StrBuf, "%s=%d", KeyFormBadIpId, BadIpPtr->IpAddrId);
	    AddStrWebPage(StrBuf);
		sprintf(StrBuf, "\" onMouseOver=\"hiLite('BtSelItemEdit%d','BtSelItemEditS','1');return true\" ", index);
		AddStrWebPage(StrBuf);
		if (BadIpPtr->IpAddrId == SessionPtr->SelBadIpId)
		{
			sprintf(StrBuf, "\" onMouseOut=\"hiLite('BtSelItemEdit%d','BtSelItemEditA','0');return true\" ", index);
			EditBadIpPtr = BadIpPtr;
		}
		else
		{
		    sprintf(StrBuf, "\" onMouseOut=\"hiLite('BtSelItemEdit%d','BtSelItemEditP','0');return true\" ", index);
		}
		AddStrWebPage(StrBuf);
		sprintf(StrBuf, "\" onClick=\"hiLite('BtSelItemEdit%d','BtSelItemEditD','1');return true\">\r\n", index);
		AddStrWebPage(StrBuf);
		sprintf(StrBuf, "<img name = \"BtSelItemEdit%d\" ", index);
		AddStrWebPage(StrBuf);
		if (BadIpPtr->IpAddrId == SessionPtr->SelBadIpId)
		{
	        AddStrWebPage("src=\"images/BtSelItemEditAct.png\"");
		}
		else
		{
		    AddStrWebPage("src=\"images/BtSelItemEditPas.png\"");
		}
		AddStrWebPage(" border=\"0\" width=\"20\" height=\"20\" ALT=\"Select item for edit.\"></a></td>\r\n");
        AddStrWebPage("</tr>\r\n");
		index++;
		SelObjPtr = (ObjListTask*)GetNextObjectList(&BadIpList);
	}
    AddStrWebPage("</table>\r\n");

	if ((SessionPtr->SelBadIpId > 0) && EditBadIpPtr)
	{
		AddStrWebPage("<table width=\"90%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\" align=\"center\">\r\n");
        AddStrWebPage("<tr><td>\r\n");

	    AddStrWebPage("<fieldset>\r\n<legend><span class=\"sectiontableheader\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_BADIP_DB_EDIT_LINE_ID);
        AddStrWebPage("</span></legend>\r\n");

        AddStrWebPage("<form action=\"");
		AddStrWebPage(GenPageBadIpDBManage);
		AddStrWebPage("\" method=\"post\" name=\"BadIpEditForm\">\r\n");

		AddStrWebPage("<table width=\"100%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");
		AddStrWebPage("<tr align=\"left\"><td>\r\n");

		/* Ip address field edit */
#ifdef WIN32    
        IpAddr1 = (int)((EditBadIpPtr->IpAddr&0xff000000)>>24);
        IpAddr2 = (int)((EditBadIpPtr->IpAddr&0x00ff0000)>>16);
        IpAddr3 = (int)((EditBadIpPtr->IpAddr&0x0000ff00)>>8);
	    IpAddr4 = (int)(EditBadIpPtr->IpAddr& 0x000000ff);
#else
    #ifdef _SUN_BUILD_
        IpAddr1 = (int)((EditBadIpPtr->IpAddr&0xff000000)>>24);
        IpAddr2 = (int)((EditBadIpPtr->IpAddr&0x00ff0000)>>16);
        IpAddr3 = (int)((EditBadIpPtr->IpAddr&0x0000ff00)>>8);
	    IpAddr4 = (int)(EditBadIpPtr->IpAddr&0x000000ff);
    #else
        IpAddr1 = (int)(EditBadIpPtr->IpAddr& 0x000000ff); 
	    IpAddr2 = (int)((EditBadIpPtr->IpAddr&0x0000ff00)>>8);
	    IpAddr3 = (int)((EditBadIpPtr->IpAddr&0x00ff0000)>>16);
	    IpAddr4 = (int)((EditBadIpPtr->IpAddr&0xff000000)>>24);
    #endif
#endif
		AddStrWebPage("<div id=\"ip_addr_div\" ");
		AddStrWebPage(InputLineStyle);
		AddStrWebPage(">\r\n");
        AddStrWebPage("<label for=\"ip_addr_name\">");
        SetRusTextBuf(NULL, SITE_RUS_IP_ADDRESS_LINE_ID);
        AddStrWebPage(":</label></div>\r\n");
        AddStrWebPage("<input type=\"text\" id=\"ip_addr_name_1\" name=\"ipaddr_");
	    AddStrWebPage("1\" class=\"inputbox\" size=\"3\" ");
		sprintf(StrBuf, "value=\"%d\" maxlength=\"3\" >&nbsp;<b>.</b>\r\n", IpAddr1);
	    AddStrWebPage( StrBuf);	

        AddStrWebPage("<input type=\"text\" id=\"ip_addr_name_2\" name=\"ipaddr_");
	    AddStrWebPage("2\" class=\"inputbox\" size=\"3\" ");
		sprintf(StrBuf, "value=\"%d\" maxlength=\"3\" >&nbsp;<b>.</b>\r\n", IpAddr2);
	    AddStrWebPage( StrBuf);	

        AddStrWebPage("<input type=\"text\" id=\"ip_addr_name_3\" name=\"ipaddr_");
	    AddStrWebPage("3\" class=\"inputbox\" size=\"3\" ");
		sprintf(StrBuf, "value=\"%d\" maxlength=\"3\" >&nbsp;<b>.</b>\r\n", IpAddr3);
	    AddStrWebPage( StrBuf);	

        AddStrWebPage("<input type=\"text\" id=\"ip_addr_name_4\" name=\"ipaddr_");
	    AddStrWebPage("4\" class=\"inputbox\" size=\"3\" ");
		sprintf(StrBuf, "value=\"%d\" maxlength=\"3\" >&nbsp;\r\n", IpAddr4);
	    AddStrWebPage( StrBuf);	

		AddStrWebPage("</td></tr>\r\n");
		
	    SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
	    SetHiddenStrParForm(NULL, FormKeySessionId, SessionPtr->SesionIdKey);
	    SetHiddenIntParForm(NULL, FormKeyUserId, SessionPtr->UserPtr->UserId);
	    SetHiddenIntParForm(NULL, KeyFormBadIpDbOper, DB_BADIP_SAVE_REQ);

		// Bad IP record save request button
		AddStrWebPage("<tr align=\"center\"><td>\r\n");
        AddStrWebPage("<table width=\"80%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\" align=\"center\">\r\n");
        AddStrWebPage("<tr><td>\r\n");

        AddStrWebPage("<input type=\"submit\" name=\"Submit\" class=\"button\" value=\"");
        SetRusTextBuf(NULL, SITE_RUS_SHOP_DB_ITEM_SAVE_LINE_ID);
		AddStrWebPage("\" onclick=\"return(on_submit_badip_record());\" >\r\n");
		AddStrWebPage("</form>\r\n");

		AddStrWebPage("</td/><td>\r\n");

	    // Bad IP record delete request button
        AddStrWebPage("<form action=\"");
        SetServerHttpAddr(NULL);
	    AddStrWebPage(GenPageBadIpDBManage);
	    AddStrWebPage("\" method=\"post\" name=\"BadIpDeleteForm\">\r\n");

	    SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);
	    SetHiddenStrParForm(NULL, FormKeySessionId, SessionPtr->SesionIdKey);
	    SetHiddenIntParForm(NULL, FormKeyUserId, SessionPtr->UserPtr->UserId);
	    SetHiddenIntParForm(NULL, KeyFormBadIpDbOper, DB_BADIP_DEL_REQ);

	    AddStrWebPage("<input type=\"submit\" value=\"");
        SetRusTextBuf(NULL, SITE_RUS_BADIP_DB_DEL_BT_LINE_ID);
        AddStrWebPage("\" class=\"button\" onclick=\"\" >\r\n");
	    AddStrWebPage("</form>\r\n");

		AddStrWebPage("</td/></tr></table>\r\n");

		AddStrWebPage("</td></tr>\r\n");
		AddStrWebPage("</table>\r\n");
		AddStrWebPage("</fieldset>\r\n");
		AddStrWebPage("</tr></td></table>\r\n");
	}
	AddEndPageShopWebPage(BufAnsw, SessionPtr);
}
//---------------------------------------------------------------------------
