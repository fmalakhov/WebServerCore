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

#include "UserInfoDataBase.h"
#include "GroupDataBase.h"

ListItsTask  UserInfoList;

extern char UserDbNamePath[];
extern unsigned char UserInfoEncodeKey[];
extern unsigned int UserInfoEncodeKeyLen;

static char AdminName[] = "admin";
static char AdminPassword[] = "password";
static unsigned int NextFreeUserId = 1;

bool isPrimaryGroupAccessUser(USER_INFO *UserPtr);
static unsigned char* ContactInfoPack(unsigned char* BufPtr, CONTACT_INFO* ContactPtr);
static unsigned char* ContactInfoUnpack(unsigned char* BufPtr, CONTACT_INFO* ContactPtr);
//---------------------------------------------------------------------------
void UserInfoDBLoad(USER_DB_INFO *UserDbInfoPtr)
{
    unsigned int  ReadVal, ExtDbLoadLen;
	FILE          *HandleBD;
	USER_INFO     *NewUserPtr;
	char          *CwdRet = NULL;
	int	          ReadedLine;
	unsigned char   ReadBuf[2048];
	unsigned char   *DecompPtr = NULL;
#ifdef _LINUX_X86_        
    struct timeb  hires_cur_time;
    struct tm     *CurrTime;
#endif
	unsigned char BdIdVer[8];
	char          PathFile[1024];
    
	UserInfoList.Count = 0;
	UserInfoList.CurrTask = NULL;
	UserInfoList.FistTask = NULL;
#ifdef WIN32
	CwdRet = _getcwd(PathFile, 512);
#else
	CwdRet = getcwd(PathFile, 512);
#endif
	strcat(PathFile, UserDbNamePath);
    memset(&BdIdVer[0], 0, 6);
	if ((HandleBD = fopen(PathFile,"rb" )) != NULL)
	{
        ReadedLine = fread(&BdIdVer[0], 6, 1, HandleBD);
		if ((ReadedLine) && (BdIdVer[0] == USER_DB_FILE_IDENTITY_1) && 
			(BdIdVer[1] == USER_DB_FILE_IDENTITY_2))
		{
		    NewUserPtr = (USER_INFO*)AllocateMemory(sizeof(USER_INFO));
			if (!NewUserPtr)
			{
			    fclose(HandleBD);
				return;
			}
			memset(NewUserPtr, 0, sizeof(USER_INFO));

			if (UserDbInfoPtr->UserRecordSize > 0)
			{
				NewUserPtr->ExtUserInfoPtr = (void*)AllocateMemory(UserDbInfoPtr->UserRecordSize);
				if (!NewUserPtr->ExtUserInfoPtr)
				{
					fclose(HandleBD);
					FreeMemory(NewUserPtr);
					return;
				}
				memset(NewUserPtr->ExtUserInfoPtr, 0, UserDbInfoPtr->UserRecordSize);
			}

			if (UserDbInfoPtr->OnLoadVerExtDbRecCB)
			{
				ExtDbLoadLen = (UserDbInfoPtr->OnLoadVerExtDbRecCB)(BdIdVer[5]);
			}
			else
			{
				ExtDbLoadLen = 0;
			}

			switch(BdIdVer[5])
			{   
			    case 1: /* IMS WEB INFO Latest */
					ReadedLine = fread(&ReadBuf[0], (USER_INFO_BASE_REC_SIZE_V1 - 8), 1, HandleBD);
					break;

				case 2: /* PCLS WEB */
                    ReadedLine = fread(&ReadBuf[0], USER_INFO_BASE_REC_SIZE_V1, 1, HandleBD);
                    break; 

			    case 5: /* IMS WEB Latest */
					ReadedLine = fread(&ReadBuf[0], (USER_INFO_BASE_REC_SIZE_V1 - 1), 1, HandleBD);
					break;

                default:
                    ReadedLine = fread(&ReadBuf[0], (USER_INFO_BASE_REC_SIZE_V1 + ExtDbLoadLen), 1, HandleBD);
                    break;                    
			}
	        while (1)
			{
			    if (!ReadedLine)
				{
				    FreeMemory(NewUserPtr);
				    break;
				}
                
			    switch(BdIdVer[5])
			    { 
			        case 1: /* IMS WEB INFO Latest */
                        BufferSecure((unsigned char*)UserInfoEncodeKey, UserInfoEncodeKeyLen,
							(char*)ReadBuf, (char*)ReadBuf, (USER_INFO_BASE_REC_SIZE_V1 - 8));
					    break;

                    case 2: /* PCLS WEB */
                        BufferSecure((unsigned char*)UserInfoEncodeKey, UserInfoEncodeKeyLen,
							(char*)ReadBuf, (char*)ReadBuf, USER_INFO_BASE_REC_SIZE_V1);
                        break; 

			        case 5: /* IMS WEB Latest */
                        BufferSecure((unsigned char*)UserInfoEncodeKey, UserInfoEncodeKeyLen,
							(char*)ReadBuf, (char*)ReadBuf, (USER_INFO_BASE_REC_SIZE_V1 - 1));
					    break;

                    default:
                        BufferSecure((unsigned char*)UserInfoEncodeKey, UserInfoEncodeKeyLen,
							(char*)ReadBuf, (char*)ReadBuf, (USER_INFO_BASE_REC_SIZE_V1 + ExtDbLoadLen));
                        break;                    
			    }

			    DecompPtr = &ReadBuf[0];
                DecompPtr = Uint32Unpack(DecompPtr, &NewUserPtr->UserId);
			    DecompPtr = Uint32Unpack(DecompPtr, &NewUserPtr->UserType);    
                                                            
                if (BdIdVer[5] < 2)
                {                
                    if (isPrimaryGroupAccessUser(NewUserPtr))
                    {
                        NewUserPtr->GroupMask = GetAllGroupAccessMask();
                    }
                    else
                    {
                        NewUserPtr->GroupMask = NO_GROUP_ACCESS_MASK;
                    }
                }
                else if (BdIdVer[5] == 5) /* Read from IMS WEB serv */
				{
                    DecompPtr = Uint32Unpack(DecompPtr, &ReadVal);
                    NewUserPtr->GroupMask = (unsigned long long int)ReadVal;
				}
				else
                {
                    DecompPtr = Uint32Unpack(DecompPtr, &ReadVal); 
                    NewUserPtr->GroupMask = ((unsigned long long int)ReadVal << 32);               
                    DecompPtr = Uint32Unpack(DecompPtr, &ReadVal);
                    NewUserPtr->GroupMask |= (unsigned long long int)ReadVal;
                }

	            memcpy(&NewUserPtr->Name[0], DecompPtr, (MAX_LEN_USER_INFO_NAME+1));
	            DecompPtr += (MAX_LEN_USER_INFO_NAME+1);
	            memcpy(&NewUserPtr->UserName[0], DecompPtr, (MAX_LEN_USER_INFO_USER_NAME+1));
	            DecompPtr += (MAX_LEN_USER_INFO_USER_NAME+1);
	            memcpy(&NewUserPtr->Email[0], DecompPtr, (MAX_LEN_USER_INFO_EMAIL+1));
	            DecompPtr += (MAX_LEN_USER_INFO_EMAIL+1);
	            memcpy(&NewUserPtr->Passwd[0], DecompPtr, (MAX_LEN_USER_INFO_PASSWD+1));
	            DecompPtr += (MAX_LEN_USER_INFO_PASSWD+1);
                DecompPtr = ContactInfoUnpack(DecompPtr, &NewUserPtr->Contact);
				DecompPtr = DateUnpack(DecompPtr, &NewUserPtr->RegisterTime);
				DecompPtr = DateUnpack(DecompPtr, &NewUserPtr->LastVisitTime);

                if (UserDbInfoPtr->OnReadExtDbRecCB)
					DecompPtr = (UserDbInfoPtr->OnReadExtDbRecCB)(NewUserPtr->ExtUserInfoPtr, BdIdVer[5], DecompPtr);

			    if (NextFreeUserId <= NewUserPtr->UserId) NextFreeUserId = NewUserPtr->UserId + 1;
		        NewUserPtr->MessageList.Count = 0;
		        NewUserPtr->MessageList.CurrTask = NULL;
	         	NewUserPtr->MessageList.FistTask = NULL;                
		        AddStructList(&UserInfoList, NewUserPtr);

				NewUserPtr = (USER_INFO*)AllocateMemory(sizeof(USER_INFO));
				if (!NewUserPtr) break;
				memset(NewUserPtr, 0, sizeof(USER_INFO));
				if (UserDbInfoPtr->UserRecordSize > 0)
				{
					NewUserPtr->ExtUserInfoPtr = (void*)AllocateMemory(UserDbInfoPtr->UserRecordSize);
					if (!NewUserPtr->ExtUserInfoPtr)
					{
						FreeMemory(NewUserPtr);
						break;
					}
					memset(NewUserPtr->ExtUserInfoPtr, 0, UserDbInfoPtr->UserRecordSize);
				}

			    switch(BdIdVer[5])
			    {
			        case 1: /* IMS WEB INFO Latest */
					    ReadedLine = fread(&ReadBuf[0], (USER_INFO_BASE_REC_SIZE_V1 - 8), 1, HandleBD);
					    break;

                    case 2: /* PCLS WEB */
                        ReadedLine = fread(&ReadBuf[0], USER_INFO_BASE_REC_SIZE_V1, 1, HandleBD);
                        break;

			        case 5: /* IMS WEB Latest */
					    ReadedLine = fread(&ReadBuf[0], (USER_INFO_BASE_REC_SIZE_V1 - 1), 1, HandleBD);
					    break;

                    default:
						ReadedLine = fread(&ReadBuf[0], (USER_INFO_BASE_REC_SIZE_V1 + ExtDbLoadLen), 1, HandleBD);
                        break;                    
			    }
			}
	        fclose(HandleBD);
			if (BdIdVer[5] != UserDbInfoPtr->DbVersion) UserInfoDBSave(UserDbInfoPtr);
		}
	}
	else
	{
	    NewUserPtr = (USER_INFO*)AllocateMemory(sizeof(USER_INFO));
		memset(NewUserPtr, 0, sizeof(USER_INFO));

		if (UserDbInfoPtr->UserRecordSize > 0)
		{
			NewUserPtr->ExtUserInfoPtr = (void*)AllocateMemory(UserDbInfoPtr->UserRecordSize);
			if (!NewUserPtr->ExtUserInfoPtr)
			{
				FreeMemory(NewUserPtr);
				return;
			}
			memset(NewUserPtr->ExtUserInfoPtr, 0, UserDbInfoPtr->UserRecordSize);
		}

	    strcpy(NewUserPtr->UserName, AdminName);
	    strcpy(NewUserPtr->Name, "Admin");
	    strcpy(NewUserPtr->Email, "mymail@mail.ru");
	    strcpy(NewUserPtr->Passwd, AdminPassword);
        if (UserDbInfoPtr->OnCreateExtDbRecCB)
			(UserDbInfoPtr->OnCreateExtDbRecCB)(NewUserPtr->ExtUserInfoPtr);
	    NewUserPtr->UserId = NextFreeUserId++;
		NewUserPtr->UserType = UserDbInfoPtr->AdmUserType;
        NewUserPtr->GroupMask = GetAllGroupAccessMask();
	    NewUserPtr->Contact.CountryId = 0;
	    NewUserPtr->Contact.ZipCode = 0;
	    NewUserPtr->Contact.UserTitleId = 0;
	    memset(&NewUserPtr->Contact.CompanyName, 0, MAX_LEN_COMPANY_NAME);
	    memset(&NewUserPtr->Contact.FirstName, 0, MAX_LEN_USER_INFO_NAME);
	    memset(&NewUserPtr->Contact.MiddleName, 0, MAX_LEN_USER_INFO_NAME);
	    memset(&NewUserPtr->Contact.LastName, 0, MAX_LEN_USER_INFO_NAME);
	    memset(&NewUserPtr->Contact.Address1, 0, MAX_LEN_ADDR_1_NAME);
	    memset(&NewUserPtr->Contact.Address2, 0, MAX_LEN_ADDR_2_NAME);
	    memset(&NewUserPtr->Contact.City, 0, MAX_LEN_CITY_NAME);
	    memset(&NewUserPtr->Contact.LandPhone, 0, MAX_LEN_PHONE_NUM);
	    memset(&NewUserPtr->Contact.MobilePhone, 0, MAX_LEN_PHONE_NUM);
	    memset(&NewUserPtr->Contact.FaxPhone, 0, MAX_LEN_PHONE_NUM);
        strcpy(NewUserPtr->Contact.Email, NewUserPtr->Email);

#ifdef WIN32
	    GetSystemTime(&NewUserPtr->RegisterTime);
		memcpy(&NewUserPtr->LastVisitTime, &NewUserPtr->RegisterTime, sizeof(SYSTEMTIME));
#else
        ftime(&hires_cur_time);
        CurrTime = localtime(&hires_cur_time.time);
        memcpy(&NewUserPtr->RegisterTime, CurrTime, sizeof(struct tm));
		memcpy(&NewUserPtr->LastVisitTime, CurrTime, sizeof(struct tm));
#endif        

		NewUserPtr->MessageList.Count = 0;
		NewUserPtr->MessageList.CurrTask = NULL;
		NewUserPtr->MessageList.FistTask = NULL;

	    AddStructList(&UserInfoList, NewUserPtr);
		UserInfoDBSave(UserDbInfoPtr);
	}
	return;
}
//---------------------------------------------------------------------------
void UserInfoDBSave(USER_DB_INFO *UserDbInfoPtr)
{
	FILE        *HandleBD;
	USER_INFO   *SelUserPtr = NULL;
    ObjListTask	*SelObjPtr = NULL;
	char        *CwdRet = NULL;
	unsigned char BdIdVer[8];
	char        PathFile[1024];
	unsigned char   *CompPtr = NULL;
    unsigned char   SaveBuf[2048];

	if (UserInfoList.Count < 2) return;
#ifdef WIN32
	CwdRet = _getcwd(PathFile, 512);
#else
	CwdRet = getcwd(PathFile, 512);
#endif
	strcat(PathFile, UserDbNamePath);
	memset(&BdIdVer[0], 0, 6);
    BdIdVer[0] = USER_DB_FILE_IDENTITY_1;
    BdIdVer[1] = USER_DB_FILE_IDENTITY_2;
	BdIdVer[5] = UserDbInfoPtr->DbVersion;
	HandleBD = fopen(PathFile,"wb");
	if ( !HandleBD ) return;
	fwrite(&BdIdVer[0], 6, 1, HandleBD);
	SelObjPtr = (ObjListTask*)GetFistObjectList(&UserInfoList);
	while(SelObjPtr)
	{
	    SelUserPtr = (USER_INFO*)SelObjPtr->UsedTask;
		CompPtr = &SaveBuf[0];
        CompPtr = Uint32Pack(CompPtr, SelUserPtr->UserId);
		CompPtr = Uint32Pack(CompPtr, SelUserPtr->UserType);
        CompPtr = Uint32Pack(CompPtr, (unsigned int)(SelUserPtr->GroupMask >> 32));
        CompPtr = Uint32Pack(CompPtr, (unsigned int)(SelUserPtr->GroupMask & 0xFFFFFFFF));

	    memcpy(CompPtr, &SelUserPtr->Name[0], (MAX_LEN_USER_INFO_NAME+1));
	    CompPtr += (MAX_LEN_USER_INFO_NAME+1);
	    memcpy(CompPtr, &SelUserPtr->UserName[0], (MAX_LEN_USER_INFO_USER_NAME+1));
	    CompPtr += (MAX_LEN_USER_INFO_USER_NAME+1);
	    memcpy(CompPtr, &SelUserPtr->Email[0], (MAX_LEN_USER_INFO_EMAIL+1));
	    CompPtr += (MAX_LEN_USER_INFO_EMAIL+1);
	    memcpy(CompPtr, &SelUserPtr->Passwd[0], (MAX_LEN_USER_INFO_PASSWD+1));
	    CompPtr += (MAX_LEN_USER_INFO_PASSWD+1);
        CompPtr = ContactInfoPack(CompPtr, &SelUserPtr->Contact);
		CompPtr = DatePack(CompPtr, &SelUserPtr->RegisterTime);
		CompPtr = DatePack(CompPtr, &SelUserPtr->LastVisitTime);

        if (UserDbInfoPtr->OnSaveExtDbRecCB)
			CompPtr = (UserDbInfoPtr->OnSaveExtDbRecCB)(SelUserPtr->ExtUserInfoPtr, CompPtr);

		BufferSecure((unsigned char*)UserInfoEncodeKey, UserInfoEncodeKeyLen,
			(char*)SaveBuf, (char*)SaveBuf, (USER_INFO_BASE_REC_SIZE_V1 + UserDbInfoPtr->UserRecordPackSize));
		fwrite(&SaveBuf[0], (USER_INFO_BASE_REC_SIZE_V1 + UserDbInfoPtr->UserRecordPackSize), 1, HandleBD);
		SelObjPtr = (ObjListTask*)GetNextObjectList(&UserInfoList);
	}
	fclose(HandleBD);
	return;
}
//---------------------------------------------------------------------------
USER_INFO* NewUserInfoCreate(USER_DB_INFO *UserDbInfoPtr)
{
#ifdef _LINUX_X86_        
    struct timeb hires_cur_time;
    struct tm    *CurrTime;
#endif
	USER_INFO   *NewUserPtr = NULL;

    NewUserPtr = (USER_INFO*)AllocateMemory(sizeof(USER_INFO));
	if (!NewUserPtr) return NULL;
	memset(NewUserPtr, 0, sizeof(USER_INFO));
	if (UserDbInfoPtr->UserRecordSize > 0)
	{
		NewUserPtr->ExtUserInfoPtr = (void*)AllocateMemory(UserDbInfoPtr->UserRecordSize);
		if (!NewUserPtr->ExtUserInfoPtr)
		{
			FreeMemory(NewUserPtr);
			return NULL;
		}
		memset(NewUserPtr->ExtUserInfoPtr, 0, UserDbInfoPtr->UserRecordSize);
	}

    if (UserDbInfoPtr->OnCreateExtDbRecCB)
		(UserDbInfoPtr->OnCreateExtDbRecCB)(NewUserPtr->ExtUserInfoPtr);

	NewUserPtr->Contact.CountryId = 0;
	NewUserPtr->Contact.ZipCode = 0;
	NewUserPtr->Contact.UserTitleId = 0;
	memset(&NewUserPtr->Contact.CompanyName, 0, MAX_LEN_COMPANY_NAME);
	memset(&NewUserPtr->Contact.FirstName, 0, MAX_LEN_USER_INFO_NAME);
	memset(&NewUserPtr->Contact.MiddleName, 0, MAX_LEN_USER_INFO_NAME);
	memset(&NewUserPtr->Contact.LastName, 0, MAX_LEN_USER_INFO_NAME);
	memset(&NewUserPtr->Contact.Address1, 0, MAX_LEN_ADDR_1_NAME);
	memset(&NewUserPtr->Contact.Address2, 0, MAX_LEN_ADDR_2_NAME);
	memset(&NewUserPtr->Contact.City, 0, MAX_LEN_CITY_NAME);
	memset(&NewUserPtr->Contact.LandPhone, 0, MAX_LEN_PHONE_NUM);
	memset(&NewUserPtr->Contact.MobilePhone, 0, MAX_LEN_PHONE_NUM);
	memset(&NewUserPtr->Contact.FaxPhone, 0, MAX_LEN_PHONE_NUM);
	memset(&NewUserPtr->Contact.Email, 0, MAX_LEN_USER_INFO_EMAIL);

#ifdef WIN32
	GetSystemTime(&NewUserPtr->RegisterTime);
	memcpy(&NewUserPtr->LastVisitTime, &NewUserPtr->RegisterTime, sizeof(SYSTEMTIME));
#else
    ftime(&hires_cur_time);
    CurrTime = localtime(&hires_cur_time.time);
    memcpy(&NewUserPtr->RegisterTime, CurrTime, sizeof(struct tm));
	memcpy(&NewUserPtr->LastVisitTime, CurrTime, sizeof(struct tm));
#endif        
	NewUserPtr->MessageList.Count = 0;
	NewUserPtr->MessageList.CurrTask = NULL;
	NewUserPtr->MessageList.FistTask = NULL;
	return NewUserPtr;
}
//---------------------------------------------------------------------------
void NewUserInfoAddDb(USER_DB_INFO *UserDbInfoPtr, USER_INFO *NewUserPtr)
{
    NewUserPtr->UserId = NextFreeUserId++;
	AddStructList(&UserInfoList, NewUserPtr);
	UserInfoDBSave(UserDbInfoPtr);
}
//---------------------------------------------------------------------------
USER_INFO* CheckLoginUserInfoDb(char *login, char *passwd)
{
	USER_INFO   *SelUserPtr = NULL;
	USER_INFO   *FindUserPtr = NULL;
    ObjListTask	*SelObjPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&UserInfoList);
	while(SelObjPtr)
	{
	    SelUserPtr = (USER_INFO*)SelObjPtr->UsedTask;
		if ((!strcmp(SelUserPtr->UserName,  login)) &&
			(!strcmp(SelUserPtr->Passwd,  passwd)))
		{
			FindUserPtr = SelUserPtr;
			break;
		}
		SelObjPtr = (ObjListTask*)GetNextObjectList(&UserInfoList);
	}
	return FindUserPtr;
}
//---------------------------------------------------------------------------
void UserInfoDBClose()
{
	USER_INFO   *SelUserPtr = NULL;
    ObjListTask	*SelObjPtr = NULL;
	ObjListTask	*SelMessageObjPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&UserInfoList);
	while(SelObjPtr)
	{
	    SelUserPtr = (USER_INFO*)SelObjPtr->UsedTask;
		SelMessageObjPtr = (ObjListTask*)GetFistObjectList(&SelUserPtr->MessageList);
		while(SelMessageObjPtr)
		{
			RemStructList(&SelUserPtr->MessageList, SelMessageObjPtr);
			SelMessageObjPtr = (ObjListTask*)GetFistObjectList(&SelUserPtr->MessageList);
		}
		if (SelUserPtr->ExtUserInfoPtr) FreeMemory(SelUserPtr->ExtUserInfoPtr);
		FreeMemory(SelUserPtr);
		RemStructList(&UserInfoList, SelObjPtr);
		SelObjPtr = (ObjListTask*)GetFistObjectList(&UserInfoList);
	}
	return;
}
//---------------------------------------------------------------------------
USER_INFO* GetUserInfoById(unsigned int userId)
{
	USER_INFO   *SelUserPtr = NULL;
	USER_INFO   *FindUserPtr = NULL;
    ObjListTask	*SelObjPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&UserInfoList);
	while(SelObjPtr)
	{
	    SelUserPtr = (USER_INFO*)SelObjPtr->UsedTask;
		if (SelUserPtr->UserId == userId)
		{
			FindUserPtr = SelUserPtr;
			break;
		}
		SelObjPtr = (ObjListTask*)GetNextObjectList(&UserInfoList);
	}
	return FindUserPtr;
}
//---------------------------------------------------------------------------
USER_INFO* GetUserInfoDbByNameEmail(char *name, char *email)
{
	USER_INFO   *SelUserPtr = NULL;
	USER_INFO   *FindUserPtr = NULL;
    ObjListTask	*SelObjPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&UserInfoList);
	while(SelObjPtr)
	{
	    SelUserPtr = (USER_INFO*)SelObjPtr->UsedTask;
		if ((!strcmp(SelUserPtr->Contact.FirstName,  name)) &&
			(!strcmp(SelUserPtr->Contact.Email,  email)))
		{
			FindUserPtr = SelUserPtr;
			break;
		}
		SelObjPtr = (ObjListTask*)GetNextObjectList(&UserInfoList);
	}
	return FindUserPtr;
}
//---------------------------------------------------------------------------
static unsigned char* ContactInfoPack(unsigned char* BufPtr, CONTACT_INFO* ContactPtr)
{
    BufPtr = Uint32Pack(BufPtr, ContactPtr->CountryId);
	BufPtr = Uint32Pack(BufPtr, ContactPtr->ZipCode);
	*BufPtr++ = (unsigned char)ContactPtr->UserTitleId;
	memcpy(BufPtr, &ContactPtr->CompanyName[0], (MAX_LEN_COMPANY_NAME+1));
	BufPtr += (MAX_LEN_COMPANY_NAME+1);
    memcpy(BufPtr, &ContactPtr->FirstName[0], (MAX_LEN_USER_INFO_NAME+1));
	BufPtr += (MAX_LEN_USER_INFO_NAME+1);
    memcpy(BufPtr, &ContactPtr->MiddleName[0], (MAX_LEN_USER_INFO_NAME+1));
	BufPtr += (MAX_LEN_USER_INFO_NAME+1);
    memcpy(BufPtr, &ContactPtr->LastName[0], (MAX_LEN_USER_INFO_NAME+1));
	BufPtr += (MAX_LEN_USER_INFO_NAME+1);
    memcpy(BufPtr, &ContactPtr->Address1[0], (MAX_LEN_ADDR_1_NAME+1));
	BufPtr += (MAX_LEN_ADDR_1_NAME+1);
    memcpy(BufPtr, &ContactPtr->Address2[0], (MAX_LEN_ADDR_2_NAME+1));
	BufPtr += (MAX_LEN_ADDR_2_NAME+1);
    memcpy(BufPtr, &ContactPtr->City[0], (MAX_LEN_CITY_NAME+1));
	BufPtr += (MAX_LEN_CITY_NAME+1);
    memcpy(BufPtr, &ContactPtr->LandPhone[0], (MAX_LEN_PHONE_NUM));
	BufPtr += (MAX_LEN_PHONE_NUM);
    memcpy(BufPtr, &ContactPtr->MobilePhone[0], (MAX_LEN_PHONE_NUM));
	BufPtr += (MAX_LEN_PHONE_NUM);
    memcpy(BufPtr, &ContactPtr->FaxPhone[0], (MAX_LEN_PHONE_NUM));
	BufPtr += (MAX_LEN_PHONE_NUM);
    memcpy(BufPtr, &ContactPtr->Email[0], (MAX_LEN_USER_INFO_EMAIL));
	BufPtr += (MAX_LEN_USER_INFO_EMAIL);
	return BufPtr;
}
//---------------------------------------------------------------------------
static unsigned char* ContactInfoUnpack(unsigned char* BufPtr, CONTACT_INFO* ContactPtr)
{
    BufPtr = Uint32Unpack(BufPtr, &ContactPtr->CountryId);
	BufPtr = Uint32Unpack(BufPtr, &ContactPtr->ZipCode);
	ContactPtr->UserTitleId = *BufPtr++;
	memcpy(&ContactPtr->CompanyName[0], BufPtr, (MAX_LEN_COMPANY_NAME+1));
	BufPtr += (MAX_LEN_COMPANY_NAME+1);
    memcpy(&ContactPtr->FirstName[0], BufPtr, (MAX_LEN_USER_INFO_NAME+1));
	BufPtr += (MAX_LEN_USER_INFO_NAME+1);
    memcpy(&ContactPtr->MiddleName[0], BufPtr, (MAX_LEN_USER_INFO_NAME+1));
	BufPtr += (MAX_LEN_USER_INFO_NAME+1);
    memcpy(&ContactPtr->LastName[0], BufPtr, (MAX_LEN_USER_INFO_NAME+1));
	BufPtr += (MAX_LEN_USER_INFO_NAME+1);
    memcpy(&ContactPtr->Address1[0], BufPtr, (MAX_LEN_ADDR_1_NAME+1));
	BufPtr += (MAX_LEN_ADDR_1_NAME+1);
    memcpy(&ContactPtr->Address2[0], BufPtr, (MAX_LEN_ADDR_2_NAME+1));
	BufPtr += (MAX_LEN_ADDR_2_NAME+1);
    memcpy(&ContactPtr->City[0], BufPtr, (MAX_LEN_CITY_NAME+1));
	BufPtr += (MAX_LEN_CITY_NAME+1);
    memcpy(&ContactPtr->LandPhone[0], BufPtr, (MAX_LEN_PHONE_NUM));
	BufPtr += (MAX_LEN_PHONE_NUM);
    memcpy(&ContactPtr->MobilePhone[0], BufPtr, (MAX_LEN_PHONE_NUM));
	BufPtr += (MAX_LEN_PHONE_NUM);
    memcpy(&ContactPtr->FaxPhone[0], BufPtr, (MAX_LEN_PHONE_NUM));
	BufPtr += (MAX_LEN_PHONE_NUM);
    memcpy(&ContactPtr->Email[0], BufPtr, (MAX_LEN_USER_INFO_EMAIL));
	BufPtr += (MAX_LEN_USER_INFO_EMAIL);
	return BufPtr;
}
//---------------------------------------------------------------------------
void AddPrimaryUserGroupMask(USER_DB_INFO *UserDbInfoPtr, unsigned long long int SetMask)
{
	USER_INFO   *SelUserPtr = NULL;
    ObjListTask	*SelObjPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&UserInfoList);
	while(SelObjPtr)
	{
	    SelUserPtr = (USER_INFO*)SelObjPtr->UsedTask;
        if (isPrimaryGroupAccessUser(SelUserPtr))
            SelUserPtr->GroupMask |= SetMask;
		SelObjPtr = (ObjListTask*)GetNextObjectList(&UserInfoList);
	}
    UserInfoDBSave(UserDbInfoPtr);
}
//---------------------------------------------------------------------------
void RemUserGroupMask(USER_DB_INFO *UserDbInfoPtr, unsigned long long int ClearMask)
{
	USER_INFO   *SelUserPtr = NULL;
    ObjListTask	*SelObjPtr = NULL;

	SelObjPtr = (ObjListTask*)GetFistObjectList(&UserInfoList);
	while(SelObjPtr)
	{
	    SelUserPtr = (USER_INFO*)SelObjPtr->UsedTask;
        SelUserPtr->GroupMask &= (~ClearMask);
		SelObjPtr = (ObjListTask*)GetNextObjectList(&UserInfoList);
	}
    UserInfoDBSave(UserDbInfoPtr);
}
//---------------------------------------------------------------------------
