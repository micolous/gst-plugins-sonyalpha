# gst-plugins-sonyalphalive

This is a project to access the Sony Alpha Liveview (Smart Remote) API using GStreamer.

- todo: implement demuxer
- todo: implement liveview

## Prototype

There is a prototype demuxer written in Python, `extract_frames.py`.  This does not handle any of the setup/handshake process.

## Setting up a connection
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

## Framing format

The file is divided into payloads with a common header.  One of the payload types contains JPEG frame data.

This is written out a little differently to [Sony's documentation](http://dl.developer.sony.com/cameras/sdks/CameraRemoteAPIbeta_SDK_2.20.zip) in order to simplify.

All values are big endian (network byte order).

### Common header (16 bytes)

* `uint8` start_byte == 0xFF
* `uint8` payload_type
* `uint16` sequence_number
* `uint32` timestamp (more on this later)
* `char[4]` magic_sequence == 0x24 0x35 0x68 0x79
* `uint24` payload_size
* `uint8` padding_size

### Payloads

These are indicated by the `payload_type` variable:

#### `0x01` Frame payload (120 + payload_size + padding_size bytes)

* `char[4]` reserved
* `uint8` reserved == 0x00
* `char[115]` reserved
* `char[payload_size]` jpeg_frame
* `char[padding_size]` padding

For frame payloads, the timestamp is given in milliseconds.  The timestamp is relative to when the capture was started.

There are about 18 frames per second of video, but this framerate is never guaranteed.

#### `0x02` Frame info payload (120 + frame_info_size + padding_size bytes)

* `uint16` frame_info_version
* `uint16` frame_count
* `uint16` frame_info_size
* `char[114]` reserved
* `char[frame_info_size]` frame_info
* `char[padding_size]` padding


## Acknowledgements

Camera Remote API by Sony


