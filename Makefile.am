SUBDIRS = src tools

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA= appmgr.pc

.PHONY: doxygen
doxygen: docs/Doxyfile
	rm -rf apidocs
	mkdir apidocs
	doxygen docs/Doxyfile

MAINTAINERCLEANFILES =	\
	aclocal.m4	\
	autom4te.cache	\
	config.h	\
	config.h.in	\
	config.log	\
	config.status	\
	configure	\
	depcomp		\
	clpapp.pc	\
	Makefile.in	\
	install-sh	\
	missing		\
	stamp-h1	\
	config.guess	\
	config.sub	\
	ltmain.sh	\
	core		\
	*~		\
	svn-commit*
