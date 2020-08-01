/* GStreamer
 * Copyright (C) 2016 Erick Arroyo <erick.arroyo@ridgerun.com>
 * Copyright (C) 2020 Jennifer Caballero <jennifer.caballero@ridgerun.com>
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
 * Given two pipelines, play the first one, wait and then play
 * the other one, it should not be delay in the video when it is display
 * only if the stream-sync property is set to compensate-ts (2)
 */
GST_START_TEST (interpipe_stream_sync_compensate_ts)
{
  GstPipeline *sink1;
  GstPipeline *sink2;


  GstPipeline *src;
  GstElement *vtsrc1;
  GstElement *vtsrc2;


  GstElement *intersrc;
  GstElement *asink;
  GstSample *outsample;
  GstBuffer *buffer;
  GstClockTime buffer_timestamp1, buffer_timestamp2;
  GError *error = NULL;

  /* Create two sink pipelines */
  sink1 =
      GST_PIPELINE (gst_parse_launch
      ("videotestsrc name=vtsrc1 is-live=true pattern=18 ! capsfilter caps=video/x-raw,width=320,height=240,framerate=(fraction)5/1 ! interpipesink "
          "name=intersink1 drop=true async=false sync=true", &error));
  fail_if (error);
  vtsrc1 = gst_bin_get_by_name (GST_BIN (sink1), "vtsrc1");
  sink2 =
      GST_PIPELINE (gst_parse_launch
      ("videotestsrc name=vtsrc2 is-live=true ! capsfilter caps=video/x-raw,width=320,height=240,framerate=(fraction)5/1 ! interpipesink "
          "name=intersink2 drop=true async=false sync=true", &error));
  fail_if (error);
  vtsrc2 = gst_bin_get_by_name (GST_BIN (sink2), "vtsrc2");

  /* Create one source pipeline */
  src =
      GST_PIPELINE (gst_parse_launch
      ("interpipesrc name=intersrc listen-to=intersink1 stream-sync=compensate-ts block-switch=false allow-renegotiation=true format=3 ! capsfilter caps=video/x-raw,width=[320,1920],height=[240,1080],framerate=(fraction)5/1 ! "
          "appsink name=asink drop=true async=false sync=true", &error));
  fail_if (error);
  intersrc = gst_bin_get_by_name (GST_BIN (src), "intersrc");
  asink = gst_bin_get_by_name (GST_BIN (src), "asink");

  /* 
   * Play the pipelines
   * gst_element_get_state blocks up execution until the state change is
   * completed. It's used here to guarantee a secuential pipeline initialization
   * and avoid concurrency errors.
   */
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (sink1), GST_STATE_PLAYING));
  fail_if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (GST_ELEMENT (src),
          GST_STATE_PLAYING));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (sink2), GST_STATE_PLAYING));

  /* Verifies if the caps are set correctly to the listeners
   */
  outsample = gst_app_sink_pull_sample (GST_APP_SINK (asink));
  buffer = gst_sample_get_buffer (outsample);
  fail_if (!buffer);
  buffer_timestamp1 = GST_BUFFER_PTS (buffer);
  GST_ERROR ("Buffer timestamp (dts): %" GST_TIME_FORMAT,
      GST_TIME_ARGS (GST_BUFFER_DTS (buffer)));
  fail_if (buffer_timestamp1 == 0);
  /* Change to another video src */

  g_object_set (G_OBJECT (intersrc), "listen-to", "intersink2", NULL);

  outsample = gst_app_sink_pull_sample (GST_APP_SINK (asink));
  buffer = gst_sample_get_buffer (outsample);
  buffer_timestamp2 = GST_BUFFER_PTS (buffer);
  fail_if (buffer_timestamp2 == 0);

  fail_if (buffer_timestamp2 < buffer_timestamp1);

  /* Stop pipelines */
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (sink1), GST_STATE_NULL));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (sink2), GST_STATE_NULL));
  fail_if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (GST_ELEMENT (src),
          GST_STATE_NULL));

  /* Cleanup */
  g_object_unref (sink1);
  g_object_unref (sink2);
  g_object_unref (vtsrc1);
  g_object_unref (vtsrc2);
  g_object_unref (intersrc);
  g_object_unref (asink);
  g_object_unref (src);

}

GST_END_TEST;

static Suite *
gst_interpipe_suite (void)
{
  Suite *suite = suite_create ("Interpipe");
  TCase *tc1 = tcase_create ("interpipe_stream_sync");

  suite_add_tcase (suite, tc1);
  tcase_add_test (tc1, interpipe_stream_sync_compensate_ts);

  return suite;
}

GST_CHECK_MAIN (gst_interpipe);
