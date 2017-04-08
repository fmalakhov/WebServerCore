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
#include "BackupFilesRollback.h"

char BackupObjExt[] = ".backup";

void ServerVersionSet();
//---------------------------------------------------------------------------
#ifdef WIN32
void BackupFilesRollback(FILE *BatFilePtr, char *StartPathPtr, char *ExecObjName, 
    char *ExecObjMarker, unsigned int *UpgradeObjCountPtr)
#else
void BackupFilesRollback(char *StartPathPtr, char *SudoCmd, char *ExecObjName, 
    char *ExecObjMarker, unsigned int *UpgradeObjCountPtr)
#endif
{
    int           i, j; 
	char		  *PathSearth = NULL;
    char		  *SysCmd = NULL;
    char          *FileNamePtr = NULL;
#ifdef WIN32
	HANDLE		  HDIRFILE;
	WIN32_FIND_DATA Data_File;
#endif
#ifdef _LINUX_X86_
    int           SysRet;
    DIR           *DirPtr;
    struct dirent *DirEntPtr;
    struct stat   st;
#endif
    char          ObjName[256];

	PathSearth = (char*)malloc(1024*sizeof(char));
    if (!PathSearth)
    {
        printf("Fail to memory allocation for objects rollback (%d)\n", __LINE__);
        return;
    }
    SysCmd = (char*)malloc(4096*sizeof(char));
    if (!SysCmd)
    {
        printf("Fail to memory allocation for objects rollback (%d)\n", __LINE__);
        free(PathSearth);
        return;
    }
	strcpy(PathSearth, StartPathPtr); 
    FileNamePtr = &PathSearth[strlen(PathSearth)];    
#ifdef _LINUX_X86_
    DirPtr = opendir(StartPathPtr);
	if (DirPtr)
    {
        while ((DirEntPtr = readdir(DirPtr)) != NULL)
	    {
			strcpy(FileNamePtr, "/");
		    strcat(FileNamePtr, (char*)DirEntPtr->d_name);
            stat(PathSearth, &st);
            i = FindCmdRequest(DirEntPtr->d_name, BackupObjExt);
            j = FindCmdRequest(DirEntPtr->d_name, ExecObjMarker);
            if (((st.st_mode & S_IFMT) != S_IFMT) && (i != -1))
		    {
                i -= (sizeof(BackupObjExt)/sizeof(char) - 1);
                memcpy(ObjName, DirEntPtr->d_name, i);
                ObjName[i] = 0;
                *FileNamePtr = 0;
                printf("%s Rollup object: %s\n", SetTimeStampLine(), ObjName);                                
                if (j != -1)
                {
                    /* Executible file rollback */
                    strcpy(ExecObjName, ObjName);
                    
                    /* Current version remove */
                    sprintf(SysCmd, "%srm %s/%s\n", SudoCmd, PathSearth, ObjName);
                    SysRet = system(SysCmd);

                    /* Backup version move to new version */
                    sprintf(SysCmd, "%smv %s/%s%s %s/%s\n", SudoCmd, PathSearth, ObjName, 
                        BackupObjExt, PathSearth, ObjName);
                    SysRet = system(SysCmd);                    
                }
                else
                {                
                    /* Backup version copy to new version */
                    sprintf(SysCmd, "%scp %s/%s%s %s/%s\n", SudoCmd, PathSearth, ObjName, 
                        BackupObjExt, PathSearth, ObjName);
                    SysRet = system(SysCmd);

                    /* Backup version remove */
                    sprintf(SysCmd, "%srm %s/%s%s\n", SudoCmd, PathSearth, ObjName, BackupObjExt);
                    SysRet = system(SysCmd);
                }
                *UpgradeObjCountPtr = *UpgradeObjCountPtr + 1;
		    }
            else if (((st.st_mode & S_IFMT) == S_IFDIR) &&
                (strcmp(DirEntPtr->d_name, ".") != 0) &&
                (strcmp(DirEntPtr->d_name, "..") != 0))
            {
                /* Check files for rollback in sub directory */
                BackupFilesRollback(PathSearth, SudoCmd, ExecObjName, 
                    ExecObjMarker, UpgradeObjCountPtr);
            }
            else if (j != -1)
            {
                /* Executible object is found */
                strcpy(ExecObjName, DirEntPtr->d_name);
            }
	    }
        closedir(DirPtr);
    }
#endif    
#ifdef WIN32    
	strcpy(FileNamePtr, "\\*.*");	
	HDIRFILE = FindFirstFile((LPCTSTR)PathSearth, (LPWIN32_FIND_DATA)&Data_File);
	if (HDIRFILE != INVALID_HANDLE_VALUE)
    {
        while ( 1 )
	    {
            if (!(Data_File.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))
		    {
				strcpy(FileNamePtr, "\\");
		        strcat(FileNamePtr, (char*)&Data_File.cFileName);            
                i = FindCmdRequest((char*)(Data_File.cFileName), BackupObjExt);
                j = FindCmdRequest((char*)(Data_File.cFileName), ExecObjMarker);
                if (i != -1)
		        {
                    i -= (sizeof(BackupObjExt)/sizeof(char) - 1);
                    memcpy(ObjName, Data_File.cFileName, i);
                    ObjName[i] = 0;
                    *FileNamePtr = 0;
                    printf("%s Rollup object: %s\r\n", SetTimeStampLine(), ObjName);

                    if (j != -1)
                    {
                        /* Executible file rollback */
                        strcpy(ExecObjName, ObjName);

                        /* Current version remove */
                        sprintf(SysCmd, "del %s\\%s.exe\r\n", PathSearth, ObjName);
                        fwrite(SysCmd, strlen(SysCmd), 1, BatFilePtr);
                        
                        /* Backup version move to new version */
                        sprintf(SysCmd, "move %s\\%s%s %s\\%s.exe\r\n", PathSearth, ObjName, 
                            BackupObjExt, PathSearth, ObjName);
                        fwrite(SysCmd, strlen(SysCmd), 1, BatFilePtr);
                    }
                    else
                    {                
                        /* Backup version copy to new version */
                        sprintf(SysCmd, "copy %s\\%s%s %s\\%s\r\n", PathSearth, ObjName, 
                            BackupObjExt, PathSearth, ObjName);
                        fwrite(SysCmd, strlen(SysCmd), 1, BatFilePtr);

                        /* Backup version remove */
                        sprintf(SysCmd, "del %s\\%s%s\r\n", PathSearth, ObjName, BackupObjExt);
                        fwrite(SysCmd, strlen(SysCmd), 1, BatFilePtr);
                    }
                    *UpgradeObjCountPtr = *UpgradeObjCountPtr + 1;
		        }
		    }
            else
            {
                /* Check files for rollback in sub directory */
                BackupFilesRollback(BatFilePtr, PathSearth, ExecObjName, 
                    ExecObjMarker, UpgradeObjCountPtr);
            }
		    if (!FindNextFile(HDIRFILE, &Data_File) &&
		        (GetLastError() == ERROR_NO_MORE_FILES)) break;
	    }
        FindClose(HDIRFILE);
    }
#endif
    free(SysCmd);
    free(PathSearth);
}
//---------------------------------------------------------------------------
