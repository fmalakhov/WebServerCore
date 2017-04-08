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

#include "CommonPlatform.h"
#include "SysLibTool.h"
#include "SysWebFunction.h"
#include "WebServInfo.h"

/* Pointer to the end of current part of HTML page generation string */
char *EndHtmlPageGenPtr = NULL;
/* Name of Web server */
char WebServerName[33];
/* Version of server */
char ServerVersion[32];

extern unsigned char UserBufEncKey[];
extern unsigned int EncKeyLen;

char Text1SymConv[] = "&quot;";
char Text2SymConv[] = "&amp;";
char Text3SymConv[] = "&lt;";
char Text4SymConv[] = "&gt;";
char Text5SymConv[] = "&cr;";
char Text6SymConv[] = "&nl;";

char Manf1[]="Jan";
char Manf2[]="Feb";
char Manf3[]="Mar";
char Manf4[]="Apr";
char Manf5[]="May";
char Manf6[]="Jun";
char Manf7[]="Jul";
char Manf8[]="Aug";
char Manf9[]="Sep";
char Manf10[]="Oct";
char Manf11[]="Now";
char Manf12[]="Dec";

char *TablDefNameManf[]={Manf1,Manf2,Manf3,Manf4,Manf5,Manf6,
                         Manf7,Manf8,Manf9,Manf10,Manf11,Manf12};

char Dey1[]="Sun";
char Dey2[]="Mon";
char Dey3[]="Tue";
char Dey4[]="Wed";
char Dey5[]="Thu";
char Dey6[]="Fri";
char Dey7[]="Sat";

char *TablDefNameDey[]={Dey1,Dey2,Dey3,Dey4,Dey5,Dey6,Dey7};

/* HTML char tags */
char HtmlSpaceTag[] = "&nbsp;";

static unsigned char PublicEncodeKeyList[] = {\
0x06, 0xc7, 0x11, 0x9f, 0x4b, 0x5f, 0xcb, 0x63, 0x12, 0x93, 0x42, 0xbf, 0x85, 0x5f, 0x6f, 0x93, \
0xdb, 0xc9, 0x8c, 0xf7, 0xec, 0xb3, 0xc8, 0x59, 0xac, 0x74, 0xc5, 0x04, 0xb2, 0xba, 0x42, 0x71, \
0x8f, 0xd2, 0x3f, 0x52, 0x29, 0xc3, 0x8f, 0x3e, 0x63, 0xf3, 0x9b, 0xf4, 0x44, 0x39, 0xc1, 0x93, \
0xd9, 0x02, 0xa0, 0xc7, 0x94, 0x88, 0x45, 0x37, 0x63, 0xb7, 0x6b, 0x4a, 0x2e, 0x2e, 0xdc, 0x08, \
0x15, 0x8b, 0xa2, 0x1f, 0x82, 0x79, 0x81, 0x62, 0x75, 0x0d, 0x8d, 0x46, 0x22, 0x01, 0xb5, 0xab, \
0x38, 0xe6, 0x23, 0x81, 0xf9, 0xbe, 0x49, 0x0e, 0x72, 0x9e, 0x01, 0x66, 0xf6, 0x63, 0x0c, 0x66, \
0xb9, 0xfb, 0xda, 0x29, 0x22, 0x5b, 0xf3, 0x62, 0x31, 0x21, 0x55, 0x0d, 0xa0, 0xfa, 0xae, 0xce, \
0x89, 0x56, 0xcf, 0x8f, 0xb4, 0xe2, 0xa4, 0x15, 0x73, 0x82, 0x20, 0x37, 0x9b, 0x93, 0x6e, 0xe6 };

static unsigned int PublicEncodeKeyLen = sizeof(PublicEncodeKeyList)/sizeof(unsigned char);

