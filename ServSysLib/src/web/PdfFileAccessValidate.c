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

#include <sys/stat.h>
#ifdef _LINUX_X86_
#include <dirent.h>
#endif

#include "BaseWebServer.h"
#include "BaseHtmlConstData.h"

extern char KeySessionId[];
extern char SecKeyId[];
extern char ThrWebServName[];
extern PARAMWEBSERV *ParWebServPtr;
//---------------------------------------------------------------------------
bool PdfFileAccessValidate(PARAMWEBSERV *ParWebServPtr, char *BufAnsw, char *HttpCmd)
{
	bool         isParseDone = false;
    char*        FText = NULL;
	char*        FStrt = NULL;
	USER_SESSION *SessionPtr = NULL;
    unsigned int i, pars_read, SecKeyForm, SessionId, UserId;

	for(;;)
	{
        FText = (char*)AllocateMemory(strlen(HttpCmd)+1);
		if (!FText) break;
	    FStrt = FText;
        strcpy(FText, HttpCmd);

		/* Session identifier validation */
		i = FindCmdRequest(FText, KeySessionId);
		if (i == -1) break;
		if (strlen(&HttpCmd[i]) < SESSION_ID_KEY_LEN) break;
        SessionId = FindSessionByKey(&HttpCmd[i]);
		SessionPtr = GetSessionBySessionId(&ParWebServPtr->SessionManager, SessionId);
		if (!SessionPtr) break;
		if (SessionId != SessionPtr->SessionId) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);

		/* PDF extention file name validate */
		i = FindCmdRequest(FText, ".pdf");
		if (i == -1) break;

		/* Security key validation */
		i = FindCmdRequest(FText, SecKeyId);
		if (i == -1) break;
        FText = ParseParForm( &FText[i] );
        if (!FText) break;
	    pars_read = sscanf(FText, "%d", &SecKeyForm);
	    if (!pars_read) break;
		if (SecKeyForm != SessionPtr->SecureKey) break;
		FText = FStrt;
		strcpy(FText,HttpCmd);
		isParseDone = true;
		break;
	}
	if (FStrt) FreeMemory(FStrt);
	return isParseDone;
}
//---------------------------------------------------------------------------
