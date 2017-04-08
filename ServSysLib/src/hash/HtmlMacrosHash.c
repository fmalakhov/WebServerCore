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
#include "HtmlMacrosHash.h"

unsigned int HtmlMacroHashEntityCnt = 0;
unsigned int HtmlMacroRecInHashCnt = 0;

HTML_MACROS_HASH_CHAR_HOP HtmlBodyMacrosHashHop;
HTML_MACROS_HASH_CHAR_HOP HtmlLineMacrosHashHop;

static unsigned char AsciiHtmlMacrosCharToIndex[] ={
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,   0,   0,   0,   0,   0,   0,
   0,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,
  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,   0,   0,   0,   0,  29,
   0,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,
  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,   0,   0,   0,   0,  29,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 };
//---------------------------------------------------------------------------
void InitHtmlMacrosHash(HTML_MACROS_HASH_CHAR_HOP *NameHop)
{
	memset(NameHop, 0, sizeof(HTML_MACROS_HASH_CHAR_HOP));	
}
//---------------------------------------------------------------------------
bool AddHtmlMacrosHash(HTML_MACROS_HASH_CHAR_HOP *RootNameHop, 
					 char *HtmlMacrosName, void *HtmlMacrosHandlerPtr)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex, NameLen;
	HTML_MACROS_HASH_CHAR_HOP *NewHashHopCharPtr = NULL;
	HTML_MACROS_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	HTML_MACROS_HASH_RECORD   *NewRecordPtr = NULL;

	NameLen = strlen(HtmlMacrosName);
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	   HopIndex = AsciiHtmlMacrosCharToIndex[(unsigned char)HtmlMacrosName[CharIndex]];
	   if (!HopIndex)
	   {
		   if ((HtmlMacrosName[CharIndex] < '0') || (HtmlMacrosName[CharIndex] > '9'))
		       printf("Add HTML to hash is failed for (%s)\n", HtmlMacrosName);
		   break;
	   }
	   if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	   {
	       /* New char in hash for this hop is detected */
		   NewHashHopCharPtr = (HTML_MACROS_HASH_CHAR_HOP*)AllocateMemory(sizeof(HTML_MACROS_HASH_CHAR_HOP));
		   if (!NewHashHopCharPtr)
		   {
		       Result = false;
		       break;
		   }
		   InitHtmlMacrosHash(NewHashHopCharPtr);
		   SelHashHopCharPtr->HashCharHop[HopIndex] = NewHashHopCharPtr;
		   SelHashHopCharPtr->UsedCharCount++;
		   NewHashHopCharPtr->ParentHashCharPtr = (void*)SelHashHopCharPtr;
		   NewHashHopCharPtr->ParentHashCharIndex = HopIndex;
		   SelHashHopCharPtr = NewHashHopCharPtr;
		   HtmlMacroHashEntityCnt++;
	   }
	   else
	   {
	       /* Existing char in hash for this hop is detected */
		   SelHashHopCharPtr = SelHashHopCharPtr->HashCharHop[HopIndex];
	   }
	}
	if (Result)
	{
		if (SelHashHopCharPtr->Record) return Result;
	    /* Macros hash fillout */
		NewRecordPtr = (HTML_MACROS_HASH_RECORD*)AllocateMemory(sizeof(HTML_MACROS_HASH_RECORD));
		if (!NewRecordPtr)
		{		
		    Result = false;
		}
		else
		{
		    SelHashHopCharPtr->Record = NewRecordPtr;
			NewRecordPtr->HashCharHopPtr = SelHashHopCharPtr;
			NewRecordPtr->HtmlMacrosHandlerPtr = HtmlMacrosHandlerPtr;
			HtmlMacroRecInHashCnt++;
        }
	}
	return Result;
}
//---------------------------------------------------------------------------
HTML_MACROS_HASH_RECORD* FindHtmlMacrosHash(HTML_MACROS_HASH_CHAR_HOP *RootNameHop,
										char *HtmlMacrosName)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex, NameLen;
	HTML_MACROS_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	HTML_MACROS_HASH_RECORD   *SelRecordPtr = NULL;

	NameLen = strlen(HtmlMacrosName);
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	    HopIndex = AsciiHtmlMacrosCharToIndex[(unsigned char)HtmlMacrosName[CharIndex]];
		if (!HopIndex)
		{
			if (!SelHashHopCharPtr) Result = false;
			break;
		}
	    if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	    {
	        Result = false;
            break;
	    }
	    else
	    {
	        /* Existing char in hash for this hop is detected */
		    SelHashHopCharPtr = SelHashHopCharPtr->HashCharHop[HopIndex];
	    }
	}
	if (!Result || !SelHashHopCharPtr->Record) return NULL;
	return SelHashHopCharPtr->Record;
}
//---------------------------------------------------------------------------
bool RemHtmlMacrosHash(HTML_MACROS_HASH_CHAR_HOP *RootNameHop, char *HtmlMacrosName)
{
    HTML_MACROS_HASH_RECORD *HtmlMacrosRecPtr = NULL;
	HTML_MACROS_HASH_CHAR_HOP *ParentHashHopCharPtr = NULL;
	HTML_MACROS_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	
    HtmlMacrosRecPtr = FindHtmlMacrosHash(RootNameHop, HtmlMacrosName);
	if (!HtmlMacrosRecPtr)
	{
	    /* File no found in hash */
	    return false;
	}
	SelHashHopCharPtr = (HTML_MACROS_HASH_CHAR_HOP*)HtmlMacrosRecPtr->HashCharHopPtr;
	FreeMemory(HtmlMacrosRecPtr);
	SelHashHopCharPtr->Record = NULL;
	HtmlMacroRecInHashCnt--;
	while(!SelHashHopCharPtr || (SelHashHopCharPtr != RootNameHop))
	{
	    if (!SelHashHopCharPtr->UsedCharCount)
	    {
	        ParentHashHopCharPtr = (HTML_MACROS_HASH_CHAR_HOP*)SelHashHopCharPtr->ParentHashCharPtr;
		    ParentHashHopCharPtr->HashCharHop[SelHashHopCharPtr->ParentHashCharIndex] = NULL;
		    FreeMemory(SelHashHopCharPtr);
			HtmlMacroHashEntityCnt--;
		    ParentHashHopCharPtr->UsedCharCount--;
			SelHashHopCharPtr = ParentHashHopCharPtr;
	    }
	    else
	    {
	        break;
	    }
	}
	return true;
}
//---------------------------------------------------------------------------
void CloseHtmlMacrosHash(HTML_MACROS_HASH_CHAR_HOP *RootNameHop)
{
	CloseHtmlMacrosHashHop(RootNameHop);
}
//---------------------------------------------------------------------------
void CloseHtmlMacrosHashHop(HTML_MACROS_HASH_CHAR_HOP *SelHashHopCharPtr)
{
	unsigned int index;
	HTML_MACROS_HASH_CHAR_HOP *NextHashHopCharPtr = NULL;

	if (SelHashHopCharPtr->UsedCharCount)
	{
		for(index=1;index <= MAX_CHAR_HTML_MACROS_HASH_INDEX;index++)
		{
			if (SelHashHopCharPtr->HashCharHop[index])
			{
                NextHashHopCharPtr = SelHashHopCharPtr->HashCharHop[index];
				if (!NextHashHopCharPtr->UsedCharCount)
				{
					if (NextHashHopCharPtr->Record)
                        FreeMemory(NextHashHopCharPtr->Record);
					FreeMemory(NextHashHopCharPtr);
					SelHashHopCharPtr->HashCharHop[index] = NULL;
					SelHashHopCharPtr->UsedCharCount--;
					if (!SelHashHopCharPtr->UsedCharCount) break;
				}
				else
				{
                    CloseHtmlMacrosHashHop(NextHashHopCharPtr);
					if (!NextHashHopCharPtr->UsedCharCount)
					{
					    if (NextHashHopCharPtr->Record)
                            FreeMemory(NextHashHopCharPtr->Record);
					    FreeMemory(NextHashHopCharPtr);
					    SelHashHopCharPtr->HashCharHop[index] = NULL;
					    SelHashHopCharPtr->UsedCharCount--;
					    if (!SelHashHopCharPtr->UsedCharCount) break;
					}
				}
			}
		}
	}
}
//---------------------------------------------------------------------------
void InitHtmlMacrosCmdHash(HTML_MACROS_HASH_CHAR_HOP *NameHop, 
						   CMD_INFO* CmdListPtr, unsigned int NumRecList)
{
	unsigned int i;

    InitHtmlMacrosHash(NameHop);
	for (i=0;i < NumRecList;i++)
	{
        AddHtmlMacrosHash(NameHop, (char*)CmdListPtr[i].HtmlCmdName,
			(void*)&CmdListPtr[i]);
	}
}
//---------------------------------------------------------------------------
void InitHtmlBodyCmdHash(CMD_INFO* CmdListPtr, unsigned int NumRecList)
{
    InitHtmlMacrosCmdHash(&HtmlBodyMacrosHashHop, CmdListPtr, NumRecList);
}
//---------------------------------------------------------------------------
void InitHtmlLineCmdHash(CMD_INFO* CmdListPtr, unsigned int NumRecList)
{
    InitHtmlMacrosCmdHash(&HtmlLineMacrosHashHop, CmdListPtr, NumRecList);
}
//---------------------------------------------------------------------------
void CloseHtmlBodyCmdHash()
{
    CloseHtmlMacrosHash(&HtmlBodyMacrosHashHop);
}
//---------------------------------------------------------------------------
void CloseHtmlLineCmdHash()
{
    CloseHtmlMacrosHash(&HtmlLineMacrosHashHop);
}
//---------------------------------------------------------------------------
CMD_INFO* FindHtmlMacrosCmdHash(HTML_MACROS_HASH_CHAR_HOP *NameHop, 
								char *HtmlMacrosName)
{
	HTML_MACROS_HASH_RECORD *FindRecPtr = NULL;

    FindRecPtr = FindHtmlMacrosHash(NameHop, HtmlMacrosName);
	if (FindRecPtr) return (CMD_INFO*)FindRecPtr->HtmlMacrosHandlerPtr;
	else            return NULL;
}
//---------------------------------------------------------------------------
CMD_INFO* FindHtmlBodyCmdHash(char *HtmlMacrosName)
{
	return FindHtmlMacrosCmdHash(&HtmlBodyMacrosHashHop, HtmlMacrosName);
}
//---------------------------------------------------------------------------
CMD_INFO* FindHtmlLineCmdHash(char *HtmlMacrosName)
{
	return FindHtmlMacrosCmdHash(&HtmlLineMacrosHashHop, HtmlMacrosName);
}
//---------------------------------------------------------------------------
