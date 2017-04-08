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

#include <sys/stat.h>
#include "TextListDataBase.h"
#include "HtmlMacrosHash.h"

extern ListItsTask  UserInfoList;
extern char *EndHtmlPageGenPtr;
extern char Text3SymConv[];
extern char Text4SymConv[];
extern char ServerVersion[];
extern char EngTextDbNamePath[];
extern char RusTextDbNamePath[];
extern unsigned char gLanguageType;
char AnsiToHtmlRusConver[(LOWER_RUS_YA_CHAR-UPPER_RUS_A_CHAR+2)*8];

HTML_MACROS_HASH_CHAR_HOP TextLineMacrosHashHop;

extern void HandleLineSetSysName(void *ParPtr);
extern void HandleLineSetServerVersion(void *ParPtr);
static bool HandleTextLineTemplate(void *ParWebServ, 
    unsigned char *LineTemplPtr, unsigned char *ParamBuf, unsigned int LineId);

static CMD_INFO TextCmdInLineList[] = {
    (unsigned char*)"SETSYSNAME",       HandleLineSetSysName,
	(unsigned char*)"SETSERVERVERSION", HandleLineSetServerVersion
};

#define TextCmdInLineListLen  sizeof(TextCmdInLineList)/sizeof(CMD_INFO)

static char TextListDbName[] = "TextListDB";

static void CharToHtmlTagConvert();

