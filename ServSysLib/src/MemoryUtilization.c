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

#include "MemoryUtilization.h"
#ifdef _ESXI_PLATFORM_
#include "CpuUtilization.h"
#endif

#ifdef _LINUX_X86_
  #ifdef _SUN_BUILD_
    char MemStatFileName[] = "proc_stat.log";
  #else
    char MemStatFileName[] = "/proc/meminfo";
  #endif
#endif
#ifdef WIN32
#define MEM_MB_SIZE_DIV 1048576
#endif
//---------------------------------------------------------------------------
bool GetServMemUsage(unsigned char *MemProcPtr, char *TempPath, bool SysMemUseFlag)  
{
	bool         Result = false;
    unsigned int AvailMemory = 0;
    unsigned int FreeMemory = 0;
    bool         isAvailMemRead = false;
    bool         isFreeMemRead = false;
    int          size = 1024;
#ifdef _LINUX_X86_
    int          i, c, pos;
    bool         isBufferMemRead = false;
    bool         isCachMemRead = false;
    unsigned int BuffersMemory = 0;
    unsigned int CachedMemory = 0;
#endif
    int          pars_read = 0;
	FILE         *StatFilePtr = NULL;
    char         *BufferPtr = NULL;
#ifdef _SUN_BUILD_
    unsigned int SP1, SP2, SP3, SwapMem;
    char         SystemCmd[512];
#endif
#ifdef WIN32
	MEMORYSTATUSEX memInfo;
#endif

    if (!MemProcPtr || !TempPath) return false;
	BufferPtr = (char*)malloc(size*sizeof(char)+1);
    if (!BufferPtr) return false;
    
#ifdef _LINUX_X86_
  #ifdef _SUN_BUILD_
    /* Check memory usage for SUN OS platforms */
    strcpy(SystemCmd, TempPath);
    strcat(SystemCmd, MemStatFileName);
    if (access(SystemCmd, F_OK) == 0)
	{            
        sprintf(SystemCmd, "rm %s%s\n", TempPath, MemStatFileName);
        system(SystemCmd);  
    }  
    sprintf(SystemCmd, "prtconf -p | grep Memory > %s%s\n", TempPath, MemStatFileName);
    system(SystemCmd);
    
    strcpy(SystemCmd, TempPath);
    strcat(SystemCmd, MemStatFileName);
    StatFilePtr = fopen(SystemCmd, "r");
    if(StatFilePtr) 
    {
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
        i = FindCmdRequest(BufferPtr, "Memory size:");
        if (i != -1)
        {
            pars_read = sscanf(&BufferPtr[i], "%u", &AvailMemory);
            if (pars_read > 0)
            {
                AvailMemory *= 1024;
                isAvailMemRead = true;
            }
        }
        fclose(StatFilePtr);
        sprintf(SystemCmd, "rm %s%s\n", TempPath, MemStatFileName);
        system(SystemCmd);    
    }
	else
	{
		printf("Fail to open %s file for read\n", SystemCmd);
	}
   
    sprintf(SystemCmd, "vmstat > %s%s\n", TempPath, MemStatFileName);
    system(SystemCmd);
    strcpy(SystemCmd, TempPath);
    strcat(SystemCmd, MemStatFileName);
    StatFilePtr = fopen(SystemCmd, "r");
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
            if ((FindCmdRequest(BufferPtr, "memory") == -1) &&
                (FindCmdRequest(BufferPtr, "free") == -1))
            {
                pars_read = sscanf(BufferPtr, "%u %u %u %u %u", 
                    &SP1, &SP2, &SP3, &SwapMem, &FreeMemory);
                if (pars_read == 5) isFreeMemRead = true;
                break;
            }
		} while(c != EOF); 
        fclose(StatFilePtr);
        sprintf(SystemCmd, "rm %s%s\n", TempPath, MemStatFileName);
        system(SystemCmd);    
    }
	else
	{
		printf("Fail to open %s file for read\n", MemStatFileName);
	}
  #else
    /* Check memory usage for Linux platforms */
    StatFilePtr = fopen(MemStatFileName, "r");
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
            
            if (!isAvailMemRead)
            {
                i = FindCmdRequest(BufferPtr, "MemTotal:");
                if (i != -1)
                {
                    /* Available memory value read */
                    pars_read = sscanf(&BufferPtr[i], "%u", &AvailMemory);
                    if (pars_read > 0) isAvailMemRead = true;                                
                }
            }
            
            if (!isFreeMemRead)
            {
                i = FindCmdRequest(BufferPtr, "MemFree:");
                if (i != -1)
                {
                    /* Free memory value read */
                    pars_read = sscanf(&BufferPtr[i], "%u", &FreeMemory);
                    if (pars_read > 0) isFreeMemRead = true;
                }
            }

            if (!isBufferMemRead)
            {
                i = FindCmdRequest(BufferPtr, "Buffers:");
                if (i != -1)
                {
                    /* Buffers memory value read */
                    pars_read = sscanf(&BufferPtr[i], "%u", &BuffersMemory);
                    if (pars_read > 0) isBufferMemRead = true;
                }
            }

            if (!isCachMemRead)
            {
                i = FindCmdRequest(BufferPtr, "Cached:");
                if (i != -1)
                {
                    /* Cached memory value read */
                    pars_read = sscanf(&BufferPtr[i], "%u", &CachedMemory);
                    if (pars_read > 0) isCachMemRead = true;
                }
            }
            
            if (isAvailMemRead & isFreeMemRead & isBufferMemRead & isCachMemRead) break;
		} while(c != EOF); 
        fclose(StatFilePtr);
        if (SysMemUseFlag) FreeMemory += (BuffersMemory + CachedMemory);
    }
	else
	{
		printf("Fail to open %s file for read\n", MemStatFileName);
	}
  #endif
