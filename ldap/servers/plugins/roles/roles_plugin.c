/** BEGIN COPYRIGHT BLOCK
 * Copyright (C) 2001 Sun Microsystems, Inc. Used by permission.
 * Copyright (C) 2005 Red Hat, Inc.
 * All rights reserved.
 * END COPYRIGHT BLOCK **/

/*
	Code to implement server roles features
*/

#include "slap.h"

#include "vattr_spi.h"

#include "roles_cache.h"
#include "statechange.h"


#ifdef SOURCEFILE 
#undef SOURCEFILE
#endif
#define SOURCEFILE "roles_plugin.c"
static char *sourcefile = SOURCEFILE;

#define STATECHANGE_ROLES_ID "Roles"
#define STATECHANGE_ROLES_CONFG_FILTER "objectclass=nsRoleDefinition"
#define STATECHANGE_ROLES_ENTRY_FILTER "objectclass=*"

#define ROLES_PLUGIN_SUBSYSTEM	"roles-plugin"	/* for logging */
static void * roles_plugin_identity = NULL;

static Slapi_PluginDesc pdesc = { "roles",
		PLUGIN_MAGIC_VENDOR_STR, PRODUCTTEXT, "roles plugin" };

#ifdef _WIN32
int *module_ldap_debug = 0;

void plugin_init_debug_level(int *level_ptr)
{
	module_ldap_debug = level_ptr;
}
#endif

static int roles_start( Slapi_PBlock *pb );
static int roles_post_op( Slapi_PBlock *pb );
static int roles_close( Slapi_PBlock *pb );
static void roles_set_plugin_identity(void * identity);

/* roles_init
   ----------
   Initialization of the plugin
 */
int roles_init( Slapi_PBlock *pb )
{
	int rc = 0;
	void *plugin_identity = NULL; 

	slapi_log_error( SLAPI_LOG_PLUGIN, ROLES_PLUGIN_SUBSYSTEM,
			"=> roles_init\n" );

	slapi_pblock_get (pb, SLAPI_PLUGIN_IDENTITY, &plugin_identity);
	PR_ASSERT (plugin_identity);
	roles_set_plugin_identity(plugin_identity);

    if (    slapi_pblock_set( pb, SLAPI_PLUGIN_VERSION,
					(void *)SLAPI_PLUGIN_VERSION_01 ) != 0 ||
			slapi_pblock_set( pb, SLAPI_PLUGIN_DESCRIPTION,
					(void *)&pdesc ) != 0 || 
			slapi_pblock_set( pb, SLAPI_PLUGIN_START_FN,
					(void *)roles_start ) != 0 ||
            slapi_pblock_set(pb, SLAPI_PLUGIN_POST_MODIFY_FN,
                     (void *) roles_post_op ) != 0 ||
            slapi_pblock_set(pb, SLAPI_PLUGIN_POST_MODRDN_FN,
                     (void *) roles_post_op ) != 0 ||
            slapi_pblock_set(pb, SLAPI_PLUGIN_POST_ADD_FN,
                     (void *) roles_post_op ) != 0 ||
            slapi_pblock_set(pb, SLAPI_PLUGIN_POST_DELETE_FN,
                     (void *) roles_post_op ) != 0 ||
            slapi_pblock_set(pb, SLAPI_PLUGIN_CLOSE_FN,
                     (void *) roles_close ) != 0 )
	{
		slapi_log_error( SLAPI_LOG_FATAL, ROLES_PLUGIN_SUBSYSTEM,
					"roles_init failed\n" );
		rc = -1;
    }

	slapi_log_error( SLAPI_LOG_PLUGIN, ROLES_PLUGIN_SUBSYSTEM,
			"<= roles_init %d\n", rc );
	return rc;
}

/* roles_start
   -----------
   kexcoff: cache build at init or at startup ? 
 */
static int roles_start( Slapi_PBlock *pb )
{
	int rc = 0;
	void **statechange_api;

	slapi_log_error( SLAPI_LOG_PLUGIN, ROLES_PLUGIN_SUBSYSTEM,
			"=> roles_start\n" );
	
	roles_cache_init();

    /* from Pete Rowley for vcache
     * PLUGIN DEPENDENCY ON STATECHANGE PLUGIN
     *
     * register objectclasses which indicate a
     * role configuration entry, and therefore
     * a globally significant change for the vcache
     */
 
    if(!slapi_apib_get_interface(StateChange_v1_0_GUID, &statechange_api))
    {
        statechange_register(statechange_api, STATECHANGE_ROLES_ID, NULL, STATECHANGE_ROLES_CONFG_FILTER, &vattr_global_invalidate, (notify_callback) statechange_vattr_cache_invalidator_callback(statechange_api));
    }

	slapi_log_error( SLAPI_LOG_PLUGIN, ROLES_PLUGIN_SUBSYSTEM,
			"<= roles_start %d\n", rc );
	return rc;
}

