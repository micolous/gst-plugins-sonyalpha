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

GST_DEBUG_CATEGORY_STATIC (gst_sonyalpha_demux_debug);
#define GST_CAT_DEFAULT gst_sonyalpha_demux_debug

#define DEFAULT_SINGLE_STREAM	FALSE

enum
{
  PROP_0,
};

static GstStaticPadTemplate sonyalpha_demux_src_template_factory =
GST_STATIC_PAD_TEMPLATE ("src_%u",
    GST_PAD_SRC,
    GST_PAD_SOMETIMES,
    GST_STATIC_CAPS_ANY);

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
  gst_element_class_set_static_metadata (gstelement_class, "SonyAlpha demuxer",
      "Codec/Demuxer",
      "demux sonyalpha streams",
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
  gst_pad_set_chain_function (sonyalpha->sinkpad,
      GST_DEBUG_FUNCPTR (gst_sonyalpha_demux_chain));

  sonyalpha->adapter = gst_adapter_new ();
  sonyalpha->mime_type = "image/jpeg";
  sonyalpha->content_length = -1;
  sonyalpha->header_completed = FALSE;
  sonyalpha->scanpos = 0;
  sonyalpha->singleStream = DEFAULT_SINGLE_STREAM;
}

static void
gst_sonyalpha_demux_remove_src_pads (GstSonyAlphaDemux * demux)
{
  while (demux->srcpads != NULL) {
    GstSonyAlphaPad *mppad = demux->srcpads->data;

    gst_element_remove_pad (GST_ELEMENT (demux), mppad->pad);
    g_free (mppad->mime);
    g_free (mppad);
    demux->srcpads = g_slist_delete_link (demux->srcpads, demux->srcpads);
  }
  demux->srcpads = NULL;
  demux->numpads = 0;
}

static void
gst_sonyalpha_demux_dispose (GObject * object)
{
  GstSonyAlphaDemux *demux = GST_SONYALPHA_DEMUX (object);

  if (demux->adapter != NULL)
    g_object_unref (demux->adapter);
  demux->adapter = NULL;
  g_free (demux->mime_type);
  demux->mime_type = NULL;
  gst_sonyalpha_demux_remove_src_pads (demux);

  G_OBJECT_CLASS (parent_class)->dispose (object);
}

static const gchar *
gst_sonyalpha_demux_get_gstname (GstSonyAlphaDemux * demux, gchar * mimetype)
{
  GstSonyAlphaDemuxClass *klass;
  const gchar *gstname;

  klass = GST_SONYALPHA_DEMUX_GET_CLASS (demux);

  /* use hashtable to convert to gst name */
  gstname = g_hash_table_lookup (klass->gstnames, mimetype);
  if (gstname == NULL) {
    /* no gst name mapping, use mime type */
    gstname = mimetype;
  }
  GST_DEBUG_OBJECT (demux, "gst name for %s is %s", mimetype, gstname);
  return gstname;
}

static GstFlowReturn
gst_sonyalpha_combine_flows (GstSonyAlphaDemux * demux, GstSonyAlphaPad * pad,
    GstFlowReturn ret)
{
  GSList *walk;

  /* store the value */
  pad->last_ret = ret;

  /* any other error that is not-linked can be returned right
   * away */
  if (ret != GST_FLOW_NOT_LINKED)
    goto done;

  /* only return NOT_LINKED if all other pads returned NOT_LINKED */
  for (walk = demux->srcpads; walk; walk = g_slist_next (walk)) {
    GstSonyAlphaPad *opad = (GstSonyAlphaPad *) walk->data;

    ret = opad->last_ret;
    /* some other return value (must be SUCCESS but we can return
     * other values as well) */
    if (ret != GST_FLOW_NOT_LINKED)
      goto done;
  }
  /* if we get here, all other pads were unlinked and we return
   * NOT_LINKED then */
done:
  return ret;
}

static GstSonyAlphaPad *
gst_sonyalpha_find_pad_by_mime (GstSonyAlphaDemux * demux, gchar * mime,
    gboolean * created)
{
  GSList *walk;

  walk = demux->srcpads;
  while (walk) {
    GstSonyAlphaPad *pad = (GstSonyAlphaPad *) walk->data;

    if (!strcmp (pad->mime, mime)) {
      if (created) {
        *created = FALSE;
      }
      return pad;
    }

    walk = walk->next;
  }
  /* pad not found, create it */
  {
    GstPad *pad;
    GstSonyAlphaPad *mppad;
    gchar *name;
    const gchar *capsname;
    GstCaps *caps;

    mppad = g_new0 (GstSonyAlphaPad, 1);

    GST_DEBUG_OBJECT (demux, "creating pad with mime: %s", mime);

    name = g_strdup_printf ("src_%u", demux->numpads);
    pad =
        gst_pad_new_from_static_template (&sonyalpha_demux_src_template_factory,
        name);
    g_free (name);

    mppad->pad = pad;
    mppad->mime = g_strdup (mime);
    mppad->last_ret = GST_FLOW_OK;
    mppad->last_ts = GST_CLOCK_TIME_NONE;
    mppad->discont = TRUE;

    demux->srcpads = g_slist_prepend (demux->srcpads, mppad);
    demux->numpads++;

    /* take the mime type, convert it to the caps name */
    capsname = gst_sonyalpha_demux_get_gstname (demux, mime);
    caps = gst_caps_from_string (capsname);
    GST_DEBUG_OBJECT (demux, "caps for pad: %s", capsname);
    gst_pad_use_fixed_caps (pad);
    gst_pad_set_active (pad, TRUE);
    gst_pad_set_caps (pad, caps);
    gst_caps_unref (caps);

    gst_element_add_pad (GST_ELEMENT_CAST (demux), pad);

    if (created) {
      *created = TRUE;
    }

    if (demux->singleStream) {
      gst_element_no_more_pads (GST_ELEMENT_CAST (demux));
    }

    return mppad;
  }
}


