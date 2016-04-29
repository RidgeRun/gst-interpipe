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

#include <gst/check/gstcheck.h>
#include <gst/video/gstvideometa.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>

/*
 * When the set_caps event is received, 
 * the listener will be configured accordingly
 */
GST_START_TEST (interpipe_set_caps)
{
  GstPipeline *sink;
  GstPipeline *src1;
  GstPipeline *src2;
  GstElement *vtest;
  GstElement *asink1;
  GstElement *asink2;
  GstCaps *caps1;
  GstCaps *caps2;
  GstSample *outsample1;
  GstSample *outsample2;
  GError *error = NULL;

  /* Create one sink and three source pipelines */
  sink =
      GST_PIPELINE (gst_parse_launch ("videotestsrc name=vtest ! interpipesink "
          "name=sink async=true", &error));
  fail_if (error);

  vtest = gst_bin_get_by_name (GST_BIN (sink), "vtest");

  src1 =
      GST_PIPELINE (gst_parse_launch
      ("interpipesrc listen-to=sink ! capsfilter caps=video/x-raw,format=(string)I420,width=[320,640],height=[240,480],framerate=(fraction)30/1 ! "
          "appsink name=asink1 async=false", &error));
  fail_if (error);

  asink1 = gst_bin_get_by_name (GST_BIN (src1), "asink1");

  src2 =
      GST_PIPELINE (gst_parse_launch
      ("interpipesrc listen-to=sink ! capsfilter caps=video/x-raw,format=(string)I420,width=[500,640],height=[300,480],framerate=(fraction)30/1 ! "
          "appsink name=asink2 async=false", &error));
  fail_if (error);

  asink2 = gst_bin_get_by_name (GST_BIN (src2), "asink2");

  /* Play the pipelines */
  gst_element_change_state (GST_ELEMENT (sink),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);
  gst_element_change_state (GST_ELEMENT (src1),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);
  gst_element_change_state (GST_ELEMENT (src2),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);

  /* Verifies if the caps are set correctly to the listeners
   */
  outsample1 = gst_app_sink_pull_sample (GST_APP_SINK (asink1));
  caps1 = gst_sample_get_caps (outsample1);
  fail_if (!caps1);

  outsample2 = gst_app_sink_pull_sample (GST_APP_SINK (asink2));
  caps2 = gst_sample_get_caps (outsample2);
  fail_if (!caps2);

  fail_if (!gst_caps_is_equal (caps1, caps2));
  gst_sample_unref (outsample1);
  gst_sample_unref (outsample2);

  /* Stop pipelines */
  gst_element_change_state (GST_ELEMENT (sink), GST_STATE_CHANGE_READY_TO_NULL);
  gst_element_change_state (GST_ELEMENT (src1), GST_STATE_CHANGE_READY_TO_NULL);
  gst_element_change_state (GST_ELEMENT (src2), GST_STATE_CHANGE_READY_TO_NULL);

  /* Cleanup */
  g_object_unref (vtest);
  g_object_unref (asink1);
  g_object_unref (asink2);
  g_object_unref (sink);
  g_object_unref (src1);
  g_object_unref (src2);
}

GST_END_TEST;

static Suite *
gst_interpipe_suite (void)
{
  Suite *suite = suite_create ("Interpipe");
  TCase *tc = tcase_create ("set_caps");

  suite_add_tcase (suite, tc);
  tcase_add_test (tc, interpipe_set_caps);

  return suite;
}

GST_CHECK_MAIN (gst_interpipe);
