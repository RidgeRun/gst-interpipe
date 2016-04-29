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

#ifndef __GST_INTER_PIPE_INODE_H__
#define __GST_INTER_PIPE_INODE_H__

#include <gst/gst.h>
#include "gstinterpipeilistener.h" 

G_BEGIN_DECLS

#define GST_INTER_PIPE_TYPE_INODE (gst_inter_pipe_inode_get_type())
G_DECLARE_INTERFACE(GstInterPipeINode, gst_inter_pipe_inode, GST_INTER_PIPE, INODE, GObject)

/**
 * GstInterPipeINodeInterface:
 * Interface that potential node should implement to integrate 
 * to the GstInterPipe core.
 *
 * @add_listener: Add the #GstInterPipeIListener given through
 * @listener and store it in an internal list. See
 * #gst_inter_pipe_inode_add_listener.
 *
 * @remove_listener: Remove @listener from the internal list. See
 * #gst_inter_pipe_inode_remove_listener.
 *
 * @receive_event: Receive the upstream #GstEvent passed through
 * @event and forward it upstream. It is responsability of the node to
 * decide if the event can be forwarded or not. See
 * #gst_inter_pipe_inode_receive_event.
 */
struct _GstInterPipeINodeInterface
{
  GTypeInterface parent_iface;

  gboolean (* add_listener) (GstInterPipeINode *iface, GstInterPipeIListener * listener);
  gboolean (* remove_listener) (GstInterPipeINode *iface, GstInterPipeIListener * listener);
  gboolean (* receive_event) (GstInterPipeINode *iface, GstEvent *event);
};

/**
 * gst_inter_pipe_inode_add_listener:
 * @iface: (transfer none)(not nullable): The object implementing the interface.
 * @listener: (transfer none)(not nullable): The listener to be stored inside the node.
 *
 * Store the given @listener in an internal list.
 *
 * Returns: True if the listener was successfully stored, False otherwise.
 */
gboolean gst_inter_pipe_inode_add_listener (GstInterPipeINode *iface, GstInterPipeIListener * listener);

/**
 * gst_inter_pipe_remove_listener:
 * @iface: (transfer none)(not nullable): The object implementing the interface.
 * @listener: (transfer none)(not nullable): The listener to be removed from the internal list.
 *
 * Look for @listener in the internal storage and remove it. 
 *
 * Returns: True if the listener was found and successfully removed, False otherwise.
 */
gboolean gst_inter_pipe_inode_remove_listener (GstInterPipeINode *iface, GstInterPipeIListener * listener);

/**
 * gst_inter_pipe_receive_event:
 * @iface: (transfer none)(not nullable): The object implementing the interface.
 * @event: (transfer full)(not nullable): The #GstEvent to push upstream.
 *
 * Given an upstream #GstEvent in @event, decide if it can be pushed
 * upstream and send it. If multiple listeners are connected to the
 * node, it is recommended that the event is not pushed upstream,
 * since it may affect the node's state for the other
 * listeners. 
 *
 * Returns: True if the node is able to receive the event, False otherwise.
 */
gboolean gst_inter_pipe_inode_receive_event (GstInterPipeINode *iface, GstEvent *event);


G_END_DECLS

#endif //__GST_INTER_PIPE_INODE_H__
