/* GStreamer
 * Copyright (C) 2013-2016 Michael Grüner <michael.gruner@ridgerun.com>
 * Copyright (C) 2014 Jose Jimenez <jose.jimenez@ridgerun.com>
 * Copyright (C) 2016 Carlos Rodriguez <carlos.rodriguez@ridgerun.com>
 * Copyright (C) 2016 Erick Arroyo <erick.arroyo@ridgerun.com>
 * Copyright (C) 2016 Marco Madrigal <marco.madrigal@ridgerun.com>
 *
 * This file is part of gst-interpipe-1.0
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
/**
 * SECTION:gstinterpipesink
 * @see_also: #GstInterPipeSrc
 *
 * Sink element for interpipeline communication
 *
 * <refsect2>
 * <title>Example launch line</title>
 * |[
 * gst-launch \
 *   videotestsrc ! interpipesink name=test \
 *   interpipesrc listen-to=test ! xvimagesink
 * ]| Send buffers across two different pipelines
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <gst/gst.h>

#include "gstinterpipesink.h"
#include "gstinterpipeinode.h"

GST_DEBUG_CATEGORY_STATIC (gst_inter_pipe_sink_debug);
#define GST_CAT_DEFAULT gst_inter_pipe_sink_debug

enum
{
  PROP_0,
  PROP_FORWARD_EOS,
  PROP_FORWARD_EVENTS,
  PROP_NUM_LISTENERS
};

static void gst_inter_pipe_sink_update_node_name (GstInterPipeSink * sink,
    GParamSpec * pspec);
static void gst_inter_pipe_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_inter_pipe_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_inter_pipe_sink_finalize (GObject * object);
static GstFlowReturn gst_inter_pipe_sink_new_buffer (GstAppSink * sink,
    gpointer data);
static GstFlowReturn gst_inter_pipe_sink_new_preroll (GstAppSink * asink,
    gpointer data);
static void gst_inter_pipe_sink_eos (GstAppSink * sink, gpointer data);
static gboolean gst_inter_pipe_sink_add_listener (GstInterPipeINode * iface,
    GstInterPipeIListener * listener);
static gboolean gst_inter_pipe_sink_remove_listener (GstInterPipeINode * iface,
    GstInterPipeIListener * listener);
static gboolean gst_inter_pipe_sink_receive_event (GstInterPipeINode * iface,
    GstEvent * event);
static GstCaps *gst_inter_pipe_sink_get_caps (GstBaseSink * base,
    GstCaps * filter);
static gboolean gst_inter_pipe_sink_set_caps (GstBaseSink * base,
    GstCaps * filter);
static gboolean gst_inter_pipe_sink_event (GstBaseSink * base,
    GstEvent * event);
static gboolean gst_inter_pipe_sink_propose_allocation (GstBaseSink * base,
    GstQuery * query);
static gboolean gst_inter_pipe_sink_are_caps_compatible (GstInterPipeSink *
    sink, GstCaps * listener_caps, GstCaps * sinkcaps);
static GstCaps *gst_inter_pipe_sink_caps_intersect (GstCaps * caps1,
    GstCaps * caps2);
static void gst_inter_pipe_sink_intersect_listener_caps (gpointer key,
    gpointer value, gpointer user_data);
static void gst_inter_pipe_sink_forward_event (gpointer key, gpointer value,
    gpointer user_data);

static void gst_inter_pipe_inode_init (GstInterPipeINodeInterface * iface);

#define GST_INTER_PIPE_SINK_PAD(obj)                 (GST_BASE_SINK_CAST (obj)->sinkpad)

struct _GstInterPipeSink
{
  GstAppSink parent;

  /** Node name */
  gchar *node_name;

  /** The list of listeners */
  GHashTable *listeners;

  /** Enable Events notify */
  gboolean forward_events;

  /** Enable EOS notify */
  gboolean forward_eos;

  /** Last caps */
  GstCaps *caps;

  /** Negotiated caps  **/
  GstCaps *caps_negotiated;

  /** Last buffer timestamp */
  guint64 last_buffer_timestamp;

  GMutex listeners_mutex;
};

struct _GstInterPipeSinkClass
{
  GstAppSinkClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (GstInterPipeSink, gst_inter_pipe_sink,
    GST_TYPE_APP_SINK, G_IMPLEMENT_INTERFACE (GST_INTER_PIPE_TYPE_INODE,
        gst_inter_pipe_inode_init));

static void
gst_inter_pipe_sink_class_init (GstInterPipeSinkClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstBaseSinkClass *basesink_class;

  gobject_class = G_OBJECT_CLASS (klass);
  element_class = GST_ELEMENT_CLASS (klass);
  basesink_class = GST_BASE_SINK_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_inter_pipe_sink_debug, "interpipesink", 0,
      "interpipeline sink");

