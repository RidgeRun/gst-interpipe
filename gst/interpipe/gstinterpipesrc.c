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
 * SECTION:gstinterpipesrc
 * @see_also: #GstInterPipeSink
 *
 * Source element for interpipeline communication
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
#include "gstinterpipe.h"
#include "gstinterpipesrc.h"
#include "gstinterpipeilistener.h"

GST_DEBUG_CATEGORY_STATIC (gst_inter_pipe_src_debug);
#define GST_CAT_DEFAULT gst_inter_pipe_src_debug

#define GST_INTER_PIPE_SRC_PAD(obj)  (GST_BASE_SRC_CAST (obj)->srcpad)

enum
{
  PROP_0,
  PROP_LISTEN_TO,
  PROP_BLOCK_SWITCH,
  PROP_ALLOW_RENEGOTIATION,
  PROP_STREAM_SYNC,
  PROP_ACCEPT_EVENTS,
  PROP_ACCEPT_EOS_EVENT
};

static void gst_inter_pipe_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);
static void gst_inter_pipe_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);
static void gst_inter_pipe_src_finalize (GObject * object);
static GstFlowReturn gst_inter_pipe_src_create (GstBaseSrc * base,
    guint64 offset, guint size, GstBuffer ** buf);


static const gchar *gst_inter_pipe_src_get_name (GstInterPipeIListener *
    listener);
static GstCaps *gst_inter_pipe_src_get_caps (GstInterPipeIListener * listener,
    gboolean * negotiated);
static gboolean gst_inter_pipe_src_set_caps (GstInterPipeIListener * listener,
    const GstCaps * caps);
static gboolean gst_inter_pipe_src_node_added (GstInterPipeIListener * listener,
    const gchar * node_name);
static gboolean gst_inter_pipe_src_node_removed (GstInterPipeIListener *
    listener, const gchar * node_name);
static gboolean gst_inter_pipe_src_push_buffer (GstInterPipeIListener * iface,
    GstBuffer * buffer, guint64 basetime);
static gboolean gst_inter_pipe_src_push_event (GstInterPipeIListener * iface,
    GstEvent * event, guint64 basetime);
static gboolean gst_inter_pipe_src_send_eos (GstInterPipeIListener * iface);
static gboolean gst_inter_pipe_src_listen_node (GstInterPipeSrc * src,
    const gchar * node_name);
static gboolean gst_inter_pipe_src_start (GstBaseSrc * base);
static gboolean gst_inter_pipe_src_stop (GstBaseSrc * base);
static gboolean gst_inter_pipe_src_event (GstBaseSrc * base, GstEvent * event);
static void gst_inter_pipe_ilistener_init (GstInterPipeIListenerInterface *
    iface);


typedef enum
{
  GST_INTER_PIPE_SRC_RESTART_TIMESTAMP,
  GST_INTER_PIPE_SRC_PASSTHROUGH_TIMESTAMP,
  GST_INTER_PIPE_SRC_COMPENSATE_TIMESTAMP
} GstInterPipeSrcStreamSync;


#define GST_TYPE_INTER_PIPE_SRC_STREAM_SYNC (gst_inter_pipe_src_stream_sync_get_type ())
static GType
gst_inter_pipe_src_stream_sync_get_type (void)
{
  static GType inter_pipe_src_stream_sync_type = 0;
  static const GEnumValue stream_sync_types[] = {
    {GST_INTER_PIPE_SRC_RESTART_TIMESTAMP, "Restart Timestamp", "restart-ts"},
    {GST_INTER_PIPE_SRC_PASSTHROUGH_TIMESTAMP, "Passthrough Timestamp",
        "passthrough-ts"},
    {GST_INTER_PIPE_SRC_COMPENSATE_TIMESTAMP, "Compensate Timestamp",
        "compensate-ts"},
    {0, NULL, NULL}
  };
  if (!inter_pipe_src_stream_sync_type) {
    inter_pipe_src_stream_sync_type =
        g_enum_register_static ("GstInterPipeSrcStreamSync", stream_sync_types);
  }
  return inter_pipe_src_stream_sync_type;
}

