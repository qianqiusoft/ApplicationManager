/** \file clp-app-mgr-lib.h
 * \brief Application Manager Library header for Celunite Linux Platform
 * 
 * \author Gaurav Roy [gaurav.roy@celunite.com]
 * \author Abhiraj Butala [abhiraj.butala@celunite.com]
 * \author Kaustubh Atrawalkar [kaustubh.atrawalkar@celunite.com]
 * 
 * The APIs to be used by the application developer are declared here. 
 */

#ifndef __CLP_APP_MGR_LIB_H__
#define __CLP_APP_MGR_LIB_H__

#include <glib.h>
#include "clp-app-mgr.h"

#ifdef ENABLE_FREEZEMGR
#include <clp_freeze_mgr_lib.h>
#include <clp_restore_mgr_lib.h>
#else
int connect_to_restoredaemon();
int restore_app(char *appname);
int freezeme(char *appname);
#endif

#define CLP_SCREEN_HEIGHT			320		/**< Screen height for Celunite Linux Platform */
#define CLP_SCREEN_WIDTH			240		/**< Screen width for Celunite Linux Platform */
#define CLP_PANEL_HEIGHT			20		/**< Panel height for Celunite Linux Platform */


enum _ClpAppMgrRotationType					/**< Enum for type of Rotation*/
{
	CLP_APP_MGR_CLOCKWISE=0,				/**< Rotate the window in clockwise direction*/
	CLP_APP_MGR_ANTICLOCKWISE 				/**< Rotate the window in anticlockwise direction>*/
};

enum _ClpAppMgrInstanceType					/**< Enum for type of Instance Support*/
{
	CLP_APP_MGR_SINGLE=0,					/**< Single Instance application */
	CLP_APP_MGR_MULTIPLE					/**< Multiple Instance application */
};

/* service discovery */
typedef struct _ClpAppMgrServices				/**< struct for getting service information */
{
	gchar 		*app_name;				/**< Name of the application which provides this service */
	gchar 		*app_exec_name;				/**< Executable Name of the application which provides this service */
	gchar 		*service_name;				/**< name of the service */
	gchar		*service_menu;				/**< Menu string to be displayed about the service */

}ClpAppMgrServices;
/* service discovery end */

typedef enum _ClpAppMgrRotationType ClpAppMgrRotationType;	/**< typedef for Enum for type of rotation */
typedef enum _ClpAppMgrInstanceType ClpAppMgrInstanceType;	/**< typedef for Enum for type of application */

/* Functions to be registerd */
typedef void (*app_pause) (void *);    				/**< function pointer for pause handler*/
typedef void (*app_stop) (void *); 				/**< function pointer for stop handler*/
typedef void (*app_resume) (void *);				/**< function pointer for resume handler*/
typedef void (*app_rotate) (void *, int);			/**< function pointer for rotate handler*/
typedef void (*app_death) (void *, int);			/**< function pointer for app death handler param is pid of dying app*/
typedef void (*app_exec) (guint, gchar **);			/**< function pointer for restore handler*/
typedef void (*app_list_change) (GList *);			/**< function pointer for app list change handler*/
typedef void (*app_message) (guint, gchar **);			/**< function pointer for app messaged*/

typedef void (*app_focus_gained) (void *);    			/**< function pointer for app_ua_gained handler*/
typedef void (*app_focus_lost) (void *);    			/**< function pointer for app_ua_lost handler*/
typedef void (*post_init) (void *);    				/**< function pointer for post_init handler*/


/*APIs for application initialization */
gint clp_app_mgr_init (const gchar *name, const guint priority, const ClpAppMgrInstanceType instance);
gint clp_app_mgr_async_init (const gchar *name, const guint priority, const ClpAppMgrInstanceType instance,const post_init post_init_handler);
gint clp_app_mgr_exec (const gchar *application, ...);
gint clp_app_mgr_exec_application (const gchar *application, const va_list ap);
gint clp_app_mgr_exec_argv (const gchar *application, gint no_of_params, gchar** params_list);
	

gint clp_app_mgr_send_message (const gchar *application, va_list ap);

/* API to get names identities */
gchar* clp_app_mgr_get_name(void);
gchar* clp_app_mgr_get_instance_name(void);

/* API for registering the handlers */
void clp_app_mgr_register_exec_handler(const app_exec exec_handler);
void clp_app_mgr_register_stop_handler(const app_stop stop_handler);
void clp_app_mgr_register_death_handler(const app_death death_handler);
void clp_app_mgr_register_rotate_handler(const app_rotate rotate_handler);
void clp_app_mgr_register_message_handler(const app_message message_handler);
//void clp_app_mgr_register_app_list_change_handler (const app_list_change list_change_handler);
void clp_app_mgr_wm_register_focus_lost_handler(const app_focus_lost focus_lost_handler);
void clp_app_mgr_wm_register_focus_gained_handler(const app_focus_gained focus_gained_handler);

gint clp_app_mgr_set_visibility(gboolean visibility);
gint clp_app_mgr_get_priority(gint pid, guint *our_priority);

/* APIs for restore & close & stop*/
gint clp_app_mgr_restore(const gchar *app);
gint clp_app_mgr_close(void);
gint clp_app_mgr_close_by_name(const gchar *app);
gint clp_app_mgr_close_by_red_key(const gchar *app);
gint clp_app_mgr_stop(const gchar *app);

/* API for Rotation support */
gint clp_app_mgr_rotate(const ClpAppMgrRotationType rotationtype); 


/* API for Service Discovery */
gchar* clp_app_mgr_mime_from_file(const gchar *filename);
gchar* clp_app_mgr_mime_from_string(const gchar *data);
GSList* clp_app_mgr_get_services(const gchar* mimetype);
gint clp_app_mgr_service_invoke(const gchar* application, ...);
gint clp_app_mgr_handle_file(const gchar *filepath);
gint clp_app_mgr_handle_string(const gchar *data);
gint clp_app_mgr_handle_mime(const gchar *mime_type, const gchar *mime_data);

#endif /*__CLP_APP_MGR_LIB_H__ */

