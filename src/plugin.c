#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include "gstdvswitchsrc.h"
#include "gstdvswitchsink.h"

GST_DEBUG_CATEGORY (gst_dvswitch_debug);

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
dvswitch_init (GstPlugin * dvswitch)
{
  GST_DEBUG_CATEGORY_INIT (gst_dvswitch_debug, "dvswitch",
      0, "Debug for dvswitch src/sink elements");

  if (!gst_element_register (dvswitch, "dvswitchsrc", GST_RANK_NONE,
      GST_TYPE_DVSWITCHSRC))
    return FALSE;

  if (!gst_element_register (dvswitch, "dvswitchsink", GST_RANK_NONE,
      GST_TYPE_DVSWITCH_SINK))
    return FALSE;

  return TRUE;
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "dvswitch"
#endif

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    dvswitch,
    "Reads/Writes a DIF/DV stream from/to a DVSwitch server.",
    dvswitch_init,
    VERSION,
    "LGPL",
    PACKAGE_NAME,
    "https://github.com/timvideos/gst-plugins-dvswitch"
)
