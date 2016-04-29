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
 * Given a sink pipeline and a src pipeline, 
 * they have to be attached correctly and the interpipesrc
 * has to use the name property of the interpipesink.
 */
GST_START_TEST (interpipe_node_name_removed)
{
  GstPipeline *sink;
  GstPipeline *src;
  GstElement *asrc;
  GstElement *asink;
  GstBuffer *buffer;
  GstBuffer *outbuffer;
  GstSample *outsample;
  GError *error = NULL;

  /* Create a sink and a source pipelines */
  sink = GST_PIPELINE (gst_parse_launch ("appsrc name=asrc ! interpipesink "
          "name=sink async=true", &error));
  fail_if (error);

  asrc = gst_bin_get_by_name (GST_BIN (sink), "asrc");

  src = GST_PIPELINE (gst_parse_launch ("interpipesrc listen-to=sink ! "
          "appsink name=asink async=false", &error));
  fail_if (error);

  asink = gst_bin_get_by_name (GST_BIN (src), "asink");

  /* Play the pipelines */
  gst_element_change_state (GST_ELEMENT (sink),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);
  gst_element_change_state (GST_ELEMENT (src),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);

  /* Creating two buffers */
  buffer = gst_buffer_new ();

  /* Push a buffer to the appsrc and receive it in the appsink */
  gst_app_src_push_buffer (GST_APP_SRC (asrc), buffer);

  /* Receive the buffer, will return NULL if the buffer is not received
   */
  outsample = gst_app_sink_pull_sample (GST_APP_SINK (asink));
  outbuffer = gst_sample_get_buffer (outsample);
  fail_if (!outbuffer);
  gst_sample_unref (outsample);

  /* Stop pipelines */
  gst_element_change_state (GST_ELEMENT (sink), GST_STATE_CHANGE_READY_TO_NULL);
  gst_element_change_state (GST_ELEMENT (src), GST_STATE_CHANGE_READY_TO_NULL);

  /* Cleanup */
  g_object_unref (asrc);
  g_object_unref (asink);
  g_object_unref (sink);
  g_object_unref (src);
}

GST_END_TEST;

static Suite *
gst_interpipe_suite (void)
{
  Suite *suite = suite_create ("Interpipe");
  TCase *tc = tcase_create ("node_name_removed");

  suite_add_tcase (suite, tc);
  tcase_add_test (tc, interpipe_node_name_removed);

  return suite;
}

GST_CHECK_MAIN (gst_interpipe);
