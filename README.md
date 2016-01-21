# gst-plugins-sonyalphalive

This is a project to access the Sony Alpha Liveview (Smart Remote) API using GStreamer.

This consists only of a demuxer, `sonyalphademux`, which can demux the liveview stream into JPEG frames at a variable bitrate.

## Setting up a connection

**Note:** This shows an example connection with a Sony a6000.  Other cameras may have different URLs and requirements.

### On the camera...

You need to go into the Applications menu, and select "Smart Remote Embedded".  You may need to update the firmware first in order to have access to this menu.

### On the computer...

Connect to the Wi-Fi network which is indicated on the camera's screen.

You can do UPNP SSDP in order to find the camera's server address.  But it is probably the default gateway that the camera has given...

```
$ ip route
default via 192.168.122.1 dev wlp5s0  proto static  metric 600 
```

The API root is then `http://192.168.122.1:8080/sony/`.

Once the computer is connected, Smart Remote on the camera should show "Connecting..."  It will continue to wait while you setup the rest of the connection.

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

**Note:** Smart Remote Embedded only allows one connection to stream live data from the camera.  If you create a second connection and open the `liveviewstream`, the first connection will be dropped.

## Acknowledgements

Camera Remote API by Sony


