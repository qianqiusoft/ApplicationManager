/** \file clp-app-mgr-config.h
 * \brief Application Manager Config Header for Linux Platform
 *
 * This file contains the MACROS and other global declarations required for the Application Manager Library and Daemon.
 */

#ifndef __CLP_APP_MGR_CONFIG_H__
#define __CLP_APP_MGR_CONFIG_H__

#include <amplog.h>

#define CLP_APP_MGR_DAEMON_NAME			"ClpAppMgrDaemon"
#define GCONF_APPS_DIR				"/appmgr"
#define LIBSEGFAULT                             "/usr/lib/libSegFault.so"
#define JVM					"runMidlet"
#define CLP_APP_PATH				"CLP_APP_PATH"

#define CLP_APP_MGR_VENDOR_SERVICE      	"org.freedesktop.DBus"  	/**< DBUS Service */
#define CLP_APP_MGR_VENDOR_INTERFACE    	"org.freedesktop.DBus"  	/**< DBUS Interface */
#define CLP_APP_MGR_VENDOR_OBJECT       	"/org/freedesktop/DBus" 	/**< DBUS Objectpath */

#define CLP_APP_MGR_DBUS_SERVICE        	"org.clp.appmanager"            /**< Application Manager Service name */
#define CLP_APP_MGR_DBUS_INTERFACE      	"org.clp.appmanager"            /**< Application Manager Interface name*/
#define CLP_APP_MGR_DBUS_OBJECT         	"/org/clp/appmanager"           /**< Application Manager Object name*/

#define CLP_WIN_MGR_DBUS_SERVICE        	"org.clp.matchboxwm"            /**< Matchbox Window Manager Service name */
#define CLP_WIN_MGR_DBUS_INTERFACE      	"org.clp.matchboxwm"            /**< Matchbox Window Manager Interface name*/
#define CLP_WIN_MGR_DBUS_OBJECT         	"/org/clp/matchboxwm"           /**< Matchbox Window Manager Object name*/

#define CLP_PANEL_DBUS_SERVICE			"org.celunite.PanelText"	/**< fbPanel Service Name */
#define CLP_PANEL_DBUS_INTERFACE		"org.celunite.PanelText"	/**< fbPanel Interface Name */
#define CLP_PANEL_DBUS_OBJECT  			"/org/celunite/PanelText"	/**< fbPanel Object Path Name */

#define CLP_JVM_DBUS_SERVICE			"org.clp.application.phoneME"	/**< phoneME Service Name */
#define CLP_JVM_DBUS_INTERFACE			"org.clp.application.phoneME"	/**< phoneME Interface Name */
#define CLP_JVM_DBUS_OBJECT  			"/org/clp/application/phoneME"	/**< phoneME Object Path Name */

#define CLP_LIMO_AMS_DBUS_SERVICE		"am.dbus.interface"		/**< LIMO AMS Service Name */
#define CLP_LIMO_AMS_DBUS_INTERFACE		"am.dbus.interface"		/**< LIMO AMS Interface Name */
#define CLP_LIMO_AMS_DBUS_OBJECT		"/app_manager"			/**< LIMO AMS Object Path Name */

#define CLP_APP_MGR_RESOURCE_AUDIO              "Audio"                 	/**< Audio Resource */
#define CLP_APP_MGR_RESOURCE_VIDEO              "Video"                 	/**< Video Resource */

#define CLP_APP_MGR_EXEC_TYPE_JAVA		"java"				/**< Java executable type */
#define CLP_APP_MGR_EXEC_TYPE_ELF		"elf"	                        /**< ELF Executable type */

#define CLP_APP_MGR_DBUS_SIGNAL_PAUSE           	"pause"                	/**< 'pause' dbus signal */
#define CLP_APP_MGR_DBUS_SIGNAL_STOP            	"stop"                 	/**< 'stop' dbus signal */
#define CLP_APP_MGR_DBUS_SIGNAL_RESUME          	"resume"               	/**< 'resume' dbus signal */
#define CLP_APP_MGR_DBUS_SIGNAL_THEMECHANGE     	"themechange"          	/**< 'themechange' dbus signal */
#define CLP_APP_MGR_DBUS_SIGNAL_CLEARPID		"ClearPID"		/**< 'clearpid' dbus signal */	
#define CLP_APP_MGR_DBUS_SIGNAL_EXEC			"exec"			/**< 'exec' dbus signal */
#define CLP_APP_MGR_DBUS_SIGNAL_APPLIST_CHANGE		"applistchange"		/**< 'applist change ' dbus signal */
#define CLP_APP_MGR_DBUS_SIGNAL_ROTATE			"rotate"		/**< 'rotate state change ' dbus signal */
#define CLP_APP_MGR_DBUS_SIGNAL_APPEXIT			"AppExit"		/**< 'AppExit' dbus signal */
#define CLP_WIN_MGR_DBUS_SIGNAL_UA_GAINED		"UserInteractionGained"	/**< 'UserInteractionGained' dbus signal */
#define CLP_WIN_MGR_DBUS_SIGNAL_UA_LOST			"UserInteractionLost"	/**< 'UserInteractionLost' dbus signal */
#define CLP_APP_MGR_DBUS_SIGNAL_FOCUS_LOST		"FocusLost"		/**< 'FocusLost' dbus signal */
#define CLP_APP_MGR_DBUS_SIGNAL_FOCUS_GAINED		"FocusGained"		/**< 'FocusGained' dbus signal */
#define CLP_APP_MGR_DBUS_SIGNAL_MESSAGE			"Message"		/**< 'Message' dbus signal */

