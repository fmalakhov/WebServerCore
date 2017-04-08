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

#ifdef _ESXI_PLATFORM_
#include <sys/stat.h>
#endif
#include "CpuUtilization.h"
#include "SysLibTool.h"

SERVER_CPU_UTIL ServerCpuUtil;

#ifdef _ESXI_PLATFORM_
  char EsxiLogFileName[] = "stats.log";
  char PhyCpuTotalMarker[] = "\\Physical Cpu(_Total)\\% Processor Time";
  char CpuInfoMarker[] = "\\Physical Cpu(";
  char PhyCoreUtilMarker[] = "Core Util Time";
#else
  #ifdef _LINUX_X86_
    #ifdef _SUN_BUILD_
      char ProcStatFileName[] = "proc_stat.log";
    #else
      char ProcStatFileName[] = "/proc/stat";
    #endif
  #endif
#endif

#ifdef WIN32
DWORDLONG GetCpuTimeMs(FILETIME *CpuTime);
#endif
//---------------------------------------------------------------------------
void ServerCpuUtilInit(char *TempPath)
{
    ServerCpuUtil.isInitCpuCalc = false;
    ServerCpuUtil.DeltaTimeMs = 1;
    ServerCpuUtil.TempDataPath = TempPath;
#ifdef _ESXI_PLATFORM_    
    ServerCpuUtil.ServUtilMeasure.CpuCores = 0;
    if(GetServCpuUtil(&ServerCpuUtil.ServUtilMeasure, ServerCpuUtil.TempDataPath))
        ServerCpuUtil.isInitCpuCalc = true;    
#else
  #ifdef WIN32
    ServerCpuUtil.ServUtilMeasure[0].CpuCores = 1;
    ServerCpuUtil.ServUtilMeasure[1].CpuCores = 1;
  #endif
	if (GetServCpuUtil(&ServerCpuUtil.ServUtilMeasure[0], ServerCpuUtil.TempDataPath))
        ServerCpuUtil.isInitCpuCalc = true;
#endif
}
//---------------------------------------------------------------------------
float GetLastAvgUserCpu()
{
#ifdef WIN32
    unsigned int DeltaUser;
    float        UserCpuUsage;
#endif 

#ifdef _ESXI_PLATFORM_
    float SummCpuUse;
    
    if (!ServerCpuUtil.isInitCpuCalc || !ServerCpuUtil.ServUtilMeasure.CpuCores) return 0;
    if (ServerCpuUtil.ServUtilMeasure.isCoreUtil)
    {
        SummCpuUse = ServerCpuUtil.ServUtilMeasure.CpouUtilList[0].ProcessorTime + \
            ServerCpuUtil.ServUtilMeasure.CpouUtilList[0].UtilTime;
    }
    else
    {
        SummCpuUse = ServerCpuUtil.ServUtilMeasure.CpouUtilList[0].UtilTime;        
    }
    if (SummCpuUse > 100.0) SummCpuUse = 100.0;
    return SummCpuUse;
#else
    if (!ServerCpuUtil.isInitCpuCalc ||
        !ServerCpuUtil.ServUtilMeasure[0].CpuCores || 
        ServerCpuUtil.ServUtilMeasure[0].CpuCores != ServerCpuUtil.ServUtilMeasure[1].CpuCores) return 0;
  #ifdef _LINUX_X86_
    #ifdef _SUN_BUILD_
    return (float)ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[0].UserCpu;
    #else
    return ((float)(ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[0].UserCpu - \
        ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[0].UserCpu) / ServerCpuUtil.DeltaTimeMs) / \
        (float)ServerCpuUtil.ServUtilMeasure[0].CpuCores;
    #endif
  #endif
  #ifdef WIN32
    DeltaUser = (unsigned int)(GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[0].LastUserTime) - \
        GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[0].LastUserTime));                
    if (DeltaUser > ((unsigned int)ServerCpuUtil.DeltaTimeMs*ServerCpuUtil.ActiveCores)) UserCpuUsage = 100.0;
    else UserCpuUsage = (float)(((float)DeltaUser / (ServerCpuUtil.DeltaTimeMs*(float)ServerCpuUtil.ActiveCores)) * 100.0);
	return UserCpuUsage;
  #endif
