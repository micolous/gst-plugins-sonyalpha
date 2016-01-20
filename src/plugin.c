#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gst/gst.h>
#include "sonyalphademux.h"

GST_DEBUG_CATEGORY (gst_sonyalpha_debug);

/* entry point to initialize the plug-in
 * initialize the plug-in itself
 * register the element factories and other features
 */
static gboolean
sonyalpha_init (GstPlugin * plugin)
{
  GST_DEBUG_CATEGORY_INIT (gst_sonyalpha_debug, "sonyalpha",
      0, "Debug for Sony Alpha elements");

  return gst_element_register (plugin, "sonyalphademux", GST_RANK_PRIMARY,
      GST_TYPE_SONYALPHA_DEMUX);
}

/* PACKAGE: this is usually set by autotools depending on some _INIT macro
 * in configure.ac and then written into and defined in config.h, but we can
 * just set it ourselves here in case someone doesn't use autotools to
 * compile this code. GST_PLUGIN_DEFINE needs PACKAGE to be defined.
 */
#ifndef PACKAGE
#define PACKAGE "sonyalpha"
#endif

GST_PLUGIN_DEFINE (
    GST_VERSION_MAJOR,
    GST_VERSION_MINOR,
    sonyalpha,
    "Sony Alpha plugin",
    sonyalpha_init,
    VERSION,
    "LGPL",
    PACKAGE_NAME,
    "https://github.com/micolous/gst-plugins-sonyalpha"
)
