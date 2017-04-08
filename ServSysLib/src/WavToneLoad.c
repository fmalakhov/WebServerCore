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
#include "WavToneLoad.h"
//---------------------------------------------------------------------------
void WavToneLoad(WAV_SAMPLE_BLOCK *WavDataPtr, char *TonePathNamePtr, unsigned int SampleRate)
{
	bool          HeaderParse = false;
	FILE          *FileHandler;    
	int           SysRes = 0;
	unsigned int  FileSize, WaveBlkLen, ReadValue;
    struct stat   st;
	unsigned char *UnpackPtr = NULL;
	unsigned char HeaderBlk[WAV_HEADER_LEN+1];

	WavDataPtr->WavLen = 0;
	WavDataPtr->WavBody = NULL;
	SysRes = stat(TonePathNamePtr, &st);
	if ((SysRes == 0) && ((unsigned int)st.st_size >= WAV_HEADER_LEN))
	{
	    FileSize = (unsigned int)st.st_size;
		WaveBlkLen = FileSize - WAV_HEADER_LEN;
		WavDataPtr->WavLen = WaveBlkLen >> 1;
	    FileHandler = fopen(TonePathNamePtr, "rb");
	    if (FileHandler)
	    {
		    SysRes = fread(HeaderBlk, WAV_HEADER_LEN, 1, FileHandler);
			if (SysRes)
			{
				for(;;)
				{
				    UnpackPtr = Uint32UnpackBE(HeaderBlk, &ReadValue);
					if (ReadValue != RIFF_ID) break;
					UnpackPtr = Uint32UnpackBE(UnpackPtr, &ReadValue);
					UnpackPtr = Uint32UnpackBE(UnpackPtr, &ReadValue);
					if (ReadValue != WAVE_ID) break;
					UnpackPtr = Uint32UnpackBE(UnpackPtr, &ReadValue);
					if (ReadValue != FMT_ID) break;
					UnpackPtr = Uint32UnpackBE(UnpackPtr, &ReadValue);
					if (ReadValue != 16) break;
					UnpackPtr = Uint16UnpackBE(UnpackPtr, &ReadValue);
					if (ReadValue != FORMAT_PCM) break;
					UnpackPtr = Uint16UnpackBE(UnpackPtr, &ReadValue);
					if (ReadValue != 1) break;
					UnpackPtr = Uint32UnpackBE(UnpackPtr, &ReadValue); /* Sample rate */
					if (ReadValue != SampleRate) break;
					UnpackPtr = Uint32UnpackBE(UnpackPtr, &ReadValue); /* Bytes rate */
					if (ReadValue != (SampleRate<<1)) break;
					UnpackPtr = Uint16UnpackBE(UnpackPtr, &ReadValue);
					if (ReadValue != 2) break;
					UnpackPtr = Uint16UnpackBE(UnpackPtr, &ReadValue);
					if (ReadValue != 16) break;
					UnpackPtr = Uint32UnpackBE(UnpackPtr, &ReadValue); /* "DATA" */
					UnpackPtr = Uint32UnpackBE(UnpackPtr, &ReadValue); /* Data size */
					HeaderParse = true;
					break;
				}

				if (!HeaderParse)
				{
					printf("Fail to parse header of %s wav sample\r\n", TonePathNamePtr);
				}
				else
				{
				    WavDataPtr->WavBody = (unsigned char*)AllocateMemory((WaveBlkLen + 1)*sizeof(unsigned char));
				    if (WavDataPtr->WavBody)
				    {
					    SysRes = fread(WavDataPtr->WavBody, WaveBlkLen, 1, FileHandler);
					    if (!SysRes)
					    {
						    printf("Fail to load body of %s wav sample\r\n", TonePathNamePtr);
						    FreeMemory(WavDataPtr->WavBody);
						    WavDataPtr->WavBody = NULL;
						    WavDataPtr->WavLen = 0;
					    }
				    }
				    else
				    {
				 	    printf("Fail to memory allocation for wav samle body\r\n");
					    WavDataPtr->WavLen = 0;
				    }
				}
			}
			else
			{
				printf("Fail to load header of %s wav sample\r\n", TonePathNamePtr);
				WavDataPtr->WavLen = 0;
		    }
		    fclose(FileHandler);
    	}
	    else
	    {
		    printf("Fail to open %s wav sample\r\n", TonePathNamePtr);
	    }
	}
	else
	{
		printf("Wav sample %s is not available\n", TonePathNamePtr);
	}
}
//--------------------------------------------------------------------------
