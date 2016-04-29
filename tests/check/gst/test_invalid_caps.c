/* GStreamer
 * Copyright (C) 2016 Carlos Rodriguez <carlos.rodriguez@ridgerun.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <unistd.h>
#include <gst/check/gstcheck.h>
#include <gst/video/gstvideometa.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

/*
 * Given one interpipesrc and one interpipesink, when there's not
 * caps intersection, the caps must not be set.
 */

GST_START_TEST (invalid_caps)
{
  GstPipeline *sink;
  GstPipeline *src;
  GstElement *intersink;
  GstElement *intersrc;
  GstCaps *caps1;
  GstCaps *caps2;
  GError *error = NULL;

  /* Create two sink pipelines */
  sink =
      GST_PIPELINE (gst_parse_launch
      ("videotestsrc name=vtsrc ! capsfilter caps=video/x-raw,width=640,height=480,framerate=(fraction)30/1 ! interpipesink "
          "name=intersink sync=true", &error));
  fail_if (error);

  intersink = gst_bin_get_by_name (GST_BIN (sink), "intersink");

  /* Create one source pipeline */

  src =
      GST_PIPELINE (gst_parse_launch
      ("interpipesrc name=intersrc listen-to=intersink ! capsfilter caps=video/x-raw,format=(string)I420,width=320,height=240,framerate=(fraction)30/1 ! "
          "appsink name=asink async=false", &error));
  fail_if (error);

  intersrc = gst_bin_get_by_name (GST_BIN (src), "intersrc");

  /* Play the pipelines */
  gst_element_change_state (GST_ELEMENT (sink),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);
  gst_element_change_state (GST_ELEMENT (src),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);

  /* Verifies if interpipesink and interpipesrc have caps set
   */
  sleep (1);

  caps1 = gst_app_src_get_caps (GST_APP_SRC (intersrc));
  fail_if (caps1);

  caps2 = gst_app_sink_get_caps (GST_APP_SINK (intersink));
  fail_if (!caps2);

  /* Stop pipelines */
  gst_element_change_state (GST_ELEMENT (sink), GST_STATE_CHANGE_READY_TO_NULL);
  gst_element_change_state (GST_ELEMENT (src), GST_STATE_CHANGE_READY_TO_NULL);

  /* Cleanup */
  g_object_unref (intersink);
  g_object_unref (intersrc);
  g_object_unref (sink);
  g_object_unref (src);
}

GST_END_TEST;


static Suite *
gst_interpipe_suite (void)
{
  Suite *suite = suite_create ("Interpipe");
  TCase *tc1 = tcase_create ("invalid_caps");

  suite_add_tcase (suite, tc1);
  tcase_add_test (tc1, invalid_caps);

  return suite;
}

GST_CHECK_MAIN (gst_interpipe);
