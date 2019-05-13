# Define source code
gst_interpipes_sources = [
  'gstinterpipe.c',
  'gstinterpipeilistener.c',
  'gstinterpipeinode.c',
  'gstinterpipesink.c',
  'gstinterpipesrc.c',
  'gstplugin.c',
]

gst_interpipes_headers = [
  'gstinterpipe.h',
  'gstinterpipeilistener.h',
  'gstinterpipeinode.h',
  'gstinterpipesink.h',
  'gstinterpipesrc.h',
]

# Build plugin library
gst_interpipes_lib = library('gstinterpipes-@0@'.format(apiversion),
  gst_interpipes_sources,
  c_args : gst_c_args,
  version : apiversion,
  include_directories : configinc,
  install : true,
  install_dir : plugin_install_dir,
  dependencies : [glib_dep, gst_dep, gst_base_dep, gst_app_dep],
)

# Define the library as an internal dependency to the current build
interpipes_dep = declare_dependency(link_with: gst_interpipes_lib,
  dependencies : [glib_dep, gst_dep, gst_base_dep, gst_app_dep])
