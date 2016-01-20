/* GStreamer

 *
 * gstsonyalphademux.c: sonyalpha stream demuxer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin St, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

/**
 * SECTION:element-sonyalphademux
 * @see_also: #GstSonyAlphaMux
 *
 *
 * <refsect2>
 * <title>Sample pipelines</title>
 * |[
 * gst-launch-1.0 filesrc location=/tmp/test.sonyalpha ! sonyalphademux ! image/jpeg,framerate=\(fraction\)5/1 ! jpegparse ! jpegdec ! videoconvert ! autovideosink
 * ]| a simple pipeline to demux a sonyalpha file muxed with #GstSonyAlphaMux
 * containing JPEG frames.
 * </refsect2>
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "sonyalphademux.h"

GST_DEBUG_CATEGORY_EXTERN (gst_sonyalpha_debug);
#define GST_CAT_DEFAULT gst_sonyalpha_debug

enum
{
  PROP_0,
};

static GstStaticPadTemplate sonyalpha_demux_src_template_factory =
GST_STATIC_PAD_TEMPLATE ("src",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS ("image/jpeg")
    );

static GstStaticPadTemplate sonyalpha_demux_sink_template_factory =
GST_STATIC_PAD_TEMPLATE ("sink",
    GST_PAD_SINK,
    GST_PAD_ALWAYS,
    GST_STATIC_CAPS ("application/x-sony-alpha-live")
    );


static GstFlowReturn gst_sonyalpha_demux_chain (GstPad * pad,
    GstObject * parent, GstBuffer * buf);

static GstStateChangeReturn gst_sonyalpha_demux_change_state (GstElement *
    element, GstStateChange transition);

static void gst_sonyalpha_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec);

static void gst_sonyalpha_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec);

static void gst_sonyalpha_demux_dispose (GObject * object);

#define gst_sonyalpha_demux_parent_class parent_class
G_DEFINE_TYPE (GstSonyAlphaDemux, gst_sonyalpha_demux, GST_TYPE_ELEMENT);

static void
gst_sonyalpha_demux_class_init (GstSonyAlphaDemuxClass * klass)
{
  int i;

  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);
  GstElementClass *gstelement_class = GST_ELEMENT_CLASS (klass);

  gobject_class->dispose = gst_sonyalpha_demux_dispose;
  gobject_class->set_property = gst_sonyalpha_set_property;
  gobject_class->get_property = gst_sonyalpha_get_property;

  gstelement_class->change_state = gst_sonyalpha_demux_change_state;

  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sonyalpha_demux_sink_template_factory));
  gst_element_class_add_pad_template (gstelement_class,
      gst_static_pad_template_get (&sonyalpha_demux_src_template_factory));
  gst_element_class_set_static_metadata (gstelement_class, "Sony Alpha demuxer",
      "Codec/Demuxer",
      "demux sonyalpha live streams",
      "Michael Farrell <micolous+git@gmail.com>");
}

static void
gst_sonyalpha_demux_init (GstSonyAlphaDemux * sonyalpha)
{
  /* create the sink pad */
  sonyalpha->sinkpad =
      gst_pad_new_from_static_template (&sonyalpha_demux_sink_template_factory,
      "sink");
  gst_element_add_pad (GST_ELEMENT_CAST (sonyalpha), sonyalpha->sinkpad);
  sonyalpha->srcpad =
      gst_pad_new_from_static_template (&sonyalpha_demux_src_template_factory,
      "src");
  gst_element_add_pad (GST_ELEMENT_CAST (sonyalpha), sonyalpha->srcpad);

  gst_pad_set_chain_function (sonyalpha->sinkpad,
      GST_DEBUG_FUNCPTR (gst_sonyalpha_demux_chain));


  sonyalpha->adapter = gst_adapter_new ();
  sonyalpha->header_completed = FALSE;
  sonyalpha->scanpos = 0;
}


static void
gst_sonyalpha_demux_dispose (GObject * object)
{
  GstSonyAlphaDemux *demux = GST_SONYALPHA_DEMUX (object);

  if (demux->adapter != NULL)
    g_object_unref (demux->adapter);
  demux->adapter = NULL;

  G_OBJECT_CLASS (parent_class)->dispose (object);
}


