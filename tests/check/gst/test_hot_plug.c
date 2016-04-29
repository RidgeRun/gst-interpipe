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
 * Given two separate pipelines, when I repeatedly connect/disconnect then 
 * the pipeline will keep working fine.
 */

GST_START_TEST (interpipe_hot_plug_equal_caps)
{
  GstPipeline *sink1;
  GstPipeline *sink2;
  GstPipeline *src;
  GstElement *vtsrc1;
  GstElement *vtsrc2;
  GstElement *asink;
  GstElement *intersrc;
  char *listen_to_1;
  char *listen_to_2;
  GError *error = NULL;

  /* Create two sink pipelines */
  sink1 =
      GST_PIPELINE (gst_parse_launch
      ("videotestsrc name=vtsrc1 ! capsfilter caps=video/x-raw,width=1920,height=1080,framerate=(fraction)60/1 ! interpipesink "
          "name=sink1 async=true", &error));
  fail_if (error);

  vtsrc1 = gst_bin_get_by_name (GST_BIN (sink1), "vtsrc1");

  sink2 =
      GST_PIPELINE (gst_parse_launch
      ("videotestsrc name=vtsrc2 ! capsfilter caps=video/x-raw,width=1920,height=1080,framerate=(fraction)60/1 ! interpipesink "
          "name=sink2 async=true", &error));
  fail_if (error);

  vtsrc2 = gst_bin_get_by_name (GST_BIN (sink2), "vtsrc2");

  /* Create one source pipeline */

  src =
      GST_PIPELINE (gst_parse_launch
      ("interpipesrc name=intersrc listen-to=sink1 ! "
          "appsink name=asink async=false", &error));
  fail_if (error);

  asink = gst_bin_get_by_name (GST_BIN (src), "asink");
  intersrc = gst_bin_get_by_name (GST_BIN (src), "intersrc");

  /* Play the pipelines */
  gst_element_change_state (GST_ELEMENT (sink1),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);
  gst_element_change_state (GST_ELEMENT (sink2),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);
  gst_element_change_state (GST_ELEMENT (src),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);


  /* Check the listen-to property when its changed. */
  g_object_get (G_OBJECT (intersrc), "listen-to", &listen_to_1, NULL);
  g_object_set (G_OBJECT (intersrc), "listen-to", "sink2", NULL);
  g_object_get (G_OBJECT (intersrc), "listen-to", &listen_to_2, NULL);
  fail_if (g_strcmp0 (listen_to_1, listen_to_2) == 0);

  /* Stop pipelines */
  gst_element_change_state (GST_ELEMENT (sink1),
      GST_STATE_CHANGE_READY_TO_NULL);
  gst_element_change_state (GST_ELEMENT (sink2),
      GST_STATE_CHANGE_READY_TO_NULL);
  gst_element_change_state (GST_ELEMENT (src), GST_STATE_CHANGE_READY_TO_NULL);

  /* Cleanup */
  g_object_unref (vtsrc1);
  g_object_unref (vtsrc2);
  g_object_unref (asink);
  g_object_unref (intersrc);
  g_object_unref (sink1);
  g_object_unref (sink2);
  g_object_unref (src);
  g_free (listen_to_1);
  g_free (listen_to_2);

}

GST_END_TEST;

GST_START_TEST (interpipe_hot_plug_different_caps)
{
  GstPipeline *sink1;
  GstPipeline *sink2;
  GstPipeline *src;
  GstElement *vtsrc1;
  GstElement *vtsrc2;
  GstElement *asink;
  GstElement *intersrc;
  char *listen_to_1;
  char *listen_to_2;
  GError *error = NULL;

  /* Create two sink pipelines */
  sink1 =
      GST_PIPELINE (gst_parse_launch
      ("videotestsrc name=vtsrc1 ! capsfilter caps=video/x-raw,width=1920,height=1080,framerate=(fraction)60/1 ! interpipesink "
          "name=sink1 async=true", &error));
  fail_if (error);

  vtsrc1 = gst_bin_get_by_name (GST_BIN (sink1), "vtsrc1");

  sink2 =
      GST_PIPELINE (gst_parse_launch
      ("videotestsrc name=vtsrc2 ! capsfilter caps=video/x-raw,width=1920,height=1080,framerate=(fraction)30/1 ! interpipesink "
          "name=sink2 async=true", &error));
  fail_if (error);

  vtsrc2 = gst_bin_get_by_name (GST_BIN (sink2), "vtsrc2");

  /* Create one source pipeline */

  src =
      GST_PIPELINE (gst_parse_launch
      ("interpipesrc name=intersrc allow-renegotiation=false listen-to=sink1 ! "
          "appsink name=asink async=false", &error));
  fail_if (error);

  asink = gst_bin_get_by_name (GST_BIN (src), "asink");
  intersrc = gst_bin_get_by_name (GST_BIN (src), "intersrc");

  /* Play the pipelines */
  gst_element_change_state (GST_ELEMENT (sink1),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);
  gst_element_change_state (GST_ELEMENT (sink2),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);
  gst_element_change_state (GST_ELEMENT (src),
      GST_STATE_CHANGE_PAUSED_TO_PLAYING);


  /* Check the listen-to property when its changed. */
  sleep (2);
  g_object_get (G_OBJECT (intersrc), "listen-to", &listen_to_1, NULL);
  g_object_set (G_OBJECT (intersrc), "listen-to", "sink2", NULL);
  g_object_get (G_OBJECT (intersrc), "listen-to", &listen_to_2, NULL);
  /* fail_if (!(g_strcmp0 (listen_to_1, listen_to_2) == 0)); */

  /* Stop pipelines */
  gst_element_change_state (GST_ELEMENT (sink1),
      GST_STATE_CHANGE_READY_TO_NULL);
  gst_element_change_state (GST_ELEMENT (sink2),
      GST_STATE_CHANGE_READY_TO_NULL);
  gst_element_change_state (GST_ELEMENT (src), GST_STATE_CHANGE_READY_TO_NULL);

  /* Cleanup */
  g_object_unref (vtsrc1);
  g_object_unref (vtsrc2);
  g_object_unref (asink);
  g_object_unref (intersrc);
  g_object_unref (sink1);
  g_object_unref (sink2);
  g_object_unref (src);
  g_free (listen_to_1);
  g_free (listen_to_2);

}

GST_END_TEST;


static Suite *
gst_interpipe_suite (void)
{
  Suite *suite = suite_create ("Interpipe");
  TCase *tc1 = tcase_create ("hot_plug_equal_caps");
  TCase *tc2 = tcase_create ("hot_plug_different_caps");

  suite_add_tcase (suite, tc1);
  tcase_add_test (tc1, interpipe_hot_plug_equal_caps);

  suite_add_tcase (suite, tc2);
  tcase_add_test (tc2, interpipe_hot_plug_different_caps);

  return suite;
}

GST_CHECK_MAIN (gst_interpipe);
