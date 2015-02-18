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
#include <libxfce4panel/libxfce4panel.h>

#include "pulseaudio-plugin.h"
#include "pulseaudio-config.h"
#include "pulseaudio-button.h"

#define V_MUTED  0
#define V_LOW    1
#define V_MEDIUM 2
#define V_HIGH   3


/* Icons for different volume levels */
static const char *icons[] = {
  "audio-volume-muted",
  "audio-volume-low",
  "audio-volume-medium",
  "audio-volume-high",
  NULL
};



static void                 pulseaudio_button_finalize        (GObject            *object);
static gboolean             pulseaudio_button_button_press    (GtkWidget          *widget,
                                                               GdkEventButton     *event);
static gboolean             pulseaudio_button_button_release  (GtkWidget          *widget,
                                                               GdkEventButton     *event);
static gboolean             pulseaudio_button_scroll_event    (GtkWidget          *widget,
                                                               GdkEventScroll     *event);
static void                 pulseaudio_button_menu_deactivate (PulseaudioButton   *button,
                                                               GtkMenu            *menu);
static void                 pulseaudio_button_update_icons    (PulseaudioButton   *button);
static void                 pulseaudio_button_update          (PulseaudioButton   *button,
                                                               gboolean            force_update);


struct _PulseaudioButton
{
  GtkToggleButton       __parent__;

  PulseaudioConfig     *config;
  PulseaudioVolume     *volume;

  GtkWidget            *image;

  /* Icon size currently used */
  gint                  icon_size;
  gint                  size;

  /* Array of preloaded icons */
  guint                 pixbuf_idx;
  GdkPixbuf           **pixbufs;

  GtkMenu              *menu;

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
  gtkwidget_class->button_release_event = pulseaudio_button_button_release;
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
  button->pixbuf_idx = 0;
  button->pixbufs = g_new0 (GdkPixbuf*, G_N_ELEMENTS (icons)-1);
  pulseaudio_button_update_icons (button);

  /* Setup Gtk style */
  css_provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (css_provider, "#pulseaudio-button { -GtkWidget-focus-padding: 0; -GtkWidget-focus-line-width: 0; -GtkButton-default-border: 0; -GtkButton-inner-border: 0; padding: 1px; border-width: 1px;}", -1, NULL);
  gtk_style_context_add_provider (GTK_STYLE_CONTEXT (gtk_widget_get_style_context (GTK_WIDGET (button))), GTK_STYLE_PROVIDER (css_provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);

  /* Intercept scroll events */
  gtk_widget_add_events (GTK_WIDGET (button), GDK_SCROLL_MASK);

  button->config = NULL;
  button->volume = NULL;

  button->menu = NULL;
  button->volume_changed_id = 0;
  button->deactivate_id = 0;

  button->image = xfce_panel_image_new ();
  gtk_container_add (GTK_CONTAINER (button), button->image);
  gtk_widget_show (button->image);

  g_object_set (G_OBJECT (button), "has-tooltip", TRUE, NULL);

  pulseaudio_button_update (button, TRUE);

  //button->align_box = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
  //gtk_container_add (GTK_CONTAINER (button), button->align_box);
  //gtk_widget_show (button->align_box);
}



static void
pulseaudio_button_finalize (GObject *object)
{
  PulseaudioButton *button = PULSEAUDIO_BUTTON (object);
  guint             i;

  /* Free pre-allocated icon pixbufs */
  for (i = 0; i < G_N_ELEMENTS (icons)-1; ++i)
    if (GDK_IS_PIXBUF (button->pixbufs[i]))
      g_object_unref (G_OBJECT (button->pixbufs[i]));
  g_free (button->pixbufs);

  if (button->menu != NULL)
    {
      gtk_menu_detach (button->menu);
      gtk_menu_popdown (button->menu);
      button->menu = NULL;
    }

  (*G_OBJECT_CLASS (pulseaudio_button_parent_class)->finalize) (object);
}


static gboolean
pulseaudio_button_button_press (GtkWidget      *widget,
                                GdkEventButton *event)
{
  PulseaudioButton *button = PULSEAUDIO_BUTTON (widget);

  if(event->button == 1) /* left button */
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
      return TRUE;
    }

  if(event->button == 2) /* middle button */
    {
      gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), TRUE);
      return TRUE;
    }

  return FALSE;
}


static gboolean
pulseaudio_button_button_release (GtkWidget      *widget,
                                  GdkEventButton *event)
{
  PulseaudioButton *button = PULSEAUDIO_BUTTON (widget);
  GError *error = NULL;

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), FALSE);

  if(event->button == 1) /* left button */
    {
      if (!xfce_spawn_command_line_on_screen (gtk_widget_get_screen (widget),
                                              pulseaudio_config_get_mixer_name (button->config),
                                              FALSE, FALSE, &error))
        {
          xfce_dialog_show_error (NULL, error, ("Failed to execute command \"%s\"."),
                                  pulseaudio_config_get_mixer_name (button->config));
          g_error_free (error);
        }
      return TRUE;
    }

  if (event->button == 2) /* middle button */
    {
      pulseaudio_volume_toggle_muted (button->volume);
      return TRUE;
    }

  return FALSE;
}


