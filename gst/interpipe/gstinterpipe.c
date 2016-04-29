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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gst/gst.h>
#include <string.h>

#include "gstinterpipe.h"

/**
 * SECTION:gstinterpipe
 *
 * GstInterpipe Core handling inter pipeline communication. 
 */

GST_DEBUG_CATEGORY (gst_inter_pipe_debug);
#define GST_CAT_DEFAULT gst_inter_pipe_debug

typedef struct _GstInterPipeListenerPriv GstInterPipeListenerPriv;
struct _GstInterPipeListenerPriv
{
  GstInterPipeIListener *listener;
  const gchar *listen_to;
};

/* Global mutexes for singletons */
static GMutex listeners_mutex;
static GMutex nodes_mutex;

static GHashTable *gst_inter_pipe_get_listeners ();
static GHashTable *gst_inter_pipe_get_nodes ();
static void gst_inter_pipe_notify_node_added (gpointer listener_name,
    gpointer _listener, gpointer data);
static void gst_inter_pipe_notify_node_removed (gpointer _listener_name,
    gpointer _listener, gpointer data);
static gboolean gst_inter_pipe_leave_listeners_table (GstInterPipeIListener *
    listener);
static gboolean gst_inter_pipe_leave_node_priv (GstInterPipeIListener *
    listener);

static GHashTable *
gst_inter_pipe_get_listeners (void)
{
  /* The listeners singleton */
  static GHashTable *gst_inter_pipe_listeners = NULL;

  if (!gst_inter_pipe_listeners) {
    gst_inter_pipe_listeners = g_hash_table_new (g_str_hash, g_str_equal);
  }
  return gst_inter_pipe_listeners;
}

static GHashTable *
gst_inter_pipe_get_nodes (void)
{
  /* The nodes singleton */
  static GHashTable *gst_inter_pipe_nodes = NULL;

  if (!gst_inter_pipe_nodes) {
    gst_inter_pipe_nodes = g_hash_table_new (g_str_hash, g_str_equal);
  }
  return gst_inter_pipe_nodes;
}

GstInterPipeINode *
gst_inter_pipe_get_node (const gchar * node_name)
{
  GHashTable *nodes;
  GstInterPipeINode *value;

  g_return_val_if_fail (node_name != NULL, NULL);

  g_mutex_lock (&nodes_mutex);
  nodes = gst_inter_pipe_get_nodes ();

  value = (GstInterPipeINode *) g_hash_table_lookup (nodes, node_name);
  g_mutex_unlock (&nodes_mutex);

  return value;
}

gboolean
gst_inter_pipe_listen_node (GstInterPipeIListener * listener,
    const gchar * node_name)
{
  GstInterPipeINode *node;
  GstInterPipeListenerPriv *listener_priv;
  GHashTable *listeners;
  const gchar *listener_name;

  g_return_val_if_fail (listener != NULL, FALSE);
  g_return_val_if_fail (node_name != NULL, FALSE);

  g_mutex_lock (&listeners_mutex);

  listeners = gst_inter_pipe_get_listeners ();
  listener_name = gst_inter_pipe_ilistener_get_name (listener);

  GST_INFO ("listener %s listen to node %s", listener_name, node_name);
  if (g_hash_table_contains (listeners, listener_name)) {
    /*TODO: check if listener is the same listener from the list? */
    listener_priv =
        (GstInterPipeListenerPriv *) g_hash_table_lookup (listeners,
        listener_name);
    if (!g_strcmp0 (listener_priv->listen_to, node_name))
      goto already_listen;

    if (listener_priv->listen_to)
      gst_inter_pipe_leave_node_priv (listener);

  } else {
    listener_priv = g_malloc (sizeof (GstInterPipeListenerPriv));
    listener_priv->listener = listener;
  }

  GST_INFO ("Adding new listener %s to node %s", listener_name, node_name);

  node = gst_inter_pipe_get_node (node_name);

  /* If the node is not in the list we will notify later
     when it connects */
  if (node == NULL) {
    GST_INFO ("Node is not available yet, connecting later.");
    listener_priv->listen_to = NULL;
  } else {
    if (!gst_inter_pipe_inode_add_listener (node, listener))
      goto add_failed;
    listener_priv->listen_to = node_name;
  }

  g_hash_table_insert (listeners, (gchar *) listener_name,
      (gpointer) listener_priv);

  g_mutex_unlock (&listeners_mutex);

  return TRUE;
already_listen:
  {
    GST_INFO ("Already listening to node %s", node_name);
    g_mutex_unlock (&listeners_mutex);
    return TRUE;
  }
add_failed:
  {
    GST_WARNING ("Could not add listener %s to node %s", listener_name,
        node_name);
    g_mutex_unlock (&listeners_mutex);
    return FALSE;
  }
}

static gboolean
gst_inter_pipe_leave_listeners_table (GstInterPipeIListener * listener)
{
  GstInterPipeListenerPriv *listener_priv;
  GHashTable *listeners;
  const gchar *listener_name;

  listeners = gst_inter_pipe_get_listeners ();
  listener_name = gst_inter_pipe_ilistener_get_name (listener);

  listener_priv = g_hash_table_lookup (listeners, listener_name);
  if (!g_hash_table_remove (listeners, listener_name))
    return FALSE;

  g_free (listener_priv);

  return TRUE;
}

