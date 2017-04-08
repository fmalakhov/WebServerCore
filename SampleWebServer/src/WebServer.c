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

#include <dirent.h>
#include <sys/stat.h>
#include "CommonPlatform.h"
#include "vistypes.h"
#include "ThrCernel.h"
#include "SysSwVersion.h"

CERNELTOOL  NetParTool;

extern char WebServerName[];
extern char ServerVersion[];

static char WebServerVerDate[] = WEBSERVER_SW_GEN_DATE;
bool gSWUpgradeRequired = false;
bool gSWRollbackRequired = false;

static void ServerVersionSet();
//---------------------------------------------------------------------------
int main(int argc, char* argv[])
{   
    DEFSYSCERN     CernelTool;

    if (!ServSysLibInit()) exit(EXIT_FAILURE);
    signal(SIGPIPE, SIG_IGN);
   
    CernelTool.NetParCern = &NetParTool;
    ServerVersionSet();
    THRCernelTool(&CernelTool);
    ServSysLibClose();
	printf("\n*** %s shutdown is completed! ***\n", WebServerName);
    exit(EXIT_SUCCESS);
}
//---------------------------------------------------------------------------
static void ServerVersionSet()
{
    unsigned char   PrimVer;
    unsigned char   VerIdStr[8];

    PrimVer = (unsigned char)((WEBSERV_SW_VERSION & 0xff000000) >> 24);
    switch(PrimVer)
    {
        case 0x01: 
            strcpy(VerIdStr, "D");
            break;

        case 0x03:
            strcpy(VerIdStr, "R");
            break;

        default:
            sprintf(VerIdStr, "%d", PrimVer);
            break;            
    }

    sprintf(ServerVersion, "%s%02X.%02X.%02X/%s", VerIdStr, 
       (WEBSERV_SW_VERSION & 0xff0000) >> 16,
       (WEBSERV_SW_VERSION & 0x00ff00) >> 8,
        WEBSERV_SW_VERSION & 0x0000ff, WebServerVerDate);       
}
//---------------------------------------------------------------------------
