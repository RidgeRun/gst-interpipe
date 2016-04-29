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

#ifndef __GST_INTER_PIPE_H__
#define __GST_INTER_PIPE_H__

#include "gstinterpipeilistener.h"
#include "gstinterpipeinode.h"

G_BEGIN_DECLS
/**
 * gst_inter_pipe_get_node:
 * @node_name: (transfer none)(not nullable): the name of the node to return
 *
 * Search for @node_name in the list of existing nodes and return the
 * appropriate GstInterPipeINode. If the node was not found
 * null will be returned accordingly.
 *
 * Returns: (transfer none)(nullable): The respective GstInterPipeINode or null
 */
GstInterPipeINode * gst_inter_pipe_get_node (const gchar * node_name);

/**
 * gst_inter_pipe_listen_node:
 * @listener:(transfer none)(not nullable): The listener object to attach to the node
 * @node_name:(transfer none)(not nullable): The name of the node to attach to
 *
 * Register a listener to listen to an specific node. If the node 
 * is not yet available, the listener will be notified later.
 *
 * Returns: TRUE if the listener was registered correctly, FALSE otherwise. 
 */
gboolean gst_inter_pipe_listen_node (GstInterPipeIListener * listener,
    const gchar * node_name);
/**
 * gst_inter_pipe_leave_node:
 * @listener: (transfer none)(not nullable): The listener to detach
 *
 * Disconnect a listener from its respective node.
 *
 * Returns: TRUE if the listener was detached correclty, FALSE otherwise
 */
gboolean gst_inter_pipe_leave_node (GstInterPipeIListener * listener);

/**
 * gst_inter_pipe_add_node:
 * @node: (transfer full)(not nullable): The node object to register
 *
 * Add a new node to the available nodes list
 *
 * Returns: TRUE if the node was added succesfully, FALSE otherwise. 
 */

gboolean gst_inter_pipe_add_node (GstInterPipeINode * node,
    const gchar * node_name);

/**
 * gst_inter_pipe_remove_node:
 * @node: (transfer none)(not nullable): The node object to be removed. 
 *
 * Remove a node from the nodes list.
 *
 * Returns: TRUE if the node was removed succesfully, FALSE otherwise. 
 */
gboolean gst_inter_pipe_remove_node (GstInterPipeINode * node,
    const gchar * node_name);

G_END_DECLS
#endif // __GST_INTER_PIPE_H__
