# if ! defined( SysWebFunctionH )
#	define SysWebFunctionH /* only include me once */

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

extern char *EndHtmlPageGenPtr;

#define DEF_HTTP_IP_PORT    80
#define DEF_HTTPS_IP_PORT   443
#define SIZE_FORM_BUF_HTTP	8192

void SetDateHeadRequest(char *StrOut);
void SetExpDateFileHeadRequest(char *StrOut);
void SetStartDateFileHeadRequest(char *StrOut);
void SetExpDateCookie(char *StrOut);
void FormHeaderGoodAnswerWWW(char *StrOut, char *ServerName);
int  FormStrPar(char *Dest,char* Sourse); //
int FindCmdReqLine(char *BufSourse, char *TestText, int LineLen);
unsigned char ConvStrHex(char *Str);
void ConverStrFormRequest(char *ResultStr,char *SourseStr,int MaxLenStr);
void FormStrEditForm( char *Result, char *Sourse ); //From string for sting edit on WEB form.
void FormPieceLineString( char *Dest, char *String, unsigned MaxViewLen ); //Form not full string in destination string.
bool StringParParse(char *CmdHttp, char *DestString, char *Key, int MaxStringLen);
bool StringOptParParse(char *CmdHttp, char *DestString, char *Key, int MaxStringLen);
char* GetZoneParFunction(unsigned char *CmdLinePtr);
char* TextAreaConvertFile(char *BaseTextPtr);
void TextAreaConvertBase(char *SrcTextPtr, char *DestTextPtr, unsigned int MaxLen);
void BufferSecure(unsigned char *Key, unsigned int KeyLen, char *DecBuf, char *EncBuf, unsigned int len);
void ConfirmKeyGen(unsigned char DevType, char *ConfirmKeyStrPtr);
unsigned int SecureKeyGen();
void AddStrWebPage(char *SrcStr);
void AddLenStrWebPage(char *SrcStr, unsigned int len);
void AddSpaceSetStrWebPage(char *SrcStr);
void SetDateWebPage();
void SetHiddenIntParForm(char *BufAnsw, char *ParPtr, int Value);
void SetHiddenStrParForm(char *BufAnsw, char *ParPtr, char *ValPtr);
void CreateRespNoAcess(char *BufAnsw,char *LocalAdress,int LocalPortServ);
void CreateRespWrongData(char *BufAnsw, char *LocalAdress, int LocalPortServ);
void CreateRespTooLargeData(char *BufAnsw, char *LocalAdress, int LocalPortServ);
void CreateRespServerShutdown(char *BufAnsw,char *LocalAdress,int LocalPortServ);
void CloseHttpSocket(SOCKET HttpSocket);
void CreateHttpInvalidFileNameResp( char *StrHTM, char *FileName, char *LocalAdress, int LocalPortServ);
void SessionKeyGen(unsigned char *DestEncodePtr);
void UserAuthEncodeGen(unsigned char *UserAuthEncodePtr);
void AddSessionKeyWebPage(unsigned char *KeyPtr);
bool LoginDataDecrypt(unsigned char *SessionKey, unsigned char *UserAuthEncode, char *EncData, char *LoginPtr, char *PasswdPtr);
#ifdef WIN32
bool isTime1MoreTime2(SYSTEMTIME *TimeItem1, SYSTEMTIME *TimeItem2);
unsigned char* DatePack(unsigned char* BufPtr, SYSTEMTIME *DatePtr);
unsigned char* DateUnpack(unsigned char* BufPtr, SYSTEMTIME *DatePtr);
#else
bool isTime1MoreTime2(struct tm *TimeItem1, struct tm *TimeItem2);
unsigned char* DatePack(unsigned char* BufPtr, struct tm *DatePtr);
unsigned char* DateUnpack(unsigned char* BufPtr, struct tm *DatePtr);
void SetLastModifyedDate(char *StrOut, time_t LastTime);
#endif
//---------------------------------------------------------------------------
#endif  /* if ! defined( SysWebFunctionH ) */
