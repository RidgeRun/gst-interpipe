import time
import gi

gi.require_version("Gst", "1.0")
from gi.repository import Gst
from logzero import logger


def handle_source_downtime(timers2, interpipesrcs, interpipesinks, dummysink, src_pipelines, permissible_cam_timeout):
    """
    Change to dummy source, if buffers stop flowing in original source pipeline.

    Args:
        timers2 (list): Timestamps of when buffers were recently updated
        interpipesrcs (list): List of interpipesources in mainpipeline
        interpipesinks (list): List of interpipesinks which send buffers to mainpipeline
        dummysink (list): List of dummy interpipesinks in dummypipeline
        src_pipelines (list): List containing source pipelines
        permissible_cam_timeout (float): Maximum time for which a video source can be without fresh data.
    """
    time_1 = time.time()
    current_dummies = [
        i for i, _ in enumerate(timers2) if interpipesrcs[i].get_property("listen-to").startswith("dummy_source_")
    ]
    # Iterate through all the sources' latest buffer timestamps
    for i, time_2 in enumerate(timers2):
        # If the first buffer for said video source hasn't flown yet, then the dummy source will not be
        # able to negotiate a proper shape, etc to send videotestsrc buffers in
        # NOTE: Lines 157-159 are commented out as caps aren't set(or required) for dummy buffers from original source.
        # if data_started[i] is False:
        #     logger.debug(f"Missing first contact from source {i+1} to gst core pipeline")
        #     continue

        # If said video source is currently linked to a dummy source
        if i in current_dummies:
            # Restart source if actual video source hasn't sent a buffer in the recent past
            if time_1 - time_2 > permissible_cam_timeout:
                src_pipelines[i].set_state(Gst.State.NULL)
                src_pipelines[i].set_state(Gst.State.PLAYING)

            # Else switch the dummy input back to the actual video source
            else:
                original_sink = interpipesinks[i].get_property("name")
                interpipesrcs[i].set_property("listen-to", original_sink)
                current_dummies.remove(i)
                logger.debug(f"Input {i} is listening back to {original_sink}\n")

        # If said video source is linked to a actual video source albeit a zombie (no buffers for a while)
        elif (i not in current_dummies) and time_1 - time_2 > permissible_cam_timeout:
            logger.debug(f"No buffers are recieved from actual source: {i}\n")
            dummy = Gst.Object.get_name(dummysink[i])
            interpipesrcs[i].set_property("listen-to", dummy)
            current_dummies.append(i)
            logger.debug(f"Input: {i} has been changed to its dummy input\n")
        # If no switch is to be made for said video source, then move on to next cam
        else:
            continue
        logger.debug(f"Current dummies: {current_dummies}\n")


def source_stream_pad_buffer_probe(_pad, info, vid_source_timer, index, _source):
    gst_buffer = info.get_buffer()
    # logger.info(f"buffers coming through for source_pipeline:{_source.name}-{index} \n")
    if not gst_buffer:
        logger.error("Unable to get GstBuffer ")
        return Gst.PadProbeReturn.DROP
    time_2 = time.time()
    vid_source_timer[index] = time_2
    return Gst.PadProbeReturn.OK


def create_gst_ele(element, name=""):
    """
    Create GStreamer Element along with catching any failures in the making.

    Args:
        element (str): Type of element to be made
        name (str): Name to be assigned internal to GStreamer for that element

    Returns:
        Gstreamer Element: Element that was requested
    """
    if name:
        ele = Gst.ElementFactory.make(element, name)
    else:
        ele = Gst.ElementFactory.make(element)
    if not ele:
        logger.error(f"Unable to create Element: {element}, with name {name}")
    return ele


