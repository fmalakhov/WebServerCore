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
#include "FileDataHash.h"
#include "HtmlTemplateParser.h"

extern char KeyNameId[];
extern char KeyUserNameId[];
extern char EmailId[];
extern char PasswordId[];
extern char SecKeyId[];
extern char KeySessionId[];
extern char KeyUserId[];
extern char FormKeySessionId[];
extern char FormKeyUserId[];
extern char WebServerName[];

extern PARAMWEBSERV *ParWebServPtr;
extern READWEBSOCK  *ParReadHttpSocketPtr;
extern char *EndHtmlPageGenPtr;
extern char *HashDirArray[];
extern FILE_HASH_CHAR_HOP FileHashHop;
extern unsigned int UpdateRecInHashCount;
extern unsigned int HashEntityCount;
extern unsigned int RecInHashCount;
extern char ThrWebServName[];
extern unsigned char UserBufEncKey[];
extern unsigned int EncKeyLen;
extern WEB_SERVER_CUST_CFG_INFO WebCustomCfg;
extern SAMPLE_SERVER_CUSTOM_CONFIG SampleCustomCfg;
extern const unsigned int HashDirArraySize;

#define SITE_ID_VER_IDENTITY_LEN 2*UINT_PACK_SIZE
#define MAIL_CONFIG_PACK_SIZE 3*UINT_PACK_SIZE + (MAX_LEN_SERVER_NAME+1) + (MAX_LEN_SERVER_EMAIL+1) +\
        (MAX_LEN_SERVER_LOGIN+1) + (MAX_LEN_SERVER_PASSWD+1)
#define MAIL_CONFIG_PACK_SIZE_V2 (MAIL_CONFIG_PACK_SIZE + 1)
#define MAIL_CONFIG_PACK_SIZE_V3 (MAIL_CONFIG_PACK_SIZE_V2 + 1)
#define GENERAL_CONFIG_PACK_SIZE 4*UINT_PACK_SIZE + 2*sizeof(char)
#define SHOP_INFO_CONFIG_PACK_SIZE     UINT_PACK_SIZE + (MAX_LEN_COMPANY_NAME+1) + (MAX_LEN_URL_SERVER+1) +\
	    (MAX_LEN_REGION_NAME+1) + (MAX_LEN_CITY_NAME+1) + (MAX_LEN_ADDR_1_NAME+1) + (4*(MAX_LEN_PHONE_NUM+1)) +\
        (2*(MAX_LOCATION_LEN+1))

#define BANNER_HEADER_SIZE 2+(MAX_LEN_BANNER_NAME+1)+UINT_PACK_SIZE
#define SMS_CONFIG_INFO_LEN 3*UINT_PACK_SIZE + (MAX_LEN_SMS_SERVER_NAME+1) +\
	    (MAX_LEN_SMS_SERVER_ACCESS_ID+1) + (MAX_LEN_SMS_SRC_LEN+1) + 2 + (MAX_LEN_PROXY_ADDR+1)
#define SMS_SENT_PHONE_INFO_LEN 1 + (MAX_LEN_SMS_PHONE_NUM+1)

#define MAX_BANNERS_LIST         32
#define CURR_CONFIG_VER          3
#define SITE_CONFIG_CHECK_ID     0xFECCA8BC

static char KeySelEditGrp[]     = "cfg_edit_grp";

static char KeySmtpMailClient[] = "smtp_mail_client";
static char KeySmtpServName[]   = "smtp_serv_name";
static char KeySmtpServPort[]   = "smtp_serv_port";
static char KeySmtpServTime[]   = "smtp_serv_to";
static char KeySmtpIntMail[]    = "smtp_serv_int";
static char KeySmtpServEmail[]  = "smtp_serv_mail";
static char KeySmtpServLogin[]  = "smtp_serv_login";
static char KeySmtpServPasswd[] = "smtp_serv_pwd";
static char KeySmtpServEncode[] = "smtp_serv_encode";
static char FormKeyCfgChgId[]   = "chg_cfg";

static char KeyMaxOpenSess[]    = "gcfg_max_sessions";
static char KeyMaxOpenIpSess[]  = "gcfg_max_sess_ip";
static char KeyCompressEnable[] = "gcfg_en_compress";
static char KeyMinPageSizeCompr[] = "gcfg_min_pgcomp_size";
static char KeyKeepAliveEnable[]  = "gcfg_en_keep_alive";
static char KeyKeepAliveTime[]    = "gcfg_keep_alive_to";

static char KeyShopZipCode[]      = "shop_zip_code";
static char KeyShopUrl[]          = "shop_url";
static char KeyShopRegion[]       = "shop_region";
static char KeyShopCity[]         = "shop_city";
static char KeyShopAddr[]         = "shop_address";
static char KeyShopLandPhone[]    = "shop_land_phone";
static char KeyShopMobPhone1[]    = "shop_mob_phone1";
static char KeyShopMobPhone2[]    = "shop_mob_phone2";
static char KeyShopFax[]          = "shop_fax_phone";
static char KeyShopLatit[]        = "shop_crd_latit";
static char KeyShopLongit[]       = "shop_crd_longit";
static char KeyShopDelivPrice[]   = "shop_deliv_price";
static char KeyShopFreeDelivPrice[] = "shop_frdlv_price";

static char KeyFormBannerId[]     = "banner_index";
static char KeyFormBanLocId[]     = "banner_loc_id";
static char KeyFormBannerBody[]   = "banner_body";
static char KeyFormBannerName[]   = "banner_name";
static char KeyFormBannerActive[] = "banner_active";

static char KeySmsServName[]      = "sms_serv_name";
static char KeySmsServPort[]      = "sms_serv_port";
static char KeySmsServTime[]      = "sms_serv_to";
static char KeyFormSmsPhone[]     = "sms_phone";
static char KeyFormSmsRus[]       = "sms_rus_lang";
static char KeySmsAccessId[]      = "sms_access_id";
static char KeySmsSrcName[]       = "sms_src_name";
static char KeyFormPhoneId[]      = "sms_phone_id";
static char KeyFormProxyFlag[]    = "sms_proxy_flag";
static char KeyFormProxyAddr[]    = "sms_proxy_addr";
static char KeyFormProxyPort[]    = "sms_proxy_port";

unsigned int NameEditGrpList[] = {
SITE_RUS_CFG_GRP_NAME_GENERAL_LINE_ID,
SITE_RUS_CFG_GRP_NAME_MAIL_LINE_ID,
SITE_RUS_CFG_GRP_NAME_SHOP_LINE_ID,
SITE_RUS_CFG_GRP_NAME_BANNER_LINE_ID,
SITE_RUS_CFG_GRP_NAME_SMS_LINE_ID
};

unsigned int BannerLocationNameList[] = {
SITE_RUS_CFG_BLOC_HIDE_LINE_ID,
SITE_RUS_CFG_BLOC_PGDWN_LINE_ID,
SITE_RUS_CFG_BLOC_PGRIGHT_LINE_ID
};

static char* EncodeModeNameList[] = {"None", "SSL", "TLS"}; 

void GenParEditPageShow();
void MailParEditPageShow();
void ShopParEditPageShow();
void BannerParEditPageShow(USER_SESSION *SessionPtr);

