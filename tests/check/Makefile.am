include $(top_srcdir)/common/check.mak

CHECK_REGISTRY = $(top_builddir)/tests/check/test-registry.reg

REGISTRY_ENVIRONMENT = \
	GST_REGISTRY_1_0=$(CHECK_REGISTRY)

AM_TESTS_ENVIRONMENT = \
	CK_DEFAULT_TIMEOUT=120                                  \
	$(REGISTRY_ENVIRONMENT)					\
	GST_PLUGIN_SYSTEM_PATH_1_0=				\
	GST_PLUGIN_PATH_1_0=$(top_builddir)/gst/interpipe/.libs:$(GST_PLUGINS_DIR)

# the core dumps of some machines have PIDs appended, test registry and
# profiling data
CLEANFILES = core core.* test-registry.* *.gcno *.gcda

SUPPRESSIONS = $(top_srcdir)/common/gst.supp

clean-local: clean-local-check

$(CHECK_REGISTRY):
	$(AM_TESTS_ENVIRONMENT)

check_PROGRAMS = gst/test_allow_renegotiation_property \
                 gst/test_anonymous_connection \
                 gst/test_block_switch \
                 gst/test_buffer_properties \
                 gst/test_caps_renegotiation \
                 gst/test_stream_sync \
                 gst/test_get_caps \
                 gst/test_hot_plug \
                 gst/test_in_bounds_events \
                 gst/test_invalid_caps \
                 gst/test_node_name_removed \
                 gst/test_out_of_bounds_events \
                 gst/test_out_of_bounds_upstream_events \
                 gst/test_reconfigure_event \
                 gst/test_set_caps

TESTS = $(check_PROGRAMS)


AM_CFLAGS =	$(GST_CFLAGS) \
		$(GST_BASE_CFLAGS) \
		$(GST_PLUGINS_BASE_CFLAGS) \
		$(GST_APP_CFLAGS) \
		$(GST_VIDEO_CFLAGS) \
		$(GST_CHECK_CFLAGS)
LIBS = $(GST_PLUGINS_BASE_LIBS) \
			-lgstvideo-@GST_API_VERSION@ \
			$(GST_BASE_LIBS) $(GST_LIBS) $(GST_APP_LIBS) \
			$(GST_CHECK_LIBS) $(GST_VIDEO_LIBS)

# valgrind testing
# these just need valgrind fixing, period
VALGRIND_TO_FIX = 

VALGRIND_IGNORE = 

# these need fixing because the threads cause segfaults under valgrind
TESTS_THREADED =

VALGRIND_TESTS_DISABLE = 					\
	$(TESTS_THREADED)					\
	$(VALGRIND_IGNORE)					\
	$(VALGRIND_TO_FIX)

# indexers does not get tested yet
COVERAGE_DIRS = plugins/tracers

COVERAGE_FILES = $(foreach dir,$(COVERAGE_DIRS),$(wildcard $(top_builddir)/$(dir)/*.gcov))
COVERAGE_FILES_REL = $(subst $(top_builddir)/,,$(COVERAGE_FILES))
COVERAGE_OUT_FILES = $(foreach dir,$(COVERAGE_DIRS),$(wildcard $(top_builddir)/$(dir)/*.gcov.out))
COVERAGE_OUT_FILES_REL = $(subst $(top_builddir)/,,$(COVERAGE_OUT_FILES))

debug:
	echo $(GST_PLUGINS_DIR)
	echo $(COVERAGE_FILES)
	echo $(COVERAGE_FILES_REL)

.PHONY: coverage
if GST_GCOV_ENABLED
# we rebuild a registry and do gst-inspect so that all the get/set codepaths
# are also covered
coverage:
	for file in `find $(top_builddir) -name '*.gcda'`; do rm $$file; done
	-rm $(CHECK_REGISTRY)
	echo "Inspecting all elements"
	for e in `$(GST_INSPECT) | head -n -2 | cut -d: -f2`; do $(GST_INSPECT) $$e > /dev/null 2>&1; done
	make check
	make coverage-report
else
coverage:
	echo "You need to configure with --enable-gcov to get coverage data"
	exit 1
endif

coverage-report:
	rm -r coverage
	for dir in $(COVERAGE_DIRS); do 				\
	  mkdir -p coverage/$$dir; 					\
	  make -C $(top_builddir)/$$dir gcov;				\
        done
	for dir in $(COVERAGE_DIRS); do					\
	    files="`ls $(top_builddir)/$$dir/*.gcov.out 2> /dev/null`"; \
          if test ! -z "$$files"; then					\
	    perl $(top_srcdir)/common/coverage/coverage-report.pl 	\
	      $(top_builddir)/$$dir/*.gcov.out >			\
	      coverage/$$dir/index.xml;					\
	    xsltproc $(top_srcdir)/common/coverage/coverage-report.xsl	\
	      coverage/$$dir/index.xml > coverage/$$dir/index.html;	\
	  fi; \
        done
	for file in $(COVERAGE_FILES_REL); do				\
	  echo Generating coverage/$$file.html; 			\
	  perl $(top_srcdir)/common/coverage/coverage-report-entry.pl   \
	    $(top_builddir)/$$file > coverage/$$file.html; 		\
	done
