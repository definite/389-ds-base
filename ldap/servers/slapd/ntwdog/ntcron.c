/** BEGIN COPYRIGHT BLOCK
 * Copyright (C) 2001 Sun Microsystems, Inc. Used by permission.
 * Copyright (C) 2005 Red Hat, Inc.
 * All rights reserved.
 * END COPYRIGHT BLOCK **/
//                                                                          //
//  Name: NTCRON                                                            //
//	 Platforms: WIN32                                                       //
//  Description: unix cron functionality in a separate thread               //
//  Notes:                                                                  //
//  The following assumptions are made:                                     //
//  - gszServerRoot is set to c:\netscape\server                            //
//  - ns-cron.conf and cron.conf are available                              //
//  Todo:                                                                   //
//  - handle time format variations of hh:mm                                //
//  - keep track of children                                                //
//  ......................................................................  //
//  Revision History:                                                       //
//  03-26-96  Initial Version, Andy Hakim (ahakim@netscape.com)             //
//  07-10-96  Modified for Directory Server, pkennedy@netscape.com			//
//--------------------------------------------------------------------------//
#include <windows.h>
#include "ntwatchdog.h"
#include "ntslapdmessages.h" // event log msgs constants //
#include "cron_conf.h"

static cron_conf_list *cclist   = NULL;


//--------------------------------------------------------------------------//
//                                                                          //
//--------------------------------------------------------------------------//
BOOL CRON_CheckDay(LPSYSTEMTIME lpstNow, char *szDays)
{
	BOOL bReturn = FALSE;
	char szToday[16];
	if(GetDateFormat((LCID)NULL, 0, lpstNow, "ddd", szToday, sizeof(szToday)) != 0)
	{
		strlwr(szDays);
		strlwr(szToday);
		if(strstr(szDays, szToday) != NULL)
			bReturn = TRUE;
	}
	return(bReturn);
}



//--------------------------------------------------------------------------//
//                                                                          //
//--------------------------------------------------------------------------//
BOOL CRON_CheckTime(LPSYSTEMTIME lpstNow, char *szTime)
{
	BOOL bReturn = FALSE;
	char szCurrentTime[16];
	char szStartTime[16];

	strcpy(szStartTime, szTime);

	if(szTime[1] == ':')
		wsprintf(szStartTime, "0%s", szTime);

	if(GetTimeFormat((LCID)LOCALE_SYSTEM_DEFAULT, TIME_FORCE24HOURFORMAT, lpstNow, "hh:mm", szCurrentTime, sizeof(szCurrentTime)) != 0)
	{
		if(strcmp(szCurrentTime, szStartTime) == 0)
			bReturn = TRUE;
	}
	return(bReturn);
}



//--------------------------------------------------------------------------//
//                                                                          //
//--------------------------------------------------------------------------//
BOOL CRON_StartJob(PROCESS_INFORMATION *pi, cron_conf_obj *cco)
{
	BOOL bReturn = FALSE;
	STARTUPINFO sui;

   sui.cb               = sizeof(STARTUPINFO);
   sui.lpReserved       = 0;
   sui.lpDesktop        = NULL;
   sui.lpTitle          = NULL;
   sui.dwX              = 0;
   sui.dwY              = 0;
   sui.dwXSize          = 0;
   sui.dwYSize          = 0;
   sui.dwXCountChars    = 0;
   sui.dwYCountChars    = 0;
   sui.dwFillAttribute  = 0;
   sui.dwFlags          = STARTF_USESHOWWINDOW;
   sui.wShowWindow      = SW_SHOWMINIMIZED;
   sui.cbReserved2      = 0;
   sui.lpReserved2      = 0;
   sui.hStdInput 			= 0;
   sui.hStdOutput 		= 0;
   sui.hStdError 			= 0;
	
	bReturn = CreateProcess(NULL, cco->command, NULL, NULL,
                   TRUE, 0, NULL, cco->dir, &sui, pi );
	if(!bReturn)
		WD_SysLog(EVENTLOG_ERROR_TYPE, MSG_CRON_STARTFAILED, cco->name);

	return(bReturn);
}


//--------------------------------------------------------------------------//
//                                                                          //
//--------------------------------------------------------------------------//
BOOL CRON_CheckConfFile()
{
	BOOL bReturn = FALSE;
	PROCESS_INFORMATION pi;
	SYSTEMTIME stNow;

	GetLocalTime(&stNow); // note: this provides time adjusted for local time zone
	
	if(cron_conf_read())
		cclist = cron_conf_get_list();

	while((cclist) && (cclist->obj))
	{
		cron_conf_obj *cco = cclist->obj;
	 	if((cco->days) && (cco->start_time) && (cco->command))
		{
			if(CRON_CheckDay(&stNow, cco->days) && CRON_CheckTime(&stNow, cco->start_time))
			{
				bReturn = CRON_StartJob(&pi, cco);
            CLOSEHANDLE(pi.hProcess);
            CLOSEHANDLE(pi.hThread);
			}
		}
		cclist = cclist->next;
	}
	cron_conf_free();
	return bReturn;
}



//--------------------------------------------------------------------------//
//                                                                          //
//--------------------------------------------------------------------------//
LPTHREAD_START_ROUTINE CRON_ThreadProc(HANDLE hevWatchDogExit)
{
	BOOL bExit = FALSE;
	while(!bExit)
	{
		CRON_CheckConfFile();
		if(WaitForSingleObject(hevWatchDogExit, 1000*DEFAULT_CRON_TIME) != WAIT_TIMEOUT)
			bExit = TRUE;
	}
	return 0;
}
