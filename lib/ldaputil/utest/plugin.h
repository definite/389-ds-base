/** BEGIN COPYRIGHT BLOCK
 * Copyright (C) 2001 Sun Microsystems, Inc. Used by permission.
 * Copyright (C) 2005 Red Hat, Inc.
 * All rights reserved.
 * END COPYRIGHT BLOCK **/
#ifndef _CERTMAP_PLUGIN_H
#define _CERTMAP_PLUGIN_H

#ifdef __cplusplus
extern "C" {
#endif

extern int plugin_init_fn (void *certmap_info, const char *issuerName,
			   const char *issuerDN);

#ifdef __cplusplus
}
#endif

#endif /* _CERTMAP_PLUGIN_H */
