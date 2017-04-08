# if ! defined( BackRequestDataBaseH )
#	define BackRequestDataBaseH	/* only include me once */

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

#ifndef SysWebFunctionH
#include "SysWebFunction.h"
#endif

#ifndef WebServInfoH
#include "WebServInfo.h"
#endif

#define MAX_LEN_USER_BACK_MSG        2048
#define MAX_REC_BACK_USR_INF_LIST    1000

typedef struct {
	unsigned int  BackReqId;
#ifdef WIN32
	SYSTEMTIME    SubmitDate;
#else        
	struct tm     SubmitDate;
#endif 
    unsigned char BackOperId;
	bool          IsCompleted;
	char UserName[MAX_LEN_USER_INFO_NAME+1];
	char UserMail[MAX_LEN_USER_INFO_EMAIL+1];
    char UserPhone[MAX_LEN_PHONE_NUM+1];
	char UserMessage[MAX_LEN_USER_BACK_MSG+1];
	char ShopRespMsg[MAX_LEN_USER_BACK_MSG+1];
} BACK_USER_INFO_REQ;

void BackReqDBLoad();
void BackReqDbSave();
void BackReqDBClear();
BACK_USER_INFO_REQ* BackReqDbAddBackReq();

//---------------------------------------------------------------------------
#endif  /* if ! defined( BackRequestDataBaseH ) */
