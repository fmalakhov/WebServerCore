# if ! defined( UserInfoDataBaseH )
#	define UserInfoDataBaseH	/* only include me once */

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

#define USER_DB_FILE_IDENTITY_1 0xCA
#define USER_DB_FILE_IDENTITY_2 0xEE

typedef void (*TOnCreateExtDbRecCB)(void *ExtUserInfoPtr);
typedef unsigned int (*TOnLoadVerExtDbRecCB)(unsigned char LoadVersion);
typedef unsigned char* (*TOnReadExtDbRecCB)(void *ExtUserInfoPtr, unsigned char LoadVersion, unsigned char *DataPtr);
typedef unsigned char* (*TOnSaveExtDbRecCB)(void *ExtUserInfoPtr, unsigned char *DataPtr);

typedef struct {
	unsigned int        AdmUserType;
	unsigned char       DbVersion;
	unsigned int        UserRecordSize;
	unsigned int        UserRecordPackSize;
	TOnCreateExtDbRecCB OnCreateExtDbRecCB;
	TOnLoadVerExtDbRecCB OnLoadVerExtDbRecCB;
	TOnReadExtDbRecCB   OnReadExtDbRecCB;
	TOnSaveExtDbRecCB   OnSaveExtDbRecCB;
} USER_DB_INFO;

typedef struct {
	unsigned int UserId;
	unsigned int UserType;
    unsigned long long int GroupMask;
	void *ExtUserInfoPtr;
	char Name[MAX_LEN_USER_INFO_NAME+1];
	char UserName[MAX_LEN_USER_INFO_USER_NAME+1];
	char Email[MAX_LEN_USER_INFO_EMAIL+1];
	char Passwd[MAX_LEN_USER_INFO_PASSWD+1];
    CONTACT_INFO Contact;
#ifdef WIN32         
	SYSTEMTIME   RegisterTime;
	SYSTEMTIME   LastVisitTime;
#else
	struct tm    RegisterTime;
    struct tm    LastVisitTime;
#endif        
	ListItsTask MessageList;
} USER_INFO;

#define USER_INFO_BASE_REC_SIZE_V1 (2*UINT_PACK_SIZE + (MAX_LEN_USER_INFO_NAME+1)+\
        (MAX_LEN_USER_INFO_USER_NAME+1) + (MAX_LEN_USER_INFO_EMAIL+1) +\
        (MAX_LEN_USER_INFO_PASSWD+1) + CONTACT_INFO_PACK_SIZE + 2*DATE_PACK_SIZE +\
		2*UINT_PACK_SIZE)

void UserInfoDBLoad(USER_DB_INFO *UserDbInfoPtr);
void UserInfoDBSave(USER_DB_INFO *UserDbInfoPtr);
void UserInfoDBClose();
USER_INFO* CheckLoginUserInfoDb(char *login, char *passwd);
USER_INFO* GetUserInfoById(unsigned int userId);
USER_INFO* GetUserInfoDbByNameEmail(char *name, char *email);
void AddPrimaryUserGroupMask(USER_DB_INFO *UserDbInfoPtr, unsigned long long int SetMask);
void RemUserGroupMask(USER_DB_INFO *UserDbInfoPtr, unsigned long long int ClearMask);
USER_INFO* NewUserInfoCreate();
void NewUserInfoAddDb(USER_DB_INFO *UserDbInfoPtr, USER_INFO *NewUserPtr);

//---------------------------------------------------------------------------
#endif  /* if ! defined( UserInfoDataBaseH ) */
