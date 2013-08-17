/** \file limo-app-mgr-lib.c
 * 
 * \brief Application Manager Library implementation for LiMo App Manager
 * 
 * \author Kaustubh Atrawalkar [kaustubh.atrawalkar@celunite.com]
 * 
 * The APIs to be used by the application developer are implemented here.
 */

#include <dbus/dbus.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-lowlevel.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/types.h>
#include <unistd.h>
#include "clp-app-mgr-lib.h"
#include "clp-app-mgr-config.h"
#include <xdgmime.h>							/* service discovery :APIs to get mime types based on file name and strings */
#include <dlfcn.h>
#include <gtk/gtk.h>
#include <app-manager.h>
#include <gconf/gconf-client.h>

#define LIMO_APPS_DIR				"/LiMo/System/AppInfo"
#define MAX_NO_OF_LINES				100
#define MAX_NO_OF_APPS_PER_MIME_TYPE		20

static int ClpAppMgrAppLaunchWithArgs (int app_id, void *app_model_data, int *inst_id, va_list va_args);
static int ClpAppMgrAppLaunchWithArgv (int app_id, void *app_model_data, int *inst_id, int argc, char **params);

#ifndef ENABLE_FREEZEMGR
int connect_to_restoredaemon() {return 0;}
int restore_app(char *appname) {return 0;}
int freezeme(char *appname) {return 0;} 
#endif

typedef struct _ClpAppMgrGlobalInfo	                		/**< structure for storing the global information of the application */
{
	gint 		pid;						/**< Process ID of the application */
	gint		app_id;						/**< ID of the Application */
	gint		inst_id;					/**< Instance Id of the Application */
	gchar 		*app_name;					/**< Name of the Application */
	gchar		*instance_name;					/**< Instance Name of the application */
	DBusConnection 	*bus_conn;              	                /**< global DBusConnection pointer*/
	gboolean	init_done;					/**< boolean to check if clp_app_mgr_init() is done or not*/
	app_stop 	stop_callback;					/**< function pointer for stop handler*/
	app_exec	exec_callback;					/**< function pointer for restore handler*/
	app_rotate	rotate_callback;				/**< function pointer for rotate handler*/
	app_death	death_callback;					/**< function pointer for restore handler*/
	app_focus_gained	app_focus_gained_callback;		/**< function pointer for app_focus_gained handler*/
	app_focus_lost	app_focus_lost_callback;			/**< function pointer for app_focus_lost handler*/
	app_message	message_callback;				/**< function pointer for app_messaged*/
	post_init	post_init_callback;				/**< function pointer for post_init handler*/
}ClpAppMgrGlobalInfo;

typedef struct _ClpAppMgrThemeInfo					/**< structure for storing the theme information */
{
	gchar 		theme[MAX_SIZE];				/**< name of the theme */
	gchar 		rcfile[2*MAX_SIZE];				/**< the rc file path of the theme */
}ClpAppMgrThemeInfo;

static ClpAppMgrGlobalInfo appclient_context;				/**< Instance of ClpAppMgrGlobalInfo to store information */
static gchar dbus_interface[MAX_SIZE] = CLP_APP_MGR_DBUS_INTERFACE;     /**< dbus Interface on which the application waits for signals */ 
static gchar dbus_object[MAX_SIZE] = CLP_APP_MGR_DBUS_OBJECT;           /**< dbus object path on which the application will be registered */

static DBusHandlerResult message_func (DBusConnection*, DBusMessage*, gpointer);
static GSList* read_theme_list(gchar *directory);


/** \brief Get the LIMO AMS dbus proxy 
 *
 * \param proxy Return value for DBusGProxy
 *
 * \return Returns TRUE for successfully getting the DBusGProxy. FALSE on error.
 *
 * Returns the DBusGProxy for the LIMO AMS. 
 */
static bool app_get_dbus_proxy(DBusGProxy **proxy)
{
	CLP_APPMGR_ENTER_FUNCTION();
	DBusGConnection *connection;
	GError *error = NULL;	

	g_type_init ();

	connection = dbus_g_bus_get (DBUS_BUS_SYSTEM, &error);
	if (connection == NULL)
	{
		CLP_APPMGR_WARN("Unable to get DBus System bus !");
		g_error_free (error);
		CLP_APPMGR_EXIT_FUNCTION();
		return false;
	}

	if (! ( *proxy = dbus_g_proxy_new_for_name (connection,	CLP_LIMO_AMS_DBUS_SERVICE, CLP_LIMO_AMS_DBUS_OBJECT, CLP_LIMO_AMS_DBUS_INTERFACE) ) )
	{
		CLP_APPMGR_WARN("Unable to get proxy !!");
		CLP_APPMGR_EXIT_FUNCTION();
		return false;
	}
	
	CLP_APPMGR_EXIT_FUNCTION();
	return true;
}


/** \brief Get the name of the application instance
 *
 * \return gchar * of the name of the application
 *
 * Returns name of the instance of application. Name will be used for inter-app communication.
 * Name string is internal pointer and shouldnt be freed. This shouldn't be used for execing
 */
gchar* clp_app_mgr_get_instance_name()
{
	CLP_APPMGR_ENTER_FUNCTION();
	return appclient_context.instance_name;
	CLP_APPMGR_EXIT_FUNCTION();
}


/** \brief Get the name of the application exec
 *
 * \return gchar * of the name of the application
 *
 * Returns name of the application. Name will be used for execing. It can set this name with Alarm manager etc for getting exec via middleware.
 * Name string is internal pointer and shouldnt be freed. This should be used for execing
 */
gchar* clp_app_mgr_get_name()
{
	CLP_APPMGR_ENTER_FUNCTION();
	return appclient_context.app_name;
	CLP_APPMGR_EXIT_FUNCTION();
}


/** \brief Registers the application with the Application Manager
 * 
 * \param name the name of the application to be registered
 * \param priority the priority of the application to be registered. Lower the value, higher the priority. 0 indicates highest priority, A number of bands of priority are placed in the clp-app-mgr-lib.h header
 * \param instance a boolean value with CLP_APP_MGR_SINGLE indicating single instance application and CLP_APP_MGR_MULTIPLE indicating Multiple instance application
 * 
 * \return CLP_APP_MGR_SUCCESS - Registration was successful
 * \return CLP_APP_MGR_FAILURE - Registration was unsuccessful
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 * 
 * 	Registers application with application manager.
 *  	Mandatory for all applications. (usually shielded by cel_app_new (g_object_new) of cel_app object.
 *  	The priority and instance params are only required, if registry doesnt have appropriate values.
 *  	Application becomes a schedulable entity and is active only after successful execution of this call
 * 
 */