#define CLP_APP_MGR_APP_INIT_METHOD             	"AppInit"              	/**< AppInit Method exported by Application Manager Daemon*/
#define CLP_APP_MGR_APP_EXEC_METHOD             	"AppExec"              	/**< AppExec Method exported by Application Manager Daemon*/
#define CLP_APP_MGR_APP_CLOSE_METHOD            	"AppClose"             	/**< AppClose Method exported by Application Manager Daemon*/
#define CLP_APP_MGR_APP_CLOSE_BY_NAME_METHOD           	"AppCloseByName"       	/**< AppCloseByName Method exported by Application Manager Daemon*/
#define CLP_APP_MGR_GET_ACTIVE_APPS_METHOD		"GetActiveApps"		/**< GetActiveApps Method exported by Application Mangaer Daemon*/
#define CLP_APP_MGR_GET_NUM_OF_ACTIVE_APPS_METHOD	"GetNumOfActiveApps"	/**< GetNumOfActiveApps Method exported by Application Mangaer Daemon*/
#define CLP_APP_MGR_GET_NUM_OF_ACTIVE_INSTANCES_OF_APP_METHOD	"GetNumOfActiveInstOfApp"	/**< GetNumOfActiveInstOfApp Method exported by Application Mangaer Daemon*/
#define CLP_APP_MGR_IS_APP_ACTIVE_METHOD		"IsAppActive"		/**< IsAppActive Method exported by Application Mangaer Daemon*/
#define CLP_APP_MGR_GET_APPLICATION_ID_METHOD		"GetApplicationId"	/**< GetApplicationId Method exported by Application Mangaer Daemon*/
#define CLP_APP_MGR_GET_ACTIVE_INSTANCES_OF_APP_METHOD	"GetActiveInstOfApp"	/**< GetActiveInstOfApp Method exported by Application Mangaer Daemon*/
#define CLP_APP_MGR_GET_INSTANCE_INFO_METHOD		"GetInstanceInfo"	/**< GetInstanceInfo Method exported by Application Mangaer Daemon*/
#define CLP_APP_MGR_APP_MINIMIZE_METHOD			"AppMinimize"		/**< AppMinimize Method exported by Application Mangaer Daemon*/
#define CLP_APP_MGR_APP_RESTORE_METHOD			"AppRestore"		/**< AppRestore Method exported by Application Mangaer Daemon*/
#define CLP_APP_MGR_POWER_OFF_METHOD			"PowerOff"		/**< PowerOff Method exported by Application Manager Daemon*/	
#define CLP_APP_MGR_ROTATE_WINDOW_METHOD		"RotateWindow"		/**< RotateWindow Method exported by Application Manager Daemon*/	
#define CLP_APP_MGR_GET_PRIORITY_METHOD			"GetPriority"		/**< GetPriority Method exported by Application Manager Daemon*/
#define CLP_APP_MGR_SERVICE_DISCOVER_METHOD		"ServiceDiscover"	/**< Service Discover Method exported by Application Manager Daemon*/	
#define CLP_APP_MGR_HANDLE_CONTENT_METHOD		"HandleContent"		/**< Handle Content Method exported by Application Manager Daemon*/	
#define CLP_APP_MGR_SET_VISIBILITY_METHOD		"SetVisibility"		/**< Set VIsibility Method exported by Application Manager Daemon*/	
#define CLP_APP_MGR_GET_INSTALLED_APPS_METHOD		"GetInstalledApps"	/**< GetInstalledApps Method exported by Application Mangaer Daemon*/