/* roles_close
   -----------
   kexcoff: ??
 */
static int roles_close( Slapi_PBlock *pb )
{
	int rc = 0;

	slapi_log_error( SLAPI_LOG_PLUGIN, ROLES_PLUGIN_SUBSYSTEM,
			"=> roles_close\n" );
	
	roles_cache_stop();

	slapi_log_error( SLAPI_LOG_PLUGIN, ROLES_PLUGIN_SUBSYSTEM,
			"<= roles_close %d\n", rc );
	return rc;
}

/* roles_sp_get_value
   ------------------
	Enumerate the values of the role attribute.
	We do this by first locating all the roles which are in scope
	Then we iterate over the in-scope roles calling Slapi_Role_Check(). 
	For those which pass the check, we add their DN to the attribute's value set.
*/
int roles_sp_get_value(vattr_sp_handle *handle, 
						vattr_context *c, 
						Slapi_Entry *e, 
						char *type, 
						Slapi_ValueSet** results,
						int *type_name_disposition, 
						char** actual_type_name, 
						int flags, 
						int *free_flags, 
						void *hint)
{
	int rc = -1;

	rc = roles_cache_listroles(e, 1, results);
    if (rc == 0) 
	{
		*free_flags = SLAPI_VIRTUALATTRS_RETURNED_COPIES;
		*actual_type_name = strdup(NSROLEATTR);

		if (type_name_disposition) 
		{
			*type_name_disposition = SLAPI_VIRTUALATTRS_TYPE_NAME_MATCHED_EXACTLY_OR_ALIAS; 
		}
	}

	/* Need to check the return code here because the caller 
		doesn't understand roles return codes */

	return rc;
}


/*	roles_sp_compare_value
    ----------------------
    Compare the value of the role attribute with a presented value.
	Return true or false to the client.
 */

int roles_sp_compare_value(vattr_sp_handle *handle, vattr_context *c, Slapi_Entry *e, char *type, Slapi_Value *test_this, int* result,int flags, void *hint)
{
	int rc = 0;
	Slapi_DN the_dn;

	/* Extract the role's DN from the value passed in */
	slapi_sdn_init_dn_byref(&the_dn,slapi_value_get_string(test_this));

	return (roles_check(e,&the_dn,result));
}

int roles_sp_list_types(vattr_sp_handle *handle,Slapi_Entry *e,vattr_type_list_context *type_context,int flags)
{
	static char* test_type_name = NSROLEATTR; 
	int ret =0;

	if ( 0 == ( flags & SLAPI_VIRTUALATTRS_LIST_OPERATIONAL_ATTRS )) {
		/*
		 * Operational attributes were NOT requested.  Since the only
		 * attribute type we service is nsRole which IS operational,
		 * there is nothing for us to do in this case.
		 */
		return 0;
	}

	ret = roles_cache_listroles(e, 0, NULL);
    if(ret == 0)
	{
	    vattr_type_thang thang = {0};
	    thang.type_name = test_type_name;
	    thang.type_flags = SLAPI_ATTR_FLAG_OPATTR;
	    slapi_vattrspi_add_type(type_context,&thang,SLAPI_VIRTUALATTRS_REQUEST_POINTERS);
	}
	return 0;
}

/* What do we do on shutdown ? */
int roles_sp_cleanup()
{
	return 0;
}

/* roles_post_op
    -----------
    Catch all for all post operations that change entries
    in some way - this simply notifies the cache of a
    change - the cache decides if action is necessary
*/
static int roles_post_op( Slapi_PBlock *pb )
{
    slapi_log_error( SLAPI_LOG_PLUGIN, ROLES_PLUGIN_SUBSYSTEM, "--> roles_post_op\n");

    roles_cache_change_notify(pb);

    slapi_log_error( SLAPI_LOG_PLUGIN, ROLES_PLUGIN_SUBSYSTEM, "<-- roles_post_op\n");
    return 0; /* always succeed */
}

static void roles_set_plugin_identity(void * identity)
{
    roles_plugin_identity=identity;
}

void * roles_get_plugin_identity()
{
    return roles_plugin_identity;
}

