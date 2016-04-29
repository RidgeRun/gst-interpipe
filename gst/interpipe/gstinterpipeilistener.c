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

#include "gstinterpipeilistener.h"

/**
 * SECTION:gstinterpipeilistener
 * @see_also: #GstInterPipeINode
 *
 * Interface to decouple specific nodes and listeners from the core.
 */
G_DEFINE_INTERFACE (GstInterPipeIListener, gst_inter_pipe_ilistener,
    G_TYPE_OBJECT);

static void
gst_inter_pipe_ilistener_default_init (GstInterPipeIListenerInterface * iface)
{
  //NOP
}

gboolean
gst_inter_pipe_ilistener_node_added (GstInterPipeIListener * self,
    const gchar * node_name)
{
  GstInterPipeIListenerInterface *iface;

  g_return_val_if_fail (GST_INTER_PIPE_IS_ILISTENER (self), FALSE);
  g_return_val_if_fail (node_name, FALSE);

  iface = GST_INTER_PIPE_ILISTENER_GET_IFACE (self);
  g_return_val_if_fail (iface->node_added != NULL, FALSE);

  return iface->node_added (self, node_name);
}

gboolean
gst_inter_pipe_ilistener_node_removed (GstInterPipeIListener * self,
    const gchar * node_name)
{
  GstInterPipeIListenerInterface *iface;

  g_return_val_if_fail (GST_INTER_PIPE_IS_ILISTENER (self), FALSE);
  g_return_val_if_fail (node_name, FALSE);

  iface = GST_INTER_PIPE_ILISTENER_GET_IFACE (self);
  g_return_val_if_fail (iface->node_removed != NULL, FALSE);

  return iface->node_removed (self, node_name);
}

gboolean
gst_inter_pipe_ilistener_push_buffer (GstInterPipeIListener * self,
    GstBuffer * buffer, guint64 basetime)
{
  GstInterPipeIListenerInterface *iface;

  g_return_val_if_fail (GST_INTER_PIPE_IS_ILISTENER (self), FALSE);
  g_return_val_if_fail (buffer, FALSE);

  iface = GST_INTER_PIPE_ILISTENER_GET_IFACE (self);
  g_return_val_if_fail (iface->push_buffer != NULL, FALSE);

  return iface->push_buffer (self, buffer, basetime);
}

const gchar *
gst_inter_pipe_ilistener_get_name (GstInterPipeIListener * self)
{
  GstInterPipeIListenerInterface *iface;

  g_return_val_if_fail (GST_INTER_PIPE_IS_ILISTENER (self), NULL);

  iface = GST_INTER_PIPE_ILISTENER_GET_IFACE (self);
  g_return_val_if_fail (iface->get_name != NULL, NULL);

  return iface->get_name (self);
}

GstCaps *
gst_inter_pipe_ilistener_get_caps (GstInterPipeIListener * self,
    gboolean * negotiated)
{
  GstInterPipeIListenerInterface *iface;

  g_return_val_if_fail (GST_INTER_PIPE_IS_ILISTENER (self), NULL);

  iface = GST_INTER_PIPE_ILISTENER_GET_IFACE (self);
  g_return_val_if_fail (iface->get_caps != NULL, NULL);

  return iface->get_caps (self, negotiated);
}


gboolean
gst_inter_pipe_ilistener_set_caps (GstInterPipeIListener * self,
    const GstCaps * caps)
{
  GstInterPipeIListenerInterface *iface;

  g_return_val_if_fail (GST_INTER_PIPE_IS_ILISTENER (self), FALSE);

  iface = GST_INTER_PIPE_ILISTENER_GET_IFACE (self);
  g_return_val_if_fail (iface->set_caps != NULL, FALSE);

  return iface->set_caps (self, caps);
}


gboolean
gst_inter_pipe_ilistener_push_event (GstInterPipeIListener * self,
    GstEvent * event, guint64 basetime)
{
  GstInterPipeIListenerInterface *iface;

  g_return_val_if_fail (GST_INTER_PIPE_IS_ILISTENER (self), FALSE);
  g_return_val_if_fail (event, FALSE);

  iface = GST_INTER_PIPE_ILISTENER_GET_IFACE (self);
  g_return_val_if_fail (iface->push_event != NULL, FALSE);

  return iface->push_event (self, event, basetime);
}

gboolean
gst_inter_pipe_ilistener_send_eos (GstInterPipeIListener * self)
{
  GstInterPipeIListenerInterface *iface;

  g_return_val_if_fail (GST_INTER_PIPE_IS_ILISTENER (self), FALSE);

  iface = GST_INTER_PIPE_ILISTENER_GET_IFACE (self);
  g_return_val_if_fail (iface->push_event != NULL, FALSE);

  return iface->send_eos (self);
}