def bus_call(_bus, message, loop, gst_pipeline):
    """
    Handle all messages coming through to the core pipeline.

    Args:
        bus (Gst.Bus): Reference to bus of pipeline
        message (Gst.Message): Type of message that is to be handled
        loop (GObject.MainLoop): Main event loop
        gst_pipeline (Gst.Pipeline): reference to main pipeline
        uris: contains list of input uris
    Returns:
        True, if messages handled correctly, else throws error
    """
    type_ = message.type
    if type_ == Gst.MessageType.EOS:
        logger.debug(f"End-of-stream from: {Gst.Object.get_name(message.src)}\n")
        loop.quit()
    elif type_ == Gst.MessageType.WARNING:
        err, debug = message.parse_warning()
        if "Impossible to configure latency" not in debug:
            # errors from flvmuxer in the case of rtmp output are ignored
            logger.debug(f"{err}:- {debug}\n")
    elif type_ == Gst.MessageType.ERROR:
        err, debug = message.parse_error()
        if "watchdog" in Gst.Object.get_name(message.src):
            logger.debug("GStreamer Mux watchdog triggered")
            gst_pipeline.pipeline.set_state(Gst.State.NULL)
            gst_pipeline.pipeline.send_event(Gst.Event.new_eos())
            logger.debug("Sent EoS")
            gst_pipeline.cleanup()
            logger.debug("Restart completed")
        elif "interpipesrc" in str(Gst.Object.get_name(message.src)).lower():
            if "Internal data stream error" in str(err):
                logger.debug(f"Internal data stream error in {Gst.Object.get_name(message.src)}\n")
            else:
                logger.debug(f"{err}:- {debug}\n")
        else:
            logger.debug(f"{err}:- {debug}\n")
        loop.quit()
    elif type_ in (
        Gst.MessageType.STATE_CHANGED,
        Gst.MessageType.STREAM_STATUS,
        Gst.MessageType.ELEMENT,
        Gst.MessageType.TAG,
        Gst.MessageType.DURATION_CHANGED,
        Gst.MessageType.ASYNC_DONE,
        Gst.MessageType.NEW_CLOCK,
        Gst.MessageType.PROGRESS,
        Gst.MessageType.BUFFERING,
        Gst.MessageType.QOS,
        Gst.MessageType.LATENCY,
    ):
        pass
    elif type_ == Gst.MessageType.STREAM_START:
        logger.debug("New Data Stream started\n")
    else:
        logger.debug(f"Unknown message type {type_}: {message}\n")
    return True


def bus_call_src_pipeline(_bus, message, loop, gst_pipeline):
    """
    Handle all messages coming through to the source pipeline.

    Args:
        bus (Gst.Bus): Reference to bus of pipeline
        message (Gst.Message): Type of message that is to be handled
        loop (GObject.MainLoop): Main event loop
        gst_pipeline (Gst.Pipeline): reference to the source pipeline
        index (int): index
        uri_name (str): input uri fed to src pipeline
        files_list (list): list containing the files matching wildcard pattern
        input_datatype (string): input datatype of source uri

    Returns:
        True, if messages handled correctly, else throws error
    """
    type_ = message.type
    if type_ == Gst.MessageType.EOS:
        logger.debug("EoS from URI\n")
        loop.quit()
    elif type_ == Gst.MessageType.WARNING:
        err, debug = message.parse_warning()
        logger.debug(f"{err}:- {debug}\n")
    elif type_ == Gst.MessageType.ERROR:
        parse_error_message(message, gst_pipeline, loop)
    elif type_ in (
        Gst.MessageType.STATE_CHANGED,
        Gst.MessageType.STREAM_STATUS,
        Gst.MessageType.ELEMENT,
        Gst.MessageType.TAG,
        Gst.MessageType.DURATION_CHANGED,
        Gst.MessageType.ASYNC_DONE,
        Gst.MessageType.NEW_CLOCK,
        Gst.MessageType.PROGRESS,
        Gst.MessageType.BUFFERING,
        Gst.MessageType.QOS,
        Gst.MessageType.LATENCY,
    ):
        pass
    elif type_ == Gst.MessageType.ELEMENT:
        logger.debug(f"{message} and {message.src}\n")
    elif type_ == Gst.MessageType.STREAM_START:
        logger.debug(f"New Data Stream started at source {Gst.Object.get_name(gst_pipeline)}\n")
    else:
        if "tsdemux" in str(Gst.Object.get_name(message.src)):
            return True
        logger.debug(f"Unknown message type {type_}: {message}, source: {Gst.Object.get_name(message.src)}\n")
    return True


def parse_error_message(message, gst_pipeline, loop):
    """
    Parse error messages sent from bus and processes them accordingly.

    Args:
        message (Gst.MessageType.ERROR): Error message from bus
        gst_pipeline (Gst.Pipeline): reference to the source pipeline
        loop (GObject.MainLoop): Main event loop
    """
    err, debug = message.parse_error()
    if any(domain in err.domain for domain in ("stream", "resource")):
        logger.debug(f"Stream/Resource Error. Pausing the pipeline {Gst.Object.get_name(gst_pipeline)}\n")
        try:
            gst_pipeline.set_state(Gst.State.NULL)

        # Suppose it occurs on first run, the gst_pipeline.pipeline object itself hasn't been created yet
        except AttributeError:
            pass
    else:
        logger.error(f"{err}:- {debug}\n")
        loop.quit()
