# if ! defined( HttpPageGenH )
#	define HttpPageGenH /* only include me once */

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

#ifndef TextLineCodeH
#include "TextLineCode.h"
#endif

#ifndef TextListDataBaseH
#include "TextListDataBase.h"
#endif

void AddPart1ShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr);
void SetMenuWebPage(char *BufAnsw, USER_SESSION *SessionPtr);
void AddBeginMiddlePartShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr);
void SortArraysInit();
void ShowForUserShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void UserRegisterShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void LostPasswdShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void NewUserRegister(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void ExistUserAuth(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void ExistUserExit(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void UserContactManage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void UserContactSet(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void RegClientsList(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void ServerConfigWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void PasswdSentMail(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void AdminServStatsWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void ActiveSessionsList(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void AdminPasswdChange(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void MenuTreeCreate();
unsigned char* ContactInfoPack(unsigned char* BufPtr, CONTACT_INFO* ContactPtr);
unsigned char* ContactInfoUnpack(unsigned char* BufPtr, CONTACT_INFO* ContactPtr);
void UserContactPageGen(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void SiteMapFileGen(PARAMWEBSERV *ParWebServ);

void DelUserRegShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void ChgUserTypeShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);

void GroupDataBase(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void ChgViewGroupDataBase(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void ChgUserHostAccessGrpWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);
void ChgUserGroupSetWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);

/* User's define page handler function prototypes */
void ShowOverviewShopWebPage(char *BufAnsw, USER_SESSION *SessionPtr, char *HttpCmd);

//---------------------------------------------------------------------------
#endif  /* if ! defined( HttpPageGensH ) */
