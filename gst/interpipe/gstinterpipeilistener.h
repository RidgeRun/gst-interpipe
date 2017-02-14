/* GStreamer
 * Copyright (C) 2013-2016 Michael Grüner <michael.gruner@ridgerun.com>
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

#ifndef __GST_INTER_PIPE_ILISTENER_H__
#define __GST_INTER_PIPE_ILISTENER_H__

#include <gst/gst.h>

G_BEGIN_DECLS

#define GST_INTER_PIPE_TYPE_ILISTENER (gst_inter_pipe_ilistener_get_type())
#define GST_INTER_PIPE_ILISTENER(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), \
    GST_INTER_PIPE_TYPE_ILISTENER, GstInterPipeIListener))
#define GST_INTER_PIPE_IS_ILISTENER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), \
    GST_INTER_PIPE_TYPE_ILISTENER))
#define GST_INTER_PIPE_ILISTENER_GET_IFACE(inst) (G_TYPE_INSTANCE_GET_INTERFACE ((inst), \
    GST_INTER_PIPE_TYPE_ILISTENER, GstInterPipeIListenerInterface))

typedef struct _GstInterPipeIListener GstInterPipeIListener;    /* dummy object */
typedef struct _GstInterPipeIListenerInterface GstInterPipeIListenerInterface;

/**
 * GstInterPipeIListenerInterface:
 * Interface that potential listeners should implement to integrate
 * to the GstInterPipe core.
 *
 * @get_name: Return the name of the object implementing the
 *   interface.  See #gst_inter_pipe_ilistener_get_name.
 *
 * @get_caps: Return the set of supported caps. The element
 *   implementing the interface should consider it's downstream
 *   capabilities. Additionally, it should output through the
 *   @negotiated variable whether or not caps are already configured in
 *   the element. See #gst_inter_pipe_ilistener_get_caps.
 *
 * @set_caps: Configure the given caps into the element. This caps
 * should be forwarded to downstream elements. See
 * #gst_inter_pipe_ilistener_set_caps.
 *
 * @node_added: Callback function that will be called whenever a new
 * node is connected. It's responsibility of the listener to decide
 * whether or not it is interested in the new node. See
 * #gst_inter_pipe_ilistener_node_added.
 *
 * @node_removed: Callback function that will be called whenever a
 * node is disconnected. It's responsibility of the listener to decide
 * whether or not it is interested in the node removal. See
 * #gst_inter_pipe_ilistener_node_removed.
 *
 * @push_buffer: Push the given buffer into the listener's
 * pipeline. Furthermore, consider @basetime in order to compensate
 * its timestamp if required. See
 * #gst_inter_pipe_ilistener_push_buffer.
 *
 * @push_event: Push the given event into the listener's
 * pipeline. Furthermore, consider @basetime in order to compensate
 * its timestamp if required. See
 * #gst_inter_pipe_ilistener_push_event.
 *
 * @send_eos: Send an EOS event into the listener's pipeline. See
 * #gst_inter_pipe_ilistener_send_eos.
 */
struct _GstInterPipeIListenerInterface
{
  /*<private>*/
  GTypeInterface parent_iface;

  /*<public>*/
  const gchar * (*get_name) (GstInterPipeIListener *iface);
  GstCaps * (*get_caps) (GstInterPipeIListener *iface, gboolean *negotiated);
  gboolean (*set_caps) (GstInterPipeIListener *iface, const GstCaps *caps);
  gboolean (* node_added) (GstInterPipeIListener *iface, const gchar *node_name);
  gboolean (* node_removed) (GstInterPipeIListener *iface, const gchar *node_removed);
  gboolean (* push_buffer) (GstInterPipeIListener *iface, GstBuffer *buffer, guint64 basetime);
  gboolean (* push_event) (GstInterPipeIListener *iface, GstEvent *event, guint64 basetime);
  gboolean (* send_eos) (GstInterPipeIListener *iface);
};

/**
 * gst_inter_pipe_ilistener_get_name:
 * @iface: (transfer none)(not nullable): The object to query the name from
 *
 * Return the name associated with the object implementing the interface.
 *
 * Returns: (transfer none): The name of the object implementing the interface.
 */
const gchar * gst_inter_pipe_ilistener_get_name (GstInterPipeIListener *iface);

