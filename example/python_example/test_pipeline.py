"""A sample gst-pipeline to help showcase switching of sources."""
import gi

gi.require_version("Gst", "1.0")
import sys
import time
from pathlib import Path
from threading import Timer

import test_utils
from gi.repository import GLib, GObject, Gst
from logzero import logger
from test_utils import create_gst_ele

PERMISSIBLE_TIMEOUT = 10
INTERPIPESINK = []
INTERPIPESRC = []
DUMMYSINKS = []
SRC_PIPELINES = []
VIDEO_SOURCE_TIMER = []

class RepeatedTimer:
    """Custom Timer class to execute function every x seconds."""

    def __init__(self, interval, function, *args, **kwargs):
        """
        Doesn't create multiple threads and takes into account time drifts https://stackoverflow.com/a/40965385.

        Args:
            interval (int): Time in (s) for the function to be repeated
            function (callback_function): The function that is to be called repeatedly
        """
        self._timer = None
        self.interval = interval
        self.function = function
        self.args = args
        self.kwargs = kwargs
        self.is_running = False
        self.next_call = time.time()
        self.start()

    def _run(self):
        self.is_running = False
        self.start()
        self.function(*self.args, **self.kwargs)

    def start(self):
        """Start the thread process for Timer."""
        if not self.is_running:
            self.next_call += self.interval
            self._timer = Timer(self.next_call - time.time(), self._run)
            self._timer.start()
            self.is_running = True

    def stop(self):
        """Stop the Timer thread."""
        self._timer.cancel()
        self.is_running = False


def create_dummy_pipeline(index, dummy_pipeline):

    vid_test_source = create_gst_ele("videotestsrc", f"dummysrc_{index}")
    vid_test_source.set_property("is-live", True)
    vid_test_source.set_property("pattern", "black")

    dummy_inter_sink = create_gst_ele("interpipesink")
    dummy_inter_sink.set_property("name", f"dummy_source_{index}")
    dummy_inter_sink.set_property("forward-eos", True)
    dummy_inter_sink.set_property("sync", False)
    DUMMYSINKS.append(dummy_inter_sink)

    dummy_vidconv = create_gst_ele("videoconvert",f"dummy_conv_{index}")
    video_scale = create_gst_ele("videoscale", f"videoscaler{index}")
    caps_vc = create_gst_ele("capsfilter", f"normal_caps{index}")
    caps_vc.set_property("caps", Gst.Caps.from_string("video/x-raw,width=1080,height=720,format=RGB"))

    dummy_pipeline.add(vid_test_source)
    dummy_pipeline.add(dummy_vidconv)
    dummy_pipeline.add(video_scale)
    dummy_pipeline.add(caps_vc)
    dummy_pipeline.add(dummy_inter_sink)


    vid_test_source.link(dummy_vidconv)
    dummy_vidconv.link(video_scale)
    video_scale.link(caps_vc)
    caps_vc.link(dummy_inter_sink)




def create_source_pipeline(index, url):
    src_pipeline = Gst.Pipeline.new(f"source-pipeline{index}")
    if not src_pipeline:
        logger.error("Unable to create source-pipeline")

    def on_rtspsrc_pad_added(r, pad):
        r.link(queue)

    rtsp_source = create_gst_ele("rtspsrc")
    rtsp_source.set_property("location", url)
    rtsp_source.set_property("do-rtsp-keep-alive", 1)
    rtsp_source.connect("pad-added", on_rtspsrc_pad_added)

    queue = create_gst_ele("queue", "queue")
    rtsp_decoder = create_gst_ele("rtph264depay", f"decoder{index}")
    h264parser = create_gst_ele("h264parse", "parser1")
    avdec_decoder = create_gst_ele("avdec_h264", "av_decoder")

    vid_converter = create_gst_ele("videoconvert", f"src_convertor{index}")
    video_scale = create_gst_ele("videoscale", f"src_videoscaler_{index}")

    filter1 = create_gst_ele("capsfilter", f"filter{index}")
    filter1.set_property("caps", Gst.Caps.from_string("video/x-raw,width=1080,height=720,format=RGB"))

    input_interpipesink = create_gst_ele("interpipesink")
    input_interpipesink.set_property("name", f"sink{index}")
    input_interpipesink.set_property("forward-eos", False)
    input_interpipesink.set_property("sync", False)
    input_interpipesink.set_property("drop", True)
    input_interpipesink.set_property("forward-events", True)
    

    logger.debug("Adding elements to Source Pipeline \n")
    src_pipeline.add(rtsp_source)
    src_pipeline.add(queue)
    src_pipeline.add(rtsp_decoder)
    src_pipeline.add(h264parser)
    src_pipeline.add(avdec_decoder)

    src_pipeline.add(vid_converter)
    src_pipeline.add(video_scale)
    src_pipeline.add(filter1)

    src_pipeline.add(input_interpipesink)

    logger.debug("Linking elements in the Source Pipeline \n")
    # rtsp_source.link(queue)
    queue.link(rtsp_decoder)
    rtsp_decoder.link(h264parser)
    h264parser.link(avdec_decoder)

    avdec_decoder.link(vid_converter)
    vid_converter.link(video_scale)
    video_scale.link(filter1)
    filter1.link(input_interpipesink)

    INTERPIPESINK.append(input_interpipesink)
    SRC_PIPELINES.append(src_pipeline)
    VIDEO_SOURCE_TIMER.append(0)

    decoder_srcpad = rtsp_decoder.get_static_pad("src")
    if not decoder_srcpad:
        logger.error("Unable to create src pad\n")
    decoder_srcpad.add_probe(
        Gst.PadProbeType.BUFFER,
        test_utils.source_stream_pad_buffer_probe,
        VIDEO_SOURCE_TIMER,
        index,
        rtsp_decoder,
    )


