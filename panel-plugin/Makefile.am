
AM_CPPFLAGS = \
	-I$(top_srcdir) \
	-DG_LOG_DOMAIN=\"pulseaudio-plugin\" \
	-DPACKAGE_LOCALE_DIR=\"$(localedir)\" \
	-DDEFAULT_MIXER_COMMAND=\"$(DEFAULT_MIXER_COMMAND)\" \
	-DDATADIR=\""$(datadir)"\" \
	$(PLATFORM_CPPFLAGS)

#
# PulseAudio plugin
#
plugindir = \
	$(libdir)/xfce4/panel/plugins

plugin_LTLIBRARIES = \
	libpulseaudio-plugin.la

libpulseaudio_built_sources = \
	pulseaudio-dialog_ui.h

libpulseaudio_plugin_la_SOURCES = \
	$(libpulseaudio_built_sources) \
	pulseaudio-debug.c \
	pulseaudio-debug.h \
	pulseaudio-volume.c \
	pulseaudio-volume.h \
	pulseaudio-button.c \
	pulseaudio-button.h \
	pulseaudio-config.c \
	pulseaudio-config.h \
	pulseaudio-plugin.c \
	pulseaudio-plugin.h \
	pulseaudio-dialog.c \
	pulseaudio-dialog.h \
	pulseaudio-menu.c \
	pulseaudio-menu.h \
	scalemenuitem.c \
	scalemenuitem.h


libpulseaudio_plugin_la_CFLAGS = \
	$(PULSEAUDIO_CFLAGS) \
	$(GLIB_CFLAGS) \
	$(GTK_CFLAGS) \
	$(LIBXFCE4UTIL_CFLAGS) \
	$(LIBXFCE4UI_CFLAGS) \
	$(LIBXFCE4PANEL_CFLAGS) \
	$(XFCONF_CFLAGS) \
	$(KEYBINDER_CFLAGS) \
	$(PLATFORM_CFLAGS)

libpulseaudio_plugin_la_LDFLAGS = \
	-avoid-version \
	-module \
	-no-undefined \
	-export-symbols-regex '^xfce_panel_module_(preinit|init|construct)' \
	$(PLATFORM_LDFLAGS)

libpulseaudio_plugin_la_LIBADD = \
	$(PULSEAUDIO_LIBS) \
	$(GLIB_LIBS) \
	$(GTK_LIBS) \
	$(LIBXFCE4UTIL_LIBS) \
	$(LIBXFCE4UI_LIBS) \
	$(LIBXFCE4PANEL_LIBS) \
	$(XFCONF_LIBS) \
	$(KEYBINDER_LIBS) \
	$(LIBM)

#
# Desktop file
#
desktopdir = \
	$(datadir)/xfce4/panel/plugins

desktop_in_files = \
	pulseaudio.desktop.in.in

desktop_DATA = $(desktop_in_files:.desktop.in.in=.desktop)

@INTLTOOL_DESKTOP_RULE@

EXTRA_DIST = \
	pulseaudio-dialog.glade \
	$(desktop_in_files)

DISTCLEANFILES = \
	$(desktop_DATA)

if MAINTAINER_MODE
BUILT_SOURCES = \
	$(libpulseaudio_built_sources)

DISTCLEANFILES += \
	$(libpulseaudio_built_sources)

pulseaudio-dialog_ui.h: pulseaudio-dialog.glade
	$(AM_V_GEN) xdt-csource --static --strip-comments --strip-content --name=pulseaudio_dialog_ui $< >$@
endif

# vi:set ts=8 sw=8 noet ai nocindent syntax=automake:
