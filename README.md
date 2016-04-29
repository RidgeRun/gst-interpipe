# GstInterpipe 1.0
GstInterpipe is a Gstreamer plug-in that allows communication between
two independent pipelines. The plug-in consists of two elements:
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
