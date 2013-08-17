CFLAGS = $(GTK_CFLAGS) $(DBUS_CFLAGS) $(GCONF_CFLAGS) $(LIBXDGMIME_CFLAGS) $(AMPLOG_CFLAGS) -DCLP_APP_MGR_LOG_DIR=\"${localstatedir}"/log"\" -DCLP_APP_MGR_DATA_DIR=\"${datadir}"/appmgr/"\"
CFLAGS += $(ENABLE_FREEZEMGR) $(FREEZEMGR_CFLAGS) $(AMP_LOG_LEVEL) #Add the logging severity/level flags
CFLAGS += -DG_LOG_DOMAIN=\"AmpClpAppMgr\" #Define log domain macro
LDFLAGS += $(FREEZEMGR_LIBS) $(GTK_LIBS) $(DBUS_LIBS) $(GCONF_LIBS) $(LIBXDGMIME_LIBS) $(AMPLOG_LIBS)  -ldl -lappmgr
INCLUDES = $(DBUS_CFLAGS) $(GCONF_CFLAGS) $(LIBXDGMIME_CFLAGS) $(AMPLOG_CFLAGS) -Wall -DAPPLICATION_EXEC_PATH=\"${bindir}"/"\" -DCLP_APP_MGR_NO_ICON=\"$(datadir)"/appmgr/images/noimage.png"\" -DREAD_THEME_DIR=\"$(sysconfdir)\" -DAPPLICATION_INFO_PATH=\"$(datadir)"/applications/"\"


mylibdir = $(libdir)
mylib_PROGRAMS =  libclpappmgr.so
libclpappmgr_so_SOURCES = limo-app-mgr-lib.c clp-app-mgr-lib.h clp-app-mgr-config.h clp-app-mgr.h
libclpappmgr_so_LDFLAGS = -shared -fPIC
libclpappmgrincludedir = $(includedir)
libclpappmgrinclude_HEADERS = clp-app-mgr-lib.h clp-app-mgr.h

#Dbus configuration file
dbusconfdir = $(sysconfdir)/dbus-1/system.d
dbusconf_in_files = clp-app-mgr.conf.in
dbusconf_DATA = $(dbusconf_in_files:.conf.in=.conf)


# Rule to make the conf file with datadir expanded
$(dbusconf_DATA): $(dbusconf_in_files) Makefile
	sed -e "s|@datadir1@|$(datadir)|" $< > $@
	test -z $(localstatedir)/log || mkdir -p -- . $(localstatedir)/log
	test -z $(datadir)/appmgr || mkdir -p -- . $(datadir)/appmgr

DISTCLEANFILES =			\
	$(dbusconf_DATA)

MAINTAINERCLEANFILES =	\
	*~		\
	core		\
	svn-commit*	\
	Makefile.in
