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
#include <dirent.h>
#endif
#include <sys/stat.h>
#include "BaseWebServer.h"
#include "ThrReportMen.h"

extern char ThrWebServName[];
extern PARAMWEBSERV *ParWebServPtr;
extern READWEBSOCK *ParReadHttpSocketPtr;
//---------------------------------------------------------------------------
void SetServerHttpAddr(char *BufAnsw)
{
	char StrLine[16];
	bool iSLocalPortSet = false;

    if (!ParWebServPtr) return;
	if (ParReadHttpSocketPtr && ParReadHttpSocketPtr->SslPtr)
	{
	    if (BufAnsw) strcat(BufAnsw, "https://");
	    else         AddStrWebPage("https://");
	}
	else
	{
	    if (BufAnsw) strcat(BufAnsw, "http://");
	    else         AddStrWebPage("http://");
	}
    
    if (ParReadHttpSocketPtr)
    {
        if (ParReadHttpSocketPtr->WebChanId == PRIMARY_WEB_CHAN)
        {
            SetIpAddrToString(ParWebServPtr->LocalAddrIP, ParWebServPtr->PrimConnWeb.LocalIPServer);
            if (strlen(ParWebServPtr->PrimLocalHostName) > 0)
	        {
		        /* Check for request from IP addresses 192.168.XX.XX */
        #ifdef WIN32
		        if ((unsigned int)(ParReadHttpSocketPtr->HttpClientIP & 0x0000ffff) == 0x0000a8c0)
        #else
          #ifdef _SUN_BUILD_
                if ((unsigned int)(ParReadHttpSocketPtr->HttpClientIP & 0xffff0000) == 0xc0a80000)
          #else
		        if ((unsigned int)(ParReadHttpSocketPtr->HttpClientIP & 0x0000ffff) == 0x0000a8c0)
          #endif
        #endif
		        {
			        StrLine[0] = 0;
			        if ((ParWebServPtr->ServCustomCfg.PrimWebAccIPPort != DEF_HTTP_IP_PORT) &&
						(ParWebServPtr->ServCustomCfg.PrimWebAccIPPort != DEF_HTTPS_IP_PORT))
				        sprintf(StrLine, ":%d", ParWebServPtr->ServCustomCfg.PrimWebAccIPPort);
			        if (BufAnsw) 
			        {
				        strcat(BufAnsw, ParWebServPtr->LocalAddrIP);
				        strcat(BufAnsw, StrLine);
			        }
			        else
			        {
				        AddStrWebPage(ParWebServPtr->LocalAddrIP);
				        AddStrWebPage(StrLine);
			        }
			        iSLocalPortSet = true;
		        }
		        else
		        {
		            if (BufAnsw) strcat(BufAnsw, ParWebServPtr->PrimLocalHostName);
			        else         AddStrWebPage(ParWebServPtr->PrimLocalHostName);
		        }
	        }
	        else
	        {
		        if (BufAnsw) strcat(BufAnsw, ParWebServPtr->LocalAddrIP);
		        else         AddStrWebPage(ParWebServPtr->LocalAddrIP);
	        }
	        if ((!iSLocalPortSet) && (ParWebServPtr->ServCustomCfg.PrimExtServIPPort != DEF_HTTP_IP_PORT) &&
				(ParWebServPtr->ServCustomCfg.PrimExtServIPPort != DEF_HTTPS_IP_PORT))
	        {
		        sprintf(StrLine, ":%d/", ParWebServPtr->ServCustomCfg.PrimExtServIPPort);
	        }
	        else
	        {
		        strcpy(StrLine, "/");
	        }
        }
        else
        {
            SetIpAddrToString(ParWebServPtr->LocalAddrIP, ParWebServPtr->SecondConnWeb.LocalIPServer);
            if (strlen(ParWebServPtr->SecondLocalHostName) > 0)
	        {
		        /* Check for request from IP addresses 192.168.XX.XX */
        #ifdef WIN32
		        if ((unsigned int)(ParReadHttpSocketPtr->HttpClientIP & 0x0000ffff) == 0x0000a8c0)
        #else
          #ifdef _SUN_BUILD_
                if ((unsigned int)(ParReadHttpSocketPtr->HttpClientIP & 0xffff0000) == 0xc0a80000)
          #else
		        if ((unsigned int)(ParReadHttpSocketPtr->HttpClientIP & 0x0000ffff) == 0x0000a8c0)
          #endif
        #endif
		        {
			        StrLine[0] = 0;
			        if ((ParWebServPtr->ServCustomCfg.SecondWebAccIPPort != DEF_HTTP_IP_PORT) &&
						(ParWebServPtr->ServCustomCfg.SecondWebAccIPPort != DEF_HTTPS_IP_PORT))
				        sprintf(StrLine, ":%d", ParWebServPtr->ServCustomCfg.SecondWebAccIPPort);
			        if (BufAnsw) 
			        {
				        strcat(BufAnsw, ParWebServPtr->LocalAddrIP);
				        strcat(BufAnsw, StrLine);
			        }
			        else
			        {
				        AddStrWebPage(ParWebServPtr->LocalAddrIP);
				        AddStrWebPage(StrLine);
			        }
			        iSLocalPortSet = true;
		        }
		        else
		        {
		            if (BufAnsw) strcat(BufAnsw, ParWebServPtr->SecondLocalHostName);
			        else         AddStrWebPage(ParWebServPtr->SecondLocalHostName);
		        }
	        }
	        else
	        {
		        if (BufAnsw) strcat(BufAnsw, ParWebServPtr->LocalAddrIP);
		        else         AddStrWebPage(ParWebServPtr->LocalAddrIP);
	        }
	        if ((!iSLocalPortSet) && (ParWebServPtr->ServCustomCfg.SecondExtServIPPort != DEF_HTTP_IP_PORT) &&
				(ParWebServPtr->ServCustomCfg.SecondExtServIPPort != DEF_HTTPS_IP_PORT))
	        {
		        sprintf(StrLine, ":%d/", ParWebServPtr->ServCustomCfg.SecondExtServIPPort);
	        }
	        else
	        {
		        strcpy(StrLine, "/");
	        }        
        }
    }
    else
    {
        /* No Reader ptr - primary channel is used */
        if (strlen(ParWebServPtr->PrimLocalHostName) > 0)
	    {
		    if (BufAnsw) strcat(BufAnsw, ParWebServPtr->PrimLocalHostName);
			else         AddStrWebPage(ParWebServPtr->PrimLocalHostName);
	    }
	    else
	    {
		    if (BufAnsw) strcat(BufAnsw, ParWebServPtr->LocalAddrIP);
		    else         AddStrWebPage(ParWebServPtr->LocalAddrIP);
	    }
	    if ((ParWebServPtr->ServCustomCfg.PrimExtServIPPort != DEF_HTTP_IP_PORT) &&
			(ParWebServPtr->ServCustomCfg.PrimExtServIPPort != DEF_HTTPS_IP_PORT))
	    {
		    sprintf(StrLine, ":%d/", ParWebServPtr->ServCustomCfg.PrimExtServIPPort);
	    }
	    else
	    {
		    strcpy(StrLine, "/");
	    }
    }
    
	if (BufAnsw) strcat(BufAnsw, StrLine);
	else         AddStrWebPage(StrLine);
}
//---------------------------------------------------------------------------
