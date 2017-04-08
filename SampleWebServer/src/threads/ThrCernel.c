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
#include "ThrCernel.h"
#include "SysLibTool.h"
#include "TrTimeOutThread.h"
#include "SysWebFunction.h"
#include "SysMessages.h"

unsigned int gCloseCernelFlag;
extern char ServerVersion[];
extern char WebServerName[];
extern char OwnerCompany[];
extern char SystemName[];

WEB_SERVER_CUST_CFG_INFO WebCustomCfg;
SAMPLE_SERVER_CUSTOM_CONFIG SampleCustomCfg;

char MsgInfoDelimer[]    = {"***************************************************************\n"};
char MsgReadyServUse[]   = {"Web Server is ready for HTTP requests handle\n"};

//---------------------------------------------------------------------------
void THRCernelTool(DEFSYSCERN *CernelTool)
{
	char          *CwdRet = NULL;
    SERVER_CUSTOM_CONFIG *CustCfgPtr = NULL;
    struct timespec tim;

    CwdRet = getcwd(CernelTool->StartPath,512);
    CustCfgPtr = &CernelTool->ParWebServer.ServCustomCfg;
    WebCustomCfg.BaseServCustCfg = CustCfgPtr;
    WebCustomCfg.ExtCustCfgPtr = (void*)&SampleCustomCfg;
    CustomConfigInit(&WebCustomCfg);
    SampleCustCfgInit(&WebCustomCfg);
    CustomConfigLoad(&WebCustomCfg);

    sprintf(WebServerName, "%sWebServer", SystemName);
    
    printf("\n%s", MsgInfoDelimer);
    printf(" %s - Version: %s\n", WebServerName, ServerVersion);    
    printf(" Copyright (c) 2017 %s\n", OwnerCompany);
    printf("%s", MsgInfoDelimer);

    // The start of system timer process.
    if (TimerThreadCreate() != 0)
    {
        printf("\n%s Failed to init timer's threaddue to error\n", SetTimeStampLine());
        return;
    }

    if (ReportThreadCreate() == -1)
    {
        printf("\n%s Failed to init report manager\n", SetTimeStampLine());
        return;
    }

    printf("Report meneger process started.\r\n");
 
    /* The start of WWW server process. */
    CernelTool->ParWebServer.isStartDone = false;
    WebServThreadCreate(&CernelTool->ParWebServer, TM_ONTIMEOUT);
    if (!CernelTool->ParWebServer.isStartDone)
    {
        printf("\n%s Failed to init web server thread\n", SetTimeStampLine());
        WebServThreadClose(&CernelTool->ParWebServer);       
        ReportThreadClose();
        TimerThreadClose();
        return;
    }

    printf("%s", MsgInfoDelimer);    
    printf("       %s %s", SystemName, MsgReadyServUse);    
    printf("%s", MsgInfoDelimer);
    printf(" Prim. Web server IP port:         %d\r\n",CernelTool->ParWebServer.ServCustomCfg.PrimWebAccIPPort);
	printf(" Prim. ext. web server IP port:    %d\r\n",CernelTool->ParWebServer.ServCustomCfg.PrimExtServIPPort);
    if (CernelTool->ParWebServer.ServCustomCfg.SecWebPort)
    {
        printf(" Second. Web server IP port:       %d\r\n",CernelTool->ParWebServer.ServCustomCfg.SecondWebAccIPPort );
	    printf(" Second. ext. web server IP port:  %d\r\n",CernelTool->ParWebServer.ServCustomCfg.SecondExtServIPPort );    
    }
    printf("%s\r\n", MsgInfoDelimer);

    /*********** Mine Cernel Execute *************/
    for(;;)
    {
        if (gCloseCernelFlag > 0) break;
        tim.tv_sec = 0;
        tim.tv_nsec = 500000000L; /* 500 ms check interval */
        if (nanosleep(&tim, (struct timespec*)NULL) < 0)                
            printf("Cernel nano sleep system call processing error %d.\n", errno);
    }   
    WebServThreadClose(&CernelTool->ParWebServer);   
    ReportThreadClose();
    TimerThreadClose();     
}
//---------------------------------------------------------------------------
