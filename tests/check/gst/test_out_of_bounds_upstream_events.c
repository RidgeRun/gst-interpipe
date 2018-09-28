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

static gboolean get_event_one_listener (GstPad * pad, GstObject * parent,
    GstEvent * event);
static gboolean get_event_two_listeners (GstPad * pad, GstObject * parent,
    GstEvent * event);

static gboolean
get_event_one_listener (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean ret = FALSE;
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_LATENCY:
      ret = TRUE;
      GST_INFO ("Latency test event arrives to the interpipesink");
      break;
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  fail_if (!ret);
  return TRUE;
}

static gboolean
get_event_two_listeners (GstPad * pad, GstObject * parent, GstEvent * event)
{
  gboolean ret = FALSE;
  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_LATENCY:
      ret = TRUE;
      GST_ERROR
          ("Latency test event arrives to the interpipesink and it should not");
      break;
    default:
      ret = gst_pad_event_default (pad, parent, event);
      break;
  }
  fail_if (ret);
  return TRUE;
}

/*
 * Given two independent pipelines, when an
 * upstream event arrives to the interpipesrc
 * it will be forwarded to the interpipesink 
 */
GST_START_TEST (interpipe_out_of_bounds_upstream_events_one_listener)
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
  GstClockTime latency;

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
  fail_if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (pipelinesrc,
          GST_STATE_PLAYING));
  fail_if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (pipelinesink,
          GST_STATE_PLAYING));

  /* Create pads */
  srcpad = gst_element_get_static_pad (appsrc, "src");
  fail_if (!srcpad);
  sinkpad = gst_element_get_static_pad (fsink, "sink");
  fail_if (!srcpad);

  /* Set event function on pad */
  gst_pad_set_event_function (srcpad, get_event_one_listener);

  /* Send Latency event */
  latency = 1;
  fail_if (!gst_pad_push_event (sinkpad, gst_event_new_latency (latency)));

  /* Stop pipelines */
  fail_if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (pipelinesrc,
          GST_STATE_NULL));
  fail_if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (pipelinesink,
          GST_STATE_NULL));

  /* Cleanup */
  g_object_unref (pipelinesrc);
  g_object_unref (pipelinesink);
}

GST_END_TEST;

/*
 * Given three independent pipelines, when an
 * upstream event arrives to one interpipesrc
 * it will try to be forwarded to the interpipesink,
 * but it can be send because there is another listener. 
 */
GST_START_TEST (interpipe_out_of_bounds_upstream_events_two_listeners)
{
  GstElement *pipelinesrc;
  GstElement *pipelinesink;
  GstElement *pipelinesink2;
  GstElement *appsrc;
  GstElement *intersink;
  GstElement *intersrc;
  GstElement *fsink;
  GstElement *intersrc2;
  GstElement *fsink2;
  GstPad *srcpad;
  GstPad *sinkpad;
  GError *error = NULL;
  GstClockTime latency;

  /* Create one sink and one source pipelines */
  pipelinesrc = gst_pipeline_new ("src_pipe");
  pipelinesink = gst_pipeline_new ("sink_pipe");
  pipelinesink2 = gst_pipeline_new ("sink_pipe2");

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

  intersrc2 =
      gst_parse_launch ("interpipesrc name=display2 listen-to=videosrc1",
      &error);
  fail_if (error);

  fsink2 = gst_parse_launch ("fakesink sync=true async=false", &error);
  fail_if (error);


  gst_bin_add_many (GST_BIN (pipelinesrc), appsrc, intersink, NULL);
  gst_element_link_many (appsrc, intersink, NULL);
  gst_bin_add_many (GST_BIN (pipelinesink), intersrc, fsink, NULL);
  gst_element_link_many (intersrc, fsink, NULL);
  gst_bin_add_many (GST_BIN (pipelinesink2), intersrc2, fsink2, NULL);
  gst_element_link_many (intersrc2, fsink2, NULL);

  /* Play the pipelines */
  fail_if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (pipelinesrc,
          GST_STATE_PLAYING));
  fail_if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (pipelinesink,
          GST_STATE_PLAYING));
  fail_if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (pipelinesink2,
          GST_STATE_PLAYING));

  /* Create pads */
  srcpad = gst_element_get_static_pad (appsrc, "src");
  fail_if (!srcpad);
  sinkpad = gst_element_get_static_pad (fsink, "sink");
  fail_if (!srcpad);

  /* Set event function on pad */
  gst_pad_set_event_function (srcpad, get_event_two_listeners);

  /* Send Latency event */
  latency = 1;
  fail_if (!gst_pad_push_event (sinkpad, gst_event_new_latency (latency)));

  /* Stop pipelines */
  fail_if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (pipelinesrc,
          GST_STATE_NULL));
  fail_if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (pipelinesink,
          GST_STATE_NULL));
  fail_if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (pipelinesink2,
          GST_STATE_NULL));

  /* Cleanup */
  g_object_unref (pipelinesrc);
  g_object_unref (pipelinesink);
  g_object_unref (pipelinesink2);
}

GST_END_TEST;



static Suite *
gst_interpipe_suite (void)
{
  Suite *suite = suite_create ("Interpipe");
  TCase *tc = tcase_create ("out_of_bounds_upstream_events_one_listener");
  TCase *tc2 = tcase_create ("out_of_bounds_upstream_events_two_listeners");

  suite_add_tcase (suite, tc);
  tcase_add_test (tc, interpipe_out_of_bounds_upstream_events_one_listener);

  suite_add_tcase (suite, tc2);
  tcase_add_test (tc2, interpipe_out_of_bounds_upstream_events_two_listeners);

  return suite;
}

GST_CHECK_MAIN (gst_interpipe);