static gboolean
pulseaudio_button_scroll_event (GtkWidget *widget, GdkEventScroll *event)
{
  PulseaudioButton *button = PULSEAUDIO_BUTTON (widget);
  gdouble volume =  pulseaudio_volume_get_volume (button->volume);
  gdouble volume_step = pulseaudio_config_get_volume_step (button->config) / 100.0;
  gdouble new_volume;


  new_volume = MIN (MAX (volume + (1.0 - 2.0 * event->direction) * volume_step, 0.0), 1.0);
  pulseaudio_volume_set_volume (button->volume, new_volume);
  //g_debug ("dir: %d %f -> %f", event->direction, volume, new_volume);

  return TRUE;
}


static void
pulseaudio_button_menu_deactivate (PulseaudioButton *button,
                                   GtkMenu          *menu)
{
  g_return_if_fail (IS_PULSEAUDIO_BUTTON (button));
  g_return_if_fail (GTK_IS_MENU (menu));

  if (button->deactivate_id)
    {
      g_signal_handler_disconnect (menu, button->deactivate_id);
      button->deactivate_id = 0;
    }

  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (button), FALSE);
}


static void
pulseaudio_button_update_icons (PulseaudioButton *button)
{
  GtkIconTheme   *icon_theme;
  guint           i;

  g_return_if_fail (IS_PULSEAUDIO_BUTTON (button));

  icon_theme = gtk_icon_theme_get_for_screen (gtk_widget_get_screen (GTK_WIDGET (button)));

  /* Pre-load all icons */
  for (i = 0; i < G_N_ELEMENTS (icons)-1; ++i)
    {
      if (GDK_IS_PIXBUF (button->pixbufs[i]))
        g_object_unref (G_OBJECT (button->pixbufs[i]));

      button->pixbufs[i] = gtk_icon_theme_load_icon (icon_theme,
                                                     icons[i],
                                                     button->icon_size,
                                                     GTK_ICON_LOOKUP_USE_BUILTIN,
                                                     NULL);
    }

  /* Update the state of the button */
  pulseaudio_button_update (button, TRUE);
}


static void
pulseaudio_button_update (PulseaudioButton *button,
                          gboolean          force_update)
{
  guint     idx;
  gdouble   volume;
  gboolean  muted;
  gchar    *tip_text;

  g_return_if_fail (IS_PULSEAUDIO_BUTTON (button));

  volume = pulseaudio_volume_get_volume (button->volume);
  muted = pulseaudio_volume_get_muted (button->volume);
  if (muted)
    idx = V_MUTED;
  else if (volume <= 0.0)
    idx = V_MUTED;
  else if (volume <= 0.3)
    idx = V_LOW;
  else if (volume <= 0.7)
    idx = V_MEDIUM;
  else
    idx = V_HIGH;

  if (muted)
    //tip_text = g_strdup_printf (_("Volume %d%% (muted)"), (gint) round (volume * 100));
    tip_text = g_strdup_printf (("Volume %d%% (muted)"), (gint) round (volume * 100));
  else
    //tip_text = g_strdup_printf (_("Volume %d%%"), (gint) round (volume * 100));
    tip_text = g_strdup_printf (("Volume %d%%"), (gint) round (volume * 100));
  gtk_widget_set_tooltip_text (GTK_WIDGET (button), tip_text);
  g_free (tip_text);

  if (force_update || button->pixbuf_idx != idx)
    {
      button->pixbuf_idx = idx;
      xfce_panel_image_set_from_pixbuf (XFCE_PANEL_IMAGE (button->image), button->pixbufs[button->pixbuf_idx]);
    }
}


void
pulseaudio_button_set_size (PulseaudioButton *button,
                            gint              size)
{
  g_return_if_fail (IS_PULSEAUDIO_BUTTON (button));
  g_return_if_fail (size > 0);

  button->icon_size = size - 4;
  gtk_widget_set_size_request (GTK_WIDGET (button), size, size);
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
pulseaudio_button_new (PulseaudioConfig *config,
                       PulseaudioVolume *volume)
{
  PulseaudioButton *button;

  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), NULL);

  button = g_object_new (TYPE_PULSEAUDIO_BUTTON, NULL);

  button->volume = volume;
  button->config = config;
  button->volume_changed_id =
    g_signal_connect_swapped (G_OBJECT (button->volume), "volume-changed",
                              G_CALLBACK (pulseaudio_button_volume_changed), button);

  return GTK_WIDGET (button);
}


