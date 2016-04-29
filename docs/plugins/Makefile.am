GST_DOC_SCANOBJ = $(top_srcdir)/common/gstdoc-scangobj

## Process this file with automake to produce Makefile.in

# The name of the module, e.g. 'glib'.
#DOC_MODULE=gst-plugins-libs-@GST_API_VERSION@
MODULE=gst-interpipe
DOC_MODULE=$(MODULE)

# for upload-doc.mak
DOC=$(MODULE)
FORMATS=html
html: html-build.stamp
include $(top_srcdir)/common/upload-doc.mak

# The top-level SGML file. Change it if you want.
DOC_MAIN_SGML_FILE=$(DOC_MODULE)-docs.xml

# The directory containing the source code.
# gtk-doc will search all .c & .h files beneath here for inline comments
# documenting functions and macros.
DOC_SOURCE_DIR = $(top_srcdir)/gst/interpipe

# Extra options to supply to gtkdoc-scan.
SCAN_OPTIONS=

# Used for dependencies.
HFILE_GLOB=$(top_srcdir)/gst/interpipe/*.h 
CFILE_GLOB=$(top_srcdir)/gst/interpipe/*.c 

# Images to copy into HTML directory.
HTML_IMAGES =

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE).
content_files =

# Other files to distribute.
extra_files =

# CFLAGS and LDFLAGS for compiling scan program. Only needed if your app/lib
# contains GtkObjects/GObjects and you want to document signals and properties.
GTKDOC_CFLAGS = $(GST_CFLAGS) -I$(top_builddir)/gst/interpipe/
GTKDOC_LIBS =  $(GST_LIBS) 

GTKDOC_CC=$(LIBTOOL) --tag=CC --mode=compile $(CC)
GTKDOC_LD=$(LIBTOOL) --tag=CC --mode=link $(CC)

# If you need to override some of the declarations, place them in this file
DOC_OVERRIDES = $(DOC_MODULE)-overrides.txt

include $(top_srcdir)/common/gtk-doc-plugins.mak
