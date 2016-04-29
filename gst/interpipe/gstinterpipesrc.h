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

#ifndef __GST_INTER_PIPE_SRC_H__
#define __GST_INTER_PIPE_SRC_H__

#include <gst/gst.h>
#include <gst/app/gstappsrc.h>
#include <string.h>
#include "gstinterpipe.h"

G_BEGIN_DECLS
#define GST_TYPE_INTER_PIPE_SRC \
  (gst_inter_pipe_src_get_type())
#define GST_INTER_PIPE_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_INTER_PIPE_SRC,GstInterPipeSrc))
#define GST_INTER_PIPE_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_INTER_PIPE_SRC,GstInterPipeSrcClass))
#define GST_IS_INTER_PIPE_SRC(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_INTER_PIPE_SRC))
#define GST_IS_INTER_PIPE_SRC_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_INTER_PIPE_SRC))

/**
 * GstInterPipeSrc:
 *
 * Opaque #GstInterPipeSrc structure.
 */
typedef struct _GstInterPipeSrc GstInterPipeSrc;

/**
 * GstInterPipeSrcClass:
 *
 * Opaque #GstInterPipeSrcClass structure.
 */
typedef struct _GstInterPipeSrcClass GstInterPipeSrcClass;

GType gst_inter_pipe_src_get_type (void);

G_END_DECLS
#endif /* __GST_INTER_PIPE_SRC_H__ */