gint 
clp_app_mgr_async_init (const gchar *name, const guint priority, const ClpAppMgrInstanceType instance, const post_init post_init_handler)
{
	CLP_APPMGR_ENTER_FUNCTION();
	CLP_APPMGR_PARAM_ERROR((name && (strcmp(name, ""))),"Parameter 'name' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(name) <= NAME_SIZE),"Parameter 'name' exceeds the maximum allowed name size");
	appclient_context.post_init_callback  = post_init_handler;
	clp_app_mgr_init(name, priority, instance);
	if(appclient_context.post_init_callback!=NULL) {
		(appclient_context.post_init_callback)(NULL);
	} else {
		CLP_APPMGR_INFO("could not call post init callback of clpapp!!");
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief Registers the application with the Application Manager
 * 
 * \param name the name of the application to be registered
 * \param priority the priority of the application to be registered. Lower the value, higher the priority. 0 indicates highest priority, A number of bands of priority are placed in the clp-app-mgr-lib.h header
 * \param instance a boolean value with CLP_APP_MGR_SINGLE indicating single instance application and CLP_APP_MGR_MULTIPLE indicating Multiple instance application
 * 
 * \return CLP_APP_MGR_SUCCESS - Registration was successful
 * \return CLP_APP_MGR_FAILURE - Registration was unsuccessful
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 * 
 * 	Registers application with application manager.
 *  	Mandatory for all applications. (usually shielded by cel_app_new (g_object_new) of cel_app object.
 *  	The priority and instance params are only required, if registry doesnt have appropriate values.
 *  	Application becomes a schedulable entity and is active only after successful execution of this call
 * 
 */
gint 
clp_app_mgr_init (const gchar *name, const guint priority, const ClpAppMgrInstanceType instance)
{
	CLP_APPMGR_ENTER_FUNCTION();
	CLP_APPMGR_PARAM_ERROR((name && (strcmp(name, ""))),"Parameter 'name' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(name) <= NAME_SIZE),"Parameter 'name' exceeds the maximum allowed name size");
	
	DBusError error;
//	gint return_code;
	g_type_init();
	amp_log_init();
	load_libsegfault ();
	dbus_error_init (&error);

	gchar **split = g_strsplit(name,".",2);
	appclient_context.app_name = g_strdup(split[0]);
	appclient_context.pid = getpid();
	g_strfreev(split);
/*	return_code = AppMgrAppGetCurrentId (&(appclient_context.app_id));
	if (return_code)
	{
		CLP_APPMGR_WARN_V("Error retreiving AppID from AppServer ! Error code - %d", return_code);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
	
	return_code = AppMgrAppGetCurrentInstId (&(appclient_context.inst_id));
	if (return_code)
	{
		CLP_APPMGR_WARN_V("Error retreiving InstID from AppServer ! Error code - %d", return_code);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
*/	g_strlcat(dbus_interface,".", MAX_SIZE);
	g_strlcat(dbus_interface, appclient_context.app_name, MAX_SIZE);
	g_strlcat(dbus_object,"/", MAX_SIZE);
	g_strlcat(dbus_object, appclient_context.app_name, MAX_SIZE);

	GConfClient *client = gconf_client_get_default();

	gchar *key_path = g_strconcat(GCONF_APPS_DIR,"/", appclient_context.app_name, "/info/PID", NULL);
	CLP_APPMGR_INFO_V("Writing PID to Key Path - %s\n", key_path);
	gconf_client_set_int (client, key_path, appclient_context.pid, NULL);
	g_free(key_path);
	
	key_path = g_strconcat (GCONF_APPS_DIR, "/", appclient_context.app_name, "/info/AppID", NULL);
	appclient_context.app_id = gconf_client_get_int(client, key_path, NULL);
	g_free(key_path);
	key_path = g_strconcat (GCONF_APPS_DIR, "/", appclient_context.app_name, "/LastInstId", NULL);
	appclient_context.inst_id = gconf_client_get_int(client, key_path, NULL);
	g_free(key_path);
	
	gchar appid[NAME_SIZE];
	sprintf(appid, "%d", appclient_context.app_id);
	key_path = g_strconcat(LIMO_APPS_DIR, "/", appid , "/AppMultiInstance", NULL);
	gboolean instance_type = gconf_client_get_bool(client, key_path, NULL);
	g_free(key_path);

	if(instance_type) {
		gchar instance_id[NAME_SIZE];
		sprintf(instance_id,"%d",appclient_context.inst_id);
		appclient_context.instance_name = g_strconcat(appclient_context.app_name,":",instance_id,NULL);
		CLP_APPMGR_INFO_V("Got Instance name as - %s", appclient_context.instance_name);
		g_strlcat(dbus_interface, instance_id, MAX_SIZE);
		g_strlcat(dbus_object, instance_id, MAX_SIZE);
	}
	else {
		CLP_APPMGR_INFO_V("The application %s supports single instance !", appclient_context.app_name);
		appclient_context.instance_name = g_strdup(appclient_context.app_name);
	}
	
	/* Get the System bus handle*/
	DBusConnection *connection = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
	if(!connection)
	{
		CLP_APPMGR_WARN("Failed to connect to D-Bus Daemon: !");
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_DBUS_CALL_FAIL;
	}
	
	appclient_context.bus_conn = connection;
	
 	CLP_APPMGR_INFO_V("DBUS Connection Opened : 0x%x", (guint)appclient_context.bus_conn);
	
	/* Concatanate the application name with default interface and object path*/
			
	CLP_APPMGR_INFO_V("Registering the Dbus Interface as %s Object Path as %s !",dbus_interface, dbus_object);

	appclient_context.stop_callback   = NULL;
	appclient_context.exec_callback   = NULL;
	appclient_context.rotate_callback = NULL;
	appclient_context.death_callback  = NULL;
	appclient_context.app_focus_gained_callback = NULL;
	appclient_context.app_focus_lost_callback = NULL;
	appclient_context.message_callback = NULL;
	appclient_context.init_done = TRUE;

	/* Add the signal match and signal filter for the application so that it receives
	 * the signals from Application Manager*/
	gchar match_str[MAX_SIZE] = "type='signal',interface='";
	g_strlcat(match_str, dbus_interface, MAX_SIZE);
	g_strlcat(match_str, "'", MAX_SIZE);
	dbus_bus_add_match (appclient_context.bus_conn, match_str, NULL);

	gchar match_str1[MAX_SIZE] =  "type='signal',interface='";
	g_strlcat(match_str1, CLP_APP_MGR_DBUS_INTERFACE, MAX_SIZE);
	g_strlcat(match_str1, "'", MAX_SIZE);
	dbus_bus_add_match (appclient_context.bus_conn, match_str1, NULL);

	gchar match_str2[MAX_SIZE] =  "type='signal',interface='";
	g_strlcat(match_str2, CLP_WIN_MGR_DBUS_INTERFACE, MAX_SIZE);
	g_strlcat(match_str2, "'", MAX_SIZE);
	dbus_bus_add_match (appclient_context.bus_conn, match_str2, NULL);

	dbus_connection_add_filter (appclient_context.bus_conn, message_func, NULL, NULL);
	CLP_APPMGR_INFO_V("Init Success (App:%s PID:%u)",appclient_context.app_name, appclient_context.pid);
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief Registers the application's exit callback function.
 *
 * \param app_death_callback callback for application exit signal handler
 * 
 * This API does the job of registering the callback functions for standard application death signals from Application Manager.
 */
void
clp_app_mgr_register_death_handler(const app_death app_death_callback)
{
	CLP_APPMGR_ENTER_FUNCTION();
	appclient_context.death_callback = app_death_callback;
	CLP_APPMGR_EXIT_FUNCTION();
	return;
}


/** \brief Registers the application stop callback function.
 *
 * \param callback callback for applist change signal handler
 * 
 * This API does the job of registering the callback functions for stoping applications from Application Manager.
 */
void
clp_app_mgr_register_stop_handler(const app_stop callback)
{
	CLP_APPMGR_ENTER_FUNCTION();
	appclient_context.stop_callback = callback;
	CLP_APPMGR_EXIT_FUNCTION();
	return;
}


/** \brief  Registers the application's rotate callback function.
 *
 * \param app_rotate_callback callback for rotate signal handler
 * 
 * This API does the job of registering the callback functions for rotation from Application Manager/Window Manager.
 * Resize of the window will be done automatically to screen co-ordinates.
 * This handler should do application specific repositioning (if needed). For Eq: a 3x2 grid maybe changed to fit orientation of screen
 */
void
clp_app_mgr_register_rotate_handler(const app_rotate app_rotate_callback)
{
	CLP_APPMGR_ENTER_FUNCTION();
	appclient_context.rotate_callback = app_rotate_callback;
	CLP_APPMGR_EXIT_FUNCTION();
	return;
}


/** \brief Registers the application's exec restore callback function.
 * 
 * \param exec_func callback for exec restore signal handler
 * 
 * Called whenever other application requests services of this application and the application is single instance application.
 * Service params are in the key value pair and in main argc argv way. 
 * Goptions library should be used to parse command line and act appropriately. 
 */
void
clp_app_mgr_register_exec_handler(const app_exec exec_func)
{
	CLP_APPMGR_ENTER_FUNCTION();
	appclient_context.exec_callback = exec_func;
	CLP_APPMGR_EXIT_FUNCTION();
	return;
}


/** \brief Get the ID of the application
 *
 * \param appname Name of the applications whose ID need to be retreive
 *
 * \return gint ID of the Application
 *
 * Returns ID of the application.
 */
static gint clp_app_mgr_get_app_id(const gchar *appname)
{
	CLP_APPMGR_ENTER_FUNCTION();
	CLP_APPMGR_PARAM_ERROR((appname && (strcmp(appname, ""))),"Parameter 'appname' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(appname) <= NAME_SIZE),"Parameter 'appname' exceeds the maximum allowed name size");
	gint app_id;
	GError *err = NULL;
	GConfClient *client = gconf_client_get_default();
	gchar *key_path = g_strconcat(GCONF_APPS_DIR, "/", appname, "/info/AppID", NULL);
	app_id = gconf_client_get_int (client, key_path, &err);
	CLP_APPMGR_INFO_V("Key Path - %s Value : %d\n", key_path, app_id);
	g_free(key_path);
	CLP_APPMGR_EXIT_FUNCTION();
	return app_id;
}


/** \brief Launches the application whose Name is passed as parameter
 *
 * \param application the name of the application to be execed from the caller application 
 *  
 * \return CLP_APP_MGR_SUCCESS - Application exec was successful.
 * \return CLP_APP_MGR_FAILURE - Application exec failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *  	   
 * This API can be used for launching the applications on request from other components or applications.
 * The destination application name will be followed by {name value } pairs and truncated by NULL
 * If application doesnt exist failure will be returned
 * This function will be wrapped by CelApp.
 */
gint clp_app_mgr_exec(const gchar *application, ...)
{
	CLP_APPMGR_ENTER_FUNCTION();
	CLP_APPMGR_PARAM_ERROR((application && (strcmp(application, ""))),"Parameter 'application' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(application) <= NAME_SIZE),"Parameter 'application' exceeds the maximum allowed name size");
	va_list args;
	gchar *value;
	gint return_code, inst_id, app_id;
	gint no_of_params = 1;
	gchar *params[NAME_SIZE];
	app_id = clp_app_mgr_get_app_id(application);

	// calls the exec with params and the parameters to be passed are taken from the service and argc,argv format.
	va_start (args, application);
	return_code = ClpAppMgrAppLaunchWithArgs (app_id, NULL , &inst_id, args);
	
	value = va_arg(args, char*);
	if(return_code == APPMGR_ERROR_APP_ALREADY_RUNNING)
	{
		DBusMessageIter iter, array_iter;
		gchar 		array_sig[2];
		gint 		i;
		DBusError 	error;
		array_sig[0] = DBUS_TYPE_STRING;
		array_sig[1] = '\0';
		
		gchar *app_interface = g_strconcat (CLP_APP_MGR_DBUS_INTERFACE,".", application, NULL);
		gchar *app_objectpath =	g_strconcat (CLP_APP_MGR_DBUS_OBJECT, "/", application, NULL);
		
		while (value) {
			params[no_of_params] = g_strdup(value);
			no_of_params++;
			value = va_arg (args, char*);
		}

		CLP_APPMGR_INFO_V("Restore ( Application : %s(%d), ObjectPath : %s, Interface: %s Num of Params : %d)", application, app_id, app_objectpath, app_interface, no_of_params);
		dbus_error_init (&error);
		
		DBusConnection *bus_conn = dbus_bus_get (DBUS_BUS_SYSTEM, &error);

		DBusMessage *msg = dbus_message_new_signal (app_objectpath, app_interface, CLP_APP_MGR_DBUS_SIGNAL_EXEC);
		if(msg == NULL)
        	{
			CLP_APPMGR_WARN("Not Enough Memory to create new dbus Message");
                	CLP_APPMGR_EXIT_FUNCTION();
			return CLP_APP_MGR_DBUS_CALL_FAIL;
		}
	
		dbus_message_iter_init_append(msg, &iter);
		dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &no_of_params);
		dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, array_sig, &array_iter);
		dbus_message_iter_append_basic (&array_iter, DBUS_TYPE_STRING, &application);
		
		for(i=1; i<no_of_params; i++) {
			CLP_APPMGR_INFO_V("Restore ( Param %u : %s )",i, params[i]);
			dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &params[i]);
		}
		dbus_message_iter_close_container(&iter, &array_iter);

		if (!dbus_connection_send(bus_conn, msg, 0)) 
		{
			CLP_APPMGR_WARN("Out Of Memory!");
			return CLP_APP_MGR_OUT_OF_MEMORY;
		}
	
		g_free(app_interface);
		g_free(app_objectpath);	
		dbus_connection_flush(bus_conn);
		dbus_message_unref(msg);
	}
	else if(return_code!=0||inst_id<=0)
        {
                CLP_APPMGR_WARN_V("Launching application[%d] failed !! Error_Code :%d", inst_id, return_code);
		va_end(args);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
        }
	va_end(args);
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief Limo AMS implementation for LunchWithArgs
 *
 * \param app_id AppID of the application to be launched.
 * \param app_model_data AppModel with which the application to be launched.
 * \param inst_id Return value of the inst id assigned to launched application.
 * \param va_args VA_LIST for parameters to be passed with application.
 *
 * \return ERROR_CODE (LIMO error_codes). 0 on successfully launching the application.
 */
static int ClpAppMgrAppLaunchWithArgs (int app_id, void *app_model_data, int *inst_id, va_list va_args)
{
	CLP_APPMGR_ENTER_FUNCTION();
	DBusGProxy *proxy;
	GError *error = NULL;
	int error_code = -1;
	char *args = NULL;
	char *value;
	char delim[2];
	delim[0] = 16;
	delim[1] ='\0';

	GConfClient *client = gconf_client_get_default();
	gboolean shutdown = gconf_client_get_bool(client,"/appmgr/Shutdown",NULL);
	if (shutdown)
		return -1;
	g_object_unref(G_OBJECT(client));
	
	if ( inst_id == NULL)
	{
		CLP_APPMGR_WARN("Inst_ID pointer is NULL !!");
		CLP_APPMGR_EXIT_FUNCTION();
		return -8;
	}

	value = va_arg(va_args, char*);
	if(value) {
		args = g_strdup(value);
		
		value = va_arg(va_args, char*);
		while(value) {
			args = g_strconcat(args, delim, value, NULL);
			value = va_arg(va_args, char*);
		}
	}
	
	if ( !app_get_dbus_proxy(&proxy))
	{
		CLP_APPMGR_WARN("Unable to get LIMO AMS dbus proxy !");
		CLP_APPMGR_EXIT_FUNCTION();
		return APPMGR_ERROR_INTERNAL_TRANSPORT_ERROR;
	}
	
	if (!dbus_g_proxy_call (proxy, "app_launch_call",&error,
				G_TYPE_INT, app_id,
				G_TYPE_STRING,args,
				G_TYPE_UINT, app_model_data,
				G_TYPE_INVALID,
				G_TYPE_INT, inst_id,
                		G_TYPE_INT, &error_code,
				G_TYPE_INVALID))
	{
		CLP_APPMGR_WARN("Unable to make proxy call !");
		error_code = APPMGR_ERROR_INTERNAL_TRANSPORT_ERROR;
		g_object_unref (proxy);
		CLP_APPMGR_EXIT_FUNCTION();
		return error_code;
	}
        
	g_object_unref (proxy);
	
	if (0 == error_code)	
	{
		CLP_APPMGR_INFO_V("Application (AppID - %d) launched successfully.",app_id);
		CLP_APPMGR_ENTER_FUNCTION();
		return 0;
	}
	else
	{
		CLP_APPMGR_INFO_V("Unable to launch application (AppID - %d) Error code - %d!", app_id, error_code);
		CLP_APPMGR_EXIT_FUNCTION();
		return error_code;
	}
}


/** \brief Launches the application whose Name is passed as parameter
 *
 * \param application the name of the application to be execed from the caller application 
 * \param old_ap variable argument list of parameters to be supplied to the application
 *  
 * \return CLP_APP_MGR_SUCCESS - Application exec was successful.
 * \return CLP_APP_MGR_FAILURE - Application exec failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *  	   
 * This API can be used for launching the applications on request from other components.
 * If application doesnt exist failure will be returned
 */
gint 
clp_app_mgr_exec_application (const gchar *application, const va_list old_ap)
{
	CLP_APPMGR_ENTER_FUNCTION();
	CLP_APPMGR_PARAM_ERROR((application && (strcmp(application, ""))),"Parameter 'application' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(application) <= NAME_SIZE),"Parameter 'application' exceeds the maximum allowed name size");
	gchar *value;
	gint return_code, inst_id, app_id;
	gint no_of_params = 1;
	gchar *params[NAME_SIZE];

	app_id = clp_app_mgr_get_app_id(application);

	// calls the exec with params and the parameters to be passed are taken from the service and argc,argv format.
	return_code = ClpAppMgrAppLaunchWithArgs (app_id, NULL , &inst_id, old_ap);
	value = va_arg(old_ap, char*);
	if(return_code == APPMGR_ERROR_APP_ALREADY_RUNNING)
	{
		DBusMessageIter iter, array_iter;
		gchar 		array_sig[2];
		gint 		i;
		DBusError 	error;
		array_sig[0] = DBUS_TYPE_STRING;
		array_sig[1] = '\0';

		gchar *app_interface = g_strconcat (CLP_APP_MGR_DBUS_INTERFACE,".", application, NULL);
		gchar *app_objectpath =	g_strconcat (CLP_APP_MGR_DBUS_OBJECT, "/", application, NULL);

		while (value) {
			params[no_of_params] = g_strdup(value);
			no_of_params++;
			value = va_arg (old_ap, char*);
		}

		CLP_APPMGR_INFO_V("Restore ( Application : %s(%d), ObjectPath : %s, Interface: %s Num of Params : %d)", application, app_id, app_objectpath, app_interface, no_of_params);
		dbus_error_init (&error);
		
		DBusConnection *bus_conn = dbus_bus_get (DBUS_BUS_SYSTEM, &error);

		DBusMessage *msg = dbus_message_new_signal (app_objectpath, app_interface, CLP_APP_MGR_DBUS_SIGNAL_EXEC);
		if(msg == NULL)
        	{
			CLP_APPMGR_WARN("Not Enough Memory to create new dbus Message");
                	CLP_APPMGR_EXIT_FUNCTION();
			return CLP_APP_MGR_DBUS_CALL_FAIL;
		}
	
		dbus_message_iter_init_append(msg, &iter);
		dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &no_of_params);
		dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, array_sig, &array_iter);
		dbus_message_iter_append_basic (&array_iter, DBUS_TYPE_STRING, &application);
		
		for(i=1; i<no_of_params; i++) {
			CLP_APPMGR_INFO_V("Restore ( Param %u : %s )",i, params[i]);
			dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &params[i]);
		}
		dbus_message_iter_close_container(&iter, &array_iter);

		if (!dbus_connection_send(bus_conn, msg, 0))
		{
			CLP_APPMGR_WARN("Out Of Memory!");
			return CLP_APP_MGR_OUT_OF_MEMORY;			
		}
		
		g_free(app_interface);
		g_free(app_objectpath);
		dbus_connection_flush(bus_conn);
		dbus_message_unref(msg);
	}
	else if(return_code!=0||inst_id<=0)
        {
		CLP_APPMGR_WARN_V("Launching application[%d] failed !! Error_Code :%d", inst_id, return_code);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
        }
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief Limo AMS implementation for LunchWithArgv
 *
 * \param app_id AppID of the application to be launched.
 * \param app_model_data AppModel with which the application to be launched.
 * \param inst_id Return value of the inst id assigned to launched application.
 * \param params List of parameters to be passed with application.
 *
 * \return ERROR_CODE (LIMO error_codes). 0 on successfully launching the application.
 */
static int ClpAppMgrAppLaunchWithArgv (int app_id, void *app_model_data, int *inst_id, int argc, char **params)
{
	CLP_APPMGR_ENTER_FUNCTION();
	DBusGProxy *proxy;
	GError *error = NULL;
	int error_code = -1;
	char *args = "";
	int i = 0;
	char delim[2];
	delim[0] = 16;
	delim[1] = '\0';

	GConfClient *client = gconf_client_get_default();
	gboolean shutdown = gconf_client_get_bool(client,"/appmgr/Shutdown",NULL);
	if (shutdown)
		return -1;
	g_object_unref(G_OBJECT(client));

	if ( inst_id == NULL)
	{
		CLP_APPMGR_WARN("Inst_ID pointer is NULL !!");
		CLP_APPMGR_EXIT_FUNCTION();
		return -8;
	}
	
	while(argc>0) {
		args = g_strconcat(args,delim, params[i], NULL);
		i++;
		argc--;
	}
	
	if ( !app_get_dbus_proxy(&proxy))
	{
		CLP_APPMGR_WARN("Unable to get LIMO AMS dbus proxy !");
		CLP_APPMGR_EXIT_FUNCTION();
		return APPMGR_ERROR_INTERNAL_TRANSPORT_ERROR;
	}
	
if (!dbus_g_proxy_call (proxy, "app_launch_call",&error,
				G_TYPE_INT, app_id,
				G_TYPE_STRING,args,
				G_TYPE_UINT, app_model_data,
				G_TYPE_INVALID,
				G_TYPE_INT, inst_id,
                		G_TYPE_INT, &error_code,
				G_TYPE_INVALID))
	{
		CLP_APPMGR_WARN("Unable to make proxy call !");
		error_code = APPMGR_ERROR_INTERNAL_TRANSPORT_ERROR;
		g_object_unref (proxy);
		CLP_APPMGR_EXIT_FUNCTION();
		return error_code;
	}
        
	g_object_unref (proxy);
	
	if (0 == error_code)
	{
		CLP_APPMGR_INFO_V("Application (AppID - %d) launched successfully.",app_id);
		CLP_APPMGR_EXIT_FUNCTION();
		return 0;
	}
	else
	{
		CLP_APPMGR_INFO_V("Unable to launch application (AppID - %d) Error code - %d!", app_id, error_code);
		CLP_APPMGR_EXIT_FUNCTION();
		return error_code;
	}
}


/** \brief Launches the application with arguments passed in argc argv format
 *
 * \param application the name of the application to be execed from the caller application 
 * \param no_of_params no of arguments the application takes
 * \param params_list arguments list which application takes
 *  
 * \return CLP_APP_MGR_SUCCESS - Application exec was successful.
 * \return CLP_APP_MGR_FAILURE - Application exec failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *  	   
 * This API can be used for launching the applications on request from other components.
 * If application doesnt exist failure will be returned
 */
gint 
clp_app_mgr_exec_argv (const gchar *application, gint no_of_params, gchar** params_list)
{
	CLP_APPMGR_ENTER_FUNCTION();
	CLP_APPMGR_PARAM_ERROR((application && (strcmp(application, ""))),"Parameter 'application' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(application) <= NAME_SIZE),"Parameter 'application' exceeds the maximum allowed name size");
	gint return_code, inst_id, app_id;

	app_id = clp_app_mgr_get_app_id(application);

	// calls the exec with params and the parameters to be passed are taken from the service and argc,argv format.
	return_code = ClpAppMgrAppLaunchWithArgv (app_id, NULL , &inst_id, no_of_params, params_list);
	
	if(return_code == APPMGR_ERROR_APP_ALREADY_RUNNING)
	{
		DBusMessageIter iter, array_iter;
		gchar 		array_sig[2];
		gint 		i;
		DBusError 	error;
		array_sig[0] = DBUS_TYPE_STRING;
		array_sig[1] = '\0';
		
		gchar *app_interface = g_strconcat (CLP_APP_MGR_DBUS_INTERFACE,".", application, NULL);
		gchar *app_objectpath =	g_strconcat (CLP_APP_MGR_DBUS_OBJECT, "/", application, NULL);

		CLP_APPMGR_INFO_V("Restore ( Application : %s(%d), ObjectPath : %s, Interface: %s Num of Params : %d)", application, app_id, app_objectpath, app_interface, no_of_params);
		dbus_error_init (&error);
		DBusConnection *bus_conn = dbus_bus_get (DBUS_BUS_SYSTEM, &error);
		DBusMessage *msg = dbus_message_new_signal (app_objectpath, app_interface, CLP_APP_MGR_DBUS_SIGNAL_EXEC);
		if(msg == NULL)
        	{
			CLP_APPMGR_WARN("Not Enough Memory to create new dbus Message");
                	CLP_APPMGR_EXIT_FUNCTION();
			return CLP_APP_MGR_DBUS_CALL_FAIL;
		}
		no_of_params++;
		dbus_message_iter_init_append(msg, &iter);
		dbus_message_iter_append_basic(&iter, DBUS_TYPE_UINT32, &no_of_params);
		dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, array_sig, &array_iter);
		dbus_message_iter_append_basic (&array_iter, DBUS_TYPE_STRING, &application);
		no_of_params--;
		
		for(i=0; i<no_of_params; i++) {
			CLP_APPMGR_INFO_V("Restore ( Param %u : %s )",i, params_list[i]);
			dbus_message_iter_append_basic(&array_iter, DBUS_TYPE_STRING, &params_list[i]);
		}
		dbus_message_iter_close_container(&iter, &array_iter);

		if (!dbus_connection_send(bus_conn, msg, 0))
		{
			CLP_APPMGR_WARN("Out Of Memory!");
			return CLP_APP_MGR_OUT_OF_MEMORY;
		}
	
		g_free(app_interface);
		g_free(app_objectpath);	
		dbus_connection_flush(bus_conn);
		dbus_message_unref(msg);
	}

	else if(return_code!=0||inst_id<=0)
        {
		CLP_APPMGR_WARN_V("Launching application[%d] failed !! Error_Code :%d", inst_id, return_code);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
        }
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief API to rotate the application window by 90 degress
 *
 * \param rotationtype  An enum; CLOCKWISE indicates rotate in clockwise direction and ANTICLOCKWISE indicates rotate in anticlockwise direction
 * 
 * This API provides the support for rotating the window of the application by 90 degrees in clockwise or anticlockwise direction. 
 * The screen will rotate for all applications. 
 * No application specific rotation is provided for this release.
 * As a result of this call, all applications receive a signal "rotate" and their rotate handler is called
 */
gint 
clp_app_mgr_rotate(const ClpAppMgrRotationType rotationtype)
{
	CLP_APPMGR_ENTER_FUNCTION();
	//TODO;
	CLP_APPMGR_EXIT_FUNCTION();
	return 0;
}


/** \brief API to stop the currently active application
 *
 * \param app the name of the application to be stopped
 * 
 * \return CLP_APP_MGR_SUCCESS - Application stopped successfully
 * \return CLP_APP_MGR_FAILURE - Application could not be stopped.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory.
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * This API can be used to stop the current active application. 
 * The stop callback will be called of the stopped application.
 * The application will receive Signal "stop".
 */
gint 
clp_app_mgr_stop(const gchar *app)
{
	/* restore the application to the display. send the restore signal */
	CLP_APPMGR_ENTER_FUNCTION();
	
	CLP_APPMGR_PARAM_ERROR((app && (strcmp(app, ""))),"Parameter 'app' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(app) <= NAME_SIZE),"Parameter 'app' exceeds the maximum allowed name size");
	DBusError 	error;
		
	gchar **split = g_strsplit(app,":",2);
	gchar *app_interface = g_strconcat (CLP_APP_MGR_DBUS_INTERFACE,".", split[0], split[1], NULL);
	gchar *app_objectpath =	g_strconcat (CLP_APP_MGR_DBUS_OBJECT, "/", split[0], split[1], NULL);
	
	CLP_APPMGR_INFO_V("Sending STOP Signal ( Application : %s, ObjectPath : %s, Interface: %s)", app, app_objectpath, app_interface);
	dbus_error_init (&error);
		
	DBusConnection *bus_conn = dbus_bus_get (DBUS_BUS_SYSTEM, &error);

	DBusMessage *msg = dbus_message_new_signal (app_objectpath, app_interface, CLP_APP_MGR_DBUS_SIGNAL_STOP);
	if(msg == NULL)
       	{
		CLP_APPMGR_WARN("Not Enough Memory to create new dbus Message");
               	CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_DBUS_CALL_FAIL;
	}
	if (!dbus_connection_send(bus_conn, msg, 0)) 
	{
		CLP_APPMGR_WARN("Out Of Memory!");
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}

	g_free(app_interface);
	g_free(app_objectpath);
	dbus_connection_flush(bus_conn);
	dbus_message_unref(msg);
	return CLP_APP_MGR_SUCCESS;
}


/** \brief API to restore the minimized application window
 *
 * \param app the name of the application whose window has to be restored
 * 
 * \return CLP_APP_MGR_SUCCESS - Application restored
 * \return CLP_APP_MGR_FAILURE - Application could not be resoterd.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory.
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * This API can be used to restore the minimized application. This API is to be used by the switcher 
 * applet for switching between the minimized applications.
 * No callback is called in the application.
 * The application will receive GSignal "user_attention_gained" and "widget_focus_gained"
 */
gint 
clp_app_mgr_restore(const gchar *app)
{
	CLP_APPMGR_ENTER_FUNCTION();
	CLP_APPMGR_PARAM_ERROR((app && (strcmp(app, ""))),"Parameter 'app' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(app) <= NAME_SIZE),"Parameter 'app' exceeds the maximum allowed name size");
	DBusMessage *msg;
  	DBusMessageIter args;
	int pid, return_code, appid;
	int instid = atoi(app);
	return_code = AppMgrAppGetInstInfo(instid, &appid, &pid);

	msg = dbus_message_new_method_call (CLP_WIN_MGR_DBUS_SERVICE, CLP_WIN_MGR_DBUS_OBJECT, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_FOCUS_PID_METHOD);
	if (msg == NULL)
       	{ 
	      	CLP_APPMGR_WARN("Message Null");
		CLP_APPMGR_EXIT_FUNCTION();
	      	return FALSE;
	}
	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &pid))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return FALSE;
	}
	if (!dbus_connection_send(appclient_context.bus_conn, msg, 0))
	{
		CLP_APPMGR_WARN("Out Of Memory!");
		CLP_APPMGR_EXIT_FUNCTION();
		return FALSE;
	}
	CLP_APPMGR_EXIT_FUNCTION();
	return TRUE;
}


/** \brief API to close the application by name for red key press
 * 
 * \return CLP_APP_MGR_SUCCESS - Application close successful.
 * \return CLP_APP_MGR_FAILURE - Error in shutting down the application
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory.
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 * 
 * This API can be used to close the application by name cleanly when red key is pressed. 
 * It calls The AppCloseName method exported by the Application manager.
 * The AppManager then removes the application information from the list maintained by it.
 */
gint 
clp_app_mgr_close_by_red_key(const gchar *app)
{
	/* restore the application to the display. send the restore signal */
	CLP_APPMGR_ENTER_FUNCTION();
	
	CLP_APPMGR_PARAM_ERROR((app && (strcmp(app, ""))),"Parameter 'app' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(app) <= NAME_SIZE),"Parameter 'app' exceeds the maximum allowed name size");

	gchar *flag = clp_app_mgr_get_property(app, "X-RedKeyKill");
	CLP_APPMGR_INFO_V("Got redkeykill property for %s as %s", app, flag);
	if (!g_strcmp0 (flag, "true") || !g_strcmp0 (flag, "TRUE")) {
	
	gint inst_id, return_code;
	GError *err = NULL;
	GConfClient *client = gconf_client_get_default();
	gchar *key_path = g_strconcat(GCONF_APPS_DIR, "/", app, "/LastInstId", NULL);
	inst_id = gconf_client_get_int (client, key_path, &err);
	CLP_APPMGR_INFO_V("Key Path - %s Inst ID : %d\n", key_path, inst_id);
	g_free(key_path);

	DBusMessage *msg;
	gchar dbusinterface[MAX_SIZE] = CLP_APP_MGR_DBUS_INTERFACE;     /**< dbus Interface on which the application waits for signals */ 
	gchar dbusobject[MAX_SIZE] = CLP_APP_MGR_DBUS_OBJECT;           /**< dbus object path on which the application will be registered */
		
	g_strlcat(dbusinterface, ".", MAX_SIZE);
	g_strlcat(dbusinterface, app, MAX_SIZE);
	g_strlcat(dbusobject, "/", MAX_SIZE);
	g_strlcat(dbusobject, app, MAX_SIZE);
	
	msg = dbus_message_new_signal (dbusobject, dbusinterface, CLP_APP_MGR_DBUS_SIGNAL_STOP);
     	 
	if (NULL == msg)
       	{ 
	 	CLP_APPMGR_WARN("Message Null");
  		CLP_APPMGR_EXIT_FUNCTION();
	    	return CLP_APP_MGR_DBUS_CALL_FAIL;
	}

	dbus_connection_send (appclient_context.bus_conn, msg, NULL);
	dbus_message_unref(msg);
	sleep(2);
	return_code = AppMgrAppKill (inst_id);
	if(return_code) {
		CLP_APPMGR_INFO_V("Unable to kill application %s (inst id %d)- error code : %d !!", app, inst_id, return_code);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
	}
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief API to close the application by name
 * 
 * \return CLP_APP_MGR_SUCCESS - Application close successful.
 * \return CLP_APP_MGR_FAILURE - Error in shutting down the application
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory.
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 * 
 * This API can be used to close the application by name cleanly. It calls The AppCloseName method exported by the Application manager.
 * The AppManager then removes the application information from the list maintained by it.
 */
gint 
clp_app_mgr_close_by_name(const gchar *app)
{
	/* restore the application to the display. send the restore signal */
	CLP_APPMGR_ENTER_FUNCTION();
	
	CLP_APPMGR_PARAM_ERROR((app && (strcmp(app, ""))),"Parameter 'app' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(app) <= NAME_SIZE),"Parameter 'app' exceeds the maximum allowed name size");

	gint inst_id, return_code;
	GError *err = NULL;
	GConfClient *client = gconf_client_get_default();
	gchar *key_path = g_strconcat(GCONF_APPS_DIR, "/", app, "/LastInstId", NULL);
	inst_id = gconf_client_get_int (client, key_path, &err);
	CLP_APPMGR_INFO_V("Key Path - %s Inst ID : %d\n", key_path, inst_id);
	g_free(key_path);

	DBusMessage *msg;
	gchar dbusinterface[MAX_SIZE] = CLP_APP_MGR_DBUS_INTERFACE;     /**< dbus Interface on which the application waits for signals */ 
	gchar dbusobject[MAX_SIZE] = CLP_APP_MGR_DBUS_OBJECT;           /**< dbus object path on which the application will be registered */
		
	g_strlcat(dbusinterface, ".", MAX_SIZE);
	g_strlcat(dbusinterface, app, MAX_SIZE);
	g_strlcat(dbusobject, "/", MAX_SIZE);
	g_strlcat(dbusobject, app, MAX_SIZE);
	
	msg = dbus_message_new_signal (dbusobject, dbusinterface, CLP_APP_MGR_DBUS_SIGNAL_STOP);
     	 
	if (NULL == msg)
       	{ 
	 	CLP_APPMGR_WARN("Message Null");
  		CLP_APPMGR_EXIT_FUNCTION();
	    	return CLP_APP_MGR_DBUS_CALL_FAIL;
	}

	dbus_connection_send (appclient_context.bus_conn, msg, NULL);
	dbus_message_unref(msg);
	sleep(2);

	return_code = AppMgrAppKill (inst_id);
	if(return_code) {
		CLP_APPMGR_INFO_V("Unable to kill application %s (inst id %d)- error code : %d !!", app, inst_id, return_code);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief API to close the application
 * 
 * \return CLP_APP_MGR_SUCCESS - Application close successful.
 * \return CLP_APP_MGR_FAILURE - Error in shutting down the application
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory.
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 * 
 * This API can be used to close the application cleanly. It is expected that this API is called before the application quits.
 * It calls The AppClose method exported by the Application manager. The AppManager then removes the application information from the 
 * list maintained by it.
 * This function is called from inside cel_app_stop
 */
gint 
clp_app_mgr_close(void)
{
	CLP_APPMGR_ENTER_FUNCTION();

	CLP_APPMGR_INFO_V("Application %s(%u) - Instance ID - %d - Shutting Down...",appclient_context.instance_name, getpid(), appclient_context.inst_id);
	gint return_code = AppMgrAppKill (appclient_context.inst_id);
	if(return_code) {
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief function for handling dbus signals and calling corresponding callback functions.
 * 
 * \param bus_conn the DBusConnection pointer 
 * \param msg the DBusMessage pointer
 * \param user_data the gpointer to send user data if any
 * 
 * \return  DBusHandlerResult is returned indicating whether the signal was handled or not
 * 
 * \warning This function is internal to the Library
 * 
 * This is an internal function that is passed to dbus daemon to handle the signals that the application receives.
 */
static DBusHandlerResult 
message_func (DBusConnection *bus_conn, DBusMessage *msg, gpointer user_data)
{
	CLP_APPMGR_ENTER_FUNCTION();
	CLP_APPMGR_INFO_V("Signal Received %s %s, Sender : %s", dbus_message_get_interface(msg), dbus_message_get_member(msg), dbus_message_get_sender(msg));

	/* Signal handler function*/
	if (dbus_message_is_signal (msg, dbus_interface, CLP_APP_MGR_DBUS_SIGNAL_STOP))
	{
		/* handle the stop signal...redirect the application's stop handler*/
		if(appclient_context.stop_callback!=NULL)
		{
			(appclient_context.stop_callback) (NULL);
		}
	}
	if (dbus_message_is_signal (msg, CLP_APP_MGR_DBUS_INTERFACE, CLP_APP_MGR_DBUS_SIGNAL_STOP))
	{
		/* handle the stop signal...redirect the application's stop handler*/
		if(appclient_context.stop_callback!=NULL)
		{
			(appclient_context.stop_callback) (NULL);
		}
	}
	else if (dbus_message_is_signal (msg, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_DBUS_SIGNAL_UA_GAINED))
	{
		/* handle the focus_gained signal... redirect the application's focus_gained handler */
		gint pid;
	        DBusMessageIter iter;
	        dbus_message_iter_init(msg, &iter);
	        dbus_message_iter_get_basic(&iter, &pid);
		if (pid == getpid()) {

			if(appclient_context.app_focus_gained_callback!=NULL)
			{
				(appclient_context.app_focus_gained_callback) (NULL);
			}
		}
	}
	else if (dbus_message_is_signal (msg, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_DBUS_SIGNAL_UA_LOST))
	{
		/* handle the focus_lost signal... redirect the application's focus_lost handler */
		gint pid;
	        DBusMessageIter iter;
	        dbus_message_iter_init(msg, &iter);
	        dbus_message_iter_get_basic(&iter, &pid);
		if (pid == getpid()) {

			if(appclient_context.app_focus_lost_callback!=NULL)
			{
				(appclient_context.app_focus_lost_callback) (NULL);
			}
		}
	}
	else if (dbus_message_is_signal (msg, CLP_APP_MGR_DBUS_INTERFACE, CLP_APP_MGR_DBUS_SIGNAL_ROTATE))
	{
		// Panel doesnt rotate now 
	}
	else if (dbus_message_is_signal (msg, dbus_interface, CLP_APP_MGR_DBUS_SIGNAL_EXEC))
	{
		if(appclient_context.exec_callback!=NULL) {
			DBusMessageIter iter, array_iter;
			guint no_of_param,i;
			gchar *temp=NULL;
			gchar **params_list=NULL;

			dbus_message_iter_init(msg, &iter);
			dbus_message_iter_get_basic(&iter, &no_of_param);
			dbus_message_iter_next(&iter);
			
			if(((params_list = (gchar **)g_malloc0(((sizeof(gchar *))* no_of_param))) == NULL)) {
		       	        CLP_APPMGR_WARN("Out Of Memory!"); 
				CLP_APPMGR_EXIT_FUNCTION();
				return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
			}
			CLP_APPMGR_INFO_V("Application Restored through app_exec Num Params .. %u", no_of_param );
			if(no_of_param != 0) 
			{
				dbus_message_iter_recurse(&iter, &array_iter);
				for(i=0; i<no_of_param;i++)
				{

					dbus_message_iter_get_basic(&array_iter, &temp);
					if(((params_list[i] = (gchar *)g_malloc0(strlen(temp) + 1 )) == NULL)) {
						CLP_APPMGR_WARN("Out Of Memory!"); 
						CLP_APPMGR_EXIT_FUNCTION();
						return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
					}

					g_stpcpy (params_list[i], temp);
					CLP_APPMGR_INFO_V("Restore ( Param %u : %s )",i, params_list[i] );
					dbus_message_iter_next(&array_iter);
				}
			}
			(appclient_context.exec_callback)(no_of_param, params_list);
		
			for(i=0;i<no_of_param;i++)
				g_free(params_list[i]);
			g_free(params_list);
		}
	}
	else if (dbus_message_is_signal (msg, CLP_APP_MGR_DBUS_INTERFACE, CLP_APP_MGR_DBUS_SIGNAL_APPEXIT ))
	{
		if(appclient_context.death_callback){
			guint process_id;
			DBusMessageIter iter;
			dbus_message_iter_init(msg, &iter);
			dbus_message_iter_get_basic(&iter, &process_id);

			CLP_APPMGR_INFO_V("Application died with pid : %u!!", process_id);
			(appclient_context.death_callback)(NULL, process_id);
		}
	}
	else if (dbus_message_is_signal (msg, dbus_interface, CLP_APP_MGR_DBUS_SIGNAL_MESSAGE))
	{
		if(appclient_context.message_callback!=NULL) {
			DBusMessageIter iter, array_iter;
			guint no_of_param,i;
			gchar *temp=NULL;
			gchar **message_list=NULL;

			dbus_message_iter_init(msg, &iter);
			dbus_message_iter_get_basic(&iter, &no_of_param);
			dbus_message_iter_next(&iter);
			
			CLP_APPMGR_INFO_V("Application got message with Num Params .. %u", no_of_param );
			if(no_of_param != 0) 
			{
				message_list = (gchar **)g_malloc0(sizeof(gchar *)* no_of_param);
				dbus_message_iter_recurse(&iter, &array_iter);
				
				for(i=0; i<no_of_param;i++)
				{
					dbus_message_iter_get_basic(&array_iter, &temp);
					CLP_APPMGR_INFO_V("Restore ( Message %u : %s )",i, temp);
					message_list[i] = g_strdup(temp);
					dbus_message_iter_next(&array_iter);
				}
			}
			(appclient_context.message_callback)(no_of_param, message_list);
			for (i=0;i<no_of_param;i++)
				g_free(message_list[i]);
			g_free(message_list);
		}
	}
	else
	{
		CLP_APPMGR_EXIT_FUNCTION();
		return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
	}

	CLP_APPMGR_EXIT_FUNCTION();
	return DBUS_HANDLER_RESULT_HANDLED;
}


/**\brief The function for getting the list of installed themes
 *
 * \return GList with the list of installed themes
 *
 * get_installed_themes returns pre-installed themes.
 * it is a glist of names of the themes.
 * list of pre-installed themes are in gconf with the name and path to the theme file.
 * To be used by settings-theme to query available themes.
 */
GSList* 
clp_app_mgr_get_installed_themes()
{
	CLP_APPMGR_ENTER_FUNCTION();
	gchar *dir = gtk_rc_get_theme_dir();
	GSList *themes = read_theme_list(dir);
	GSList *themeNameList =NULL;
	ClpAppMgrThemeInfo *themeStruct=NULL;
	
	/* collect the list in GList*/
	while(themes) {
		themeStruct = (ClpAppMgrThemeInfo *)themes->data;
		themeNameList =  g_slist_prepend(themeNameList, (gchar*)themeStruct->theme);
		themes = g_slist_remove(themes, themeStruct);
	}
	g_slist_free(themes);
	g_free(dir);
	CLP_APPMGR_EXIT_FUNCTION();
	return themeNameList;
}


/**\brief Internal Function reading the themes from the gtk theme directory
 *
 * \param dirname The gtk directory containing the pre installed themes
 *
 * \warning This function is internal to the library and cannto be accessed by the application developer
 *
 * The function reads the list of themes from the provided directory and return back to the get_installed_themes
 * Freeing of the GList Returned is responsibility of user
 */
static GSList* 
read_theme_list(gchar *dirname)
{
	CLP_APPMGR_ENTER_FUNCTION();
	GSList *theme_list = NULL;	
	GDir *dir = NULL;
	ClpAppMgrThemeInfo *theme_info = NULL;
	gchar *localThemeName = NULL;

	if ((dir = g_dir_open(dirname, 0, NULL))!=NULL)
	{
		while ((localThemeName = (gchar *)g_dir_read_name(dir)))
		{
			theme_info = (ClpAppMgrThemeInfo*)g_malloc0(sizeof(ClpAppMgrThemeInfo));
			g_stpcpy(theme_info->theme, localThemeName);
			if (g_file_test(theme_info->theme, G_FILE_TEST_IS_DIR))
				continue;

			gchar *ptemp =  g_strdup_printf("%s/%s/gtk-2.0/gtkrc",dirname,theme_info->theme);
			g_stpcpy(theme_info->rcfile, ptemp);
			g_free(ptemp);

			if (!g_file_test(theme_info->rcfile, G_FILE_TEST_IS_REGULAR))
			{
				g_free(theme_info);
				continue;
			}
			theme_list = g_slist_prepend(theme_list, (ClpAppMgrThemeInfo*)theme_info);
		}	
		g_dir_close(dir);
		CLP_APPMGR_EXIT_FUNCTION();
		return theme_list;
	}
	else
	{
		CLP_APPMGR_WARN("Cannot open the Theme Directory !!Check if it exists.");
		CLP_APPMGR_EXIT_FUNCTION();
		return NULL;
	}
}


/**\brief Install the provided theme
 *
 * \param theme_name Name of the theme to be installed
 *
 * \return CLP_APP_MGR_SUCCESS - Theme applied successfully.
 * \return 1 - No Themes Available
 * \return 2 - Could not open gtkrc file
 * \return 3 - Could not locate themeName in installed list of themes
 *         
 * The function generates the gtkrc file in the home directory with the new theme
 * and sends a dbus signal of theme change to all the application (broadcast)
 * the app manager lib catches the same and actually reloads the theme
 * TODO: Upgrade to move the icon themeing etc
 */
gint 
clp_app_mgr_apply_theme(const gchar* theme_name)
{
	CLP_APPMGR_ENTER_FUNCTION();

	CLP_APPMGR_PARAM_ERROR((theme_name && (strcmp(theme_name, ""))),"Parameter 'theme_name' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(theme_name) <= NAME_SIZE),"Parameter 'theme_name' exceeds the maximum allowed name size");

	GSList *theme_list = NULL;
	gchar gtkrc[2*MAX_SIZE];
	gboolean successFlag = FALSE;
	ClpAppMgrThemeInfo *themeInfo = NULL;
	FILE *gtkrc_fh;
	gchar *include_file = NULL;
	
	/* Read the list of themes*/
	theme_list = read_theme_list(gtk_rc_get_theme_dir());
	
	if(theme_list == NULL) {
		CLP_APPMGR_EXIT_FUNCTION();
		return 1;	/* No themes installed	*/
	}
	
	g_stpcpy (gtkrc, READ_THEME_DIR);
	g_strlcat (gtkrc, "/gtk-2.0/gtkrc", (2*MAX_SIZE));
	gtkrc_fh = fopen(gtkrc,"w+");
	if(gtkrc_fh == NULL) {
		CLP_APPMGR_WARN("Can not open the gtkrc file !!");
		CLP_APPMGR_EXIT_FUNCTION();
		return 2;	/* Could not open GTK Rc files*/
	}
	
	/* Traverse the GSList to locate the theme given*/
	while(theme_list)
	{
		if(!strcmp( ((ClpAppMgrThemeInfo *)theme_list->data)->theme, theme_name))
		{
			successFlag = TRUE;
			break;
		}		
		theme_list = theme_list->next;
	}
	
	if(successFlag == FALSE) {
		CLP_APPMGR_EXIT_FUNCTION();
		return 3;	/*  Could not locate themeName in installed list of themes */
	}
	
	themeInfo = (ClpAppMgrThemeInfo *)theme_list->data;
	include_file = themeInfo->rcfile;
	fprintf(gtkrc_fh, "# -- THEME AUTO-WRITTEN DO NOT EDIT\n" "include \"%s\"\n\n", include_file);
	fprintf(gtkrc_fh, "# -- THEME AUTO-WRITTEN DO NOT EDIT\n");

	fclose(gtkrc_fh);

	gchar *default_files[] = { gtkrc, NULL };
	gtk_rc_set_default_files(default_files);
	gtk_rc_reparse_all_for_settings(gtk_settings_get_default(), TRUE);
	GdkEventClient event = { GDK_CLIENT_EVENT, NULL, TRUE, gdk_atom_intern("_GTK_READ_RCFILES", FALSE), 8 };
	gdk_event_send_clientmessage_toall((GdkEvent *) & event);
	
	g_slist_free(theme_list);
	CLP_APPMGR_EXIT_FUNCTION();
        return CLP_APP_MGR_SUCCESS;
}


/**\brief Get the list of currently running application
 * 
 * \return GList of ClpAppMgrActiveApp 
 *
 * The function will get the list of currently running active applications in the form of GList. 
 * The data part is ClpAppMgrActiveApp structure which contains the required information about the Application.
 *  
 */
GList*
clp_app_mgr_get_active_apps()
{
	CLP_APPMGR_ENTER_FUNCTION();
	gint *apps = NULL;
	gint num_of_active_apps = 0, i;
	gint return_code;

	GList *active_apps = NULL;

	return_code = AppMgrAppGetRunningApps (&apps, &num_of_active_apps);
	if(!return_code)
	{
		for (i = 0; i < num_of_active_apps; i++) {
			int *instid = NULL, no_of_active_inst = 0;
			int appid, return_code, pid, j;

			return_code = AppMgrAppGetRunningInstances(apps[i], &instid, &no_of_active_inst);

			if(!return_code)
			{
				for(j = 0; j < no_of_active_inst; j++) {
				return_code = AppMgrAppGetInstInfo (instid[j], &appid, &pid);
				
				gchar app_id[NAME_SIZE];
				sprintf(app_id,"%d",appid);
				GConfClient *client = gconf_client_get_default();
				gconf_client_add_dir(client, GCONF_APPS_DIR, GCONF_CLIENT_PRELOAD_NONE, NULL);
				gchar *key_path = g_strconcat(LIMO_APPS_DIR, "/", app_id , NULL);
				CLP_APPMGR_INFO_V("Key Path - %s\n", key_path);
				gchar *temp;
				GError *err=NULL;
	
				temp = g_strconcat(key_path, "/AppExecName", NULL);
				gchar *t = gconf_client_get_string (client, temp, &err);
				gchar *key_path_appmgr = g_strconcat(GCONF_APPS_DIR, "/", t, "/info", NULL);
				
				temp = g_strconcat (key_path_appmgr, "/Name", NULL);
				t = gconf_client_get_string (client, temp, &err);
				if( t==NULL )		continue;
				ClpAppMgrActiveApp *new_app = (ClpAppMgrActiveApp*)g_malloc0(sizeof (ClpAppMgrActiveApp));
				g_stpcpy (new_app->title, t);
				g_free(t);
				g_free(temp);
			
				err=NULL;
				temp = g_strconcat (key_path_appmgr, "/Command", NULL);
				gchar **temp_name = g_strsplit((gchar *)(gconf_client_get_string (client, temp, &err))," ",2);
				t = gconf_client_get_string (client, temp, &err);
				g_stpcpy (new_app->name, t);
				g_free(t);
				g_free(temp);
				g_strfreev(temp_name);
			
				err=NULL;
				temp = g_strconcat (key_path_appmgr, "/Icon", NULL);
				new_app->icon = gconf_client_get_string (client, temp, &err);
				g_free(temp);
				
				new_app->pid = pid;
				
				err=NULL;
				temp = g_strconcat (key_path_appmgr, "/Visibility", NULL);
				new_app->visibility = gconf_client_get_bool (client, temp, &err);
				g_free(temp);
				
				err=NULL;
				temp = g_strconcat (key_path_appmgr, "/Immortal", NULL);
				new_app->immortal = gconf_client_get_bool (client, temp, &err);
				g_free(temp);

				g_free(key_path);
				g_free(key_path_appmgr);
				
				active_apps = g_list_append(active_apps, new_app);
				}
			} 
			else 
			{
				CLP_APPMGR_WARN_V("Unable to get Running Instance of App %d ! Error Code - %d", apps[i], return_code);
			}
		}
	}
	else {
		CLP_APPMGR_WARN_V("Unable to get Running Apps !! Error Code %d", return_code);
	}
		
	CLP_APPMGR_EXIT_FUNCTION();
	return active_apps;
}


/**\brief Get the number of currently running application
 * 
 * \return gint Number of active applications 
 *
 * The function will return the number of currently running applications. 
 */
gint 
clp_app_mgr_get_num_of_active_apps() 
{
	CLP_APPMGR_ENTER_FUNCTION();

	gint num_of_active_apps, return_code;
	gint *apps = NULL;
	
	return_code = AppMgrAppGetRunningApps(&apps, &num_of_active_apps);
	if(return_code) {
		CLP_APPMGR_WARN_V("Unable to get Running Apps !! Error Code %d", return_code);
	}

	CLP_APPMGR_INFO_V("Currently Active Applications: %d",num_of_active_apps);
	CLP_APPMGR_EXIT_FUNCTION();
	return num_of_active_apps;
}


/**\brief Get the number of currently active instances of a particular application
 *
 * \param appname the name of the application whose instances are to be queried
 *
 * \return gint Number of active instances 
 *
 * The function will return the number of currently active instances of the given application. 
 */
gint clp_app_mgr_get_num_of_active_instances_of_app(gchar *appname)
{
	CLP_APPMGR_ENTER_FUNCTION();
	CLP_APPMGR_PARAM_ERROR((appname && (strcmp(appname, ""))),"Parameter 'appname' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(appname) <= NAME_SIZE),"Parameter 'appname' exceeds the maximum allowed name size");
	gint return_code = 0;
	gint *inst_ids = NULL;
	gint num_of_instances = 0;
	gint appid = clp_app_mgr_get_app_id(appname);
	return_code = AppMgrAppGetRunningInstances(appid, &inst_ids, &num_of_instances);
	if(return_code) {
		CLP_APPMGR_WARN_V("Unable to get Running Instances of App %d !! Error Code %d", appid, return_code);
	}

	CLP_APPMGR_INFO_V("Currently Active Instance : %d",num_of_instances);
	CLP_APPMGR_EXIT_FUNCTION();
	return num_of_instances;
}


/**\brief Check if the application is active
 *
 * \param appname the name of the application to check if its active
 *
 * \return gint TRUE if application is active, FALSE if not active and CLP_APP_MGR_FAILURE otherwise 
 *
 * The function will return TRUE/FALSE based on whether the application is active or not 
 */
gint clp_app_mgr_is_app_active(gchar *appname)
{
	CLP_APPMGR_ENTER_FUNCTION();
	CLP_APPMGR_PARAM_ERROR((appname && (strcmp(appname, ""))),"Parameter 'appname' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(appname) <= NAME_SIZE),"Parameter 'appname' exceeds the maximum allowed name size");
	
	gint return_code;
	gboolean is_app_active = FALSE;
	gint appid = clp_app_mgr_get_app_id(appname);
	return_code = AppMgrAppIsRunning (appid);
	if(return_code) {
		CLP_APPMGR_WARN_V("Unable to get Running Status of App %d !! Error Code %d", appid, return_code);
	}
	else 
		is_app_active = TRUE;

	CLP_APPMGR_EXIT_FUNCTION();
	return is_app_active;
}


/**\brief Given the pid of the application, return its application id.
 *
 * \param pid the pid of the application whose id is to be queried
 *
 * \return gchar* the application id (the instance name) of the given pid if there is valid app with given pid 
 *  and NULL when there is no application active with given pid.  
 *
 * The function will return application id of the given pid. It will return NULL if there is no active 
 * application with given pid or if there is any error.  
 */
gchar* clp_app_mgr_get_application_id(gint pid)
{
	CLP_APPMGR_ENTER_FUNCTION();
	gint *inst_ids = NULL;
	gint num_returned=0, appid;
	pid_t return_pid;
	gint return_code;
	
	return_code = AppMgrAppGetRunningInstancesInPid (pid, &inst_ids, &num_returned);
	if (return_code) {
		CLP_APPMGR_WARN_V("Failed to get running instances of Application Pid - %d ! Error code %d", pid, return_code);
	}

	return_code = AppMgrAppGetInstInfo (inst_ids[0], &appid, &return_pid);
	if (return_code) {
		CLP_APPMGR_WARN_V("Failed to get running instances of Application Pid - %d ! Error code %d", pid, return_code);
	}
	if(pid == return_pid)
		CLP_APPMGR_INFO_V("Got the application id for PID = %d AppId = %d", pid, appid);
	
	gchar app_id [NAME_SIZE];
	sprintf(app_id, "%d", appid);
	gchar *application_id = g_strdup(app_id);
	
	CLP_APPMGR_EXIT_FUNCTION();
	return application_id;
}


/**\brief Given the application name, return the active instances of that application.
 *
 * \param appname the application whose instances are to be queried
 *
 * \return GList of the active instance names of the given application name. It returns NULL if list is empty or if there is any error.  
 *
 * The function will return GList of active instances of given application. It will return NULL if any error.  
 */
GList* clp_app_mgr_get_active_instances_of_app(gchar *appname)
{
	CLP_APPMGR_ENTER_FUNCTION();
	CLP_APPMGR_PARAM_ERROR((appname && (strcmp(appname, ""))),"Parameter 'appname' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(appname) <= NAME_SIZE),"Parameter 'appname' exceeds the maximum allowed name size");
	
	gint appid, *inst_ids = NULL, num_of_instances = 0, i;
	GList *instances_list = NULL;
	ClpAppMgrActiveApp *data;

	appid = clp_app_mgr_get_app_id(appname);

	gint return_code = AppMgrAppGetRunningInstances (appid, &inst_ids, &num_of_instances);

	if(return_code) {
		CLP_APPMGR_WARN_V("Failed to get running instances of App %s! Error Code - %d",appname, return_code);
	}

	for (i = 0; i < num_of_instances; i++) {
		gchar instance[NAME_SIZE];
		sprintf (instance, "%s:%d", appname, inst_ids[i]);
		CLP_APPMGR_INFO_V("Instance Name: %s",(gchar *)instance);
		data = clp_app_mgr_get_application_instance_info(instance);
		instances_list = g_list_append(instances_list, data);
	}
	
	CLP_APPMGR_EXIT_FUNCTION();
	return instances_list;
}


/**\brief Given the application instance name, return its information.
 *
 * \param instance_name the instance name of the application whose information is to be queried.
 *
 * \return ClpAppMgrActiveApp* It contains the instance information of the instance name passed. It is NULL if 
 *  any error or if that instance is not active when information is queried.  
 *
 * The function will return instance information of the name passed if that instance is active. It will return NULL otherwise. 
 */
ClpAppMgrActiveApp *clp_app_mgr_get_application_instance_info(gchar *instance_name)
{
	CLP_APPMGR_ENTER_FUNCTION();
	CLP_APPMGR_PARAM_ERROR((instance_name && (strcmp(instance_name, ""))),"Parameter 'instance_name' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(instance_name) <= NAME_SIZE),"Parameter 'instance_name' exceeds the maximum allowed name size");

	gint *inst_ids=NULL, instid, appid, return_code, pid, num_of_instances;
	gchar **split = g_strsplit(instance_name,":",2);
	if( split[1]==NULL || g_strcasecmp(split[1],"")==0 )
	{
		appid = clp_app_mgr_get_app_id(split[0]);
		return_code = AppMgrAppGetRunningInstances (appid, &inst_ids, &num_of_instances);
		instid = inst_ids[0];
	}
	else
	{
		instid = atoi(*(split+1));
	}
	return_code = AppMgrAppGetInstInfo (instid, &appid, &pid);

	if(!return_code)
	{
		ClpAppMgrActiveApp *new_app = (ClpAppMgrActiveApp*)g_malloc0(sizeof (ClpAppMgrActiveApp));
		gchar app_id[NAME_SIZE];
		sprintf(app_id,"%d",appid);
		GConfClient *client = gconf_client_get_default();
		gconf_client_add_dir(client, GCONF_APPS_DIR, GCONF_CLIENT_PRELOAD_NONE, NULL);
		gchar *key_path = g_strconcat(GCONF_APPS_DIR, "/", split[0], "/info", NULL);
		CLP_APPMGR_INFO_V("Key Path - %s\n", key_path);
		gchar *temp;
		GError *err=NULL;
		temp = g_strconcat (key_path, "/Name", NULL);
		gchar *t = gconf_client_get_string (client, temp, &err);
		g_stpcpy (new_app->title, t);
		g_free(t);
		g_free(temp);
	
		err=NULL;
		temp = g_strconcat (key_path, "/Command", NULL);
		gchar **temp_name = g_strsplit((gchar *)(gconf_client_get_string (client, temp, &err))," ",2);
		t = gconf_client_get_string (client, temp, &err);
		g_stpcpy (new_app->name, t);
		g_free(t);
		g_free(temp);
		g_strfreev(temp_name);
			
		err=NULL;
		temp = g_strconcat (key_path, "/Icon", NULL);
		new_app->icon = gconf_client_get_string (client, temp, &err);
		g_free(temp);
				
		new_app->pid = pid;
		
		err=NULL;
		temp = g_strconcat (key_path, "/Visibility", NULL);
		new_app->visibility = gconf_client_get_bool (client, temp, &err);
		g_free(temp);
		
		g_free(key_path);

		CLP_APPMGR_EXIT_FUNCTION();
		return new_app;
	}
	else 
	{
		CLP_APPMGR_WARN(" Invalid parameter 'instance_name' detected ");
		CLP_APPMGR_EXIT_FUNCTION();
		return NULL;
	}
}


/* service discovery */
/**\brief Given a file name, suggest the Mimetype
 *
 * \param filename The File Name whose Mime Type is to be queried
 *
 * The function returns the mime type of the file name. If filename is NULL, it returns NULL. For filename as empty string it returns 'application/octet-stream'. It uses API provided by xdgmime.
 */
gchar*
clp_app_mgr_mime_from_file(const gchar *filename)
{
	CLP_APPMGR_ENTER_FUNCTION();
	if(filename==NULL)
	{
		CLP_APPMGR_WARN("Parameter 'filename' is NULL");
		CLP_APPMGR_EXIT_FUNCTION();
		return NULL;
	}
	gchar *out_mime_type = (gchar*) xdg_mime_get_mime_type_for_file (filename,NULL);
	CLP_APPMGR_EXIT_FUNCTION();
	return out_mime_type;
	
}


/**\brief Given some data string (phone number or url string ), suggest the Mimetype
 *
 * \param data The data whose Mime Type is to be queried
 *
 * The function returns the mime type of the data string. If 'data' is NULL, it returns NULL. For 'data' as empty string it returns 'application/octet-stream'. It uses API provided by xdgmime.
 */
gchar*
clp_app_mgr_mime_from_string(const gchar *data)
{
	CLP_APPMGR_ENTER_FUNCTION();
	if(data==NULL)
	{
		CLP_APPMGR_WARN("Parameter 'data' is NULL");
		CLP_APPMGR_EXIT_FUNCTION();
		return NULL;
	}
	gchar *out_mime_type = (gchar*) xdg_mime_get_mime_type_from_file_name (data);
	CLP_APPMGR_EXIT_FUNCTION();
	return out_mime_type;
}


/**\brief Discover the available services for a given Mime Type
 *
 * \param mimetype The Mime Type for whom available services are to be queried
 *
 * The function returns the list of all the services associated with the Mime Type.
 */
GSList*
clp_app_mgr_get_services(const gchar *mimetype)
{
	CLP_APPMGR_ENTER_FUNCTION();

	if(mimetype==NULL || strcmp(mimetype,"")==0)
	{
		CLP_APPMGR_WARN("Mimetype is either NULL or empty string. Hence list returned will be NULL.");
		CLP_APPMGR_EXIT_FUNCTION();
		return NULL;
	}

	gchar *contents, **arr_str, **arr_mime, **arr_desktop, **arr_srvc, *key;
	gchar *app_name=NULL, *app_exec_name=NULL;
	gsize length;
	GError *error;
	GSList *list=NULL;
 	gint i=1, j, k;
	ClpAppMgrServices *service_info;
	
	g_file_get_contents(APPLICATION_INFO_PATH"mimeinfo.cache",&contents,&length,&error);
	
	arr_str = g_strsplit(contents,"\n",MAX_NO_OF_LINES);
	
	for( i=1; *(arr_str+i)!=NULL; i++ )
	{
		arr_mime = g_strsplit(*(arr_str+i),"=",2);
		
		if( *arr_mime==NULL )	break;   //Because it was printing for the (null) value also n giving seg fault
		
		if( g_strcasecmp(mimetype,*arr_mime)==0 )
		{
			arr_desktop = g_strsplit(*(arr_mime+1),";",MAX_NO_OF_APPS_PER_MIME_TYPE);
			for( j=0; (*(arr_desktop+j)!=NULL); j++ )
			{
				key = g_strconcat(APPLICATION_INFO_PATH,*(arr_desktop+j),NULL);
				if( g_strcasecmp(key,APPLICATION_INFO_PATH)==0 )	break;
				g_file_get_contents(key,&contents,&length,&error);
				g_free(key);

				arr_str = g_strsplit(contents,"\n",MAX_NO_OF_LINES);

			        for( i=1; *(arr_str+i)!=NULL; i++ )
			        {
			                arr_mime = g_strsplit(*(arr_str+i),"=",2);
					if( *arr_mime==NULL )   break;   //Because it was printing for the (null) value also n giving seg fault
					
					if( g_strcasecmp("Name",*arr_mime)==0 )
					{
						app_name = *(arr_mime+1);
						continue;
					}

					if( g_strcasecmp("Exec",*arr_mime)==0 )
					{
						app_exec_name = *(arr_mime+1);
						continue;
					}	
				}

			        for( i=1; *(arr_str+i)!=NULL; i++ )
			        {
			                arr_mime = g_strsplit(*(arr_str+i),"=",2);
					if( *arr_mime==NULL )   break;   //Because it was printing for the (null) value also n giving seg fault

				        if( g_strcasecmp("Services",*arr_mime)==0 || g_strcasecmp("X-Services",*arr_mime)==0 )
			                {
						arr_srvc = g_strsplit(*(arr_mime+1),";",MAX_NO_OF_APPS_PER_MIME_TYPE);
						for( k=0; *(arr_srvc+k)!=NULL; k++ )
						{
							if( g_strcasecmp(*(arr_srvc+k),"")==0 )		break;
							
							service_info = (ClpAppMgrServices*)g_malloc0(sizeof (ClpAppMgrServices));

							service_info->app_name = g_strdup(app_name);
							service_info->app_exec_name = g_strdup(app_exec_name);
							
							gchar **serv_menu;
						        serv_menu = g_strsplit(*(arr_srvc+k),",",2);
							service_info->service_name = g_strdup(*serv_menu); 
							if( *(serv_menu+1)==NULL ) 	service_info->service_menu = g_strdup(*serv_menu);
							else 		service_info->service_menu = g_strdup(*(serv_menu+1));
							
							list = g_slist_append(list,service_info);
						}
						break;
					}
				}
			}
			break;
		}
	}	

	CLP_APPMGR_EXIT_FUNCTION();
	return list;
}


/** \brief Service Invocation function 
 *
 * \param application Name of the application to be execed for the service, Should be Null terminated
 * 
 * \return CLP_APP_MGR_SUCCESS - Application service invocation got successful
 * \return CLP_APP_MGR_FAILURE - Application service invocation failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * The function invokes the respective application with the service and passed params 
 */
gint clp_app_mgr_service_invoke(const gchar *application, ...)
{
	CLP_APPMGR_ENTER_FUNCTION();
	CLP_APPMGR_PARAM_ERROR((application && (strcmp(application, ""))),"Parameter 'application' is NULL or empty string.");
	CLP_APPMGR_PARAM_ERROR((strlen(application) <= NAME_SIZE),"Parameter 'application' exceeds the maximum allowed name size");
	va_list args;
	gint rv;
	
	// calls the exec with params and the parameters to be passed are taken from the service and argc,argv format.
	va_start (args, application);
	rv = clp_app_mgr_exec_application (application, args);
	va_end (args);
	CLP_APPMGR_EXIT_FUNCTION();
	return rv;
}


/** \brief Handle Content (Invoke default service) function for this string based MIME 
 *
 * \param mime_type MIME type for the string to be handled
 * \param mime_data MIME content string For eq Phone number
 *
 * \return CLP_APP_MGR_SUCCESS - Application service invocation got successful
 * \return CLP_APP_MGR_FAILURE - Application service invocation failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * Function to be used for action on URI, phone numbers etc.
 */
gint clp_app_mgr_handle_mime(const gchar *mime_type, const gchar *mime_data)
{
	CLP_APPMGR_ENTER_FUNCTION();

	if(mime_type==NULL || mime_data== NULL)
	{
		CLP_APPMGR_WARN("Parameter is NULL and hence it cannot be handled");
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}

	if(strcmp(mime_type,"application/octet-stream")==0)
	{
		CLP_APPMGR_WARN("No valid mime type for the string passed (defaulted to 'application/octet-stream') and hence it cannot be handled");
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}

	gchar *contents, **arr_str, **arr_mime, **arr_desktop, **arr_srvc, *key;
	gsize length;
	GError *error;
	int i, dbus_call_flag=0, success_flag=0, service_not_empty=0;
	gchar **str_srvc;
	
	g_file_get_contents(APPLICATION_INFO_PATH"mimeinfo.cache",&contents,&length,&error);

	arr_str = g_strsplit(contents,"\n",MAX_NO_OF_LINES);
	g_free(contents);
	
	for( i=1; *(arr_str+i)!=NULL; i++ )
	{
		arr_mime = g_strsplit(*(arr_str+i),"=",2);
		
		if( *arr_mime==NULL )	break;   //Because it was printing for the (null) value also n giving seg fault
		
		if( g_strcasecmp(mime_type,*arr_mime)==0 )
		{
			arr_desktop = g_strsplit(*(arr_mime+1),";",MAX_NO_OF_APPS_PER_MIME_TYPE);
			gchar *appname = g_strdup (*(g_strsplit(*arr_desktop,".",2)));
			CLP_APPMGR_INFO_V(" Default Application = %s\n",appname);

			key = g_strconcat(APPLICATION_INFO_PATH,*arr_desktop,NULL);
			if( g_strcasecmp(key,APPLICATION_INFO_PATH)==0 )	break;
			g_file_get_contents(key,&contents,&length,&error);
			g_free(key);
					
			arr_str = g_strsplit(contents,"\n",MAX_NO_OF_LINES);
			g_free(contents);

			for( i=1; *(arr_str+i)!=NULL; i++ )
			{
		                arr_mime = g_strsplit(*(arr_str+i),"=",2);
				if( *arr_mime==NULL )   break;   //Because it was printing for the (null) value also n giving seg fault

				if( g_strcasecmp("ExecType",*arr_mime)==0 || g_strcasecmp("X-ExecType",*arr_mime)==0 )
					if( g_strcasecmp("dbus",*(arr_mime+1))==0 )
						dbus_call_flag = 1;

			        if( g_strcasecmp("Services",*arr_mime)==0 || g_strcasecmp("X-Services",*arr_mime)==0 )
		                {
					arr_srvc = g_strsplit(*(arr_mime+1),";",MAX_NO_OF_APPS_PER_MIME_TYPE);
					if( *arr_srvc!=NULL )
					{
						str_srvc = g_strsplit(*arr_srvc,",",2);
						CLP_APPMGR_INFO_V(" Default Service = %s\n",*str_srvc);
						service_not_empty = 1;
					}
				}
			}

			if( dbus_call_flag==1 && service_not_empty==1 )
			{
				GConfClient *client = gconf_client_get_default();
				gchar *key_path = g_strconcat("/appmgr/",appname,"/info/DBusService",NULL);
				gchar *dbus_service = gconf_client_get_string (client, key_path, NULL);
				g_free(key_path);
				key_path = g_strconcat("/appmgr/",appname,"/info/DBusObjPath",NULL);
				gchar *dbus_objpath = gconf_client_get_string (client, key_path, NULL);
				g_free(key_path);
				key_path = g_strconcat("/appmgr/",appname,"/info/DBusInterface",NULL);
				gchar *dbus_interface = gconf_client_get_string (client, key_path, NULL);
				g_free(key_path);
					
				CLP_APPMGR_INFO("The service handler is Middleware module. Calling a remote HandleMime method !!");
				DBusGConnection *connection;
				DBusGProxy *proxy;
				GError *gerror=NULL;
				connection = dbus_g_bus_get(DBUS_BUS_SYSTEM, &gerror);
				proxy = dbus_g_proxy_new_for_name(connection, dbus_service, dbus_objpath, dbus_interface);
				CLP_APPMGR_INFO_V("Calling - %s %s %s with args - %s %s",dbus_service, dbus_objpath, dbus_interface, mime_type, mime_data);
				dbus_g_proxy_call_no_reply(proxy, *str_srvc, G_TYPE_STRING, mime_type, G_TYPE_STRING, mime_data, G_TYPE_INVALID, G_TYPE_INVALID);
			}
			else if( dbus_call_flag==0 && service_not_empty==1 )
				clp_app_mgr_service_invoke(appname, *str_srvc, mime_data,NULL);
			else if( dbus_call_flag==0 && service_not_empty==0 )
				clp_app_mgr_service_invoke(appname, mime_data,NULL);
			success_flag = 1;
			break;
		}
	}

	if(success_flag)
	{
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_SUCCESS;
	}
	else
	{
		CLP_APPMGR_WARN_V(" Unsupported Content - %s",mime_type);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
}


/** \brief Handle Content (Invoke default service) function for this string based MIME 
 *
 * \param data MIME content string For eq Phone number
 *
 * \return CLP_APP_MGR_SUCCESS - Application service invocation got successful
 * \return CLP_APP_MGR_FAILURE - Application service invocation failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * Function to be used for default action on URI, phone numbers etc.
 */
gint clp_app_mgr_handle_string(const gchar *data)
{
	CLP_APPMGR_ENTER_FUNCTION();

	if(data==NULL)
	{
		CLP_APPMGR_WARN("Parameter 'data' is NULL and hence it cannot be handled");
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}

	gchar *mime_type = (gchar*) xdg_mime_get_mime_type_from_file_name (data);
	CLP_APPMGR_EXIT_FUNCTION();
	return (clp_app_mgr_handle_mime(mime_type, data));
}


/** \brief Handle Content (Invoke default service) function 
 *
 * \param filepath The file name alongwith the path for which default service is to be invoked
 * 
 * \return CLP_APP_MGR_SUCCESS - Application service invocation got successful
 * \return CLP_APP_MGR_FAILURE - Application service invocation failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * The function invokes the default application's service for the filename passed with appropriate params
 */
gint clp_app_mgr_handle_file(const gchar *filepath)
{
	CLP_APPMGR_ENTER_FUNCTION();

	if(filepath==NULL)
	{
		CLP_APPMGR_WARN("Parameter 'filepath' is NULL and hence it cannot be handled");
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
	 
	gchar *mime_type = (gchar*) xdg_mime_get_mime_type_for_file (filepath,NULL);

	CLP_APPMGR_EXIT_FUNCTION();
	return (clp_app_mgr_handle_mime(mime_type, filepath));
}

/* service discovery end */


/* matchbox window manager support starts here..*/

/**\brief Internal Function to Get the list of windows from DBUS message
 *
 * \return GSList of ClpAppMgrWindowInfo
 *
 * The function will get the list of windows in the form of GSList. 
 * The data part is ClpAppMgrWindowInfo structure which comtains the required information about the Application.
 */
static GSList *clp_app_mgr_wm_parse_window_list(DBusMessage *msg)
{
	CLP_APPMGR_ENTER_FUNCTION();
	GSList *window_list=NULL;
	DBusMessageIter iter, array_iter_read, struct_iter_read;
	gint num_elem, i;
	
	gint pid;
	gint windowid;
	gchar *title=NULL;
	gchar *icon=NULL;
	
	dbus_message_iter_init(msg, &iter);
	dbus_message_iter_get_basic(&iter, &num_elem);

	if(num_elem) 
	{
		dbus_message_iter_next(&iter);
		dbus_message_iter_recurse(&iter, &array_iter_read);
	}
	
	for (i = 0; i < num_elem; i++) {
		ClpAppMgrWindowInfo *new_window = (ClpAppMgrWindowInfo*)g_malloc0(sizeof (ClpAppMgrWindowInfo));

		dbus_message_iter_recurse(&array_iter_read, &struct_iter_read);
		
		dbus_message_iter_get_basic(&struct_iter_read, &title);
		new_window->title = g_strdup(title);
		dbus_message_iter_next(&struct_iter_read);
		CLP_APPMGR_INFO_V("\npid:%d,id:%d,title:%s,icon:%s",new_window->pid,new_window->windowid,new_window->title,new_window->icon);
		
		dbus_message_iter_get_basic(&struct_iter_read, &icon);
		new_window->icon = g_strdup(icon);
		dbus_message_iter_next(&struct_iter_read);
		CLP_APPMGR_INFO_V("\npid:%d,id:%d,title:%s,icon:%s",new_window->pid,new_window->windowid,new_window->title,new_window->icon);

		dbus_message_iter_get_basic(&struct_iter_read, &pid);
		new_window->pid = pid;
		dbus_message_iter_next(&struct_iter_read);
		CLP_APPMGR_INFO_V("\npid:%d,id:%d,title:%s,icon:%s",new_window->pid,new_window->windowid,new_window->title,new_window->icon);
		
		dbus_message_iter_get_basic(&struct_iter_read, &windowid);
		new_window->windowid = windowid;
		dbus_message_iter_next(&array_iter_read);
		CLP_APPMGR_INFO_V("\npid:%d,id:%d,title:%s,icon:%s",new_window->pid,new_window->windowid,new_window->title,new_window->icon);
		
		window_list = g_slist_append(window_list, new_window);


		CLP_APPMGR_INFO_V("\npid:%d,id:%d,title:%s,icon:%s",new_window->pid,new_window->windowid,new_window->title,new_window->icon);
	}

	CLP_APPMGR_EXIT_FUNCTION();
	return window_list;
}


/** \brief List the displayable windows in the system
 *
 * \return List of windows currently registered with the window manager 
 *
 * The function gives the list of windows. Mainly useful for the switcher 
 */
GSList* clp_app_mgr_wm_get_window_list()
{
	CLP_APPMGR_ENTER_FUNCTION();

        DBusMessage *msg;
	DBusError error;
	dbus_error_init(&error);
			        
	GSList *window_list=NULL;
				        
	msg = dbus_message_new_method_call (CLP_WIN_MGR_DBUS_SERVICE, CLP_WIN_MGR_DBUS_OBJECT, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_GET_WINDOW_LIST_METHOD);      
        if (NULL == msg)
        {
               CLP_APPMGR_WARN("Message Null");
               CLP_APPMGR_EXIT_FUNCTION();
	       return NULL;
	}

	DBusMessage *reply = dbus_connection_send_with_reply_and_block (appclient_context.bus_conn, msg, -1, &error);
	if (reply==NULL)
	{       
	       CLP_APPMGR_WARN_V("Got Reply Null : error: %s", error.message);
	       CLP_APPMGR_EXIT_FUNCTION();
	       return NULL;
	}
								        
	window_list = clp_app_mgr_wm_parse_window_list(reply);
	CLP_APPMGR_EXIT_FUNCTION();
	
	return window_list;
}


/** \brief Locks the screen
 *
 * \return CLP_APP_MGR_SUCCESS - lock screen successful
 * \return CLP_APP_MGR_FAILURE - lock screen failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * Locks the screen. Locking may fail if higher priority application has taken lock.
 * If higher priority application requests focus, the lock is permanently broken.
 * If lower priority application requests focus, the lock prevails and window is not brought forward. 
 * Locking screen doesnt lock input, but most keys will be relayed to the focussed application
 */
gint clp_app_mgr_wm_get_screen_exclusive()
{
	CLP_APPMGR_ENTER_FUNCTION();
	
	DBusMessage *msg;
  	DBusMessageIter args;
	DBusError error;
	dbus_error_init(&error);	
	gint stat;
	pid_t pid = getpid();

	msg = dbus_message_new_method_call (CLP_WIN_MGR_DBUS_SERVICE, CLP_WIN_MGR_DBUS_OBJECT, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_SET_LOCK_METHOD);
	if (NULL == msg)
       	{ 
	      	CLP_APPMGR_WARN("Message Null");
		CLP_APPMGR_EXIT_FUNCTION();
	      	return CLP_APP_MGR_DBUS_CALL_FAIL;
	}

	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &pid))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}

	guint lock = 1;
	
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &lock))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}
	
	DBusMessage *reply = dbus_connection_send_with_reply_and_block (appclient_context.bus_conn, msg, -1, &error);
	if (reply == NULL) 
	{ 
	        CLP_APPMGR_WARN_V("Got Reply Null : error: %s", error.message);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_DBUS_REPLY_FAIL;
	}
	
	if (!dbus_message_iter_init(reply, &args))
        	CLP_APPMGR_WARN("Message has no arguments!");
	else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
        	CLP_APPMGR_WARN("Argument is not an integer!");
	else
        	dbus_message_iter_get_basic(&args, &stat);

	dbus_message_unref(msg);
	dbus_message_unref(reply);
	
	if(stat==0)
	{
		CLP_APPMGR_WARN("Could not acquire the screen ");
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
	
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief Get top window of the application
 *
 * \param pid The pid of the application whose top window is to be queried.
 * \param top_window The top window shall be returned here.
 * 
 * \return CLP_APP_MGR_SUCCESS - unlock screen successful
 * \return CLP_APP_MGR_FAILURE - unlock screen failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * Wrapper for window manager 'GetWindow' method call. Gets the top window of the application whose pid is sent. The title of the top window shall 
 * be returned.  
 */
gint clp_app_mgr_wm_get_top_window_of_application(gint pid, gchar **top_window)
{
	CLP_APPMGR_ENTER_FUNCTION();
	
	DBusMessage *msg;
  	DBusMessageIter args;
	DBusError error;
	dbus_error_init(&error);	
	gchar *stat;

	msg = dbus_message_new_method_call (CLP_WIN_MGR_DBUS_SERVICE, CLP_WIN_MGR_DBUS_OBJECT, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_GET_TOP_WINDOW_OF_APP_METHOD);
	if (NULL == msg)
       	{ 
	      	CLP_APPMGR_WARN("Message Null");
		CLP_APPMGR_EXIT_FUNCTION();
	      	return CLP_APP_MGR_DBUS_CALL_FAIL;
	}

	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &pid))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}

	DBusMessage *reply = dbus_connection_send_with_reply_and_block (appclient_context.bus_conn, msg, -1, &error);
	if (reply == NULL) 
	{ 
	        CLP_APPMGR_WARN_V("Got Reply Null : error: %s", error.message);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_DBUS_REPLY_FAIL;
	}
	
	if (!dbus_message_iter_init(reply, &args))
        	CLP_APPMGR_WARN("Message has no arguments!");
	else if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args))
        	CLP_APPMGR_WARN("Argument is not an integer!");
	else
        	dbus_message_iter_get_basic(&args, &stat);

	dbus_message_unref(msg);
	dbus_message_unref(reply);
	
	*top_window = g_strdup(stat);
	CLP_APPMGR_INFO_V("Current Top Window of Application with pid %d: %s",pid,stat);	
	
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief UnLocks the screen
 *
 * \return CLP_APP_MGR_SUCCESS - unlock screen successful
 * \return CLP_APP_MGR_FAILURE - unlock screen failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * UnLocks the screen. 
 */
gint clp_app_mgr_wm_release_screen()
{
	CLP_APPMGR_ENTER_FUNCTION();
	
	DBusMessage *msg;
  	DBusMessageIter args;
	DBusError error;
	dbus_error_init(&error);	
	gint stat;
	pid_t pid = getpid();

	msg = dbus_message_new_method_call (CLP_WIN_MGR_DBUS_SERVICE, CLP_WIN_MGR_DBUS_OBJECT, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_SET_LOCK_METHOD);
	if (NULL == msg)
       	{ 
	      	CLP_APPMGR_WARN("Message Null");
		CLP_APPMGR_EXIT_FUNCTION();
	      	return CLP_APP_MGR_DBUS_CALL_FAIL;
	}

	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &pid))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}

	guint lock = 0;
	
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &lock))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}
	
	DBusMessage *reply = dbus_connection_send_with_reply_and_block (appclient_context.bus_conn, msg, -1, &error);
	if (reply == NULL) 
	{ 
	        CLP_APPMGR_WARN_V("Got Reply Null : error: %s", error.message);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_DBUS_REPLY_FAIL;
	}
	
	if (!dbus_message_iter_init(reply, &args))
        	CLP_APPMGR_WARN("Message has no arguments!");
	else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
        	CLP_APPMGR_WARN("Argument is not an integer!");
	else
        	dbus_message_iter_get_basic(&args, &stat);

	dbus_message_unref(msg);
	dbus_message_unref(reply);
	
	if(stat==0)
	{
		CLP_APPMGR_WARN("Could not release the screen ");
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
	
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief Bring application with pid to focus 
 *
 * \param pid Process Id to be restored to the top of stack.
 *
 * \return CLP_APP_MGR_SUCCESS - unlock screen successful
 * \return CLP_APP_MGR_FAILURE - unlock screen failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * The application is restored to gain screen focus. Destination applicaiton will get a user attention gained signal as a result.
 * 
 */
gint clp_app_mgr_wm_restore_application(gint pid)
{
	CLP_APPMGR_ENTER_FUNCTION();
	
	DBusMessage *msg;
  	DBusMessageIter args;
	DBusError error;
	dbus_error_init(&error);	
	gint stat;

	msg = dbus_message_new_method_call (CLP_WIN_MGR_DBUS_SERVICE, CLP_WIN_MGR_DBUS_OBJECT, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_FOCUS_PID_METHOD);
	if (NULL == msg)
       	{ 
	      	CLP_APPMGR_WARN("Message Null");
		CLP_APPMGR_EXIT_FUNCTION();
	      	return CLP_APP_MGR_DBUS_CALL_FAIL;
	}

	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &pid))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}

	DBusMessage *reply = dbus_connection_send_with_reply_and_block (appclient_context.bus_conn, msg, -1, &error);
	if (reply == NULL) 
	{ 
	        CLP_APPMGR_WARN_V("Got Reply Null : error: %s", error.message);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_DBUS_REPLY_FAIL;
	}
	
	if (!dbus_message_iter_init(reply, &args))
        	CLP_APPMGR_WARN("Message has no arguments!");
	else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
        	CLP_APPMGR_WARN("Argument is not an integer!");
	else
        	dbus_message_iter_get_basic(&args, &stat);

	dbus_message_unref(msg);
	dbus_message_unref(reply);
	
	if(stat==0)
	{
		if(error.message) {
			CLP_APPMGR_WARN_V(" Application with pid :%d could not be restored. Got Status as %d Error: %s",pid, stat, error.message);
		} else {
			CLP_APPMGR_WARN_V(" Application with pid :%d could not be restored. Got Status as %d",pid, stat);
		}
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
	
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief Restore the application having the window 
 *
 * \param windowid Window to be restored
 *
 * \return CLP_APP_MGR_SUCCESS - unlock screen successful
 * \return CLP_APP_MGR_FAILURE - unlock screen failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * The application is restored to gain screen focus. Destination applicaiton will get a user attention gained signal as a result.
 * Window corresponding to the window id is brought right up front and the others are cascaded behind it.
 * 
 */
gint clp_app_mgr_wm_restore_window(gint windowid)
{
	CLP_APPMGR_ENTER_FUNCTION();
	
	DBusMessage *msg;
  	DBusMessageIter args;
	DBusError error;
	dbus_error_init(&error);	
	gint stat;

	msg = dbus_message_new_method_call (CLP_WIN_MGR_DBUS_SERVICE, CLP_WIN_MGR_DBUS_OBJECT, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_FOCUS_ID_METHOD);
	if (NULL == msg)
       	{ 
	      	CLP_APPMGR_WARN("Message Null");
		CLP_APPMGR_EXIT_FUNCTION();
	      	return CLP_APP_MGR_DBUS_CALL_FAIL;
	}

	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &windowid))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}

	DBusMessage *reply = dbus_connection_send_with_reply_and_block (appclient_context.bus_conn, msg, -1, &error);
	if (reply == NULL) 
	{ 
	        CLP_APPMGR_WARN_V("Got Reply Null : error: %s", error.message);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_DBUS_REPLY_FAIL;
	}
	
	if (!dbus_message_iter_init(reply, &args))
        	CLP_APPMGR_WARN("Message has no arguments!");
	else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
        	CLP_APPMGR_WARN("Argument is not an integer!");
	else
        	dbus_message_iter_get_basic(&args, &stat);

	dbus_message_unref(msg);
	dbus_message_unref(reply);
	
	if(stat==0)
	{
		CLP_APPMGR_WARN_V(" Window with windowid :%d could not be restored.",windowid);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
	
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief Minimise application and send it to the last in stacking order 
 *
 * \param pid process id of the application.
 * 
 * \return CLP_APP_MGR_SUCCESS - unlock screen successful
 * \return CLP_APP_MGR_FAILURE - unlock screen failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * Application will be sent to the last spot in the window stacking order.
 * Next application will gain focus.
 * User attention lost signal will be given to the caller and user attention gained will be given to next application.
 * Process Id can be got from the get_active_apps API by the switcher
 * 
 */
gint clp_app_mgr_wm_minimize_application(gint pid)
{
	CLP_APPMGR_ENTER_FUNCTION();
	
	DBusMessage *msg;
  	DBusMessageIter args;
	DBusError error;
	dbus_error_init(&error);	
	gint stat;

	msg = dbus_message_new_method_call (CLP_WIN_MGR_DBUS_SERVICE, CLP_WIN_MGR_DBUS_OBJECT, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_MINIMIZE_PID_METHOD);
	if (NULL == msg)
       	{ 
	      	CLP_APPMGR_WARN("Message Null");
		CLP_APPMGR_EXIT_FUNCTION();
	      	return CLP_APP_MGR_DBUS_CALL_FAIL;
	}

	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &pid))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}

	DBusMessage *reply = dbus_connection_send_with_reply_and_block (appclient_context.bus_conn, msg, -1, &error);
	if (reply == NULL) 
	{ 
	        CLP_APPMGR_WARN_V("Got Reply Null : error: %s", error.message);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_DBUS_REPLY_FAIL;
	}
	
	if (!dbus_message_iter_init(reply, &args))
        	CLP_APPMGR_WARN("Message has no arguments!");
	else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
        	CLP_APPMGR_WARN("Argument is not an integer!");
	else
        	dbus_message_iter_get_basic(&args, &stat);

	dbus_message_unref(msg);
	dbus_message_unref(reply);
	
	if(stat==0)
	{
		CLP_APPMGR_WARN_V(" Application with pid :%d could not be minimized.",pid);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
	
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief Minimise application and send it to the last in stacking order 
 *
 * \param windowid window identifier to be sent back
 * 
 * \return CLP_APP_MGR_SUCCESS - unlock screen successful
 * \return CLP_APP_MGR_FAILURE - unlock screen failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * Application will be sent to the last spot in the window stacking order.
 * Next application will gain focus.
 * User attention lost signal will be given to the caller and user attention gained will be given to next application.
 * Process Id can be got from the get_active_apps API by the switcher
 * 
 */
gint clp_app_mgr_wm_minimize_window(gint windowid)
{
	CLP_APPMGR_ENTER_FUNCTION();
	
	DBusMessage *msg;
  	DBusMessageIter args;
	DBusError error;
	dbus_error_init(&error);	
	gint stat;

	msg = dbus_message_new_method_call (CLP_WIN_MGR_DBUS_SERVICE, CLP_WIN_MGR_DBUS_OBJECT, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_MINIMIZE_ID_METHOD);
	if (NULL == msg)
       	{ 
	      	CLP_APPMGR_WARN("Message Null");
		CLP_APPMGR_EXIT_FUNCTION();
	      	return CLP_APP_MGR_DBUS_CALL_FAIL;
	}

	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &windowid))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}

	DBusMessage *reply = dbus_connection_send_with_reply_and_block (appclient_context.bus_conn, msg, -1, &error);
	if (reply == NULL) 
	{ 
	        CLP_APPMGR_WARN_V("Got Reply Null : error: %s", error.message);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_DBUS_REPLY_FAIL;
	}
	
	if (!dbus_message_iter_init(reply, &args))
        	CLP_APPMGR_WARN("Message has no arguments!");
	else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
        	CLP_APPMGR_WARN("Argument is not an integer!");
	else
        	dbus_message_iter_get_basic(&args, &stat);

	dbus_message_unref(msg);
	dbus_message_unref(reply);
	
	if(stat==0)
	{
		CLP_APPMGR_WARN_V(" Window with windowid :%d could not be minimized.",windowid);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
	
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief Get available dimensions for the screen available to the application. 
 *
 * \param  height pointer as return param
 * \param  width pointer as return param
 *  
 * \return CLP_APP_MGR_SUCCESS - successful
 * \return CLP_APP_MGR_FAILURE - failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * Get dimension of the screen. Screen dimensions may change due to change in panel width, defferent resolution and rotated orientation.
 * 
 */
gint clp_app_mgr_wm_get_available_screen_dimensions(gint *height, gint *width)
{
	CLP_APPMGR_ENTER_FUNCTION();
	DBusMessage *msg;
  	DBusMessageIter args;
	DBusError error;
	dbus_error_init(&error);

	msg = dbus_message_new_method_call (CLP_WIN_MGR_DBUS_SERVICE, CLP_WIN_MGR_DBUS_OBJECT, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_GET_SCREEN_DIMENSIONS_METHOD);
	if (NULL == msg)
       	{ 
		CLP_APPMGR_WARN("Message Null");
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_DBUS_CALL_FAIL;
	}
	
	DBusMessage *reply = dbus_connection_send_with_reply_and_block (appclient_context.bus_conn, msg, -1, &error);

	if (reply==NULL)
	{
	        CLP_APPMGR_WARN_V("Got Reply Null : error: %s", error.message);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}
	
	if (!dbus_message_iter_init(reply, &args))
	{
		CLP_APPMGR_WARN("Message has no arguments!");
	}
	else
	{
		if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
		{
			CLP_APPMGR_WARN("Argument is not integer!");
			CLP_APPMGR_EXIT_FUNCTION();
			return CLP_APP_MGR_FAILURE;
		}
	
		dbus_message_iter_get_basic(&args, width);

		dbus_message_iter_next(&args);

		if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
		{
			CLP_APPMGR_WARN("Argument is not integer!");
			CLP_APPMGR_EXIT_FUNCTION();
			return CLP_APP_MGR_FAILURE;
		}

		dbus_message_iter_get_basic(&args, height);
	}

	dbus_message_unref(msg);
	dbus_message_unref(reply);
	
	if(width==0||height==0)
	{
		CLP_APPMGR_WARN("Improper screen dimensions given.. ");
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}

	CLP_APPMGR_INFO_V("width:%d   height:%d",*width,*height);
	
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief Send Window move/resize request to window manager. 
 *
 * \param resizeinfo ClpAppMgrWinResizeInfo struct which has reuired information 
 *  
 * \return CLP_APP_MGR_SUCCESS - successful
 * \return CLP_APP_MGR_FAILURE - failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * Send Window move/resize request to window manager.
 */
gint clp_app_mgr_wm_move_resize_window(ClpAppMgrWinResizeInfo resizeinfo)
{
	CLP_APPMGR_ENTER_FUNCTION();
	
	DBusMessage *msg;
  	DBusMessageIter args;
	DBusError error;
	dbus_error_init(&error);	
	gint stat;

	msg = dbus_message_new_method_call (CLP_WIN_MGR_DBUS_SERVICE, CLP_WIN_MGR_DBUS_OBJECT, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_MOVE_RESIZE_WINDOW_METHOD);
	if (NULL == msg)
       	{ 
	      	CLP_APPMGR_WARN("Message Null");
		CLP_APPMGR_EXIT_FUNCTION();
	      	return CLP_APP_MGR_DBUS_CALL_FAIL;
	}

	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &resizeinfo.windowid))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}

	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &resizeinfo.x_move))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}
	 
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &resizeinfo.y_move))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}
	
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &resizeinfo.width))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}
	
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &resizeinfo.height))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}
	
	DBusMessage *reply = dbus_connection_send_with_reply_and_block (appclient_context.bus_conn, msg, -1, &error);
	if (reply == NULL) 
	{ 
	        CLP_APPMGR_WARN_V("Got Reply Null : error: %s", error.message);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_DBUS_REPLY_FAIL;
	}
	
	if (!dbus_message_iter_init(reply, &args))
        	CLP_APPMGR_WARN("Message has no arguments!");
	else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
        	CLP_APPMGR_WARN("Argument is not an integer!");
	else
        	dbus_message_iter_get_basic(&args, &stat);

	dbus_message_unref(msg);
	dbus_message_unref(reply);
	
	if(stat==0)
	{
		CLP_APPMGR_WARN_V(" Window with windowid :%d could not be moved/resized.",resizeinfo.windowid);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}

	CLP_APPMGR_INFO_V(" Window with windowid :%d successfully moved/resized.",resizeinfo.windowid);	
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief Register user attention handler 
 *
 * \param app_focus_gained_callback callback function to be called on attention gained and lost 
 *  
 * Gained is called when any window of the application gains focus. The application wasnt in focus previously. 
 */
