# if ! defined( GroupDataBaseH )
#	define GroupDataBaseH	/* only include me once */

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

#define NO_GROUP_ACCESS_MASK       0x00
#define MAX_LEN_GROUP_BASE_LINE    256
#define MAX_LEN_GROUP_BASE_NAME    32
#define MIN_USER_GRP_ID            1
#define MAX_USER_GRP_ID            63

typedef struct {
    unsigned char  GroupId;
    ObjListTask    *ObjPtr;
    unsigned char  GroupName[MAX_LEN_GROUP_BASE_NAME+1];
} GROUP_INFO_TYPE;

void GroupDBLoad();
void GroupDbSave();
void GroupDBClear();
GROUP_INFO_TYPE* GroupDbAddItem();
void GroupDbRemItem(GROUP_INFO_TYPE *RemGroupPtr);
GROUP_INFO_TYPE* GetGroupByGroupId(unsigned char GroupId);
unsigned long long int GetAllGroupAccessMask();

//---------------------------------------------------------------------------
#endif  /* if ! defined( GroupDataBaseH ) */
