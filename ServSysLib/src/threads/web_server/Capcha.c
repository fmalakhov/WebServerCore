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
//---------------------------------------------------------------------------
void LoadCapcha(PARAMWEBSERV *ParWebServ)
{
	int           i, pars_read, CapchaFileId;
	char		  *PathSearth = NULL;
    char          *FileNamePtr = NULL;
	char          *CwdRet = NULL;
#ifdef WIN32
	unsigned int  FileSize;
	HANDLE		  HDIRFILE;
	WIN32_FIND_DATA Data_File;
#else
    DIR           *DirPtr;
    struct dirent *DirEntPtr;
    struct stat   st;
#endif

	PathSearth = (char*)AllocateMemory(1024*sizeof(char));
	if (!PathSearth) return;
#ifdef WIN32
	CwdRet = _getcwd(PathSearth, 512);
#else
    CwdRet = getcwd(PathSearth, 512);
#endif
    ParWebServ->CapchaTableSize = 0;

#ifdef WIN32
	strcat(PathSearth, "\\CapchaImage\\*.*");	
	HDIRFILE = FindFirstFile((LPCWSTR)PathSearth, &Data_File);
	if (HDIRFILE != INVALID_HANDLE_VALUE)
    {
        while ( 1 )
	    {
            if ( !(Data_File.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY) )
		    {
				FileNamePtr = (char*)&Data_File.cFileName;
                FileSize = ((unsigned int)Data_File.nFileSizeHigh)<<16;
		        FileSize += (unsigned int)Data_File.nFileSizeLow;
				if ((FileSize < MAX_ADD_CASHE_FILE_LEN) &&
					(FindCmdRequest(FileNamePtr, "Thumbs.db") == -1))
				{
					for(;;)
					{
		                i = FindCmdRequest(FileNamePtr, ".png");
		                if (i == -1) break;
	                    pars_read = sscanf(FileNamePtr, "%d", &CapchaFileId);
	                    if (!pars_read) break;
					    ParWebServ->CapchaTable[ParWebServ->CapchaTableSize] = (unsigned int)CapchaFileId;
                        ParWebServ->CapchaTableSize++;
						break;
					}
				}
		    }
		    if (!FindNextFile(HDIRFILE,&Data_File) &&
		        (GetLastError() == ERROR_NO_MORE_FILES)) break;
	    }
        FindClose(HDIRFILE);
    }
#else
	strcat(PathSearth, "/CapchaImage");
    DirPtr = opendir(PathSearth);
	if (DirPtr)
    {
        while ((DirEntPtr = readdir(DirPtr)) != NULL)
	    {
            stat(PathSearth, &st);
			FileNamePtr = DirEntPtr->d_name;
            if (((st.st_mode & S_IFMT) != S_IFMT) &&
                (strcmp(FileNamePtr, ".") != 0) &&
                (strcmp(FileNamePtr, "..") != 0) &&
				((unsigned int)st.st_size < MAX_ADD_CASHE_FILE_LEN) &&
				(FindCmdRequest(FileNamePtr, "Thumbs.db") == -1))
		    {
				for(;;)
				{
		            i = FindCmdRequest(FileNamePtr, ".png");
		            if (i == -1) break;
	                pars_read = sscanf(FileNamePtr, "%d", &CapchaFileId);
	                if (!pars_read) break;
					ParWebServ->CapchaTable[ParWebServ->CapchaTableSize] = (unsigned int)CapchaFileId;
                    ParWebServ->CapchaTableSize++;
				    break;
				}
		    }
	    }
        closedir(DirPtr);
    }
#endif
	printf("There are %d capcha codes were detected\n", ParWebServ->CapchaTableSize);
	FreeMemory(PathSearth);
	return;
}
//---------------------------------------------------------------------------
unsigned int SetExpectSessionCapchaCode(PARAMWEBSERV *ParWebServ)
{
    return (unsigned int)(rand()) % ParWebServ->CapchaTableSize;
}
//---------------------------------------------------------------------------
bool UserCapchaCodeValidate(unsigned int ExpectedCapchaCode, unsigned int EnterCapchaCode)
{
    register unsigned int ExpectCapchaCode;

    ExpectCapchaCode = ParWebServPtr->CapchaTable[ExpectedCapchaCode];
	if (ExpectCapchaCode == (unsigned int)EnterCapchaCode) return true;
    else                                                   return false;
}
//---------------------------------------------------------------------------