void ServerConfigWebPageGen(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void SetKeyParCheckScript(char *Key);
void SetEditStringLineWebPage(char *Key, unsigned int HeadlineId,
	char *SrcLine, int MaxLineLen, int LineEditSize, int HeadWith, bool isPasswd);
void SetEditIntLineWebPage(char *Key, unsigned int HeadlineId,
	int Value, int MaxLineLen, int LineEditSize, int HeadWith, bool isPasswd);
void SetEditBoolLineWebPage(char *Key, unsigned int HeadlineId,
	bool Value, int HeadWith);
void SetHiddenServCfgPar(USER_SESSION *SessionPtr, unsigned int Operation);
void SmsParEditPageShow(USER_SESSION *SessionPtr);
//---------------------------------------------------------------------------
void ServerConfigWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
	bool       isParseDone = false;
	bool       isMainPage = false;
	bool       isShutdown = false;
    char       *FText = NULL;
	char       *FStrt = NULL;
	char       *TextConvertBufPtr = NULL;
	USER_INFO* NewUserPtr = NULL;
    BANNER_INFO *NewBannerPtr = NULL;
	SENTSMSNUM  *NewPhonePtr = NULL;
	SENTSMSNUM  *SelPhonePtr = NULL;
	int        i, j, k, m, pars_read, OperReq;
	unsigned int SecKeyForm, UserId, ReadVal, TextLen;
	unsigned int index = 1;
	BANNER_INFO  *SelBannerPtr = NULL;
	ObjListTask  *SelObjPtr = NULL;
	char         *BodyBufPtr = NULL;
	unsigned int SecKeyLen = 0;
	char         *ParNamePtr = NULL;
	char         ParName[64];
	char         SectionKey[128];

	for(;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
	    FStrt = FText;
        strcpy(FText, HttpCmd);        
        
        i = FindCmdRequest(HttpCmd, "Content-Disposition: form-data; name=\"");
		if (i != -1)
		{
			/* banner data is received - needs to be recoded */
			j = FindCmdRequest(HttpCmd, "------");
			if (j != -1)
			{
				/* Inter section key load */
				SectionKey[SecKeyLen++] = '\r';
				SectionKey[SecKeyLen++] = '\n';
			    j -= 6;
				for(;;)
				{
					if ((HttpCmd[j] == '\r') || (HttpCmd[j] == '\n') ||
						(HttpCmd[j] == 0)) break;
                    SectionKey[SecKeyLen++] = HttpCmd[j++];
					if (SecKeyLen > 127) break;
				}
				SectionKey[SecKeyLen] = 0;
			}
			for (;;)
			{
			    j= 0;
			    ParNamePtr = &ParName[0];
			    while(HttpCmd[i+j] != '"')
				{
                    *FStrt++ = HttpCmd[i+j];
					*ParNamePtr++ = HttpCmd[i+j];
				    j++;
				}
				*ParNamePtr = 0;
                *FStrt++ = '=';
			    k = FindCmdRequest(&HttpCmd[i+j], "\r\n\r\n");
			    if (k == -1) break;
				m = FindCmdRequest(&HttpCmd[i+j+k], SectionKey);
				if (m == -1) break;
				if (strcmp(ParName, KeyFormBannerBody) != 0)
				{
				    memcpy(FStrt, &HttpCmd[i+j+k], m-SecKeyLen);
				    FStrt += (m-SecKeyLen);
				}
				else
				{
                    BodyBufPtr = (char*)AllocateMemory((m-SecKeyLen+1)*sizeof(char));
                    memcpy(BodyBufPtr, &HttpCmd[i+j+k], m-SecKeyLen);
                    BodyBufPtr[m-SecKeyLen] = 0;
                    *FStrt++ = '1';
				}
				*FStrt++ = '&';
			    i = i+j+k+m;
                k = FindCmdRequest(&HttpCmd[i], "Content-Disposition: form-data; name=\"");
			    if (k == -1) break;
				i += k;
			}
			*FStrt = 0;
			strcpy(HttpCmd, FText);
			FStrt = FText;
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

		i = FindCmdRequest(FText, KeyUserId);
		if (i == -1) break;
		pars_read = sscanf(&HttpCmd[i], "%d", &UserId);
		if (!pars_read) break;
		if (UserId != SessionPtr->UserPtr->UserId) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		i = FindCmdRequest(FText, FormKeyCfgChgId);
		if (i != -1)
		{
			/* Check for server DB update request key. */
			FText = ParseParForm( &FText[i] );
            if (!FText) break;
			TextLen = strlen(FText);
			if (!TextLen) break;
		    pars_read = sscanf(FText, "%d", &ReadVal);
		    if (!pars_read) break;
			if ((ReadVal < MIN_CFG_CMD_ID) || (ReadVal > MAX_CFG_CMD_ID)) break;
			OperReq = ReadVal;
		    FText = FStrt;
		    strcpy(FText,HttpCmd);
			if (OperReq == SERV_CFG_CMD_SAVE_PAGE)
			{
			    switch(SessionPtr->SelConfigEditGrpId)
			    {
			        case GEC_MAIL:
                        /* Flag of external mail client usage parse */
			            i = FindCmdRequest(FText, KeySmtpMailClient);
		                if (i != -1)
			            {
			                FText = ParseParForm( &FText[i] );
                            if (!FText) break;
			                TextLen = strlen(FText);
			                if (!TextLen) break;
		                    pars_read = sscanf(FText, "%d", &ReadVal);
		                    if (!pars_read) break;
			                if (ReadVal == 1)
				            {
                                ParWebServPtr->MailWorker.MailClientCfg.MailClientUse = true;
				            }
		                    FText = FStrt;
		                    strcpy(FText,HttpCmd);
			            }
			            else
			            {
                            ParWebServPtr->MailWorker.MailClientCfg.MailClientUse =false;
			            }
                    
			            /* SMTP server name config. parameter parse */
			            if (!StringParParse(FText, (char*)&ParWebServPtr->MailWorker.MailClientCfg.SmtpServerName,
				            KeySmtpServName, MAX_LEN_SERVER_NAME)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* SMTP server encode type config. parameter parse */
			            i = FindCmdRequest(FText, KeySmtpServEncode);
		                if (i == -1) break;
			            FText = ParseParForm( &FText[i] );
                        if (!FText) break;
			            TextLen = strlen(FText);
			            if (!TextLen) break;
		                pars_read = sscanf(FText, "%d", &ReadVal);
		                if (!pars_read) break;
                        if ((ReadVal < MIN_SMTP_ENCODE_TYPE) || (ReadVal > MAX_SMTP_ENCODE_TYPE)) break;
			            ParWebServPtr->MailWorker.MailClientCfg.SmtpEncodeType = ReadVal;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* SMTP server IP port config. parameter parse */
			            i = FindCmdRequest(FText, KeySmtpServPort);
		                if (i == -1) break;
			            FText = ParseParForm( &FText[i] );
                        if (!FText) break;
			            TextLen = strlen(FText);
			            if (!TextLen) break;
		                pars_read = sscanf(FText, "%d", &ReadVal);
		                if (!pars_read) break;
			            ParWebServPtr->MailWorker.MailClientCfg.SmtpIpPort = ReadVal;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* SMTP server timeout config. parameter parse */
			            i = FindCmdRequest(FText, KeySmtpServTime);
		                if (i == -1) break;
			            FText = ParseParForm( &FText[i] );
                        if (!FText) break;
			            TextLen = strlen(FText);
			            if (!TextLen) break;
		                pars_read = sscanf(FText, "%d", &ReadVal);
		                if (!pars_read) break;
			            ParWebServPtr->MailWorker.MailClientCfg.SmtpTimeout = ReadVal;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* SMTP server inter mail delay config. parameter parse */
			            i = FindCmdRequest(FText, KeySmtpIntMail);
		                if (i == -1) break;
			            FText = ParseParForm( &FText[i] );
                        if (!FText) break;
			            TextLen = strlen(FText);
			            if (!TextLen) break;
		                pars_read = sscanf(FText, "%d", &ReadVal);
		                if (!pars_read) break;
			            ParWebServPtr->MailWorker.MailClientCfg.MailSendInt = ReadVal;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Shop e-mail config. parameter parse */
			            if (!StringParParse(FText, (char*)&ParWebServPtr->MailWorker.MailClientCfg.MailFrom,
				            KeySmtpServEmail, MAX_LEN_SERVER_EMAIL)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Mail server login config. parameter parse */
			            if (!StringParParse(FText, (char*)&ParWebServPtr->MailWorker.MailClientCfg.MailLogin,
				            KeySmtpServLogin, MAX_LEN_SERVER_LOGIN)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Mail server passwd config. parameter parse */
			            if (!StringParParse(FText, (char*)&ParWebServPtr->MailWorker.MailClientCfg.MailPasswd,
				            KeySmtpServPasswd, MAX_LEN_SERVER_PASSWD)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);
			            ServerConfigSave(ParWebServPtr);
                        isMainPage = true; 
                        isParseDone = true;                       
			            break;
                        
                    case GEC_GENERAL:
			            /* Max open sessions. parameter parse */
			            i = FindCmdRequest(FText, KeyMaxOpenSess);
		                if (i == -1) break;
			            FText = ParseParForm( &FText[i] );
                        if (!FText) break;
			            TextLen = strlen(FText);
			            if (!TextLen) break;
		                pars_read = sscanf(FText, "%d", &ReadVal);
		                if (!pars_read) break;
			            if ((ReadVal < 5) || (ReadVal > MAX_JOB_WEB_REQ)) break;
			            ParWebServPtr->GeneralCfg.MaxOpenSessions = ReadVal;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Max open sessions per IP. parameter parse */
			            i = FindCmdRequest(FText, KeyMaxOpenIpSess);
		                if (i == -1) break;
			            FText = ParseParForm( &FText[i] );
                        if (!FText) break;
			            TextLen = strlen(FText);
			            if (!TextLen) break;
		                pars_read = sscanf(FText, "%d", &ReadVal);
		                if (!pars_read) break;
			            if ((ReadVal < 2) || (ReadVal > ParWebServPtr->GeneralCfg.MaxOpenSessions)) break;
			            ParWebServPtr->GeneralCfg.MaxSesionPerIP = ReadVal;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Min size of page for compression parameter parse */
			            i = FindCmdRequest(FText, KeyMinPageSizeCompr);
		                if (i == -1) break;
			            FText = ParseParForm( &FText[i] );
                        if (!FText) break;
			            TextLen = strlen(FText);
			            if (!TextLen) break;
		                pars_read = sscanf(FText, "%d", &ReadVal);
		                if (!pars_read) break;
			            if ((ReadVal < 512) || (ReadVal > 8000)) break;
			            ParWebServPtr->GeneralCfg.MinHtmlSizeCompress = ReadVal;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Keep-Alive timeout parameter parse */
			            i = FindCmdRequest(FText, KeyKeepAliveTime);
		                if (i == -1) break;
			            FText = ParseParForm( &FText[i] );
                        if (!FText) break;
			            TextLen = strlen(FText);
			            if (!TextLen) break;
		                pars_read = sscanf(FText, "%d", &ReadVal);
		                if (!pars_read) break;
			            if ((ReadVal < 2) || (ReadVal > 20)) break;
			            ParWebServPtr->GeneralCfg.KeepAliveTimeout = ReadVal;
			            ParWebServPtr->PrimConnWeb.TimeOut = ReadVal;
                        ParWebServPtr->SecondConnWeb.TimeOut = ReadVal;
                        SetKeepAliveTimeout(&ParWebServPtr->PrimConnWeb, ReadVal);
                        SetKeepAliveTimeout(&ParWebServPtr->SecondConnWeb, ReadVal);
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Enable compression parameter parse */
			            i = FindCmdRequest(FText, KeyCompressEnable);
		                if (i != -1)
			            {
			                FText = ParseParForm( &FText[i] );
                            if (!FText) break;
			                TextLen = strlen(FText);
			                if (!TextLen) break;
		                    pars_read = sscanf(FText, "%d", &ReadVal);
		                    if (!pars_read) break;
			                if (ReadVal == 1) ParWebServPtr->GeneralCfg.HtmlPageComprssEnable = true;
		                    FText = FStrt;
		                    strcpy(FText,HttpCmd);
			            }
			            else
			            {
				            ParWebServPtr->GeneralCfg.HtmlPageComprssEnable = false;
			            }

			            /* Enable keep-alive parameter parse */
			            i = FindCmdRequest(FText, KeyKeepAliveEnable);
		                if (i != -1)
			            {
			                FText = ParseParForm( &FText[i] );
                            if (!FText) break;
			                TextLen = strlen(FText);
			                if (!TextLen) break;
		                    pars_read = sscanf(FText, "%d", &ReadVal);
		                    if (!pars_read) break;
			                if (ReadVal == 1)
				            {
					            ParWebServPtr->GeneralCfg.KeepAliveEnable = true;
					            ParWebServPtr->PrimConnWeb.isKeepAlive = true;
                                ParWebServPtr->SecondConnWeb.isKeepAlive = true;
                                ParWebServPtr->SenderWorker.KeepAliveEnable = true;
                                ParWebServPtr->ReaderWorker.KeepAliveEnable = true;
				            }
		                    FText = FStrt;
		                    strcpy(FText,HttpCmd);
			            }
			            else
			            {
				            ParWebServPtr->GeneralCfg.KeepAliveEnable = false;
				            ParWebServPtr->PrimConnWeb.isKeepAlive = false;
                            ParWebServPtr->SecondConnWeb.isKeepAlive = false;
                            ParWebServPtr->SenderWorker.KeepAliveEnable = true;
                            ParWebServPtr->ReaderWorker.KeepAliveEnable = true; 
			            }
			            ServerConfigSave(ParWebServPtr);
                        isMainPage = true;
                        isParseDone = true;                        
			            break;
                        
                    case GEC_SHOP:
			            /* Shop zip code parameter parse */
			            i = FindCmdRequest(FText, KeyShopZipCode);
		                if (i == -1) break;
			            FText = ParseParForm( &FText[i] );
                        if (!FText) break;
			            TextLen = strlen(FText);
			            if (!TextLen) break;
		                pars_read = sscanf(FText, "%d", &ReadVal);
		                if (!pars_read) break;
			            if ((ReadVal < 1) || (ReadVal > MAX_ZIP_CODE_ID)) break;
			            ParWebServPtr->ShopInfoCfg.ZipCode = ReadVal;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Shop URL parameter parse */
			            if (!StringParParse(FText, (char*)&ParWebServPtr->ShopInfoCfg.URL,
				            KeyShopUrl, MAX_LEN_URL_SERVER)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Shop region parameter parse */
			            if (!StringOptParParse(FText, (char*)&ParWebServPtr->ShopInfoCfg.Region,
				            KeyShopRegion, MAX_LEN_REGION_NAME)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Shop city parameter parse */
			            if (!StringParParse(FText, (char*)&ParWebServPtr->ShopInfoCfg.City,
				            KeyShopCity, MAX_LEN_CITY_NAME)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Shop address parameter parse */
			            if (!StringParParse(FText, (char*)&ParWebServPtr->ShopInfoCfg.Address,
				            KeyShopAddr, MAX_LEN_ADDR_1_NAME)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Shop land phone parameter parse */
			            if (!StringOptParParse(FText, (char*)&ParWebServPtr->ShopInfoCfg.LandPhone,
				            KeyShopLandPhone, MAX_LEN_PHONE_NUM)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Shop mobile phone 1 parameter parse */
			            if (!StringParParse(FText, (char*)&ParWebServPtr->ShopInfoCfg.MobilePhone1,
				            KeyShopMobPhone1, MAX_LEN_PHONE_NUM)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Shop mobile phone 2 parameter parse */
			            if (!StringOptParParse(FText, (char*)&ParWebServPtr->ShopInfoCfg.MobilePhone2,
				            KeyShopMobPhone2, MAX_LEN_PHONE_NUM)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Shop fax phone parameter parse */
			            if (!StringOptParParse(FText, (char*)&ParWebServPtr->ShopInfoCfg.FaxPhone,
				            KeyShopFax, MAX_LEN_PHONE_NUM)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Shop location latitude parameter parse */
			            if (!StringParParse(FText, (char*)&ParWebServPtr->ShopInfoCfg.LocLatitude,
				            KeyShopLatit, MAX_LOCATION_LEN)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Shop location longitude parameter parse */
			            if (!StringParParse(FText, (char*)&ParWebServPtr->ShopInfoCfg.LocLongitude,
				            KeyShopLongit, MAX_LOCATION_LEN)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);
			            ServerConfigSave(ParWebServPtr);
			            isMainPage = true;
                        isParseDone = true;
                        break;
                        
                    case GEC_BANNER:
	                    index = 1;
	                    SelObjPtr = (ObjListTask*)GetFistObjectList(&ParWebServPtr->BannerList);
	                    while(SelObjPtr)
						{
							if (SessionPtr->SelBannerId == index)
							{
                                SelBannerPtr = (BANNER_INFO*)SelObjPtr->UsedTask;
								break;
							}
							index++;
                            SelObjPtr = (ObjListTask*)GetNextObjectList(&ParWebServPtr->BannerList);
						}
						if (!SelBannerPtr) break;

					    i = FindCmdRequest(FText, KeyFormBannerActive);
		                if (i != -1)
						{
					        FText = ParseParForm( &FText[i] );
                            if (!FText) break;
					        TextLen = strlen(FText);
					        if (!TextLen) break;
		                    pars_read = sscanf(FText, "%d", &ReadVal);
		                    if (!pars_read) break;
							SelBannerPtr->isActive = (bool)ReadVal;
		                    FText = FStrt;
		                    strcpy(FText,HttpCmd);
						}
						else
						{
					        SelBannerPtr->isActive = false;
						}

			            /* Banner location id parameter parse */
			            i = FindCmdRequest(FText, KeyFormBanLocId);
		                if (i == -1) break;
			            FText = ParseParForm( &FText[i] );
                        if (!FText) break;
			            TextLen = strlen(FText);
			            if (!TextLen) break;
		                pars_read = sscanf(FText, "%d", &ReadVal);
		                if (!pars_read) break;
			            if ((ReadVal < MIN_BPL_TYPE_ID) || (ReadVal > MAX_BPL_TYPE_ID)) break;
			            SelBannerPtr->Location = ReadVal;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Banner name parameter parse */
			            if (!StringParParse(FText, (char*)&SelBannerPtr->Name,
				            KeyFormBannerName, MAX_LEN_BANNER_NAME)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

						/* Banner body parameter parse */
	                    i = FindCmdRequest(FText, KeyFormBannerBody);
		                if (i == -1) break;
	                    FText = ParseParForm(&FText[i]);
                        if (!FText) break;
	                    TextLen = strlen(FText);
	                    if (TextLen)
						{
							FreeMemory(SelBannerPtr->BodyPtr);
							SelBannerPtr->BodyPtr = BodyBufPtr;
							BodyBufPtr = NULL;
						}
		                else
						{
			                SelBannerPtr->BodyPtr[0] = 0;
						}
		                FText = FStrt;
					    strcpy(FText,HttpCmd);
						ServerConfigSave(ParWebServPtr);
						SessionPtr->SelBannerId = 0;
                        isParseDone = true;                    
                        break;

					default: /* GEC_SMS */
			            /* SMS server name config. parameter parse */
			            if (!StringParParse(FText, (char*)&ParWebServPtr->SmsWorker.SmsClientCfg.SmsServerName,
				            KeySmsServName, MAX_LEN_SMS_SERVER_NAME)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* SMS server IP port config. parameter parse */
			            i = FindCmdRequest(FText, KeySmsServPort);
		                if (i == -1) break;
			            FText = ParseParForm( &FText[i] );
                        if (!FText) break;
			            TextLen = strlen(FText);
			            if (!TextLen) break;
		                pars_read = sscanf(FText, "%d", &ReadVal);
		                if (!pars_read) break;
			            ParWebServPtr->SmsWorker.SmsClientCfg.SmsIpPort = ReadVal;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* SMS server timeout config. parameter parse */
			            i = FindCmdRequest(FText, KeySmsServTime);
		                if (i == -1) break;
			            FText = ParseParForm( &FText[i] );
                        if (!FText) break;
			            TextLen = strlen(FText);
			            if (!TextLen) break;
		                pars_read = sscanf(FText, "%d", &ReadVal);
		                if (!pars_read) break;
			            ParWebServPtr->SmsWorker.SmsClientCfg.SmsTimeout = ReadVal;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* SMS server access id config. parameter parse */
			            if (!StringParParse(FText, (char*)&ParWebServPtr->SmsWorker.SmsClientCfg.AccessId,
				            KeySmsAccessId, MAX_LEN_SMS_SERVER_ACCESS_ID)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* SMS server src name config. parameter parse */
			            if (!StringParParse(FText, (char*)&ParWebServPtr->SmsWorker.SmsClientCfg.SmsSrcName,
				            KeySmsSrcName, MAX_LEN_SMS_SRC_LEN)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Proxt server usage parameter parse */
			            i = FindCmdRequest(FText, KeyFormProxyFlag);
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
					            ParWebServPtr->SmsWorker.SmsClientCfg.isUsedProxy = true;
				            }
		                    FText = FStrt;
		                    strcpy(FText,HttpCmd);
			            }
			            else
			            {
                            ParWebServPtr->SmsWorker.SmsClientCfg.isUsedProxy = false;
			            }

			            /* Proxy server server IP port config. parameter parse */
			            i = FindCmdRequest(FText, KeyFormProxyPort);
		                if (i == -1) break;
			            FText = ParseParForm(&FText[i]);
                        if (!FText) break;
			            TextLen = strlen(FText);
			            if (!TextLen) break;
		                pars_read = sscanf(FText, "%d", &ReadVal);
		                if (!pars_read) break;
			            ParWebServPtr->SmsWorker.SmsClientCfg.ProxyServPort = ReadVal;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

			            /* Proxy server address. parameter parse */
			            if (!StringParParse(FText, (char*)&ParWebServPtr->SmsWorker.SmsClientCfg.ProxyServAddr,
				            KeyFormProxyAddr, MAX_LEN_PROXY_ADDR)) break;
		                FText = FStrt;
		                strcpy(FText,HttpCmd);

						ServerConfigSave(ParWebServPtr);
						SessionPtr->SelBannerId = 0;
                        isParseDone = true;
						break;
                }
				if (BodyBufPtr)
				{
					FreeMemory(BodyBufPtr);
					BodyBufPtr = NULL;
				}
			}
			else if (OperReq == SERV_CFG_FILE_HASH_UPD)
			{
	            FileDataHashUpdate(&ParWebServPtr->StartPath[0], &ParWebServPtr->ServCustomCfg,
		            HashDirArray, HashDirArraySize);
				isMainPage = true;
                isParseDone = true;
			}
			else if (OperReq == SERV_CFG_CUSTOM_UPD)
			{
                CustomConfigLoad(&WebCustomCfg);
				isMainPage = true;
                isParseDone = true;
			}
			else if (OperReq == WEB_SERV_SHUTDOWN_REQ)
			{
				ParWebServPtr->StopServReq = true;
				isShutdown = true;
                isParseDone = false;
			}
            else if (OperReq == SERV_CFG_CMD_CHG_PAGE)
            {
			    /* Edit page set parameter parse */
			    i = FindCmdRequest(FText, KeySelEditGrp);
		        if (i == -1) break;
			    FText = ParseParForm( &FText[i] );
                if (!FText) break;
			    TextLen = strlen(FText);
			    if (!TextLen) break;
		        pars_read = sscanf(FText, "%d", &ReadVal);
		        if (!pars_read) break;
			    if ((ReadVal < MIN_GEC_TYPE_ID) || (ReadVal > MAX_GEC_TYPE_ID)) break;
		        FText = FStrt;
		        strcpy(FText,HttpCmd);            
                SessionPtr->SelConfigEditGrpId = ReadVal;
				SessionPtr->SelBannerId = 0;
				SessionPtr->SelSmsPhoneId = 0;
                isParseDone = true;                
            }
			else if (OperReq == SERV_CFG_CMD_ADD_BANNER)
			{
				if (ParWebServPtr->BannerList.Count > (MAX_BANNERS_LIST - 1))
				{
					/* Max number of supported banners is reached */
                    isParseDone = true;
					break;
				}
				/* New banner create request handle */
				NewBannerPtr = (BANNER_INFO*)AllocateMemory(sizeof(BANNER_INFO));
				if (NewBannerPtr)
				{
					NewBannerPtr->isActive = false;
                    NewBannerPtr->Location = BPL_HIDE_AREA_PGBEG;
				    memset(NewBannerPtr->Name, 0, MAX_LEN_BANNER_NAME+1);
					strcpy(NewBannerPtr->Name,"<Name>");
                    NewBannerPtr->BodyPtr = (char*)AllocateMemory(24*sizeof(char));
					if (NewBannerPtr->BodyPtr)
					{
				        strcpy(NewBannerPtr->BodyPtr, "<Place body here>\r\n");
						NewBannerPtr->ObjPtr = AddStructListObj(&ParWebServPtr->BannerList, NewBannerPtr);
						ServerConfigSave(ParWebServPtr);
				        isParseDone = true;
					}
					else
					{
                        printf("Fail to memory allocation for banner body\n");
						FreeMemory(NewBannerPtr);
					}
				}
				else
				{
					printf("Fail to memory allocation for new SMS phone record\n");
				}
			}
			else if (OperReq == SERV_CFG_CMD_SEL_EDIT)
			{
		        i = FindCmdRequest(FText, KeyFormBannerId);
				if (i == -1) break;
				FText = ParseParForm( &FText[i] );
                if (!FText) break;
				TextLen = strlen(FText);
				if (!TextLen) break;
		        pars_read = sscanf(FText, "%d", &ReadVal);
		        if (!pars_read) break;
		        FText = FStrt;
		        strcpy(FText,HttpCmd);
				SessionPtr->SelBannerId = ReadVal;
			    isParseDone = true;
			}
			else if (OperReq == SERV_CFG_CMD_REM_BANNER)
			{
				if (!SessionPtr->SelBannerId) break;
	            index = 1;
	            SelObjPtr = (ObjListTask*)GetFistObjectList(&ParWebServPtr->BannerList);
	            while(SelObjPtr)
				{
				    if (SessionPtr->SelBannerId == index)
					{
                        SelBannerPtr = (BANNER_INFO*)SelObjPtr->UsedTask;
						break;
					}
					index++;
                    SelObjPtr = (ObjListTask*)GetNextObjectList(&ParWebServPtr->BannerList);
				}
				if (!SelBannerPtr) break;
                RemStructList(&ParWebServPtr->BannerList, SelBannerPtr->ObjPtr);
				if (SelBannerPtr->BodyPtr) FreeMemory(SelBannerPtr->BodyPtr);
                FreeMemory(SelBannerPtr);
                ServerConfigSave(ParWebServPtr);
				SessionPtr->SelBannerId = 0;
			    isParseDone = true;
			}
			else if (OperReq == SERV_CFG_CMD_ADD_PHONE)
			{
				if (ParWebServPtr->SmsWorker.SmsClientCfg.SmsDestNumList.Count > (MAX_SMS_PHONE_LIST - 1))
				{
					/* Max number of supported phones is reached */
                    isParseDone = true;
					break;
				}
				/* New phone SMS delivery create request handle */
				NewPhonePtr = (SENTSMSNUM*)AllocateMemory(sizeof(SENTSMSNUM));
				if (NewPhonePtr)
				{
					NewPhonePtr->isRusLangInfo = false;
				    memset(NewPhonePtr->PhoneNum, 0, MAX_LEN_SMS_PHONE_NUM+1);
					strcpy(NewPhonePtr->PhoneNum,"<Number>");
					NewPhonePtr->ObjPtr = AddStructListObj(&ParWebServPtr->SmsWorker.SmsClientCfg.SmsDestNumList, NewPhonePtr);
					ServerConfigSave(ParWebServPtr);
				    isParseDone = true;
				}
				else
				{
					printf("Fail to memory allocation for new banner\n");
				}
			}
			else if (OperReq == SERV_CFG_CMD_SEL_PHED)
			{
		        i = FindCmdRequest(FText, KeyFormPhoneId);
				if (i == -1) break;
				FText = ParseParForm( &FText[i] );
                if (!FText) break;
				TextLen = strlen(FText);
				if (!TextLen) break;
		        pars_read = sscanf(FText, "%d", &ReadVal);
		        if (!pars_read) break;
		        FText = FStrt;
		        strcpy(FText,HttpCmd);
				SessionPtr->SelSmsPhoneId = ReadVal;
			    isParseDone = true;
			}
			else if (OperReq == SERV_CFG_CMD_CHG_SMS_NUM)
			{
		        index = 1;
	            SelObjPtr = (ObjListTask*)GetFistObjectList(&ParWebServPtr->SmsWorker.SmsClientCfg.SmsDestNumList);
	            while(SelObjPtr)
				{
					if (SessionPtr->SelSmsPhoneId == index)
					{
                        SelPhonePtr = (SENTSMSNUM*)SelObjPtr->UsedTask;
						break;
					}
					index++;
                    SelObjPtr = (ObjListTask*)GetNextObjectList(&ParWebServPtr->SmsWorker.SmsClientCfg.SmsDestNumList);
				}
			    if (!SelPhonePtr) break;

				i = FindCmdRequest(FText, KeyFormSmsRus);
		        if (i != -1)
				{
				    FText = ParseParForm( &FText[i] );
                    if (!FText) break;
					TextLen = strlen(FText);
					if (!TextLen) break;
		            pars_read = sscanf(FText, "%d", &ReadVal);
		            if (!pars_read) break;
					SelPhonePtr->isRusLangInfo = (bool)ReadVal;
		            FText = FStrt;
		            strcpy(FText,HttpCmd);
				}
				else
				{
				    SelPhonePtr->isRusLangInfo = false;
				}
			    
				/* Phone number parameter parse */
			    if (!StringParParse(FText, (char*)&SelPhonePtr->PhoneNum,
				    KeyFormSmsPhone, MAX_LEN_SMS_PHONE_NUM)) break;
		        FText = FStrt;
		        strcpy(FText,HttpCmd);
                ServerConfigSave(ParWebServPtr);
				SessionPtr->SelSmsPhoneId = 0;
			    isParseDone = true;

			}
			else /* SERV_CFG_CMD_REM_SMS_NUM */
			{
				if (!SessionPtr->SelSmsPhoneId) break;
	            index = 1;
	            SelObjPtr = (ObjListTask*)GetFistObjectList(&ParWebServPtr->SmsWorker.SmsClientCfg.SmsDestNumList);
	            while(SelObjPtr)
				{
				    if (SessionPtr->SelSmsPhoneId == index)
					{
                        SelPhonePtr = (SENTSMSNUM*)SelObjPtr->UsedTask;
						break;
					}
					index++;
                    SelObjPtr = (ObjListTask*)GetNextObjectList(&ParWebServPtr->SmsWorker.SmsClientCfg.SmsDestNumList);
				}
				if (!SelPhonePtr) break;
                RemStructList(&ParWebServPtr->SmsWorker.SmsClientCfg.SmsDestNumList, SelPhonePtr->ObjPtr);
                FreeMemory(SelPhonePtr);
                ServerConfigSave(ParWebServPtr);
				SessionPtr->SelSmsPhoneId = 0;
			    isParseDone = true;
			}
		}
		else
		{
			SessionPtr->SelConfigEditGrpId = GEC_GENERAL;
			SessionPtr->SelBannerId = 0;
            isParseDone = true;
		}
		break;
	}
    if (isParseDone)
	{
		if (!isMainPage) ServerConfigWebPageGen(BufAnsw, SessionPtr, HttpCmd);
		else  		     MainPageSetShopWebPage(BufAnsw, SessionPtr, HttpCmd);
	}
	else
	{
	    AddBeginPageShopWebPage(BufAnsw, SessionPtr);
		if (isShutdown)
		{
		    strcat(BufAnsw,"<center><font size=\"2\" color=\"green\">");
	     	SetRusTextBuf(BufAnsw, SITE_RUS_SERVER_SHUTDOWN_START_INF_LINE_ID);

		}
		else
		{
		    strcat(BufAnsw,"<center><font size=\"3\" color=\"red\">");
	     	SetRusTextBuf(BufAnsw, SITE_RUS_ITEM_NOT_FOUND_LINE_ID);
		}
		strcat(BufAnsw,"</font></center>\r\n");
		AddEndPageShopWebPage(BufAnsw, SessionPtr);
	}
	FreeMemory(FStrt);
}
//---------------------------------------------------------------------------
void ServerConfigWebPageGen(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd)
{
    unsigned int GrpId;
    char         StrLineBuf[256];
    
	SessionPtr->SecureKey = SecureKeyGen();
	AddBeginPageShopWebPage(BufAnsw, SessionPtr);
    EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];

	if((SessionPtr->SelConfigEditGrpId == GEC_BANNER) ||
	   (SessionPtr->SelConfigEditGrpId == GEC_SMS))
	{
	    /* Script for item for edit selection support */
		SetHtmlTemlateBody(ParWebServPtr, ParReadHttpSocketPtr,
		    SessionPtr, "SelButtonScript.html");
	}

    AddStrWebPage("<script language=\"javascript\" type=\"text/javascript\">\r\n");
    AddStrWebPage("function submitbutton_reg() {\r\n");
    AddStrWebPage("var form = document.ServConfForm;\r\n");
	AddStrWebPage("var isvalid = true;\r\n");
	AddStrWebPage("var fail_reason = 1;\r\n");
    AddStrWebPage("var r = new RegExp(\"[\\<|\\>|\\\"|\\'|\\%|\\;|\\(|\\)|\\&|\\+|\\-]\", \"i\");\r\n");

	switch(SessionPtr->SelConfigEditGrpId)
	{
	    case GEC_GENERAL:
	        SetKeyParCheckScript(KeyMaxOpenSess);
	        SetKeyParCheckScript(KeyMaxOpenIpSess);
	        SetKeyParCheckScript(KeyMinPageSizeCompr);
	        SetKeyParCheckScript(KeyKeepAliveTime);
			break;

		case GEC_MAIL:
	        SetKeyParCheckScript(KeySmtpServName);
	        SetKeyParCheckScript(KeySmtpServPort);
	        SetKeyParCheckScript(KeySmtpServTime);
	        SetKeyParCheckScript(KeySmtpIntMail);
	        SetKeyParCheckScript(KeySmtpServEmail);
			break;

		case GEC_SHOP:
	        SetKeyParCheckScript(KeyShopUrl);
	        SetKeyParCheckScript(KeyShopCity);
	        SetKeyParCheckScript(KeyShopAddr);
	        SetKeyParCheckScript(KeyShopMobPhone1);
	        SetKeyParCheckScript(KeyShopLatit);
	        SetKeyParCheckScript(KeyShopLongit);
	        SetKeyParCheckScript(KeyShopZipCode);
            break;

		case GEC_BANNER:
	        SetKeyParCheckScript(KeyFormBannerName);
	        SetKeyParCheckScript(KeyFormBannerBody);
			break;

		default:
			SetKeyParCheckScript(KeySmsServName);
			SetKeyParCheckScript(KeySmsAccessId);
			break;
	}
	AddStrWebPage("if(!isvalid) {\r\n");
	AddStrWebPage("if(fail_reason == 1) {\r\n");
    AddStrWebPage("   alert( \"");
	SetOriginalRusTextBuf(NULL, SITE_RUS_FIELD_FILL_REQ_LINE_ID);
	AddStrWebPage("\" );}\r\n");
	AddStrWebPage("return false;\r\n");
    AddStrWebPage("} else {\r\n");
	AddStrWebPage("form.submit();}\r\n");
    AddStrWebPage("}\r\n</script>\r\n");

	// Form header
    AddStrWebPage("<h3><center>\r\n");
    SetRusTextBuf(NULL, SITE_RUS_SHOP_SERVER_CONFIG_LINE_ID);
    AddStrWebPage("</center></h3><br>\r\n");

	AddStrWebPage("<table cellpadding=\"0\" cellspacing=\"0\" border=\"0\" width=\"");
	if(SessionPtr->SelConfigEditGrpId == GEC_BANNER) AddStrWebPage("95");
	else                                             AddStrWebPage("70");
	AddStrWebPage("%\" align=\"center\" bgcolor=\"");
	AddStrWebPage(ParWebServPtr->ServCustomCfg.ServConfigBgColor1);
	AddStrWebPage("\">\r\n");
    AddStrWebPage("<tr><td colspan=\"2\">\r\n");
    AddStrWebPage("<table cellpadding=\"0\" cellspacing=\"0\" border=\"0\" align=\"center\"><tr>\r\n"); 
    for(GrpId=0;GrpId < MAX_GEC_TYPE_ID;GrpId++)
    {
        AddStrWebPage("<td width=\"80\" height=\"25\" cellpadding=\"2\" cellspacing=\"2\" border=\"0\" align=\"center\" bgcolor=\"");
		if ((GrpId+1) == SessionPtr->SelConfigEditGrpId) AddStrWebPage(ParWebServPtr->ServCustomCfg.ServConfigBgColor2);
		else                                             AddStrWebPage(ParWebServPtr->ServCustomCfg.ServConfigBgColor1);
		AddStrWebPage("\"><a href=\"");
        SetServerHttpAddr(NULL);
        AddStrWebPage(GenPageServerConfig);
        sprintf(StrLineBuf, "?%s=%d;%s=%s;%s=%d;%s=%d;%s=%d;",
        SecKeyId, SessionPtr->SecureKey,
        FormKeySessionId, SessionPtr->SesionIdKey,
        FormKeyUserId, SessionPtr->UserPtr->UserId,
        FormKeyCfgChgId, SERV_CFG_CMD_CHG_PAGE,
        KeySelEditGrp, (GrpId+1));
        AddStrWebPage(StrLineBuf);
        AddStrWebPage("\">");
		SetRusTextBuf(NULL, NameEditGrpList[GrpId]);
		AddStrWebPage("</a></td>\r\n");
    }
    AddStrWebPage("</tr></table></td></tr>\r\n");
	AddStrWebPage("<tr><td height=\"10\" colspan=\"2\">&nbsp;</td></tr>\r\n");
	AddStrWebPage("<tr><td colspan=\"2\">\r\n");

	AddStrWebPage("<fieldset>\r\n<legend><span class=\"sectiontableheader\">\r\n");
	switch(SessionPtr->SelConfigEditGrpId)
	{
		case GEC_GENERAL:
		    SetRusTextBuf(NULL, SITE_RUS_GENE_SERV_CFG_LINE_ID);
		    break;

		case GEC_MAIL:
			SetRusTextBuf(NULL, SITE_RUS_MAIL_SERV_CFG_LINE_ID);
			break;

		case GEC_SHOP:
			SetRusTextBuf(NULL, SITE_RUS_SHOP_INFO_CFG_LINE_ID);
			break;

		case GEC_BANNER:
			SetRusTextBuf(NULL, SITE_RUS_BANNER_CFG_LINE_ID);
			break;

		default:
			SetRusTextBuf(NULL, SITE_RUS_SMS_CFG_LINE_ID);
			break;
	}
    AddStrWebPage("</span></legend>\r\n");

	if (SessionPtr->SelConfigEditGrpId == GEC_BANNER)
	{
	    /* New banner record create request button */
        AddStrWebPage("<center><form action=\"");
        SetServerHttpAddr(NULL);
	    AddStrWebPage(GenPageServerConfig);
	    AddStrWebPage("\" method=\"post\" name=\"BannerCreateForm\">\r\n");
        SetHiddenServCfgPar(SessionPtr, SERV_CFG_CMD_ADD_BANNER);

	    AddStrWebPage("<input type=\"submit\" value=\"");
        SetRusTextBuf(NULL, SITE_RUS_NEW_BANNER_SET_LINE_ID);
        AddStrWebPage("\" class=\"button\" onclick=\"document.BannerCreateForm.submit();\" >\r\n");
	    AddStrWebPage("</form></center>\r\n");

		BannerParEditPageShow(SessionPtr);
		AddStrWebPage("</fieldset>\r\n");
	}
	else if (SessionPtr->SelConfigEditGrpId == GEC_SMS)
	{
		SmsParEditPageShow(SessionPtr);
		AddStrWebPage("</fieldset>\r\n");
	}
	else
	{
        AddStrWebPage("<form action=\"");
        SetServerHttpAddr(NULL);
	    AddStrWebPage(GenPageServerConfig);
	    AddStrWebPage("\" method=\"post\" name=\"ServConfForm\">\r\n");

	    AddStrWebPage("<table cellpadding=\"0\" cellspacing=\"0\" border=\"0\" class=\"contentpane\">\r\n");
	    AddStrWebPage("<tr><td colspan=\"2\"></td></tr>\r\n");

	    switch(SessionPtr->SelConfigEditGrpId)
		{
		    case GEC_GENERAL:
		        GenParEditPageShow();
		        break;

		    case GEC_MAIL:
			    MailParEditPageShow();
			    break;

		    default:
			    ShopParEditPageShow();
			    break;
		}
	    AddStrWebPage("<tr><td colspan=\"2\"></td></tr>\r\n");
 	    AddStrWebPage("<tr><td colspan=\"2\"></td></tr>\r\n");
	    AddStrWebPage("</table>\r\n");
	    AddStrWebPage("</fieldset>\r\n");

        SetHiddenServCfgPar(SessionPtr, SERV_CFG_CMD_SAVE_PAGE);
	    AddStrWebPage("<input type=\"button\" value=\"");
	    SetRusTextBuf(NULL, SITE_RUS_SENT_REQ_LINE_ID);
        AddStrWebPage("\" class=\"button\" onclick=\"submitbutton_reg()\" >\r\n");
	    AddStrWebPage("</form>\r\n");
	}
	/* Botton for files hash update check */
    AddStrWebPage("<form action=\"");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageServerConfig);
	AddStrWebPage("\" method=\"post\" name=\"FileHashUpdateForm\">\r\n");
	SetHiddenServCfgPar(SessionPtr, SERV_CFG_FILE_HASH_UPD);
	AddStrWebPage("<input type=\"button\" value=\"");
	SetRusTextBuf(NULL, SITE_RUS_FILE_HASH_UPDATE_LINE_ID);
	AddStrWebPage("\" class=\"button\" onclick=\"document.FileHashUpdateForm.submit()\" >\r\n");
    AddStrWebPage("</form>\r\n");

	/* Botton for server customisation configuration reload */
    AddStrWebPage("<form action=\"");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageServerConfig);
	AddStrWebPage("\" method=\"post\" name=\"CustCofReloadForm\">\r\n");
	SetHiddenServCfgPar(SessionPtr, SERV_CFG_CUSTOM_UPD);
	AddStrWebPage("<input type=\"button\" value=\"");
	SetRusTextBuf(NULL, SITE_RUS_CUSTOM_UPDATE_LINE_ID);
	AddStrWebPage("\" class=\"button\" onclick=\"document.CustCofReloadForm.submit()\" >\r\n");
    AddStrWebPage("</form>\r\n");

	/* Botton for WEB server graceful shutdown */
    AddStrWebPage("<form action=\"");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageServerConfig);
	AddStrWebPage("\" method=\"post\" name=\"WebServerShutdownForm\">\r\n");
	SetHiddenServCfgPar(SessionPtr, WEB_SERV_SHUTDOWN_REQ);
	AddStrWebPage("<input type=\"button\" value=\"");
	SetRusTextBuf(NULL, SITE_RUS_WEB_SERVER_SHUTDOWN_LINE_ID);
	AddStrWebPage("\" class=\"button\" onclick=\"document.WebServerShutdownForm.submit()\" >\r\n");
    AddStrWebPage("</form>\r\n");

	AddStrWebPage("</td></tr></table>\r\n");
	AddEndPageShopWebPage(BufAnsw, SessionPtr);
}
//---------------------------------------------------------------------------
void GenParEditPageShow()
{
	/* Max. number of simult. open sessions */
	SetEditIntLineWebPage(KeyMaxOpenSess, SITE_RUS_CFG_GEN_MAX_SESS_LINE_ID,
	ParWebServPtr->GeneralCfg.MaxOpenSessions, 4, 30, 50, false);

	/* Max. number of simult. open sessions per IP address */
	SetEditIntLineWebPage(KeyMaxOpenIpSess, SITE_RUS_CFG_GEN_MAX_IP_SESS_LINE_ID,
	ParWebServPtr->GeneralCfg.MaxSesionPerIP, 4, 30, 50, false);

	/* Flag of enable HTML pages compression */
    SetEditBoolLineWebPage(KeyCompressEnable, SITE_RUS_CFG_GEN_COMPR_EN_LINE_ID,
	    ParWebServPtr->GeneralCfg.HtmlPageComprssEnable, 50);

	/* Min HTML page size for compression */
	SetEditIntLineWebPage(KeyMinPageSizeCompr, SITE_RUS_CFG_GEN_MIN_PG_COMP_SZ_LINE_ID,
	ParWebServPtr->GeneralCfg.MinHtmlSizeCompress, 3, 30, 50, false);

	/* Flag of enable keep-alive in user's sessions */
    SetEditBoolLineWebPage(KeyKeepAliveEnable, SITE_RUS_CFG_GEN_KEEP_AL_EN_LINE_ID,
	    ParWebServPtr->GeneralCfg.KeepAliveEnable, 50);

	/* Time-out for keep-alive session */
	SetEditIntLineWebPage(KeyKeepAliveTime, SITE_RUS_CFG_GEN_KEEP_AL_TIME_LINE_ID,
	ParWebServPtr->GeneralCfg.KeepAliveTimeout, 2, 30, 50, false);
}
//---------------------------------------------------------------------------
void MailParEditPageShow()
{
    unsigned int index;
    char         StrBuf[64];

	/* Flag of enable HTML pages compression */
    SetEditBoolLineWebPage(KeySmtpMailClient, SITE_RUS_CFG_MAIL_CLIENT_USE_LINE_ID,
	    ParWebServPtr->MailWorker.MailClientCfg.MailClientUse, 50);

	/* SMTP server name configuration */
	SetEditStringLineWebPage(KeySmtpServName, SITE_RUS_CFG_SMTP_SERV_NAME_LINE_ID,
	ParWebServPtr->MailWorker.MailClientCfg.SmtpServerName, MAX_LEN_SERVER_NAME, 30, 50, false);

	/* Drop box for SMTP encode mode selection */
	AddStrWebPage("<tr><td width=\"50%%\"><div id=\"encode_type_div\" >\r\n");
    AddStrWebPage("<label for=\"encode_type_div\">");
    SetRusTextBuf(NULL, SITE_RUS_CFG_SMS_SERV_ENCODE_LINE_ID);
    AddStrWebPage("</label></div></td>\r\n");
    AddStrWebPage("<td><select class=\"inputbox\" name=\"");
	AddStrWebPage(KeySmtpServEncode);
	AddStrWebPage("\" >\r\n");
	for (index=MIN_SMTP_ENCODE_TYPE;index <= MAX_SMTP_ENCODE_TYPE;index++)
	{
        if (ParWebServPtr->MailWorker.MailClientCfg.SmtpEncodeType == index)
			  sprintf(StrBuf, "<option value=\"%d\" selected=\"selected\">", index);
		else  sprintf(StrBuf, "<option value=\"%d\">", index);
		AddStrWebPage(StrBuf);
		AddStrWebPage(EncodeModeNameList[index-1]);
		AddStrWebPage("</option>\r\n");
	}
    AddStrWebPage("</select>\r\n");
	AddStrWebPage("</td>\r\n");

	/* SMTP server IP port configuration */
	SetEditIntLineWebPage(KeySmtpServPort, SITE_RUS_CFG_SMTP_PORT_NUM_LINE_ID,
	ParWebServPtr->MailWorker.MailClientCfg.SmtpIpPort, MAX_LEN_SERVER_PORT, 30, 50, false);

	/* SMTP server timeout configuration */
	SetEditIntLineWebPage(KeySmtpServTime, SITE_RUS_CFG_SMTP_TIMEOUT_LINE_ID,
	ParWebServPtr->MailWorker.MailClientCfg.SmtpTimeout, MAX_LEN_SERVER_TIMEOUT, 30, 50, false);

	/* Inter mail sent interval configuration */
	SetEditIntLineWebPage(KeySmtpIntMail, SITE_RUS_CFG_MAIL_INT_LINE_ID,
	ParWebServPtr->MailWorker.MailClientCfg.MailSendInt, MAX_LEN_SERVER_TIMEOUT, 30, 50, false);

	/* Shop E-mail configuration */
	SetEditStringLineWebPage(KeySmtpServEmail, SITE_RUS_CFG_SHOP_MAIL_LINE_ID,
	ParWebServPtr->MailWorker.MailClientCfg.MailFrom, MAX_LEN_SERVER_EMAIL, 30, 50, false);

	/* SMTP server login configuration */
	SetEditStringLineWebPage(KeySmtpServLogin, SITE_RUS_CFG_SMTP_LOGIN_LINE_ID,
	ParWebServPtr->MailWorker.MailClientCfg.MailLogin, MAX_LEN_SERVER_LOGIN, 30, 50, false);

	/* SMTP server passwd configuration */
	SetEditStringLineWebPage(KeySmtpServPasswd, SITE_RUS_CFG_SMTP_PASSWD_NUM_LINE_ID,
	ParWebServPtr->MailWorker.MailClientCfg.MailPasswd, MAX_LEN_SERVER_PASSWD, 30, 50, true);
}
//---------------------------------------------------------------------------
void ShopParEditPageShow()
{
	/* Shop postal zip code configuration */
	SetEditIntLineWebPage(KeyShopZipCode, SITE_RUS_CFG_SHOP_ZIP_CODE_LINE_ID,
	ParWebServPtr->ShopInfoCfg.ZipCode, MAX_ZIP_CODE_ID, 30, 50, false);

	/* Shop URL configuration */
	SetEditStringLineWebPage(KeyShopUrl, SITE_RUS_CFG_SHOP_URL_LINE_ID,
	ParWebServPtr->ShopInfoCfg.URL, MAX_LEN_URL_SERVER, 30, 50, false);

	/* Shop region configuration */
	SetEditStringLineWebPage(KeyShopRegion, SITE_RUS_CFG_SHOP_REGION_LINE_ID,
	ParWebServPtr->ShopInfoCfg.Region, MAX_LEN_REGION_NAME, 30, 50, false);

	/* Shop city configuration */
	SetEditStringLineWebPage(KeyShopCity, SITE_RUS_CFG_SHOP_CITY_LINE_ID,
	ParWebServPtr->ShopInfoCfg.City, MAX_LEN_CITY_NAME, 30, 50, false);

	/* Shop address configuration */
	SetEditStringLineWebPage(KeyShopAddr, SITE_RUS_CFG_SHOP_ADDRESS_LINE_ID,
	ParWebServPtr->ShopInfoCfg.Address, MAX_LEN_ADDR_1_NAME, 30, 50, false);

	/* Shop land phone configuration */
	SetEditStringLineWebPage(KeyShopLandPhone, SITE_RUS_CFG_SHOP_LAND_PHONE_LINE_ID,
	ParWebServPtr->ShopInfoCfg.LandPhone, MAX_LEN_PHONE_NUM, 30, 50, false);

	/* Shop mobile phone 1 configuration */
	SetEditStringLineWebPage(KeyShopMobPhone1, SITE_RUS_CFG_SHOP_MOB_PHONE1_LINE_ID,
	ParWebServPtr->ShopInfoCfg.MobilePhone1, MAX_LEN_PHONE_NUM, 30, 50, false);

	/* Shop mobile phone 2 configuration */
	SetEditStringLineWebPage(KeyShopMobPhone2, SITE_RUS_CFG_SHOP_MOB_PHONE2_LINE_ID,
	ParWebServPtr->ShopInfoCfg.MobilePhone2, MAX_LEN_PHONE_NUM, 30, 50, false);

	/* Shop fax phone configuration */
	SetEditStringLineWebPage(KeyShopFax, SITE_RUS_CFG_SHOP_FAX_PHONE_LINE_ID,
	ParWebServPtr->ShopInfoCfg.FaxPhone, MAX_LEN_PHONE_NUM, 30, 50, false);

	/* Shop latitude location configuration */
	SetEditStringLineWebPage(KeyShopLatit, SITE_RUS_CFG_SHOP_LATITUDE_LINE_ID,
	ParWebServPtr->ShopInfoCfg.LocLatitude, MAX_LOCATION_LEN, 30, 50, false);

	/* Shop longitude location configuration */
	SetEditStringLineWebPage(KeyShopLongit, SITE_RUS_CFG_SHOP_LONGITUDE_LINE_ID,
	ParWebServPtr->ShopInfoCfg.LocLongitude, MAX_LOCATION_LEN, 30, 50, false);
}
//---------------------------------------------------------------------------
void SmsParEditPageShow(USER_SESSION *SessionPtr)
{
	unsigned int index = 1;
	SENTSMSNUM  *SelPhonePtr = NULL;
	SENTSMSNUM  *EditPhonePtr = NULL;
	ObjListTask  *SelObjPtr = NULL;
	char         StrBuf[256];

    AddStrWebPage("<form action=\"");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageServerConfig);
	AddStrWebPage("\" method=\"post\" name=\"ServConfForm\">\r\n");

	AddStrWebPage("<table cellpadding=\"0\" cellspacing=\"0\" border=\"0\" class=\"contentpane\">\r\n");
	AddStrWebPage("<tr><td colspan=\"2\"></td></tr>\r\n");

	/* SMS server name configuration */
	SetEditStringLineWebPage(KeySmsServName, SITE_RUS_CFG_SMS_SERV_NAME_LINE_ID,
	ParWebServPtr->SmsWorker.SmsClientCfg.SmsServerName, MAX_LEN_SMS_SERVER_NAME, 30, 50, false);

	/* SMS server IP port configuration */
	SetEditIntLineWebPage(KeySmsServPort, SITE_RUS_CFG_SMS_PORT_NUM_LINE_ID,
	ParWebServPtr->SmsWorker.SmsClientCfg.SmsIpPort, MAX_LEN_SMS_SERVER_PORT, 30, 50, false);

	/* SMS server timeout configuration */
	SetEditIntLineWebPage(KeySmsServTime, SITE_RUS_CFG_SMS_TIMEOUT_LINE_ID,
	ParWebServPtr->SmsWorker.SmsClientCfg.SmsTimeout, MAX_LEN_SMS_SERVER_TIMEOUT, 30, 50, false);

	/* SMS server access id configuration */
	SetEditStringLineWebPage(KeySmsAccessId, SITE_RUS_CFG_SMS_ACCESS_ID_LINE_ID,
	ParWebServPtr->SmsWorker.SmsClientCfg.AccessId, MAX_LEN_SMS_SERVER_ACCESS_ID, 30, 50, false);

	/* SMS source name configuration */
	SetEditStringLineWebPage(KeySmsSrcName, SITE_RUS_CFG_SMS_SRC_LINE_ID,
	ParWebServPtr->SmsWorker.SmsClientCfg.SmsSrcName, MAX_LEN_SMS_SRC_LEN, 30, 50, false);

	/* Flag of enable SMS proxy server */
    SetEditBoolLineWebPage(KeyFormProxyFlag, SITE_RUS_CFG_SMS_PROXY_EN_LINE_ID,
	    ParWebServPtr->SmsWorker.SmsClientCfg.isUsedProxy, 50);

	/* Proxy server address */
	SetEditStringLineWebPage(KeyFormProxyAddr, SITE_RUS_CFG_SMS_PROXY_ADDR_LINE_ID,
	ParWebServPtr->SmsWorker.SmsClientCfg.ProxyServAddr, MAX_LEN_PROXY_ADDR, 30, 50, false);

	/* Proxy server IP port configuration */
	SetEditIntLineWebPage(KeyFormProxyPort, SITE_RUS_CFG_SMS_PROXY_PORT_LINE_ID,
	ParWebServPtr->SmsWorker.SmsClientCfg.ProxyServPort, MAX_LEN_SMS_SERVER_PORT, 30, 50, false);

	AddStrWebPage("<tr><td colspan=\"2\"></td></tr>\r\n");
 	AddStrWebPage("<tr><td colspan=\"2\"></td></tr>\r\n");
	AddStrWebPage("</table>\r\n");

    SetHiddenServCfgPar(SessionPtr, SERV_CFG_CMD_SAVE_PAGE);

	AddStrWebPage("<input type=\"button\" value=\"");
	SetRusTextBuf(NULL, SITE_RUS_SENT_REQ_LINE_ID);
    AddStrWebPage("\" class=\"button\" onclick=\"submitbutton_reg()\" >\r\n");
	AddStrWebPage("</form><br><hr><br>\r\n");

    AddStrWebPage("<center><form action=\"");
    SetServerHttpAddr(NULL);
	AddStrWebPage(GenPageServerConfig);
	AddStrWebPage("\" method=\"post\" name=\"PhoneCreateForm\">\r\n");
    SetHiddenServCfgPar(SessionPtr, SERV_CFG_CMD_ADD_PHONE);

	AddStrWebPage("<input type=\"submit\" value=\"");
    SetRusTextBuf(NULL, SITE_RUS_NEW_PHONE_SET_LINE_ID);
    AddStrWebPage("\" class=\"button\" onclick=\"document.PhoneCreateForm.submit();\" >\r\n");
	AddStrWebPage("</form></center>\r\n");

	if (!ParWebServPtr->SmsWorker.SmsClientCfg.SmsDestNumList.Count)
	{
		AddStrWebPage("<center><h2>\r\n");
		SetRusTextBuf(NULL, SITE_RUS_SMS_PHONE_LIST_EMPTY_LINE_ID);
		AddStrWebPage("</h2></center>\r\n");
		return;
	}

    AddStrWebPage("<table width=\"100%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");
    AddStrWebPage("<tr align=\"left\" class=\"sectiontableheader\">\r\n");
	AddStrWebPage("<th width=\"24\">Nn</th>\r\n");
    AddStrWebPage("<th>");
	SetRusTextBuf(NULL, SITE_RUS_ORDER_MOB_PHONE_LINE_ID);
    AddStrWebPage("</th><th>\r\n");
	SetRusTextBuf(NULL, SITE_RUS_SMS_RUS_LANG_SEL_LINE_ID);
	AddStrWebPage("</th><th width=\"64\">\r\n");
	SetRusTextBuf(NULL, SITE_RUS_SERV_DB_OPER_LINE_ID);
	AddStrWebPage("</th></tr>\r\n");
	index = 1;
	SelObjPtr = (ObjListTask*)GetFistObjectList(&ParWebServPtr->SmsWorker.SmsClientCfg.SmsDestNumList);
	while(SelObjPtr)
	{
	    SelPhonePtr = (SENTSMSNUM*)SelObjPtr->UsedTask;
        AddStrWebPage("<tr valign=\"top\" class=\"sectiontableentry1\">\r\n");

	    /* Show number of phone in list */
		sprintf(StrBuf, "<td>%d</td>\r\n", index);
		AddStrWebPage(StrBuf);

		/* Show phone number in list */
        AddStrWebPage("<td>");
		AddStrWebPage(SelPhonePtr->PhoneNum);
        AddStrWebPage("</td>\r\n");

		/* Shov SMS language */
		AddStrWebPage("<td align=\"center\" valign=\"center\"><img src=\"");
        SetServerHttpAddr(NULL);
		AddStrWebPage("images/");
		if (SelPhonePtr->isRusLangInfo) AddStrWebPage("SetEnGreen.png");
		else                            AddStrWebPage("SetDisRed.png");
		AddStrWebPage("\" title=\"");
		SetRusTextBuf(NULL, SITE_RUS_SMS_TEST_RUS_LINE_ID);
	    AddStrWebPage("\"></td>\r\n");

		/* Show operation selector */
		AddStrWebPage("<td align=\"center\" valign=\"center\"><a href=\"");
        SetServerHttpAddr(NULL);
		AddStrWebPage(GenPageServerConfig);

        sprintf(StrBuf, "?%s=%d;%s=%s;%s=%d;%s=%d;%s=%d;%s=%d",
        SecKeyId, SessionPtr->SecureKey,
        FormKeySessionId, SessionPtr->SesionIdKey,
        FormKeyUserId, SessionPtr->UserPtr->UserId,
        FormKeyCfgChgId, SERV_CFG_CMD_SEL_PHED,
        KeySelEditGrp, GEC_SMS,
		KeyFormPhoneId, index);
        AddStrWebPage(StrBuf);

		sprintf(StrBuf, "\" onMouseOver=\"hiLite('BtSelItemEdit%d','BtSelItemEditS','1');return true\" ", index);
		AddStrWebPage(StrBuf);
		if (index == SessionPtr->SelSmsPhoneId)
		{
			sprintf(StrBuf, "\" onMouseOut=\"hiLite('BtSelItemEdit%d','BtSelItemEditA','0');return true\" ", index);
			EditPhonePtr = SelPhonePtr;
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
		if (index == SessionPtr->SelSmsPhoneId)
		{
	        AddStrWebPage("src=\"images/BtSelItemEditAct.png\"");
		}
		else
		{
		    AddStrWebPage("src=\"images/BtSelItemEditPas.png\"");
		}
		AddStrWebPage(" border=\"0\" width=\"20\" height=\"20\" ALT=\"Select item for edit.\"></a>\r\n");
        AddStrWebPage("</td></tr>\r\n");

		index++;
		SelObjPtr = (ObjListTask*)GetNextObjectList(&ParWebServPtr->SmsWorker.SmsClientCfg.SmsDestNumList);
	}
    AddStrWebPage("</table>\r\n");

	if ((SessionPtr->SelSmsPhoneId > 0) && EditPhonePtr)
	{
	    AddStrWebPage("<fieldset>\r\n<legend><span class=\"sectiontableheader\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_SMS_PHONE_EDIT_LINE_ID);
        AddStrWebPage("</span></legend>\r\n");

        AddStrWebPage("<form action=\"");
		AddStrWebPage(GenPageServerConfig);
		AddStrWebPage("\" method=\"post\" name=\"PhoneConfForm\">\r\n");

		AddStrWebPage("<table width=\"100%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");

	    /* Text input for phone number set */
		AddStrWebPage("<tr align=\"left\">\r\n");
		AddStrWebPage("<td colspan=\"2\"><div id=\"phone_num_div\" ");
		AddStrWebPage( InputLineStyle);
		AddStrWebPage(">\r\n");
        AddStrWebPage("<label for=\"phone_num_id\">");
        SetRusTextBuf(NULL, SITE_RUS_SMS_PHONE_NUM_LINE_ID);
        AddStrWebPage(": </label></div>\r\n");
        AddStrWebPage("<input type=\"text\" id=\"");
		AddStrWebPage(KeyFormSmsPhone);
		AddStrWebPage("\" name=\"");
        AddStrWebPage(KeyFormSmsPhone);
	    AddStrWebPage("\" class=\"inputbox\" size=\"20\" ");
		sprintf(StrBuf, "value=\"%s\" maxlength=\"%d\" >\r\n", 
			EditPhonePtr->PhoneNum, MAX_LEN_SMS_PHONE_NUM);
	    AddStrWebPage(StrBuf);
		AddStrWebPage("</td></tr>\r\n");

		/* SMS language field edit */
		AddStrWebPage("<tr><td colspan=\"2\">\r\n");
		AddStrWebPage("<div id=\"");
		AddStrWebPage(KeyFormSmsRus);
		AddStrWebPage("_div\" ");
		AddStrWebPage(InputLineStyle);
		AddStrWebPage(">\r\n");
        AddStrWebPage("<label for=\"");
		AddStrWebPage(KeyFormSmsRus);
		AddStrWebPage("\">");
        SetRusTextBuf(NULL, SITE_RUS_SMS_RUS_LINE_ID);
        AddStrWebPage(": </label></div>\r\n");
	    AddStrWebPage("<input type=\"checkbox\" name=\"");
		AddStrWebPage(KeyFormSmsRus);
	    AddStrWebPage("\" id=\"");
		AddStrWebPage(KeyFormSmsRus);
		AddStrWebPage("\" class=\"inputbox\" value=\"1\" alt=\"");
	    SetRusTextBuf(NULL, SITE_RUS_SMS_LANG_INFO_LINE_ID);
		AddStrWebPage("\"");
		if (EditPhonePtr->isRusLangInfo) AddStrWebPage(" checked"); 
        AddStrWebPage(" ></td></tr>\r\n");

		SetHiddenServCfgPar(SessionPtr, SERV_CFG_CMD_CHG_SMS_NUM);

		/* Key for banner record save */
		AddStrWebPage("<tr align=\"center\"><td>\r\n");
        AddStrWebPage("<input type=\"submit\" name=\"Submit\" class=\"button\" value=\"");
        SetRusTextBuf(NULL, SITE_RUS_SMS_PHONE_SAVE_LINE_ID);
		AddStrWebPage("\" onclick=\"document.PhoneConfForm.submit();\" >\r\n");
		AddStrWebPage("</form>\r\n");
		AddStrWebPage("</td>\r\n");

		/* Key for banner record delete */
        AddStrWebPage("<td><form action=\"");
		AddStrWebPage(GenPageServerConfig);
		AddStrWebPage("\" method=\"post\" name=\"SmsNumRemForm\">\r\n");

        SetHiddenServCfgPar(SessionPtr, SERV_CFG_CMD_REM_SMS_NUM);

        AddStrWebPage("<input type=\"submit\" name=\"Submit\" class=\"button\" value=\"");
        SetRusTextBuf(NULL, SITE_RUS_SMS_NUM_DELETE_LINE_ID);
		AddStrWebPage("\" onclick=\"document.SmsNumRemForm.submit();\" >\r\n");
		AddStrWebPage("</td></tr>\r\n");

		AddStrWebPage("</table>\r\n");
		AddStrWebPage("</form>\r\n");
		AddStrWebPage("</fieldset>\r\n");
	}
}
//---------------------------------------------------------------------------
void BannerParEditPageShow(USER_SESSION *SessionPtr)
{
	unsigned int index = 1;
	BANNER_INFO  *SelBannerPtr = NULL;
	BANNER_INFO  *EditBannerPtr = NULL;
	ObjListTask  *SelObjPtr = NULL;
	char         StrBuf[256];

	if (!ParWebServPtr->BannerList.Count)
	{
		AddStrWebPage("<center><h2>\r\n");
		SetRusTextBuf(NULL, SITE_RUS_BANNER_LIST_EMPTY_LINE_ID);
		AddStrWebPage("</h2></center>\r\n");
		return;
	}
    AddStrWebPage("<table width=\"100%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");
    AddStrWebPage("<tr align=\"left\" class=\"sectiontableheader\">\r\n");
	AddStrWebPage("<th width=\"24\">Nn</th>\r\n");
    AddStrWebPage("<th width=\"150\">");
	SetRusTextBuf(NULL, SITE_RUS_BANNER_LOCATION_LINE_ID);
    AddStrWebPage("</th><th>\r\n");
	SetRusTextBuf(NULL, SITE_RUS_BANNER_NAME_LINE_ID);
	AddStrWebPage("</th><th width=\"46\">\r\n");
	SetRusTextBuf(NULL, SITE_RUS_BANNER_STATUS_INF_LINE_ID);
	AddStrWebPage("</th><th width=\"64\">\r\n");
	SetRusTextBuf(NULL, SITE_RUS_SERV_DB_OPER_LINE_ID);
	AddStrWebPage("</th></tr>\r\n");
	index = 1;
	SelObjPtr = (ObjListTask*)GetFistObjectList(&ParWebServPtr->BannerList);
	while(SelObjPtr)
	{
	    SelBannerPtr = (BANNER_INFO*)SelObjPtr->UsedTask;
        AddStrWebPage("<tr valign=\"top\" class=\"sectiontableentry1\">\r\n");

	    /* Show number of banner in list */
		sprintf(StrBuf, "<td>%d</td>\r\n", index);
		AddStrWebPage(StrBuf);

		/* Show banner location */
        AddStrWebPage("<td>");
        SetRusTextBuf(NULL, BannerLocationNameList[SelBannerPtr->Location-1]);
		AddStrWebPage("</td>\r\n");

		/* Show banner name */
		AddStrWebPage("<td>");
		SetRusTextBufName(NULL, (unsigned char*)&SelBannerPtr->Name[0]);
        AddStrWebPage("</td>\r\n");

		/* Shov activity of banner */
		AddStrWebPage("<td align=\"center\" valign=\"center\"><img src=\"");
        SetServerHttpAddr(NULL);
		AddStrWebPage("images/");
		if (SelBannerPtr->isActive) AddStrWebPage("SetEnGreen.png");
		else                        AddStrWebPage("SetDisRed.png");
		AddStrWebPage("\" title=\"");
		SetRusTextBuf(NULL, SITE_RUS_BANNER_ACTIVE_STATUS_LINE_ID);
	    AddStrWebPage("\"></td>\r\n");

		/* Show operation selector */
		AddStrWebPage("<td align=\"center\" valign=\"center\"><a href=\"");
        SetServerHttpAddr(NULL);
		AddStrWebPage(GenPageServerConfig);

        sprintf(StrBuf, "?%s=%d;%s=%s;%s=%d;%s=%d;%s=%d;%s=%d",
        SecKeyId, SessionPtr->SecureKey,
        FormKeySessionId, SessionPtr->SesionIdKey,
        FormKeyUserId, SessionPtr->UserPtr->UserId,
        FormKeyCfgChgId, SERV_CFG_CMD_SEL_EDIT,
        KeySelEditGrp, GEC_BANNER,
		KeyFormBannerId, index);
        AddStrWebPage(StrBuf);

		sprintf(StrBuf, "\" onMouseOver=\"hiLite('BtSelItemEdit%d','BtSelItemEditS','1');return true\" ", index);
		AddStrWebPage(StrBuf);
		if (index == SessionPtr->SelBannerId)
		{
			sprintf(StrBuf, "\" onMouseOut=\"hiLite('BtSelItemEdit%d','BtSelItemEditA','0');return true\" ", index);
			EditBannerPtr = SelBannerPtr;
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
		if (index == SessionPtr->SelBannerId)
		{
	        AddStrWebPage("src=\"images/BtSelItemEditAct.png\"");
		}
		else
		{
		    AddStrWebPage("src=\"images/BtSelItemEditPas.png\"");
		}
		AddStrWebPage(" border=\"0\" width=\"20\" height=\"20\" ALT=\"Select item for edit.\"></a>\r\n");
        AddStrWebPage("</td></tr>\r\n");

		index++;
		SelObjPtr = (ObjListTask*)GetNextObjectList(&ParWebServPtr->BannerList);
	}
    AddStrWebPage("</table>\r\n");
	if ((SessionPtr->SelBannerId > 0) && EditBannerPtr)
	{
	    AddStrWebPage("<fieldset>\r\n<legend><span class=\"sectiontableheader\">\r\n");
        SetRusTextBuf(NULL, SITE_RUS_BANNER_EDIT_LINE_ID);
        AddStrWebPage("</span></legend>\r\n");

        AddStrWebPage("<form action=\"");
		AddStrWebPage(GenPageServerConfig);
		AddStrWebPage("\" method=\"post\" name=\"ServConfForm\" enctype=\"multipart/form-data\">\r\n");

		AddStrWebPage("<table width=\"100%\" cellspacing=\"2\" cellpadding=\"2\" border=\"0\">\r\n");

	    /* Drop box for banner location selection */
		AddStrWebPage("<tr align=\"left\">\r\n");
		AddStrWebPage("<td><div id=\"banner_loc_div\" ");
		AddStrWebPage( InputLineStyle);
		AddStrWebPage(">\r\n");
        AddStrWebPage("<label for=\"banner_loc_id\">");
        SetRusTextBuf(NULL, SITE_RUS_BANNER_LOCATION_LINE_ID);
        AddStrWebPage(": </label></div>\r\n");

		AddStrWebPage("<style>select{width: 150px;}</style>\r\n");
        AddStrWebPage("<select class=\"inputbox\" name=\"");
		AddStrWebPage(KeyFormBanLocId);
		AddStrWebPage("\" >\r\n");

		for (index=MIN_BPL_TYPE_ID;index <= MAX_BPL_TYPE_ID;index++)
		{
            if (EditBannerPtr->Location == index)
		    {
			    sprintf(StrBuf, "<option value=\"%d\" selected=\"selected\">", index);
		    }
		    else
		    {
                sprintf(StrBuf, "<option value=\"%d\">", index);
		    }
		    AddStrWebPage(StrBuf);
			SetRusTextBuf(NULL, BannerLocationNameList[index-1]);
		    AddStrWebPage("</option>\r\n");
		}
        AddStrWebPage("</select>\r\n");
		AddStrWebPage("</td>\r\n");

		/* Banner name filed edit */
		AddStrWebPage("<td><div id=\"");
		AddStrWebPage(KeyFormBannerName);
		AddStrWebPage("_div\" ");
		AddStrWebPage(InputLineStyle);
		AddStrWebPage(">\r\n");
        AddStrWebPage("<label for=\"");
		AddStrWebPage(KeyFormBannerName);
		AddStrWebPage("\">");
        SetRusTextBuf(NULL, SITE_RUS_BANNER_NAME_LINE_ID);
        AddStrWebPage(": </label></div>\r\n");
        AddStrWebPage("<input type=\"text\" id=\"");
		AddStrWebPage(KeyFormBannerName);
		AddStrWebPage("\" name=\"");
        AddStrWebPage(KeyFormBannerName);
	    AddStrWebPage("\" class=\"inputbox\" size=\"20\" ");
		sprintf(StrBuf, "value=\"%s\" maxlength=\"%d\" >\r\n", 
			EditBannerPtr->Name, MAX_LEN_BANNER_NAME);
	    AddStrWebPage(StrBuf);
		AddStrWebPage("</td></tr>\r\n");

		/* Banner activity field edit */
		AddStrWebPage("<tr><td colspan=\"2\">\r\n");
		AddStrWebPage("<div id=\"");
		AddStrWebPage(KeyFormBannerActive);
		AddStrWebPage("_div\" ");
		AddStrWebPage(InputLineStyle);
		AddStrWebPage(">\r\n");
        AddStrWebPage("<label for=\"");
		AddStrWebPage(KeyFormBannerActive);
		AddStrWebPage("\">");
        SetRusTextBuf(NULL, SITE_RUS_BANNER_ACTIVE_LINE_ID);
        AddStrWebPage(": </label></div>\r\n");
	    AddStrWebPage("<input type=\"checkbox\" name=\"");
		AddStrWebPage(KeyFormBannerActive);
	    AddStrWebPage("\" id=\"");
		AddStrWebPage(KeyFormBannerActive);
		AddStrWebPage("\" class=\"inputbox\" value=\"1\" alt=\"");
	    SetRusTextBuf(NULL, SITE_RUS_BANNER_ACTIVE_LINE_ID);
		AddStrWebPage("\"");
		if (EditBannerPtr->isActive) AddStrWebPage(" checked"); 
        AddStrWebPage(" ></td></tr>\r\n");

		/* Banner text edit filed */
        AddStrWebPage("<tr align=\"center\"><td colspan=\"2\">\r\n");
		AddStrWebPage("<div id=\"");
		AddStrWebPage(KeyFormBannerBody);
		AddStrWebPage("_div\" style=\"float:left;wtext-align:right;vertical-align:bottom;font-weight: bold;padding-right: 5px;\">\r\n");
		SetRusTextBuf(NULL, SITE_RUS_BANNER_BODY_LINE_ID);
	    AddStrWebPage(":<br ></div><textarea id= \"");
		AddStrWebPage(KeyFormBannerBody);
		AddStrWebPage("\" name=\"");
		AddStrWebPage(KeyFormBannerBody);
		AddStrWebPage("\" rows=10 cols=60 wrap=\"off\" ");
        AddStrWebPage(">");
		AddStrWebPage((char*)EditBannerPtr->BodyPtr);
	    AddStrWebPage("</textarea>\r\n");
		AddStrWebPage("</td></tr>\r\n");

		SetHiddenServCfgPar(SessionPtr, SERV_CFG_CMD_SAVE_PAGE);

		/* Key for banner record save */
		AddStrWebPage("<tr align=\"center\"><td>\r\n");
        AddStrWebPage("<input type=\"submit\" name=\"Submit\" class=\"button\" value=\"");
        SetRusTextBuf(NULL, SITE_RUS_BANNER_SAVE_LINE_ID);
		AddStrWebPage("\" onclick=\"submitbutton_reg();\" >\r\n");
		AddStrWebPage("</form>\r\n");
		AddStrWebPage("</td>\r\n");

		/* Key for banner record delete */
        AddStrWebPage("<td><form action=\"");
		AddStrWebPage(GenPageServerConfig);
		AddStrWebPage("\" method=\"post\" name=\"BannerRemForm\">\r\n");

        SetHiddenServCfgPar(SessionPtr, SERV_CFG_CMD_REM_BANNER);

        AddStrWebPage("<input type=\"submit\" name=\"Submit\" class=\"button\" value=\"");
        SetRusTextBuf(NULL, SITE_RUS_BANNER_DELETE_LINE_ID);
		AddStrWebPage("\" onclick=\"\" >\r\n");
		AddStrWebPage("</td></tr>\r\n");

		AddStrWebPage("</table>\r\n");
		AddStrWebPage("</form>\r\n");
		AddStrWebPage("</fieldset>\r\n");
	}
}
//---------------------------------------------------------------------------
void ServerConfigLoad(PARAMWEBSERV *ParServPtr)
{
	FILE            *HandleCfg;
	BANNER_INFO     *LoadBannerPtr = NULL;
	SENTSMSNUM      *LoadPhoneNumPtr = NULL;
	char            *CwdRet = NULL;
	unsigned int    BannerBodyLen;
	char            PathFile[1024];
	int	            ReadedLine;
	bool            isNeedsCreate = false;
	unsigned int    ConfigVer, ConfigID, BlkReadLen, BannersInList, PhoneNumInList;
	unsigned char   ReadBuf[2048];
	unsigned char   *DecompPtr = NULL;

	CwdRet = getcwd(PathFile, 512);
#ifdef _VCL60ENV_
    strcat(PathFile, "\\");
#else
    strcat(PathFile, "/");
#endif
    strcat(PathFile, LogsGrpName);
	strcat(PathFile, ServerConfigExt);

	if ((HandleCfg = fopen(PathFile,"rb" )) != NULL)
	{
		for(;;)
		{
            ReadedLine = fread(&ReadBuf[0], SITE_ID_VER_IDENTITY_LEN, 1, HandleCfg);
		    if (!ReadedLine)
			{
                printf("Failed to load version field of configuration\r\n");
				isNeedsCreate = true;
			    break;
			}
			DecompPtr = &ReadBuf[0];
			DecompPtr = Uint32Unpack(DecompPtr, &ConfigID);
            DecompPtr = Uint32Unpack(DecompPtr, &ConfigVer);
			if (ConfigID != SITE_CONFIG_CHECK_ID)
			{
                printf("Configuration file has wrong ID ( File is corrupted)\r\n");
				isNeedsCreate = true;
				break;
			}

            switch(ConfigVer)
            {
                case 1:
                    ReadedLine = fread(&ReadBuf[0], MAIL_CONFIG_PACK_SIZE, 1, HandleCfg);
                    break;

                case 2:
                    ReadedLine = fread(&ReadBuf[0], MAIL_CONFIG_PACK_SIZE_V2, 1, HandleCfg);
                    break;
                
                default:
                    ReadedLine = fread(&ReadBuf[0], MAIL_CONFIG_PACK_SIZE_V3, 1, HandleCfg);
                    break;
            }    
		    if (!ReadedLine)
			{
                printf("Failed to load mail client part of server configuration\r\n");
				isNeedsCreate = true;
			    break;
			}
            DecompPtr = &ReadBuf[0];
            if (ConfigVer > 1)
            {
                 ParServPtr->MailWorker.MailClientCfg.MailClientUse = (bool)*DecompPtr++;
            }
            if (ConfigVer > 2) ParServPtr->MailWorker.MailClientCfg.SmtpEncodeType = *DecompPtr++;
			DecompPtr = Uint32Unpack(DecompPtr, &ParServPtr->MailWorker.MailClientCfg.SmtpIpPort);
			DecompPtr = Uint32Unpack(DecompPtr, &ParServPtr->MailWorker.MailClientCfg.SmtpTimeout);
			DecompPtr = Uint32Unpack(DecompPtr, &ParServPtr->MailWorker.MailClientCfg.MailSendInt);
	        memcpy(&ParServPtr->MailWorker.MailClientCfg.SmtpServerName[0], DecompPtr, (MAX_LEN_SERVER_NAME+1));
	        DecompPtr += (MAX_LEN_SERVER_NAME+1);
	        memcpy(&ParServPtr->MailWorker.MailClientCfg.MailFrom[0], DecompPtr, (MAX_LEN_SERVER_EMAIL+1));
	        DecompPtr += (MAX_LEN_SERVER_EMAIL+1);
	        memcpy(&ParServPtr->MailWorker.MailClientCfg.MailLogin[0], DecompPtr, (MAX_LEN_SERVER_LOGIN+1));
	        DecompPtr += (MAX_LEN_SERVER_LOGIN+1);
	        memcpy(&ParServPtr->MailWorker.MailClientCfg.MailPasswd[0], DecompPtr, (MAX_LEN_SERVER_PASSWD+1));
	        DecompPtr += (MAX_LEN_SERVER_PASSWD+1);

            ReadedLine = fread(&ReadBuf[0], GENERAL_CONFIG_PACK_SIZE, 1, HandleCfg);
		    if (!ReadedLine)
			{
                printf("Failed to load general part of server configuration\r\n");
				isNeedsCreate = true;
			    break;
			}
            DecompPtr = &ReadBuf[0];
			DecompPtr = Uint32Unpack(DecompPtr, &ParServPtr->GeneralCfg.MaxOpenSessions);
			DecompPtr = Uint32Unpack(DecompPtr, &ParServPtr->GeneralCfg.MaxSesionPerIP);
			DecompPtr = Uint32Unpack(DecompPtr, &ParServPtr->GeneralCfg.MinHtmlSizeCompress);
            DecompPtr = Uint32Unpack(DecompPtr, &ParServPtr->GeneralCfg.KeepAliveTimeout);
			ParServPtr->GeneralCfg.HtmlPageComprssEnable = (bool)(*DecompPtr++);
			ParServPtr->GeneralCfg.KeepAliveEnable = (bool)(*DecompPtr++);

            BlkReadLen = SHOP_INFO_CONFIG_PACK_SIZE;
			ReadedLine = fread(&ReadBuf[0], BlkReadLen, 1, HandleCfg);
		    if (!ReadedLine)
			{
                printf("Failed to load shop info. part of server configuration\r\n");
				isNeedsCreate = true;
			    break;
			}
            DecompPtr = &ReadBuf[0];
            DecompPtr = Uint32Unpack(DecompPtr, &ParServPtr->ShopInfoCfg.ZipCode);
	        DecompPtr += (MAX_LEN_COMPANY_NAME+1);
	        memcpy(&ParServPtr->ShopInfoCfg.URL[0], DecompPtr, (MAX_LEN_URL_SERVER+1));
	        DecompPtr += (MAX_LEN_URL_SERVER+1);
	        memcpy(&ParServPtr->ShopInfoCfg.Region[0], DecompPtr, (MAX_LEN_REGION_NAME+1));
	        DecompPtr += (MAX_LEN_REGION_NAME+1);
	        memcpy(&ParServPtr->ShopInfoCfg.City[0], DecompPtr, (MAX_LEN_CITY_NAME+1));
	        DecompPtr += (MAX_LEN_CITY_NAME+1);
	        memcpy(&ParServPtr->ShopInfoCfg.Address[0], DecompPtr, (MAX_LEN_ADDR_1_NAME+1));
	        DecompPtr += (MAX_LEN_ADDR_1_NAME+1);
	        memcpy(&ParServPtr->ShopInfoCfg.LandPhone[0], DecompPtr, (MAX_LEN_PHONE_NUM+1));
	        DecompPtr += (MAX_LEN_PHONE_NUM+1);
	        memcpy(&ParServPtr->ShopInfoCfg.MobilePhone1[0], DecompPtr, (MAX_LEN_PHONE_NUM+1));
	        DecompPtr += (MAX_LEN_PHONE_NUM+1);
	        memcpy(&ParServPtr->ShopInfoCfg.MobilePhone2[0], DecompPtr, (MAX_LEN_PHONE_NUM+1));
	        DecompPtr += (MAX_LEN_PHONE_NUM+1);
	        memcpy(&ParServPtr->ShopInfoCfg.FaxPhone[0], DecompPtr, (MAX_LEN_PHONE_NUM+1));
	        DecompPtr += (MAX_LEN_PHONE_NUM+1);
	        memcpy(&ParServPtr->ShopInfoCfg.LocLatitude[0], DecompPtr, (MAX_LOCATION_LEN+1));
	        DecompPtr += (MAX_LOCATION_LEN+1);
	        memcpy(&ParServPtr->ShopInfoCfg.LocLongitude[0], DecompPtr, (MAX_LOCATION_LEN+1));
	        DecompPtr += (MAX_LOCATION_LEN+1);
            
			/* Load banners list information */
			ReadedLine = fread(&ReadBuf[0], 1, 1, HandleCfg);
		    if (!ReadedLine)
			{
                printf("Failed to load number of bunners in list part of server configuration\r\n");
				isNeedsCreate = true;
			    break;
			}
            BannersInList = (unsigned int)ReadBuf[0];
			while (BannersInList > 0)
			{
                ReadedLine = fread(&ReadBuf[0], BANNER_HEADER_SIZE, 1, HandleCfg);
				if (!ReadedLine)
				{
                    printf("Failed to load bunner header in list part of server configuration\r\n");
					break;
				}
				LoadBannerPtr = (BANNER_INFO*)AllocateMemory(sizeof(BANNER_INFO));
				if (!LoadBannerPtr)
				{
					printf("Failed to header memory allocation for bunner\r\n");
					break;
				}
                DecompPtr = &ReadBuf[0];
				LoadBannerPtr->isActive = *DecompPtr++;
				LoadBannerPtr->Location = *DecompPtr++;
	            memcpy(&LoadBannerPtr->Name[0], DecompPtr, (MAX_LEN_BANNER_NAME+1));
	            DecompPtr += (MAX_LEN_BANNER_NAME+1);
				DecompPtr = Uint32Unpack(DecompPtr, &BannerBodyLen);
                LoadBannerPtr->BodyPtr = (char*)AllocateMemory((BannerBodyLen*sizeof(char))+1);
				if (!LoadBannerPtr->BodyPtr)
				{
					printf("Failed to body memory allocation for bunner\r\n");
					FreeMemory(LoadBannerPtr);
					break;
				}
				ReadedLine = fread(LoadBannerPtr->BodyPtr, BannerBodyLen, 1, HandleCfg);
				if (!ReadedLine)
				{
                    printf("Failed to bunner body read from configuration file\r\n");
					FreeMemory(LoadBannerPtr->BodyPtr);
					FreeMemory(LoadBannerPtr);
					break;
				}
				LoadBannerPtr->BodyPtr[BannerBodyLen] = 0;
				LoadBannerPtr->ObjPtr = AddStructListObj(&ParServPtr->BannerList, LoadBannerPtr);
				BannersInList--;
			}
            /* Load SMS configuration information */
			ReadedLine = fread(&ReadBuf[0], SMS_CONFIG_INFO_LEN, 1, HandleCfg);
		    if (!ReadedLine)
			{
                printf("Failed to load SMS configuration general part of server configuration\r\n");
				isNeedsCreate = true;
			    break;
			}
            DecompPtr = &ReadBuf[0];
			DecompPtr = Uint32Unpack(DecompPtr, &ParServPtr->SmsWorker.SmsClientCfg.SmsIpPort);
			DecompPtr = Uint32Unpack(DecompPtr, &ParServPtr->SmsWorker.SmsClientCfg.SmsTimeout);
	        memcpy(&ParServPtr->SmsWorker.SmsClientCfg.SmsServerName[0], DecompPtr, (MAX_LEN_SMS_SERVER_NAME+1));
	        DecompPtr += (MAX_LEN_SMS_SERVER_NAME+1);
	        memcpy(&ParServPtr->SmsWorker.SmsClientCfg.AccessId[0], DecompPtr, (MAX_LEN_SMS_SERVER_ACCESS_ID+1));
	        DecompPtr += (MAX_LEN_SMS_SERVER_ACCESS_ID+1);
	        memcpy(&ParServPtr->SmsWorker.SmsClientCfg.SmsSrcName[0], DecompPtr, (MAX_LEN_SMS_SRC_LEN+1));
	        DecompPtr += (MAX_LEN_SMS_SRC_LEN+1);
            
            ParServPtr->SmsWorker.SmsClientCfg.isUsedProxy = (bool)*DecompPtr++;
			DecompPtr = Uint32Unpack(DecompPtr, &ParServPtr->SmsWorker.SmsClientCfg.ProxyServPort);
	        memcpy(&ParServPtr->SmsWorker.SmsClientCfg.ProxyServAddr[0], DecompPtr, (MAX_LEN_PROXY_ADDR+1));
	        DecompPtr += (MAX_LEN_PROXY_ADDR+1);                        
            PhoneNumInList = (unsigned int)(*DecompPtr++);
			while (PhoneNumInList > 0)
			{
                ReadedLine = fread(&ReadBuf[0], SMS_SENT_PHONE_INFO_LEN, 1, HandleCfg);
				if (!ReadedLine)
				{
                    printf("Failed to load SMS phone number record of server configuration\r\n");
					break;
				}
				LoadPhoneNumPtr = (SENTSMSNUM*)AllocateMemory(sizeof(SENTSMSNUM));
				if (!LoadPhoneNumPtr)
				{
					printf("Failed to header memory allocation for bunner\r\n");
					break;
				}
                DecompPtr = &ReadBuf[0];
				LoadPhoneNumPtr->isRusLangInfo = *DecompPtr++;
	            memcpy(&LoadPhoneNumPtr->PhoneNum[0], DecompPtr, (MAX_LEN_SMS_PHONE_NUM+1));
	            DecompPtr += (MAX_LEN_SMS_PHONE_NUM+1);
				LoadPhoneNumPtr->ObjPtr = AddStructListObj(&ParServPtr->SmsWorker.SmsClientCfg.SmsDestNumList, LoadPhoneNumPtr);
				PhoneNumInList--;
			}
			break;
		}
		fclose(HandleCfg);
		if (isNeedsCreate) ServerConfigSave(ParServPtr);
	}
	else
	{
		/* Default configuration save since config. file is absent */
		ServerConfigSave(ParServPtr);
	}
}
//---------------------------------------------------------------------------
void ServerConfigSave(PARAMWEBSERV *ParServPtr)
{
	unsigned int    BannerElmLen = 0;
	unsigned int    BannerBodyLen = 0;
	FILE            *HandleCfg;
	BANNER_INFO     *SelBannerPtr = NULL;
	SENTSMSNUM      *SelPhoneNumPtr = NULL;
	char            *CwdRet = NULL;
	unsigned char   *CompPtr = NULL;
	ObjListTask     *SelObjPtr = NULL;
	char            PathFile[1024];
    unsigned char   SaveBuf[4096];

	CwdRet = getcwd(PathFile, 512);
#ifdef _VCL60ENV_
    strcat(PathFile, "\\");
#else
    strcat(PathFile, "/");
#endif
    strcat(PathFile, LogsGrpName);    
	strcat(PathFile, ServerConfigExt);

	if ((HandleCfg = fopen(PathFile,"wb" )) != NULL)
	{
		/* Config. file identity and version fields store */
        CompPtr = &SaveBuf[0];
        CompPtr = Uint32Pack(CompPtr, SITE_CONFIG_CHECK_ID);
		CompPtr = Uint32Pack(CompPtr, CURR_CONFIG_VER);
		fwrite(&SaveBuf[0], SITE_ID_VER_IDENTITY_LEN, 1, HandleCfg);

		/* Config. file SMTP client information store */
		CompPtr = &SaveBuf[0];
        *CompPtr++ = ParServPtr->MailWorker.MailClientCfg.MailClientUse;
        *CompPtr++ = ParServPtr->MailWorker.MailClientCfg.SmtpEncodeType;
        CompPtr = Uint32Pack(CompPtr, ParServPtr->MailWorker.MailClientCfg.SmtpIpPort);
		CompPtr = Uint32Pack(CompPtr, ParServPtr->MailWorker.MailClientCfg.SmtpTimeout);
		CompPtr = Uint32Pack(CompPtr, ParServPtr->MailWorker.MailClientCfg.MailSendInt);
	    memcpy(CompPtr, &ParServPtr->MailWorker.MailClientCfg.SmtpServerName[0], (MAX_LEN_SERVER_NAME+1));
		CompPtr += (MAX_LEN_SERVER_NAME+1);
	    memcpy(CompPtr, &ParServPtr->MailWorker.MailClientCfg.MailFrom[0], (MAX_LEN_SERVER_EMAIL+1));
		CompPtr += (MAX_LEN_SERVER_EMAIL+1);
	    memcpy(CompPtr, &ParServPtr->MailWorker.MailClientCfg.MailLogin[0], (MAX_LEN_SERVER_LOGIN+1));
		CompPtr += (MAX_LEN_SERVER_LOGIN+1);
	    memcpy(CompPtr, &ParServPtr->MailWorker.MailClientCfg.MailPasswd[0], (MAX_LEN_SERVER_PASSWD+1));
		CompPtr += (MAX_LEN_SERVER_PASSWD+1);
		fwrite(&SaveBuf[0], MAIL_CONFIG_PACK_SIZE_V3, 1, HandleCfg);

		/* Config. file general config. information store */
		CompPtr = &SaveBuf[0];
        CompPtr = Uint32Pack(CompPtr, ParServPtr->GeneralCfg.MaxOpenSessions);
		CompPtr = Uint32Pack(CompPtr, ParServPtr->GeneralCfg.MaxSesionPerIP);
		CompPtr = Uint32Pack(CompPtr, ParServPtr->GeneralCfg.MinHtmlSizeCompress);
		CompPtr = Uint32Pack(CompPtr, ParServPtr->GeneralCfg.KeepAliveTimeout);
        *CompPtr++ = (unsigned char)ParServPtr->GeneralCfg.HtmlPageComprssEnable;
		*CompPtr++ = (unsigned char)ParServPtr->GeneralCfg.KeepAliveEnable;
		fwrite(&SaveBuf[0], GENERAL_CONFIG_PACK_SIZE, 1, HandleCfg);

		/* Config. file shop information. information store */
		CompPtr = &SaveBuf[0];
		CompPtr = Uint32Pack(CompPtr, ParServPtr->ShopInfoCfg.ZipCode);
	    memcpy(CompPtr, &ParServPtr->ShopInfoCfg.Name[0], (MAX_LEN_COMPANY_NAME+1));
		CompPtr += (MAX_LEN_COMPANY_NAME+1);
	    memcpy(CompPtr, &ParServPtr->ShopInfoCfg.URL[0], (MAX_LEN_URL_SERVER+1));
		CompPtr += (MAX_LEN_URL_SERVER+1);
	    memcpy(CompPtr, &ParServPtr->ShopInfoCfg.Region[0], (MAX_LEN_REGION_NAME+1));
		CompPtr += (MAX_LEN_REGION_NAME+1);
	    memcpy(CompPtr, &ParServPtr->ShopInfoCfg.City[0], (MAX_LEN_CITY_NAME+1));
		CompPtr += (MAX_LEN_CITY_NAME+1);
	    memcpy(CompPtr, &ParServPtr->ShopInfoCfg.Address[0], (MAX_LEN_ADDR_1_NAME+1));
		CompPtr += (MAX_LEN_ADDR_1_NAME+1);
	    memcpy(CompPtr, &ParServPtr->ShopInfoCfg.LandPhone[0], (MAX_LEN_PHONE_NUM+1));
		CompPtr += (MAX_LEN_PHONE_NUM+1);
	    memcpy(CompPtr, &ParServPtr->ShopInfoCfg.MobilePhone1[0], (MAX_LEN_PHONE_NUM+1));
		CompPtr += (MAX_LEN_PHONE_NUM+1);
	    memcpy(CompPtr, &ParServPtr->ShopInfoCfg.MobilePhone2[0], (MAX_LEN_PHONE_NUM+1));
		CompPtr += (MAX_LEN_PHONE_NUM+1);
	    memcpy(CompPtr, &ParServPtr->ShopInfoCfg.FaxPhone[0], (MAX_LEN_PHONE_NUM+1));
		CompPtr += (MAX_LEN_PHONE_NUM+1);
	    memcpy(CompPtr, &ParServPtr->ShopInfoCfg.LocLatitude[0], (MAX_LOCATION_LEN+1));
		CompPtr += (MAX_LOCATION_LEN+1);
	    memcpy(CompPtr, &ParServPtr->ShopInfoCfg.LocLongitude[0], (MAX_LOCATION_LEN+1));
		CompPtr += (MAX_LOCATION_LEN+1);
		fwrite(&SaveBuf[0], SHOP_INFO_CONFIG_PACK_SIZE, 1, HandleCfg);

		/* Config. file banners list information. information store */
		CompPtr = &SaveBuf[0];
        *CompPtr = (unsigned char)ParServPtr->BannerList.Count;
		BannerElmLen = 1;
        fwrite(&SaveBuf[0], BannerElmLen, 1, HandleCfg);

	    SelObjPtr = (ObjListTask*)GetFistObjectList(&ParServPtr->BannerList);
	    while(SelObjPtr)
		{
	        SelBannerPtr = (BANNER_INFO*)SelObjPtr->UsedTask;
			BannerBodyLen = (unsigned int)strlen(SelBannerPtr->BodyPtr);
            CompPtr = &SaveBuf[0];
			*CompPtr++ = (unsigned char)SelBannerPtr->isActive;
            *CompPtr++ = (unsigned char)SelBannerPtr->Location;
			BannerElmLen = 2;
	        memcpy(CompPtr, &SelBannerPtr->Name[0], (MAX_LEN_BANNER_NAME+1));
		    CompPtr += (MAX_LEN_BANNER_NAME+1);
            BannerElmLen += (MAX_LEN_BANNER_NAME+1);
            CompPtr = Uint32Pack(CompPtr, BannerBodyLen);
			BannerElmLen += UINT_PACK_SIZE;
            memcpy(CompPtr, SelBannerPtr->BodyPtr, BannerBodyLen);
			CompPtr += BannerBodyLen;
			BannerElmLen += BannerBodyLen;
            fwrite(&SaveBuf[0], BannerElmLen, 1, HandleCfg);

            SelObjPtr = (ObjListTask*)GetNextObjectList(&ParServPtr->BannerList);
		}

		/* Config. file SMS delivery information store */
		CompPtr = &SaveBuf[0];
        CompPtr = Uint32Pack(CompPtr, ParServPtr->SmsWorker.SmsClientCfg.SmsIpPort);
		CompPtr = Uint32Pack(CompPtr, ParServPtr->SmsWorker.SmsClientCfg.SmsTimeout);
	    memcpy(CompPtr, &ParServPtr->SmsWorker.SmsClientCfg.SmsServerName[0], (MAX_LEN_SMS_SERVER_NAME+1));
		CompPtr += (MAX_LEN_SMS_SERVER_NAME+1);
	    memcpy(CompPtr, &ParServPtr->SmsWorker.SmsClientCfg.AccessId[0], (MAX_LEN_SMS_SERVER_ACCESS_ID+1));
		CompPtr += (MAX_LEN_SMS_SERVER_ACCESS_ID+1);
	    memcpy(CompPtr, &ParServPtr->SmsWorker.SmsClientCfg.SmsSrcName[0], (MAX_LEN_SMS_SRC_LEN+1));
		CompPtr += (MAX_LEN_SMS_SRC_LEN+1);
        *CompPtr++ = (unsigned char)ParServPtr->SmsWorker.SmsClientCfg.isUsedProxy;
        CompPtr = Uint32Pack(CompPtr, ParServPtr->SmsWorker.SmsClientCfg.ProxyServPort);
	    memcpy(CompPtr, &ParServPtr->SmsWorker.SmsClientCfg.ProxyServAddr[0], (MAX_LEN_PROXY_ADDR+1));
		CompPtr += (MAX_LEN_SMS_SERVER_NAME+1);
        
        *CompPtr++ = (unsigned char)ParServPtr->SmsWorker.SmsClientCfg.SmsDestNumList.Count;
		fwrite(&SaveBuf[0], SMS_CONFIG_INFO_LEN, 1, HandleCfg);
        
	    SelObjPtr = (ObjListTask*)GetFistObjectList(&ParServPtr->SmsWorker.SmsClientCfg.SmsDestNumList);
	    while(SelObjPtr)
		{
			SelPhoneNumPtr = (SENTSMSNUM*)SelObjPtr->UsedTask;
            CompPtr = &SaveBuf[0];
			*CompPtr++ = (unsigned char)SelPhoneNumPtr->isRusLangInfo;
	        memcpy(CompPtr, &SelPhoneNumPtr->PhoneNum[0], (MAX_LEN_SMS_PHONE_NUM+1));
			CompPtr += (MAX_LEN_SMS_PHONE_NUM+1);
            fwrite(&SaveBuf[0], SMS_SENT_PHONE_INFO_LEN, 1, HandleCfg);
            SelObjPtr = (ObjListTask*)GetNextObjectList(&ParServPtr->SmsWorker.SmsClientCfg.SmsDestNumList);
		}
	    fclose(HandleCfg);
	}
	else
	{
		printf("Failed to save server configuration\r\n");
	}
}
//---------------------------------------------------------------------------
void SetKeyParCheckScript(char *Key)
{
    AddStrWebPage("if (form.");
	AddStrWebPage( Key);
	AddStrWebPage(".value == \"\") {\r\n");
	AddStrWebPage("document.getElementById('");
    AddStrWebPage( Key);
	AddStrWebPage("_div').style.color = \"red\";\r\n");
	AddStrWebPage("isvalid = false;fail_reason = 1;}\r\n");
	AddStrWebPage("else {document.getElementById('");
    AddStrWebPage( Key);
	AddStrWebPage("_div').style.color = \"white\";}\r\n\r\n");
}
//---------------------------------------------------------------------------
void SetEditStringLineWebPage(char *Key, unsigned int HeadlineId,
	char *SrcLine, int MaxLineLen, int LineEditSize, int HeadWith, bool isPasswd)
{
	char StrBuf[64];

	sprintf(StrBuf, "<tr><td width=\"%d%%\">\r\n", HeadWith);
	AddStrWebPage(StrBuf);
	AddStrWebPage("<div id=\"");
	AddStrWebPage( Key);
	AddStrWebPage("_div\" >\r\n");
    AddStrWebPage("<label for=\"");
	AddStrWebPage( Key);
	AddStrWebPage("\">\r\n");
    SetRusTextBuf(NULL, HeadlineId);
    AddStrWebPage("</label></div>\r\n");
	AddStrWebPage("</td>\r\n");
	AddStrWebPage("<td><input class=\"inputbox\" type=\"");
	if (isPasswd) AddStrWebPage("password");
	else          AddStrWebPage("text");
	AddStrWebPage("\" name=\"");
    AddStrWebPage( Key);		
	AddStrWebPage("\" size=\"");
	sprintf(StrBuf, "%d", LineEditSize);
	AddStrWebPage(StrBuf);
	AddStrWebPage("\" value=\"");
	AddStrWebPage( SrcLine);
	AddStrWebPage("\" maxlength=\"");
	sprintf(StrBuf, "%d\" >\r\n", MaxLineLen);
	AddStrWebPage(StrBuf);
	AddStrWebPage("</td></tr>\r\n\r\n");
}
//---------------------------------------------------------------------------
void SetEditIntLineWebPage(char *Key, unsigned int HeadlineId,
	int Value, int MaxLineLen, int LineEditSize, int HeadWith, bool isPasswd)
{
	char StrBuf[64];

	sprintf(StrBuf, "<tr><td width=\"%d%%\">\r\n", HeadWith);
	AddStrWebPage(StrBuf);
	AddStrWebPage("<div id=\"");
	AddStrWebPage( Key);
	AddStrWebPage("_div\" >\r\n");
    AddStrWebPage("<label for=\"");
	AddStrWebPage( Key);
	AddStrWebPage("\">\r\n");
    SetRusTextBuf(NULL, HeadlineId);
    AddStrWebPage("</label></div>\r\n");
	AddStrWebPage("</td>\r\n");
	AddStrWebPage("<td><input class=\"inputbox\" type=\"");
	if (isPasswd) AddStrWebPage("password");
	else          AddStrWebPage("text");
	AddStrWebPage("\" name=\"");
    AddStrWebPage( Key);		
	AddStrWebPage("\" size=\"");
	sprintf(StrBuf, "%d", LineEditSize);
	AddStrWebPage(StrBuf);
	sprintf(StrBuf, "\" value=\"%d\" ", Value);
	AddStrWebPage(StrBuf);
	AddStrWebPage("maxlength=\"");
	sprintf(StrBuf, "%d\" >\r\n", MaxLineLen);
	AddStrWebPage(StrBuf);
	AddStrWebPage("</td></tr>\r\n\r\n");
}
//---------------------------------------------------------------------------
void SetEditBoolLineWebPage(char *Key, unsigned int HeadlineId,
	bool Value, int HeadWith)
{
	char StrBuf[64];

	sprintf(StrBuf, "<tr><td width=\"%d%%\">\r\n", HeadWith);
	AddStrWebPage(StrBuf);
	AddStrWebPage("<div id=\"");
	AddStrWebPage(Key);
	AddStrWebPage("_div\" >\r\n");
    AddStrWebPage("<label for=\"");
	AddStrWebPage(Key);
	AddStrWebPage("\">\r\n");
    SetRusTextBuf(NULL, HeadlineId);
    AddStrWebPage("</label></div>\r\n");
	AddStrWebPage("</td>\r\n");
	AddStrWebPage("<td><input type=\"checkbox\" name=\"");
    AddStrWebPage(Key);
	AddStrWebPage("\" id=\"");
	AddStrWebPage(Key);
	AddStrWebPage("\" class=\"inputbox\" value=\"1\" alt=\"");
	SetRusTextBuf(NULL, HeadlineId);
    AddStrWebPage("\"");
	if (Value) AddStrWebPage(" checked"); 
	AddStrWebPage(">\r\n</td></tr>\r\n\r\n");
}
//---------------------------------------------------------------------------
void SetHiddenServCfgPar(USER_SESSION *SessionPtr, unsigned int Operation)
{
    /* Add secure key for HTML page handle. */
	SetHiddenIntParForm(NULL, SecKeyId, SessionPtr->SecureKey);

    /* Add session id for HTML page handle. */
	SetHiddenStrParForm(NULL, FormKeySessionId, SessionPtr->SesionIdKey);

    /* Add user id for HTML page handle. */
	SetHiddenIntParForm(NULL, FormKeyUserId, SessionPtr->UserPtr->UserId);

	/* Add configuration change request for HTML page handle. */
	SetHiddenIntParForm(NULL, FormKeyCfgChgId, Operation);

	/* Add configuration change request for HTML page handle. */
	SetHiddenIntParForm(NULL, KeySelEditGrp, SessionPtr->SelConfigEditGrpId);
}
//---------------------------------------------------------------------------
void PageCreateServShutdownPageGen(char *BufAnsw, USER_SESSION *SessionPtr)
{
	AddBeginPageShopWebPage(BufAnsw, SessionPtr);
    EndHtmlPageGenPtr = &BufAnsw[strlen(BufAnsw)];

	AddStrWebPage("<div class=\"componentheading\">");
    SetRusTextBuf(NULL, SITE_RUS_SERV_SHUT_RES_LINE_ID);
	AddStrWebPage("</div>\r\n");
    AddStrWebPage("<br>\r\n");
    AddStrWebPage("<span style=\"text-align:left;font-weight: bold;font-size: 20px;color: green;\">");
    SetRusTextBuf(NULL, SITE_RUS_SERV_SHUT_SUCC_LINE_ID);
    AddStrWebPage("</span><br>\r\n");                  
	AddEndPageShopWebPage(BufAnsw, SessionPtr);
}
//---------------------------------------------------------------------------
