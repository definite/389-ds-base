/** BEGIN COPYRIGHT BLOCK
 * Copyright (C) 2001 Sun Microsystems, Inc. Used by permission.
 * Copyright (C) 2005 Red Hat, Inc.
 * All rights reserved.
 * END COPYRIGHT BLOCK **/
// EVENTLOG.H
//
// This file contains the defines that make NT an installable service.
//
// 1/12/95 aruna
//

// Functions in eventlog.c

#ifndef _EVENTLOG_H_
#define _EVENTLOG_H_

#include "netsite.h"


#if defined(XP_WIN32)

NSPR_BEGIN_EXTERN_C

NSAPI_PUBLIC HANDLE InitializeLogging(char *szEventLogName);
NSAPI_PUBLIC BOOL TerminateLogging(HANDLE hEventSource);
NSAPI_PUBLIC BOOL LogErrorEvent(HANDLE hEventSource, WORD fwEventType, WORD fwCategory, DWORD IDEvent, LPTSTR chMsg, LPTSTR lpszMsg);

NSPR_END_EXTERN_C

#endif /* XP_WIN32 */


#endif
