/** BEGIN COPYRIGHT BLOCK
 * Copyright (C) 2001 Sun Microsystems, Inc. Used by permission.
 * Copyright (C) 2005 Red Hat, Inc.
 * All rights reserved.
 * END COPYRIGHT BLOCK **/
/*
 * cb_debug.c - debugging-related code for Chaining backend
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "cb.h"

#ifdef _WIN32
int *module_ldap_debug = 0;

void plugin_init_debug_level(int *level_ptr)
{
        module_ldap_debug = level_ptr;
}
#endif

