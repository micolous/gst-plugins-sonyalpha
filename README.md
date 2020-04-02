# [gst-plugins-sonyalpha](https://github.com/micolous/gst-plugins-sonyalpha)

This project provides a GStreamer demuxer, `sonyalphademux`, which produces
regular JPEG frames from the Sony Alpha Liveview (Smart Remote) API.

[Sony publish a list of cameras that support Remote API][sony-devices].
This code is only tested on the Sony a6000 (ILCE-6000).

## Building / installing the plugin

```
./autogen.sh
make
cp src/.libs/libgstsonyalpha.so ~/.local/share/gstreamer-1.0/plugins/
```

## Setting up a Wi-Fi connection

**Note:** This shows an example connection with a Sony a6000 (ILCE-6000). 
Other cameras may have different URLs and menus.

### 1. Enable remote control from the camera

1. Press <kbd>MENU</kbd>.

1. Navigate to `Airplane Mode`, and set it to `Off` (if not already)

1. Navigate to `Application List`, and press <kbd>ENTER</kbd>

1. Select `Smart Remote Embedded`, and press <kbd>ENTER</kbd>

   (If this is _not_ present, you may need to update the firmware or install
   the application first.)

After it has started, the camera should indicate the Wi-Fi SSID and password
to connect to.

### 2. Connect to the network from the computer

1. Connect to the Wi-Fi network indicated on the camera's screen.

1. Once the computer is connected, Smart Remote on the camera should show
   "Connecting..." It will continue to wait while you setup the rest of the
   connection.

## Bootstrap the remote control API

### API service discovery

The proper way to discover the camera is to do a [UPnP SSDP request][ssdp].

The messaging required is documented in [Sony's SDK][sony-sdk].  If you're
writing a program to interface with this camera, your code should discover
the camera in this way.

If you just want to experiment... UPnP is hard.  On the camera's Wi-Fi
network, the default gateway is probably the camera's IP...

```
$ ip route
default via 192.168.122.1 dev wlp5s0  proto static  metric 600 
```

The API root is `/sony/` and is hosted on port 8080; so for this example the
complete API URL is `http://192.168.122.1:8080/sony/`.

### Submitting requests to the JSON-RPC endpoint

The camera has a HTTP endpoint that accepts command as JSON-encoded HTTP
POST data.

[Complete documentation is available in Sony's SDK][sony-sdk], but important
methods for activating liveview are noted here:

#### Get supported methods (getAvailableApiList)

```
$ curl -d '{"method": "getAvailableApiList","params": [],"id": 1,"version": "1.0"}' http://192.168.122.1:8080/sony/camera; echo ''
{"id":1,"result":[["getVersions","getMethodTypes","getApplicationInfo","getAvailableApiList","getEvent","startRecMode","stopRecMode"]]}
```

#### Start recording mode (startRecMode)

Despite the command name, this only makes the camera operate in remote
shooting mode, giving access to all functionality:

```
$ curl -d '{"method": "startRecMode","params": [],"id": 1,"version": "1.0"}' http://192.168.122.1:8080/sony/camera; echo ''
{"id":1,"result":[0]}
```

Once in recording mode, `getAvailableApiList` should show more methods:

```
$ curl -d '{"method": "getAvailableApiList","params": [],"id": 1,"version": "1.0"}' http://192.168.122.1:8080/sony/camera; echo ''
{"id":1,"result":[["getVersions","getMethodTypes","getApplicationInfo","getAvailableApiList","getEvent","actTakePicture","stopRecMode","startLiveview","stopLiveview","actZoom","setSelfTimer","getSelfTimer","getAvailableSelfTimer","getSupportedSelfTimer","getExposureCompensation","getAvailableExposureCompensation","getSupportedExposureCompensation","setShootMode","getShootMode","getAvailableShootMode","getSupportedShootMode","getSupportedFlashMode"]]}
```

#### Start liveview mode (startLiveview)

**Note:** the camera must already be in recording mode to use this command.

```
$ curl -d '{"method": "startLiveview","params": [],"id": 1,"version": "1.0"}' http://192.168.122.1:8080/sony/camera; echo ''
{"id":1,"result":["http://192.168.122.1:8080/liveview/liveviewstream"]}
```

This will then give you a HTTP path where you can stream live camera images. 
You can stop and start accessing the liveview stream whenever you like.

This image format is described in the [Framing Format docs][framing-format],
and is what `sonyalphademux` decodes!

## Viewing the live stream with GStreamer

Once the camera has `startRecMode` and `startLiveview` (see above), you can
build a simple GStreamer pipeline to watch the incoming video feed:

```
$ gst-launch-1.0 souphttpsrc location=http://192.168.122.1:8080/liveview/liveviewstream \
	! sonyalphademux ! jpegparse ! jpegdec ! videoconvert ! autovideosink
```

## Recording the livestream with curl, and playing back with GStreamer

You can also save some frames for later using curl, then play them back with
a similar pipeline:

```
$ curl http://192.168.122.1:8080/liveview/liveviewstream > /tmp/cameraframes
(Wait for 10 seconds) ^C
$ gst-launch-1.0 filesrc location=/tmp/cameraframes \
	! sonyalphademux ! jpegparse ! jpegdec ! videoconvert ! autovideosink
```

There is no audio on the stream, and the additional metadata from the
`frame_info` payload is not read in this version of the demuxer.

The video is of a variable framerate and the demuxer does not advertise a
frame size.  You will discover the framesize from the `jpegparse` element. 
The framerate is variable.

**Note:** Smart Remote Embedded only allows one connection to stream live
data from the camera.  If you create a second connection and open the
`liveviewstream`, the first connection will be dropped.

## Copyright / License / Acknowledgements

Copyright 2016 Michael Farrell https://github.com/micolous

License: LGPL 2+

This code is based on the `multipartdemux` plugin in `gst-plugins-good`:

* Copyright 2006 Sjoerd Simons <sjoerd@luon.net>
* Copyright 2004 Wim Taymans <wim@fluendo.com>

Camera Remote API by Sony.

[framing-format]: ./FRAMING_FORMAT.md
[sony-devices]: https://developer.sony.com/develop/cameras/device-support/
[sony-sdk]: http://dl.developer.sony.com/cameras/sdks/CameraRemoteAPIbeta_SDK_2.20.zip
[ssdp]: https://en.wikipedia.org/wiki/Simple_Service_Discovery_Protocol

