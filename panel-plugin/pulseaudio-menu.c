/*  Copyright (c) 2015 Andrzej <ndrwrdck@gmail.com>
 *  Copyright (c) 2015 Simon Steinbeiss <ochosi@xfce.org>
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */



/*
 *  This file implements a plugin menu
 *
 */



#include <glib.h>
#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>
#include <libxfce4util/libxfce4util.h>

#include "pulseaudio-menu.h"
#include "scalemenuitem.h"


struct _PulseaudioMenu
{
  GtkMenu             __parent__;

  PulseaudioVolume     *volume;
  PulseaudioConfig     *config;
  GtkWidget            *button;
  GtkWidget            *range_output;
  GtkWidget            *mute_output_item;

  gulong                volume_changed_id;
};

struct _PulseaudioMenuClass
{
  GtkMenuClass        __parent__;
};


static void             pulseaudio_menu_finalize         (GObject       *object);


G_DEFINE_TYPE (PulseaudioMenu, pulseaudio_menu, GTK_TYPE_MENU)

static void
pulseaudio_menu_class_init (PulseaudioMenuClass *klass)
{
  GObjectClass      *gobject_class;
  GtkWidgetClass    *gtkwidget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = pulseaudio_menu_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
}



static void
pulseaudio_menu_init (PulseaudioMenu *menu)
{
  menu->volume                         = NULL;
  menu->button                         = NULL;
  menu->range_output                   = NULL;
  menu->mute_output_item               = NULL;
}


static void
pulseaudio_menu_finalize (GObject *object)
{
  PulseaudioMenu *menu;

  menu = PULSEAUDIO_MENU (object);

  G_OBJECT_CLASS (pulseaudio_menu_parent_class)->finalize (object);
}


static void
pulseaudio_menu_output_range_scroll (GtkWidget        *widget,
                                     GdkEvent         *event,
                                     PulseaudioMenu   *menu)
{
  gdouble         new_volume;
  gdouble         volume;
  gdouble         volume_step;
  GdkEventScroll *scroll_event;

  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));
  volume =  pulseaudio_volume_get_volume (menu->volume);
  volume_step = pulseaudio_config_get_volume_step (menu->config) / 100.0;

  scroll_event = (GdkEventScroll*)event;

  new_volume = MIN (MAX (volume + (1.0 - 2.0 * scroll_event->direction) * volume_step, 0.0), 1.0);
  pulseaudio_volume_set_volume (menu->volume, new_volume);
  //printf ("scroll %d %g %g\n", scroll_event->direction, volume, new_volume);
}

static void
pulseaudio_menu_output_range_value_changed (PulseaudioMenu   *menu,
                                            GtkWidget        *widget)
{
  gdouble  new_volume;

  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));

  new_volume = gtk_range_get_value (GTK_RANGE (menu->range_output)) / 100.0;
  pulseaudio_volume_set_volume (menu->volume, new_volume);
  //printf ("range value changed %g\n", new_volume);
}


static void
pulseaudio_menu_mute_output_item_toggled (PulseaudioMenu   *menu,
                                          GtkCheckMenuItem *menu_item)
{
  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));

  pulseaudio_volume_set_muted (menu->volume, gtk_check_menu_item_get_active (menu_item));
}



static void
pulseaudio_menu_run_audio_mixer (PulseaudioMenu   *menu,
                                 GtkCheckMenuItem *menu_item)
{
  GError    *error = NULL;
  GtkWidget *message_dialog;

  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));

  if (!xfce_spawn_command_line_on_screen (gtk_widget_get_screen (GTK_WIDGET (menu)),
                                          pulseaudio_config_get_mixer_command (menu->config),
                                          FALSE, FALSE, &error))
    {
      message_dialog = gtk_message_dialog_new_with_markup (NULL,
                                                           GTK_DIALOG_DESTROY_WITH_PARENT,
                                                           GTK_MESSAGE_ERROR,
                                                           GTK_BUTTONS_CLOSE,
                                                           _("<big><b>Failed to execute command \"%s\".</b></big>\n\n%s"),
                                                           pulseaudio_config_get_mixer_command (menu->config),
                                                           error->message);
      gtk_window_set_title (GTK_WINDOW (message_dialog), _("Error"));
      gtk_dialog_run (GTK_DIALOG (message_dialog));
      gtk_widget_destroy (message_dialog);
      //xfce_dialog_show_error (NULL, error, _("Failed to execute command \"%s\"."),
      //                        pulseaudio_config_get_mixer_command (menu->config));
      g_error_free (error);
    }
}



