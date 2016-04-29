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
#include "gstinterpipesrc.h"
#include "gstinterpipesink.h"

GST_DEBUG_CATEGORY_EXTERN (gst_inter_pipe_debug);

static gboolean
plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_inter_pipe_debug, "interpipe", 0,
      "interpipeline base");

  gst_element_register (plugin, "interpipesrc", GST_RANK_NONE,
      GST_TYPE_INTER_PIPE_SRC);
  gst_element_register (plugin, "interpipesink", GST_RANK_NONE,
      GST_TYPE_INTER_PIPE_SINK);
  return TRUE;
}

GST_PLUGIN_DEFINE (GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    interpipe,
    "Elements to communicate buffers across pipelines",
    plugin_init, VERSION, "Proprietary", PACKAGE_NAME,
    "http://www.ridgerun.com")
