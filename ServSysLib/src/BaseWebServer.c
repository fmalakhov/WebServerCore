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

#ifdef _LINUX_X86_
#include <sys/syscall.h>
#endif
#include "BaseWebServer.h"
#include "ThrReportMen.h"
#include "HtmlTemplateParser.h"

extern ListItsTask  MobileDeviceList;
extern WebServChannel  gWebServMsgChannel;
extern char ThrWebServName[];
extern unsigned int HtmlPgHashEntityCnt;
extern unsigned int HtmlPgRecInHashCnt;
//---------------------------------------------------------------------------
bool MainWebReqHandler(PARAMWEBSERV *ParWebServ, unsigned int SysShowHostIP)
{
    WebServMessage  *WebServMsgPtr = NULL;
    char            LogBuf[64];

    printf("Trust system show host: ");
    if (SysShowHostIP > 0)
    {
        SetIpAddrToString(LogBuf, SysShowHostIP);
        printf("%s\r\n", LogBuf);
    }
    else
    {
        printf("N/A\r\n");
    }

    ServSysLibVersionShow();
    
    /* initialize SSL library and register algorithms */
    if(SSL_library_init() < 0)
    {       
        printf("Could not initialize the OpenSSL library\r\n");
	    WebConnChanClose(ParWebServ);
		return false;
    }
    
    /* Open SSL license information */
    printf("\n%s\n", OPENSSL_VERSION_TEXT);
    printf("Copyright (c) 1998-2016 The OpenSSL Project.  All rights reserved.\n");
    printf("This product includes software developed by the OpenSSL Project\n");
    printf("for use in the OpenSSL Toolkit. (http://www.openssl.org/)\n");

#ifdef WIN32
    printf("WEB server task was successfuly run.\n");
	WinThreadMsgSend(ParWebServ->ThrAnswStart, ParWebServ->IDAnswStart, 1, 0 );
#else
    printf("WEB server task (%u) was successfuly run.\n", (unsigned int)syscall(SYS_gettid));
	ParWebServ->isStartDone = true;
	sem_post(&ParWebServ->WebServInfo.semx);
#endif

    DebugLogPrint(NULL, "%s: *************** %s Server was successfully started *****************\r\n", 
		ThrWebServName, ParWebServ->ShopInfoCfg.Name);

	EventLogPrint(NULL, "%s: *************** %s Server was successfully started *****************\r\n", 
		ThrWebServName, ParWebServ->ShopInfoCfg.Name);

    /* Read the data from the WEB server messages queue. */
    while (GetWebServMessageQueue(&gWebServMsgChannel.WebServQueue, (void**)&WebServMsgPtr))
    {
        /* Process the external thread report message */
	    HandlerWebServerReq(ParWebServ, &WebServMsgPtr->Msg);
        FreeWebServMsgPool(&gWebServMsgChannel, WebServMsgPtr);
		if (ParWebServ->StopServReq) break;    
    }

    DebugLogPrint(NULL,"%s: Web Server manager thread stop was initiated\n", ThrWebServName);
    WebConnChanClose(ParWebServ); 
    
    if ((ParWebServ->SmsWorker.SmsSendThrList.Count > 0) ||
		(ParWebServ->MailWorker.MailSendThrList.Count > 0) ||
		(ParWebServ->SenderWorker.HttpSendThrList.Count > 0) ||
		(GetNumberActiveReaders(&ParWebServ->ReaderWorker) > 0))
	{
        /* Some not finished tasks are present, needs to wait close them */
        while (GetWebServMessageQueue(&gWebServMsgChannel.WebServQueue, (void**)&WebServMsgPtr))
        {
            /* Process the external thread report message */
            if (!HandlerStopServerReq(ParWebServ, &WebServMsgPtr->Msg)) break;
            FreeWebServMsgPool(&gWebServMsgChannel, WebServMsgPtr);
        }
	}

	DebugLogPrint(NULL, "%s: All active Send, Receive, Mail and SNS are close - Active stop is initiated\n", ThrWebServName);
	StatsDataSave();
	Sleep(20);
	return true;
}
//---------------------------------------------------------------------------
void SetAddrConnectChan(PARAMWEBSERV *ParWebServ)
{
    ParWebServ->PrimLocalIPAddrServ = GetLocalIPServer(ParWebServ->ServCustomCfg.PrimLocalIPAddrServ, 
        NULL, ParWebServ->PrimLocalHostName);
    if (strlen(ParWebServ->ServCustomCfg.PrimLocalHostName) > 0)
        strcpy(ParWebServ->PrimLocalHostName, ParWebServ->ServCustomCfg.PrimLocalHostName);
    SetIpAddrToString(ParWebServ->LocalAddrIP, ParWebServ->PrimLocalIPAddrServ);
    
    if (ParWebServ->ServCustomCfg.SecWebPort)
    {
        ParWebServ->SecondLocalIPAddrServ = GetLocalIPServer(ParWebServ->ServCustomCfg.SecondLocalIPAddrServ, 
            NULL, ParWebServ->SecondLocalHostName);
        if (strlen(ParWebServ->ServCustomCfg.SecondLocalHostName) > 0)
            strcpy(ParWebServ->SecondLocalHostName, ParWebServ->ServCustomCfg.SecondLocalHostName);
    }

    if ((ParWebServ->ServCustomCfg.ForwardIpPort > 0) && 
	    (strlen(ParWebServ->ServCustomCfg.ForwardUrl) > 0))
    {
        ParWebServ->ForwardLocalIPAddrServ = GetLocalIPServer(ParWebServ->ServCustomCfg.ForwardLocalIPAddrServ, 
            NULL, ParWebServ->ForwardLocalHostName);
        if (strlen(ParWebServ->ServCustomCfg.ForwardLocalHostName) > 0)
            strcpy(ParWebServ->ForwardLocalHostName, ParWebServ->ServCustomCfg.ForwardLocalHostName);
    }
}
//---------------------------------------------------------------------------
void ConnChanInfoInit(PARAMWEBSERV *ParWebServ)
{
	ParWebServ->PrimConnWeb.UserConnectCount = 0;
	ParWebServ->PrimConnWeb.ConnWebCloseReq = false;
    ParWebServ->PrimConnWeb.IDMessConnWeb = WSU_CONNECTUSER;
    ParWebServ->PrimConnWeb.IDMessCloseCWeb = WSU_CLOSECONNWEB;
    ParWebServ->PrimConnWeb.IPPortUserWEB = ParWebServ->ServCustomCfg.PrimWebAccIPPort;
    ParWebServ->PrimConnWeb.IPLocalAddress = ParWebServ->PrimLocalIPAddrServ;
#ifdef WIN32
    ParWebServ->PrimConnWeb.IDAnswStart = WSU_RES_START_NET_CONN;
#endif
    ParWebServ->PrimConnWeb.isStartDone = false;
	ParWebServ->PrimConnWeb.isForfardChan = false;
    ParWebServ->PrimConnWeb.WebServerSocket = -1;
    ParWebServ->PrimConnWeb.ReaderWorkerPtr = &ParWebServ->ReaderWorker;
    ParWebServ->PrimConnWeb.WebChanId = PRIMARY_WEB_CHAN;
	ParWebServ->PrimConnWeb.TimeOut = ParWebServ->GeneralCfg.KeepAliveTimeout;
	ParWebServ->PrimConnWeb.isKeepAlive = ParWebServ->GeneralCfg.KeepAliveEnable;
    ParWebServ->PrimConnWeb.isDdosProtect = ParWebServ->ServCustomCfg.PrimPortDdosProtect;
	ParWebServ->PrimConnWeb.isContentDelivery = ParWebServ->ServCustomCfg.PrimContentDelivery;
	if (ParWebServ->ServCustomCfg.PrimContentDelivery && (strlen(ParWebServ->ServCustomCfg.PrimContentRootDir) > 0))
	{
	    ParWebServ->PrimConnWeb.ServRootDir = &ParWebServ->ServCustomCfg.PrimContentRootDir[0];
	}
	else
	{
	    ParWebServ->PrimConnWeb.ServRootDir = &ParWebServ->StartPath[0];
	}
	ParWebServ->PrimConnWeb.MobDevListPtr = &MobileDeviceList;
    ParWebServ->PrimConnWeb.WebServIpPort = ParWebServ->ServCustomCfg.WebServMsgPort;
    ParWebServ->PrimConnWeb.isHttpSecure = ParWebServ->ServCustomCfg.PrimPortHttpSecure;
    ParWebServ->PrimConnWeb.ServerKeyFile = ParWebServ->ServCustomCfg.PrimPortKeyFile;
    ParWebServ->PrimConnWeb.SertificateFile = ParWebServ->ServCustomCfg.PrimPortSertFile;
    printf("Local IP addr for primary connect thread: 0x%08x\n", ParWebServ->PrimConnWeb.IPLocalAddress);
 
    /* Secondary WEB channel creation */
    if (ParWebServ->ServCustomCfg.SecWebPort)
    {
	    ParWebServ->SecondConnWeb.UserConnectCount = 0;
	    ParWebServ->SecondConnWeb.ConnWebCloseReq = false;
        ParWebServ->SecondConnWeb.IDMessConnWeb = WSU_CONNECTUSER;
        ParWebServ->SecondConnWeb.IDMessCloseCWeb = WSU_CLOSECONNWEB;
        ParWebServ->SecondConnWeb.IPPortUserWEB = ParWebServ->ServCustomCfg.SecondWebAccIPPort;
        ParWebServ->SecondConnWeb.IPLocalAddress = ParWebServ->SecondLocalIPAddrServ;
#ifdef WIN32
        ParWebServ->SecondConnWeb.IDAnswStart = WSU_RES_START_NET_CONN;
#endif
        ParWebServ->SecondConnWeb.isStartDone = false;
		ParWebServ->SecondConnWeb.isForfardChan = false;
        ParWebServ->SecondConnWeb.WebServerSocket = -1;
        ParWebServ->SecondConnWeb.ReaderWorkerPtr = &ParWebServ->ReaderWorker;
        ParWebServ->SecondConnWeb.WebChanId = SECONDARY_WEB_CHAN;
	    ParWebServ->SecondConnWeb.TimeOut = ParWebServ->GeneralCfg.KeepAliveTimeout;
	    ParWebServ->SecondConnWeb.isKeepAlive = ParWebServ->GeneralCfg.KeepAliveEnable;
        ParWebServ->SecondConnWeb.isDdosProtect = ParWebServ->ServCustomCfg.SecondPortDdosProtect;
		ParWebServ->SecondConnWeb.isContentDelivery = ParWebServ->ServCustomCfg.SecondContentDelivery;
	    if (ParWebServ->ServCustomCfg.SecondContentDelivery && (strlen(ParWebServ->ServCustomCfg.SecondContentRootDir) > 0))
	    {
	        ParWebServ->SecondConnWeb.ServRootDir = &ParWebServ->ServCustomCfg.PrimContentRootDir[0];
	    }
	    else
	    {
	        ParWebServ->SecondConnWeb.ServRootDir = &ParWebServ->StartPath[0];
	    }
	    ParWebServ->SecondConnWeb.ServRootDir = &ParWebServ->StartPath[0];
	    ParWebServ->SecondConnWeb.MobDevListPtr = &MobileDeviceList;
        ParWebServ->SecondConnWeb.WebServIpPort = ParWebServ->ServCustomCfg.WebServMsgPort;
        ParWebServ->SecondConnWeb.isHttpSecure = ParWebServ->ServCustomCfg.SecondPortHttpSecure;
        ParWebServ->SecondConnWeb.ServerKeyFile = ParWebServ->ServCustomCfg.SecondPortKeyFile;
        ParWebServ->SecondConnWeb.SertificateFile = ParWebServ->ServCustomCfg.SecondPortSertFile;
        printf("Local IP addr for secondary connect thread: 0x%08x\n", ParWebServ->SecondConnWeb.IPLocalAddress);    
    }

	if ((ParWebServ->ServCustomCfg.ForwardIpPort > 0) && 
	    (strlen(ParWebServ->ServCustomCfg.ForwardUrl)  > 0))
	{
	    ParWebServ->ForwardConnWeb.UserConnectCount = 0;
	    ParWebServ->ForwardConnWeb.ConnWebCloseReq = false;
        ParWebServ->ForwardConnWeb.IDMessConnWeb = WSU_CONNECTUSER;
        ParWebServ->ForwardConnWeb.IDMessCloseCWeb = WSU_CLOSECONNWEB;
        ParWebServ->ForwardConnWeb.IPPortUserWEB = ParWebServ->ServCustomCfg.ForwardIpPort;
        ParWebServ->ForwardConnWeb.IPLocalAddress = ParWebServ->ForwardLocalIPAddrServ;
#ifdef WIN32
        ParWebServ->ForwardConnWeb.IDAnswStart = WSU_RES_START_NET_CONN;
#endif
        ParWebServ->ForwardConnWeb.isStartDone = false;
		ParWebServ->ForwardConnWeb.isForfardChan = true;
        ParWebServ->ForwardConnWeb.WebServerSocket = -1;
        ParWebServ->ForwardConnWeb.ReaderWorkerPtr = &ParWebServ->ReaderWorker;
        ParWebServ->ForwardConnWeb.WebChanId = FORWARD_WEB_CHAN;
	    ParWebServ->ForwardConnWeb.TimeOut = ParWebServ->GeneralCfg.KeepAliveTimeout;
	    ParWebServ->ForwardConnWeb.isKeepAlive = ParWebServ->GeneralCfg.KeepAliveEnable;
        ParWebServ->ForwardConnWeb.isDdosProtect = ParWebServ->ServCustomCfg.ForwardPortDdosProtect;
	    ParWebServ->ForwardConnWeb.ServRootDir = &ParWebServ->StartPath[0];
	    ParWebServ->ForwardConnWeb.MobDevListPtr = &MobileDeviceList;
        ParWebServ->ForwardConnWeb.WebServIpPort = ParWebServ->ServCustomCfg.WebServMsgPort;
        ParWebServ->ForwardConnWeb.isHttpSecure = ParWebServ->ServCustomCfg.ForwardPortHttpSecure;
        ParWebServ->ForwardConnWeb.ServerKeyFile = ParWebServ->ServCustomCfg.ForwardPortKeyFile;
        ParWebServ->ForwardConnWeb.SertificateFile = ParWebServ->ServCustomCfg.ForwardPortSertFile;
        printf("Local IP addr for forward connect thread: 0x%08x\n", ParWebServ->ForwardConnWeb.IPLocalAddress); 	
	}
}
//---------------------------------------------------------------------------
bool WebConnChanOpen(PARAMWEBSERV *ParWebServ)
{
    char LogBuf[32];

    ConnWebThreadCreate(&ParWebServ->PrimConnWeb);
    if (!ParWebServ->PrimConnWeb.isStartDone) return false;

    if (ParWebServ->ServCustomCfg.SecWebPort)
    {
        ConnWebThreadCreate(&ParWebServ->SecondConnWeb);
        if (!ParWebServ->SecondConnWeb.isStartDone)
        {
	        ConnWebClose(&ParWebServ->PrimConnWeb);
            return false;
        }    
    }    
    
    if ((ParWebServ->ServCustomCfg.ForwardIpPort > 0) && 
	    (strlen(ParWebServ->ServCustomCfg.ForwardUrl)  > 0))
    {
        ConnWebThreadCreate(&ParWebServ->ForwardConnWeb);
        if (!ParWebServ->ForwardConnWeb.isStartDone)
        {
			if (ParWebServ->ServCustomCfg.SecWebPort) ConnWebClose(&ParWebServ->SecondConnWeb);
	        ConnWebClose(&ParWebServ->PrimConnWeb);
            return false;
        }    
    }
	
	SetIpAddrToString(ParWebServ->LocalAddrIP, ParWebServ->PrimConnWeb.LocalIPServer);
    strcpy(ParWebServ->MailWorker.MailClientCfg.LocalIpAddr, ParWebServ->LocalAddrIP);
    printf(" Server primary local IP address:   %s\r\n", ParWebServ->LocalAddrIP);
    if (strlen(ParWebServ->PrimLocalHostName))
    {
        printf(" Server primary local name:         %s\r\n", ParWebServ->PrimLocalHostName);
    }
    else
    {
        printf(" Server primary local name:         %s\r\n", ParWebServ->LocalAddrIP);
    }
    
    if (ParWebServ->ServCustomCfg.SecWebPort)
    {
        SetIpAddrToString(ParWebServ->LocalAddrIP, ParWebServ->SecondConnWeb.LocalIPServer);
        printf(" Second. Web server IP port:        %d\r\n",ParWebServ->ServCustomCfg.SecondWebAccIPPort );
	    printf(" Second. ext. web server IP port:   %d\r\n",ParWebServ->ServCustomCfg.SecondExtServIPPort );    
        printf(" Server secondary local IP address: %s\r\n", ParWebServ->LocalAddrIP);
        if (strlen(ParWebServ->SecondLocalHostName))
        {
            printf(" Server secondary local name:       %s\r\n", ParWebServ->SecondLocalHostName);
        }
        else
        {
            SetIpAddrToString(LogBuf, ParWebServ->SecondLocalIPAddrServ);
            printf(" Server secondary local name:       %s\r\n", LogBuf);
        }
    }
    
	if ((ParWebServ->ServCustomCfg.ForwardIpPort > 0) && 
	    (strlen(ParWebServ->ServCustomCfg.ForwardUrl) > 0))
	{
        printf(" Forward. Web server IP port:        %d\r\n",
		    ParWebServ->ServCustomCfg.ForwardIpPort);

		SetIpAddrToString(LogBuf, ParWebServ->ForwardConnWeb.IPLocalAddress);
        printf(" Server forward local IP address: %s\r\n", LogBuf);
        if (strlen(ParWebServ->ForwardLocalHostName) > 0)
	    {
	        printf(" Server forward local name:         %s\r\n",
			    ParWebServ->ForwardLocalHostName);
	    }
	    else
	    {
		    printf(" Server forward local name:         %s\r\n", LogBuf);
	    }  		
	}	
    return true;	
}
//---------------------------------------------------------------------------
void WebConnChanClose(PARAMWEBSERV *ParWebServ)
{
	ConnWebClose(&ParWebServ->PrimConnWeb);
    Sleep(20);
    if (ParWebServ->ServCustomCfg.SecWebPort)
    {
	    ConnWebClose(&ParWebServ->SecondConnWeb);
        Sleep(20);    
    }

    if ((ParWebServ->ServCustomCfg.ForwardIpPort > 0) && 
	    (strlen(ParWebServ->ServCustomCfg.ForwardUrl)  > 0))
    {
	    ConnWebClose(&ParWebServ->ForwardConnWeb);
        Sleep(20);    
    }

}
//---------------------------------------------------------------------------
void HtmlCmdHashInit(HTML_PAGE_HASH_CHAR_HOP *HtmlPageHashPtr, 
					 GROUP_HTML_HANDLERS* HtmlHandlePtr, char *Name)
{
	unsigned int i, j;
	unsigned int UserTypeMask = 0x01;
	HTML_HANDLER_ARRAY *HtmlHandlerArrPtr = NULL;

	for(i=0;i < HtmlHandlePtr->TotalGropus;i++)
	{
		HtmlHandlerArrPtr = &HtmlHandlePtr->HtmlHandlerArr[i];
	    for(j=0;j < HtmlHandlerArrPtr->AllPages;j++)
	    {
            if (!AddHtmlPageHash(HtmlPageHashPtr,
			     HtmlHandlerArrPtr->ArrPageHandler[j].HtmlPageName, 
			     UserTypeMask, &HtmlHandlerArrPtr->ArrPageHandler[j]))
			{
				break;
			}
	    }
		UserTypeMask <<= 1;
	}
	printf("HTML pages (%s) hash load is done (Entryes=%d, Pages=%d)\n", 
		Name, HtmlPgHashEntityCnt, HtmlPgRecInHashCnt);
}
//---------------------------------------------------------------------------
void HandleeTimeStampSetTimerExp(PARAMWEBSERV *WebServInfoPtr)
{
#ifdef WIN32
	SYSTEMTIME   CurrTime;
	DWORD        rc;
#else       
    struct timeb hires_cur_time;
    struct tm    *cur_time;     
#endif

	*WebServInfoPtr->ReqTimeStr = 0;
	*WebServInfoPtr->ExpDateCookieStr = 0;
	*WebServInfoPtr->LogTimeStampStr = 0;
	SetDateHeadRequest(WebServInfoPtr->ReqTimeStr);
	SetExpDateCookie(WebServInfoPtr->ExpDateCookieStr);
#ifdef WIN32
    if ((rc = WaitForSingleObject(gMemoryMutex, INFINITE)) == WAIT_FAILED)
	{
		printf("SetLogTimeStamp - Fail to get memory mutex\r\n");
        return;
	}
    GetSystemTime(&CurrTime);
    sprintf(WebServInfoPtr-->LogTimeStampStr, "%02d/%02d/%04d-%02d:%02d:%02d.%03d | ",
		CurrTime.wDay, CurrTime.wMonth, CurrTime.wYear,
		CurrTime.wHour, CurrTime.wMinute, 
		CurrTime.wSecond, CurrTime.wMilliseconds); 
    if (!ReleaseMutex(gMemoryMutex)) 
        printf("SetLogTimeStamp - Fail to release memory mutex\r\n");
#else
    ftime(&hires_cur_time);
    cur_time = localtime(&hires_cur_time.time);
    sprintf(WebServInfoPtr->LogTimeStampStr, "%02d/%02d/%04d-%02d:%02d:%02d.%03d | ",
           cur_time->tm_mday, (cur_time->tm_mon+1),
           (cur_time->tm_year+1900), cur_time->tm_hour,
           cur_time->tm_min, cur_time->tm_sec, hires_cur_time.millitm); 
#endif
}
//---------------------------------------------------------------------------
#ifdef _LINUX_X86_
void WebServThreadCreate(PARAMWEBSERV *WebServInfoPtr, unsigned int TimerExpMsgId)
{
    pthread_attr_t	attr, *attrPtr = &attr;
    struct sched_param	sched;

    WebServMsgApiInit(TimerExpMsgId);
    sem_init(&WebServInfoPtr->WebServInfo.semx, 0, 0);
	WebServInfoPtr->WebServInfo.StopTimerFlag = 0;
    pthread_attr_init(attrPtr);
    pthread_attr_setdetachstate(attrPtr, PTHREAD_CREATE_DETACHED);
    pthread_attr_setscope(attrPtr, PTHREAD_SCOPE_SYSTEM);
    if (pthread_attr_getschedparam(attrPtr, &sched) == 0)
    {
	    sched.sched_priority = 0;
	    pthread_attr_setschedparam(attrPtr, &sched);
    }	
    printf("\nWeb server thread sturtup\n");	
    if (pthread_create(&WebServInfoPtr->WebServInfo.event_thr, &attr, &THRWebServer, 
	    WebServInfoPtr) != 0)
    {
	    printf("Web server thread create with %d error!\n", errno);
        WebServMsgApiClose();
    }
	else
	{
        sem_wait(&WebServInfoPtr->WebServInfo.semx);
	}
}
//---------------------------------------------------------------------------
void WebServThreadClose(PARAMWEBSERV *ParWebServPtr)
{
    ParWebServPtr->WebServInfo.StopTimerFlag = 1;
    pthread_join(ParWebServPtr->WebServInfo.event_thr, NULL);
    sem_destroy(&ParWebServPtr->WebServInfo.semx);
    WebServMsgApiClose();
    return;
}
#endif
//---------------------------------------------------------------------------
unsigned int GetRealTimeMarker()
{
#ifndef WIN32
    struct timespec spec;
	
    clock_gettime(CLOCK_REALTIME, &spec);
	return (unsigned int)spec.tv_sec;
#endif
}
//---------------------------------------------------------------------------
