/*
*********************************************************************************
*                        C  M O D U L E  F I L E
*
*   Copyright MFBS, Inc. 2014-2015
*   The copyright notice above does not evidence any
*   actual or intended publication of such source code.
*   The code contains MFBS Confidential Restricted Information.
*
*********************************************************************************
*   FILE NAME       :    SampleCustomConfigDataBase.c
*   FUNCTION NAME   :    SampleCustomConfigDataBase
*   ORIGINATORS     :    Fedor Malakhov
*   DATE OF ORIGIN  :    12/05/2014
*   CALLING SEQUENCE:    Server customisation configuration .
*
*---------------------------------- PURPOSE ------------------------------------
* Function of HTTP page geration.
*
*--------------------------------- COMMAND SYNTAX ----------------------------
*---------------------------------- SYNOPSIS ----------------------------------
*
*       CAUTIONS:
*                none.
*
*       ASSUMPTIONS/PRECONDITIONS:
*               1) The parameters passed to the function have been parsed.
*
*       PARAMETER DESCRIPTION:
*
*               Inputs:
*
*               Returned by reference:
*                       none.
*
*               Return:
*                       none
*
*       POSTCONDITIONS:
*               none.
*
*-------------------------- GLOBAL DATA DESCRIPTION ----------------------------*
*--------------------------- PROJECT SPECIFIC DATA -----------------------------*
*       none.
*--------------------------- DESCRIPTION OF LOGIC ------------------------------*
* Embedded in the code.
*--------------------------------- REVISIONS -----------------------------------
* Date        Name         Prob#          Description
* ----------  -----------  -------------  --------------------------------------
* 12/05/2014  F. Malakhov  CCMPD01955995  Created initial version.
***********************************************************************************
*/
/*--------------------------- HEADER FILE INCLUDES ----------------------------*/
#include "SampleCustomConfigDataBase.h"
/*---------------------GLOBAL VARIABLES ---------------------------------------*/
/*-----------------------STATIC GLOBAL VARIABLES-------------------------------*/
/*----------------------- FUNCTION PROTOTYPE INCLUDES -------------------------*/
static void HandleSysShowHostIPAddr(void *ExtDataPtr, char *CmdLinePtr);
/*-------------------------------- CONSTANTS ----------------------------------*/
/*------------------------------- ENUMERATIONS --------------------------------*/
/*----------------------------- BASIC DATA TYPES ------------------------------*/
/*------------------------ STRUCTURE/UNION DATA TYPES -------------------------*/
/*------------------------------- STATIC DATA ---------------------------------*/
static DEF_EXT_CMD_HANDLER TablCustomCfgCmdDb[] = {
        "SysShowHostIPAddr",      HandleSysShowHostIPAddr      /* IP address of host were System status is shown */   
	};

#define CUSTOM_CFG_LEN sizeof(TablCustomCfgCmdDb)/sizeof(DEF_EXT_CMD_HANDLER)
/*---------------------------------- MACROS -----------------------------------*/
/*--------------------------- FUNCTION DEFINITION -----------------------------*/
/*-------------------------- LOCAL VARIABLES ----------------------------------*/
/*------------------------------- CODE ----------------------------------------*/
void SampleCustCfgInit(WEB_SERVER_CUST_CFG_INFO *CustCfgInfoPtr)
{
	SERVER_CUSTOM_CONFIG      *CustCfgPtr = NULL;
    SAMPLE_SERVER_CUSTOM_CONFIG *SampleCustCfgPtr = NULL;

    SampleCustCfgPtr = (SAMPLE_SERVER_CUSTOM_CONFIG*)CustCfgInfoPtr->ExtCustCfgPtr;
    memset(SampleCustCfgPtr->SysShowHostIPAddr, 0, MAX_LEN_IP_ADDR);    
    SampleCustCfgPtr->SysShowHostIP = 0;
}
//---------------------------------------------------------------------------
static void HandleSysShowHostIPAddr(void *ExtDataPtr, char *CmdLinePtr)
{
	int  pars_read, IpAddr1, IpAddr2, IpAddr3, IpAddr4;
    SAMPLE_SERVER_CUSTOM_CONFIG *CustCfgPtr = NULL;

    CustCfgPtr = (SAMPLE_SERVER_CUSTOM_CONFIG*)ExtDataPtr;    
    if (strlen(CmdLinePtr) > MAX_LEN_IP_ADDR) return;
	strncpy((char*)&CustCfgPtr->SysShowHostIPAddr[0],
        (const char*)CmdLinePtr, MAX_LEN_IP_ADDR);
    pars_read = sscanf(CmdLinePtr, "%u.%u.%u.%u", &IpAddr1, &IpAddr2, &IpAddr3, &IpAddr4);
    if (pars_read < 4) return;
    if ((IpAddr1 > 255) || (IpAddr2 > 255) || (IpAddr3 > 255) || (IpAddr4 > 255)) return;
#ifdef _SUN_BUILD_
    CustCfgPtr->SysShowHostIP = IpAddr4;       
    CustCfgPtr->SysShowHostIP |= (IpAddr3 << 8); 
    CustCfgPtr->SysShowHostIP |= (IpAddr2 << 16);
    CustCfgPtr->SysShowHostIP |= (IpAddr1 << 24);
#else
    CustCfgPtr->SysShowHostIP = IpAddr1;       
    CustCfgPtr->SysShowHostIP |= (IpAddr2 << 8); 
    CustCfgPtr->SysShowHostIP |= (IpAddr3 << 16);
    CustCfgPtr->SysShowHostIP |= (IpAddr4 << 24);
#endif
}
//---------------------------------------------------------------------------