static gboolean
gst_inter_pipe_leave_node_priv (GstInterPipeIListener * listener)
{
  GHashTable *listeners;
  GstInterPipeINode *node;
  GstInterPipeListenerPriv *listener_priv;
  const gchar *listener_name;

  g_return_val_if_fail (listener != NULL, FALSE);

  listeners = gst_inter_pipe_get_listeners ();
  listener_name = gst_inter_pipe_ilistener_get_name (listener);

  listener_priv =
      (GstInterPipeListenerPriv *) g_hash_table_lookup (listeners,
      listener_name);
  if (!listener_priv)
    goto no_listener;

  if (listener_priv->listen_to) {
    GST_INFO ("listener %s leaving node %s", listener_name,
        listener_priv->listen_to);

    node = gst_inter_pipe_get_node (listener_priv->listen_to);
    if (node == NULL)
      goto no_node;

    if (!gst_inter_pipe_inode_remove_listener (node, listener))
      goto remove_error;

  }

  return TRUE;

no_listener:
  {
    GST_WARNING ("Listener is not in the connected listeners list");
    return FALSE;
  }
no_node:
  {
    GST_WARNING ("Node %s not found. Could not leave node.",
        listener_priv->listen_to);
    listener_priv->listen_to = NULL;
    return FALSE;
  }
remove_error:
  {
    GST_WARNING
        ("The listener %s was not listening to %s, there's something very wrong",
        listener_name, listener_priv->listen_to);
    listener_priv->listen_to = NULL;
    return FALSE;
  }

}

gboolean
gst_inter_pipe_leave_node (GstInterPipeIListener * listener)
{
  gboolean ret = TRUE;

  g_mutex_lock (&listeners_mutex);

  ret = gst_inter_pipe_leave_node_priv (listener);
  if (!ret)
    goto out;

  if (!gst_inter_pipe_leave_listeners_table (listener))
    goto list_error;

out:
  g_mutex_unlock (&listeners_mutex);
  return ret;

list_error:
  {
    GST_WARNING ("Could not leave node");
    g_mutex_unlock (&listeners_mutex);
    return FALSE;
  }
}

static void
gst_inter_pipe_notify_node_added (gpointer listener_name, gpointer _listener,
    gpointer data)
{
  GstInterPipeListenerPriv *listener_priv = _listener;
  GstInterPipeIListener *listener = listener_priv->listener;
  gchar *node_name = data;

  GST_INFO ("Notifying new node added: %s", node_name);

  gst_inter_pipe_ilistener_node_added (listener, node_name);
}

gboolean
gst_inter_pipe_add_node (GstInterPipeINode * node, const gchar * node_name)
{
  GHashTable *nodes;
  GHashTable *listeners;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (node_name != NULL, FALSE);

  g_mutex_lock (&nodes_mutex);

  nodes = gst_inter_pipe_get_nodes ();
  if (g_hash_table_contains (nodes, node_name))
    goto no_unique;

  GST_INFO ("Adding node %s", node_name);

  if (!g_hash_table_insert (nodes, (gchar *) node_name, (gpointer) node))
    goto add_error;

  g_mutex_unlock (&nodes_mutex);

  listeners = gst_inter_pipe_get_listeners ();
  g_hash_table_foreach (listeners, gst_inter_pipe_notify_node_added,
      (gpointer) node_name);



  return TRUE;

no_unique:
  {
    GST_WARNING ("Could not add node %s, it is not unique.", node_name);
    g_mutex_unlock (&nodes_mutex);
    return FALSE;
  }
add_error:
  {
    GST_INFO ("Could not add node %s", node_name);
    g_mutex_unlock (&nodes_mutex);
    return FALSE;
  }
}

static void
gst_inter_pipe_notify_node_removed (gpointer _listener_name, gpointer _listener,
    gpointer data)
{
  gchar *node_name = data;
  GstInterPipeListenerPriv *listener_priv = _listener;
  GstInterPipeIListener *listener = listener_priv->listener;

  GST_INFO ("Notifying node removed: %s", node_name);

  gst_inter_pipe_ilistener_node_removed (listener, node_name);
}

gboolean
gst_inter_pipe_remove_node (GstInterPipeINode * node, const gchar * node_name)
{
  GHashTable *nodes;
  GHashTable *listeners;

  g_return_val_if_fail (node != NULL, FALSE);
  g_return_val_if_fail (node_name != NULL, FALSE);

  g_mutex_lock (&nodes_mutex);

  nodes = gst_inter_pipe_get_nodes ();
  GST_INFO ("Removing node %s", node_name);
  if (!g_hash_table_remove (nodes, (gconstpointer) node_name)) {
    GST_WARNING ("Node %s not found. Could not remove it.", node_name);
    g_mutex_unlock (&nodes_mutex);
    return FALSE;
  }
  g_mutex_unlock (&nodes_mutex);

  listeners = gst_inter_pipe_get_listeners ();
  g_hash_table_foreach (listeners, gst_inter_pipe_notify_node_removed,
      (gpointer) node_name);

  return TRUE;
}
