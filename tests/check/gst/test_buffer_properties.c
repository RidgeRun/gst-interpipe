/* GStreamer
 * Copyright (C) 2016 Michael Grüner <michael.gruner@ridgerun.com>
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
 * Given one sink pipeline and three src pipelines, 
 * buffer's metadata should match on all pipelines
 */
GST_START_TEST (interpipe_buffer_properties)
{
  GstPipeline *sink;
  GstPipeline *src1;
  GstPipeline *src2;
  GstPipeline *src3;
  GstElement *asrc;
  GstElement *asink1;
  GstElement *asink2;
  GstElement *asink3;
  GstBuffer *buffer;
  GstBuffer *outbuffer;
  GstSample *outsample;
  GError *error = NULL;
  GstVideoMeta *new_meta;

  /* Create one sink and three source pipelines */
  sink = GST_PIPELINE (gst_parse_launch ("appsrc name=asrc ! interpipesink "
          "name=sink async=true", &error));
  fail_if (error);

  asrc = gst_bin_get_by_name (GST_BIN (sink), "asrc");

  src1 = GST_PIPELINE (gst_parse_launch ("interpipesrc listen-to=sink ! "
          "appsink name=asink1 async=false", &error));
  fail_if (error);

  asink1 = gst_bin_get_by_name (GST_BIN (src1), "asink1");

  src2 = GST_PIPELINE (gst_parse_launch ("interpipesrc listen-to=sink ! "
          "appsink name=asink2 async=false", &error));
  fail_if (error);

  asink2 = gst_bin_get_by_name (GST_BIN (src2), "asink2");

  src3 = GST_PIPELINE (gst_parse_launch ("interpipesrc listen-to=sink ! "
          "appsink name=asink3 async=false", &error));
  fail_if (error);

  asink3 = gst_bin_get_by_name (GST_BIN (src3), "asink3");

  /* Play the pipelines */
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (sink), GST_STATE_PLAYING));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (src1), GST_STATE_PLAYING));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (src2), GST_STATE_PLAYING));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (src3), GST_STATE_PLAYING));

  /* Create the buffer and insert a custom meta */
  buffer = gst_buffer_new ();
  gst_buffer_add_video_meta (buffer, GST_VIDEO_FRAME_FLAG_INTERLACED,
      GST_VIDEO_FORMAT_I420, 100, 100);

  /* Push the buffer to the appsrc and receive it in the appsinks */
  gst_app_src_push_buffer (GST_APP_SRC (asrc), buffer);

  /* Receive the buffer and get it's meta. If its not a videometa, 
     the getter will return NULL
   */
  outsample = gst_app_sink_pull_sample (GST_APP_SINK (asink1));
  outbuffer = gst_sample_get_buffer (outsample);
  new_meta = gst_buffer_get_video_meta (outbuffer);
  fail_if (!new_meta);
  gst_sample_unref (outsample);

  outsample = gst_app_sink_pull_sample (GST_APP_SINK (asink2));
  outbuffer = gst_sample_get_buffer (outsample);
  new_meta = gst_buffer_get_video_meta (outbuffer);
  fail_if (!new_meta);
  gst_sample_unref (outsample);

  outsample = gst_app_sink_pull_sample (GST_APP_SINK (asink3));
  outbuffer = gst_sample_get_buffer (outsample);
  new_meta = gst_buffer_get_video_meta (outbuffer);
  fail_if (!new_meta);
  gst_sample_unref (outsample);

  /* Stop pipelines */
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (sink), GST_STATE_NULL));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (src1), GST_STATE_NULL));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (src2), GST_STATE_NULL));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (src3), GST_STATE_NULL));

  /* Cleanup */
  g_object_unref (asrc);
  g_object_unref (asink1);
  g_object_unref (asink2);
  g_object_unref (asink3);
  g_object_unref (sink);
  g_object_unref (src1);
  g_object_unref (src2);
  g_object_unref (src3);
}

GST_END_TEST;

static Suite *
gst_interpipe_suite (void)
{
  Suite *suite = suite_create ("Interpipe");
  TCase *tc = tcase_create ("buffer_properties");

  suite_add_tcase (suite, tc);
  tcase_add_test (tc, interpipe_buffer_properties);

  return suite;
}

GST_CHECK_MAIN (gst_interpipe);