  gst_element_class_set_static_metadata (element_class,
      "Internal pipeline sink",
      "Generic/Sink",
      "Sink for internal pipeline buffers communication",
      "Michael Grüner <michael.gruner@ridgerun.com>");

  gobject_class->set_property = gst_inter_pipe_sink_set_property;
  gobject_class->get_property = gst_inter_pipe_sink_get_property;
  gobject_class->finalize = gst_inter_pipe_sink_finalize;

  g_object_class_install_property (gobject_class, PROP_FORWARD_EOS,
      g_param_spec_boolean ("forward-eos", "Forward EOS",
          "Forward the EOS event to all the listeners",
          FALSE, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_FORWARD_EVENTS,
      g_param_spec_boolean ("forward-events", "Forward events",
          "Forward downstream events to all the listeners (except for EOS)",
          TRUE, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_NUM_LISTENERS,
      g_param_spec_uint ("num-listeners", "Number of listeners",
          "Number of interpipe sources listening to this specific sink",
          0, G_MAXUINT, 0, G_PARAM_READABLE));

  basesink_class->get_caps = GST_DEBUG_FUNCPTR (gst_inter_pipe_sink_get_caps);
  basesink_class->set_caps = GST_DEBUG_FUNCPTR (gst_inter_pipe_sink_set_caps);
  basesink_class->event = GST_DEBUG_FUNCPTR (gst_inter_pipe_sink_event);
  basesink_class->propose_allocation =
      GST_DEBUG_FUNCPTR (gst_inter_pipe_sink_propose_allocation);
}

static void
gst_inter_pipe_sink_update_node_name (GstInterPipeSink * sink,
    GParamSpec * pspec)
{
  GstInterPipeINode *node;

  node = GST_INTER_PIPE_INODE (sink);

  if (sink->node_name) {
    gst_inter_pipe_remove_node (node, sink->node_name);
    g_free (sink->node_name);
  }

  sink->node_name = gst_object_get_name (GST_OBJECT (sink));
  gst_inter_pipe_add_node (node, sink->node_name);
}

static void
gst_inter_pipe_sink_init (GstInterPipeSink * sink)
{
  GstAppSinkCallbacks callbacks;

  sink->caps = NULL;
  sink->caps_negotiated = NULL;
  sink->node_name = NULL;
  sink->listeners = g_hash_table_new (g_direct_hash, g_direct_equal);
  sink->forward_eos = FALSE;
  sink->forward_events = FALSE;
  sink->last_buffer_timestamp = 0;

  g_mutex_init (&sink->listeners_mutex);

  /* Set the struct buffer to 0's so if in the future more callbacks are added
   * does not cause a segmentation fault down the line
   */
  memset (&callbacks, 0, sizeof (callbacks));

  /* AppSink callbacks */
  callbacks.eos = GST_DEBUG_FUNCPTR (gst_inter_pipe_sink_eos);
  callbacks.new_sample = GST_DEBUG_FUNCPTR (gst_inter_pipe_sink_new_buffer);
  callbacks.new_preroll = GST_DEBUG_FUNCPTR (gst_inter_pipe_sink_new_preroll);
  gst_app_sink_set_callbacks (GST_APP_SINK (sink), &callbacks, NULL, NULL);

  /*AppSink configuration */
  gst_app_sink_set_drop (GST_APP_SINK (sink), TRUE);
  gst_base_sink_set_sync (GST_BASE_SINK (sink), FALSE);
  gst_app_sink_set_max_buffers (GST_APP_SINK (sink), 3);

  /* When a change in the interpipesink name happens, the callback function
     will update the node name and the nodes list */
  g_object_notify (G_OBJECT (sink), "name");

  g_signal_connect (sink, "notify::name",
      G_CALLBACK (gst_inter_pipe_sink_update_node_name), NULL);
}


static void
gst_inter_pipe_sink_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstInterPipeSink *sink;

  g_return_if_fail (GST_IS_INTER_PIPE_SINK (object));

  sink = GST_INTER_PIPE_SINK (object);

  switch (prop_id) {
    case PROP_FORWARD_EOS:
      sink->forward_eos = g_value_get_boolean (value);
      break;
    case PROP_FORWARD_EVENTS:
      sink->forward_events = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_inter_pipe_sink_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstInterPipeSink *sink;
  GHashTable *listeners;

  g_return_if_fail (GST_IS_INTER_PIPE_SINK (object));

  sink = GST_INTER_PIPE_SINK (object);
  listeners = GST_INTER_PIPE_SINK_LISTENERS (sink);

  switch (prop_id) {
    case PROP_NUM_LISTENERS:
      g_mutex_lock (&sink->listeners_mutex);
      g_value_set_uint (value, g_hash_table_size (listeners));
      g_mutex_unlock (&sink->listeners_mutex);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_inter_pipe_sink_finalize (GObject * object)
{
  GstInterPipeSink *sink;
  GstInterPipeINode *node;

  sink = GST_INTER_PIPE_SINK (object);
  node = GST_INTER_PIPE_INODE (sink);

  if (sink->node_name != NULL) {
    GST_DEBUG_OBJECT (sink, "Removing node %s and associated listeners",
        sink->node_name);
    gst_inter_pipe_remove_node (node, sink->node_name);
    g_free (sink->node_name);
  }

  if (sink->caps) {
    gst_caps_unref (sink->caps);
  }

  if (sink->caps_negotiated) {
    gst_caps_unref (sink->caps_negotiated);
  }

  g_hash_table_destroy (sink->listeners);

  g_mutex_clear (&sink->listeners_mutex);

  /* Chain up to the parent class */
  G_OBJECT_CLASS (gst_inter_pipe_sink_parent_class)->finalize (object);
}

static void
gst_inter_pipe_sink_update_listener_caps (gpointer key, gpointer data,
    gpointer user_data)
{
  GstInterPipeIListener *listener;
  GstAppSink *appsink;
  gchar *listener_name;
  GstCaps *caps;
  gpointer *data_array = user_data;

  appsink = GST_APP_SINK (data_array[0]);
  caps = GST_CAPS (data_array[1]);

  listener = GST_INTER_PIPE_ILISTENER (data);
  listener_name = (gchar *) key;

  GST_LOG_OBJECT (appsink, "Setting caps %" GST_PTR_FORMAT " to %s",
      caps, listener_name);

  gst_inter_pipe_ilistener_set_caps (listener,
      GST_INTER_PIPE_SINK (appsink)->caps);
}


static gboolean
gst_inter_pipe_sink_are_caps_compatible (GstInterPipeSink * sink,
    GstCaps * listener_caps, GstCaps * sinkcaps)
{
  GstCaps *renegotiated_caps = NULL;

  renegotiated_caps = gst_caps_intersect (listener_caps, sinkcaps);

  if (gst_caps_is_empty (renegotiated_caps)) {
    GST_ERROR_OBJECT (sink, "No caps intersection between listener and sink");
    return FALSE;
  }

  GST_INFO_OBJECT (sink, "Renegotiated caps: %" GST_PTR_FORMAT,
      renegotiated_caps);

  return TRUE;
}

static GstCaps *
gst_inter_pipe_sink_caps_intersect (GstCaps * caps1, GstCaps * caps2)
{
  if (!caps1 && !caps2) {
    return NULL;
  }

  if (!caps1) {
    return gst_caps_ref (caps2);
  }

  if (!caps2) {
    return gst_caps_ref (caps1);
  }

  return gst_caps_intersect (caps1, caps2);
}

static void
gst_inter_pipe_sink_intersect_listener_caps (gpointer key, gpointer value,
    gpointer user_data)
{
  GstInterPipeSink *sink;
  GstInterPipeIListener *listener;
  GstCaps *caps_listener;
  GstCaps *caps_intersection;
  gboolean src_negotiated;

  sink = GST_INTER_PIPE_SINK (user_data);
  listener = GST_INTER_PIPE_ILISTENER (value);

  caps_listener = NULL;
  caps_intersection = NULL;

  caps_listener = gst_inter_pipe_ilistener_get_caps (listener, &src_negotiated);
  GST_INFO_OBJECT (sink, "Listener %s caps: %" GST_PTR_FORMAT,
      GST_OBJECT_NAME (listener), caps_listener);

  caps_intersection =
      gst_inter_pipe_sink_caps_intersect (caps_listener, sink->caps_negotiated);

  /* Replace old intersection with new one */
  if (sink->caps_negotiated) {
    gst_caps_unref (sink->caps_negotiated);
  }

  sink->caps_negotiated = caps_intersection;
  gst_caps_unref (caps_listener);
}

static GstCaps *
gst_inter_pipe_sink_get_caps (GstBaseSink * base, GstCaps * filter)
{
  GstInterPipeSink *sink;
  GstInterPipeIListener *listener;
  GHashTable *listeners;
  GList *listeners_list = NULL;
  GList *l = NULL;

  sink = GST_INTER_PIPE_SINK (base);

  g_mutex_lock (&sink->listeners_mutex);
  listeners = GST_INTER_PIPE_SINK_LISTENERS (sink);

  if (0 == g_hash_table_size (listeners)) {
    GST_INFO_OBJECT (sink, "No listeners yet, accepting any caps");
    if (filter)
      filter = gst_caps_ref (filter);
    goto nolisteners;
  }

  GST_INFO_OBJECT (sink, "Caps from upstream: %" GST_PTR_FORMAT, filter);

  /* Find the intersection of all the listeners */
  g_hash_table_foreach (listeners, gst_inter_pipe_sink_intersect_listener_caps,
      sink);
  g_mutex_unlock (&sink->listeners_mutex);

  if (!sink->caps_negotiated || gst_caps_is_empty (sink->caps_negotiated)) {
    GST_ERROR_OBJECT (sink,
        "Failed to obtain an intersection between listener caps");
    goto nointersection;
  }

  GST_INFO_OBJECT (sink, "Caps negotiated between listeners: %" GST_PTR_FORMAT,
      sink->caps_negotiated);

  if (filter) {
    /* Check if listener caps and upstream caps can intersect */
    if (!sink->caps_negotiated
        || !gst_caps_can_intersect (sink->caps_negotiated, filter)) {
      GST_ERROR_OBJECT (sink,
          "Failed to obtain an intersection between upstream elements and listeners");
      goto nointersection;
    }
  }

  return gst_caps_ref (sink->caps_negotiated);

nolisteners:
  {
    g_mutex_unlock (&sink->listeners_mutex);
    return filter;
  }

nointersection:
  {
    listeners_list = g_hash_table_get_values (listeners);
    if (listeners_list) {
      for (l = listeners_list; l != NULL; l = l->next) {
        listener = l->data;
        if (!gst_inter_pipe_leave_node (listener))
          GST_WARNING_OBJECT (listener, "Unable to remove listener from node");
      }
    }
    g_list_free (listeners_list);

    if (sink->caps_negotiated)
      gst_caps_unref (sink->caps_negotiated);

    sink->caps_negotiated = NULL;
    return NULL;
  }
}

static gboolean
gst_inter_pipe_sink_set_caps (GstBaseSink * base, GstCaps * caps)
{
  GstInterPipeSink *sink;
  GHashTable *listeners;
  gboolean ret = TRUE;

  sink = GST_INTER_PIPE_SINK (base);
  listeners = GST_INTER_PIPE_SINK_LISTENERS (sink);

  GST_BASE_SINK_CLASS (gst_inter_pipe_sink_parent_class)->set_caps (base, caps);

  GST_INFO_OBJECT (sink, "Incoming Caps: %" GST_PTR_FORMAT, caps);
  GST_INFO_OBJECT (sink, "Negotiated Caps: %" GST_PTR_FORMAT,
      sink->caps_negotiated);

  gst_caps_replace (&sink->caps, caps);
  gst_app_sink_set_caps (GST_APP_SINK (sink), caps);

  /* No one is listening to me I can accept caps */
  if (0 == g_hash_table_size (listeners))
    return TRUE;

  g_mutex_lock (&sink->listeners_mutex);
  if (sink->caps_negotiated
      && (gst_caps_can_intersect (sink->caps_negotiated, caps))) {
    gpointer data[2];

    data[0] = sink;
    data[1] = caps;
    g_hash_table_foreach (listeners, gst_inter_pipe_sink_update_listener_caps,
        data);

    GST_INFO_OBJECT (sink, "Listeners caps updated");
  } else {
    GST_WARNING_OBJECT (sink,
        "There's not caps intersection between node %s and listeners. Caps won't be set",
        sink->node_name);
    ret = FALSE;
  }

  g_mutex_unlock (&sink->listeners_mutex);
  return ret;

}

static void
gst_inter_pipe_sink_forward_event (gpointer key, gpointer data,
    gpointer user_data)
{
  GstInterPipeIListener *listener;
  GstInterPipeSink *sink;
  GstEvent *event;
  guint64 basetime;
  gpointer *data_array;

  listener = GST_INTER_PIPE_ILISTENER (data);
  data_array = user_data;
  sink = GST_INTER_PIPE_SINK (data_array[0]);
  event = GST_EVENT (data_array[1]);

  if (GST_EVENT_IS_SERIALIZED (event)) {
    GST_INFO_OBJECT (sink, "Incoming serialized event %s",
        GST_EVENT_TYPE_NAME (event));

    /* Update serial event timestamp */
    GST_EVENT_TIMESTAMP (event) = sink->last_buffer_timestamp;
    GST_INFO_OBJECT (sink, "Event timestamp %" GST_TIME_FORMAT,
        GST_TIME_ARGS (GST_EVENT_TIMESTAMP (event)));
  } else {
    GST_INFO_OBJECT (sink, "Incoming non-serialized event %s",
        GST_EVENT_TYPE_NAME (event));
  }

  switch (GST_EVENT_TYPE (event)) {
    case GST_EVENT_EOS:
    case GST_EVENT_CAPS:
      /*We manage the event with other functions */
      break;
    default:
      basetime = gst_element_get_base_time (GST_ELEMENT (sink));
      gst_inter_pipe_ilistener_push_event (listener, gst_event_ref (event),
          basetime);
      break;
  }
}

static gboolean
gst_inter_pipe_sink_event (GstBaseSink * base, GstEvent * event)
{
  GstInterPipeSink *sink;
  GHashTable *listeners;
  gpointer data_array[2];

  sink = GST_INTER_PIPE_SINK (base);

  g_mutex_lock (&sink->listeners_mutex);
  listeners = GST_INTER_PIPE_SINK_LISTENERS (sink);

  if (sink->forward_events) {
    data_array[0] = sink;
    data_array[1] = event;
    g_hash_table_foreach (listeners, gst_inter_pipe_sink_forward_event,
        (gpointer) data_array);
  }
  g_mutex_unlock (&sink->listeners_mutex);
  return GST_BASE_SINK_CLASS (gst_inter_pipe_sink_parent_class)->event (base,
      event);
}


struct AllocQueryCtx
{
  GstInterPipeSink *sink;
  GstQuery *query;
  GstAllocationParams params;
  guint size;
  guint min_buffers;
  gboolean first_query;
  guint num_listeners;
};


static gboolean
gst_inter_pipe_sink_forward_query_allocation (gpointer key, gpointer data,
    gpointer user_data)
{
  struct AllocQueryCtx *ctx;
  GstInterPipeIListener *listener;
  gchar *listener_name;
  GstInterPipeSink *sink;
  GstQuery *query;
  GstCaps *caps;
  gboolean ret = TRUE;
  guint count, i, size, min;

  listener = GST_INTER_PIPE_ILISTENER (data);
  listener_name = (gchar *) key;
  ctx = user_data;
  sink = ctx->sink;

  GST_DEBUG_OBJECT (sink, "Aggregating allocation from listener %s",
      listener_name);

  gst_query_parse_allocation (ctx->query, &caps, NULL);

  query = gst_query_new_allocation (caps, FALSE);
  if (!gst_inter_pipe_ilistener_query (listener, query)) {
    GST_DEBUG_OBJECT (sink,
        "Allocation query failed on listener %s, ignoring allocation",
        listener_name);
    ret = FALSE;
    goto out;
  }

  /* Allocation Filter, extract of code from tee element */

  /* Allocation Params:
   * store the maximum alignment, prefix and padding, but ignore the
   * allocators and the flags which are tied to downstream allocation*/
  count = gst_query_get_n_allocation_params (query);
  for (i = 0; i < count; i++) {
    GstAllocationParams params = { 0, };

    gst_query_parse_nth_allocation_param (query, i, NULL, &params);

    GST_DEBUG_OBJECT (sink, "Aggregating AllocationParams align=%"
        G_GSIZE_FORMAT " prefix=%" G_GSIZE_FORMAT " padding=%"
        G_GSIZE_FORMAT, params.align, params.prefix, params.padding);

    if (ctx->params.align < params.align)
      ctx->params.align = params.align;

    if (ctx->params.prefix < params.prefix)
      ctx->params.prefix = params.prefix;

    if (ctx->params.padding < params.padding)
      ctx->params.padding = params.padding;
  }

  /* Allocation Pool:
   * We want to keep the biggest size and biggest minimum number of buffers to
   * make sure downstream requirement can be satisfied. We don't really care
   * about the maximum, as this is a parameter of the downstream provided
   * pool. We only read the first allocation pool as the minimum number of
   * buffers is normally constant regardless of the pool being used. */
  if (gst_query_get_n_allocation_pools (query) > 0) {
    gst_query_parse_nth_allocation_pool (query, 0, NULL, &size, &min, NULL);

    GST_DEBUG_OBJECT (sink,
        "Aggregating allocation pool size=%u min_buffers=%u", size, min);

    if (ctx->size < size)
      ctx->size = size;

    if (ctx->min_buffers < min)
      ctx->min_buffers = min;
  }

  /* Allocation Meta:
   * For allocation meta, we'll need to aggregate the argument using the new
   * GstMetaInfo::agggregate_func */
  count = gst_query_get_n_allocation_metas (query);
  for (i = 0; i < count; i++) {
    guint ctx_index;
    GType api;
    const GstStructure *param;

    api = gst_query_parse_nth_allocation_meta (query, i, &param);

    /* For the first query, copy all metas */
    if (ctx->first_query) {
      gst_query_add_allocation_meta (ctx->query, api, param);
      continue;
    }

    /* Afterward, aggregate the common params */
    if (gst_query_find_allocation_meta (ctx->query, api, &ctx_index)) {
      const GstStructure *ctx_param;

      gst_query_parse_nth_allocation_meta (ctx->query, ctx_index, &ctx_param);

      /* Keep meta which has no params */
      if (ctx_param == NULL && param == NULL)
        continue;

      GST_DEBUG_OBJECT (sink, "Dropping allocation meta %s", g_type_name (api));
      gst_query_remove_nth_allocation_meta (ctx->query, ctx_index);
    }
  }

  /* Finally, cleanup metas from the stored query that aren't support on this
   * listener. */
  count = gst_query_get_n_allocation_metas (ctx->query);
  for (i = 0; i < count;) {
    GType api = gst_query_parse_nth_allocation_meta (ctx->query, i, NULL);

    if (!gst_query_find_allocation_meta (query, api, NULL)) {
      GST_DEBUG_OBJECT (sink, "Dropping allocation meta %s", g_type_name (api));
      gst_query_remove_nth_allocation_meta (ctx->query, i);
      count--;
      continue;
    }

    i++;
  }

  ctx->first_query = FALSE;
  ctx->num_listeners++;

out:
  gst_query_unref (query);
  return ret;
}

static gboolean
gst_inter_pipe_sink_propose_allocation (GstBaseSink * base, GstQuery * query)
{
  struct AllocQueryCtx ctx = { 0 };
  GstInterPipeSink *sink;
  GHashTable *listeners;
  GHashTableIter iter;
  gboolean ret = TRUE;
  gpointer key, value;

  sink = GST_INTER_PIPE_SINK (base);

  g_mutex_lock (&sink->listeners_mutex);
  listeners = GST_INTER_PIPE_SINK_LISTENERS (sink);

  ctx.sink = sink;
  ctx.query = query;
  ctx.first_query = TRUE;
  gst_allocation_params_init (&ctx.params);

  g_hash_table_iter_init (&iter, listeners);

  while (g_hash_table_iter_next (&iter, &key, &value)) {
    ret |= gst_inter_pipe_sink_forward_query_allocation (key, value, &ctx);
  }

  if (ret) {
    guint count = gst_query_get_n_allocation_metas (query);
    guint i;

    GST_DEBUG_OBJECT (sink,
        "Final allocation parameters: align=%" G_GSIZE_FORMAT " prefix=%"
        G_GSIZE_FORMAT " padding %" G_GSIZE_FORMAT, ctx.params.align,
        ctx.params.prefix, ctx.params.padding);

    GST_DEBUG_OBJECT (sink, "Final allocation pools: size=%u  min_buffers=%u",
        ctx.size, ctx.min_buffers);

    GST_DEBUG_OBJECT (sink, "Final %u allocation meta:", count);

    for (i = 0; i < count; i++) {
      GST_DEBUG_OBJECT (sink, "    + aggregated allocation meta %s",
          g_type_name (gst_query_parse_nth_allocation_meta (ctx.query, i,
                  NULL)));
    }

    /* Allocate one more buffers when multiplexing so we don't starve the
     * downstream threads. */
    if (ctx.num_listeners > 1)
      ctx.min_buffers++;

    /* Check that we actually have parameters besides the defaults. */
    if (ctx.params.align || ctx.params.prefix || ctx.params.padding) {
      gst_query_add_allocation_param (ctx.query, NULL, &ctx.params);
    }

    /* When size == 0, buffers created from this pool would have no memory
     * allocated. */
    if (ctx.size) {
      gst_query_add_allocation_pool (ctx.query, NULL, ctx.size,
          ctx.min_buffers, 0);
    }

  } else {
    guint count = gst_query_get_n_allocation_metas (query);
    guint i;

    for (i = 1; i <= count; i++) {
      gst_query_remove_nth_allocation_meta (query, count - i);
    }
  }

  g_mutex_unlock (&sink->listeners_mutex);

  return ret;
}

/* Appsink Callbacks */
static void
gst_inter_pipe_sink_push_to_listener (gpointer key, gpointer data,
    gpointer user_data)
{
  GstInterPipeIListener *listener;
  GstInterPipeSink *sink;
  GstBuffer *buffer;
  guint64 basetime;
  gchar *listener_name;
  gpointer *data_array = user_data;

  sink = GST_INTER_PIPE_SINK (data_array[0]);
  buffer = gst_buffer_ref (GST_BUFFER (data_array[1]));

  listener = GST_INTER_PIPE_ILISTENER (data);
  listener_name = (gchar *) key;

  GST_LOG_OBJECT (sink, "Forwarding buffer %p to %s", buffer, listener_name);

  basetime = gst_element_get_base_time (GST_ELEMENT (sink));
  gst_inter_pipe_ilistener_push_buffer (listener, buffer, basetime);
}

static void
gst_inter_pipe_sink_process_sample (GstInterPipeSink * sink, GstSample * sample)
{
  GHashTable *listeners;
  GstBuffer *buffer;
  gpointer data[2];

  g_mutex_lock (&sink->listeners_mutex);
  listeners = GST_INTER_PIPE_SINK_LISTENERS (sink);

  buffer = gst_sample_get_buffer (sample);

  /* Update last_buffer_timestamp */
  sink->last_buffer_timestamp = GST_BUFFER_PTS (buffer);

  GST_LOG_OBJECT (sink, "Received new buffer %p on node %s", buffer,
      sink->node_name);

  data[0] = sink;
  data[1] = buffer;
  g_hash_table_foreach (listeners, gst_inter_pipe_sink_push_to_listener, data);
  gst_sample_unref (sample);

  g_mutex_unlock (&sink->listeners_mutex);

}

static GstFlowReturn
gst_inter_pipe_sink_new_buffer (GstAppSink * asink, gpointer data)
{
  GstInterPipeSink *sink;
  GstSample *sample;

  sink = GST_INTER_PIPE_SINK (asink);

  sample = gst_app_sink_pull_sample (asink);
  gst_inter_pipe_sink_process_sample (sink, sample);

  return GST_FLOW_OK;
}


static GstFlowReturn
gst_inter_pipe_sink_new_preroll (GstAppSink * asink, gpointer data)
{
  GstInterPipeSink *sink;
  GstSample *sample;

  sink = GST_INTER_PIPE_SINK (asink);

  sample = gst_app_sink_pull_preroll (asink);
  gst_inter_pipe_sink_process_sample (sink, sample);

  return GST_FLOW_OK;
}

static void
gst_inter_pipe_sink_send_eos (gpointer key, gpointer data, gpointer user_data)
{
  GstInterPipeSink *sink;
  GstInterPipeIListener *listener;
  gchar *listener_name;

  sink = GST_INTER_PIPE_SINK (user_data);
  listener = GST_INTER_PIPE_ILISTENER (data);
  listener_name = (gchar *) key;

  GST_LOG_OBJECT (sink, "Forwarding EOS to %s", listener_name);

  gst_inter_pipe_ilistener_send_eos (listener);
}

static void
gst_inter_pipe_sink_eos (GstAppSink * asink, gpointer data)
{
  GstInterPipeSink *sink;
  GHashTable *listeners;

  sink = GST_INTER_PIPE_SINK (asink);

  g_mutex_lock (&sink->listeners_mutex);
  listeners = GST_INTER_PIPE_SINK_LISTENERS (sink);

  GST_LOG_OBJECT (sink, "Received new EOS on node %s", sink->node_name);

  if (sink->forward_eos) {
    g_hash_table_foreach (listeners, gst_inter_pipe_sink_send_eos,
        (gpointer) sink);
  } else {
    GST_LOG_OBJECT (sink, "Ignoring EOS");
  }
  g_mutex_unlock (&sink->listeners_mutex);
}

/* GstInterPipeINode interface implementation */
static void
gst_inter_pipe_inode_init (GstInterPipeINodeInterface * iface)
{
  iface->add_listener = gst_inter_pipe_sink_add_listener;
  iface->remove_listener = gst_inter_pipe_sink_remove_listener;
  iface->receive_event = gst_inter_pipe_sink_receive_event;
}

static gboolean
gst_inter_pipe_sink_add_listener (GstInterPipeINode * iface,
    GstInterPipeIListener * listener)
{
  GstInterPipeSink *sink;
  GHashTable *listeners;
  const gchar *listener_name;
  GstCaps *srccaps, *sinkcaps;
  gboolean src_negotiated;

  g_return_val_if_fail (iface, FALSE);
  g_return_val_if_fail (listener, FALSE);

  sink = GST_INTER_PIPE_SINK (iface);

  listeners = GST_INTER_PIPE_SINK_LISTENERS (sink);
  listener_name = gst_inter_pipe_ilistener_get_name (listener);

  GST_INFO_OBJECT (sink, "Adding new listener %s", listener_name);

  /* Check caps before add listener */
  srccaps = gst_inter_pipe_ilistener_get_caps (listener, &src_negotiated);
  sinkcaps = gst_app_sink_get_caps (GST_APP_SINK (sink));

  if (src_negotiated) {
    gboolean has_listeners;

    if (!sinkcaps)
      goto add_to_list;

    has_listeners = 0 != g_hash_table_size (listeners);

    if (!sink->caps_negotiated && !has_listeners
        && !gst_caps_is_equal (srccaps, sinkcaps)) {

      if (!gst_pad_push_event (GST_INTER_PIPE_SINK_PAD (sink),
              gst_event_new_reconfigure ()))
        goto reconfigure_event_error;

      GST_INFO_OBJECT (sink, "Reconfigure event sent correctly");
    }

    if (sink->caps_negotiated && has_listeners
        && !gst_caps_is_equal (srccaps, sinkcaps)) {

      if (!gst_inter_pipe_sink_are_caps_compatible (sink, srccaps, sinkcaps))
        goto renegotiate_error;

      if (!gst_inter_pipe_ilistener_set_caps (listener, sinkcaps))
        goto set_caps_failed;
    }
  } else {
    /* If src has no caps, set it to caps from sink pad */
    GstEvent *capsev = gst_pad_get_sticky_event (GST_INTER_PIPE_SINK_PAD (sink),
        GST_EVENT_CAPS, 0);
    if (capsev) {
      GstCaps *caps;
      gst_event_parse_caps (capsev, &caps);
      GST_INFO_OBJECT (sink, "Setting listener caps to %" GST_PTR_FORMAT, caps);
      gst_inter_pipe_ilistener_set_caps (listener, caps);
      gst_event_unref (capsev);
    } else {
      GST_INFO_OBJECT (sink,
          "Cannot set caps, no caps event stuck on sink pad");
    }
  }

  if (srccaps)
    gst_caps_unref (srccaps);
  if (sinkcaps)
    gst_caps_unref (sinkcaps);

add_to_list:
  g_mutex_lock (&sink->listeners_mutex);
  if (g_hash_table_contains (listeners, listener_name))
    goto already_registered;

  g_hash_table_insert (listeners, (gpointer) listener_name,
      (gpointer) listener);

  g_mutex_unlock (&sink->listeners_mutex);

  return TRUE;

/* Errors */
renegotiate_error:
  {
    GST_ERROR_OBJECT (sink, "Can not connect listener, caps do not intersect");
    goto error;
  }
reconfigure_event_error:
  {
    GST_ERROR_OBJECT (sink, "Failed to reconfigure");
    goto error;
  }
set_caps_failed:
  {
    GST_ERROR_OBJECT (sink, "Failed to set caps to listener %s", listener_name);
    goto error;

  }
already_registered:
  {
    GST_WARNING_OBJECT (sink, "Listener %s already registered in node %s",
        listener_name, GST_OBJECT_NAME (sink));
    g_mutex_unlock (&sink->listeners_mutex);

    return TRUE;
  }
error:
  {
    if (srccaps)
      gst_caps_unref (srccaps);
    if (sinkcaps)
      gst_caps_unref (sinkcaps);
    return FALSE;
  }
}

static gboolean
gst_inter_pipe_sink_remove_listener (GstInterPipeINode * iface,
    GstInterPipeIListener * listener)
{
  GstInterPipeSink *sink;
  GHashTable *listeners;
  const gchar *listener_name;

  sink = GST_INTER_PIPE_SINK (iface);
  g_mutex_lock (&sink->listeners_mutex);

  listeners = GST_INTER_PIPE_SINK_LISTENERS (sink);
  listener_name = gst_inter_pipe_ilistener_get_name (listener);

  GST_INFO_OBJECT (sink, "Removing listener %s", listener_name);

  if (!g_hash_table_remove (listeners, listener_name))
    goto not_registered;

  if (0 == g_hash_table_size (listeners) && sink->caps_negotiated) {
    gst_caps_unref (sink->caps_negotiated);
    sink->caps_negotiated = NULL;
  }
  g_mutex_unlock (&sink->listeners_mutex);

  return TRUE;

not_registered:
  {
    GST_ERROR_OBJECT (sink, "Listener %s is not registered in node %s",
        listener_name, GST_OBJECT_NAME (sink));
    g_mutex_unlock (&sink->listeners_mutex);
    return FALSE;
  }
}

static gboolean
gst_inter_pipe_sink_receive_event (GstInterPipeINode * iface, GstEvent * event)
{
  GstInterPipeSink *self;
  GHashTable *listeners;
  GstPad *sinkpad;

  self = GST_INTER_PIPE_SINK (iface);
  listeners = GST_INTER_PIPE_SINK_LISTENERS (self);

  if (g_hash_table_size (listeners) != 1) {
    gst_event_unref (event);
    goto multiple_listeners;
  }

  sinkpad = GST_INTER_PIPE_SINK_PAD (self);
  return gst_pad_push_event (sinkpad, event);

multiple_listeners:
  {
    GST_WARNING_OBJECT (self, "Could not send event upstream, "
        "more than one listener is connected");
    return FALSE;
  }
}