/**
 * gst_inter_pipe_ilistener_get_caps:
 * @iface: (transfer none)(not nullable): The object to query the #GstCaps from.
 * @negotiated: (out): Whether or not the listener already has the caps negotiated
 * and configured in the element.
 *
 * Return the set of supported caps of the element implementing the interface.
 * This set of caps should consider the supported caps of the downstream elements.
 * If the caps negotiation has already taken place, this caps should be the ones
 * configured in the element and @negotiated should be set to true.
 *
 * Returns: (transfer full): The #GstCaps supported by the listener's pipeline.
 */
GstCaps * gst_inter_pipe_ilistener_get_caps (GstInterPipeIListener *iface, gboolean *negotiated);

/**
 * gst_inter_pipe_ilistener_set_caps:
 * @iface: (transfer none)(not nullable): The object to set the #GstCaps to.
 * @caps: (transfer none)(not nullable): The #GstCaps to set in the element.
 *
 * Set the given #GstCaps in the listener. This caps should be forwarded to
 * downstream elements.
 *
 * Return: True if it was possible to set the given caps, False otherwise.
 */
gboolean gst_inter_pipe_ilistener_set_caps (GstInterPipeIListener *iface,
    const GstCaps *caps);

/**
 * gst_inter_pipe_ilistener_node_added:
 * @iface: (transfer none)(not nullable): The object to notify when a new #GstInterPipeINode is added.
 * @node_name: (transfer none)(not nullable): The name of the newly connected #GstInterPipeINode.
 *
 * Callback that will be called whenever a new #GstInterPipeINode is added. It is responsibility of the
 * listener to decide whether or not it is interested in the newly added #GstInterPipeINode.
 *
 * Return: True always.
 */
gboolean gst_inter_pipe_ilistener_node_added (GstInterPipeIListener *iface,
    const gchar *node_name);

/**
 * gst_inter_pipe_ilistener_node_removed:
 * @iface: (transfer none)(not nullable): The object to notify when a #GstInterPipeINode is added.
 * @node_name: (transfer none)(not nullable): The name of the #GstInterPipeINode that was removed.
 *
 * Callback that will be called whenever a #GstInterPipeINode is removed. It is responsibility of the
 * listener to decide whether or not it is interested in the event.
 *
 * Return: True always.
 */
gboolean gst_inter_pipe_ilistener_node_removed (GstInterPipeIListener *iface,
    const gchar *node_name);

/**
 * gst_inter_pipe_ilistener_push_buffer:
 * @iface: (transfer none)(not nullable): The object that should push the #GstBuffer downstream.
 * @buffer: (transfer full)(not nullable): The #GstBuffer to be pushed downstream.
 * @basetime: The basetime of the node's pipeline. If required, the listener should
 * compensate the buffer's timestamp using the node's base time and its own, in order
 * to get an equivalent buffer time.
 *
 * Push @buffer to the downstream element. If required compensate the timestamp
 * using @basetime in order to get an equivalent buffer time.
 *
 * Return: True if the buffer was successfully pushed, False otherwise.
 */
gboolean gst_inter_pipe_ilistener_push_buffer (GstInterPipeIListener *iface,
    GstBuffer *buffer, guint64 basetime);

/**
 * gst_inter_pipe_ilistener_push_event:
 * @iface: (transfer none)(not nullable): The object that should push the #GstEvent downstream.
 * @event: (transfer full)(not nullable): The #GstEvent to be pushed downstream.
 * @basetime: The basetime of the node's pipeline. If required, the listener should
 * compensate the event's timestamp using the node's base time and its own, in order
 * to get an equivalent event time.
 *
 * Push @event to the downstream element. If required compensate the timestamp
 * using @basetime in order to get an equivalent event time.
 *
 * Return: True if the event was successfully pushed, False otherwise.
 */
gboolean gst_inter_pipe_ilistener_push_event (GstInterPipeIListener *iface,
					      GstEvent *event, guint64 basetime);

/**
 * gst_inter_pipe_ilistener_send_eos:
 * @iface: (transfer none)(not nullable): The object that should push the #GstEvent downstream.
 *
 * Push an EOS event to the downstream elements.
 *
 * Return: True if the event was successfully pushed, False otherwise.
 */
gboolean gst_inter_pipe_ilistener_send_eos (GstInterPipeIListener *iface);

GType gst_inter_pipe_ilistener_get_type (void);

G_END_DECLS

#endif //__GST_INTER_PIPE_ILISTENER_H__
