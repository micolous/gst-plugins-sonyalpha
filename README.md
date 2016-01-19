# gst-plugins-sonyalphalive

- todo: implement demuxer
- todo: implement liveview


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

#### `0x01` Frame payload

* `char[4]` reserved
* `uint8` reserved == 0x00
* `char[115]` reserved
* `char[payload_size]` jpeg_frame
* `char[padding_size]` padding

For frame payloads, the timestamp is given in milliseconds.  The timestamp is relative to when the capture was started.

There are about 18 frames per second of video, but this framerate is never guaranteed.

#### `0x02` Frame info payload

* `uint16` frame_info_version
* `uint16` frame_count
* `uint16` frame_info_size
* `char[114]` reserved
* `char[frame_info_size]` frame_info
* `char[padding_size]` padding


