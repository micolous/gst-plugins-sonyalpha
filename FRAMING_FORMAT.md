# Framing format

The file is divided into payloads with a common header.  One of the payload types contains JPEG frame data.

This is written out a little differently to [Sony's documentation](http://dl.developer.sony.com/cameras/sdks/CameraRemoteAPIbeta_SDK_2.20.zip) in order to simplify.

All values are big endian (network byte order).

## Common header (16 bytes)

* `uint8` start_byte == 0xFF
* `uint8` payload_type
* `uint16` sequence_number
* `uint32` timestamp
* `char[4]` magic_sequence == 0x24 0x35 0x68 0x79
* `uint24` payload_size
* `uint8` padding_size

## Payloads

These are indicated by the `payload_type` variable:

### Liveview payloads

All payloads are 120 + payload_size + padding_size bytes.

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

### Streaming data payloads

#### `0x11` Streaming image payload

* `uint16` image_width
* `uint16` image_height
* `char[116]` reserved
* `char[payload_size]` jpeg_frame
* `char[padding_size]` padding

#### `0x12` Streaming playback information payload

* `uint16` frame_info_version
* `char[118]` reserved
* `uint32` duration
* `uint32` playback_position
* `char[24]` reserved
* `char[padding_size]` padding

# Prototype demuxer (Python)

There is a prototype demuxer written in Python, `extract_frames.py`.  This does not handle any of the setup/handshake process.  This is used as a proof of concept, and it writes the frames as files to disk.

