# Copy the file provided in input: to a file in the build directory with
# the name output: in the current directory.

types = configure_file(input : 'gst-interpipe.types',
  output : 'gst-interpipe.types',
  copy : true)

version_conf = configuration_data()
version_conf.set('PACKAGE_VERSION', meson.project_version())
version_conf.set('GST_API_VERSION', apiversion)
version_conf.set('PACKAGE_NAME', 'GstInterpipe')
configure_file(input : 'gst-interpipe-docs.xml.in',
  output : 'gst-interpipe-docs.xml',
  configuration: version_conf)

# Compiles and installs gtkdoc documentation into prefix/share/gtk-doc/html.
gnome.gtkdoc('gst-interpipes',
  main_xml : 'gst-interpipe-docs.xml',
  src_dir : '@0@/gst/interpipe/'.format(meson.source_root()),
  gobject_typesfile : types,
  content_files : [version_entities],
  dependencies : [glib_dep, gst_dep, gst_base_dep, gst_app_dep],
  install : true)
