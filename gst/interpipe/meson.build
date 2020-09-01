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
gst_interpipes_lib = library('gstinterpipe',
  gst_interpipes_sources,
  c_args : gst_c_args,
  include_directories : configinc,
  install : true,
  install_dir : plugin_install_dir,
  dependencies : [gst_app_dep],
)

interpipe_plugin_dir = meson.current_build_dir()

# Define the library as an internal dependency to the current build
interpipes_dep = declare_dependency(link_with: gst_interpipes_lib,
  dependencies : [gst_app_dep])