static gint
sonyalpha_parse_header (GstSonyAlphaDemux * sonyalpha)
{
  const guint8 *data;
  const guint8 *dataend;
  memset(&(sonyalpha->header), 0, sizeof(GstSonyAlphaPayloadHeader));

  int datalen;

  datalen = gst_adapter_available (sonyalpha->adapter);

  if (datalen < 128)
    goto need_more_data;

  data = gst_adapter_take_buffer (sonyalpha->adapter, 128);


  /* Need a header */
  if (GST_READ_UINT8(data) != 0xFF) {
    GST_DEBUG_OBJECT (sonyalpha, "Expected header byte (0xFF), got %hhx instead", GST_READ_UINT8(data));
    GST_DEBUG_OBJECT (sonyalpha, "First bytes were 0x%llx 0x%llx", GST_READ_UINT64_BE(data), GST_READ_UINT64_BE(data + 8));
    goto wrong_header;
  }

  /* Check for a magic sequence */
  uint32_t magic = GST_READ_UINT32_BE(data + 8);
  if (magic != htonl(0x24356879)) {
    GST_DEBUG_OBJECT (sonyalpha, "Did not get magic number (0x24356879), got 0x%lx instead", magic);
    goto wrong_header;
  }

  sonyalpha->header.payload_type = GST_READ_UINT8(data + 1);
  sonyalpha->header.sequence_number = GST_READ_UINT16_BE(data + 2);

  /* Get the first timestamp of the file */
  if (sonyalpha->header.payload_type == 0x01) {
    sonyalpha->header.timestamp = GST_READ_UINT32_BE(data + 4);
    if (sonyalpha->first_timestamp == 0) {
      sonyalpha->first_timestamp = sonyalpha->header.timestamp;      
      GST_DEBUG_OBJECT (sonyalpha, "First timestamp value is %lu.", sonyalpha->first_timestamp);
    }
  }
  
  /* Get the payload and padding sizes */
  sonyalpha->header.payload_size = GST_READ_UINT24_BE(data + 12);
  sonyalpha->header.padding_size = GST_READ_UINT8(data + 15);
  GST_DEBUG_OBJECT (sonyalpha, "p%01hhu | seq=%06d | ts=%010llu | s=%lu+%hhu", sonyalpha->header.payload_type, sonyalpha->header.sequence_number, sonyalpha->header.timestamp, sonyalpha->header.payload_size, sonyalpha->header.padding_size);

  gst_adapter_unmap (sonyalpha->adapter);
  return 128;


need_more_data:
  GST_DEBUG_OBJECT (sonyalpha, "Need more data for the header");
  gst_adapter_unmap (sonyalpha->adapter);

  return SONYALPHA_NEED_MORE_DATA;

wrong_header:
  {
    GST_ELEMENT_ERROR (sonyalpha, STREAM, DEMUX, (NULL),
        ("Wrong header found for sonyalpha"));
    gst_adapter_unmap (sonyalpha->adapter);
    return SONYALPHA_DATA_ERROR;
  }
}


