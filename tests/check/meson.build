# Tests and condition when to skip the test
core_tests = [
  [ 'gst/test_allow_renegotiation_property.c' ],
  [ 'gst/test_anonymous_connection.c' ],
  [ 'gst/test_block_switch.c' ],
  [ 'gst/test_caps_renegotiation.c' ],
  [ 'gst/test_enable_sync.c' ],
  [ 'gst/test_get_caps.c' ],
  [ 'gst/test_hot_plug.c' ],
  [ 'gst/test_in_bounds_events.c' ],
  [ 'gst/test_interpipe_synchronization.c' ],
  [ 'gst/test_invalid_caps.c' ],
  [ 'gst/test_node_name_removed.c' ],
  [ 'gst/test_out_of_bounds_events.c' ],
  [ 'gst/test_out_of_bounds_upstream_events.c' ],
  [ 'gst/test_reconfigure_event.c' ],
  [ 'gst/test_set_caps.c' ],
]

# Add C Definitions for tests
test_defines = [
  '-DTESTFILE="' + meson.current_source_dir() + '/meson.build"',
]

# Define test dependencies
test_deps=[gst_app_dep, gst_check_dep, interpipes_dep]

# Define plugins path
gst_dep = dependency('gstreamer-1.0')
plugins_dir = gst_dep.get_pkgconfig_variable('pluginsdir')

# Build and run tests
foreach t : core_tests
  fname = t[0]
  test_name = fname.split('.')[0].underscorify()
  skip_test = t.get(1, false)
  link_with_libs = t.get(2, [])

  if not skip_test
    # Creates a new executable for each test
    exe = executable(test_name, fname,
        c_args : gst_c_args + test_defines,
        cpp_args : gst_c_args + test_defines,
        include_directories : [configinc],
        link_with : link_with_libs,
        dependencies : test_deps,
    )
    # Create enviomen object to  stores information about the environment
    # variables set during tests.

    env = environment()
    env.set('GST_PLUGIN_PATH_1_0', interpipe_plugin_dir + ':' + plugins_dir)
    env.set('GST_PLUGIN_SYSTEM_PATH_1_0', '')
    env.set('CK_DEFAULT_TIMEOUT', '120')
    env.set('GST_REGISTRY', '@0@/@1@.registry'.format(meson.current_build_dir(), test_name))
    # Run tests
    test(test_name, exe, env: env, timeout : 60)
  endif
endforeach
