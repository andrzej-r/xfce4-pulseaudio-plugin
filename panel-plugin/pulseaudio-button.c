/*  Copyright (c) 2014-2015 Andrzej <ndrwrdck@gmail.com>
 *  Copyright (c) 2015      Simon Steinbeiss <ochosi@xfce.org>
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
 *  This file implements a pulseaudio button class controlling
 *  and displaying pulseaudio volume.
 *
 */



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_MATH_H
#include <math.h>
#endif


#include <glib.h>
#include <gtk/gtk.h>
#include <libxfce4util/libxfce4util.h>
#include <libxfce4panel/libxfce4panel.h>

#include "pulseaudio-plugin.h"
#include "pulseaudio-config.h"
#include "pulseaudio-menu.h"
#include "pulseaudio-button.h"

#define V_MUTED  0
#define V_LOW    1
#define V_MEDIUM 2
#define V_HIGH   3


/* Icons for different volume levels */
static const char *icons[] = {
  "audio-volume-muted-symbolic",
  "audio-volume-low-symbolic",
  "audio-volume-medium-symbolic",
  "audio-volume-high-symbolic",
  NULL
};



static void                 pulseaudio_button_finalize        (GObject            *object);
static gboolean             pulseaudio_button_button_press    (GtkWidget          *widget,
                                                               GdkEventButton     *event);
static gboolean             pulseaudio_button_scroll_event    (GtkWidget          *widget,
                                                               GdkEventScroll     *event);
static void                 pulseaudio_button_menu_deactivate (PulseaudioButton   *button,
                                                               GtkMenuShell       *menu);
static void                 pulseaudio_button_update_icons    (PulseaudioButton   *button);
static void                 pulseaudio_button_update          (PulseaudioButton   *button,
                                                               gboolean            force_update);


struct _PulseaudioButton
{
  GtkToggleButton       __parent__;

  PulseaudioPlugin     *plugin;
  PulseaudioConfig     *config;
  PulseaudioVolume     *volume;

  GtkWidget            *image;

  /* Icon size currently used */
  gint                  icon_size;
  const gchar          *icon_name;

  GtkWidget            *menu;

  gulong                volume_changed_id;
  gulong                deactivate_id;
};

struct _PulseaudioButtonClass
{
  GtkToggleButtonClass __parent__;
};




G_DEFINE_TYPE (PulseaudioButton, pulseaudio_button, GTK_TYPE_TOGGLE_BUTTON)

static void
pulseaudio_button_class_init (PulseaudioButtonClass *klass)
{
  GObjectClass      *gobject_class;
  GtkWidgetClass    *gtkwidget_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = pulseaudio_button_finalize;

  gtkwidget_class = GTK_WIDGET_CLASS (klass);
  gtkwidget_class->button_press_event   = pulseaudio_button_button_press;
  gtkwidget_class->scroll_event         = pulseaudio_button_scroll_event;
}



