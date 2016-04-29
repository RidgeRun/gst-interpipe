dnl pkg-config-based checks for GStreamer modules and dependency modules

dnl specific:
dnl AG_GST_CHECK_GST_APP([MAJMIN], [MINVER], [REQUIRED])
dnl AG_GST_CHECK_GST_VIDEO([MAJMIN], [MINVER], [REQUIRED])

dnl ===========================================================================
dnl AG_GST_CHECK_GST_APP([GST-API_VERSION], [MIN-VERSION], [REQUIRED])
dnl
dnl Sets GST_APP_CFLAGS and GST_APP_LIBS.
dnl
dnl ===========================================================================
AC_DEFUN([AG_GST_CHECK_GST_APP],
[
  AG_GST_CHECK_MODULES(GST_APP, gstreamer-app-[$1], [$2],
    [GStreamer App], [$3])
])

dnl ===========================================================================
dnl AG_GST_CHECK_GST_VIDEO([GST-API_VERSION], [MIN-VERSION], [REQUIRED])
dnl
dnl Sets GST_VIDEO_CFLAGS and GST_VIDEO_LIBS.
dnl
dnl ===========================================================================
AC_DEFUN([AG_GST_CHECK_GST_VIDEO],
[
  AG_GST_CHECK_MODULES(GST_VIDEO, gstreamer-video-[$1], [$2],
    [GStreamer Video], [$3])
])

