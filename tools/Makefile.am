CPPFLAGS = $(GTK_CFLAGS) $(DBUS_CFLAGS) $(LIBXDGMIME_CFLAGS) -Wall
LDFLAGS = $(GTK_LIBS) $(DBUS_LIBS) $(LIBXDGMIME_LIBS) -L../src  -lclpappmgr -lappmgr -lappmgr-ids -lnoti

bin_PROGRAMS =  app-launcher        
				

app_launcher_SOURCES = clp-app-mgr-app-launcher.c

MAINTAINERCLEANFILES =	\
	Makefile	\
	Makefile.in	\
	core		\
	svn-commit*	\
	*~		
