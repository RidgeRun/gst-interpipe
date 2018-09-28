/* GStreamer
 * Copyright (C) 2016 Erick Arroyo <erick.arroyo@ridgerun.com>
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
 * Given one interpipesink with fixed caps and one interpipesrc listening to it, the renegotiation
 * of caps would be done only if the interpipesink doesn't have other listeners and there is intersection
 * between the caps.
 */

typedef struct _CondData
{
  GMutex *mutex;
  GCond *cond;
  gboolean *condition;
} CondData;

static void free_condition (GstPad * pad, const GParamSpec * pspec,
    gpointer data);

static void
free_condition (GstPad * pad, const GParamSpec * pspec, gpointer data)
{
  CondData *conditiondata = (CondData *) data;
  g_mutex_lock (conditiondata->mutex);
  *conditiondata->condition = TRUE;
  g_cond_signal (conditiondata->cond);
  g_mutex_unlock (conditiondata->mutex);
}

GST_START_TEST (reconfigure_event)
{
  GstPipeline *sink1;
  GstPipeline *sink2;
  GstPipeline *src1;
  GstPipeline *src2;
  GstElement *intersink1;
  GstElement *intersink2;
  GstElement *intersrc1;
  GstElement *intersrc2;
  GstElement *asink1;
  GstElement *asink2;
  GstCaps *caps1;
  GstCaps *caps2;
  GstPad *pad;
  GMutex mutex;
  GCond cond;
  gboolean capschange = FALSE;
  gulong id;
  CondData conditiondata;
  gint64 end_time;
  GError *error = NULL;

  g_mutex_init (&mutex);
  g_cond_init (&cond);

  /* Create two sink pipelines */
  sink1 =
      GST_PIPELINE (gst_parse_launch
      ("videotestsrc name=vtest1 ! capsfilter caps=video/x-raw,format=(string)I420,width=[320,1280],height=[240,720],framerate=(fraction)30/1 ! interpipesink "
          "name=intersink1 sync=true", &error));
  fail_if (error);
  intersink1 = gst_bin_get_by_name (GST_BIN (sink1), "intersink1");
  sink2 =
      GST_PIPELINE (gst_parse_launch
      ("videotestsrc name=vtest2 pattern=18 ! capsfilter caps=video/x-raw,format=(string)I420,width=[640,1280],height=[480,720],framerate=(fraction)30/1 ! interpipesink "
          "name=intersink2 sync=true", &error));
  fail_if (error);
  intersink2 = gst_bin_get_by_name (GST_BIN (sink2), "intersink2");

  /* Create two source pipelines */
  src1 =
      GST_PIPELINE (gst_parse_launch
      ("interpipesrc name=intersrc1 listen-to=intersink1 ! capsfilter caps=video/x-raw,format=(string)I420,width=[320,640],height=[240,480],framerate=(fraction)30/1 ! "
          "appsink name=asink1 async=false", &error));
  fail_if (error);
  intersrc1 = gst_bin_get_by_name (GST_BIN (src1), "intersrc1");
  asink1 = gst_bin_get_by_name (GST_BIN (src1), "asink1");
  src2 =
      GST_PIPELINE (gst_parse_launch
      ("interpipesrc name=intersrc2 listen-to=intersink2 ! capsfilter caps=video/x-raw,format=(string)I420,width=1280,height=720,framerate=(fraction)30/1 ! "
          "appsink name=asink2 async=false", &error));
  fail_if (error);
  intersrc2 = gst_bin_get_by_name (GST_BIN (src2), "intersrc2");
  asink2 = gst_bin_get_by_name (GST_BIN (src2), "asink2");

  /* Install a signal to monitor caps */
  pad = GST_BASE_SRC_PAD (G_OBJECT (intersrc1));
  fail_if (!pad);
  g_object_notify (G_OBJECT (pad), "caps");

  /*connect the caps signal */
  conditiondata.mutex = &mutex;
  conditiondata.cond = &cond;
  conditiondata.condition = &capschange;
  id = g_signal_connect (pad, "notify::caps", G_CALLBACK (free_condition),
      (gpointer) & conditiondata);

  /* 
   * Play the pipelines
   * gst_element_get_state blocks up execution until the state change is
   * completed. It's used here to guarantee a secuential pipeline initialization
   * and avoid concurrency errors.
   */
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (src1), GST_STATE_PLAYING));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_get_state (GST_ELEMENT (src1), NULL, NULL,
          GST_CLOCK_TIME_NONE));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (src2), GST_STATE_PLAYING));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_get_state (GST_ELEMENT (src2), NULL, NULL,
          GST_CLOCK_TIME_NONE));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (sink1), GST_STATE_PLAYING));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_get_state (GST_ELEMENT (sink1), NULL, NULL,
          GST_CLOCK_TIME_NONE));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (sink2), GST_STATE_PLAYING));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_get_state (GST_ELEMENT (sink2), NULL, NULL,
          GST_CLOCK_TIME_NONE));

  /* 
   * Verifies if interpipesink and interpipesrc have caps set
   * Reconfigure event is omited when an interpipe sink has two listeners,
   * that's why sources have to listen to NULL first
   */
  g_object_set (G_OBJECT (intersrc1), "listen-to", NULL, NULL);
  g_object_set (G_OBJECT (intersrc2), "listen-to", NULL, NULL);
  g_object_set (G_OBJECT (intersrc1), "listen-to", "intersink2", NULL);
  g_object_set (G_OBJECT (intersrc2), "listen-to", "intersink1", NULL);

  /* wait for the caps signal */
  g_mutex_lock (&mutex);
  capschange = FALSE;
  end_time = g_get_monotonic_time () + 5 * G_TIME_SPAN_SECOND;
  while (!capschange) {
    if (!g_cond_wait_until (&cond, &mutex, end_time)) {
      // timeout has passed.
      g_mutex_unlock (&mutex);
    }
  }
  g_mutex_unlock (&mutex);

  caps2 = gst_app_src_get_caps (GST_APP_SRC (intersrc1));
  fail_if (!caps2);

  caps1 = gst_app_sink_get_caps (GST_APP_SINK (intersink2));
  fail_if (!caps1);

  fail_if (!gst_caps_is_equal (caps1, caps2));

  /* Stop pipelines */
  g_signal_handler_disconnect (pad, id);
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (sink1), GST_STATE_NULL));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (sink2), GST_STATE_NULL));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (src1), GST_STATE_NULL));
  fail_if (GST_STATE_CHANGE_FAILURE ==
      gst_element_set_state (GST_ELEMENT (src2), GST_STATE_NULL));

  /* Cleanup */
  g_object_unref (intersink1);
  g_object_unref (intersrc1);
  g_object_unref (sink1);
  g_object_unref (src1);
  g_object_unref (intersink2);
  g_object_unref (intersrc2);
  g_object_unref (sink2);
  g_object_unref (src2);
  g_object_unref (asink1);
  g_object_unref (asink2);
  g_mutex_clear (&mutex);
  g_cond_clear (&cond);
}

GST_END_TEST;


static Suite *
gst_interpipe_suite (void)
{
  Suite *suite = suite_create ("Interpipe");
  TCase *tc1 = tcase_create ("reconfigure_event");

  suite_add_tcase (suite, tc1);
  tcase_add_test (tc1, reconfigure_event);

  return suite;
}

GST_CHECK_MAIN (gst_interpipe);
