/* GStreamer
 * Copyright (C) 2016 Carlos Rodriguez <erick.arroyo@ridgerun.com>
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
 * Given two sinks with different caps, when a src switches 
 * from one to another, the src are renegotiaded
 */

GST_START_TEST (interpipe_caps_renegotiation_intersection)
{
  GstPipeline *sink1;
  GstPipeline *sink2;
  GstPipeline *src;
  GstPipeline *src2;
  GstElement *intersink2;
  GstElement *intersrc;
  GstCaps *caps1;
  GstCaps *caps2;
  gchar *listen_to_old;
  gchar *listen_to_new;
  GError *error = NULL;

  /* Create two sink pipelines */
  sink1 =
      GST_PIPELINE (gst_parse_launch
      ("videotestsrc name=vtsrc1 ! capsfilter caps=video/x-raw,format=(string)I420,width=640,height=480,framerate=(fraction)30/1 ! interpipesink "
          "name=sink1 sync=true", &error));
  fail_if (error);

  sink2 =
      GST_PIPELINE (gst_parse_launch
      ("videotestsrc name=vtsrc2 ! capsfilter caps=video/x-raw,format=(string)I420,width=320,height=240,framerate=(fraction)30/1 ! interpipesink "
          "name=intersink2 sync=true", &error));
  fail_if (error);

  intersink2 = gst_bin_get_by_name (GST_BIN (sink2), "intersink2");

  /* Create one source pipeline */

  src =
      GST_PIPELINE (gst_parse_launch
      ("interpipesrc name=intersrc listen-to=sink1 ! capsfilter caps=video/x-raw,format=(string)I420,width=[300,640],height=[200,480],framerate=(fraction)30/1 ! "
          "appsink name=asink async=false", &error));
  fail_if (error);

  intersrc = gst_bin_get_by_name (GST_BIN (src), "intersrc");

  src2 =
      GST_PIPELINE (gst_parse_launch
      ("interpipesrc name=intersrc2 listen-to=intersink2 ! capsfilter caps=video/x-raw,format=(string)I420,width=[300,640],height=[200,480],framerate=(fraction)30/1 ! "
          "appsink name=asink2 async=false", &error));
  fail_if (error);

  /* Play the pipelines */
  gst_element_change_state (GST_ELEMENT (sink1),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);
  gst_element_change_state (GST_ELEMENT (sink2),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);
  gst_element_change_state (GST_ELEMENT (src),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);
  gst_element_change_state (GST_ELEMENT (src2),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);

  /* Verifies if interpipesink and interpipesrc have the same caps
   */
  g_object_get (G_OBJECT (intersrc), "listen-to", &listen_to_old, NULL);

  sleep (1);

  g_object_set (G_OBJECT (intersrc), "listen-to", "intersink2", NULL);

  caps1 = gst_app_src_get_caps (GST_APP_SRC (intersrc));
  fail_if (!caps1);

  caps2 = gst_app_sink_get_caps (GST_APP_SINK (intersink2));
  fail_if (!caps2);

  fail_if (!gst_caps_is_equal (caps1, caps2));

  g_object_get (G_OBJECT (intersrc), "listen-to", &listen_to_new, NULL);
  fail_if (g_strcmp0 (listen_to_old, listen_to_new) == 0);

  /* Stop pipelines */
  gst_element_change_state (GST_ELEMENT (sink1),
      GST_STATE_CHANGE_READY_TO_NULL);
  gst_element_change_state (GST_ELEMENT (sink2),
      GST_STATE_CHANGE_READY_TO_NULL);
  gst_element_change_state (GST_ELEMENT (src), GST_STATE_CHANGE_READY_TO_NULL);
  gst_element_change_state (GST_ELEMENT (src2), GST_STATE_CHANGE_READY_TO_NULL);
  /* Cleanup */
  g_object_unref (intersink2);
  g_object_unref (intersrc);
  g_object_unref (sink1);
  g_object_unref (sink2);
  g_object_unref (src);
  g_object_unref (src2);
  gst_caps_unref (caps1);
  gst_caps_unref (caps2);
  g_free (listen_to_old);
  g_free (listen_to_new);
}

GST_END_TEST;