#endif
#ifdef WIN32
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    isAvailMemRead = true;
	isFreeMemRead = true;
    FreeMemory = (unsigned int)(memInfo.ullAvailPhys/MEM_MB_SIZE_DIV);
    AvailMemory = (unsigned int)(memInfo.ullTotalPhys/MEM_MB_SIZE_DIV);
#endif
    if (isAvailMemRead & isFreeMemRead)   
    {
        *MemProcPtr = (unsigned char)(((AvailMemory - FreeMemory)*100)/AvailMemory);
        Result = true;
    }
    free(BufferPtr);
	return Result;
}
//---------------------------------------------------------------------------
bool GetAvailableMemory(unsigned int *AvailMemPtr, char *TempPath)
{
	bool         Result = false;
    unsigned int AvailMemory = 0;
    bool         isAvailMemRead = false;
    int          size = 1024;
#ifdef _LINUX_X86_
    int          i, c, pos;
#endif
    int          pars_read = 0;
	FILE         *StatFilePtr = NULL;
    char         *BufferPtr = NULL;
#ifdef _SUN_BUILD_
    unsigned int SP1, SP2, SP3, SwapMem;
    char         SystemCmd[512];
#endif
#ifdef WIN32
	MEMORYSTATUSEX memInfo;
#endif

    if (!AvailMemPtr || !TempPath) return false;
	BufferPtr = (char*)malloc(size*sizeof(char)+1);
    if (!BufferPtr) return false;
    
#ifdef _LINUX_X86_
  #ifdef _SUN_BUILD_
    /* Check memory usage for SUN OS platforms */
    strcpy(SystemCmd, TempPath);
    strcat(SystemCmd, MemStatFileName);
    if (access(SystemCmd, F_OK) == 0)
	{            
        sprintf(SystemCmd, "rm %s%s\n", TempPath, MemStatFileName);
        system(SystemCmd);  
    }  
    sprintf(SystemCmd, "prtconf -p | grep Memory > %s%s\n", TempPath, MemStatFileName);
    system(SystemCmd);
    
    strcpy(SystemCmd, TempPath);
    strcat(SystemCmd, MemStatFileName);
    StatFilePtr = fopen(SystemCmd, "r");
    if(StatFilePtr) 
    {
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
        i = FindCmdRequest(BufferPtr, "Memory size:");
        if (i != -1)
        {
            pars_read = sscanf(&BufferPtr[i], "%u", &AvailMemory);
            if (pars_read > 0)
            {
                *AvailMemPtr = AvailMemory;
                isAvailMemRead = true;
            }
        }
        fclose(StatFilePtr);
        sprintf(SystemCmd, "rm %s%s\n", TempPath, MemStatFileName);
        system(SystemCmd);    
    }
	else
	{
		printf("Fail to open %s file for read\n", SystemCmd);
	}
  #elif _ESXI_PLATFORM_  
    isAvailMemRead = true;
    *AvailMemPtr = EtsiGetAvailMemory();  
  #else
    /* Check memory usage for Linux platforms */
    StatFilePtr = fopen(MemStatFileName, "r");
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
            i = FindCmdRequest(BufferPtr, "MemTotal:");
            if (i != -1)
            {
                /* Available memory value read */
                pars_read = sscanf(&BufferPtr[i], "%u", &AvailMemory);
                if (pars_read > 0) 
                {
                    *AvailMemPtr = AvailMemory/1024;
                    isAvailMemRead = true;
                    break;
                }
            }
		} while(c != EOF); 
        fclose(StatFilePtr);
    }
	else
	{
		printf("Fail to open %s file for read\n", MemStatFileName);
	}
  #endif
#endif
#ifdef WIN32
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    GlobalMemoryStatusEx(&memInfo);
    isAvailMemRead = true;
    *AvailMemPtr = (unsigned int)(memInfo.ullTotalPhys/MEM_MB_SIZE_DIV);
#endif
    if (isAvailMemRead) Result = true;
    free(BufferPtr);
	return Result;
}
//---------------------------------------------------------------------------