static char InvalidFileName[] = {
"\r\nConnection: close\r\n"
"Content-Type: text/html\r\n\r\n"
"<!DOCTYPE HTML PUBLIC \"- HTML 2.0//EN\">\r\n"
"<HTML>\r\n<HEAD>\r\n<TITLE>404 Not Found</TITLE>\r\n</HEAD>\r\n"
"<BODY>\r\n<H1>Not Found</H1>\r\n"
"Your browser sent incorrect file request.\r\n"
"<P>\r\nFile " };
//---------------------------------------------------------------------------
void SetDateHeadRequest(char *StrOut)
{
#ifdef WIN32
    struct _SYSTEMTIME RealTime;
#else        
    struct timeb hires_cur_time;
    struct tm *cur_time;     
#endif

char FormDate[64];

#ifdef WIN32
GetLocalTime( &RealTime );
sprintf(FormDate,"Date: %s %d %s %d %02d:%02d:%02d GMT\r\n",
    TablDefNameDey[RealTime.wDayOfWeek],RealTime.wDay, TablDefNameManf[RealTime.wMonth-1],
    RealTime.wYear, RealTime.wHour, RealTime.wMinute, RealTime.wSecond);
strcat(StrOut,FormDate);
#else
    ftime(&hires_cur_time);
    cur_time = localtime(&hires_cur_time.time);
    sprintf(FormDate, "Date: %s %d %s %d %02d:%02d:%02d GMT\r\n",
        TablDefNameDey[cur_time->tm_wday], cur_time->tm_mday, TablDefNameManf[cur_time->tm_mon],
        (cur_time->tm_year+1900), cur_time->tm_hour, cur_time->tm_min, cur_time->tm_sec);
    strcat(StrOut,FormDate);
#endif
}
//---------------------------------------------------------------------------
void SetStartDateFileHeadRequest(char *StrOut)
{
#ifdef WIN32
    struct _SYSTEMTIME RealTime;
#else       
    struct timeb hires_cur_time;
    struct tm *cur_time;     
#endif

#ifdef WIN32
GetLocalTime( &RealTime );
sprintf(StrOut,"Last-Modified: %s, %d %s %d %02d:%02d:%02d GMT\r\n",
    TablDefNameDey[RealTime.wDayOfWeek],RealTime.wDay, TablDefNameManf[RealTime.wMonth-1],
    RealTime.wYear, RealTime.wHour, RealTime.wMinute, RealTime.wSecond);
#else 
    ftime(&hires_cur_time);
    cur_time = localtime(&hires_cur_time.time);
    sprintf(StrOut, "Last-Modified: %s, %d %s %d %02d:%02d:%02d GMT\r\n",
        TablDefNameDey[cur_time->tm_wday], cur_time->tm_mday, TablDefNameManf[cur_time->tm_mon],
        (cur_time->tm_year+1900), cur_time->tm_hour, cur_time->tm_min, cur_time->tm_sec);
#endif
}
//---------------------------------------------------------------------------
#ifndef WIN32
void SetLastModifyedDate(char *StrOut, time_t LastTime)
{
    struct tm *last_time; 

	last_time = localtime(&LastTime);
    sprintf(StrOut, "Last-Modified: %s, %d %s %d %02d:%02d:%02d GMT\r\n",
        TablDefNameDey[last_time->tm_wday], last_time->tm_mday, TablDefNameManf[last_time->tm_mon],
        (last_time->tm_year+1900), last_time->tm_hour, last_time->tm_min, last_time->tm_sec);
}
#endif
//---------------------------------------------------------------------------
void SetExpDateFileHeadRequest(char *StrOut)
{
#ifdef WIN32
    struct _SYSTEMTIME RealTime;
#else     
    struct timeb hires_cur_time;
    struct tm *cur_time;     
#endif

#ifdef WIN32
GetLocalTime( &RealTime );
sprintf(StrOut,"Expires: %s, %d %s %d %02d:%02d:%02d GMT\r\n",
    TablDefNameDey[RealTime.wDayOfWeek],RealTime.wDay, TablDefNameManf[RealTime.wMonth-1],
    RealTime.wYear+1, RealTime.wHour, RealTime.wMinute, RealTime.wSecond);
#else
    ftime(&hires_cur_time);
    cur_time = localtime(&hires_cur_time.time);
    sprintf(StrOut, "Expires: %s, %d %s %d %02d:%02d:%02d GMT\r\n",
        TablDefNameDey[cur_time->tm_wday], cur_time->tm_mday, TablDefNameManf[cur_time->tm_mon],
        (cur_time->tm_year+1901), cur_time->tm_hour, cur_time->tm_min, cur_time->tm_sec);
#endif
}
//---------------------------------------------------------------------------
void SetExpDateCookie(char *StrOut)
{
#ifdef WIN32
    struct _SYSTEMTIME RealTime;
#else      
    struct timeb hires_cur_time;
    struct tm *cur_time;     
#endif

#ifdef WIN32
    GetLocalTime( &RealTime );
    sprintf(StrOut,"%d-%s-%d %02d:%02d:%02d GMT",
        RealTime.wDay, TablDefNameManf[RealTime.wMonth-1],
        RealTime.wYear+1, RealTime.wHour, RealTime.wMinute, RealTime.wSecond);
#else
    ftime(&hires_cur_time);
    cur_time = localtime(&hires_cur_time.time);
    sprintf(StrOut, "%d-%s-%d %02d:%02d:%02d GMT",
        cur_time->tm_mday, TablDefNameManf[cur_time->tm_mon],
        (cur_time->tm_year+1901), cur_time->tm_hour, cur_time->tm_min, 
		cur_time->tm_sec);
#endif
}
//---------------------------------------------------------------------------
int FormStrPar(char *Dest,char* Sourse)
{
int j;

j = 0;
while ( j < (int)strlen(Sourse) )
  {if ( Sourse[j] == ' ' || Sourse[j] == '?' || Sourse[j] == '&') break;
   Dest[j] = Sourse[j];
   j++;
  }
Dest[j] = 0;
return j;
}
//---------------------------------------------------------------------------
int FindCmdReqLine(char *BufSourse, char *TestText, int LineLen)
{
    int i,j,k,rs,ReturnFind;

    i = 0;ReturnFind = -1;
    k = strlen(TestText);
    if (((int)strlen(BufSourse) < k) || (LineLen < k)) return ReturnFind;
    j = LineLen+1-k;
    while ( i < j )
	{
	    rs = strncmp(&BufSourse[i], TestText, k );
        if (rs == 0)
		{
		    ReturnFind = i+k;
            break;
		}
        i++;
	}
    return ReturnFind;
}
//---------------------------------------------------------------------------
void FormHeaderGoodAnswerWWW(char *StrHTM, char *ServerName)
{
strcpy(StrHTM,"HTTP/1.1 200 OK\r\n");
SetDateHeadRequest(StrHTM);
strcat(StrHTM,"Server: ");
strcat(StrHTM, ServerName);
strcat(StrHTM,"\r\nE Tag:\"97fe-354-ll-windows-1251\"\r\n");
strcat(StrHTM,"Accept-Range: bytes\r\n");
strcat(StrHTM,"Connection: close\r\n");
strcat(StrHTM,"Content-Type: text/html;charset=windows-1251\r\n");
strcat(StrHTM,"Vary:accept-charset\r\n\r\n");
}
//---------------------------------------------------------------------------
unsigned char ConvStrHex(char *Str)
{
unsigned char Value;

Value = ' ';
if (Str[0] >='0' && Str[0] <='9') Value = (unsigned char)((unsigned char)(Str[0]-'0')<<4);
if (Str[0] >='A' && Str[0] <='F') Value = (unsigned char)(((unsigned char)(Str[0]-'A')+10)<<4);
if (Str[0] >='a' && Str[0] <='f') Value = (unsigned char)(((unsigned char)(Str[0]-'a')+10)<<4);
if (Str[1] >='0' && Str[1] <='9') Value |= (unsigned char)((unsigned char)(Str[1]-'0'));
if (Str[1] >='A' && Str[1] <='F') Value |= (unsigned char)(((unsigned char)(Str[1]-'A')+10));
if (Str[1] >='a' && Str[1] <='f') Value |= (unsigned char)(((unsigned char)(Str[1]-'a')+10));
return Value;
}
//---------------------------------------------------------------------------
void ConverStrFormRequest(char *ResultStr,char *SourseStr,int MaxLenStr)
{
	int	m = 0;
	int	Ps = 0;
	while ( Ps < MaxLenStr )
	  {if ( SourseStr[m] == 0 || SourseStr[m] == '&' ) break;
	   switch ( SourseStr[m] )
         {case '+':ResultStr[Ps] =' ';break;
          case '%':
            ResultStr[Ps] = ConvStrHex( &SourseStr[m+1] );
            m += 2;
            break;
          default:ResultStr[Ps] = SourseStr[m];
         }
       m++;Ps++;
      }
	for (m=Ps;m > 0;m--) {if ( ResultStr[m-1] != ' ' ) break;}
	ResultStr[m] = 0;
	return;
}
//---------------------------------------------------------------------------
void FormStrEditForm( char *Result, char *Sourse )
{
    strcat(Result,"value=\"");
    strcat(Result,Sourse);
    strcat(Result,"\"");
    return;
}
//---------------------------------------------------------------------------
void FormPieceLineString( char *Dest, char *String, unsigned MaxViewLen )
{
	unsigned	Len;

	if ( !Dest || !String )	return;
	if ( strlen( String ) < MaxViewLen ) strcat(Dest,String );
	else
	  {Len = strlen(Dest);
	   memcpy( &Dest[Len],String,MaxViewLen );
	   Dest[Len+MaxViewLen] = 0;
	   strcat( Dest,"...." );
	  }
	return;
}
//---------------------------------------------------------------------------
bool StringParParse(char *CmdHttp, char *DestString, char *Key, int MaxStringLen)
{
	char *TextConvertBufPtr = NULL;
    char *FText = NULL;
	bool Result = false;
	int  i, TextLen;

	for(;;)
	{
	    i = FindCmdRequest(CmdHttp, Key);
		if (i == -1) break;
	    FText = ParseParForm( &CmdHttp[i] );
        if (!FText) break;
	    TextLen = strlen(FText);
	    if (TextLen)
		{
		    TextConvertBufPtr = (char*)AllocateMemory((TextLen+1)*sizeof(char));
            ConverStrFormRequest(TextConvertBufPtr, FText, TextLen );
		    TextLen = strlen(TextConvertBufPtr);
		    if ((!TextLen) || (TextLen > MaxStringLen))
		    {
			    FreeMemory(TextConvertBufPtr);
			    break;
		    }
		    strcpy(DestString, TextConvertBufPtr);
		    FreeMemory(TextConvertBufPtr);
		}
		else
		{
			DestString[0] = 0;
		}
		Result = true;
		break;
	}
	return Result;
}
//---------------------------------------------------------------------------
bool StringOptParParse(char *CmdHttp, char *DestString, char *Key, int MaxStringLen)
{
	char *TextConvertBufPtr = NULL;
    char *FText = NULL;
	bool Result = false;
	int  i, TextLen;

	for(;;)
	{
	    i = FindCmdRequest(CmdHttp, Key);
		if (i != -1)
		{
	        FText = ParseParForm( &CmdHttp[i] );
            if (!FText) break;
	        TextLen = strlen(FText);
	        if (TextLen)
			{
		        TextConvertBufPtr = (char*)AllocateMemory((TextLen+1)*sizeof(char));
                ConverStrFormRequest(TextConvertBufPtr, FText, TextLen );
		        TextLen = strlen(TextConvertBufPtr);
		        if ((!TextLen) || (TextLen > MaxStringLen))
				{
			        FreeMemory(TextConvertBufPtr);
			        break;
				}
		        strcpy(DestString, TextConvertBufPtr);
		        FreeMemory(TextConvertBufPtr);
			}
		    else
			{
			    DestString[0] = 0;
			}
		}
		else
		{
            memset(DestString, 0, MaxStringLen);
		}
		Result = true;
		break;
	}
	return Result;
}
//---------------------------------------------------------------------------
char* GetZoneParFunction(unsigned char *CmdLinePtr)
{
	unsigned		i,j;
	unsigned int    CmdLen;
    bool            Find;

    Find = false;
	CmdLen=strlen((const char*)CmdLinePtr);
	j = i = 0;
    for (;j < CmdLen;j++)
    {
		if (CmdLinePtr[j] == '(')
        {
			Find = true;
			break;
		}
    }
    if ( !Find ) 
	{
		return 0;
	}
    Find = false;
    j++;
    i = j;
    for (i=CmdLen-1;i > j;i--)
    {
		if (CmdLinePtr[i] == ')')
        {
			Find = true;
			break;
		}
    }
    if ( !Find ) 
	{
		return 0;
	}
    CmdLinePtr[i] = 0;
    return (char*)&CmdLinePtr[j];
}
//---------------------------------------------------------------------------
#ifdef WIN32
bool isTime1MoreTime2(SYSTEMTIME *TimeItem1, SYSTEMTIME *TimeItem2)
{
	bool isDiffBlk = false;
	unsigned int DaySumm1, DaySumm2;

	if (TimeItem1->wYear > TimeItem2->wYear)
	{
	    isDiffBlk = true;
	}
	else if (TimeItem1->wYear == TimeItem2->wYear)
	{
	    if (TimeItem1->wMonth > TimeItem2->wMonth)
		{
		    isDiffBlk = true;
		}
		else if (TimeItem1->wMonth == TimeItem2->wMonth)
		{
		    DaySumm1 =  (unsigned int)TimeItem1->wSecond;
			DaySumm1 += (unsigned int)TimeItem1->wMinute*60;
			DaySumm1 += (unsigned int)TimeItem1->wHour*3600;
			DaySumm1 += (unsigned int)TimeItem1->wDay*86400;
			DaySumm2 =  (unsigned int)TimeItem2->wSecond;
			DaySumm2 += (unsigned int)TimeItem2->wMinute*60;
			DaySumm2 += (unsigned int)TimeItem2->wHour*3600;
			DaySumm2 += (unsigned int)TimeItem2->wDay*86400;
			if (DaySumm1 > DaySumm2)
			{
			    isDiffBlk = true;
			}
	    }
    }
	return isDiffBlk;
}
#else
bool isTime1MoreTime2(struct tm *TimeItem1, struct tm *TimeItem2)
{
	bool isDiffBlk = false;
	unsigned int DaySumm1, DaySumm2;

	if (TimeItem1->tm_year > TimeItem2->tm_year)
	{
	    isDiffBlk = true;
	}
	else if (TimeItem1->tm_year == TimeItem2->tm_year)
	{
	    if (TimeItem1->tm_mon > TimeItem2->tm_mon)
		{
		    isDiffBlk = true;
		}
		else if (TimeItem1->tm_mon == TimeItem2->tm_mon)
		{
		    DaySumm1 =  (unsigned int)TimeItem1->tm_sec;
			DaySumm1 += (unsigned int)TimeItem1->tm_min*60;
			DaySumm1 += (unsigned int)TimeItem1->tm_hour*3600;
			DaySumm1 += (unsigned int)TimeItem1->tm_mday*86400;
			DaySumm2 =  (unsigned int)TimeItem2->tm_sec;
			DaySumm2 += (unsigned int)TimeItem2->tm_min*60;
			DaySumm2 += (unsigned int)TimeItem2->tm_hour*3600;
			DaySumm2 += (unsigned int)TimeItem2->tm_mday*86400;
			if (DaySumm1 > DaySumm2)
			{
			    isDiffBlk = true;
			}
	    }
    }
	return isDiffBlk;
}
#endif
//---------------------------------------------------------------------------
char* TextAreaConvertFile(char *BaseTextPtr)
{
    char *DestFilePtr = NULL;
	char *DestPtr = NULL;
	char *SrcPtr = NULL;
    unsigned int TextLen, Index, CnvLen;

	if (!BaseTextPtr) return NULL;
    TextLen = strlen(BaseTextPtr);
	if (!TextLen) return NULL;
    DestFilePtr = (char*)AllocateMemory(2*TextLen*sizeof(char));
	DestPtr = DestFilePtr;
    *DestPtr = 0;
	SrcPtr = BaseTextPtr;
	for (Index=0;Index < TextLen;Index++)
	{
		switch(*SrcPtr)
		{
		    case '\r':
				CnvLen = sizeof(Text5SymConv)/sizeof(char)-1;
				memcpy(DestPtr, Text5SymConv, CnvLen);
                DestPtr += CnvLen;
				break;

			case '\n':
				CnvLen = sizeof(Text6SymConv)/sizeof(char)-1;
				memcpy(DestPtr, Text6SymConv, CnvLen);
                DestPtr += CnvLen;
				break;

			case '"':
				CnvLen = sizeof(Text1SymConv)/sizeof(char)-1;
				memcpy(DestPtr, Text1SymConv, CnvLen);
                DestPtr += CnvLen;
				break;

			case '&':
				CnvLen = sizeof(Text2SymConv)/sizeof(char)-1;
				memcpy(DestPtr, Text2SymConv, CnvLen);
                DestPtr += CnvLen;
				break;

			case '<':
				CnvLen = sizeof(Text3SymConv)/sizeof(char)-1;
				memcpy(DestPtr, Text3SymConv, CnvLen);
                DestPtr += CnvLen;
				break;

			case '>':
				CnvLen = sizeof(Text4SymConv)/sizeof(char)-1;
				memcpy(DestPtr, Text4SymConv, CnvLen);
                DestPtr += CnvLen;
				break;

			default:
				*DestPtr++ = *SrcPtr;
				break;
		}
        SrcPtr++;
	}
    *DestPtr++ = 0;
	return DestFilePtr;
}
//---------------------------------------------------------------------------
void TextAreaConvertBase(char *SrcTextPtr, char *DestTextPtr, unsigned int MaxLen)
{
    char *DestFilePtr = NULL;
	char *DestPtr = NULL;
	char *SrcPtr = NULL;
    unsigned int TextLen, Index, CnvLen;

	if (!SrcTextPtr  || !DestTextPtr || !MaxLen) return;
    TextLen = strlen(SrcTextPtr);
	if (!TextLen)
	{
        *DestTextPtr = 0;
		return;
	}
	DestPtr = DestTextPtr;
    *DestPtr = 0;
	SrcPtr = SrcTextPtr;
	for (Index=0;Index < TextLen;Index++)
	{
		if (*SrcPtr == 0) break;
        else if(*SrcPtr == '&')
		{
			for(;;)
			{
			    CnvLen = sizeof(Text1SymConv)/sizeof(char)-1;
			    if (((TextLen - Index) >= CnvLen) &&
					(memcmp(SrcPtr, Text1SymConv, CnvLen) == 0))
				{
                    *DestPtr++ = '"';
					SrcPtr += CnvLen;
					break;
				}
			    CnvLen = sizeof(Text2SymConv)/sizeof(char)-1;
			    if (((TextLen - Index) >= CnvLen) &&
					(memcmp(SrcPtr, Text2SymConv, CnvLen) == 0))
				{
                    *DestPtr++ = '&';
					SrcPtr += CnvLen;
					break;
				}
			    CnvLen = sizeof(Text3SymConv)/sizeof(char)-1;
			    if (((TextLen - Index) >= CnvLen) &&
					(memcmp(SrcPtr, Text3SymConv, CnvLen) == 0))
				{
                    *DestPtr++ = '<';
					SrcPtr += CnvLen;
					break;
				}
			    CnvLen = sizeof(Text4SymConv)/sizeof(char)-1;
			    if (((TextLen - Index) >= CnvLen) &&
					(memcmp(SrcPtr, Text4SymConv, CnvLen) == 0))
				{
                    *DestPtr++ = '>';
					SrcPtr += CnvLen;
					break;
				}
			    CnvLen = sizeof(Text5SymConv)/sizeof(char)-1;
			    if (((TextLen - Index) >= CnvLen) &&
					(memcmp(SrcPtr, Text5SymConv, CnvLen) == 0))
				{
                    *DestPtr++ = '\r';
					SrcPtr += CnvLen;
					break;
				}
			    CnvLen = sizeof(Text6SymConv)/sizeof(char)-1;
			    if (((TextLen - Index) >= CnvLen) &&
					(memcmp(SrcPtr, Text6SymConv, CnvLen) == 0))
				{
                    *DestPtr++ = '\n';
					SrcPtr += CnvLen;
					break;
				}
                *DestPtr++ = *SrcPtr;
			    *SrcPtr++;
				break;
			}
		}
		else
		{
            *DestPtr++ = *SrcPtr;
			*SrcPtr++;
		}
		if ((unsigned int)(DestPtr - DestTextPtr) > (MaxLen-1)) break;
	}
	*DestPtr = 0;
}
//---------------------------------------------------------------------------
#ifdef WIN32         
unsigned char* DatePack(unsigned char* BufPtr, SYSTEMTIME *DatePtr)
{
    *BufPtr++ = (unsigned char)DatePtr->wDay;
	*BufPtr++ = (unsigned char)DatePtr->wMonth;
	*BufPtr++ = (unsigned char)((DatePtr->wYear & 0xff00) >> 8);
	*BufPtr++ = (unsigned char)(DatePtr->wYear & 0x00ff);
	*BufPtr++ = (unsigned char)DatePtr->wHour;
    *BufPtr++ = (unsigned char)DatePtr->wMinute;
	*BufPtr++ = (unsigned char)DatePtr->wSecond;
	return BufPtr;
}
#else       
unsigned char* DatePack(unsigned char* BufPtr, struct tm *DatePtr)
{
    *BufPtr++ = (unsigned char)DatePtr->tm_mday;
	*BufPtr++ = (unsigned char)(DatePtr->tm_mon+1);
	*BufPtr++ = (unsigned char)(((DatePtr->tm_year+1900) & 0xff00) >> 8);
	*BufPtr++ = (unsigned char)((DatePtr->tm_year+1900) & 0x00ff);
	*BufPtr++ = (unsigned char)DatePtr->tm_hour;
    *BufPtr++ = (unsigned char)DatePtr->tm_min;
	*BufPtr++ = (unsigned char)DatePtr->tm_sec;
	return BufPtr;
}
#endif
//---------------------------------------------------------------------------
#ifdef WIN32
unsigned char* DateUnpack(unsigned char* BufPtr, SYSTEMTIME *DatePtr)
{
    DatePtr->wDay = *BufPtr++;
	DatePtr->wMonth = *BufPtr++;
	DatePtr->wYear  = ((unsigned short)(*BufPtr++) << 8);
	DatePtr->wYear |= (unsigned short)(*BufPtr++);
	DatePtr->wHour = *BufPtr++;
    DatePtr->wMinute = *BufPtr++;
	DatePtr->wSecond = *BufPtr++;
	return BufPtr;
}
#else       
unsigned char* DateUnpack(unsigned char* BufPtr, struct tm *DatePtr)
{
	unsigned int UnpackYear;

    DatePtr->tm_mday = *BufPtr++;
	DatePtr->tm_mon = ((*BufPtr++) - 1);
	UnpackYear = (unsigned int)(*BufPtr++) << 8;
	UnpackYear |= (unsigned int)(*BufPtr++);
	DatePtr->tm_year = UnpackYear - 1900;
	DatePtr->tm_hour = *BufPtr++;
    DatePtr->tm_min = *BufPtr++;
	DatePtr->tm_sec = *BufPtr++;
	return BufPtr;
}
#endif
//---------------------------------------------------------------------------
void SetDateWebPage()
{
	char           CmdGenBuf[164];
#ifdef WIN32
	SYSTEMTIME     SysTime;
#else
        struct timeb hires_cur_time;
	struct tm      *SysTime;
#endif

#ifdef WIN32
    GetSystemTime(&SysTime);
	sprintf(CmdGenBuf,"%02d.%02d.%d",SysTime.wDay, SysTime.wMonth, SysTime.wYear);
#else 
    ftime(&hires_cur_time);
    SysTime = localtime(&hires_cur_time.time);
	sprintf(CmdGenBuf,"%02d.%02d.%d",SysTime->tm_mday, SysTime->tm_mon+1, SysTime->tm_year+1900);
#endif
	AddStrWebPage(CmdGenBuf);
}
//---------------------------------------------------------------------------
unsigned int SecureKeyGen()
{
	return (unsigned int)((double)(rand()%RAND_GEN_MASK) / 
		(RAND_GEN_MAX + 1) * (SECURE_RANGE_MAX - SECURE_RANGE_MIN) + SECURE_RANGE_MIN);
}
//---------------------------------------------------------------------------
void ConfirmKeyGen(unsigned char DevType, char *ConfirmKeyStrPtr)
{
    unsigned int Rand1Index, Rand2Index, Rand3Index,
                 Rand4Index, Rand5Index, DateIndex;
#ifdef WIN32
	SYSTEMTIME    CurrTime;
#else       
    struct timeb  hires_cur_time;
    struct tm     *CurrTime;     
#endif

#ifdef WIN32
	GetSystemTime(&CurrTime);
	DateIndex = (unsigned int)CurrTime.wMinute;
	DateIndex += ((unsigned int)CurrTime.wHour)*60;
	DateIndex += ((unsigned int)CurrTime.wDay)*1440;
	DateIndex += ((unsigned int)CurrTime.wMonth)*44640;
	DateIndex += ((unsigned int)CurrTime.wYear-2010)*535680;
#else
    ftime(&hires_cur_time);
    CurrTime = localtime(&hires_cur_time.time);
	DateIndex = (unsigned int)CurrTime->tm_min;
	DateIndex += ((unsigned int)CurrTime->tm_hour)*60;
	DateIndex += ((unsigned int)CurrTime->tm_mday)*1440;
	DateIndex += ((unsigned int)CurrTime->tm_mon+1)*44640;
	DateIndex += ((unsigned int)CurrTime->tm_year-110)*535680;
#endif
	Rand1Index = SecureKeyGen();
    Rand2Index = SecureKeyGen();
    Rand3Index = SecureKeyGen();
    Rand4Index = SecureKeyGen();
    Rand5Index = SecureKeyGen();
    if (DevType == SDT_DESCTOP)
    {
	    sprintf(ConfirmKeyStrPtr, "%06X%06X%06X%06X%06X%06X", 
            DateIndex, Rand3Index, Rand5Index, Rand1Index, Rand4Index, Rand2Index);
    }
    else
    {
        Rand1Index = ((DateIndex ^ Rand4Index) % 900000) + 100000;
        sprintf(ConfirmKeyStrPtr, "%06d", Rand1Index);
    }
}
//---------------------------------------------------------------------------
void AddStrWebPage(char *SrcStr)
{
	register int len = strlen(SrcStr);
	memcpy(EndHtmlPageGenPtr, SrcStr, len);
	EndHtmlPageGenPtr += len;
	*EndHtmlPageGenPtr = 0;
 }
