# if ! defined( MpttParserHashH )
#	define MpttParserHashH	/* only include me once */

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

#ifndef SysLibToolH
#include "SysLibTool.h"
#endif

#define MAX_CHAR_MPTT_PARSER_HASH_INDEX    256

typedef char* (*TOnFieldHandlerCB)(void *DataPtr, char *MpttParserPtr);

typedef struct {
    char              *FieldMarker;
	TOnFieldHandlerCB FieldHandler;
} MPTT_PARSER_DATA;

typedef struct {
	MPTT_PARSER_DATA *FieldParser;
	unsigned int     Offset;
	void             *HashCharHopPtr;
} MPTT_PARSER_HASH_RECORD;

typedef struct {
   unsigned int     UsedCharCount;
   unsigned int     ParentHashCharIndex;
   void             *ParentHashCharPtr;
   MPTT_PARSER_HASH_RECORD *Record;
   void             *HashCharHop[MAX_CHAR_MPTT_PARSER_HASH_INDEX+1];
} MPTT_PARSER_HASH_CHAR_HOP;

typedef struct {
	unsigned int     MaxMpttParserIndex;
	unsigned int     ListSize;
   	MPTT_PARSER_DATA *MpttParserList;
	MPTT_PARSER_HASH_CHAR_HOP RootNameHop;
    unsigned char    AsciMpttParserCharToIndex[MAX_CHAR_MPTT_PARSER_HASH_INDEX + 1];	
} MPTT_PARSER_MESSAGE;

void CreateMpttParserHash(MPTT_PARSER_MESSAGE *MpttParserMessage);
void CloseMpttParserHash(MPTT_PARSER_MESSAGE *MpttParserMessage);
bool MpttParserMsgExtract(MPTT_PARSER_MESSAGE *MpttParserMessage, void *DataPtr, char *MpttParserPtr);
bool MpttParserLineMsgExtract(MPTT_PARSER_MESSAGE *MpttParserMessage, void *DataPtr, char *MpttParserPtr);

//---------------------------------------------------------------------------
#endif  /* if ! defined( MpttParserHashH ) */