static GstFlowReturn
gst_sonyalpha_demux_chain (GstPad * pad, GstObject * parent, GstBuffer * buf)
{
  GstSonyAlphaDemux *sonyalpha;
  GstAdapter *adapter;
  gint size = 1;
  GstFlowReturn res;

  sonyalpha = GST_SONYALPHA_DEMUX (parent));
  adapter = sonyalpha->adapter;

  res = GST_FLOW_OK;

  if (GST_BUFFER_FLAG_IS_SET (buf, GST_BUFFER_FLAG_DISCONT)) {
    gst_adapter_clear (adapter);
  }
  gst_adapter_push (adapter, buf);

  while (gst_adapter_available (adapter) > 0) {
    GstPad *srcpad;
    GstBuffer *outbuf;
    gboolean created;

    if (!sonyalpha->header_completed) {
      if ((size = sonyalpha_parse_header (sonyalpha)) < 0) {
        goto nodata;
      } else {
        sonyalpha->header_completed = TRUE;
      }
    }
    
    if (gst_adapter_available(adapter) < (sonyalpha->header.payload_size + sonyalpha->header.padding_size)) {
      GST_DEBUG_OBJECT(sonyalpha, "need %llu bytes of buffer, got only %llu bytes", 
        (sonyalpha->header.payload_size + sonyalpha->header.padding_size),
        gst_adapter_available(adapter));
      return GST_FLOW_OK;
    }
    
    /* We have enough data to continue, invalidate the header */
    sonyalpha->header_completed = FALSE;

    if (sonyalpha->header.payload_type != 0x01) {
      /* We don't handle other payloads, skip them. */
      gst_adapter_flush (adapter, sonyalpha->header.payload_size + sonyalpha->header.padding_size);
      continue;
    }

	  /* We have a file header, lets start working on these values. */
	  srcpad = sonyalpha->srcpad;
    GstClockTime ts;

    ts = gst_adapter_prev_pts (adapter, NULL);
    outbuf = gst_adapter_take_buffer (adapter, sonyalpha->header.payload_size);
    gst_adapter_flush (adapter, sonyalpha->header.padding_size);

    GstTagList *tags;
    GstSegment segment;

    gst_segment_init (&segment, GST_FORMAT_TIME);

    /* Push new segment, first buffer has 0 timestamp */
    gst_pad_push_event (srcpad, gst_event_new_segment (&segment));

    tags = gst_tag_list_new (GST_TAG_CONTAINER_FORMAT, "SonyAlpha", NULL);
    gst_pad_push_event (srcpad, gst_event_new_tag (tags));


    outbuf = gst_buffer_make_writable (outbuf);
    GST_BUFFER_TIMESTAMP (outbuf) = GST_CLOCK_TIME_NONE;
    /*
    if (srcpad->last_ts == GST_CLOCK_TIME_NONE || srcpad->last_ts != ts) {
      GST_BUFFER_TIMESTAMP (outbuf) = ts;
      srcpad->last_ts = ts;
    } else { 
      GST_BUFFER_TIMESTAMP (outbuf) = GST_CLOCK_TIME_NONE;
    }

    if (srcpad->discont) {
      GST_BUFFER_FLAG_SET (outbuf, GST_BUFFER_FLAG_DISCONT);
      srcpad->discont = FALSE;
    } else { 
      GST_BUFFER_FLAG_UNSET (outbuf, GST_BUFFER_FLAG_DISCONT);
    }
*/
    GST_DEBUG_OBJECT (sonyalpha,
        "pushing buffer with timestamp %" GST_TIME_FORMAT,
        GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (outbuf)));
    res = gst_pad_push (srcpad, outbuf);
    //res = gst_sonyalpha_combine_flows (sonyalpha, srcpad, res);
    
    if (res != GST_FLOW_OK)
      break;
    
  }

nodata:
  if (G_UNLIKELY (size == SONYALPHA_DATA_ERROR))
    return GST_FLOW_ERROR;
  if (G_UNLIKELY (size == SONYALPHA_DATA_EOS))
    return GST_FLOW_EOS;

  return res;
}

static GstStateChangeReturn
gst_sonyalpha_demux_change_state (GstElement * element,
    GstStateChange transition)
{
  GstSonyAlphaDemux *sonyalpha;
  GstStateChangeReturn ret;

  sonyalpha = GST_SONYALPHA_DEMUX (element);

  ret = GST_ELEMENT_CLASS (parent_class)->change_state (element, transition);
  if (ret == GST_STATE_CHANGE_FAILURE)
    return ret;

  switch (transition) {
    case GST_STATE_CHANGE_PLAYING_TO_PAUSED:
      break;
    case GST_STATE_CHANGE_PAUSED_TO_READY:
      sonyalpha->header_completed = FALSE;
      gst_adapter_clear (sonyalpha->adapter);
      sonyalpha->scanpos = 0;
      break;
    case GST_STATE_CHANGE_READY_TO_NULL:
      break;
    default:
      break;
  }

  return ret;
}


static void
gst_sonyalpha_set_property (GObject * object, guint prop_id,
    const GValue * value, GParamSpec * pspec)
{
  GstSonyAlphaDemux *filter;

  filter = GST_SONYALPHA_DEMUX (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}

static void
gst_sonyalpha_get_property (GObject * object, guint prop_id,
    GValue * value, GParamSpec * pspec)
{
  GstSonyAlphaDemux *filter;

  filter = GST_SONYALPHA_DEMUX (object);

  switch (prop_id) {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
  }
}


