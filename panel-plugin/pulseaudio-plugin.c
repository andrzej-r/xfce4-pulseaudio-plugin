/*  Copyright (c) 2014 Andrzej <ndrwrdck@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */



/*
 *  This file implements the main plugin class.
 *
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "pulseaudio-plugin.h"
#include "pulseaudio-volume.h"
#include "pulseaudio-button.h"

#ifdef HAVE_IDO
#include <libido/libido.h>
#endif

/* prototypes */
static void             pulseaudio_plugin_construct                        (XfcePanelPlugin       *plugin);
static void             pulseaudio_plugin_free                             (XfcePanelPlugin       *plugin);
static void             pulseaudio_plugin_show_about                       (XfcePanelPlugin       *plugin);
static void             pulseaudio_plugin_configure_plugin                 (XfcePanelPlugin       *plugin);
static gboolean         pulseaudio_plugin_size_changed                     (XfcePanelPlugin       *plugin,
                                                                            gint                   size);


struct _PulseaudioPluginClass
{
  XfcePanelPluginClass __parent__;
};

/* plugin structure */
struct _PulseaudioPlugin
{
  XfcePanelPlugin      __parent__;

  PulseaudioVolume    *volume;

  /* panel widgets */
  GtkWidget           *button;

  /* log file */
  FILE                *logfile;
};


/* define the plugin */
XFCE_PANEL_DEFINE_PLUGIN (PulseaudioPlugin, pulseaudio_plugin)




static void
pulseaudio_plugin_class_init (PulseaudioPluginClass *klass)
{
  XfcePanelPluginClass *plugin_class;

  plugin_class = XFCE_PANEL_PLUGIN_CLASS (klass);
  plugin_class->construct = pulseaudio_plugin_construct;
  plugin_class->free_data = pulseaudio_plugin_free;
  plugin_class->about = pulseaudio_plugin_show_about;
  plugin_class->configure_plugin = pulseaudio_plugin_configure_plugin;
  plugin_class->size_changed = pulseaudio_plugin_size_changed;
}



static void
pulseaudio_plugin_init (PulseaudioPlugin *pulseaudio_plugin)
{
  g_log_set_always_fatal (G_LOG_LEVEL_ERROR);

  pulseaudio_plugin->volume          = NULL;
  pulseaudio_plugin->button          = NULL;
  pulseaudio_plugin->logfile         = NULL;
}



static void
pulseaudio_plugin_free (XfcePanelPlugin *plugin)
{
}



static void
pulseaudio_plugin_show_about (XfcePanelPlugin *plugin)
{
   GdkPixbuf *icon;

   const gchar *auth[] =
     {
       "Andrzej Radecki <ndrwrdck@gmail.com>",
       NULL
     };

   g_return_if_fail (IS_PULSEAUDIO_PLUGIN (plugin));

   icon = xfce_panel_pixbuf_from_source ("xfce4-pulseaudio-plugin", NULL, 32);
   gtk_show_about_dialog (NULL,
                          "logo", icon,
                          "license", xfce_get_license_text (XFCE_LICENSE_TEXT_GPL),
                          "version", PACKAGE_VERSION,
                          "program-name", PACKAGE_NAME,
                          "comments", _("A panel plugin for controlling PulseAudio mixer."),
                          "website", "http://goodies.xfce.org/projects/panel-plugins/xfce4-pulseaudio-plugin",
                          "copyright", _("Copyright (c) 2014\n"),
                          "authors", auth, NULL);

   if (icon)
     g_object_unref (G_OBJECT (icon));
}



static void
pulseaudio_plugin_configure_plugin (XfcePanelPlugin *plugin)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN (plugin);

  //pulseaudio_dialog_show (pulseaudio_plugin->dialog, gtk_widget_get_screen (GTK_WIDGET (plugin)));
}




static void
pulseaudio_plugin_log_handler (const gchar    *domain,
                               GLogLevelFlags  level,
                               const gchar    *message,
                               gpointer        data)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN (data);
  gchar            *path;
  const gchar      *prefix;

  if (pulseaudio_plugin->logfile == NULL)
    {
      g_mkdir_with_parents (g_get_user_cache_dir (), 0755);
      path = g_build_filename (g_get_user_cache_dir (), "xfce4-pulseaudio-plugin.log", NULL);
      pulseaudio_plugin->logfile = fopen (path, "w");
      g_free (path);
    }

  if (pulseaudio_plugin->logfile)
    {
      switch (level & G_LOG_LEVEL_MASK)
        {
        case G_LOG_LEVEL_ERROR:    prefix = "ERROR";    break;
        case G_LOG_LEVEL_CRITICAL: prefix = "CRITICAL"; break;
        case G_LOG_LEVEL_WARNING:  prefix = "WARNING";  break;
        case G_LOG_LEVEL_MESSAGE:  prefix = "MESSAGE";  break;
        case G_LOG_LEVEL_INFO:     prefix = "INFO";     break;
        case G_LOG_LEVEL_DEBUG:    prefix = "DEBUG";    break;
        default:                   prefix = "LOG";      break;
        }

      fprintf (pulseaudio_plugin->logfile, "%-10s %-25s %s\n", prefix, domain, message);
      fflush (pulseaudio_plugin->logfile);
    }

  /* print log to the stdout */
  if (level & G_LOG_LEVEL_ERROR || level & G_LOG_LEVEL_CRITICAL)
    g_log_default_handler (domain, level, message, NULL);
}



static gboolean
pulseaudio_plugin_size_changed (XfcePanelPlugin *plugin,
                                gint             size)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN (plugin);

  /* The plugin only occupies a single row */
  size /= xfce_panel_plugin_get_nrows (plugin);

  pulseaudio_button_set_size (PULSEAUDIO_BUTTON (pulseaudio_plugin->button), size);

  return TRUE;
}



static void
pulseaudio_plugin_construct (XfcePanelPlugin *plugin)
{
  PulseaudioPlugin *pulseaudio_plugin = PULSEAUDIO_PLUGIN (plugin);

  #ifdef HAVE_IDO
  ido_init();
  #endif

  xfce_panel_plugin_menu_show_configure (plugin);
  xfce_panel_plugin_menu_show_about (plugin);

  xfce_panel_plugin_set_small (plugin, TRUE);

  /* setup transation domain */
  xfce_textdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR, "UTF-8");

  /* log messages to a file */
  g_log_set_default_handler (pulseaudio_plugin_log_handler, plugin);

  /* initialize xfconf */
  //pulseaudio_plugin->config = pulseaudio_config_new (xfce_panel_plugin_get_property_base (plugin));

  /* instantiate preference dialog builder */
  //pulseaudio_plugin->dialog = pulseaudio_dialog_new (pulseaudio_plugin->config);

  /* volume controller */
  pulseaudio_plugin->volume = pulseaudio_volume_new ();

  /* instantiate a button box */
  pulseaudio_plugin->button = pulseaudio_button_new (pulseaudio_plugin->volume);
  gtk_container_add (GTK_CONTAINER (plugin), GTK_WIDGET (pulseaudio_plugin->button));
  gtk_widget_show (GTK_WIDGET (pulseaudio_plugin->button));
}
