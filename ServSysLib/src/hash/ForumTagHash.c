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
#include "ForumTagHash.h"
#include "SysWebFunction.h"
#include "WebServInfo.h"

FORUM_TAG_HASH_CHAR_HOP ForumTagHashHop;
extern unsigned int SmileListSize;
extern unsigned int OperationListSize;
extern SMILE_INFO SmileList[];
extern OPERATION_INFO OperationList[];
extern char *EndHtmlPageGenPtr;
extern char AnsiToHtmlRusConver[];

unsigned char AsciiForumTagCharToIndex[] ={
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,  37,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,  46,  47,   0,   0,   0,   0,   0,   0,
   1,   2,   3,   4,   5,   6,   7,   8,   9,  10,  38,   0,   0,  39,   0,  44,
   0,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,
  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  40,   0,  41,   0,   0,
   0,  11,  12,  13,  14,  15,  16,  17,  18,  19,  20,  21,  22,  23,  24,  25,
  26,  27,  28,  29,  30,  31,  32,  33,  34,  35,  36,  42,  45,  43,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,
   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0 };

char NewLineHtmlTag[] = "<br>";
char AmpHtmlTag[] = "&amp;";
char LtHtmlTag[] = "&lt;";
char GtHtmlTag[] = "&gt;";
char QuotHtmlTag[] = "&quot;";
char NewLineEditorTag[] = "\\r\\n";
char DoubleQuoteEditorTag[] = "\"";
//---------------------------------------------------------------------------
void InitForumTagHash(FORUM_TAG_HASH_CHAR_HOP *NameHop)
{
	memset(NameHop, 0, sizeof(FORUM_TAG_HASH_CHAR_HOP));
}
//---------------------------------------------------------------------------
void LoadForumTagHash()
{
	unsigned int i;

	InitForumTagHash(&ForumTagHashHop);
	for (i=0;i < SmileListSize;i++)
	{
        AddForumTagHash(&ForumTagHashHop, (char*)SmileList[i].SmileCodePtr, 
			(void*)&SmileList[i], SMILE_FUNCT_TAG);
	}
	for (i=0;i < OperationListSize;i++)
	{
        AddForumTagHash(&ForumTagHashHop, (char*)OperationList[i].OperationStartMarkerPtr, 
			(void*)&OperationList[i], OPERATION_FUNCT_TAG);
	}
}
//---------------------------------------------------------------------------
bool AddForumTagHash(FORUM_TAG_HASH_CHAR_HOP *RootNameHop, char *ForumTag, 
					 void *TagRecordPtr, unsigned char TagType)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex, NameLen;
	char *NameOnlyPtr = NULL;
	FORUM_TAG_HASH_CHAR_HOP *NewHashHopCharPtr = NULL;
	FORUM_TAG_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	FORUM_TAG_HASH_RECORD   *NewRecordPtr = NULL;

	NameLen = strlen(ForumTag);
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	   HopIndex = AsciiForumTagCharToIndex[(unsigned char)ForumTag[CharIndex]];
	   if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	   {
	       /* New char in hash for this hop is detected */
		   NewHashHopCharPtr = (FORUM_TAG_HASH_CHAR_HOP*)AllocateMemory(sizeof(FORUM_TAG_HASH_CHAR_HOP));
		   if (!NewHashHopCharPtr)
		   {
		       Result = false;
		       break;
		   }
		   InitForumTagHash(NewHashHopCharPtr);
		   SelHashHopCharPtr->HashCharHop[HopIndex] = NewHashHopCharPtr;
		   SelHashHopCharPtr->UsedCharCount++;
		   NewHashHopCharPtr->ParentHashCharPtr = (void*)SelHashHopCharPtr;
		   NewHashHopCharPtr->ParentHashCharIndex = HopIndex;
		   SelHashHopCharPtr = NewHashHopCharPtr;
	   }
	   else
	   {
	       /* Existing char in hash for this hop is detected */
		   SelHashHopCharPtr = (FORUM_TAG_HASH_CHAR_HOP*)SelHashHopCharPtr->HashCharHop[HopIndex];
	   }
	}
	if (Result)
	{
		if (SelHashHopCharPtr->Record)
		{
			printf("In cashe of forum tags, tag (%s) already present\n", ForumTag);
            return false;
		}
	    /* Forum tag hash fillout */
		NewRecordPtr = (FORUM_TAG_HASH_RECORD*)AllocateMemory(sizeof(FORUM_TAG_HASH_RECORD));
		if (!NewRecordPtr)
		{		
		    Result = false;
		}
		else
		{
		    SelHashHopCharPtr->Record = NewRecordPtr;
			NewRecordPtr->HashCharHopPtr = SelHashHopCharPtr;
			NewRecordPtr->TagRecordPtr = TagRecordPtr;
			NewRecordPtr->TagLen = NameLen;
			NewRecordPtr->TagType = TagType;
        }
	}
	return Result;
}
//---------------------------------------------------------------------------
FORUM_TAG_HASH_RECORD* FindForumTagHash(FORUM_TAG_HASH_CHAR_HOP *RootNameHop, char *SearchLine)
{
    bool Result = true;
    unsigned int CharIndex, HopIndex, NameLen;
	FORUM_TAG_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	FORUM_TAG_HASH_RECORD   *SelRecordPtr = NULL;

	NameLen = strlen(SearchLine);
	SelHashHopCharPtr = RootNameHop;
    for (CharIndex=0;CharIndex < NameLen;CharIndex++)
	{
	    HopIndex = AsciiForumTagCharToIndex[(unsigned char)SearchLine[CharIndex]];
	    if (!SelHashHopCharPtr->HashCharHop[HopIndex])
	    {
	        if (!SelHashHopCharPtr) Result = false;
            break;
	    }
	    else
	    {
	        /* Existing char in hash for this hop is detected */
		    SelHashHopCharPtr = (FORUM_TAG_HASH_CHAR_HOP*)SelHashHopCharPtr->HashCharHop[HopIndex];
	    }
	}
	if (!Result || !SelHashHopCharPtr->Record) return NULL;
	return SelHashHopCharPtr->Record;
}
//---------------------------------------------------------------------------
bool RemForumTagHash(FORUM_TAG_HASH_CHAR_HOP *RootNameHop, char *ForumTag)
{
    FORUM_TAG_HASH_RECORD *ForumTagRecPtr = NULL;
	FORUM_TAG_HASH_CHAR_HOP *ParentHashHopCharPtr = NULL;
	FORUM_TAG_HASH_CHAR_HOP *SelHashHopCharPtr = NULL;
	
    ForumTagRecPtr = FindForumTagHash(RootNameHop, ForumTag);
	if (!ForumTagRecPtr)
	{
	    /* File no found in hash */
	    return false;
	}
	SelHashHopCharPtr = (FORUM_TAG_HASH_CHAR_HOP*)ForumTagRecPtr->HashCharHopPtr;
	FreeMemory(ForumTagRecPtr);
	SelHashHopCharPtr->Record = NULL;
	while(!SelHashHopCharPtr || (SelHashHopCharPtr != RootNameHop))
	{
	    if (!SelHashHopCharPtr->UsedCharCount)
	    {
	        ParentHashHopCharPtr = (FORUM_TAG_HASH_CHAR_HOP*)SelHashHopCharPtr->ParentHashCharPtr;
		    ParentHashHopCharPtr->HashCharHop[SelHashHopCharPtr->ParentHashCharIndex] = NULL;
		    FreeMemory(SelHashHopCharPtr);
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
void CloseForumTagHash()
{
	CloseForumTagHashHop(&ForumTagHashHop);
}
//---------------------------------------------------------------------------
void CloseForumTagHashHop(FORUM_TAG_HASH_CHAR_HOP *SelHashHopCharPtr)
{
	unsigned int index;
	FORUM_TAG_HASH_CHAR_HOP *NextHashHopCharPtr = NULL;

	if (SelHashHopCharPtr->UsedCharCount)
	{
		for(index=1;index <= MAX_CHAR_FORUM_TAG_HASH_INDEX;index++)
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
                    CloseForumTagHashHop(NextHashHopCharPtr);
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
void ConvertTextForumView(char *UserMsgPtr, unsigned int MsgLen, bool isTextOnly)
{
	unsigned int  i;
	unsigned int  CnvLen;
	unsigned int  CharId;
	unsigned char SrcSymb;

	if (isTextOnly)
	{
	    for (i=0;i < MsgLen;i++)
		{
	        SrcSymb = (unsigned char)*UserMsgPtr;
		    switch(SrcSymb)
			{
		        case '\r':
				    break;

		        case '\n':
				    CnvLen = sizeof(NewLineEditorTag)/sizeof(char)-1;
				    memcpy(EndHtmlPageGenPtr, NewLineEditorTag, CnvLen);
                    EndHtmlPageGenPtr += CnvLen;
				    break;

		        case '"':
				    CnvLen = sizeof(DoubleQuoteEditorTag)/sizeof(char)-1;
				    memcpy(EndHtmlPageGenPtr, DoubleQuoteEditorTag, CnvLen);
                    EndHtmlPageGenPtr += CnvLen;
				    break;

			    default:
				    *EndHtmlPageGenPtr++ = SrcSymb;
				    break;
			}
            UserMsgPtr++;
		}
	    return;
	}
	for (i=0;i < MsgLen;i++)
	{
	    SrcSymb = (unsigned char)*UserMsgPtr;
		if (SrcSymb >= (unsigned char)START_RUS_CHAR)
		{
	        CharId = ((unsigned int)(SrcSymb) - START_RUS_CHAR) << 3;
			memcpy(EndHtmlPageGenPtr, &AnsiToHtmlRusConver[CharId], 7);
			EndHtmlPageGenPtr += 7;
		}
		else
		{
		    switch(SrcSymb)
			{
		        case '\r':
				    break;

			    case '\n':
				    CnvLen = sizeof(NewLineHtmlTag)/sizeof(char)-1;
				    memcpy(EndHtmlPageGenPtr, NewLineHtmlTag, CnvLen);
                    EndHtmlPageGenPtr += CnvLen;
				    break;

			    case '&':
				    CnvLen = sizeof(AmpHtmlTag)/sizeof(char)-1;
				    memcpy(EndHtmlPageGenPtr, AmpHtmlTag, CnvLen);
                    EndHtmlPageGenPtr += CnvLen;
				    break;

			    case '<':
				    CnvLen = sizeof(LtHtmlTag)/sizeof(char)-1;
				    memcpy(EndHtmlPageGenPtr, LtHtmlTag, CnvLen);
                    EndHtmlPageGenPtr += CnvLen;
				    break;

			    case '>':
				    CnvLen = sizeof(GtHtmlTag)/sizeof(char)-1;
				    memcpy(EndHtmlPageGenPtr, GtHtmlTag, CnvLen);
                    EndHtmlPageGenPtr += CnvLen;
				    break;

				case '"':
				    CnvLen = sizeof(QuotHtmlTag)/sizeof(char)-1;
				    memcpy(EndHtmlPageGenPtr, QuotHtmlTag, CnvLen);
                    EndHtmlPageGenPtr += CnvLen;
					break;

			    default:
				    *EndHtmlPageGenPtr++ = SrcSymb;
				    break;
			}
		}
		UserMsgPtr++;
	}
	*EndHtmlPageGenPtr = 0;
}
//---------------------------------------------------------------------------
void UserMessageHtmlConvert(char *UserMsgPtr, bool isTextOnly)
{
    char *MsgConvPtr = NULL;
	char *StartPtr = NULL;
	SMILE_INFO *SmileInfoPtr = NULL;
	OPERATION_INFO *OperInfoPtr = NULL;
	FORUM_TAG_HASH_RECORD *ForumTagPtr = NULL;
	unsigned int Len;
	unsigned int FindPosition;

	if (!UserMsgPtr) return;
	MsgConvPtr = UserMsgPtr;
	if (!*MsgConvPtr) return;
	StartPtr = MsgConvPtr;
	FindPosition = 0;
	for(;;)
	{
        ForumTagPtr = FindForumTagHash(&ForumTagHashHop, MsgConvPtr);
	    if (ForumTagPtr)
		{
		    ConvertTextForumView(StartPtr, FindPosition, isTextOnly);
		    if (ForumTagPtr->TagType == SMILE_FUNCT_TAG)
			{
			    SmileInfoPtr = (SMILE_INFO*)ForumTagPtr->TagRecordPtr;
			    if (SmileInfoPtr->TagFunction)
				{
                    ForumTagPtr->isTextOnly = isTextOnly;
					(SmileInfoPtr->TagFunction)(ForumTagPtr);
				}
			    MsgConvPtr += ForumTagPtr->TagLen;
				StartPtr = MsgConvPtr;
				FindPosition = 0;
			}
		    else
			{
			    OperInfoPtr = (OPERATION_INFO*)ForumTagPtr->TagRecordPtr;
				MsgConvPtr += ForumTagPtr->TagLen;
		        if (OperInfoPtr->TagFunction)
				{
					ForumTagPtr->isTextOnly = isTextOnly;
					ForumTagPtr->PostTagPtr = MsgConvPtr;
					(OperInfoPtr->TagFunction)(ForumTagPtr);
					MsgConvPtr = ForumTagPtr->PostTagPtr;
				}
				StartPtr = MsgConvPtr;
				FindPosition = 0;
			}
		}
	    else
		{
            MsgConvPtr++;
			FindPosition++;
		}
        if (!*MsgConvPtr) break;
	}
	Len = strlen(StartPtr);
	if (Len) ConvertTextForumView(StartPtr, Len, isTextOnly);
}
//---------------------------------------------------------------------------