GST_START_TEST (interpipe_caps_renegotiation_no_intersection)
{
  GstPipeline *sink1;
  GstPipeline *sink2;
  GstPipeline *src;
  GstPipeline *src2;
  GstElement *intersink2;
  GstElement *intersrc;
  GstCaps *caps1;
  GstCaps *caps2;
  gchar *listen_to_old;
  gchar *listen_to_new;
  GError *error = NULL;

  /* Create two sink pipelines */
  sink1 =
      GST_PIPELINE (gst_parse_launch
      ("videotestsrc name=vtsrc1 ! capsfilter caps=video/x-raw,width=640,height=480,framerate=(fraction)30/1 ! interpipesink "
          "name=sink1 sync=true", &error));
  fail_if (error);

  sink2 =
      GST_PIPELINE (gst_parse_launch
      ("videotestsrc name=vtsrc2 ! capsfilter caps=video/x-raw,width=320,height=240,framerate=(fraction)30/1 ! interpipesink "
          "name=intersink2 sync=true", &error));
  fail_if (error);

  intersink2 = gst_bin_get_by_name (GST_BIN (sink2), "intersink2");

  /* Create one source pipeline */

  src =
      GST_PIPELINE (gst_parse_launch
      ("interpipesrc name=intersrc listen-to=sink1 ! capsfilter caps=video/x-raw,format=(string)I420,width=[400,640],height=[320,480],framerate=(fraction)30/1 ! "
          "appsink name=asink async=false", &error));
  fail_if (error);

  intersrc = gst_bin_get_by_name (GST_BIN (src), "intersrc");

  src2 =
      GST_PIPELINE (gst_parse_launch
      ("interpipesrc name=intersrc2 listen-to=intersink2 ! capsfilter caps=video/x-raw,format=(string)I420,width=320,height=240,framerate=(fraction)30/1 ! "
          "appsink name=asink2 async=false", &error));
  fail_if (error);


  /* Play the pipelines */
  gst_element_change_state (GST_ELEMENT (sink1),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);
  gst_element_change_state (GST_ELEMENT (sink2),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);
  gst_element_change_state (GST_ELEMENT (src),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);
  gst_element_change_state (GST_ELEMENT (src2),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);

  /* Verifies if interpipesink and interpipesrc have the same caps
   */
  g_object_get (G_OBJECT (intersrc), "listen-to", &listen_to_old, NULL);

  sleep (1);

  g_object_set (G_OBJECT (intersrc), "listen-to", "intersink2", NULL);

  caps1 = gst_app_src_get_caps (GST_APP_SRC (intersrc));
  fail_if (!caps1);

  caps2 = gst_app_sink_get_caps (GST_APP_SINK (intersink2));
  fail_if (!caps2);

  fail_if (gst_caps_is_equal (caps1, caps2));

  g_object_get (G_OBJECT (intersrc), "listen-to", &listen_to_new, NULL);
  fail_if (g_strcmp0 (listen_to_old, listen_to_new) != 0);

  /* Stop pipelines */
  gst_element_change_state (GST_ELEMENT (sink1),
      GST_STATE_CHANGE_READY_TO_NULL);
  gst_element_change_state (GST_ELEMENT (sink2),
      GST_STATE_CHANGE_READY_TO_NULL);
  gst_element_change_state (GST_ELEMENT (src), GST_STATE_CHANGE_READY_TO_NULL);
  gst_element_change_state (GST_ELEMENT (src2), GST_STATE_CHANGE_READY_TO_NULL);

  /* Cleanup */
  g_object_unref (intersink2);
  g_object_unref (intersrc);
  g_object_unref (sink1);
  g_object_unref (sink2);
  g_object_unref (src);
  g_object_unref (src2);
  gst_caps_unref (caps1);
  gst_caps_unref (caps2);
  g_free (listen_to_old);
  g_free (listen_to_new);
}

GST_END_TEST;


static Suite *
gst_interpipe_suite (void)
{
  Suite *suite = suite_create ("Interpipe");
  TCase *tc1 = tcase_create ("caps_renegotiation_intersection");
  TCase *tc2 = tcase_create ("caps_renegotiation_no_intersection");

  suite_add_tcase (suite, tc1);
  tcase_add_test (tc1, interpipe_caps_renegotiation_intersection);

  suite_add_tcase (suite, tc2);
  tcase_add_test (tc2, interpipe_caps_renegotiation_no_intersection);

  return suite;
}

GST_CHECK_MAIN (gst_interpipe);