def create_main_pipeline(index, main_pipeline):
    output_path = f"./source_{index}/image_%05d.jpg"
    interpipesrc = create_gst_ele("interpipesrc", f"interpipesrc{index}")
    interpipesrc.set_property("listen-to", f"sink{index}")
    interpipesrc.set_property("is-live", True)
    interpipesrc.set_property("stream-sync", 1)
    interpipesrc.set_property("emit-signals", True)
    interpipesrc.set_property("allow-renegotiation", True)
    interpipesrc.set_property("do-timestamp", True)

    vid_converter = create_gst_ele("videoconvert", f"main_convertor{index}")

    filter1 = create_gst_ele("capsfilter", f"filter{index}")
    filter1.set_property("caps", Gst.Caps.from_string("video/x-raw, format=RGB"))

    img_enc = create_gst_ele("jpegenc", f"image_decoder_{index}")

    sink = create_gst_ele("multifilesink", f"image_sink_{index}")
    
    Path(output_path).parent.mkdir(parents=True,exist_ok=True)
    sink.set_property("location", output_path)
    sink.set_property("qos", True)

    main_pipeline.add(interpipesrc)
    main_pipeline.add(vid_converter)
    main_pipeline.add(filter1)
    main_pipeline.add(img_enc)
    main_pipeline.add(sink)

    interpipesrc.link(vid_converter)

    vid_converter.link(filter1)
    filter1.link(img_enc)
    img_enc.link(sink)
    INTERPIPESRC.append(interpipesrc)

def main(args):
    if len(args) < 2:
        logger.warning("usage: %s <rtsp_url_1> [rtsp_url_2] ... [rtsp_url_N]\n" % args[0])
        sys.exit()
    
    global perf_data
    number_sources = len(args) - 1
    sources_urls = args[1:]

    # Standard GStreamer initialization
    Gst.init(None)
    logger.debug("Creating Pipeline \n ")

    global dummy_pipeline, main_pipeline
    main_pipeline = Gst.Pipeline()
    if not main_pipeline:
        logger.error("Unable to create Pipeline")
    dummy_pipeline = Gst.Pipeline.new(f"dummy-pipeline")
    if not dummy_pipeline:
        logger.error("Unable to create dummy-pipeline")


    logger.debug("Creating element \n ")
    for index, stream_url in enumerate(sources_urls):
        create_source_pipeline(index, stream_url)
        create_dummy_pipeline(index, dummy_pipeline)
        create_main_pipeline(index, main_pipeline)    

    # create an event loop and feed gstreamer bus mesages to it
    loop = GLib.MainLoop()
    for src_pipeline in SRC_PIPELINES:
        src_bus = src_pipeline.get_bus()
        src_bus.add_signal_watch()
        src_bus.connect("message", test_utils.bus_call_src_pipeline, loop, src_pipeline)

    
    bus = main_pipeline.get_bus()
    bus.add_signal_watch()
    bus.connect("message", test_utils.bus_call, loop, main_pipeline)

    logger.debug("Starting pipeline \n")
    # start play back and listed to events
    dummy_pipeline.set_state(Gst.State.PLAYING)
    for src_pipeline1 in SRC_PIPELINES:
        src_pipeline1.set_state(Gst.State.PLAYING)
    time.sleep(number_sources)
    main_pipeline.set_state(Gst.State.PLAYING)

    repeated_timer = RepeatedTimer(
    PERMISSIBLE_TIMEOUT / 4,
    test_utils.handle_source_downtime,
    VIDEO_SOURCE_TIMER,
    INTERPIPESRC,
    INTERPIPESINK,
    DUMMYSINKS,
    SRC_PIPELINES,
    PERMISSIBLE_TIMEOUT,
)
    try:
        loop.run()
    except:
        pass
    
    # cleanup
    logger.debug("Exiting app\n")
    repeated_timer.stop()
    main_pipeline.set_state(Gst.State.NULL)
    for src_pipeline2 in SRC_PIPELINES:
        src_pipeline2.set_state(Gst.State.NULL)
    dummy_pipeline.set_state(Gst.State.NULL)

if __name__ == '__main__':
    main(sys.argv)