void
clp_app_mgr_wm_register_focus_gained_handler(const app_focus_gained app_focus_gained_callback)
{
	CLP_APPMGR_ENTER_FUNCTION();
	appclient_context.app_focus_gained_callback = app_focus_gained_callback;
	CLP_APPMGR_EXIT_FUNCTION();
	return;
}


/** \brief Register user attention handler 
 *
 * \param app_focus_lost_callback callback function to be called on attention gained and lost 
 *  
 * Lost is called when all windows of the application loose focus. The application was in focus previously. 
 */
void
clp_app_mgr_wm_register_focus_lost_handler(const app_focus_lost app_focus_lost_callback)
{
	CLP_APPMGR_ENTER_FUNCTION();
	appclient_context.app_focus_lost_callback = app_focus_lost_callback;
	CLP_APPMGR_EXIT_FUNCTION();
	return;
}


/** \brief Set the priority of the window 
 *
 * \param  windowid windowid of the window whose priority is to be set 
 * \param  priority priority value to be set 
 *  
 * \return CLP_APP_MGR_SUCCESS - successful
 * \return CLP_APP_MGR_FAILURE - failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * Sets the priority of the window. Exposes Method provided by Matchbox Window Manager 
 */
gint clp_app_mgr_wm_set_window_priority(gint windowid, gint priority)
{
	CLP_APPMGR_ENTER_FUNCTION();
	
	DBusMessage *msg;
  	DBusMessageIter args;
	DBusError error;
	dbus_error_init(&error);	
	gint stat;

	msg = dbus_message_new_method_call (CLP_WIN_MGR_DBUS_SERVICE, CLP_WIN_MGR_DBUS_OBJECT, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_SET_WINDOW_PRIORITY_METHOD);
	if (NULL == msg)
       	{ 
	      	CLP_APPMGR_WARN("Message Null");
		CLP_APPMGR_EXIT_FUNCTION();
	      	return CLP_APP_MGR_DBUS_CALL_FAIL;
	}

	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &windowid))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}

	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &priority))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}
	
	DBusMessage *reply = dbus_connection_send_with_reply_and_block (appclient_context.bus_conn, msg, -1, &error);
	if (reply == NULL) 
	{ 
	        CLP_APPMGR_WARN_V("Got Reply Null : error: %s", error.message);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_DBUS_REPLY_FAIL;
	}
	
	if (!dbus_message_iter_init(reply, &args))
        	CLP_APPMGR_WARN("Message has no arguments!");
	else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
        	CLP_APPMGR_WARN("Argument is not an integer!");
	else
        	dbus_message_iter_get_basic(&args, &stat);

	dbus_message_unref(msg);
	dbus_message_unref(reply);
	
	if(stat==0)
	{
		CLP_APPMGR_WARN_V(" Priority for window with id :%d could not be set.",windowid);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
	
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief Toggles fullscreen mode of the window 
 *
 * \return CLP_APP_MGR_SUCCESS - successful
 * \return CLP_APP_MGR_FAILURE - failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * Toggles fullscreen mode of the window. Exposes Method provided by Matchbox Window Manager 
 */
gint clp_app_mgr_wm_toggle_fullscreen_window(void)
{
	CLP_APPMGR_ENTER_FUNCTION();
	
	DBusMessage *msg;
  	DBusMessageIter args;
	DBusError error;
	dbus_error_init(&error);	
	gint stat;
	
	msg = dbus_message_new_method_call (CLP_WIN_MGR_DBUS_SERVICE, CLP_WIN_MGR_DBUS_OBJECT, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_TOGGLE_FULL_SCREEN_WINDOW_METHOD);
	
	if (NULL == msg)
       	{ 
	      	CLP_APPMGR_WARN("Message Null");
		CLP_APPMGR_EXIT_FUNCTION();
	      	return CLP_APP_MGR_DBUS_CALL_FAIL;
	}

	DBusMessage *reply = dbus_connection_send_with_reply_and_block (appclient_context.bus_conn, msg, -1, &error);
	if (reply == NULL) 
	{ 
	        CLP_APPMGR_WARN_V("Got Reply Null : error: %s", error.message);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_DBUS_REPLY_FAIL;
	}
	
	if (!dbus_message_iter_init(reply, &args))
        	CLP_APPMGR_WARN("Message has no arguments!");
	else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
        	CLP_APPMGR_WARN("Argument is not an integer!");
	else
        	dbus_message_iter_get_basic(&args, &stat);

	dbus_message_unref(msg);
	dbus_message_unref(reply);
	
	if(stat==0)
	{
		CLP_APPMGR_WARN("Full screen could not be toggled.");
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
	
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


/** \brief Toggles fullscreen mode of another window
 *
 * \param windowid Window ID of a window to be fullscreen
 * \param flag Mode of the fullscreen
 *
 * \return CLP_APP_MGR_SUCCESS - successful
 * \return CLP_APP_MGR_FAILURE - failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * Toggles fullscreen mode of the window. Exposes Method provided by Matchbox Window Manager 
 */
gint clp_app_mgr_wm_fullscreen_window(gint windowid, gint flag)
{
	CLP_APPMGR_ENTER_FUNCTION();
	
	DBusMessage *msg;
  	DBusMessageIter args;
	DBusError error;
	dbus_error_init(&error);	
	gint stat;

	msg = dbus_message_new_method_call (CLP_WIN_MGR_DBUS_SERVICE, CLP_WIN_MGR_DBUS_OBJECT, CLP_WIN_MGR_DBUS_INTERFACE, CLP_WIN_MGR_FULL_SCREEN_WINDOW_METHOD);
	if (NULL == msg)
       	{ 
	      	CLP_APPMGR_WARN("Message Null");
		CLP_APPMGR_EXIT_FUNCTION();
	      	return CLP_APP_MGR_DBUS_CALL_FAIL;
	}

	dbus_message_iter_init_append(msg, &args);
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &windowid))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}
	if (!dbus_message_iter_append_basic(&args, DBUS_TYPE_INT32, &flag))
	{
		CLP_APPMGR_WARN("Out Of Memory!"); 
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_OUT_OF_MEMORY;
	}

	DBusMessage *reply = dbus_connection_send_with_reply_and_block (appclient_context.bus_conn, msg, -1, &error);
	if (reply == NULL) 
	{ 
	        CLP_APPMGR_WARN_V("Got Reply Null : error: %s", error.message);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_DBUS_REPLY_FAIL;
	}
	
	if (!dbus_message_iter_init(reply, &args))
        	CLP_APPMGR_WARN("Message has no arguments!");
	else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
        	CLP_APPMGR_WARN("Argument is not an integer!");
	else
        	dbus_message_iter_get_basic(&args, &stat);

	dbus_message_unref(msg);
	dbus_message_unref(reply);
	
	if(stat==0)
	{
		CLP_APPMGR_WARN_V(" Window with windowid :%d could not be set to full screen.",windowid);
		CLP_APPMGR_EXIT_FUNCTION();
		return CLP_APP_MGR_FAILURE;
	}
	
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}

