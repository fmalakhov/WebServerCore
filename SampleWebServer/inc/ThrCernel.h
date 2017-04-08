# if ! defined( TrCernelH )
#	define TrCernelH	/* only include me once */

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

#ifndef CommonPlatformH
#include "CommonPlatform.h"
#endif

#ifndef vistypesH
#include "vistypes.h"
#endif

#ifndef ThrWebServerH
#include "ThrWebServer.h"
#endif


#ifndef ThrReportMenH
#include "ThrReportMen.h"
#endif

#define	CN_RESULT_START		2000
#define CN_CLOSECERNEL      2001

typedef struct {
    char		    RemIPAddrTermServ[64];
    u_short		    IPPortUserWEB;
    u_short         ExtServIPPort;
	unsigned  	    LocalIPAddress;
	char            LocalHostName[512];
	unsigned long	ThrStartCern;	//The parent thread identificator;
	unsigned	    IDCloseCern;	//The identificator message close cernel process;
} CERNELTOOL;

typedef struct {
    CERNELTOOL		*NetParCern;
    PARAMWEBSERV	ParWebServer;
#ifdef _VCL60ENV_
    struct tagMSG	CernMsg;
    HANDLE		    HTRWEBSERV;
    unsigned long   ThrWEBSERV;
#endif
    char            StartPath[1024];
} DEFSYSCERN;

void THRCernelTool(DEFSYSCERN *CernelTool);

//---------------------------------------------------------------------------
#endif  /* if ! defined( TrCernelH ) */