static unsigned char *RusLineIndexList[MAX_RUS_LINES_LIST];
static unsigned char *EngLineIndexList[MAX_RUS_LINES_LIST];
//---------------------------------------------------------------------------
void RusTextListLoad(void *ParWebServPtr)
{
	DWORD PointFind;
	bool     LastRot;
#ifdef WIN32
	DWORD  SizeFile;
	HANDLE HFSndMess;
#else
	unsigned long SizeFile;
        struct stat st;
#endif
	FILE   *FileHandler;
	int pars_read = 0;
    unsigned int  ParsedLines = 0;
	unsigned int  RusTextLine;
	unsigned char *FileData;
	char          *RetCodeCwd = NULL;
	unsigned int  i, read_blk;
	int  RusLineIndex;
	char RusTextItem[1024];
	char StartPath[512];
	char BdFileName[1024];

	for (i=0;i < MAX_RUS_LINES_LIST;i++) RusLineIndexList[i] = NULL;
	CharToHtmlTagConvert();
#ifdef WIN32
	RetCodeCwd = _getcwd((char*)(&StartPath[0]),512);
#else
	RetCodeCwd = getcwd((char*)(&StartPath[0]),512);
#endif
	strcpy(BdFileName, StartPath);
	strcat(BdFileName, RusTextDbNamePath);

#ifdef _LINUX_X86_
	FileHandler = fopen(BdFileName,"rb");
	if (!FileHandler) 
	{
        printf("File DB list (%s) dos not present\n", BdFileName);
	    return;
	}
        stat(BdFileName, &st);
        if ((st.st_mode & S_IFMT) != S_IFMT)
	{                
            SizeFile = (unsigned long)st.st_size;
        }
        else
        {
            printf("File DB list (%s) is not file\n", BdFileName);
            fclose(FileHandler);
            return;
        }
#endif

#ifdef WIN32
	HFSndMess = CreateFile((LPCWSTR)BdFileName, 0,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (HFSndMess == INVALID_HANDLE_VALUE)
	{
	   return;
	}
	SizeFile = GetFileSize( HFSndMess, NULL );
	CloseHandle( HFSndMess );
	FileHandler = fopen(BdFileName,"rb");
#endif

    InitHtmlMacrosCmdHash(&TextLineMacrosHashHop, TextCmdInLineList, TextCmdInLineListLen);
	FileData = (unsigned char*)AllocateMemory( SizeFile+2 );
	read_blk = fread((unsigned char*)FileData, 1, SizeFile, FileHandler);
	LastRot = false;
    PointFind = 0;
	RusTextLine = 0;
	while ( PointFind < SizeFile )
    {
		if ( FileData[PointFind] =='\r' || FileData[PointFind] =='\n')
        {
			if ( LastRot ) goto NoMove;
            if ( FileData[PointFind] =='\r' ) LastRot = true;
            RusTextItem[RusTextLine] = 0;
            if ((RusTextItem[0] !='#') && (FindCmdRequest( RusTextItem, "comment") == -1))
            {
				if ( RusTextItem[0] !='\r' && RusTextItem[0] !='\n')
		        {
					LastRot = true;
			        RusTextItem[RusTextLine] = 0;
                    pars_read = sscanf(RusTextItem, "%d", &RusLineIndex);
					if ((pars_read) == 1 && (RusLineIndex < MAX_RUS_LINES_LIST))
					{
						for (i=0;i < RusTextLine;i++)
						{
							if (RusTextItem[i] == ' ')
							{
								break;
							}
						}
						RusLineIndexList[RusLineIndex] = (unsigned char*)AllocateMemory(RusTextLine+256);
                        if (RusLineIndexList[RusLineIndex])
                        {
                            if (!HandleTextLineTemplate(ParWebServPtr, (unsigned char*)&RusTextItem[i+1],
                                (unsigned char*)RusLineIndexList[RusLineIndex], RusLineIndex))
                            {
                                FreeMemory(RusLineIndexList[RusLineIndex]);
                                EngLineIndexList[RusLineIndex] = NULL;
                            }
                            else
                            {
                                ParsedLines++;                            
                            }
                        } 
                        else
                        {
                            printf("Fail to memory allocation for text line\n");
                        }                       
					}
			        RusTextLine = 0;
			        PointFind++;
		        }
		    }
		    else
		    {
				RusTextLine = 0;
			    FileData[RusTextLine] = 0;
		    }
		    PointFind++;
	   }
       else
       {
   NoMove:
           if (FileData[PointFind] !='\r' && FileData[PointFind] !='\n')
           {
			   RusTextItem[RusTextLine] = FileData[PointFind];
               RusTextLine++;
           }
           PointFind++;
		   LastRot = false;
        }
    }
	fclose(FileHandler);
	FreeMemory(FileData);    
    CloseHtmlMacrosHash(&TextLineMacrosHashHop);    
    printf("Russian text dictonary load is comleted (Text lines: %d)\n", ParsedLines);
}
//---------------------------------------------------------------------------
void EngTextListLoad(void *ParWebServPtr)
{
	DWORD PointFind;
	bool     LastRot;
#ifdef WIN32
	DWORD  SizeFile;
	HANDLE HFSndMess;
#endif
#ifdef _LINUX_X86_
	unsigned long SizeFile;
        struct stat st;
#endif
	FILE   *FileHandler;
	int pars_read = 0;
    unsigned int  ParsedLines = 0;
	unsigned int  RusTextLine;
	unsigned char *FileData;
	char          *RetCodeCwd = NULL;
	unsigned int  i, read_blk;
	int  EngLineIndex;
	char EngTextItem[1024];
	char StartPath[512];
	char BdFileName[1024];

	for (i=0;i < MAX_RUS_LINES_LIST;i++) EngLineIndexList[i] = NULL;
#ifdef WIN32
	RetCodeCwd = _getcwd((char*)(&StartPath[0]),512);
#else
	RetCodeCwd = getcwd((char*)(&StartPath[0]),512);
#endif
	strcpy(BdFileName, StartPath);
	strcat(BdFileName, EngTextDbNamePath);

#ifdef _LINUX_X86_
	FileHandler = fopen(BdFileName,"rb");
	if (!FileHandler) 
	{
        printf("File DB list (%s) dos not present\n", BdFileName);
	    return;
	}
        stat(BdFileName, &st);
        if ((st.st_mode & S_IFMT) != S_IFMT)
	{                
            SizeFile = (unsigned long)st.st_size;
        }
        else
        {
            printf("File DB list (%s) is not file\n", BdFileName);
            fclose(FileHandler);
            return;
        }
#endif

#ifdef WIN32
	HFSndMess = CreateFile((LPCWSTR)BdFileName, 0,FILE_SHARE_READ,NULL,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
	if (HFSndMess == INVALID_HANDLE_VALUE)
	{
	   return;
	}
	SizeFile = GetFileSize( HFSndMess, NULL );
	CloseHandle( HFSndMess );
	FileHandler = fopen(BdFileName,"rb");
#endif

    InitHtmlMacrosCmdHash(&TextLineMacrosHashHop, TextCmdInLineList, TextCmdInLineListLen);
	FileData = (unsigned char*)AllocateMemory( SizeFile+2 );
	read_blk = fread((unsigned char*)FileData, 1, SizeFile, FileHandler);
	LastRot = false;
    PointFind = 0;
	RusTextLine = 0;
	while ( PointFind < SizeFile )
    {
		if ( FileData[PointFind] =='\r' || FileData[PointFind] =='\n')
        {
			if ( LastRot ) goto NoMove;
            if ( FileData[PointFind] =='\r' ) LastRot = true;
            EngTextItem[RusTextLine] = 0;
            if ((EngTextItem[0] !='#') && (FindCmdRequest( EngTextItem, "comment") == -1 ))
            {
				if ( EngTextItem[0] !='\r' && EngTextItem[0] !='\n')
		        {
					LastRot = true;
			        EngTextItem[RusTextLine] = 0;
                    pars_read = sscanf(EngTextItem, "%d", &EngLineIndex);
					if ((pars_read) == 1 && (EngLineIndex < MAX_RUS_LINES_LIST))
					{
						for (i=0;i < RusTextLine;i++)
						{
							if (EngTextItem[i] == ' ')
							{
								break;
							}
						}                        
						EngLineIndexList[EngLineIndex] = (unsigned char*)AllocateMemory(RusTextLine+256);
                        if (EngLineIndexList[EngLineIndex])
                        {
                            if (!HandleTextLineTemplate(ParWebServPtr, (unsigned char*)&EngTextItem[i+1],
                                (unsigned char*)EngLineIndexList[EngLineIndex], EngLineIndex))
                            {
                                FreeMemory(EngLineIndexList[EngLineIndex]);
                                EngLineIndexList[EngLineIndex] = NULL;
                            }
                            else
                            {
                                ParsedLines++;                            
                            }
                        }
                        else
                        {
                            printf("Fail to memory allocation for text line\n");
                        }
					}
			        RusTextLine = 0;
			        PointFind++;
		        }
		    }
		    else
		    {
				RusTextLine = 0;
			    FileData[RusTextLine] = 0;
		    }
		    PointFind++;
	   }
       else
       {
   NoMove:
           if (FileData[PointFind] !='\r' && FileData[PointFind] !='\n')
           {
			   EngTextItem[RusTextLine] = FileData[PointFind];
               RusTextLine++;
           }
           PointFind++;
		   LastRot = false;
        }
    }
	fclose(FileHandler);
	FreeMemory(FileData);
    CloseHtmlMacrosHash(&TextLineMacrosHashHop);    
    printf("English text dictonary load is comleted (Text lines: %d)\n", ParsedLines);
}
//---------------------------------------------------------------------------
static void CharToHtmlTagConvert()
{
	unsigned int SrcCharId;
	unsigned int ResCharId;
	char CharIdTxtBuf[16];
	char *BufPtr;

	BufPtr = &AnsiToHtmlRusConver[0];
    for(SrcCharId=UPPER_RUS_A_CHAR;SrcCharId <= LOWER_RUS_YA_CHAR;SrcCharId++)
	{
	    ResCharId = SrcCharId - UPPER_RUS_A_CHAR + 1040;
		sprintf(CharIdTxtBuf, "&#%d;", ResCharId);
		memcpy(BufPtr, CharIdTxtBuf, 7);
		BufPtr += 8;
	}
}
//---------------------------------------------------------------------------
void SetRusTextBuf(char *BufAnsw, unsigned int RusTextIndex)
{
	unsigned int index, TextLen;
	unsigned int CharId;
	unsigned char SrcSymb;
	char *RusAnsiPtr = NULL;
	char *BufPtr = NULL;
	char *StartPtr = NULL;

	if ((RusTextIndex < MAX_RUS_LINES_LIST) &&
		(RusLineIndexList[RusTextIndex] != NULL) &&
        (EngLineIndexList[RusTextIndex] != NULL))
	{
	    if (BufAnsw) BufPtr = BufAnsw + strlen(BufAnsw);
		else         BufPtr = EndHtmlPageGenPtr;
		StartPtr = BufPtr;
        switch(gLanguageType)
        {
            case LGT_RUSSIAN:
	            RusAnsiPtr = (char*)RusLineIndexList[RusTextIndex];
                break;
                
            default:
                RusAnsiPtr = (char*)EngLineIndexList[RusTextIndex];
                break;
        }               
	    TextLen = strlen((const char*)RusAnsiPtr);
	    for (index=0;index < TextLen;index++)
		{
			SrcSymb = (unsigned char)*RusAnsiPtr;
		    if (SrcSymb >= (unsigned char)UPPER_RUS_A_CHAR)
		    {
				CharId = ((unsigned int)(SrcSymb) - UPPER_RUS_A_CHAR) << 3;
				memcpy(BufPtr, &AnsiToHtmlRusConver[CharId], 7);
			    BufPtr += 7;
		    }
		    else
		    {
			    *BufPtr++ = SrcSymb;
		    }
            RusAnsiPtr++;
	    }
		*BufPtr = 0;
		if (!BufAnsw) EndHtmlPageGenPtr += (unsigned int)(BufPtr - StartPtr);
	}
}
//---------------------------------------------------------------------------
int SetRusTextBufLen(char *BufPtr, unsigned int RusTextIndex, unsigned int MaxLen)
{
	unsigned int index, TextLen, SubLen = 0;
	unsigned int CharId, CharCount = 0;
	char *RusAnsiPtr = NULL;
	unsigned char SrcSymb;

	if ((RusTextIndex < MAX_RUS_LINES_LIST) &&
		(RusLineIndexList[RusTextIndex] != NULL) &&
        (EngLineIndexList[RusTextIndex] != NULL))
	{
        switch(gLanguageType)
        {
            case LGT_RUSSIAN:
	            RusAnsiPtr = (char*)RusLineIndexList[RusTextIndex];
                break;
                
            default:
                RusAnsiPtr = (char*)EngLineIndexList[RusTextIndex];
                break;
        }
	    TextLen = strlen((const char*)RusAnsiPtr);
	    for (index=0;index < TextLen;index++)
	    {
			SrcSymb = (unsigned char)(RusAnsiPtr[index]);
		    if (SrcSymb >= UPPER_RUS_A_CHAR)
		    {
				if ((MaxLen-SubLen) < 6) break;
				CharId = ((unsigned int)(SrcSymb) - UPPER_RUS_A_CHAR) << 3;
				memcpy(&BufPtr[SubLen], &AnsiToHtmlRusConver[CharId], 7);
			    SubLen += 7;
		    }
		    else
		    {
				if ((MaxLen-SubLen) < 1) break;
			    BufPtr[SubLen++] = RusAnsiPtr[index];
		    }
			CharCount++;
	    }
		BufPtr[SubLen] = 0;
	}
	return CharCount;
}
//---------------------------------------------------------------------------
void SetRusTextBufName(char *BufAnsw, unsigned char *RusAnsiPtr)
{
	unsigned int index, TextLen;
	unsigned int CharId;
	char *BufPtr = NULL;
	char *StartPtr = NULL;
    unsigned char SrcSymb;

	if (BufAnsw) BufPtr = BufAnsw + strlen(BufAnsw);
	else         BufPtr = EndHtmlPageGenPtr;
	TextLen = strlen((const char*)RusAnsiPtr);
	StartPtr = BufPtr;

	for (index=0;index < TextLen;index++)
	{
		SrcSymb = (unsigned char)*RusAnsiPtr;
		if (SrcSymb >= UPPER_RUS_A_CHAR)
		{
			CharId = ((unsigned int)(SrcSymb) - UPPER_RUS_A_CHAR) << 3;
			memcpy(BufPtr, &AnsiToHtmlRusConver[CharId], 7);
			BufPtr += 7;
		}
		else
		{
			*BufPtr++ = SrcSymb;
		}
		RusAnsiPtr++;
	}
    *BufPtr = 0;
	if (!BufAnsw) EndHtmlPageGenPtr += (unsigned int)(BufPtr - StartPtr);
}
//---------------------------------------------------------------------------
void SetProtectRusTextBufName(char *BufAnsw, unsigned char *RusAnsiPtr)
{
	unsigned int index, TextLen;
	unsigned int CharId;
	char *BufPtr = NULL;
	char *StartPtr = NULL;
    unsigned char SrcSymb;

	if (BufAnsw) BufPtr = BufAnsw + strlen(BufAnsw);
	else         BufPtr = EndHtmlPageGenPtr;
	TextLen = strlen((const char*)RusAnsiPtr);
	StartPtr = BufPtr;

	for (index=0;index < TextLen;index++)
	{
		SrcSymb = (unsigned char)*RusAnsiPtr;
		if (SrcSymb >= UPPER_RUS_A_CHAR)
		{
			CharId = ((unsigned int)(SrcSymb) - UPPER_RUS_A_CHAR) << 3;
			memcpy(BufPtr, &AnsiToHtmlRusConver[CharId], 7);
			BufPtr += 7;
		}
		else
		{
			switch(SrcSymb)
			{
			    case '<':
				    memcpy(BufPtr, Text3SymConv, 4);
                    BufPtr += 4;
					break;

				case '>':
				    memcpy(BufPtr, Text4SymConv, 4);
                    BufPtr += 4;
					break;

			    default:
			        *BufPtr++ = SrcSymb;
					break;
			}
		}
		RusAnsiPtr++;
	}
    *BufPtr = 0;
	if (!BufAnsw) EndHtmlPageGenPtr += (unsigned int)(BufPtr - StartPtr);
}
//---------------------------------------------------------------------------
void SetOriginalRusTextBuf(char *BufAnsw, unsigned int RusTextIndex)
{
	unsigned int TextLen;
	char *RusAnsiPtr = NULL;
	char *BufPtr = NULL;

	if ((RusTextIndex < MAX_RUS_LINES_LIST) &&
		(RusLineIndexList[RusTextIndex] != NULL) &&
        (EngLineIndexList[RusTextIndex] != NULL))
	{
		if (BufAnsw) BufPtr = BufAnsw + strlen(BufAnsw);
		else         BufPtr = EndHtmlPageGenPtr;
        switch(gLanguageType)
        {
            case LGT_RUSSIAN:
	            RusAnsiPtr = (char*)RusLineIndexList[RusTextIndex];
                break;
                
            default:
                RusAnsiPtr = (char*)EngLineIndexList[RusTextIndex];
                break;
        }
	    TextLen = strlen((const char*)RusAnsiPtr);
		memcpy(BufPtr, RusAnsiPtr, TextLen);
		BufPtr[TextLen] = 0;
		if (!BufAnsw) EndHtmlPageGenPtr += TextLen;
	}
}
//---------------------------------------------------------------------------
static bool HandleTextLineTemplate(void *ParWebServ, 
    unsigned char *LineTemplPtr, unsigned char *ParamBuf, unsigned int LineId)
{
	bool          isCmdParseDone = false;
	bool          TextLineParseResult = true;
	bool          isTextLineParseDone = false;
	bool          isCommentLine = false;
	unsigned char ParseZone = 0;
	unsigned char *CmdNamePtr = NULL;
	unsigned char *ParBufPtr = NULL;
	CMD_INFO      *CmdInfPtr = NULL;
	TEXT_CMD_PAR  LineCmdPar;
	unsigned int  CharsLineCnt = 0;
	unsigned char SrcSymb;
	unsigned char CmdNameBuf[64];

	ParBufPtr = ParamBuf;
    *ParamBuf = 0;
    LineCmdPar.ParWebServ = ParWebServ;
    for(;;)
	{
		switch(*LineTemplPtr)
		{
		    case '$':
				if (isCommentLine)
				{
					LineTemplPtr++;
					break;
				}
                
	            CmdNamePtr = &CmdNameBuf[0];
	            CmdNameBuf[0] = 0;
				LineTemplPtr++;
				isCmdParseDone = false;
				for(;;)
				{
					switch (*LineTemplPtr)
					{
						case ';':
							*CmdNamePtr = 0;
                            LineTemplPtr++;
							LineCmdPar.ParBufPtr = ParBufPtr;
							CmdInfPtr = FindHtmlMacrosCmdHash(&TextLineMacrosHashHop, (char*)CmdNameBuf);
							if (CmdInfPtr)
							{
                                (CmdInfPtr->HtmlFunction)(&LineCmdPar);
								ParBufPtr = &ParamBuf[strlen((const char*)ParamBuf)];
							}
							else
							{
                                printf("%s: (Line: %u) Error 1 - unknown command name (%s) was detected.\r\n", 
			                        TextListDbName, LineId, CmdNameBuf);
								TextLineParseResult = false;
							}
							isCmdParseDone = true;
							ParseZone = 0;
							break;

						case 0:
							isCmdParseDone = true;
							isTextLineParseDone = true;
                            TextLineParseResult = false;
                            printf("%s: (Line: %u) Error 2 text template line processing.\r\n",\
                                TextListDbName, LineId);
							break;

						case '\r':
						case '\n':
							isCmdParseDone = true;
                            TextLineParseResult = false;
							break;

						default:
							switch(ParseZone)
							{
							    case 0:
									if ((unsigned long)(CmdNamePtr - &CmdNameBuf[0]) < 63)
							            *CmdNamePtr++ = *LineTemplPtr++;
								    else
									{
							            isCmdParseDone = true;
										isTextLineParseDone = true;
                                        TextLineParseResult = false;
	                                    printf("%s: (Line: %u) Error 3 text template line processing.\r\n",
                                            TextListDbName, LineId);
									}
								    break;

								default:
									if (*LineTemplPtr != ' ')
									{
							            isCmdParseDone = true;
										isTextLineParseDone = true;
                                        TextLineParseResult = false;
                                        printf("%s: (Line: %u) Error 4 text template line processing.\r\n",
                                            TextListDbName, LineId);
									}
									break;
							}
							break;
					}
					if (isCmdParseDone) break;
				}
                break;

			case 0:
				isTextLineParseDone = true;
				break;

			default:
			    SrcSymb = (unsigned char)*LineTemplPtr;
				/* Check for first symbol in string - if # than this line is comment */
				if ((SrcSymb == '#') && (!CharsLineCnt)) isCommentLine = true;
				if ((SrcSymb != '\r') && (SrcSymb !='\n')) CharsLineCnt++;
				if ((!isCommentLine) && (CharsLineCnt > 0))
				{
			        *ParBufPtr++ = *LineTemplPtr;
				}
				if (SrcSymb == '\n')
				{
					CharsLineCnt = 0;
					isCommentLine = false;
                    isTextLineParseDone = true;
				}
				LineTemplPtr++;
				break;
		}
		if (isTextLineParseDone) break;
	}
	*ParBufPtr = 0;
	return TextLineParseResult;
}
//---------------------------------------------------------------------------
