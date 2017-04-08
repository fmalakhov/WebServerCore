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

extern char ThrWebServName[];
extern PARAMWEBSERV *ParWebServPtr;
extern STATS_INFO ServerStats;
extern char LogsGrpName[];
extern char StatsNameExt[];
extern char StatsHistNameExt[];
#ifdef WIN32
extern HANDLE gFileMutex;
#endif
//---------------------------------------------------------------------------
void StatsDataSave()
{
	FILE  *FileStasPtr = NULL;
	char  *PathFilePtr = NULL;
	char  *CwdRet = NULL;
	DWORD WaitResult;
#ifdef WIN32
	SYSTEMTIME CurrTime;
	bool       isFileOperReady = false;
#else       
    struct timeb hires_cur_time;
    struct tm    *cur_time;
#endif
	char  StatsInfoLine[512];

#ifdef WIN32
    WaitResult = WaitForSingleObject(gFileMutex, INFINITE);
    switch(WaitResult)
	{
	    case WAIT_OBJECT_0:
			isFileOperReady = true;
		    break;

        case WAIT_ABANDONED: 
			printf("The other thread that using mutex is closed in locked state of mutex\r\n");
            FreeMemory(PathFilePtr);
            break;

		default:
			printf("Report manager (stats save) mutex is fialed with error: %d\r\n", GetLastError());
			FreeMemory(PathFilePtr);
			break;
	}
    if (!isFileOperReady) return;
#endif

	PathFilePtr = (char*)AllocateMemory(1024*sizeof(char));
	if (!PathFilePtr) return;
#ifdef WIN32
	CwdRet = _getcwd(PathFilePtr, 512);
    strcat(PathFilePtr, "\\");
#else
	CwdRet = getcwd(PathFilePtr, 512);
    strcat(PathFilePtr, "/");
#endif
    strcat(PathFilePtr, LogsGrpName);
	strcat(PathFilePtr, StatsNameExt);
	FileStasPtr = fopen(PathFilePtr,"wb");
	if (!FileStasPtr) 
	{
		FreeMemory(PathFilePtr);
#ifdef WIN32
        if (! ReleaseMutex(gFileMutex)) 
		{ 
            printf("Fail to release mutex (stats save) in report manager task\r\n");
		}
#endif
		return;
	}
	fwrite(&ServerStats, sizeof(STATS_INFO), 1, FileStasPtr);
	fclose(FileStasPtr);

#ifdef WIN32
	CwdRet = _getcwd(PathFilePtr, 512);
    strcat(PathFilePtr, "\\");
#else
	CwdRet = getcwd(PathFilePtr, 512);
    strcat(PathFilePtr, "/");
#endif
    strcat(PathFilePtr, LogsGrpName);
	strcat(PathFilePtr, StatsHistNameExt);
    
	FileStasPtr = fopen(PathFilePtr,"ab");
	if (!FileStasPtr) 
	{
		printf("Fail to open %s stats file for append\r\n", PathFilePtr);
		FreeMemory(PathFilePtr);
#ifdef WIN32
        if (! ReleaseMutex(gFileMutex)) 
		{ 
            printf("Fail to release mutex (stats save) in report manager task\r\n");
		}
#endif
		return;
	}

#ifdef WIN32        
    GetSystemTime(&CurrTime);
	sprintf(StatsInfoLine, "%02d/%02d/%04d %02d:%02d:%02d % 8d % 8d % 8d % 8d % 8d % 8d % 8d\n",
		CurrTime.wDay, CurrTime.wMonth, CurrTime.wYear,
		CurrTime.wHour, CurrTime.wMinute, CurrTime.wSecond,   
#else        
    ftime(&hires_cur_time);
    cur_time = localtime(&hires_cur_time.time);
    sprintf(StatsInfoLine, "%02d/%02d/%04d %02d:%02d:%02d % 8d % 8d % 8d % 8d % 8d % 8d % 8d\n",
           cur_time->tm_mday, (cur_time->tm_mon+1),
           (cur_time->tm_year+1900), cur_time->tm_hour,
           cur_time->tm_min, cur_time->tm_sec, 
#endif 
		ServerStats.IntGoodUserReq, 
		ServerStats.IntBadUserReq, ServerStats.IntUserSessions,
        ServerStats.IntBotSessions, ServerStats.IntGoodBotReq, 
		ServerStats.IntSendHtmlPages, ServerStats.IntSendFiles);

	fwrite(&StatsInfoLine[0], strlen(StatsInfoLine), 1, FileStasPtr);
	fclose(FileStasPtr);

	ServerStats.IntUserSessions = 0;
    ServerStats.IntGoodUserReq = 0;
    ServerStats.IntBotSessions = 0;
	ServerStats.IntGoodBotReq = 0;
    ServerStats.IntBadUserReq = 0;
	ServerStats.IntSendHtmlPages = 0;
	ServerStats.IntSendFiles = 0;

#ifdef WIN32
    if (! ReleaseMutex(gFileMutex)) 
	{ 
        printf("Fail to release mutex (stats save) in report manager task\r\n");
	}
#endif
}
//---------------------------------------------------------------------------
void StatsDataInit()
{
	FILE         *FileStasPtr = NULL;
	char         *PathFilePtr = NULL;
	char         *CwdRet = NULL;
	unsigned int ReadFileSize;        
#ifdef WIN32        
	DWORD        SizeFile;
	HANDLE       HFStatsData;
#else
    unsigned int SizeFile;
    struct stat  st;
#endif

	PathFilePtr = (char*)AllocateMemory(1024*sizeof(char));
	if (!PathFilePtr) return;
#ifdef WIN32
	CwdRet = _getcwd(PathFilePtr, 512);
    strcat(PathFilePtr, "\\");
#else
	CwdRet = getcwd(PathFilePtr, 512);
    strcat(PathFilePtr, "/");
#endif
    strcat(PathFilePtr, LogsGrpName);
	strcat(PathFilePtr, StatsNameExt);

	memset(&ServerStats, 0, sizeof(STATS_INFO));
	FileStasPtr = fopen(PathFilePtr,"rb");
	if (!FileStasPtr) 
	{
		StatsDataSave();
		FreeMemory(PathFilePtr);
		return;
	}
#ifdef _LINUX_X86_
        stat(PathFilePtr, &st);
        if ((st.st_mode & S_IFMT) != S_IFMT)
        {
            SizeFile = (unsigned int)st.st_size;
        }
#endif
#ifdef WIN32 
	HFStatsData = CreateFile((LPCWSTR)PathFilePtr, 0, FILE_SHARE_READ,NULL,
		OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (HFStatsData == INVALID_HANDLE_VALUE)
	{
		/* Problem of already open data file*/
	    FreeMemory(PathFilePtr);
	    fclose(FileStasPtr);
	    return;
	}
	SizeFile = GetFileSize(HFStatsData, NULL);
	CloseHandle(HFStatsData);
#endif        
        
	if (SizeFile != sizeof(STATS_INFO))
	{
		/* Statistic file was corrupted */
		fclose(FileStasPtr);
		StatsDataSave();
	}
	else
	{
	    ReadFileSize = fread(&ServerStats, 1, sizeof(STATS_INFO), FileStasPtr);
		fclose(FileStasPtr);
	}

	/* Interval related stats clean */
	ServerStats.IntUserSessions = 0;
    ServerStats.IntGoodUserReq = 0;
    ServerStats.IntBotSessions = 0;
	ServerStats.IntGoodBotReq = 0;
    ServerStats.IntBadUserReq = 0;
	ServerStats.IntSendHtmlPages = 0;
	ServerStats.IntSendFiles = 0;

	FreeMemory(PathFilePtr);
}
//---------------------------------------------------------------------------
