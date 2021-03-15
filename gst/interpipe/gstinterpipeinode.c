
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

#include "gstinterpipeinode.h"

/**
 * SECTION:gstinterpipeinode
 * @see_also: #GstInterPipeIListener
 *
 * Interface to decouple specific nodes and listeners from the core.
 */
G_DEFINE_INTERFACE (GstInterPipeINode, gst_inter_pipe_inode, G_TYPE_OBJECT);

static void
gst_inter_pipe_inode_default_init (GstInterPipeINodeInterface * iface)
{
  //NOP
}

gboolean
gst_inter_pipe_inode_add_listener (GstInterPipeINode * self,
    GstInterPipeIListener * listener)
{
  GstInterPipeINodeInterface *iface;

  g_return_val_if_fail (GST_INTER_PIPE_IS_INODE (self), FALSE);

  iface = GST_INTER_PIPE_INODE_GET_IFACE (self);
  g_return_val_if_fail (iface->add_listener != NULL, FALSE);

  return iface->add_listener (self, listener);
}

gboolean
gst_inter_pipe_inode_remove_listener (GstInterPipeINode * self,
    GstInterPipeIListener * listener)
{
  GstInterPipeINodeInterface *iface;

  g_return_val_if_fail (GST_INTER_PIPE_IS_INODE (self), FALSE);

  iface = GST_INTER_PIPE_INODE_GET_IFACE (self);
  g_return_val_if_fail (iface->remove_listener != NULL, FALSE);

  return iface->remove_listener (self, listener);
}

gboolean
gst_inter_pipe_inode_receive_event (GstInterPipeINode * self, GstInterPipeIListener * listener, GstEvent * event)
{
  GstInterPipeINodeInterface *iface;

  g_return_val_if_fail (GST_INTER_PIPE_IS_INODE (self), FALSE);
  g_return_val_if_fail (listener != NULL, FALSE);

  iface = GST_INTER_PIPE_INODE_GET_IFACE (self);
  g_return_val_if_fail (iface->receive_event != NULL, FALSE);

  return iface->receive_event (self, listener, event);
}