/* Methods exported by window manager*/
#define CLP_WIN_MGR_GET_WINDOW_LIST_METHOD		"WindowList"		/**< WindowList Method exported by Window Manager */	
#define CLP_WIN_MGR_SET_LOCK_METHOD			"SetLock"		/**< SetLock Method exported by Window Manager */	
#define CLP_WIN_MGR_FOCUS_PID_METHOD			"FocusPID"		/**< FocusPID Method exported by Window Manager */	
#define CLP_WIN_MGR_FOCUS_ID_METHOD			"FocusID"		/**< FocusID Method exported by Window Manager */	
#define CLP_WIN_MGR_MINIMIZE_ID_METHOD			"MinimizeID"		/**< MinimizeID Method exported by Window Manager */	
#define CLP_WIN_MGR_MINIMIZE_PID_METHOD			"MinimizePID"		/**< MinimizePID Method exported by Window Manager */	
#define CLP_WIN_MGR_GET_SCREEN_DIMENSIONS_METHOD	"ScreenDimensions"	/**< ScreenDimensions Method exported by Window Manager */	
#define CLP_WIN_MGR_SET_WINDOW_PRIORITY_METHOD		"SetWindowPriority"	/**< SetWindowPriority Method exported by Window Manager */	
#define CLP_WIN_MGR_MOVE_RESIZE_WINDOW_METHOD		"MoveResizeWindow"	/**< MoveResizeWindow Method exported by Window Manager */	
#define CLP_WIN_MGR_FULL_SCREEN_WINDOW_METHOD		"FullScreenWindow"	/**< FullScreenWindow Method exported by Window Manager */	
#define CLP_WIN_MGR_TOGGLE_FULL_SCREEN_WINDOW_METHOD	"ToggleFullscreen"	/**< ToggleFullScreenWindow Method exported by Window Manager */	
#define CLP_WIN_MGR_GET_TOP_WINDOW_OF_APP_METHOD	"TopWindow"		/**< TopWindow Method exported by Window Manager */	

/* Signals exported by Java VM */
#define CLP_JVM_LAUNCH_MIDLET_SIGNAL			"launch_midlet"		/**< launch_midlet signal for java apps */
#define CLP_JVM_RESTORE_MIDLET_SIGNAL			"restore_midlet"	/**< restore_midlet signal for java apps */
#define CLP_JVM_MINIMIZE_MIDLET_SIGNAL			"minimize_midlet"	/**< restore_midlet signal for java apps */
#define CLP_JVM_STOP_MIDLET_SIGNAL			"stop_midlet"		/**< restore_midlet signal for java apps */

/* Logging related stuff*/
#undef	G_LOG_DOMAIN
#define	G_LOG_DOMAIN "ClpAppMgr"

#ifdef AMP_LOG_LEVEL_DEBUG
#define	CLP_APPMGR_ENTER_FUNCTION()		AMP_LOG_DEBUG_V("%s{", __PRETTY_FUNCTION__)
#define	CLP_APPMGR_EXIT_FUNCTION()		AMP_LOG_DEBUG_V("%s}", __PRETTY_FUNCTION__)
#define	CLP_APPMGR_EXIT_FUNCTION_VIA(s)		AMP_LOG_DEBUG_V("%s}via '%s'", __PRETTY_FUNCTION__, s)
#define CLP_APPMGR_INFO(s)			AMP_LOG_DEBUG(s)
#define CLP_APPMGR_INFO_V(s, ...)		AMP_LOG_DEBUG_V(s, __VA_ARGS__)
#define CLP_APPMGR_WARN(s)			AMP_LOG_WARNING(s)
#define CLP_APPMGR_WARN_V(s, ...)		AMP_LOG_WARNING_V(s, __VA_ARGS__)
#define CLP_APPMGR_ERROR(s)			AMP_LOG_ERROR(s)
#define CLP_APPMGR_ERROR_V(s, ...)		AMP_LOG_ERROR_V(s, __VA_ARGS__)
#define CLP_APPMGR_PARAM_ERROR(cond, s)		if(!cond) AMP_LOG_ERROR(s)
#endif

#ifdef AMP_LOG_LEVEL_INFO
#define	CLP_APPMGR_ENTER_FUNCTION()		G_STMT_START{ (void)0; }G_STMT_END
#define	CLP_APPMGR_EXIT_FUNCTION()		G_STMT_START{ (void)0; }G_STMT_END
#define	CLP_APPMGR_EXIT_FUNCTION_VIA(s)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_INFO(s)			G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_INFO_V(s, ...)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_WARN(s)			AMP_LOG_WARNING(s)
#define CLP_APPMGR_WARN_V(s, ...)		AMP_LOG_WARNING_V(s, __VA_ARGS__)
#define CLP_APPMGR_ERROR(s)			AMP_LOG_ERROR(s)
#define CLP_APPMGR_ERROR_V(s, ...)		AMP_LOG_ERROR_V(s, __VA_ARGS__)
#define CLP_APPMGR_PARAM_ERROR(cond, s)		if(!cond) AMP_LOG_ERROR(s)
#endif

