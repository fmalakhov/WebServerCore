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

#include "CommonPlatform.h"
#include "SysLibTool.h"
#include "SysWebFunction.h"
#include "CpuUtilization.h"

extern char *TablDefNameDey[];
extern SERVER_CPU_UTIL ServerCpuUtil;
#ifdef _LINUX_X86_
extern char ProcStatFileName[];
#endif
//---------------------------------------------------------------------------
void HandleCpuUtilMeasureTimerExp()
{
#ifdef _LINUX_X86_
	FILE         *FileCpuUtilLogPtr = NULL;
	char         *PathFilePtr = NULL;
	char         *CwdRet = NULL;
	unsigned int CpuIndex=0;
	struct tm    *cur_time;
	char         *CpuUtilInfoBuf = NULL;
	char         CoreCpuInfo[32];

#ifdef _SERVDEBUG_
	DebugLogPrint(NULL, "Begin HandlerWebServerReq function (TM_ONTIMEOUT - CPU_UTIL_MEASURE_TMR_ID)\r\n");
#endif
	CpuUtilInfoBuf = (char*)AllocateMemory(1024*sizeof(char));
	if (!CpuUtilInfoBuf) return;
    if(!GetCpuUtilNewInterval()) return;
#ifdef _ESXI_PLATFORM_
    cur_time = &ServerCpuUtil.ServUtilMeasure.LastCpuMeasureDate;
#else
    cur_time = &ServerCpuUtil.ServUtilMeasure[0].LastCpuMeasureDate;
#endif
    sprintf(CpuUtilInfoBuf, "%02d.%02d.%d %02d:%02d:%02d ",
        cur_time->tm_mday, (cur_time->tm_mon+1),
        (cur_time->tm_year+1900), cur_time->tm_hour, 
		cur_time->tm_min, cur_time->tm_sec);

    sprintf(CoreCpuInfo, " % 5.01f", GetLastAvgUserCpu());
	strcat(CpuUtilInfoBuf, CoreCpuInfo);
    sprintf(CoreCpuInfo, " % 5.01f", GetLastAvgSystemCpu());
	strcat(CpuUtilInfoBuf, CoreCpuInfo);
    sprintf(CoreCpuInfo, " % 5.01f", GetLastAvgIdleCpu());
	strcat(CpuUtilInfoBuf, CoreCpuInfo);

#ifdef _ESXI_PLATFORM_
    for(CpuIndex=0;CpuIndex < ServerCpuUtil.ServUtilMeasure.CpuCores;CpuIndex++)
#else
	for(CpuIndex=0;CpuIndex < ServerCpuUtil.ServUtilMeasure[0].CpuCores;CpuIndex++)
#endif
	{
        sprintf(CoreCpuInfo, " % 5.01f", GetLastUserCpu(CpuIndex));
		strcat(CpuUtilInfoBuf, CoreCpuInfo);
        sprintf(CoreCpuInfo, " % 5.01f", GetLastSystemCpu(CpuIndex));
		strcat(CpuUtilInfoBuf, CoreCpuInfo);
        sprintf(CoreCpuInfo, " % 5.01f", GetLastIdleCpu(CpuIndex));
		strcat(CpuUtilInfoBuf, CoreCpuInfo);
	}
	strcat(CpuUtilInfoBuf, "\n");

	PathFilePtr = (char*)AllocateMemory(1024*sizeof(char));
	if (!PathFilePtr)
	{
		FreeMemory(CpuUtilInfoBuf);
		return;
	}
	CwdRet = getcwd(PathFilePtr, 512);
	strcat(PathFilePtr, "/LogFiles/ServerCpuUtil_");
    strcat(PathFilePtr, TablDefNameDey[cur_time->tm_wday]);
    strcat(PathFilePtr, ".log");
#ifdef _ESXI_PLATFORM_
	if (ServerCpuUtil.ServUtilMeasure.LastCpuMeasureDate.tm_wday != 
		ServerCpuUtil.ServUtilMeasure.LastCpuMeasureDate.tm_wday)
#else
	if (ServerCpuUtil.ServUtilMeasure[0].LastCpuMeasureDate.tm_wday != 
		ServerCpuUtil.ServUtilMeasure[1].LastCpuMeasureDate.tm_wday)
#endif
	{
	    FileCpuUtilLogPtr = fopen(PathFilePtr,"wb");
	}
	else
	{
        FileCpuUtilLogPtr = fopen(PathFilePtr,"ab");
	}
	if (!FileCpuUtilLogPtr) 
	{
		FreeMemory(PathFilePtr);
		FreeMemory(CpuUtilInfoBuf);
		return;
	}
	fwrite(CpuUtilInfoBuf, strlen(CpuUtilInfoBuf), 1, FileCpuUtilLogPtr);
	fclose(FileCpuUtilLogPtr);
	FreeMemory(PathFilePtr);
	FreeMemory(CpuUtilInfoBuf);
#endif
}
//---------------------------------------------------------------------------