/* matchbox window manager support ends here*/

/*
gboolean forceful_power_off_handler ()
{
	pid_t pid;
	
	if((pid=fork()) < 0)
	{
		CLP_APPMGR_WARN("vfork error");
      	}
	else if (pid==0)   //child process. shall exec the 'poweroff' app.
	{
		CLP_APPMGR_INFO("Forceful Exec of the poweroff application..");
		system("/sbin/shutdown.sh");
	}
	return FALSE;

}
*/

/** \brief Power down the terminal 
 *
 * \return -1 - The application not found in the list. 
 * \return CLP_APP_MGR_SUCCESS if succeeded 
 * \return 1 - If the calling application is in focus.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory.
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * Powers the terminal down. Sending a stop signal to all applications.
 */
gint 
clp_app_mgr_power_off(void)
{
	CLP_APPMGR_ENTER_FUNCTION();
	/* Calls PowerOff Method exposed by application Manager*/
	
	//g_timeout_add(15000,forceful_power_off_handler,NULL);
	
	GConfClient *client = gconf_client_get_default();
	gconf_client_set_bool(client,"/appmgr/Shutdown",TRUE,NULL);
	g_object_unref(G_OBJECT(client));
	
	DBusMessage *mesg;
	mesg = dbus_message_new_signal (CLP_APP_MGR_DBUS_OBJECT,CLP_APP_MGR_DBUS_INTERFACE,CLP_APP_MGR_DBUS_SIGNAL_STOP);
	if(mesg == NULL)
        {
       	        CLP_APPMGR_WARN("Not Enough Memory to create new dbus Message");
		CLP_APPMGR_EXIT_FUNCTION();
		return FALSE;
	}
	if (!dbus_connection_send(appclient_context.bus_conn, mesg, 0))
	{
		CLP_APPMGR_WARN("Out Of Memory!");
		CLP_APPMGR_EXIT_FUNCTION();
		return FALSE;
	}

	CLP_APPMGR_INFO("Sent signal 'stop' to all the applications");
	
	system("/sbin/shutdown.sh");

	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_FAILURE;
}