#endif
}
//---------------------------------------------------------------------------
float GetLastAvgSystemCpu()
{
#ifdef WIN32
    unsigned int DeltaKern, DeltaIdle, DeltaSys;
    float        SysCpuUsage;
#endif 

#ifdef _ESXI_PLATFORM_
    if (!ServerCpuUtil.isInitCpuCalc || !ServerCpuUtil.ServUtilMeasure.CpuCores) return 0;
    return ServerCpuUtil.ServUtilMeasure.CpouUtilList[0].CoreTime;
#else
    if (!ServerCpuUtil.isInitCpuCalc ||
        !ServerCpuUtil.ServUtilMeasure[0].CpuCores || 
        ServerCpuUtil.ServUtilMeasure[0].CpuCores != ServerCpuUtil.ServUtilMeasure[1].CpuCores) return 0;
  #ifdef _LINUX_X86_
    #ifdef _SUN_BUILD_        
	return (float)ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[0].SystemCpu;
    #else
	return ((float)(ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[0].SystemCpu - \
        ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[0].SystemCpu) / ServerCpuUtil.DeltaTimeMs) / \
        (float)ServerCpuUtil.ServUtilMeasure[0].CpuCores;    
    #endif
  #endif
  #ifdef WIN32
    DeltaIdle = (unsigned int)(GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[0].LastIdleTime) -\
        GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[0].LastIdleTime));      
    DeltaKern = (unsigned int)(GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[0].LastKernelTime) -\
        GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[0].LastKernelTime));
    if (DeltaKern < DeltaIdle) DeltaSys = 0;
    else                       DeltaSys = DeltaKern - DeltaIdle;
    if (DeltaSys > ((unsigned int)ServerCpuUtil.DeltaTimeMs*ServerCpuUtil.ActiveCores)) SysCpuUsage = 100.0;
    else   SysCpuUsage = (float)((((float)DeltaSys / (ServerCpuUtil.DeltaTimeMs*(float)ServerCpuUtil.ActiveCores))) * 100.0);
	return SysCpuUsage;
  #endif
#endif
}
//---------------------------------------------------------------------------
float GetLastAvgIdleCpu()
{
#ifdef WIN32
    unsigned int DeltaIdle;
    float        IdleCpuUsage;
#endif 

#ifdef _ESXI_PLATFORM_
    float SummCpuUse;
    
    if (!ServerCpuUtil.isInitCpuCalc || !ServerCpuUtil.ServUtilMeasure.CpuCores) return 0;
    if (ServerCpuUtil.ServUtilMeasure.isCoreUtil)
    {
        SummCpuUse = ServerCpuUtil.ServUtilMeasure.CpouUtilList[0].ProcessorTime + \
            ServerCpuUtil.ServUtilMeasure.CpouUtilList[0].UtilTime + \
            ServerCpuUtil.ServUtilMeasure.CpouUtilList[0].CoreTime;
    }
    else
    {
        SummCpuUse = ServerCpuUtil.ServUtilMeasure.CpouUtilList[0].ProcessorTime;        
    }
    if (SummCpuUse > 100.0) return 0;
    else                    return 100.0 - SummCpuUse;
#else
    if (!ServerCpuUtil.isInitCpuCalc ||
        !ServerCpuUtil.ServUtilMeasure[0].CpuCores || 
        ServerCpuUtil.ServUtilMeasure[0].CpuCores != ServerCpuUtil.ServUtilMeasure[1].CpuCores) return 0;
  #ifdef _LINUX_X86_
    #ifdef _SUN_BUILD_   
	return (float)ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[0].IdleCpu;         
    #else
	return ((float)(ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[0].IdleCpu - \
        ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[0].IdleCpu) / ServerCpuUtil.DeltaTimeMs) / \
        (float)ServerCpuUtil.ServUtilMeasure[0].CpuCores;
    #endif
  #endif
  #ifdef WIN32
    DeltaIdle = (unsigned int)(GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[0].LastIdleTime) -\
         GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[0].LastIdleTime));
    if (DeltaIdle > ((unsigned int)ServerCpuUtil.DeltaTimeMs*ServerCpuUtil.ActiveCores)) IdleCpuUsage = 100.0;    
    else  IdleCpuUsage = (float)(((float)DeltaIdle / (ServerCpuUtil.DeltaTimeMs*(float)ServerCpuUtil.ActiveCores)) * 100.0);
	return IdleCpuUsage;
  #endif
