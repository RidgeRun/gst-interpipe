docconf = configuration_data()
docconf.set('GST_API_VERSION', apiversion)
docconf.set('VERSION', gst_interpipes_version)
docconf.set('PLUGINDIR', '@0@/lib/gstreamer-1.0'.format(get_option('prefix')))
version_entities = configure_file(input : 'version.entities.in',
  output : 'version.entities',
  configuration : docconf)

# Enter to plugins subdirectory and execute the meson.build
subdir('plugins')