/** \brief Get the application priority 
 *
 * \param pid PID of the application whose priority is to be found out
 * \param our_priority The returned priority by the application manager
 *
 * \return CLP_APP_MGR_SUCCESS - Function returned successfully. 
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory.
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * The function find the priority of the another RUNNING application based on the PID of the same.
 * Applications need not be initialised with the application to exercise this API.
 */
gint clp_app_mgr_get_priority(pid_t pid, guint *our_priority)
{
	CLP_APPMGR_ENTER_FUNCTION();
	GSList *appdirs = NULL;
	GError *err = NULL;
	GConfClient *client = gconf_client_get_default();
	appdirs = gconf_client_all_dirs(client, "/appmgr", &err);
	
	/*traverse the list of applications and read the info*/
	while (appdirs)
	{
		gchar *key_path = g_strconcat(appdirs->data,"/info/", NULL);

		CLP_APPMGR_INFO_V("key_path : %s\n",key_path);
		gchar *temp;
		gint gconf_pid;

		err=NULL;
		temp = g_strconcat (key_path, "PID", NULL);
		gconf_pid = gconf_client_get_int (client, temp, &err);
		g_free(temp);
		
		if(gconf_pid == pid)
		{
			temp = g_strconcat (key_path, "Priority", NULL);
			*our_priority = gconf_client_get_int (client, temp, &err);
			g_free(temp);
			g_free(key_path);
			CLP_APPMGR_INFO_V("Got the app -  With PID - %d Priority = %d", pid, *our_priority);
			CLP_APPMGR_EXIT_FUNCTION();
			return CLP_APP_MGR_SUCCESS;
		}
		g_free(key_path);	
		appdirs=appdirs->next;
	}

	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_FAILURE;
}