#endif
}
//---------------------------------------------------------------------------
float GetLastUserCpu(unsigned int CpuIndex)
{
#ifdef WIN32
    unsigned int DeltaUser;
    float        UserCpuUsage;
#endif 

#ifdef _ESXI_PLATFORM_
    float SummCpuUse;

    if ((CpuIndex > ServerCpuUtil.ServUtilMeasure.CpuCores) ||
    !ServerCpuUtil.isInitCpuCalc) return 0;
    if (ServerCpuUtil.ServUtilMeasure.isCoreUtil)
    {
        SummCpuUse = ServerCpuUtil.ServUtilMeasure.CpouUtilList[CpuIndex+1].ProcessorTime + \
            ServerCpuUtil.ServUtilMeasure.CpouUtilList[CpuIndex+1].UtilTime;
    }
    else
    {
        SummCpuUse = ServerCpuUtil.ServUtilMeasure.CpouUtilList[CpuIndex+1].UtilTime;    
    }
    if (SummCpuUse > 100.0) SummCpuUse = 100.0;
    return SummCpuUse;    
#else
    if ((CpuIndex > ServerCpuUtil.ServUtilMeasure[0].CpuCores) ||
    !ServerCpuUtil.isInitCpuCalc) return 0;
  #ifdef _LINUX_X86_
    #ifdef _SUN_BUILD_ 
	return (float)ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[CpuIndex+1].UserCpu;    
    #else
	return (float)(ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[CpuIndex+1].UserCpu - \
        ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[CpuIndex+1].UserCpu) / ServerCpuUtil.DeltaTimeMs;
    #endif
  #endif
  #ifdef WIN32
    DeltaUser = (unsigned int)(GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[CpuIndex+1].CpouUtilList[0].LastUserTime) - \
		GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[CpuIndex+1].LastUserTime));
    if (DeltaUser > ((unsigned int)ServerCpuUtil.DeltaTimeMs*ServerCpuUtil.ActiveCores)) UserCpuUsage = 100.0;
    else   UserCpuUsage = (float)(((float)DeltaUser / (ServerCpuUtil.DeltaTimeMs*(float)ServerCpuUtil.ActiveCores)) * 100.0);
	return UserCpuUsage;
  #endif
#endif
}
//---------------------------------------------------------------------------
float GetLastSystemCpu(unsigned int CpuIndex)
{
#ifdef WIN32
    unsigned int DeltaKern, DeltaIdle, DeltaSys;
    float        SysCpuUsage;
#endif 

#ifdef _ESXI_PLATFORM_
    if ((CpuIndex > ServerCpuUtil.ServUtilMeasure.CpuCores) ||
    !ServerCpuUtil.isInitCpuCalc) return 0;
    return ServerCpuUtil.ServUtilMeasure.CpouUtilList[CpuIndex+1].CoreTime;    
#else
    if ((CpuIndex > ServerCpuUtil.ServUtilMeasure[0].CpuCores) ||
        !ServerCpuUtil.isInitCpuCalc) return 0;
  #ifdef _LINUX_X86_
    #ifdef _SUN_BUILD_ 
	return (float)ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[CpuIndex+1].SystemCpu;    
    #else
	return (float)(ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[CpuIndex+1].SystemCpu - \
        ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[CpuIndex+1].SystemCpu) / ServerCpuUtil.DeltaTimeMs;
    #endif
  #endif
  #ifdef WIN32
    DeltaKern = (unsigned int)(GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[CpuIndex+1].LastKernelTime) - \
        GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[CpuIndex+1].LastKernelTime));
    DeltaIdle = (unsigned int)(GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[CpuIndex+1].LastIdleTime) - \
        GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[CpuIndex+1].LastIdleTime));
    if (DeltaKern < DeltaIdle) DeltaSys = 0;
    else DeltaSys = DeltaKern - DeltaIdle;  
    if (DeltaSys > ((unsigned int)ServerCpuUtil.DeltaTimeMs*ServerCpuUtil.ActiveCores)) SysCpuUsage = 100.0;
    else   SysCpuUsage = (float)(((float)DeltaSys / (ServerCpuUtil.DeltaTimeMs*(float)ServerCpuUtil.ActiveCores)) * 100.0);
	return SysCpuUsage;
  #endif
#endif
}
//---------------------------------------------------------------------------
float GetLastIdleCpu(unsigned int CpuIndex)
{
#ifdef WIN32
    unsigned int DeltaIdle;
    float        IdleCpuUsage;
#endif 

#ifdef _ESXI_PLATFORM_
    float SummCpuUse;

    if ((CpuIndex > ServerCpuUtil.ServUtilMeasure.CpuCores) ||
    !ServerCpuUtil.isInitCpuCalc) return 0;
    if (ServerCpuUtil.ServUtilMeasure.isCoreUtil)
    {
        SummCpuUse = ServerCpuUtil.ServUtilMeasure.CpouUtilList[CpuIndex+1].ProcessorTime + \
            ServerCpuUtil.ServUtilMeasure.CpouUtilList[CpuIndex+1].UtilTime + \
            ServerCpuUtil.ServUtilMeasure.CpouUtilList[CpuIndex+1].CoreTime;
    }
    else
    {
        SummCpuUse = ServerCpuUtil.ServUtilMeasure.CpouUtilList[CpuIndex+1].ProcessorTime;
    }
    if (SummCpuUse > 100.0) return 0;
    else                    return 100.0 - SummCpuUse;    
#else
    if ((CpuIndex > ServerCpuUtil.ServUtilMeasure[0].CpuCores) || 
        !ServerCpuUtil.isInitCpuCalc) return 0;
  #ifdef _LINUX_X86_
    #ifdef _SUN_BUILD_
	return (float)ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[CpuIndex+1].IdleCpu;    
    #else
	return (float)(ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[CpuIndex+1].IdleCpu - \
        ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[CpuIndex+1].IdleCpu) / ServerCpuUtil.DeltaTimeMs;
    #endif
  #endif
  #ifdef WIN32
    DeltaIdle = (unsigned int)(GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[CpuIndex+1].CpouUtilList[0].LastIdleTime) - \
		GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[CpuIndex+1].LastIdleTime));
    if (DeltaIdle > ((unsigned int)ServerCpuUtil.DeltaTimeMs*ServerCpuUtil.ActiveCores)) IdleCpuUsage = 100.0;
    else   IdleCpuUsage = (float)(((float)DeltaIdle / (ServerCpuUtil.DeltaTimeMs*(float)ServerCpuUtil.ActiveCores)) * 100.0);
	return IdleCpuUsage;
  #endif
