# if ! defined( StatsAPIH )
#	define StatsAPIH	/* only include me once */

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

void ClientRegister(unsigned char ClientType, unsigned int ClientId);
void ClientDeregister(unsigned char ClientType, unsigned int ClientId);
void StatsRegister(unsigned char ClientType, unsigned int ClientId, unsigned short StatsCode);
void StatsDevRegister(unsigned char ClientType, unsigned int ClientId, 
    unsigned short DevId, unsigned short StatsCode);
void StatsInc(unsigned char ClientType, unsigned int ClientId, unsigned short StatsCode);
void StatsIncVal(unsigned char ClientType, unsigned int ClientId, unsigned short StatsCode, unsigned int StatsValue);
void StatsDevInc(unsigned char ClientType, unsigned int ClientId, unsigned short DevId, unsigned short StatsCode);
void StatsDevIncVal(unsigned char ClientType, unsigned int ClientId, 
    unsigned short DevId, unsigned short StatsCode, unsigned int StatsValue);
void StatsMaxVal(unsigned char ClientType, unsigned int ClientId, 
    unsigned short StatsCode, unsigned int StatsValue);
void StatsDevMaxVal(unsigned char ClientType, unsigned int ClientId, 
    unsigned short DevId, unsigned short StatsCode, unsigned int StatsValue);
void StatsDevAddVal(unsigned char ClientType, unsigned int ClientId, 
    unsigned short DevId, unsigned short StatsCode, unsigned int StatsValue);
void StatsDeregister(unsigned char ClientType, unsigned int ClientId);
void StatsDevDeregister(unsigned char ClientType, unsigned int ClientId, unsigned short DevId);
void SingleStatsDevDeregister(unsigned char ClientType, unsigned int ClientId, 
    unsigned short DevId, unsigned short StatsCode);
//---------------------------------------------------------------------------
#endif  /* if ! defined( StatsAPIH ) */