/** \brief Visibility of the applicaiton 
 *
 * \param visibility Boolean to change visibility
 *
 * \return CLP_APP_MGR_SUCCESS - successful
 * \return CLP_APP_MGR_FAILURE - failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * Returned back in the get_active_apps call. The application switcher should not display inactive applications.
 * It allows application to be invisibile from switching for periods of time. 
 * 
 */
gint clp_app_mgr_set_visibility(gboolean visibility)
{
	CLP_APPMGR_ENTER_FUNCTION();
	
	gchar app_id[NAME_SIZE];
	GError *err = NULL;
	GConfClient *client = gconf_client_get_default();
	sprintf(app_id, "%d", appclient_context.app_id);
	gchar *key_path = g_strconcat(GCONF_APPS_DIR, "/", appclient_context.app_name,"/info/Visibility", NULL);
	CLP_APPMGR_INFO_V("Key Path - %s\n", key_path);
	gconf_client_set_bool(client, key_path, visibility, &err);
	g_free(key_path);
	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}


static void print_me(GList * list)
{
	GList *temp = list;
	gchar *name=NULL;
	gboolean nodisplay;
	
	while(temp)
	{
		name = ((ClpAppMgrInstalledApp *)(temp->data))->name;
		nodisplay = ((ClpAppMgrInstalledApp *)(temp->data))->nodisplay;
		CLP_APPMGR_INFO_V("Name: %s   NoDisplay: %d",name,nodisplay);
		temp = temp->next;
	}
}