static void
pulseaudio_menu_volume_changed (PulseaudioMenu   *menu,
                                PulseaudioVolume *volume)
{
  g_return_if_fail (IS_PULSEAUDIO_MENU (menu));
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));

  g_signal_handlers_block_by_func (G_OBJECT (menu->mute_output_item),
                                   pulseaudio_menu_mute_output_item_toggled,
                                   menu);
  gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (menu->mute_output_item),
                                  pulseaudio_volume_get_muted (volume));
  g_signal_handlers_unblock_by_func (G_OBJECT (menu->mute_output_item),
                                     pulseaudio_menu_mute_output_item_toggled,
                                     menu);

  gtk_range_set_value (GTK_RANGE (menu->range_output), pulseaudio_volume_get_volume (menu->volume) * 100.0);
}



GtkWidget *
pulseaudio_menu_new (PulseaudioVolume *volume,
                     PulseaudioConfig *config,
                     GtkWidget        *widget)
{
  PulseaudioMenu *menu;
  GdkScreen      *gscreen;
  GtkWidget      *mi;
  GtkWidget      *img = NULL;
  GdkPixbuf      *pix;
  GtkIconInfo    *info;
  GtkStyleContext *context;

  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), NULL);
  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), NULL);
  g_return_val_if_fail (GTK_IS_WIDGET (widget), NULL);

  if (gtk_widget_has_screen (widget))
    gscreen = gtk_widget_get_screen (widget);
  else
    gscreen = gdk_display_get_default_screen (gdk_display_get_default ());


  menu = g_object_new (TYPE_PULSEAUDIO_MENU, NULL);
  gtk_menu_set_screen (GTK_MENU (menu), gscreen);

  menu->volume = volume;
  menu->config = config;
  menu->button = widget;
  menu->volume_changed_id =
    g_signal_connect_swapped (G_OBJECT (menu->volume), "volume-changed",
                              G_CALLBACK (pulseaudio_menu_volume_changed), menu);

  /* output volume slider */
  mi = scale_menu_item_new_with_range (0.0, 100.0, 1.0);

  /* attempt to load and display the brightness icon */
  context = gtk_widget_get_style_context (GTK_WIDGET (mi));
  info = gtk_icon_theme_lookup_icon (gtk_icon_theme_get_default (), "audio-volume-high-symbolic", 24, GTK_ICON_LOOKUP_GENERIC_FALLBACK);
  pix = gtk_icon_info_load_symbolic_for_context (info, context, NULL, NULL);
  if (pix)
    {
      img = gtk_image_new_from_pixbuf (pix);
      gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), img);
    }

  scale_menu_item_set_description_label (SCALE_MENU_ITEM (mi), _("<b>Audio output volume</b>"));

  /* range slider */
  menu->range_output = scale_menu_item_get_scale (SCALE_MENU_ITEM (mi));

  /* update the slider to the current brightness level */
  //gtk_range_set_value (GTK_RANGE (menu->range_output), current_level);

  g_signal_connect_swapped (mi, "value-changed", G_CALLBACK (pulseaudio_menu_output_range_value_changed), menu);
  g_signal_connect (mi, "scroll-event", G_CALLBACK (pulseaudio_menu_output_range_scroll), menu);

  gtk_widget_show_all (mi);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), mi);

  menu->mute_output_item = gtk_check_menu_item_new_with_mnemonic (_("_Mute audio output"));
  gtk_widget_show_all (menu->mute_output_item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), menu->mute_output_item);
  g_signal_connect_swapped (G_OBJECT (menu->mute_output_item), "toggled", G_CALLBACK (pulseaudio_menu_mute_output_item_toggled), menu);

  /* separator */
  mi = gtk_separator_menu_item_new ();
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);

  /* Audio mixers */
  mi = gtk_menu_item_new_with_mnemonic (_("_Run audio mixer..."));
  gtk_widget_show (mi);
  gtk_menu_shell_append (GTK_MENU_SHELL (menu), mi);
  g_signal_connect_swapped (G_OBJECT (mi), "activate", G_CALLBACK (pulseaudio_menu_run_audio_mixer), menu);

  pulseaudio_menu_volume_changed (menu, menu->volume);


  return GTK_WIDGET (menu);
}