static gint
sonyalpha_parse_header (GstSonyAlphaDemux * sonyalpha)
{
  const guint8 *data;
  const guint8 *dataend;

  int boundary_len;
  int datalen;
  guint8 *pos;
  guint8 *end, *next;

  datalen = gst_adapter_available (sonyalpha->adapter);
  data = gst_adapter_map (sonyalpha->adapter, datalen);
  dataend = data + datalen;

  if (0 >= dataend - 136)
    goto need_more_data;

  if (G_UNLIKELY (data[0] != '\xFF')) {
    GST_DEBUG_OBJECT (sonyalpha, "Expected header byte (0xFF)");
    goto wrong_header;
  }

  gst_adapter_unmap (sonyalpha->adapter);
  return 1;


need_more_data:
  GST_DEBUG_OBJECT (sonyalpha, "Need more data for the header");
  gst_adapter_unmap (sonyalpha->adapter);

  return SONYALPHA_NEED_MORE_DATA;

wrong_header:
  {
    GST_ELEMENT_ERROR (sonyalpha, STREAM, DEMUX, (NULL),
        ("Boundary not found in the sonyalpha header"));
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

  sonyalpha = GST_SONYALPHA_DEMUX (parent);
  adapter = sonyalpha->adapter;

  res = GST_FLOW_OK;

  if (GST_BUFFER_FLAG_IS_SET (buf, GST_BUFFER_FLAG_DISCONT)) {
    GSList *l;

    for (l = sonyalpha->srcpads; l != NULL; l = l->next) {
      GstSonyAlphaPad *srcpad = l->data;

      srcpad->discont = TRUE;
    }
    gst_adapter_clear (adapter);
  }
  gst_adapter_push (adapter, buf);

  while (gst_adapter_available (adapter) > 0) {
    GstSonyAlphaPad *srcpad;
    GstBuffer *outbuf;
    gboolean created;
    gint datalen;

    if (G_UNLIKELY (!sonyalpha->header_completed)) {
      if ((size = sonyalpha_parse_header (sonyalpha)) < 0) {
        goto nodata;
      } else {
        gst_adapter_flush (adapter, size);
        sonyalpha->header_completed = TRUE;
      }
    }
    
    /*
    if ((size = sonyalpha_find_boundary (sonyalpha, &datalen)) < 0) {
      goto nodata;
    }*/
    

    /* Invalidate header info */
    sonyalpha->header_completed = FALSE;
    sonyalpha->content_length = -1;

    if (G_UNLIKELY (datalen <= 0)) {
      GST_DEBUG_OBJECT (sonyalpha, "skipping empty content.");
      gst_adapter_flush (adapter, size - datalen);
    } else {
      GstClockTime ts;

      srcpad =
          gst_sonyalpha_find_pad_by_mime (sonyalpha,
          sonyalpha->mime_type, &created);

      ts = gst_adapter_prev_pts (adapter, NULL);
      outbuf = gst_adapter_take_buffer (adapter, datalen);
      gst_adapter_flush (adapter, size - datalen);

      if (created) {
        GstTagList *tags;
        GstSegment segment;

        gst_segment_init (&segment, GST_FORMAT_TIME);

        /* Push new segment, first buffer has 0 timestamp */
        gst_pad_push_event (srcpad->pad, gst_event_new_segment (&segment));

        tags = gst_tag_list_new (GST_TAG_CONTAINER_FORMAT, "SonyAlpha", NULL);
        gst_tag_list_set_scope (tags, GST_TAG_SCOPE_GLOBAL);
        gst_pad_push_event (srcpad->pad, gst_event_new_tag (tags));
      }

      outbuf = gst_buffer_make_writable (outbuf);
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

      GST_DEBUG_OBJECT (sonyalpha,
          "pushing buffer with timestamp %" GST_TIME_FORMAT,
          GST_TIME_ARGS (GST_BUFFER_TIMESTAMP (outbuf)));
      res = gst_pad_push (srcpad->pad, outbuf);
      res = gst_sonyalpha_combine_flows (sonyalpha, srcpad, res);
      if (res != GST_FLOW_OK)
        break;
    }
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
      sonyalpha->content_length = -1;
      sonyalpha->scanpos = 0;
      gst_sonyalpha_demux_remove_src_pads (sonyalpha);
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



gboolean
gst_sonyalpha_demux_plugin_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_sonyalpha_demux_debug,
      "sonyalphademux", 0, "sonyalpha demuxer");

  return gst_element_register (plugin, "sonyalphademux", GST_RANK_PRIMARY,
      GST_TYPE_SONYALPHA_DEMUX);
}
