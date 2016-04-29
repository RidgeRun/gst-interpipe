DISTCHECK_CONFIGURE_FLAGS=--enable-gtk-doc

ALWAYS_SUBDIRS = 	\
	gst 	\
	tests	\
	common  \
	docs  


SUBDIRS = $(ALWAYS_SUBDIRS)

DIST_SUBDIRS = $(ALWAYS_SUBDIRS)

EXTRA_DIST = \
	gst-interpipe.spec depcomp \
	AUTHORS COPYING NEWS README RELEASE REQUIREMENTS \
	ChangeLog gst-interpipe.doap autogen.sh

DISTCLEANFILES = _stdint.h

ACLOCAL_AMFLAGS = -I m4 -I common/m4

include $(top_srcdir)/common/release.mak
