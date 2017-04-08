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
extern char *EndHtmlPageGenPtr;
extern PARAMWEBSERV *ParWebServPtr;
extern READWEBSOCK *ParReadHttpSocketPtr;

static void SetGoogleMapLocation();
static void SetShopInfoMetaData();
//---------------------------------------------------------------------------
void ShowContactsShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	char StrLineBuf[64];

    SetOriginalRusTextBuf(ParWebServPtr->UserTitle, SITE_RUS_CONTACTS_LINE_ID);

	EndHtmlPageGenPtr = ParWebServPtr->UserMetaData;
	*EndHtmlPageGenPtr = 0;

    AddStrWebPage("<meta name=\"description\" content=\"");
    SetOriginalRusTextBuf(NULL, SITE_RUS_CONTACT_PAGE_DESCR_LINE_ID);
	AddStrWebPage(" ");
    SetRusTextBufName(NULL, (unsigned char*)ParWebServPtr->ShopInfoCfg.Name);
	SetOriginalRusTextBuf(NULL, SITE_RUS_SHOP_CONT_INF2_LINE_ID);
	AddStrWebPage(" ");

	sprintf(StrLineBuf, "%d, ", ParWebServPtr->ShopInfoCfg.ZipCode);
    AddStrWebPage(StrLineBuf);
	if (strlen(ParWebServPtr->ShopInfoCfg.Region) > 0)
	{
	    SetRusTextBufName(NULL, (unsigned char*)ParWebServPtr->ShopInfoCfg.Region);
        AddStrWebPage(", ");
	}
    SetRusTextBufName(NULL, (unsigned char*)ParWebServPtr->ShopInfoCfg.City);
    AddStrWebPage(", ");
	SetRusTextBufName(NULL, (unsigned char*)ParWebServPtr->ShopInfoCfg.Address);
    AddStrWebPage(", ");

	if (strlen(ParWebServPtr->ShopInfoCfg.LandPhone) > 0)
	{
	    SetOriginalRusTextBuf(NULL, SITE_RUS_CFG_SHOP_LAND_PHONE_LINE_ID);
        AddStrWebPage(": ");
        SetRusTextBufName(NULL, (unsigned char*)ParWebServPtr->ShopInfoCfg.LandPhone);
		AddStrWebPage(", ");
	}
	SetOriginalRusTextBuf(NULL, SITE_RUS_CFG_SHOP_MOB_PHONE1_LINE_ID);
    AddStrWebPage(": ");
    SetRusTextBufName(NULL, (unsigned char*)ParWebServPtr->ShopInfoCfg.MobilePhone1);
	AddStrWebPage(", ");
	if (strlen(ParWebServPtr->ShopInfoCfg.MobilePhone2) > 0)
	{
	    SetOriginalRusTextBuf(NULL, SITE_RUS_CFG_SHOP_MOB_PHONE2_LINE_ID);
        AddStrWebPage(": ");
        SetRusTextBufName(NULL, (unsigned char*)ParWebServPtr->ShopInfoCfg.MobilePhone2);
		AddStrWebPage(", ");
	}
	if (strlen(ParWebServPtr->ShopInfoCfg.FaxPhone) > 0)
	{
 	    SetOriginalRusTextBuf(NULL, SITE_RUS_CFG_SHOP_FAX_PHONE_LINE_ID);
        AddStrWebPage(": ");
        SetRusTextBufName(NULL, ParWebServPtr->ShopInfoCfg.FaxPhone);
		AddStrWebPage(", ");
	}
    if ((ParReadHttpSocketPtr->WebChanId == PRIMARY_WEB_CHAN) || 
        ParWebServPtr->ServCustomCfg.SecondPortInfoEdit)
    {
	    AddStrWebPage("E-mail: ");
	    AddStrWebPage(ParWebServPtr->MailWorker.MailClientCfg.MailFrom);
	    AddStrWebPage(".");
    }
	AddStrWebPage("\" >\r\n");
	AddStrWebPage("<meta name=\"keywords\" content=\"");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.Name);
	AddStrWebPage(", ");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.Region);
    AddStrWebPage(", ");
    AddStrWebPage(ParWebServPtr->ShopInfoCfg.City);
	AddStrWebPage(", ");
    AddStrWebPage(ParWebServPtr->ShopInfoCfg.Address);
    AddStrWebPage(", ");
	SetOriginalRusTextBuf(NULL, SITE_RUS_CFG_CONTACT_INF_PHONE_LINE_ID);
    AddStrWebPage(" ");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.MobilePhone1);
	if (strlen(ParWebServPtr->ShopInfoCfg.MobilePhone2) > 0)
	{
        AddStrWebPage(", ");
	    SetOriginalRusTextBuf(NULL, SITE_RUS_CFG_CONTACT_INF_PHONE_LINE_ID);
        AddStrWebPage(" ");
	    AddStrWebPage(ParWebServPtr->ShopInfoCfg.MobilePhone2);
	}
	if (strlen(ParWebServPtr->ShopInfoCfg.LandPhone) > 0)
	{
        AddStrWebPage(", ");
	    SetOriginalRusTextBuf(NULL, SITE_RUS_CFG_CONTACT_INF_PHONE_LINE_ID);
        AddStrWebPage(" ");
	    AddStrWebPage(ParWebServPtr->ShopInfoCfg.LandPhone);
	}
	if (strlen(ParWebServPtr->ShopInfoCfg.FaxPhone) > 0)
	{
        AddStrWebPage(", ");
	    SetOriginalRusTextBuf(NULL, SITE_RUS_CFG_SHOP_FAX_PHONE_LINE_ID);
        AddStrWebPage(" ");
	    AddStrWebPage(ParWebServPtr->ShopInfoCfg.FaxPhone);
	}
	AddStrWebPage("\" >\r\n");

	AddBeginPageShopWebPage(BufAnsw, SessionPtr);
    EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];

    AddStrWebPage("<div class=\"componentheading\">\r\n");
	SetRusTextBuf(NULL, SITE_RUS_CONTACTS_LINE_ID);
    AddStrWebPage("</div><br><br>\r\n");

	SetShopInfoMetaData();

	AddStrWebPage("<table table width=\"90%\" cellspacing=\"4\" cellpadding=\"4\" border=\"0\" class=\"table0\" align=\"center\">\r\n");
    AddStrWebPage("<tbody>\r\n");
    AddStrWebPage("<tr>\r\n");
    if (ParReadHttpSocketPtr->DeviceType == SDT_DESCTOP)
	{
	    AddStrWebPage("<td valign=\"top\" align=\"center\">\r\n");
		if (strlen(ParWebServPtr->ServCustomCfg.ImgShopView) > 0)
		{
	        AddStrWebPage("<img alt=\"Shop\" title=\"\" src=\"");
		    AddStrWebPage(ParWebServPtr->ServCustomCfg.ImgShopView);
		    AddStrWebPage("\" width=\"240\" height=\"240\" border=\"5\" >\r\n");
		}
	    SetGoogleMapLocation();
        AddStrWebPage("</td>\r\n");
	}
    AddStrWebPage("<td valign=\"top\">\r\n");
    AddStrWebPage("<p><span style=\"font-size: 14pt;\"><strong><span style=\"font-size: 14pt;\">\r\n");
    SetRusTextBufName(NULL, (unsigned char*)ParWebServPtr->ShopInfoCfg.Name);
    AddStrWebPage("</span></strong></span></p>\r\n");
    AddStrWebPage("<p><br><strong><span style=\"font-family: 'times new roman', 'times'; font-size: 10pt;\">\r\n");

	sprintf(StrLineBuf, "%d, ", ParWebServPtr->ShopInfoCfg.ZipCode);
    AddStrWebPage(StrLineBuf);
	if (strlen(ParWebServPtr->ShopInfoCfg.Region) > 0)
	{
	    SetRusTextBufName(NULL, (unsigned char*)ParWebServPtr->ShopInfoCfg.Region);
        AddStrWebPage(", ");
	}
    SetRusTextBufName(NULL, (unsigned char*)ParWebServPtr->ShopInfoCfg.City);
    AddStrWebPage(", ");
	SetRusTextBufName(NULL, (unsigned char*)ParWebServPtr->ShopInfoCfg.Address);
    AddStrWebPage("</span></strong></p>\r\n");

    AddStrWebPage("<table width=\"100%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");
	if (strlen(ParWebServPtr->ShopInfoCfg.LandPhone) > 0)
	{
        AddStrWebPage("<tr><td width=\"68\"><img alt=\"Land phone\" title=\"");
	    SetRusTextBuf(NULL, SITE_RUS_CFG_SHOP_LAND_PHONE_LINE_ID);		
		AddStrWebPage("\" src=\"");
        SetServerHttpAddr(NULL);
		AddStrWebPage("images/LandPhone.png\" width=\"64\" height=\"64\" border=\"0\" ></td>\r\n");
        AddStrWebPage("<td align=\"left\"><strong><span style=\"font-family: 'times new roman', 'times'; font-size: 10pt;\">");
        SetRusTextBufName(NULL, (unsigned char*)ParWebServPtr->ShopInfoCfg.LandPhone);
	    AddStrWebPage(";</span></strong></td></tr>\r\n");
	}
    AddStrWebPage("<tr><td><img alt=\"Mobile phone 1\" title=\"");
	SetRusTextBuf(NULL, SITE_RUS_CFG_SHOP_MOB_PHONE1_LINE_ID);
	AddStrWebPage("\" src=\"");
	SetServerHttpAddr(NULL);
	AddStrWebPage("images/MobilePhone1.png\" width=\"64\" height=\"64\" border=\"0\" ></td>\r\n");
    AddStrWebPage("<td align=\"left\"><strong><span style=\"font-family: 'times new roman', 'times'; font-size: 10pt;\">");
    SetRusTextBufName(NULL, (unsigned char*)ParWebServPtr->ShopInfoCfg.MobilePhone1);
	AddStrWebPage(";</span></strong></td></tr>\r\n");
	if (strlen(ParWebServPtr->ShopInfoCfg.MobilePhone2) > 0)
	{
        AddStrWebPage("<tr><td><img alt=\"Mobile phone 2\" title=\"");
		SetRusTextBuf(NULL, SITE_RUS_CFG_SHOP_MOB_PHONE2_LINE_ID);
		AddStrWebPage("\" src=\"");
		SetServerHttpAddr(NULL);
		AddStrWebPage("images/MobilePhone2.png\" width=\"64\" height=\"64\" border=\"0\" ></td>\r\n");
        AddStrWebPage("<td align=\"left\"><strong><span style=\"font-family: 'times new roman', 'times'; font-size: 10pt;\">");
        SetRusTextBufName(NULL, (unsigned char*)ParWebServPtr->ShopInfoCfg.MobilePhone2);
	    AddStrWebPage(";</span></strong></td></tr>\r\n");
	}
	if (strlen(ParWebServPtr->ShopInfoCfg.FaxPhone) > 0)
	{
        AddStrWebPage("<tr><td><img alt=\"Fax machine\" title=\"");
		SetRusTextBuf(NULL, SITE_RUS_CFG_SHOP_FAX_PHONE_LINE_ID);
		AddStrWebPage("\" src=\"");
		SetServerHttpAddr(NULL);
		AddStrWebPage("images/FaxMachine.png\" width=\"64\" height=\"64\" border=\"0\" ></td>\r\n");
        AddStrWebPage("<td align=\"left\"><strong><span style=\"font-family: 'times new roman', 'times'; font-size: 10pt;\">");
        SetRusTextBufName(NULL, (unsigned char*)ParWebServPtr->ShopInfoCfg.FaxPhone);
	    AddStrWebPage(";</span></strong></td></tr>\r\n");
	}
    if ((ParReadHttpSocketPtr->WebChanId == PRIMARY_WEB_CHAN) || 
    ParWebServPtr->ServCustomCfg.SecondPortInfoEdit)
    {
	    AddStrWebPage("<tr><td colspan=\"2\">\r\n");
        AddStrWebPage("<strong><span style=\"font-family: 'times new roman', 'times'; font-size: 10pt;\">E-mail:&nbsp;<span style=\"text-decoration: underline;\">");
	    AddStrWebPage(ParWebServPtr->MailWorker.MailClientCfg.MailFrom);
	    AddStrWebPage("</span><a onclick=\"_gaq.push(['_trackEvent', 'mail_sent', document.URL]);\" href=\"");
	    AddStrWebPage(ParWebServPtr->MailWorker.MailClientCfg.MailFrom);
	    AddStrWebPage("\"></a></span></strong></td></tr>\r\n");

	    AddStrWebPage("<tr><td colspan=\"2\">\r\n");
        AddStrWebPage("<span style=\"text-decoration: underline;\"><strong><span style=\"font-family: 'times new roman', 'times'; font-size: 13pt;\"><a rel=\"nofollow\" href=\"");
	    SetServerHttpAddr(NULL);
	    AddStrWebPage(GenPageMain);
        AddStrWebPage("\">");
	    AddStrWebPage(ParWebServPtr->ShopInfoCfg.URL);
	    AddStrWebPage("</a></span></strong></span></td></tr>\r\n");
    }

    AddStrWebPage("</table>\r\n");
    AddStrWebPage("</td></tr>\r\n");
    AddStrWebPage("<tr><td colspan=\"2\" align=\"center\">\r\n");
    if (ParReadHttpSocketPtr->DeviceType == SDT_MOBILE)
	{
		AddStrWebPage("<img alt=\"Shop\" title=\"\" src=\"images/shop.jpg\" width=\"240\" height=\"240\" border=\"5\" >\r\n");
	    SetGoogleMapLocation();
		AddStrWebPage("</td></tr><tr><td colspan=\"2\" align=\"center\">\r\n");
	}
	// Button for return to previous page
    AddStrWebPage("<br><hr><br>\r\n");
    AddStrWebPage("<div><a href=\"");
	SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageMain);
	SetSessionIdCmdRef(SessionPtr);	
	AddStrWebPage("\" ");
	SetReturnNavEnButton();
    AddStrWebPage("</div>\r\n");

	AddStrWebPage("</td></tr>\r\n");
    AddStrWebPage("</tbody></table>\r\n");

	AddEndPageShopWebPage(BufAnsw, SessionPtr);
}
//---------------------------------------------------------------------------
static void SetGoogleMapLocation()
{
	// Show google map shop location
    AddStrWebPage("<p><iframe frameborder=\"5\" width=\"248\" height=\"248\" scrolling=\"no\" src=\"");
	AddStrWebPage("https://maps.google.ru/maps?hl=ru&ie=UTF8&ll=");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.LocLatitude);
	AddStrWebPage(",");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.LocLongitude);
	AddStrWebPage("&spn=0.005063,0.007564&t=m&z=17&vpsrc=6;gl=ru&amp;t=m&amp;source=embed&amp;z=16&amp;iwloc=A&amp;output=embed");
	AddStrWebPage("\" ></iframe>\r\n");
	AddStrWebPage("<br ></p>\r\n");
						
	// Link for open of shop location google map in different window.
    AddStrWebPage("<script type=\"text/javascript\">//<![CDATA[\r\n");
    AddStrWebPage("document.write('<a href=\"javascript:void window.open(\\'");
    AddStrWebPage("https://maps.google.ru/maps?hl=ru&ie=UTF8&ll=");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.LocLatitude);
	AddStrWebPage(",");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.LocLongitude);
	AddStrWebPage("&spn=0.005063,0.007564&t=m&z=17&vpsrc=6;gl=ru&amp;t=m&amp;source=embed&amp;z=16&amp;iwloc=A&amp");
	AddStrWebPage("\\', \\'win2\\', \\'status=no,toolbar=no,scrollbars=yes,titlebar=no,menubar=no,resizable=yes,width=1024,height=600,directories=no,location=no\\');\" title=\"\"> ");
    SetRusTextBuf(NULL,SITE_RUS_SHOP_DETAIL_MAP_LINE_ID);
	AddStrWebPage("</a>');\r\n");
	AddStrWebPage("//]]></script>\r\n");	
	AddStrWebPage("<noscript>\r\n");
	AddStrWebPage("<a href=\"");
    SetServerHttpAddr(NULL);
	AddStrWebPage("\" target=\"_blank\" title=\"\"><img src=\"");
    SetServerHttpAddr(NULL);
	AddStrWebPage("\" width=\"90\" height=\"42\" alt=\"");
	AddStrWebPage("\" border=\"0\"><br>");
	SetRusTextBuf(NULL, SITE_RUS_PICTURE_ZOOM_LINE_ID);
	AddStrWebPage("</a>\r\n");
    AddStrWebPage("</noscript>\r\n");
}
//---------------------------------------------------------------------------
static void SetShopInfoMetaData()
{
    char BufStrLine[32];

    /* Place Mobile phone contact */ 
    AddStrWebPage("<div itemscope itemtype=\"http://schema.org/ClothingStore\">\r\n");
    AddStrWebPage("<meta itemprop=\"name\" content=\"");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.Name);
	AddStrWebPage("\">\r\n");
  	AddStrWebPage("<meta itemprop=\"telephone\" content=\"");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.MobilePhone1);
	AddStrWebPage("\">\r\n");

  	AddStrWebPage("<div itemprop=\"address\" itemscope itemtype=\"http://schema.org/PostalAddress\">\r\n");
    AddStrWebPage("<meta itemprop=\"streetAddress\" content=\"");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.Address);
	AddStrWebPage("\">\r\n");
   	AddStrWebPage("<meta itemprop=\"addressLocality\" content=\"");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.City);
	AddStrWebPage("\">\r\n");
    AddStrWebPage("<meta itemprop=\"addressRegion\" content=\"");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.Region);
	AddStrWebPage("\">\r\n");
	AddStrWebPage("<meta itemprop=\"postalCode\" content=\"");
	sprintf(BufStrLine, "%d", ParWebServPtr->ShopInfoCfg.ZipCode);
	AddStrWebPage(BufStrLine);
	AddStrWebPage("\">\r\n");
    AddStrWebPage("</div>\r\n");

    /* Place address of shop */
	AddStrWebPage("<div itemprop=\"location\" itemscope itemtype=\"http://schema.org/Place\">\r\n");
    AddStrWebPage("<meta itemprop=\"name\" content=\"");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.Name);
	AddStrWebPage("\">\r\n");
  	AddStrWebPage("<meta itemprop=\"telephone\" content=\"");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.MobilePhone1);
	AddStrWebPage("\">\r\n");
  	AddStrWebPage("<div itemprop=\"address\" itemscope itemtype=\"http://schema.org/PostalAddress\">\r\n");
    AddStrWebPage("<meta itemprop=\"streetAddress\" content=\"");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.Address);
	AddStrWebPage("\">\r\n");
   	AddStrWebPage("<meta itemprop=\"addressLocality\" content=\"");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.City);
	AddStrWebPage("\">\r\n");
    AddStrWebPage("<meta itemprop=\"addressRegion\" content=\"");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.Region);
	AddStrWebPage("\">\r\n");
	AddStrWebPage("<meta itemprop=\"postalCode\" content=\"");
	sprintf(BufStrLine, "%d", ParWebServPtr->ShopInfoCfg.ZipCode);
	AddStrWebPage( BufStrLine);
	AddStrWebPage("\">\r\n");
    AddStrWebPage("</div>\r\n");

    /* Place geografical location of shop */
  	AddStrWebPage("<div itemprop=\"geo\" itemscope itemtype=\"http://schema.org/GeoCoordinates\">\r\n");
    AddStrWebPage("<meta itemprop=\"latitude\"  content=\"");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.LocLatitude);
	AddStrWebPage("\">\r\n");
    AddStrWebPage("<meta itemprop=\"longitude\" content=\"");
	AddStrWebPage(ParWebServPtr->ShopInfoCfg.LocLongitude);
	AddStrWebPage("\">\r\n");
	AddStrWebPage("</div>\r\n");

	AddStrWebPage("</div>\r\n");
}
//---------------------------------------------------------------------------