#ifdef AMP_LOG_LEVEL_MESSAGE
#define	CLP_APPMGR_ENTER_FUNCTION()		G_STMT_START{ (void)0; }G_STMT_END
#define	CLP_APPMGR_EXIT_FUNCTION()		G_STMT_START{ (void)0; }G_STMT_END
#define	CLP_APPMGR_EXIT_FUNCTION_VIA(s)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_INFO(s)			G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_INFO_V(s, ...)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_WARN(s)			AMP_LOG_WARNING(s)
#define CLP_APPMGR_WARN_V(s, ...)		AMP_LOG_WARNING_V(s, __VA_ARGS__)
#define CLP_APPMGR_ERROR(s)			AMP_LOG_ERROR(s)
#define CLP_APPMGR_ERROR_V(s, ...)		AMP_LOG_ERROR_V(s, __VA_ARGS__)
#define CLP_APPMGR_PARAM_ERROR(cond, s)		if(!cond) AMP_LOG_ERROR(s)
#endif

#ifdef AMP_LOG_LEVEL_WARNING
#define	CLP_APPMGR_ENTER_FUNCTION()		G_STMT_START{ (void)0; }G_STMT_END
#define	CLP_APPMGR_EXIT_FUNCTION()		G_STMT_START{ (void)0; }G_STMT_END
#define	CLP_APPMGR_EXIT_FUNCTION_VIA(s)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_INFO(s)			G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_INFO_V(s, ...)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_WARN(s)			AMP_LOG_WARNING(s)
#define CLP_APPMGR_WARN_V(s, ...)		AMP_LOG_WARNING_V(s, __VA_ARGS__)
#define CLP_APPMGR_ERROR(s)			AMP_LOG_ERROR(s)
#define CLP_APPMGR_ERROR_V(s, ...)		AMP_LOG_ERROR_V(s, __VA_ARGS__)
#define CLP_APPMGR_PARAM_ERROR(cond, s)		if(!cond) AMP_LOG_ERROR(s)
#endif

#ifdef AMP_LOG_LEVEL_CRITICAL
#define	CLP_APPMGR_ENTER_FUNCTION()		G_STMT_START{ (void)0; }G_STMT_END
#define	CLP_APPMGR_EXIT_FUNCTION()		G_STMT_START{ (void)0; }G_STMT_END
#define	CLP_APPMGR_EXIT_FUNCTION_VIA(s)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_INFO(s)			G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_INFO_V(s, ...)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_WARN(s)			G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_WARN_V(s, ...)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_ERROR(s)			AMP_LOG_ERROR(s)
#define CLP_APPMGR_ERROR_V(s, ...)		AMP_LOG_ERROR_V(s, __VA_ARGS__)
#define CLP_APPMGR_PARAM_ERROR(cond, s)		if(!cond) AMP_LOG_ERROR(s)
#endif

#ifdef AMP_LOG_LEVEL_ERROR
#define	CLP_APPMGR_ENTER_FUNCTION()		G_STMT_START{ (void)0; }G_STMT_END
#define	CLP_APPMGR_EXIT_FUNCTION()		G_STMT_START{ (void)0; }G_STMT_END
#define	CLP_APPMGR_EXIT_FUNCTION_VIA(s)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_INFO(s)			G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_INFO_V(s, ...)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_WARN(s)			G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_WARN_V(s, ...)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_ERROR(s)			AMP_LOG_ERROR(s)
#define CLP_APPMGR_ERROR_V(s, ...)		AMP_LOG_ERROR_V(s, __VA_ARGS__)
#define CLP_APPMGR_PARAM_ERROR(cond, s)		if(!cond) AMP_LOG_ERROR(s)
#endif

#ifdef AMP_LOG_LEVEL_NONE
#define	CLP_APPMGR_ENTER_FUNCTION()		G_STMT_START{ (void)0; }G_STMT_END
#define	CLP_APPMGR_EXIT_FUNCTION()		G_STMT_START{ (void)0; }G_STMT_END
#define	CLP_APPMGR_EXIT_FUNCTION_VIA(s)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_INFO(s)			G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_INFO_V(s, ...)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_WARN(s)			G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_WARN_V(s, ...)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_ERROR(s)			G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_ERROR_V(s, ...)		G_STMT_START{ (void)0; }G_STMT_END
#define CLP_APPMGR_PARAM_ERROR(cond, s)		G_STMT_START{ (void)0; }G_STMT_END
#endif


#include <dlfcn.h>
static inline 
void load_libsegfault (void)
{
	void * libsegfault_handle = dlopen ( LIBSEGFAULT, RTLD_NOW);
	if (!libsegfault_handle)
	  CLP_APPMGR_WARN ("Failed to load libsegfault handler:!");
}

#endif /*__CLP_APP_MGR_CONFIG_H__ */

