/* GStreamer

 *
 * gstsonyalphademux.h: sony alpha livestream demuxer
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

#ifndef __GST_SONYALPHA_DEMUX__
#define __GST_SONYALPHA_DEMUX__

#include <gst/gst.h>
#include <gst/base/gstadapter.h>

#include <string.h>
#include <arpa/inet.h>

G_BEGIN_DECLS

#define GST_TYPE_SONYALPHA_DEMUX (gst_sonyalpha_demux_get_type())
#define GST_SONYALPHA_DEMUX(obj) (G_TYPE_CHECK_INSTANCE_CAST((obj),GST_TYPE_SONYALPHA_DEMUX, GstSonyAlphaDemux))
#define GST_SONYALPHA_DEMUX_CLASS(klass) (G_TYPE_CHECK_CLASS_CAST((klass),GST_TYPE_SONYALPHA_DEMUX, GstSonyAlphaDemux))
#define GST_SONYALPHA_DEMUX_GET_CLASS(obj) (G_TYPE_INSTANCE_GET_CLASS ((obj), GST_TYPE_SONYALPHA_DEMUX, GstSonyAlphaDemuxClass))
#define GST_IS_SONYALPHA_DEMUX(obj) (G_TYPE_CHECK_INSTANCE_TYPE((obj),GST_TYPE_SONYALPHA_DEMUX))
#define GST_IS_SONYALPHA_DEMUX_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE((klass),GST_TYPE_SONYALPHA_DEMUX))

typedef struct _GstSonyAlphaDemux GstSonyAlphaDemux;
typedef struct _GstSonyAlphaDemuxClass GstSonyAlphaDemuxClass;

#define SONYALPHA_NEED_MORE_DATA -1
#define SONYALPHA_DATA_ERROR     -2
#define SONYALPHA_DATA_EOS       -3

/* all information needed for one sonyalpha stream */
typedef struct
{
  GstPad *pad;                  /* reference for this pad is held by element we belong to */

  gchar *mime;
  
  GstClockTime  last_ts;        /* last timestamp to make sure we don't send
                                 * two buffers with the same timestamp */
  GstFlowReturn last_ret;

  gboolean      discont;
  
}
GstSonyAlphaPad;


typedef struct
{
  uint8_t payload_type;
  uint16_t sequence_number;
  uint32_t timestamp;
  uint32_t payload_size;
  uint8_t padding_size;
}
GstSonyAlphaPayloadHeader;
  

/**
 * GstSonyAlphaDemux:
 *
 * The opaque #GstSonyAlphaDemux structure.
 */
struct _GstSonyAlphaDemux
{
  GstElement element;

  /* pad */
  GstPad *sinkpad;
  GstPad *srcpad;

  GstAdapter *adapter;

  /* Header information of the current frame */
  GstSonyAlphaPayloadHeader header;
  gboolean header_completed;

  /* Index inside the current data when manually looking for the boundary */
  gint scanpos;

  gboolean singleStream;
  uint32_t first_timestamp;     /* This is the first frame timestamp which we
                                   see in the stream */

};

struct _GstSonyAlphaDemuxClass
{
  GstElementClass parent_class;

  GHashTable *gstnames;
};

GType gst_sonyalpha_demux_get_type (void);

gboolean gst_sonyalpha_demux_plugin_init (GstPlugin * plugin);

G_END_DECLS

#endif /* __GST_SONYALPHA_DEMUX__ */