static void
pulseaudio_button_init (PulseaudioButton *button)
{
  GtkCssProvider *css_provider;

  gtk_widget_set_can_focus(GTK_WIDGET(button), FALSE);
  gtk_widget_set_can_default (GTK_WIDGET (button), FALSE);
  gtk_button_set_relief (GTK_BUTTON (button), GTK_RELIEF_NONE);
  gtk_button_set_use_underline (GTK_BUTTON (button),TRUE);
  gtk_button_set_focus_on_click (GTK_BUTTON (button), FALSE);
  gtk_widget_set_name (GTK_WIDGET (button), "pulseaudio-button");

  /* Preload icons */
  g_signal_connect (G_OBJECT (button), "style-updated", G_CALLBACK (pulseaudio_button_update_icons), button);

  /* Setup Gtk style */
  css_provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (css_provider, "#pulseaudio-button { -GtkWidget-focus-padding: 0; -GtkWidget-focus-line-width: 0; -GtkButton-default-border: 0; -GtkButton-inner-border: 0; padding: 1px; border-width: 1px;}", -1, NULL);
  gtk_style_context_add_provider (GTK_STYLE_CONTEXT (gtk_widget_get_style_context (GTK_WIDGET (button))), GTK_STYLE_PROVIDER (css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  /* Intercept scroll events */
  gtk_widget_add_events (GTK_WIDGET (button), GDK_SCROLL_MASK);

  button->plugin = NULL;
  button->config = NULL;
  button->volume = NULL;
  button->icon_size = 16;
  button->icon_name = NULL;

  button->menu = NULL;
  button->volume_changed_id = 0;
  button->deactivate_id = 0;

  button->image = gtk_image_new ();
  gtk_container_add (GTK_CONTAINER (button), button->image);
  gtk_widget_show (button->image);

  g_object_set (G_OBJECT (button), "has-tooltip", TRUE, NULL);

  //button->align_box = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  //gtk_container_add (GTK_CONTAINER (button), button->align_box);
  //gtk_widget_show (button->align_box);
}



static void
pulseaudio_button_finalize (GObject *object)
{
  PulseaudioButton *button = PULSEAUDIO_BUTTON (object);

  if (button->menu != NULL)
    {
      gtk_menu_detach (GTK_MENU (button->menu));
      gtk_menu_popdown (GTK_MENU (button->menu));
      button->menu = NULL;
    }

  (*G_OBJECT_CLASS (pulseaudio_button_parent_class)->finalize) (object);
}


static gboolean
pulseaudio_button_button_press (GtkWidget      *widget,
                                GdkEventButton *event)
{
  PulseaudioButton *button = PULSEAUDIO_BUTTON (widget);

  if(event->button == 1 && button->menu == NULL) /* left button */
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
      button->menu = pulseaudio_menu_new (button->volume, button->config, widget);
      gtk_menu_attach_to_widget (GTK_MENU (button->menu), widget, NULL);

      if (button->deactivate_id == 0)
        {
          button->deactivate_id = g_signal_connect_swapped
            (GTK_MENU_SHELL (button->menu), "deactivate",
             G_CALLBACK (pulseaudio_button_menu_deactivate), button);
        }

      gtk_menu_popup (GTK_MENU (button->menu),
                      NULL, NULL,
                      xfce_panel_plugin_position_menu, button->plugin,
                      //NULL, NULL,
                      1,
                      event->time);
      return TRUE;
    }

  if(event->button == 2) /* middle button */
    {
      pulseaudio_volume_toggle_muted (button->volume);
      return TRUE;
    }

  return FALSE;
}


static gboolean
pulseaudio_button_scroll_event (GtkWidget *widget, GdkEventScroll *event)
{
  PulseaudioButton *button      = PULSEAUDIO_BUTTON (widget);
  gdouble           volume      = pulseaudio_volume_get_volume (button->volume);
  gdouble           volume_step = pulseaudio_config_get_volume_step (button->config) / 100.0;
  gdouble           new_volume;


  if (event->direction == 1) // decrease volume
    new_volume = volume - volume_step;
  else if (event->direction == 0) // increase volume
    new_volume = MIN (volume + volume_step, MAX (volume, 1.0));
  else
    new_volume = volume;

  pulseaudio_volume_set_volume (button->volume, new_volume);
  //g_debug ("dir: %d %f -> %f", event->direction, volume, new_volume);

  return TRUE;
}


static void
pulseaudio_button_menu_deactivate (PulseaudioButton *button,
                                   GtkMenuShell     *menu)
{
  g_return_if_fail (IS_PULSEAUDIO_BUTTON (button));
  g_return_if_fail (GTK_IS_MENU_SHELL (menu));

  if (button->deactivate_id)
    {
      g_signal_handler_disconnect (menu, button->deactivate_id);
      button->deactivate_id = 0;
    }

  if (button->menu != NULL)
    {
      gtk_menu_detach (GTK_MENU (button->menu));
      gtk_menu_popdown (GTK_MENU (button->menu));
      button->menu = NULL;
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
}


static void
pulseaudio_button_update_icons (PulseaudioButton *button)
{
  /* Update the state of the button */
  pulseaudio_button_update (button, TRUE);
}


static void
pulseaudio_button_update (PulseaudioButton *button,
                          gboolean          force_update)
{
  gdouble      volume;
  gboolean     muted;
  gchar       *tip_text;
  const gchar *icon_name;

  g_return_if_fail (IS_PULSEAUDIO_BUTTON (button));
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (button->volume));

  volume = pulseaudio_volume_get_volume (button->volume);
  muted = pulseaudio_volume_get_muted (button->volume);
  if (muted)
    icon_name = icons[V_MUTED];
  else if (volume <= 0.0)
    icon_name = icons[V_MUTED];
  else if (volume <= 0.3)
    icon_name = icons[V_LOW];
  else if (volume <= 0.7)
    icon_name = icons[V_MEDIUM];
  else
    icon_name = icons[V_HIGH];

  if (muted)
    tip_text = g_strdup_printf (_("Volume %d%% (muted)"), (gint) round (volume * 100));
  else
    tip_text = g_strdup_printf (_("Volume %d%%"), (gint) round (volume * 100));
  gtk_widget_set_tooltip_text (GTK_WIDGET (button), tip_text);
  g_free (tip_text);

  if (force_update || icon_name != button->icon_name)
    {
      button->icon_name = icon_name;
      gtk_image_set_from_icon_name (GTK_IMAGE (button->image), icon_name, GTK_ICON_SIZE_BUTTON);
      gtk_image_set_pixel_size (GTK_IMAGE (button->image), button->icon_size);
    }
}


void
pulseaudio_button_set_size (PulseaudioButton *button,
                            gint              size)
{
  GtkStyleContext  *context;
  GtkBorder         padding;
  GtkBorder         border;
  gint              width;
  gint              xthickness;
  gint              ythickness;
  gint              size_old;

  g_return_if_fail (IS_PULSEAUDIO_BUTTON (button));
  g_return_if_fail (size > 0);

  /* Get widget's padding and border to correctly calculate the button's icon size */
  context = gtk_widget_get_style_context (GTK_WIDGET (button));
  gtk_style_context_get_padding (context, gtk_widget_get_state_flags (GTK_WIDGET (button)), &padding);
  gtk_style_context_get_border (context, gtk_widget_get_state_flags (GTK_WIDGET (button)), &border);
  xthickness = padding.left+padding.right+border.left+border.right;
  ythickness = padding.top+padding.bottom+border.top+border.bottom;

  width = size - 2* MAX (xthickness, ythickness);
  /* Since symbolic icons are usually only provided in 16px we
   * try to be clever and use size steps */
  size_old = button->icon_size;
  if (width <= 21)
      button->icon_size = 16;
  else if (width >=22 && width <= 29)
      button->icon_size = 24;
  else if (width >= 30 && width <= 40)
      button->icon_size = 32;
  else
      button->icon_size = width;

  gtk_widget_set_size_request (GTK_WIDGET (button), size, size);
  if (button->icon_size != size_old)
    pulseaudio_button_update_icons (button);
}



static void
pulseaudio_button_volume_changed (PulseaudioButton  *button,
                                  PulseaudioVolume  *volume)
{
  g_return_if_fail (IS_PULSEAUDIO_BUTTON (button));

  pulseaudio_button_update (button, FALSE);
}



GtkWidget *
pulseaudio_button_new (PulseaudioPlugin *plugin,
                       PulseaudioConfig *config,
                       PulseaudioVolume *volume)
{
  PulseaudioButton *button;

  g_return_val_if_fail (IS_PULSEAUDIO_PLUGIN (plugin), NULL);
  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), NULL);
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), NULL);

  button = g_object_new (TYPE_PULSEAUDIO_BUTTON, NULL);

  button->plugin = plugin;
  button->volume = volume;
  button->config = config;
  button->volume_changed_id =
    g_signal_connect_swapped (G_OBJECT (button->volume), "volume-changed",
                              G_CALLBACK (pulseaudio_button_volume_changed), button);

  pulseaudio_button_update (button, TRUE);

  return GTK_WIDGET (button);
}


