/* GStreamer
 * Copyright (C) 2016 Erick Arroyo <erick.arroyo@ridgerun.com>
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

#define G_MINSSIZE G_MINLONG

#include <gst/check/gstcheck.h>
#include <gst/video/gstvideometa.h>
#include <gst/app/gstappsrc.h>
#include <gst/app/gstappsink.h>
#include <gst/base/gstbasesrc.h>

/*
 * Given two independent pipelines, when a downstream in
 * bound event arrives to an interpipesrc, it will
 * be serialized correctly with the buffers queue and.
 * will be send at the correct moment 
 */

GST_START_TEST (interpipe_in_bounds_events)
{
  GstElement *pipelinesrc;
  GstElement *pipelinesink;
  GstElement *appsrc;
  GstElement *intersink;
  GstElement *intersrc;
  GstElement *fsink;
  GstElement *identity;
  GstBuffer *buf1;
  GstBuffer *buf2;
  GstBuffer *buf3;
  GstBuffer *buf4;
  GstClock *clock;
  GError *error = NULL;
  GstPad *interpad;

  /* Create one sink and one source pipelines */
  pipelinesrc = gst_pipeline_new ("src_pipe");
  pipelinesink = gst_pipeline_new ("sink_pipe");

  intersink =
      gst_parse_launch ("interpipesink name=interpipesink sync=true", &error);
  fail_if (error);

  appsrc = gst_parse_launch ("appsrc num-buffers=10 name=appsrc ", &error);
  fail_if (error);

  intersrc =
      gst_parse_launch
      ("interpipesrc name=interpipesrc listen-to=interpipesink", &error);
  fail_if (error);

  identity =
      gst_parse_launch ("identity sleep-time=6000000 silent=false", &error);
  fail_if (error);

  fsink =
      gst_parse_launch ("appsink name=appsink_end sync=true async=false",
      &error);
  fail_if (error);


  gst_bin_add_many (GST_BIN (pipelinesrc), appsrc, intersink, NULL);
  gst_element_link_many (appsrc, intersink, NULL);
  gst_bin_add_many (GST_BIN (pipelinesink), intersrc, identity, fsink, NULL);
  gst_element_link_many (intersrc, identity, fsink, NULL);

  /* Play the pipelines */
  fail_if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (pipelinesrc,
          GST_STATE_PLAYING));
  fail_if (GST_STATE_CHANGE_FAILURE == gst_element_set_state (pipelinesink,
          GST_STATE_PLAYING));

  /* Creating buffers */

  clock = gst_element_get_clock (intersrc);

  buf1 = gst_buffer_new_allocate (NULL, 2, 0);
  buf2 = gst_buffer_new_allocate (NULL, 2, 0);
  buf3 = gst_buffer_new_allocate (NULL, 2, 0);
  buf4 = gst_buffer_new_allocate (NULL, 2, 0);

  interpad = GST_BASE_SRC_PAD (GST_BASE_SRC (appsrc));

  GST_BUFFER_PTS (buf1) =
      gst_clock_get_time (clock) - gst_element_get_base_time (intersrc);
  gst_app_src_push_buffer (GST_APP_SRC (appsrc), buf1);
  GST_BUFFER_PTS (buf2) =
      gst_clock_get_time (clock) - gst_element_get_base_time (intersrc);
  gst_app_src_push_buffer (GST_APP_SRC (appsrc), buf2);


  /* Send Flush Stop event */

  gst_pad_push_event (interpad,
      gst_event_new_custom (GST_EVENT_CUSTOM_DOWNSTREAM, NULL));

  GST_BUFFER_PTS (buf3) =
      gst_clock_get_time (clock) - gst_element_get_base_time (intersrc);
  gst_app_src_push_buffer (GST_APP_SRC (appsrc), buf3);
  GST_BUFFER_PTS (buf4) =
      gst_clock_get_time (clock) - gst_element_get_base_time (intersrc);
  gst_app_src_push_buffer (GST_APP_SRC (appsrc), buf4);

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

static Suite *
gst_interpipe_suite (void)
{
  Suite *suite = suite_create ("Interpipe");
  TCase *tc = tcase_create ("in_bounds_events");

  tcase_set_timeout (tc, 40);
  suite_add_tcase (suite, tc);
  tcase_add_test (tc, interpipe_in_bounds_events);

  return suite;
}

GST_CHECK_MAIN (gst_interpipe);