#endif
}
//---------------------------------------------------------------------------
bool GetCpuUtilNewInterval()
{
#ifdef WIN32
    unsigned int DeltaKern, DeltaUser;
#endif

#ifdef _ESXI_PLATFORM_
    if (!GetServCpuUtil(&ServerCpuUtil.ServUtilMeasure, ServerCpuUtil.TempDataPath)) return false;
#else
	memcpy(&ServerCpuUtil.ServUtilMeasure[1], &ServerCpuUtil.ServUtilMeasure[0], sizeof(SERVER_BLK_CPU_UTIL));
    if (!GetServCpuUtil(&ServerCpuUtil.ServUtilMeasure[0], ServerCpuUtil.TempDataPath)) return false;
    #ifndef WIN32
	if (ServerCpuUtil.ServUtilMeasure[0].CpuCores != ServerCpuUtil.ServUtilMeasure[1].CpuCores) return false;
    #endif
  #ifdef _LINUX_X86_
	ServerCpuUtil.DeltaTimeMs = (float)(((double)(ServerCpuUtil.ServUtilMeasure[0].TimeTick - ServerCpuUtil.ServUtilMeasure[1].TimeTick))/1000.0); 
  #endif
  #ifdef WIN32
	ServerCpuUtil.DeltaTimeMs = (float)(ServerCpuUtil.ServUtilMeasure[0].TimeTick - ServerCpuUtil.ServUtilMeasure[1].TimeTick);    
    DeltaKern = (unsigned int)(GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[0].LastKernelTime) - \
        GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[0].LastKernelTime));
    DeltaUser = (unsigned int)(GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[0].CpouUtilList[0].LastUserTime) - \
		GetCpuTimeMs(&ServerCpuUtil.ServUtilMeasure[1].CpouUtilList[0].LastUserTime));        
	ServerCpuUtil.ActiveCores = (unsigned int)((float)(DeltaKern+DeltaUser)/ServerCpuUtil.DeltaTimeMs + 0.1);
  #endif