gint
clp_app_mgr_menupos_compare(gpointer a, gpointer b)
{
	return (((ClpAppMgrInstalledApp *)a)->menupos - ((ClpAppMgrInstalledApp *)b)->menupos);
}


/**\brief Get the list of currently installed application
 * 
 * \param appclass Generic Name of the applications to be retrived. Pass NULL to retrive all the apps.
 * 
 * \return GList of ClpAppMgrInstalledApp
 *
 * The function will get the list of currently installed applications in the form of GList. 
 * The data part is ClpAppMgrInstalledApp structure which comtains the required information about the Application.
 */
GList*
clp_app_mgr_get_installed_apps(gchar *appclass)
{
	CLP_APPMGR_ENTER_FUNCTION();
	GList *installed_apps = NULL;

	GSList *appdirs = NULL;
	GError *err = NULL;
	GConfClient *client = gconf_client_get_default();

	appdirs = gconf_client_all_dirs(client, "/appmgr", &err);
	
	/*traverse the list of applications and read the info*/
	while (appdirs)
	{
		ClpAppMgrInstalledApp *app = (ClpAppMgrInstalledApp*) g_malloc0 (sizeof(ClpAppMgrInstalledApp));
		gchar *key_path = g_strconcat(appdirs->data,"/info/", NULL);

		CLP_APPMGR_INFO_V("key_path : %s  -- ",key_path);
		gchar *temp;

		err=NULL;
		temp = g_strconcat (key_path, "Name", NULL);
		app->name = gconf_client_get_string (client, temp, &err);
		g_free(temp);
		if(app->name==NULL)
		{
			CLP_APPMGR_WARN("app->name is NULL. It means the gconf repository is not properly updated.");
			appdirs=appdirs->next;
			continue;
		}
		
		err=NULL;
		temp = g_strconcat (key_path, "Command", NULL);
		if(temp==NULL)
		{
			CLP_APPMGR_WARN("app->exec_name is NULL. It means the gconf repository is not properly updated.");
			appdirs=appdirs->next;
			continue;
		}
		gchar **temp_name = g_strsplit((gchar *)(gconf_client_get_string (client, temp, &err))," ",2);
		app->exec_name = g_strdup(temp_name[0]);
		g_free(temp);
		g_strfreev(temp_name);

		err=NULL;
		temp = g_strconcat (key_path, "GenericName", NULL);
		app->generic_name = gconf_client_get_string (client, temp, &err);
		g_free(temp);
		if(app->generic_name==NULL)
		{
			CLP_APPMGR_WARN("app->generic_name is NULL. It means the gconf repository is not properly updated.");
		}
		
		err=NULL;
		temp = g_strconcat (key_path, "Icon", NULL);
		app->icon = gconf_client_get_string (client, temp, &err);
		g_free(temp);
		if(app->icon==NULL)
		{
			CLP_APPMGR_WARN("app->icon is NULL. It means the gconf repository is not properly updated.");
			app->icon = g_strdup(CLP_APP_MGR_NO_ICON);
		}

		err=NULL;
		temp = g_strconcat (key_path, "NoDisplay", NULL);
		app->nodisplay = gconf_client_get_bool (client, temp, &err);
		g_free(temp);

		err=NULL;
		temp = g_strconcat (key_path, "MenuPath", NULL);
		app->menu_path = gconf_client_get_string (client, temp, &err);
		g_free(temp);
		if(app->menu_path==NULL)
		{
			CLP_APPMGR_WARN("app->menu_path is NULL. It means the gconf repository is not properly updated.");
			app->menu_path = g_strdup("/");
		}
		
		err=NULL;
		temp = g_strconcat (key_path, "MenuPos", NULL);
		app->menupos = gconf_client_get_int (client, temp, &err);
		g_free(temp);
		
		appdirs = appdirs->next;
	
		if(appclass == NULL) {
			CLP_APPMGR_INFO_V("Name: %s    NoDisplay: %d",app->name, app->nodisplay);
			installed_apps = g_list_append(installed_apps, app);
			continue;
		}
		else if(!strcmp(appclass,"menu") || !strcmp(appclass,"/")) {
			if(!strcmp(app->menu_path, "/"))
			{		
				CLP_APPMGR_INFO_V("Name: %s    NoDisplay: %d MenuPath: %s",app->name, app->nodisplay, app->menu_path);
				installed_apps = g_list_append(installed_apps, app);
				continue;
			}
		}
		else {
			if(g_str_has_prefix(app->menu_path, appclass))
			{
				CLP_APPMGR_INFO_V("Name: %s    NoDisplay: %d MenuPath: %s",app->name, app->nodisplay, app->menu_path);
				installed_apps = g_list_append(installed_apps, app);
				continue;
			}
		}
		g_free(key_path);
		g_free(app);
	}

	CLP_APPMGR_EXIT_FUNCTION();
	print_me(installed_apps);
	return installed_apps;
}


