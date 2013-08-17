dnl Process this file with autoconf to produce a configure script.



AC_INIT(src/clp-app-mgr-lib.h)
AM_CONFIG_HEADER(config.h)

PACKAGE=appmgr
VERSION=0.9

AM_INIT_AUTOMAKE($PACKAGE,$VERSION)

AC_PROG_CC
AC_STDC_HEADERS
AC_PROG_INSTALL

PKG_CHECK_MODULES(GTK, gtk+-2.0 >= 2.2)
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)

PKG_CHECK_MODULES(DBUS, dbus-glib-1)
AC_SUBST(DBUS_CFLAGS)
AC_SUBST(DBUS_LIBS)

PKG_CHECK_MODULES(LIBXDGMIME, xdgmime)
AC_SUBST(LIBXDGMIME_CFLAGS)
AC_SUBST(LIBXDGMIME_LIBS)


PKG_CHECK_MODULES(GCONF, gconf-2.0)
AC_SUBST(GCONF_CFLAGS)
AC_SUBST(GCONF_LIBS)

PKG_CHECK_MODULES(AMPLOG, amplog >= 1.0)
AC_SUBST(AMPLOG_CFLAGS)
AC_SUBST(AMPLOG_LIBS)


AC_SUBST(ENABLE_FREEZEMGR)
AC_ARG_ENABLE(freezemgr,
             AC_HELP_STRING([--enable-freezemgr=@<:@no/yes@:>@],
                            [turn on freeze @<:@default=no@:>@]),,
                            enable_freezemgr="no")
if test "x$enable_freezemgr" = "xyes"; then
	PKG_CHECK_MODULES(FREEZEMGR,freezemgr)
	AC_SUBST(FREEZEMGR_CFLAGS)
	AC_SUBST(FREEZEMGR_LIBS)
	ENABLE_FREEZEMGR="-DENABLE_FREEZEMGR"
fi

AC_DISABLE_STATIC
AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

dnl Options
AC_SUBST(AMP_LOG_LEVEL)

AC_ARG_ENABLE(log_level,
       	      AC_HELP_STRING([--enable-log-level=@<:@none/error/critical/warning/message/info/debug@:>@],
     	                     [specify logging level @<:@default=none@:>@]),,
     		              enable_log_level="warning")

if test "x$enable_log_level" = "xerror"; then
	AMP_LOG_LEVEL="-DAMP_LOG_LEVEL_ERROR"
elif test "x$enable_log_level" = "xcritical"; then
	AMP_LOG_LEVEL="-DAMP_LOG_LEVEL_CRITICAL"
elif test "x$enable_log_level" = "xwarning"; then
	AMP_LOG_LEVEL="-DAMP_LOG_LEVEL_WARNING"
elif test "x$enable_log_level" = "xmessage"; then
	AMP_LOG_LEVEL="-DAMP_LOG_LEVEL_MESSAGE"
elif test "x$enable_log_level" = "xinfo"; then
	AMP_LOG_LEVEL="-DAMP_LOG_LEVEL_INFO"
elif test "x$enable_log_level" = "xdebug"; then
	AMP_LOG_LEVEL="-DAMP_LOG_LEVEL_DEBUG"
else
	AMP_LOG_LEVEL="-DAMP_LOG_LEVEL_NONE"
fi

AC_OUTPUT([
appmgr.pc
Makefile
src/Makefile 
tools/Makefile
docs/Doxyfile
])