#endif
    ServerCpuUtil.isInitCpuCalc = true;
    return true;
}
//---------------------------------------------------------------------------
#ifdef _ESXI_PLATFORM_
float GetMemoryUtil()
{
    return ((float)ServerCpuUtil.ServUtilMeasure.FreeMemory*100.0) / \
        (float)ServerCpuUtil.ServUtilMeasure.AvailMemory;
}
//---------------------------------------------------------------------------
unsigned int EtsiGetAvailMemory()
{
    return (unsigned int)ServerCpuUtil.ServUtilMeasure.AvailMemory;
}
#endif
//---------------------------------------------------------------------------
bool GetServCpuUtil(SERVER_BLK_CPU_UTIL *ServerUtilMgrPtr, char *TempPath)
{
	bool Result = false;
#ifdef _LINUX_X86_
  #ifdef _ESXI_PLATFORM_
    int           i, j, k, SysRes;
    FILE          *l_LogFileHandle = NULL;
    struct stat   l_FileInfo;
    char          *l_LogFileDataPtr = NULL;
    int           l_DataOffset, pars_read;
    unsigned int  ReadValue;
    float         ReadFloatVal;
    unsigned int  l_LogFileLen = 0;
    unsigned int  l_LogLineBeginPoint = 0;     
    unsigned char ScanPhase = PROCESSOR_TIME_CORE_READ;
    bool          HeaderEnd = false;
    unsigned int  SkipCount = 7;
    unsigned int  ReadParNum, CpuId;
    char          SystemCmd[1024];

    if (!ServerUtilMgrPtr || !TempPath) return false;
    if (!ServerUtilMgrPtr->CpuCores)
    {
        printf("Start initial measure of host parameters...\n");
        strcpy(SystemCmd, TempPath);
        strcat(SystemCmd, EsxiLogFileName);
	    if (access(SystemCmd, F_OK) == 0)
	    {        
            sprintf(SystemCmd, "rm %s%s\n", TempPath, EsxiLogFileName);
            SysRes = system(SystemCmd);
        }
        sprintf(SystemCmd, "esxtop -b -c esxtop.rc -n 1 > %s%s\n",
            TempPath, EsxiLogFileName);
        SysRes = system(SystemCmd);
        printf("Complete initial measure of host parameters\n");
    }

    strcpy(SystemCmd, TempPath);
    strcat(SystemCmd, EsxiLogFileName);
    if (access(SystemCmd, F_OK) == 0)
    {      
        l_LogFileHandle = fopen(SystemCmd,"r");
        if (l_LogFileHandle == NULL)
        {
            printf("Error of %s log file open for read\n", SystemCmd);
            return Result;
        }
	}
    
	if (stat(SystemCmd, &l_FileInfo))
	{
        printf("Error of %s log file information read\n", SystemCmd);
	    fclose(l_LogFileHandle);
	    return Result;
	}

    l_LogFileDataPtr = (char*)AllocateMemory(l_FileInfo.st_size + 1);
    l_LogFileLen = fread(l_LogFileDataPtr, 1, l_FileInfo.st_size, l_LogFileHandle);
	if (l_LogFileLen != l_FileInfo.st_size)
	{
        printf("The %s log file read error\n", SystemCmd);
	    fclose(l_LogFileHandle);
        FreeMemory(l_LogFileDataPtr);
	    return Result;		
	}
    
    l_LogFileDataPtr[l_LogFileLen] = 0;
	l_LogLineBeginPoint = 0;
    ServerUtilMgrPtr->isCoreUtil = false;
    if (FindCmdRequest((char*)&l_LogFileDataPtr[l_LogLineBeginPoint], PhyCoreUtilMarker) != -1) ServerUtilMgrPtr->isCoreUtil = true;
    l_DataOffset = FindCmdRequest((char*)&l_LogFileDataPtr[l_LogLineBeginPoint], PhyCpuTotalMarker);
    if (l_DataOffset != -1)
    {
        k = l_DataOffset;    
        if (!ServerUtilMgrPtr->CpuCores)
        {
            /* Number of available phisical CPU cores detection */
            l_DataOffset -= strlen(PhyCpuTotalMarker);
            l_LogFileDataPtr[l_LogLineBeginPoint+l_DataOffset] = 0;
            j = l_LogLineBeginPoint;        
            for(;;)
            {
                i = FindCmdRequest((char*)&l_LogFileDataPtr[j], CpuInfoMarker);
                if (i == -1) break;
                ServerUtilMgrPtr->CpuCores++;
                j += i;
            }
            printf("Host has %d phisical cores\n", ServerUtilMgrPtr->CpuCores);
        }
        l_LogLineBeginPoint += k;

        /* Shift to next line begin */
        while(l_LogLineBeginPoint < l_LogFileLen)
        {
            if (l_LogFileDataPtr[l_LogLineBeginPoint] == '\n')
            {
                HeaderEnd = true;
                l_LogLineBeginPoint++;
                break;
            }
            l_LogLineBeginPoint++;
        }
        if (HeaderEnd)
        {
            ReadParNum = 1;
            if (l_LogFileDataPtr[l_LogLineBeginPoint] == '"')
            {
                CpuId = 0;            
                l_LogLineBeginPoint++;
                j = l_LogLineBeginPoint;
                for(;;)
                {
                    if (SkipCount > 0)
                    {
                        SkipCount--;
                    }
                    else
                    {
                        switch(ScanPhase)
                        {
                            case PROCESSOR_TIME_CORE_READ:
                                pars_read = sscanf((char*)&l_LogFileDataPtr[j], "%f", &ReadFloatVal);
                                if (!pars_read) ScanPhase = STATS_PARSE_DONE;
                                else
                                {
                                    if (ReadFloatVal > 100.0) ReadFloatVal = 100.0;
                                    CpuId++;
                                    ServerUtilMgrPtr->CpouUtilList[CpuId].ProcessorTime = ReadFloatVal;
                                    if (CpuId == ServerUtilMgrPtr->CpuCores) ScanPhase = PROCESSOR_TIME_TOTAL_READ;
                                }
                                break;
                                
                            case PROCESSOR_TIME_TOTAL_READ:
                                pars_read = sscanf((char*)&l_LogFileDataPtr[j], "%f", &ReadFloatVal);
                                if (!pars_read) ScanPhase = STATS_PARSE_DONE;
                                else
                                {
                                    if (ReadFloatVal > 100.0) ReadFloatVal = 100.0;
                                    ScanPhase = UTIL_TIME_CORE_READ;
                                    ServerUtilMgrPtr->CpouUtilList[0].ProcessorTime = ReadFloatVal;
                                    CpuId = 0;
                                }
                                break;
                                
                            case UTIL_TIME_CORE_READ:
                                pars_read = sscanf((char*)&l_LogFileDataPtr[j], "%f", &ReadFloatVal);
                                if (!pars_read) ScanPhase = STATS_PARSE_DONE;
                                else
                                {
                                    if (ReadFloatVal > 100.0) ReadFloatVal = 100.0;
                                    CpuId++;
                                    ServerUtilMgrPtr->CpouUtilList[CpuId].UtilTime = ReadFloatVal;
                                    if (CpuId == ServerUtilMgrPtr->CpuCores) ScanPhase = UTIL_TIME_TOTAL_READ;                            
                                }
                                break;

                            case UTIL_TIME_TOTAL_READ:
                                pars_read = sscanf((char*)&l_LogFileDataPtr[j], "%f", &ReadFloatVal);
                                if (!pars_read) ScanPhase = STATS_PARSE_DONE;
                                else
                                {   
                                    if (ReadFloatVal > 100.0) ReadFloatVal = 100.0;                                                                                                                            
                                    ScanPhase = CORE_TIME_CORE_READ;
                                    ServerUtilMgrPtr->CpouUtilList[0].UtilTime = ReadFloatVal;
                                    CpuId = 0;
                                    if (!ServerUtilMgrPtr->isCoreUtil)
                                    {
                                        for(i=0;i < (ServerUtilMgrPtr->CpuCores+1);i++)
                                        {
                                            if (ServerUtilMgrPtr->CpouUtilList[i].ProcessorTime >
                                                ServerUtilMgrPtr->CpouUtilList[i].UtilTime)
                                            {
                                                ServerUtilMgrPtr->CpouUtilList[i].CoreTime = \
                                                    ServerUtilMgrPtr->CpouUtilList[i].ProcessorTime - \
                                                    ServerUtilMgrPtr->CpouUtilList[i].UtilTime;
                                            }
                                            else
                                            {
                                                ServerUtilMgrPtr->CpouUtilList[CpuId].CoreTime = 0;
                                            }
                                        }
                                        ScanPhase = AVAIL_MEMORY_READ;
                                        CpuId = 0;
                                    }                                    
                                }
                                break;

                            case CORE_TIME_CORE_READ:
                                pars_read = sscanf((char*)&l_LogFileDataPtr[j], "%f", &ReadFloatVal);
                                if (!pars_read) ScanPhase = STATS_PARSE_DONE;
                                else
                                {
                                    if (ReadFloatVal > 100.0) ReadFloatVal = 100.0;                                                              
                                    CpuId++;
                                    ServerUtilMgrPtr->CpouUtilList[CpuId].CoreTime = ReadFloatVal;
                                    if (CpuId == ServerUtilMgrPtr->CpuCores) ScanPhase = CORE_TIME_TOTAL_READ;                            
                                }
                                break;

                            case CORE_TIME_TOTAL_READ:
                                pars_read = sscanf((char*)&l_LogFileDataPtr[j], "%f", &ReadFloatVal);
                                if (!pars_read) ScanPhase = STATS_PARSE_DONE;
                                else
                                {                                                                                                
                                    ScanPhase = AVAIL_MEMORY_READ;
                                    ServerUtilMgrPtr->CpouUtilList[0].CoreTime = ReadFloatVal;
                                    CpuId = 0;
                                }
                                break;
                                
                            case AVAIL_MEMORY_READ:
                                pars_read = sscanf((char*)&l_LogFileDataPtr[j], "%d", &ReadValue);
                                if (!pars_read) ScanPhase = STATS_PARSE_DONE;
                                else
                                {                                                                                                
                                    ScanPhase = FREE_MEMORY_READ;
                                    ServerUtilMgrPtr->AvailMemory = ReadValue;
                                    SkipCount = 2;
                                }
                                break;
                                
                            case FREE_MEMORY_READ:
                                pars_read = sscanf((char*)&l_LogFileDataPtr[j], "%d", &ReadValue);
                                if (!pars_read) ScanPhase = STATS_PARSE_DONE;
                                else
                                {                                                                                                
                                    ScanPhase = STATS_PARSE_DONE;    
                                    ServerUtilMgrPtr->FreeMemory = ReadValue;
                                    break;
                                }
                        }
                        if (ScanPhase == STATS_PARSE_DONE) break;
                    }
                    i = FindCmdRequest((char*)&l_LogFileDataPtr[j], "\",\"");
                    if (i == -1) break;
                    ReadParNum++;
                    j += i;
                }
                Result = true;
            }
            else
            {
                printf("No parameter start marker\n");
            }
        }
        else
        {
           printf("End of header line is not detercted\n");
        }
    }
    else
    {
        printf("CPU information not detected\n");
    }
    fclose(l_LogFileHandle);
    FreeMemory(l_LogFileDataPtr);
    sprintf(SystemCmd, "rm %s%s\n", TempPath, EsxiLogFileName);
    SysRes = system(SystemCmd);
    return Result;
  #else
    int  CpuId;
    unsigned long int UserCpu, IdleCpu, SystemCpu;    
    int  size = 1024, pos;
    int  c;
    int  pars_read = 0;
    int  isOverCpu = 1;
	int  CpuIndex = 0;
	FILE *StatFilePtr = NULL;
    char *BufferPtr = NULL;
    struct timeb  hires_cur_time;
    struct tm     *CurrTime;
  #ifdef _SUN_BUILD_
    unsigned int cpu, minf, mjf, xcal, intr, ithr, csw, icsw, migr, smtx, srw, syscl, wt;
    char SystemCmd[1024];
  #else
	unsigned long int NiceCpu, IoWaitCpu, IrqCpu, SoftIrq, Steal; 
  #endif

	BufferPtr = (char*)AllocateMemory(size*sizeof(char)+1);
	ServerUtilMgrPtr->CpuCores = 0;
	/* Current tick store */
	ServerUtilMgrPtr->TimeTick = (unsigned long int)GetTickCount();

	/* Current date and time store */
    ftime(&hires_cur_time);
    CurrTime = localtime(&hires_cur_time.time);
    memcpy(&ServerUtilMgrPtr->LastCpuMeasureDate, CurrTime, sizeof(struct tm));

  #ifdef _SUN_BUILD_
    strcpy(SystemCmd, TempPath);
    strcat(SystemCmd, ProcStatFileName);
	if (access(SystemCmd, F_OK) == 0)
	{          
        sprintf(SystemCmd, "rm %s%s\n", TempPath, ProcStatFileName);
        system(SystemCmd);    
    }
    sprintf(SystemCmd, "mpstat > %s%s\n", TempPath, ProcStatFileName);
    system(SystemCmd);
    strcpy(SystemCmd, TempPath);
    strcat(SystemCmd, ProcStatFileName);
    StatFilePtr = fopen(SystemCmd, "r");
  #else
    StatFilePtr = fopen(ProcStatFileName, "r");
  #endif    
    if(StatFilePtr) 
    {
        do 
        { 
            /* read all lines in file */
            pos = 0;
            do
			{ 
                  /* read one line */
                  c = fgetc(StatFilePtr);
                  if(c != EOF) BufferPtr[pos++] = (char)c;
                  if(pos >= size - 1)
				  {   
                      /* increase buffer length - leave room for 0 */
                      size *= 2;
                      BufferPtr = (char*)realloc(BufferPtr, size);
				  }
			} while(c != EOF && c != '\n');
            BufferPtr[pos] = 0;
            if (isOverCpu) 
			{
            #ifdef _SUN_BUILD_
                isOverCpu = 0;
			    ServerUtilMgrPtr->CpouUtilList[0].IdleCpu = 0;
			    ServerUtilMgrPtr->CpouUtilList[0].SystemCpu = 0;
                ServerUtilMgrPtr->CpouUtilList[0].UserCpu = 0;            
            #else
                pars_read = sscanf(BufferPtr, "cpu  %lu %lu %lu %lu %lu %lu %lu %lu",
                    &UserCpu, &NiceCpu, &SystemCpu, &IdleCpu, &IoWaitCpu,
                    &IrqCpu, &SoftIrq, &Steal);
                if (pars_read < 8) break;
                isOverCpu = 0;
			    ServerUtilMgrPtr->CpouUtilList[CpuIndex].IdleCpu = IdleCpu;
			    ServerUtilMgrPtr->CpouUtilList[CpuIndex].SystemCpu = SystemCpu;
                ServerUtilMgrPtr->CpouUtilList[CpuIndex].UserCpu = UserCpu;
            #endif
			}
            else
			{
            #ifdef _SUN_BUILD_
                pars_read = sscanf(BufferPtr, "%u %u %u %u %u %u %u %u %u %u %u %u %lu %lu %u %lu",
                    &cpu, &minf, &mjf, &xcal, &intr, &ithr, &csw, &icsw, &migr, &smtx, &srw, &syscl,
                    &UserCpu, &SystemCpu, &wt, &IdleCpu);
                if (pars_read < 16) break;
                if ((IdleCpu+SystemCpu+UserCpu) > 100) break;
			    ServerUtilMgrPtr->CpouUtilList[CpuIndex].IdleCpu = IdleCpu;
			    ServerUtilMgrPtr->CpouUtilList[CpuIndex].SystemCpu = SystemCpu;
                ServerUtilMgrPtr->CpouUtilList[CpuIndex].UserCpu = UserCpu;
			    ServerUtilMgrPtr->CpuCores++;
                
			    ServerUtilMgrPtr->CpouUtilList[0].IdleCpu += IdleCpu;
			    ServerUtilMgrPtr->CpouUtilList[0].SystemCpu += SystemCpu;
                ServerUtilMgrPtr->CpouUtilList[0].UserCpu += UserCpu;            
                
				Result = true;
            #else
                pars_read = sscanf(BufferPtr, "cpu%u %lu %lu %lu %lu %lu %lu %lu %lu",
                    &CpuId,  &UserCpu, &NiceCpu, &SystemCpu, &IdleCpu,
                    &IoWaitCpu, &IrqCpu, &SoftIrq, &Steal);
                if (pars_read < 9) break;
			    ServerUtilMgrPtr->CpouUtilList[CpuIndex].IdleCpu = IdleCpu;
			    ServerUtilMgrPtr->CpouUtilList[CpuIndex].SystemCpu = SystemCpu;
                ServerUtilMgrPtr->CpouUtilList[CpuIndex].UserCpu = UserCpu;
			    ServerUtilMgrPtr->CpuCores++;
				Result = true;
            #endif
			}
			if (ServerUtilMgrPtr->CpuCores == MAX_CPU_CORE) break;
		    CpuIndex++;
		} while(c != EOF); 
        fclose(StatFilePtr);
  #ifdef _SUN_BUILD_
        if (ServerUtilMgrPtr->CpuCores > 0)
        {
			ServerUtilMgrPtr->CpouUtilList[0].IdleCpu   = ServerUtilMgrPtr->CpouUtilList[0].IdleCpu / ServerUtilMgrPtr->CpuCores;
			ServerUtilMgrPtr->CpouUtilList[0].SystemCpu = ServerUtilMgrPtr->CpouUtilList[0].SystemCpu / ServerUtilMgrPtr->CpuCores;
            ServerUtilMgrPtr->CpouUtilList[0].UserCpu   =  ServerUtilMgrPtr->CpouUtilList[0].UserCpu / ServerUtilMgrPtr->CpuCores;                    
        }
        sprintf(SystemCmd, "rm %s%s\n", TempPath, ProcStatFileName);
        system(SystemCmd);            
  #endif        
    }
	else
	{
    #ifdef _SUN_BUILD_
		printf("Fail to open %s file for read\n", SystemCmd);
    #else
        printf("Fail to open %s file for read\n", ProcStatFileName);
    #endif
	}
    FreeMemory(BufferPtr);
	return Result;
  #endif
#endif
#ifdef WIN32
	bool         SysRes;
    
	SysRes = GetSystemTimes(&ServerUtilMgrPtr->CpouUtilList[0].LastIdleTime, 
		&ServerUtilMgrPtr->CpouUtilList[0].LastKernelTime, &ServerUtilMgrPtr->CpouUtilList[0].LastUserTime);
	SysRes = GetSystemTimes(&ServerUtilMgrPtr->CpouUtilList[1].LastIdleTime, 
		&ServerUtilMgrPtr->CpouUtilList[1].LastKernelTime, &ServerUtilMgrPtr->CpouUtilList[1].LastUserTime);
	ServerUtilMgrPtr->TimeTick = (unsigned long int)GetTickCount(); 
	Result = true;
	return Result;
#endif
}
//---------------------------------------------------------------------------
#ifdef WIN32
DWORDLONG GetCpuTimeMs(FILETIME *CpuTime)
{
    return ((((DWORDLONG)CpuTime->dwHighDateTime) << 32) + (DWORDLONG)CpuTime->dwLowDateTime) / 10000;
}
#endif
//---------------------------------------------------------------------------
unsigned int GetNumAvailableCores()
{
#ifdef _ESXI_PLATFORM_
    if (!ServerCpuUtil.isInitCpuCalc || !ServerCpuUtil.ServUtilMeasure.CpuCores) return 0;
    else                                 return ServerCpuUtil.ServUtilMeasure.CpuCores;
#else
  #ifdef WIN32
    return ServerCpuUtil.ActiveCores;
  #else
    if (!ServerCpuUtil.isInitCpuCalc || !ServerCpuUtil.ServUtilMeasure[0].CpuCores) return 0;
    else                                 return ServerCpuUtil.ServUtilMeasure[0].CpuCores;
  #endif
#endif    
}
//---------------------------------------------------------------------------