/** \brief Get the property of the application
 * 
 * \param application Name of the application
 * \param property The property name
 *
 * \return Return the value of the property
 *
 * The function reads the .desktop file of the provided application and reads the property value
 * and returns it to the user. 
 */
gchar* clp_app_mgr_get_property (const gchar *application, const gchar *property)
{
	CLP_APPMGR_ENTER_FUNCTION();
	GError *load_error = NULL, *error;
	GKeyFile *keyfile;
	gchar *desktop_file=NULL;
	gchar *return_value;
	
	keyfile = g_key_file_new ();
	
	desktop_file = g_strconcat(APPLICATION_INFO_PATH, application, ".desktop", NULL);
	
	g_key_file_load_from_file (keyfile, desktop_file, G_KEY_FILE_NONE, &load_error);
	
	if (load_error != NULL) 
	{
		g_propagate_error (&error, load_error);
		CLP_APPMGR_EXIT_FUNCTION();
		return NULL;
	}
	load_error = NULL;
	return_value = g_key_file_get_value (keyfile, g_key_file_get_start_group (keyfile), property, &load_error);
	if (load_error != NULL) 
	{
		g_propagate_error (&error, load_error);
		CLP_APPMGR_EXIT_FUNCTION();
		return NULL;
	}
	
	g_free(desktop_file);	
	g_key_file_free (keyfile);
	CLP_APPMGR_EXIT_FUNCTION();
	return return_value;
}


/** \brief Set the property of the application
 * 
 * \param application Name of the application
 * \param property The property name
 * \param value The value to be set
 *
 * The function opens the .desktop file of the provided application and sets the property value
 * from the user. 
 */
void clp_app_mgr_set_property (const gchar *application, const gchar *property, const gchar *value)
{
	CLP_APPMGR_ENTER_FUNCTION();
	GError *load_error = NULL, *error=NULL, *write_error=NULL;
	GKeyFile *keyfile;
	gchar *desktop_file=NULL;
	gchar *data;
	gsize length;
	gboolean res;
	
	keyfile = g_key_file_new ();
	
	desktop_file = g_strconcat(APPLICATION_INFO_PATH, application, ".desktop", NULL);

	g_key_file_load_from_file (keyfile, desktop_file, G_KEY_FILE_NONE, &load_error);

	if (load_error != NULL) 
	{
		g_propagate_error (&error, load_error);
		CLP_APPMGR_EXIT_FUNCTION();
		return;
	}
	g_key_file_set_value (keyfile, g_key_file_get_start_group (keyfile), property, value);
	
	data = g_key_file_to_data (keyfile, &length, &write_error);
	if (write_error) {
		g_propagate_error (&error, write_error);
		CLP_APPMGR_EXIT_FUNCTION();
		return;
	}
	write_error = NULL;
	res = g_file_set_contents (desktop_file, data, length, &write_error);
	if (write_error) {
		g_propagate_error (&error, write_error);
		g_free (data);
		CLP_APPMGR_EXIT_FUNCTION();
		return;
	}
	
	g_free(data);
	g_free (desktop_file);	
	g_key_file_free (keyfile);
	CLP_APPMGR_EXIT_FUNCTION();
	return;
}


/** \brief Register message received handler
 *
 * \param message_handler callback function to be called on receival of message 
 *  
 * message_handler is called when the application received the message from another componant or application 
 */
void
clp_app_mgr_register_message_handler(const app_message message_handler)
{
	CLP_APPMGR_ENTER_FUNCTION();
	appclient_context.message_callback = message_handler;
	CLP_APPMGR_EXIT_FUNCTION();
	return;
}

/** \brief Sends the message to another application
 *
 * \param application Name of the application to which the message is to be sent followed by NULL terminated message
 * \param ap va_list form of the message to be sent
 *
 * \return CLP_APP_MGR_SUCCESS - successful
 * \return CLP_APP_MGR_FAILURE - failed.
 * \return CLP_APP_MGR_OUT_OF_MEMORY - Out Of memory
 * \return CLP_APP_MGR_DBUS_CALL_FAIL - DBus Calls failed.
 * \return CLP_APP_MGR_DBUS_REPLY_FAIL - Pending reply Null.
 *
 * The function sends message to another application via dbus. Here the messages is directed to an instance of the application
 * The message can be one or more strings to be passed to the other application. NULL is to be passed at the end.
 */
gint clp_app_mgr_send_message(const gchar *application, va_list ap)
{
	CLP_APPMGR_ENTER_FUNCTION();
	CLP_APPMGR_PARAM_ERROR((application && (strcmp(application, ""))),"Parameter 'application' is NULL");
	CLP_APPMGR_PARAM_ERROR((strlen(application) <= NAME_SIZE),"Parameter 'application' exceeds the maximum allowed name size");
	
	gchar array_sig[2];
	array_sig[0] = DBUS_TYPE_STRING;
	array_sig[1] = '\0';
	gchar *value;
	gchar **message_list;
	guint no_of_param = 1;
	DBusMessage *msg;
	DBusMessageIter args, param_iter;
       	DBusError error;
	dbus_error_init(&error);

	gchar **split = g_strsplit(application,":",2);
	gchar *dbusinterface = g_strconcat (CLP_APP_MGR_DBUS_INTERFACE,".", split[0], split[1], NULL);
	gchar *dbusobject = g_strconcat (CLP_APP_MGR_DBUS_OBJECT, "/", split[0], split[1], NULL);

	CLP_APPMGR_INFO_V("Sending Message to %s application on %s interface and %s objectpath !", application, dbusinterface, dbusobject);

	msg = dbus_message_new_signal (dbusobject, dbusinterface, CLP_APP_MGR_DBUS_SIGNAL_MESSAGE);
	
	if (NULL == msg)
       	{ 
	 	CLP_APPMGR_WARN("Message Null");
		CLP_APPMGR_EXIT_FUNCTION();
      		return CLP_APP_MGR_DBUS_CALL_FAIL;
	}
	/*append arguments*/
	
	value = va_arg(ap, char*);
	int i = 0;
	message_list = (gchar **)g_malloc0((sizeof(gchar *))* MAX_SIZE);
	while (value) {
		CLP_APPMGR_INFO_V("Message Param: %s %d", value, no_of_param);
		message_list[i++] = g_strdup(value);
		value = va_arg(ap, char*);
		no_of_param++;
	}

	dbus_message_iter_init_append(msg, &args);
	dbus_message_iter_append_basic(&args, DBUS_TYPE_UINT32, &no_of_param);
	dbus_message_iter_open_container(&args, DBUS_TYPE_ARRAY, array_sig, &param_iter);
	dbus_message_iter_append_basic(&param_iter, DBUS_TYPE_STRING, &application);

	for(i=1; i<no_of_param; i++)
		dbus_message_iter_append_basic(&param_iter, DBUS_TYPE_STRING, &message_list[i-1]);

	dbus_message_iter_close_container(&args, &param_iter);

	CLP_APPMGR_INFO_V("Sending message to App: %s No of Param %d from %s(%d) ", application, no_of_param, appclient_context.instance_name, getpid());
	dbus_connection_send (appclient_context.bus_conn, msg, NULL);

	for (i=0;i<no_of_param-1;i++)
		g_free(message_list[i]);
	g_free(message_list);

	va_end(ap);
	dbus_message_unref(msg);

	CLP_APPMGR_EXIT_FUNCTION();
	return CLP_APP_MGR_SUCCESS;
}

