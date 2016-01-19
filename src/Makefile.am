# Note: plugindir is set in configure
plugin_LTLIBRARIES = libgstsonyalpha.la

# sources used to compile this plug-in
libgstsonyalpha_la_SOURCES = sonyalphademux.c sonyalphademux.h plugin.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstsonyalpha_la_CFLAGS = $(GST_CFLAGS)
libgstsonyalpha_la_LIBADD = $(GST_LIBS)
libgstsonyalpha_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstsonyalpha_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = sonyalphademux.h