struct _GstInterPipeSrc
{
  GstAppSrc parent;

  /* Name of the node to listen to */
  gchar *listen_to;

  /* Currently started and listening */
  gboolean listening;

  /* Pending serial events queue */
  GQueue *pending_serial_events;

  /* Block switch */
  gboolean block_switch;

  /* Flag that allows initial negotiation */
  gboolean first_switch;

  /* Allow caps renegotiation */
  gboolean allow_renegotiation;

  /* Stream synchronization */
  GstInterPipeSrcStreamSync stream_sync;

  /* Accept the events received from the interpipesink */
  gboolean accept_events;

  /* Accept end of stream event */
  gboolean accept_eos_event;
};

struct _GstInterPipeSrcClass
{
  GstAppSrcClass parent_class;
};

G_DEFINE_TYPE_WITH_CODE (GstInterPipeSrc, gst_inter_pipe_src, GST_TYPE_APP_SRC,
    G_IMPLEMENT_INTERFACE (GST_INTER_PIPE_TYPE_ILISTENER,
        gst_inter_pipe_ilistener_init));

static void
gst_inter_pipe_src_class_init (GstInterPipeSrcClass * klass)
{
  GObjectClass *gobject_class;
  GstElementClass *element_class;
  GstBaseSrcClass *basesrc_class;

  gobject_class = G_OBJECT_CLASS (klass);
  element_class = GST_ELEMENT_CLASS (klass);
  basesrc_class = GST_BASE_SRC_CLASS (klass);

  GST_DEBUG_CATEGORY_INIT (gst_inter_pipe_src_debug, "interpipesrc",
      0, "interpipeline source");

  gst_element_class_set_static_metadata (element_class,
      "Inter pipeline source",
      "Generic/Source",
      "Source for internal pipeline buffers communication",
      "Michael Grüner <michael.gruner@ridgerun.com>");

  gobject_class->set_property = gst_inter_pipe_src_set_property;
  gobject_class->get_property = gst_inter_pipe_src_get_property;
  gobject_class->finalize = gst_inter_pipe_src_finalize;

  g_object_class_install_property (gobject_class, PROP_LISTEN_TO,
      g_param_spec_string ("listen-to", "Listen To",
          "The name of the node to listen to.",
          NULL, G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_BLOCK_SWITCH,
      g_param_spec_boolean ("block-switch", "Block Switch",
          "Disable the ability to swich between nodes.",
          FALSE, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_ALLOW_RENEGOTIATION,
      g_param_spec_boolean ("allow-renegotiation", "Allow Renegotiation",
          "Allow the caps renegotiation with an interpipesink with different "
          "caps only if the allow-renegotiation property is set to true",
          TRUE, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_STREAM_SYNC,
      g_param_spec_enum ("stream-sync", "Stream Synchronization",
          "Define buffer synchronization between the different pipelines",
          GST_TYPE_INTER_PIPE_SRC_STREAM_SYNC,
          GST_INTER_PIPE_SRC_PASSTHROUGH_TIMESTAMP,
          G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_ACCEPT_EVENTS,
      g_param_spec_boolean ("accept-events", "Accept Events",
          "Accept the events received from the interpipesink",
          TRUE, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_property (gobject_class, PROP_ACCEPT_EOS_EVENT,
      g_param_spec_boolean ("accept-eos-event", "Accept EOS Events",
          "Accept the EOS event received from the interpipesink only if it "
          "is set to true", TRUE, G_PARAM_WRITABLE | G_PARAM_STATIC_STRINGS));

  basesrc_class->start = GST_DEBUG_FUNCPTR (gst_inter_pipe_src_start);
  basesrc_class->stop = GST_DEBUG_FUNCPTR (gst_inter_pipe_src_stop);
  basesrc_class->event = GST_DEBUG_FUNCPTR (gst_inter_pipe_src_event);
  basesrc_class->create = GST_DEBUG_FUNCPTR (gst_inter_pipe_src_create);
}

static void
gst_inter_pipe_src_init (GstInterPipeSrc * src)
{
  gst_app_src_set_emit_signals (GST_APP_SRC (src), FALSE);

  src->listen_to = NULL;
  src->listening = FALSE;
  src->pending_serial_events = g_queue_new ();
  src->block_switch = FALSE;
  src->allow_renegotiation = TRUE;
  src->first_switch = TRUE;
  src->stream_sync = GST_INTER_PIPE_SRC_PASSTHROUGH_TIMESTAMP;
  src->accept_events = TRUE;
  src->accept_eos_event = TRUE;
}

static void
gst_inter_pipe_src_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstInterPipeSrc *src;
  GstInterPipeIListener *listener;
  gchar *node_name;

  g_return_if_fail (GST_IS_INTER_PIPE_SRC (object));

  src = GST_INTER_PIPE_SRC (object);
  listener = GST_INTER_PIPE_ILISTENER (src);

  switch (prop_id) {
    case PROP_LISTEN_TO:
      node_name = g_strdup (g_value_get_string (value));
      if (!g_strcmp0 (src->listen_to, node_name)) {
        /* We are already listening to that node, so nothing to do */
        GST_INFO ("Already listening to node %s", node_name);
        g_free (node_name);
      } else if (node_name != NULL) {
        if (GST_BASE_SRC_IS_STARTED (GST_BASE_SRC (src))) {
          /* valid node_name, BaseSrc started */
          if (!gst_inter_pipe_src_listen_node (src, node_name)) {
            GST_ERROR_OBJECT (src, "Could not listen to node %s", node_name);
            g_free (node_name);
          } else {
            if (src->listen_to) {
              g_free (src->listen_to);
            }
            src->listen_to = node_name;
          }
          src->listening = TRUE;
          GST_INFO_OBJECT (src, "Listening to node %s", src->listen_to);
        } else {
          /* valid node_name, not started */
          g_free (src->listen_to);
          src->listen_to = node_name;
        }
      } else {
        if (src->listening) {
          /* NULL node name, currently listening */
          if (!gst_inter_pipe_leave_node (listener))
            GST_WARNING_OBJECT (src, "Unable to remove listener from node %s",
                src->listen_to);
          g_free (src->listen_to);
          src->listen_to = NULL;
          src->listening = FALSE;
        } else {
          /* NULL node name, not listening/started */
          g_free (src->listen_to);
          src->listen_to = NULL;
        }
      }
      break;
    case PROP_BLOCK_SWITCH:
      src->block_switch = g_value_get_boolean (value);
      break;
    case PROP_ALLOW_RENEGOTIATION:
      src->allow_renegotiation = g_value_get_boolean (value);
      break;
    case PROP_STREAM_SYNC:
      src->stream_sync = g_value_get_enum (value);
      break;
    case PROP_ACCEPT_EVENTS:
      src->accept_events = g_value_get_boolean (value);
      break;
    case PROP_ACCEPT_EOS_EVENT:
      src->accept_eos_event = g_value_get_boolean (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_inter_pipe_src_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstInterPipeSrc *src;

  g_return_if_fail (GST_IS_INTER_PIPE_SRC (object));

  src = GST_INTER_PIPE_SRC (object);

  switch (prop_id) {
    case PROP_LISTEN_TO:
      g_value_set_string (value, src->listen_to);
      break;
    case PROP_BLOCK_SWITCH:
      g_value_set_boolean (value, src->block_switch);
      break;
    case PROP_ALLOW_RENEGOTIATION:
      g_value_set_boolean (value, src->allow_renegotiation);
      break;
    case PROP_STREAM_SYNC:
      g_value_set_enum (value, src->stream_sync);
      break;
    case PROP_ACCEPT_EVENTS:
      g_value_set_boolean (value, src->accept_events);
      break;
    case PROP_ACCEPT_EOS_EVENT:
      g_value_set_boolean (value, src->accept_eos_event);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_inter_pipe_src_finalize (GObject * object)
{
  GstInterPipeSrc *src;

  src = GST_INTER_PIPE_SRC (object);

  /* Free pending serial events queue */
  g_queue_free_full (src->pending_serial_events,
      (GDestroyNotify) gst_event_unref);

  if (src->listen_to) {
    g_free (src->listen_to);
    src->listen_to = NULL;
  }

  /* Chain up to the parent class */
  G_OBJECT_CLASS (gst_inter_pipe_src_parent_class)->finalize (object);
}

/* GstBaseSrc Implementation*/
static gboolean
gst_inter_pipe_src_start (GstBaseSrc * base)
{
  GstBaseSrcClass *basesrc_class;
  GstInterPipeSrc *src;

  basesrc_class = GST_BASE_SRC_CLASS (gst_inter_pipe_src_parent_class);
  src = GST_INTER_PIPE_SRC (base);

  if (!basesrc_class->start (base))
    goto start_fail;

  if (src->listen_to) {
    if (!gst_inter_pipe_src_listen_node (src, src->listen_to)) {
      GST_ERROR_OBJECT (src, "Could not listen to node %s", src->listen_to);
      goto start_fail;
    } else {
      GST_INFO_OBJECT (src, "Listening to node %s", src->listen_to);
      src->listening = TRUE;
      goto start_done;
    }
  } else {
    /* it's valid to be started but not listening (yet) */
    goto start_done;
  }

start_done:
  if (GST_INTER_PIPE_SRC_RESTART_TIMESTAMP == src->stream_sync)
    gst_base_src_set_do_timestamp (base, TRUE);

  return TRUE;
start_fail:
  return FALSE;
}

static gboolean
gst_inter_pipe_src_stop (GstBaseSrc * base)
{
  GstBaseSrcClass *basesrc_class;
  GstInterPipeSrc *src;
  GstInterPipeIListener *listener;

  basesrc_class = GST_BASE_SRC_CLASS (gst_inter_pipe_src_parent_class);
  src = GST_INTER_PIPE_SRC (base);
  listener = GST_INTER_PIPE_ILISTENER (src);

  if (src->listening) {
    GST_INFO_OBJECT (src, "Removing listener from node %s", src->listen_to);
    gst_inter_pipe_leave_node (listener);
    src->listening = FALSE;
    g_free (src->listen_to);
    src->listen_to = NULL;
  }

  return basesrc_class->stop (base);
}

static gboolean
gst_inter_pipe_src_event (GstBaseSrc * base, GstEvent * event)
{
  GstBaseSrcClass *basesrc_class;
  GstInterPipeSrc *src;
  GstInterPipeINode *node;

  basesrc_class = GST_BASE_SRC_CLASS (gst_inter_pipe_src_parent_class);
  src = GST_INTER_PIPE_SRC (base);
  node = gst_inter_pipe_get_node (src->listen_to);

  if (GST_EVENT_IS_UPSTREAM (event)) {

    GST_INFO_OBJECT (src, "Incoming upstream event %s",
        GST_EVENT_TYPE_NAME (event));

    if (node) {
      gst_inter_pipe_inode_receive_event (node, gst_event_ref (event));
    } else
      GST_WARNING_OBJECT (src, "Node doesn't exist, event won't be forwarded");
  }

  return basesrc_class->event (base, event);
}

static GstFlowReturn
gst_inter_pipe_src_create (GstBaseSrc * base, guint64 offset, guint size,
    GstBuffer ** buf)
{
  GstInterPipeSrc *src;
  GstEvent *serial_event;
  GstPad *srcpad;
  GstFlowReturn ret;

  src = GST_INTER_PIPE_SRC (base);
  srcpad = GST_INTER_PIPE_SRC_PAD (src);

  ret =
      GST_BASE_SRC_CLASS (gst_inter_pipe_src_parent_class)->create (base,
      offset, size, buf);

  if (ret != GST_FLOW_OK) {
    GST_LOG_OBJECT (src, "parent create() returned %s",
        gst_flow_get_name (ret));
    return ret;
  }

  GST_LOG_OBJECT (src,
      "Dequeue buffer %p with timestamp (PTS) %" GST_TIME_FORMAT, *buf,
      GST_TIME_ARGS (GST_BUFFER_PTS (*buf)));

  while (!g_queue_is_empty (src->pending_serial_events)) {
    guint curr_bytes;
    /*Pending Serial Events Queue */
    serial_event = g_queue_peek_head (src->pending_serial_events);

    GST_DEBUG_OBJECT (src,
        "Got event %s with timestamp %" GST_TIME_FORMAT,
        GST_EVENT_TYPE_NAME (serial_event),
        GST_TIME_ARGS (GST_EVENT_TIMESTAMP (serial_event)));

    curr_bytes = gst_app_src_get_current_level_bytes (GST_APP_SRC (src));
    if ((GST_EVENT_TIMESTAMP (serial_event) < GST_BUFFER_PTS (*buf))
        || (curr_bytes == 0)) {

      GST_DEBUG_OBJECT (src, "Sending Serial Event %s",
          GST_EVENT_TYPE_NAME (serial_event));

      serial_event = g_queue_pop_head (src->pending_serial_events);
      gst_pad_push_event (srcpad, serial_event);
    } else {
      GST_DEBUG_OBJECT (src, "Event %s timestamp is greater than the "
          "buffer timestamp, can't send serial event yet",
          GST_EVENT_TYPE_NAME (serial_event));
          break;
    }
  }

  return ret;
}

/* GstInterPipeIListener Implementation */
static void
gst_inter_pipe_ilistener_init (GstInterPipeIListenerInterface * iface)
{
  iface->get_name = gst_inter_pipe_src_get_name;
  iface->node_added = gst_inter_pipe_src_node_added;
  iface->node_removed = gst_inter_pipe_src_node_removed;
  iface->get_caps = gst_inter_pipe_src_get_caps;
  iface->set_caps = gst_inter_pipe_src_set_caps;
  iface->push_buffer = gst_inter_pipe_src_push_buffer;
  iface->push_event = gst_inter_pipe_src_push_event;
  iface->send_eos = gst_inter_pipe_src_send_eos;
}

static const gchar *
gst_inter_pipe_src_get_name (GstInterPipeIListener * iface)
{
  return GST_OBJECT_NAME (iface);
}

static gboolean
gst_inter_pipe_src_node_added (GstInterPipeIListener * iface,
    const gchar * node_name)
{
  GstInterPipeSrc *src;

  src = GST_INTER_PIPE_SRC (iface);

  GST_INFO_OBJECT (src, "Node %s registered. Listening.", node_name);

  if (g_strcmp0 (src->listen_to, node_name) == 0) {
    gst_inter_pipe_src_listen_node (src, node_name);
  }

  return TRUE;
}

static gboolean
gst_inter_pipe_src_node_removed (GstInterPipeIListener * iface,
    const gchar * node_name)
{
  GstInterPipeSrc *src;

  src = GST_INTER_PIPE_SRC (iface);

  GST_INFO_OBJECT (src, "Node %s removed. Leaving.", node_name);
  if (g_strcmp0 (src->listen_to, node_name) == 0) {
    gst_inter_pipe_leave_node (iface);
  }

  return TRUE;
}

static GstCaps *
gst_inter_pipe_src_get_caps (GstInterPipeIListener * iface,
    gboolean * negotiated)
{
  GstInterPipeSrc *src;
  GstAppSrc *appsrc;
  GstCaps *appcaps;

  src = GST_INTER_PIPE_SRC (iface);
  appsrc = GST_APP_SRC (src);
  *negotiated = FALSE;

  appcaps = gst_app_src_get_caps (appsrc);
  if (appcaps) {
    *negotiated = TRUE;
    if (!src->allow_renegotiation)
      goto out;
    gst_caps_unref (appcaps);
  }

  appcaps = gst_pad_peer_query_caps (GST_INTER_PIPE_SRC_PAD (src), NULL);

out:
  return appcaps;
}

static gboolean
gst_inter_pipe_src_set_caps (GstInterPipeIListener * iface,
    const GstCaps * caps)
{
  GstInterPipeSrc *src;
  GstAppSrc *appsrc;
  GstCaps *appcaps;

  src = GST_INTER_PIPE_SRC (iface);
  appsrc = GST_APP_SRC (src);

  appcaps = gst_app_src_get_caps (appsrc);
  if (appcaps && !src->allow_renegotiation)
    goto allow_renegotiation_disabled;

  if (appcaps)
    gst_caps_unref (appcaps);

  gst_app_src_set_caps (appsrc, caps);

  return TRUE;

allow_renegotiation_disabled:
  {
    GST_ERROR_OBJECT (src,
        "Renegotiation not allowed, current caps %" GST_PTR_FORMAT, appcaps);
    gst_caps_unref (appcaps);
    return FALSE;
  }
}

static gboolean
gst_inter_pipe_src_push_buffer (GstInterPipeIListener * iface,
    GstBuffer * buffer, guint64 basetime)
{
  GstInterPipeSrc *src;
  GstAppSrc *appsrc;
  GstFlowReturn ret;
  guint64 srcbasetime;

  src = GST_INTER_PIPE_SRC (iface);
  appsrc = GST_APP_SRC (src);

  GST_LOG_OBJECT (src, "Incoming buffer: %p", buffer);

  if (GST_STATE (GST_ELEMENT (appsrc)) < GST_STATE_PAUSED) {
    gst_buffer_unref (buffer);
    goto out;
  }

  if (GST_INTER_PIPE_SRC_COMPENSATE_TIMESTAMP == src->stream_sync) {
    guint64 difftime;

    buffer = gst_buffer_make_writable (buffer);

    srcbasetime = gst_element_get_base_time (GST_ELEMENT (appsrc));

    GST_LOG_OBJECT (src, "Incoming Buffer timestamp (pts): %" GST_TIME_FORMAT,
        GST_TIME_ARGS (GST_BUFFER_PTS (buffer)));
    GST_LOG_OBJECT (src, "My Base Time: %" GST_TIME_FORMAT,
        GST_TIME_ARGS (srcbasetime));
    GST_LOG_OBJECT (src, "Node Base Time: %" GST_TIME_FORMAT,
        GST_TIME_ARGS (basetime));

    if (GST_STATE (src) == GST_STATE_PLAYING) {
      if (srcbasetime > basetime) {
        difftime = srcbasetime - basetime;
        if (GST_BUFFER_PTS (buffer) >= difftime) {
          GST_BUFFER_PTS (buffer) = GST_BUFFER_PTS (buffer) - difftime;
          GST_BUFFER_DTS (buffer) = GST_BUFFER_DTS (buffer) - difftime;
        } else {
          gst_buffer_unref (buffer);
          goto nosync;
        }
      } else {
        difftime = basetime - srcbasetime;
        GST_BUFFER_PTS (buffer) = GST_BUFFER_PTS (buffer) + difftime;
        GST_BUFFER_DTS (buffer) = GST_BUFFER_DTS (buffer) + difftime;
      }
    } else {
      /* srcbasetime is only valid when PLAYING, no adjustment can be done */
      GST_LOG_OBJECT (src, "Not PLAYING state yet");
      gst_buffer_unref (buffer);
      goto nosync;
    }

    GST_LOG_OBJECT (src,
        "Calculated Buffer Timestamp (PTS): %" GST_TIME_FORMAT,
        GST_TIME_ARGS (GST_BUFFER_PTS (buffer)));
  } else if (GST_INTER_PIPE_SRC_RESTART_TIMESTAMP == src->stream_sync) {
    /* Remove the incoming timestamp to be generated according this basetime */
    buffer = gst_buffer_make_writable (buffer);
    GST_BUFFER_PTS (buffer) = GST_CLOCK_TIME_NONE;
    GST_BUFFER_DTS (buffer) = GST_CLOCK_TIME_NONE;
  }

  ret = gst_app_src_push_buffer (appsrc, buffer);
  if (ret != GST_FLOW_OK)
    return FALSE;
out:
  return TRUE;

nosync:
  {
    GST_WARNING_OBJECT (src, "Buffers running time can not be synchronized yet"
        " with the interpipesrc running time");
    return FALSE;
  }

}

static gboolean
gst_inter_pipe_src_push_event (GstInterPipeIListener * iface, GstEvent * event,
    guint64 basetime)
{
  GstInterPipeSrc *src;
  GstAppSrc *appsrc;
  GstPad *srcpad;
  guint64 srcbasetime;
  gboolean ret = TRUE;

  src = GST_INTER_PIPE_SRC (iface);
  appsrc = GST_APP_SRC (src);
  srcpad = GST_INTER_PIPE_SRC_PAD (src);

  if (!src->accept_events)
    goto no_events;

  if (!GST_EVENT_IS_SERIALIZED (event)) {

    GST_INFO_OBJECT (src, "Incoming non-serialized event %s",
        GST_EVENT_TYPE_NAME (event));

    ret = gst_pad_push_event (srcpad, event);
  } else {

    event = gst_event_make_writable (event);
    srcbasetime = gst_element_get_base_time (GST_ELEMENT (appsrc));

    if (srcbasetime > basetime) {
      GST_EVENT_TIMESTAMP (event) =
          GST_EVENT_TIMESTAMP (event) - (srcbasetime - basetime);
    } else {
      GST_EVENT_TIMESTAMP (event) =
          GST_EVENT_TIMESTAMP (event) + (basetime - srcbasetime);
    }

    GST_DEBUG_OBJECT (src,
        "Event %s with calculated timestamp %" GST_TIME_FORMAT
        " enqueued on serial pending events", GST_EVENT_TYPE_NAME (event),
        GST_TIME_ARGS (GST_EVENT_TIMESTAMP (event)));

    g_queue_push_tail (src->pending_serial_events, event);
  }
  return ret;
no_events:
  {
    GST_DEBUG_OBJECT (src,
        "The interpipesrc is not currently processing the incoming events "
        "because the accept incoming events property is set to FALSE");
    return TRUE;
  }
}

static gboolean
gst_inter_pipe_src_send_eos (GstInterPipeIListener * iface)
{
  GstInterPipeSrc *src;
  GstAppSrc *appsrc;
  GstFlowReturn ret;

  src = GST_INTER_PIPE_SRC (iface);
  appsrc = GST_APP_SRC (src);

  if (src->accept_eos_event) {
    GST_LOG_OBJECT (src, "Sending EOS event");
    ret = gst_app_src_end_of_stream (appsrc);
    if (ret != GST_FLOW_OK)
      return FALSE;
  }
  return TRUE;
}

static gboolean
gst_inter_pipe_src_listen_node (GstInterPipeSrc * src, const gchar * node_name)
{
  GstInterPipeIListener *listener;

  listener = GST_INTER_PIPE_ILISTENER (src);

  if (!src->first_switch && src->block_switch)
    goto block_switch;

  if (src->first_switch)
    src->first_switch = FALSE;

  if (!gst_inter_pipe_listen_node (listener, node_name)) {
    gst_inter_pipe_leave_node (listener);
    gst_inter_pipe_listen_node (listener, src->listen_to);
    return FALSE;
  } else {
    return TRUE;
  }

block_switch:
  {
    GST_ERROR_OBJECT (src, "Can not connect to the node %s because the "
        "block-switch property is set to true", node_name);
    return FALSE;
  }
}
