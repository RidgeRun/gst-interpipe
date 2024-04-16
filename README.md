# GstInterpipe 1.0
GstInterpipe is a Gstreamer plug-in that allows communication between
two independent pipelines within the same process.

The plug-in consists of two elements:
- interpipesink
- interpipesrc

The concept behind the Interpipes project is to simplify the
construction of GStreamer applications, which often has the complexity
of requiring dynamic pipelines. It transforms the construction process
from low level pad probe manipulation to the higher level setting an
element's parameter value. Application developers don't get mired down
in stalled pipelines due to one branch of a complex pipeline changed
state.

The official user documentation is held at [RidgeRun's Developers
Wiki](http://developer.ridgerun.com/wiki/index.php?title=GstInterpipe)

The API reference is uploaded to [GitHub's project
page](http://ridgerun.github.io/gst-interpipe/).

GstInterpipe copyright (C) 2016-2022 RidgeRun LLC

This GStreamer plug-in is free software; you can redistribute it
and/or modify it under the terms of the GNU Lesser General Public
License version 2.1 as published by the Free Software Foundation.

This GStreamer plug-in is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License below for more details.
