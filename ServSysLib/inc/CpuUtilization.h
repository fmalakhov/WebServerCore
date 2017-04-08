# if ! defined( CpuUtilizationH )
#	define CpuUtilizationH	/* only include me once */

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

#ifdef _ESXI_PLATFORM_
#define PROCESSOR_TIME_CORE_READ   1
#define PROCESSOR_TIME_TOTAL_READ  2
#define UTIL_TIME_CORE_READ        3
#define UTIL_TIME_TOTAL_READ       4
#define CORE_TIME_CORE_READ        5
#define CORE_TIME_TOTAL_READ       6
#define AVAIL_MEMORY_READ          7
#define FREE_MEMORY_READ           8
#define STATS_PARSE_DONE           9
#endif

#define MAX_CPU_CORE 64

typedef struct {
#ifdef _ESXI_PLATFORM_
    float             ProcessorTime;
    float             UtilTime;
    float             CoreTime;
#else
    unsigned long int UserCpu;
	unsigned long int SystemCpu;
	unsigned long int IdleCpu;
  #ifdef WIN32 
    FILETIME          LastIdleTime;
    FILETIME          LastKernelTime;
    FILETIME          LastUserTime;
  #endif
#endif
} CORE_CPU_UTIL;

typedef struct {
#ifdef WIN32         
	SYSTEMTIME          LastCpuMeasureDate;
#endif
#ifdef _LINUX_X86_        
    struct tm           LastCpuMeasureDate;
#endif
    unsigned long int   TimeTick;
#ifdef _ESXI_PLATFORM_
    bool                isCoreUtil;
    unsigned int        AvailMemory;
    unsigned int        FreeMemory;
#endif    
    unsigned int        CpuCores;    
    CORE_CPU_UTIL       CpouUtilList[MAX_CPU_CORE+1];
} SERVER_BLK_CPU_UTIL;

typedef struct {
	bool                isInitCpuCalc;
#ifdef WIN32
    unsigned int        ActiveCores;
#endif
	float               DeltaTimeMs;  
    char                *TempDataPath;
#ifdef _ESXI_PLATFORM_
    SERVER_BLK_CPU_UTIL ServUtilMeasure;
#else
    SERVER_BLK_CPU_UTIL ServUtilMeasure[2];
#endif
} SERVER_CPU_UTIL;

void ServerCpuUtilInit(char *TempPath);
unsigned int GetNumAvailableCores();
float GetLastAvgSystemCpu();
float GetLastAvgUserCpu();
float GetLastAvgIdleCpu();
float GetLastSystemCpu(unsigned int CpuIndex);
float GetLastUserCpu(unsigned int CpuIndex);
float GetLastIdleCpu(unsigned int CpuIndex);
bool GetCpuUtilNewInterval();
bool GetServCpuUtil(SERVER_BLK_CPU_UTIL *ServerUtilMgrPtr, char *TempPath);

#ifdef _ESXI_PLATFORM_
float GetMemoryUtil();
unsigned int EtsiGetAvailMemory();
#endif

//---------------------------------------------------------------------------
#endif  /* if ! defined( CpuUtilizationH ) */
