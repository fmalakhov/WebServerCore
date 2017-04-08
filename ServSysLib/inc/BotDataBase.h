# if ! defined( BotDataBaseH )
#	define BotDataBaseH	/* only include me once */

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

#define BOT_NONE           0
#define BOT_GOOGLE         1
#define BOT_YANDEX         2
#define BOT_MAILRU         3
#define BOT_OPENSTAT       4
#define BOT_WEBMONITOR     5
#define BOT_RAMBLER        6
#define BOT_MSN            7
#define BOT_YAHOO          8
#define BOT_MJ12           9
#define BOT_BING           10
#define BOT_YANDEX_METRICA 11
#define BOT_YANDEX_FAVICON 12
#define BOT_YANDEX_DIRECT  13
#define BOT_STATDOM        14
#define BOT_ITRACKRU       15
#define BOT_LINKPAD        16
#define BOT_YANDEX_IMAGES  17
#define BOT_DOTBOT         18
#define BOT_PRCY           19
#define MIN_BOT_TYPE_ID    BOT_NONE
#define MAX_BOT_TYPE_ID    BOT_PRCY

#define MAX_LEN_BOT_KEY 512
#define MAX_LEN_BOT_NAME 128
#define MAX_LEN_BTDT_BASE_LINE 512

typedef struct {
    unsigned int  BotId;
    unsigned int  BotIndex;
    unsigned char BotName[MAX_LEN_BOT_NAME+1];
    unsigned char BotKey[MAX_LEN_BOT_KEY+1];
} BOT_INFO_TYPE;

void CmdAddNewBotDb(unsigned char *CmdLinePtr);
void BotInfoDBLoad();
void BotDBClear();
unsigned char FindBotInfoStrLine(unsigned char *StrPtr, unsigned int StrLen);

//---------------------------------------------------------------------------
#endif  /* if ! defined( BotDataBaseH ) */
