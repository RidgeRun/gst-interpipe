# Dynamic source switching with interpipes

## Introduction 

In this example, we attempt to show dynamic switching of interpipes when an `rtsp` source goes down and a dummy source takes it's place. 

NOTE: Rtsp source is used here, but this can be replicated to any other source as well. 

## Usage

1. The following command is used to run the pipeline:

    ```bash
   python3 test_pipeline.py <rtsp_url_1> [rtsp_url_2] ... [rtsp_url_N]
   
   ```
    * For eg: 
        * ```python3 test_pipeline.py "rtsp://127.x.x.1:8554/stream1" "rtsp://127.x.x.2:8554/stream2"```

# Explanation

The example code ingests rtsp stream(s) and saves every frame as a jpg image. 

This functionality is achieved by splitting the pipeline into 3:-

* Original Source Pipeline(s)- (OSP) -> This pipeline consists of the rtsp source and which is decoded to raw buffers.

* Main Pipeline -> Ingests decoded raw buffers from various source pipelines, and saves them as a jpg image.

* Dummy Pipeline(s) -> Acts as a substitute and sends in frames, when any of the OSP is down.

### Initial Flow of buffers
![Initial Pipeline](/gst-interpipe/example/python_example/img_files/Source%20Switching.drawio.png)

### Flow of buffers when source has changed
![Pipeline with Source switched](/gst-interpipe/example/python_example/img_files/Source%20Switched.drawio.png)


## How is this achieved?

A buffer probe is attached to the decoding element and the  triggers `source_stream_pad_buffer_probe` function. This keeps updating the most recent buffer timestamps that pass through the decoding element.

The variable `PERMISSIBLE_TIMEOUT` holds the timelimit in seconds an OSP can be down before switching to a dummy is initiated. 

The function `handle_source_downtime` is running in a separate thread which is executed every `PERMISSIBLE_TIMEOUT/4` seconds. 

### How does it work? 

* This function checks if a source pipeline is alive by checking if the recent buffer timestamp has exceeded the `PERMISSIBLE_TIMEOUT` value. If yes, the interpipesrc _`listen-to`_ property is changed to the corresponding dummy source pipeline.  

* Since `handle_source_downtime` runs every _`PERMISSIBLE_TIMEOUT/4`_ seconds, list of interpipesrc(s) listening to a dummy source are stored in `current_dummies`. We iterate through the recent timestamp stored in the list `timers2` and check if any of the previously "dead" source pipelines, are pushing out buffers. 

* If a new buffer hasn't arrived in the OSP _(the timestamp would still exceed the _`PERMISSIBLE_TIMEOUT/4`_ threshold)_ it is restarted. 

* If a source is back online(_the difference between the current time and latest timestamp is less than `PERMISSIBLE_TIMEOUT/4` seconds_) the interpipsrc _`listen-to`_ property is changed back to the correspondingh source interpipesink.