//---------------------------------------------------------------------------
void AddLenStrWebPage(char *SrcStr, unsigned int len)
{
	memcpy(EndHtmlPageGenPtr, SrcStr, len);
	EndHtmlPageGenPtr += len;
	*EndHtmlPageGenPtr = 0;
}
//---------------------------------------------------------------------------
void AddSpaceSetStrWebPage(char *SrcStr)
{
    int i, len;
    
	len = strlen(SrcStr);
    for (i=0;i < len;i++)
    {
        if (*SrcStr == ' ')
        {
            memcpy(EndHtmlPageGenPtr, HtmlSpaceTag, 6);  
            EndHtmlPageGenPtr += 6;
        }
        else
        {
            *EndHtmlPageGenPtr = *SrcStr;
            EndHtmlPageGenPtr++;
        }
        SrcStr++;
    }
	*EndHtmlPageGenPtr = 0;
}
//---------------------------------------------------------------------------
void SetHiddenIntParForm(char *BufAnsw, char *ParPtr, int Value)
{
	char StrBuf[128];

	sprintf(StrBuf, "name=\"%s\" value=\"%d\" >\r\n", ParPtr, Value);
	if (BufAnsw)
	{
	    strcat(BufAnsw,"<input type=\"hidden\" ");
	    strcat(BufAnsw, StrBuf);
	}
	else
	{
	    AddStrWebPage("<input type=\"hidden\" ");
	    AddStrWebPage(StrBuf);
	}
}
//---------------------------------------------------------------------------
void SetHiddenStrParForm(char *BufAnsw, char *ParPtr, char *ValPtr)
{
	char StrBuf[256];

	sprintf(StrBuf, "name=\"%s\" value=\"%s\" >\r\n", ParPtr, ValPtr);
	if (BufAnsw)
	{
	    strcat(BufAnsw,"<input type=\"hidden\" ");
	    strcat(BufAnsw, StrBuf);
	}
	else
	{
	    AddStrWebPage("<input type=\"hidden\" ");
	    AddStrWebPage(StrBuf);
	}
}
//---------------------------------------------------------------------------
void CreateRespNoAcess(char *BufAnsw,char *LocalAdress,int LocalPortServ)
{
	//Response for not acess.
	char	FormMess[128];

	strcpy(BufAnsw,"HTTP/1.1 403 Access denied\r\n");
	SetDateHeadRequest( BufAnsw );
	strcat(BufAnsw,"Server: ");
    strcat(BufAnsw, WebServerName);
	strcat(BufAnsw,"\r\nConnection: close\r\n");
	strcat(BufAnsw,"Content-Type: text/html\r\n\r\n");
	strcat(BufAnsw,"<!DOCTYPE HTML PUBLIC \"- HTML 2.0//EN\">\r\n");
	strcat(BufAnsw,"<HTML>\r\n<HEAD>\r\n<TITLE>403 Acess denied</TITLE>\r\n</HEAD>\r\n");
	strcat(BufAnsw,"<BODY>\r\n<H1>Access denied!</H1>\r\n");
	strcat(BufAnsw,"You are not autorised for access to this server.\r\n");
	strcat(BufAnsw,"<P>\r\n");
	strcat(BufAnsw,"Server ignores this web request.\r\n");
	strcat(BufAnsw,"<P>\r\n<HR>\r\n");
	sprintf(FormMess,"<ADDRESS>%s %s Server at %s Port %d </ADDRESS>\r\n",
		WebServerName, ServerVersion, LocalAdress, LocalPortServ );
	strcat(BufAnsw,FormMess);
	strcat(BufAnsw,"</BODY>\r\n</HTML>\r\n");
	return;
}
//---------------------------------------------------------------------------
void CreateRespWrongData(char *BufAnsw, char *LocalAdress, int LocalPortServ)
{
	char	FormMess[128];

	strcpy(BufAnsw,"HTTP/1.1 400 Bad Request\r\n");
	SetDateHeadRequest(BufAnsw);
	strcat(BufAnsw,"Server: ");
    strcat(BufAnsw, WebServerName);
	strcat(BufAnsw,"\r\nConnection: close\r\n");
	strcat(BufAnsw, "Content-Type: text/html\r\n\r\n");
	strcat(BufAnsw, "<!DOCTYPE HTML PUBLIC \"- HTML 2.0//EN\">\r\n");
	strcat(BufAnsw, "<HTML>\r\n<HEAD>\r\n<TITLE>400 Bad Request</TITLE>\r\n</HEAD>\r\n");
	strcat(BufAnsw, "<BODY>\r\n<H1>Not Found</H1>\r\n");
	strcat(BufAnsw, "Your browser sent a request contain unpresent data.\r\n");
	strcat(BufAnsw, "<P>\r\n");
	strcat(BufAnsw, "Server do not understend this command.\r\n");
	strcat(BufAnsw, "<P>\r\n<HR>\r\n");
	sprintf(FormMess,"<ADDRESS>%s %s Server at %s Port %d </ADDRESS>\r\n",
		WebServerName, ServerVersion, LocalAdress, LocalPortServ);
	strcat(BufAnsw, FormMess);
	strcat(BufAnsw, "</BODY>\r\n</HTML>\r\n");
	return;
}
//---------------------------------------------------------------------------
void CreateRespTooLargeData(char *BufAnsw, char *LocalAdress, int LocalPortServ)
{
	char	FormMess[128];

	strcpy(BufAnsw,"HTTP/1.1 413 Request is too large\r\n");
	SetDateHeadRequest(BufAnsw);
	strcat(BufAnsw,"Server: ");
    strcat(BufAnsw, WebServerName);
	strcat(BufAnsw,"\r\nConnection: close\r\n");
	strcat(BufAnsw, "Content-Type: text/html\r\n\r\n");
	strcat(BufAnsw, "<!DOCTYPE HTML PUBLIC \"- HTML 2.0//EN\">\r\n");
	strcat(BufAnsw, "<HTML>\r\n<HEAD>\r\n<TITLE>413 Request is too large</TITLE>\r\n</HEAD>\r\n");
	strcat(BufAnsw, "<BODY>\r\n<H1>Not Found</H1>\r\n");
	strcat(BufAnsw, "Your browser sent a request contain unpresent data.\r\n");
	strcat(BufAnsw, "<P>\r\n");
	strcat(BufAnsw, "Server is skip processing of too large request.\r\n");
	strcat(BufAnsw, "<P>\r\n<HR>\r\n");
	sprintf(FormMess,"<ADDRESS>%s %s Server at %s Port %d </ADDRESS>\r\n",
		WebServerName, ServerVersion, LocalAdress, LocalPortServ);
	strcat(BufAnsw, FormMess);
	strcat(BufAnsw, "</BODY>\r\n</HTML>\r\n");
	return;
}
//---------------------------------------------------------------------------
void CreateRespServerShutdown(char *BufAnsw,char *LocalAdress,int LocalPortServ)
{
	//Response for not acess.
	char	FormMess[128];

	FormHeaderGoodAnswerWWW(BufAnsw, WebServerName);
	strcat(BufAnsw,"<!DOCTYPE HTML PUBLIC \"- HTML 2.0//EN\">\r\n");
	strcat(BufAnsw,"<HTML>\r\n<HEAD>\r\n<TITLE>Server shutdown is in progress</TITLE>\r\n</HEAD>\r\n");
	strcat(BufAnsw,"<BODY>\r\n<H1>Server shutdown</H1>\r\n");
	strcat(BufAnsw,"We could not handle you request since server during shutdown.\r\n");
	strcat(BufAnsw,"<P>\r\n");
	strcat(BufAnsw,"Server did not handle this web request.\r\n");
	strcat(BufAnsw,"<P>\r\n<HR>\r\n");
	sprintf(FormMess,"<ADDRESS>%s %s Server at %s Port %d </ADDRESS>\r\n",
		WebServerName, ServerVersion, LocalAdress, LocalPortServ );
	strcat(BufAnsw,FormMess);
	strcat(BufAnsw,"</BODY>\r\n</HTML>\r\n");
	return;
}
//---------------------------------------------------------------------------
void CloseHttpSocket(SOCKET HttpSocket)
{
#ifdef WIN32
    closesocket(HttpSocket);
#else
    int Res = shutdown(HttpSocket, SHUT_RDWR);
    close(HttpSocket);
#endif
}
//---------------------------------------------------------------------------
void CreateHttpInvalidFileNameResp( char *StrHTM, char *FileName, char *LocalAdress, int LocalPortServ)
{
	char FormMess[128];

	EndHtmlPageGenPtr = StrHTM;
	*StrHTM = 0;
	*FormMess = 0;
    AddLenStrWebPage("HTTP/1.1 404 Not Found\r\n", 24);
	SetDateHeadRequest(FormMess);
	AddStrWebPage(FormMess);
	AddLenStrWebPage("Server: ", 8);
    AddStrWebPage(WebServerName);
	AddLenStrWebPage(InvalidFileName, sizeof(InvalidFileName)/sizeof(char) - 1);
	AddStrWebPage(FileName);
	AddLenStrWebPage(" not found on server.\r\n<P>\r\n<HR>\r\n", 34);
	sprintf(FormMess, "<ADDRESS>%s %s Server at %s Port %d </ADDRESS>\r\n",
		WebServerName, ServerVersion, LocalAdress, LocalPortServ);
	AddStrWebPage(FormMess);
	AddLenStrWebPage("</BODY>\r\n</HTML>\r\n", 18);
	return;
}
//---------------------------------------------------------------------------
void SessionKeyGen(unsigned char *DestEncodePtr)
{
	unsigned int    i;
    unsigned char   EncodeChar;
    unsigned char   *SrcEncodePtr;

    /* Session related public key fillout */
    SrcEncodePtr = PublicEncodeKeyList;
    EncodeChar = UserBufEncKey[((unsigned int)GetTickCount() ^ (unsigned int)rand()) % EncKeyLen];
    for(i=0;i < PublicEncodeKeyLen;i++)
    {
        *DestEncodePtr = *SrcEncodePtr ^ EncodeChar;
        DestEncodePtr++;
        SrcEncodePtr++;
    }
}
//---------------------------------------------------------------------------
void AddSessionKeyWebPage(unsigned char *KeyPtr)
{
    bool         isFirst = true;
    unsigned int i;
    char         StrBuf[16];

    for(i=0;i < PublicEncodeKeyLen;i++)
    {
        if (isFirst)
        {
           sprintf(StrBuf, "0x%02x", *KeyPtr++);
           AddStrWebPage(StrBuf);
           isFirst = false;
        }
        else
        {
           sprintf(StrBuf, ", 0x%02x", *KeyPtr++);
           AddStrWebPage(StrBuf);        
        }
    }    
}
//---------------------------------------------------------------------------
void UserAuthEncodeGen(unsigned char *UserAuthEncodePtr)
{
    unsigned char   EncodeShift;
    unsigned int    Rand1Index, CurrTick;
    
    Rand1Index = SecureKeyGen();
    CurrTick = (unsigned int)(GetTickCount() & 0xffffffff);
    Rand1Index ^= CurrTick;
    EncodeShift = (unsigned char)(rand()%USER_AUTH_ENCODE_PUBLIC_KEY_LEN);
    *UserAuthEncodePtr++ = EncodeShift;
    *UserAuthEncodePtr++ = (unsigned char)((Rand1Index & 0xff000000) >> 24);
    *UserAuthEncodePtr++ = (unsigned char)((Rand1Index & 0xff0000) >> 16);
    *UserAuthEncodePtr++ = (unsigned char)((Rand1Index & 0xff00) >> 8);
    *UserAuthEncodePtr++ = (unsigned char)(Rand1Index & 0xff);
}
//---------------------------------------------------------------------------
bool LoginDataDecrypt(unsigned char *SessionKey, unsigned char *UserAuthEncode, char *EncData, char *LoginPtr, char *PasswdPtr)
{
    unsigned char RVal;
    bool          ErrDetect = false;
    unsigned int  i, j, k, l, m;
    unsigned char DecodeData[MAX_LEN_USER_INFO_USER_NAME + MAX_LEN_USER_INFO_PASSWD + 3];

    k = 1;
    m = 0;
    j = strlen(EncData);
    l = (MAX_LEN_USER_INFO_USER_NAME+MAX_LEN_USER_INFO_PASSWD+2)*2;
    if (j > l) return false;
    l = *UserAuthEncode;
    for(i=0;i < j;i+=2)
    {
        if (EncData[i] > '9')
        {
            if ((EncData[i] < 'A') || (EncData[i] > 'F'))
            {
                ErrDetect = true;
                break;
            }
            else
            {
                RVal = ((EncData[i] - 'A') + 10) << 4;            
            }
        }
        else
        {
            if ((EncData[i] < '0') || (EncData[i] > '9'))
            {
                ErrDetect = true;
                break;
            }
            else
            {
                RVal = (EncData[i] - '0') << 4;            
            }
        }
        
        if (EncData[i+1] > '9')
        {
            if ((EncData[i+1] < 'A') || (EncData[i+1] > 'F'))
            {
                ErrDetect = true;
                break;
            }
            else
            {
                RVal |= ((EncData[i+1] - 'A') + 10);
            }
        }
        else
        {
            if ((EncData[i+1] < '0') || (EncData[i+1] > '9'))
            {
                ErrDetect = true;
                break;
            }
            else
            {
                RVal |= (EncData[i+1] - '0');
            }
        }
        DecodeData[m++] = RVal ^ (SessionKey[l++] ^ UserAuthEncode[k++]);
        if (l == PublicEncodeKeyLen) l = 0;
        if (k == 5) k = 1;
    }
    if (ErrDetect) return false;    
    j >> 2;
    k = 0;
    i = (unsigned int)DecodeData[k++];
    j--;
    if ((j < i) || (i > MAX_LEN_USER_INFO_USER_NAME)) return false;
    memcpy(LoginPtr, &DecodeData[k], i);
    LoginPtr[i] = 0;
    k += i;
    j -= i;        
    i = (unsigned int)DecodeData[k++];
    j--;
    if ((j < i) || (i > MAX_LEN_USER_INFO_PASSWD)) return false;
    memcpy(PasswdPtr, &DecodeData[k], i);
    PasswdPtr[i] = 0;
    return true;
}
//---------------------------------------------------------------------------
