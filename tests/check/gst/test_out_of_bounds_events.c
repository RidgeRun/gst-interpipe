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
 * Given two independent pipelines, when a downstream out
 * of bounds event arrives to an interpipesink, it will
 * be forwarded to the next interpipesrc. 
 */
GST_START_TEST (interpipe_out_of_bounds_events)
{
  GstElement *pipelinesrc;
  GstElement *pipelinesink;
  GstElement *appsrc;
  GstElement *intersink;
  GstElement *intersrc;
  GstElement *fsink;
  GstPad *srcpad;
  GstPad *sinkpad;
  GError *error = NULL;

  /* Create one sink and one source pipelines */
  pipelinesrc = gst_pipeline_new ("src_pipe");
  pipelinesink = gst_pipeline_new ("sink_pipe");

  intersink =
      gst_parse_launch ("interpipesink name=videosrc1 sync=true", &error);
  fail_if (error);

  appsrc = gst_parse_launch ("appsrc name=appsrc", &error);
  fail_if (error);

  intersrc =
      gst_parse_launch ("interpipesrc name=display listen-to=videosrc1",
      &error);
  fail_if (error);

  fsink = gst_parse_launch ("fakesink sync=true async=false", &error);
  fail_if (error);


  gst_bin_add_many (GST_BIN (pipelinesrc), appsrc, intersink, NULL);
  gst_element_link_many (appsrc, intersink, NULL);
  gst_bin_add_many (GST_BIN (pipelinesink), intersrc, fsink, NULL);
  gst_element_link_many (intersrc, fsink, NULL);

  /* Play the pipelines */
  gst_element_set_state (pipelinesrc, GST_STATE_PLAYING);
  gst_element_set_state (pipelinesink, GST_STATE_PLAYING);

  sleep (1);

  /* Create pads */
  srcpad = gst_element_get_static_pad (appsrc, "src");
  fail_if (!srcpad);
  sinkpad = gst_element_get_static_pad (fsink, "sink");
  fail_if (!srcpad);

  /* Send Flush Start event */
  fail_if (!gst_pad_push_event (srcpad, gst_event_new_flush_start ()));

  sleep (1);

  fail_if (!GST_PAD_IS_FLUSHING (sinkpad));

  /* Stop pipelines */
  gst_element_set_state (pipelinesrc, GST_STATE_NULL);
  gst_element_set_state (pipelinesink, GST_STATE_NULL);

  /* Cleanup */
  g_object_unref (pipelinesrc);
  g_object_unref (pipelinesink);
}

GST_END_TEST;

static Suite *
gst_interpipe_suite (void)
{
  Suite *suite = suite_create ("Interpipe");
  TCase *tc = tcase_create ("out_of_bounds_events");

  suite_add_tcase (suite, tc);
  tcase_add_test (tc, interpipe_out_of_bounds_events);

  return suite;
}

GST_CHECK_MAIN (gst_interpipe);
