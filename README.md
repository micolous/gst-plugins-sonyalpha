# [gst-plugins-sonyalpha](https://github.com/micolous/gst-plugins-sonyalpha)

This is a project to access the Sony Alpha Liveview (Smart Remote) API using GStreamer.

This consists only of a demuxer, `sonyalphademux`, which can demux the liveview stream into JPEG frames at a variable bitrate.

[Sony publish a list of cameras that support Remote API](https://developer.sony.com/develop/cameras/device-support/).  This code is only tested on the Sony a6000 (ILCE-6000).

## Building / installing the plugin

```
./autogen.sh
make
cp src/.libs/libgstsonyalpha.so ~/.local/share/gstreamer-1.0/plugins/
```

## Setting up a connection

**Note:** This shows an example connection with a Sony a6000 (ILCE-6000).  Other cameras may have different URLs and menus.

### On the camera...

You need to go into the `Applications` menu, and select `Smart Remote Embedded`.

You may need to update the firmware first in order to have access to this menu, or to install the application.

### On the computer...

Connect to the Wi-Fi network which is indicated on the camera's screen.

Once the computer is connected, Smart Remote on the camera should show "Connecting..."  It will continue to wait while you setup the rest of the connection.

#### Service discovery

The proper way to discover the camera is to do a [UPnP SSDP request](https://en.wikipedia.org/wiki/Simple_Service_Discovery_Protocol).  The messaging required is documented in [Sony's protocol documentation](http://dl.developer.sony.com/cameras/sdks/CameraRemoteAPIbeta_SDK_2.20.zip).  If you're writing a program to interface with this camera, your code should discover the camera in this way.

But for our examples, UPnP is hard.  It is probably the default gateway that the camera has given on it's Wi-Fi network, with port 8080 and the path `/sony/`...

```
$ ip route
default via 192.168.122.1 dev wlp5s0  proto static  metric 600 
```

The API root is then `http://192.168.122.1:8080/sony/`.

#### Submitting requests to the JSON-RPC endpoint

Send requests using a JSON POST data to the `camera` endpoint relative to the API root.  This one is to get the supported methods:

```
$ curl -d '{"method": "getAvailableApiList","params": [],"id": 1,"version": "1.0"}'  http://192.168.122.1:8080/sony/camera; echo ''
{"id":1,"result":[["getVersions","getMethodTypes","getApplicationInfo","getAvailableApiList","getEvent","startRecMode","stopRecMode"]]}
```

You then need to probably `startRecMode` to get access to all the functions:

```
$ curl -d '{"method": "startRecMode","params": [],"id": 1,"version": "1.0"}'  http://192.168.122.1:8080/sony/camera; echo ''
{"id":1,"result":[0]}
```

The camera should then enter shooting mode, and you should get access to more functionality:

```
$ curl -d '{"method": "getAvailableApiList","params": [],"id": 1,"version": "1.0"}'  http://192.168.122.1:8080/sony/camera; echo ''
{"id":1,"result":[["getVersions","getMethodTypes","getApplicationInfo","getAvailableApiList","getEvent","actTakePicture","stopRecMode","startLiveview","stopLiveview","actZoom","setSelfTimer","getSelfTimer","getAvailableSelfTimer","getSupportedSelfTimer","getExposureCompensation","getAvailableExposureCompensation","getSupportedExposureCompensation","setShootMode","getShootMode","getAvailableShootMode","getSupportedShootMode","getSupportedFlashMode"]]}
```

To start liveview mode:

```
$ curl -d '{"method": "startLiveview","params": [],"id": 1,"version": "1.0"}'  http://192.168.122.1:8080/sony/camera; echo ''
{"id":1,"result":["http://192.168.122.1:8080/liveview/liveviewstream"]}
```

This will then give you a HTTP path where you can stream live camera images.  This image format is described in the Framing Format below.

You can stop and start accessing the liveview stream whenever you like.

## Viewing the live stream with GStreamer

Once the camera has `startRecMode` and `startLiveview` (see above), you can build a simple GStreamer pipeline to watch the incoming video feed:

```
$ gst-launch-1.0 souphttpsrc location=http://192.168.122.1:8080/liveview/liveviewstream ! sonyalphademux ! jpegparse ! jpegdec ! videoconvert ! autovideosink
```

## Recording the livestream with curl, and playing back with GStreamer

You can also save some frames for later using curl, then play them back with a similar pipeline:

```
$ curl http://192.168.122.1:8080/liveview/liveviewstream > /tmp/cameraframes
(Wait for 10 seconds) ^C
$ gst-launch-1.0 filesrc location=/tmp/cameraframes ! sonyalphademux ! jpegparse ! jpegdec ! videoconvert ! autovideosink
```

There is no audio on the stream, and the additional metadata from the frame_info payload is not read in this version of the demuxer.

The video is of a variable framerate and the demuxer does not advertise a frame size.  You will discover the framesize from the `jpegparse` element.  The framerate is variable.

**Note:** Smart Remote Embedded only allows one connection to stream live data from the camera.  If you create a second connection and open the `liveviewstream`, the first connection will be dropped.

## Copyright / License / Acknowledgements

Copyright 2016 Michael Farrell <https://github.com/micolous>

License: LGPL 2+

This code is based on the `multipartdemux` plugin in `gst-plugins-good`:

* Copyright 2006 Sjoerd Simons <sjoerd@luon.net>
* Copyright 2004 Wim Taymans <wim@fluendo.com>

Camera Remote API by Sony.


