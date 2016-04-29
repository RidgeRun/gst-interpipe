plugin_LTLIBRARIES = libgstinterpipe.la

# sources used to compile this plug-in
libgstinterpipe_la_SOURCES = gstplugin.c gstinterpipe.c \
			gstinterpipesrc.c \
			gstinterpipesink.c \
			gstinterpipeilistener.c \
			gstinterpipeinode.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstinterpipe_la_CFLAGS = $(GST_CFLAGS) \
			$(GST_BASE_CFLAGS) \
			$(GST_PLUGINS_BASE_CFLAGS) \
			$(GST_APP_CFLAGS)
libgstinterpipe_la_LIBADD = $(GST_PLUGINS_BASE_LIBS) \
			$(GST_BASE_LIBS) $(GST_LIBS) $(GST_APP_LIBS)
libgstinterpipe_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstinterpipe_la_LIBTOOLFLAGS = --tag=disable-static $(GST_PLUGIN_LIBTOOLFLAGS)

# headers we need but don't want installed
noinst_HEADERS = gstinterpipesrc.h gstinterpipesink.h gstinterpipe.h
