/** \file clp-app-mgr.h
 * \brief Application Manager System Object for Celunite Linux Platform
 * 
 * \author Gaurav Roy [gaurav.roy@celunite.com]
 * \author Abhiraj Butala [abhiraj.butala@celunite.com]
 * \author Kaustubh Atrawalkar [kaustubh.atrawalkar@celunite.com]
 * 
 * The APIs to be used by the applications to access application manager are declared here. 
 */



#ifndef __CLP_APP_MGR_H__

#define __CLP_APP_MGR_H__

#include <glib.h>

#define CLP_APP_MGR_ENTRY_POINT			"main" /**< Entry point of the shared object application */


#define CLP_APP_MGR_PRIORITY_CRITICAL		0		/**< Supercritical Application Highest Priority */
#define CLP_APP_MGR_PRIORITY_NORMAL		10		/**< Normal Application Like Browser etc */
#define CLP_APP_MGR_PRIORITY_LOW		100		/**< Lower priority application that may run in background */
#define CLP_APP_MGR_PRIORITY_NICE_APP		1000		/**< Application should run only when noone else is around */

#define NAME_SIZE            	 	        256     	/**< The maximum size of the application's name*/
#define MAX_SIZE                        	256             /**< The maximum size for dbus connections*/
#define NO_APPLICATION                        	"none"          /**< Returned when no application exists which matches the given criteria */



enum _ClpAppMgrErrorCodes					/**< Standard Error Codes for Application Manager*/
{
	CLP_APP_MGR_FAILURE		= -1,			/**< The function failed to return */
	CLP_APP_MGR_SUCCESS 		= 0,			/**< The function executed successfully */
	CLP_APP_MGR_OUT_OF_MEMORY	= 0xd0,			/**< Memory overflow error */
	CLP_APP_MGR_DBUS_CALL_FAIL	= 0xd1,			/**< Dbus call failed error */
	CLP_APP_MGR_DBUS_REPLY_FAIL	= 0xd2,			/**< Reply to the method call failed */
	CLP_APP_MGR_LIB_NOTIFY_FAIL	= 0xd3,			/**< Lib notify error */
	CLP_APPMGR_GTK_FAIL		= 0xd4,			/**< Gtk error */
	CLP_APP_MGR_DLAPP_FAIL		= 0xd5,			/**< Dynamic Symbol resolution error */
	CLP_APP_MGR_INIT_FAILURE	= 0xd6			/**< Init failure */
};

struct _ClpAppMgrActiveApp					/**< Struct for active application info */
{
	gint pid;						/**< pid of the application */
	gchar name[NAME_SIZE];					/**< instance name of the application */
	gchar title[NAME_SIZE];					/**< title of the application */
	gchar *icon;						/**< icon of the application */
	gboolean visibility;					/**< visibility of the application */
	gboolean immortal;					/**< immortality of the application */
};

struct _ClpAppMgrInstalledApp					/**< Struct for installed application info */
{
	gchar *name;						/**< name of the application */
	gchar *generic_name;					/**< generic name (class) of the application */
	gchar *icon;						/**< icon of the application */
	gchar *exec_name;					/**< exec name of the application */
	gchar *menu_path;					/**< menu path of the application */
	gboolean nodisplay;					/**< nodisplay flag (whether to display in menus) */
	gint menupos;						/**< menu position of the application */
};


/*window manager */
typedef struct _ClpAppMgrWindowInfo
{
        gint pid;
        guint windowid;
        gchar *icon;
        gchar *title;

}ClpAppMgrWindowInfo;

typedef struct _ClpAppMgrWinResizeInfo
{
	gint windowid;
	gint x_move;
	gint y_move;
	gint width;
	gint height;

}ClpAppMgrWinResizeInfo;
/*window manager end */



typedef enum _ClpAppMgrErrorCodes ClpAppMgrErrorCodes;		/**< typedef for enum for error codes */
typedef struct _ClpAppMgrActiveApp ClpAppMgrActiveApp;		/**< typedef for Active apps structure */
typedef struct _ClpAppMgrInstalledApp ClpAppMgrInstalledApp;	/**< typedef for Installed apps struct */

/* API for switiching off the cell ! */
gint clp_app_mgr_power_off(void);

/* APIs for Window manager Support*/
GSList* clp_app_mgr_wm_get_window_list();
gint clp_app_mgr_wm_get_screen_exclusive();
gint clp_app_mgr_wm_release_screen();
gint clp_app_mgr_wm_restore_application(gint pid);
gint clp_app_mgr_wm_restore_window(gint windowid);
gint clp_app_mgr_wm_minimize_application(gint pid);
gint clp_app_mgr_wm_minimize_window(gint windowid);
gint clp_app_mgr_wm_get_available_screen_dimensions(gint *height, gint *width);
gint clp_app_mgr_wm_set_window_priority(gint windowid, gint priority);
gint clp_app_mgr_wm_move_resize_window(ClpAppMgrWinResizeInfo resizeinfo);
gint clp_app_mgr_wm_fullscreen_window(gint windowid, gint flag);
gint clp_app_mgr_wm_toggle_fullscreen_window(void);
gint clp_app_mgr_wm_get_top_window_of_application(gint pid, gchar **top_window);

/* Themeing Support */
GSList* clp_app_mgr_get_installed_themes();
gint clp_app_mgr_apply_theme(const gchar *theme);

/* Querying of information of active applications*/
GList* clp_app_mgr_get_active_apps();
gint clp_app_mgr_get_num_of_active_apps();   				
GList* clp_app_mgr_get_active_instances_of_app(gchar *appname); 	
gint clp_app_mgr_get_num_of_active_instances_of_app(gchar *appname);	
gint clp_app_mgr_is_app_active(gchar *appname);				
gchar* clp_app_mgr_get_application_id(gint pid);			
ClpAppMgrActiveApp *clp_app_mgr_get_application_instance_info(gchar *instance_name);	

/* Querying of information of installed applications*/
GList* clp_app_mgr_get_installed_apps(gchar *appclass);

/* application configuration APIs */
gchar* clp_app_mgr_get_property (const gchar *application, const gchar *property);
void clp_app_mgr_set_property (const gchar *application, const gchar *property, const gchar *value);

#endif

