/*
 *  Copyright (C) 2014 Andrzej <ndrwrdck@gmail.com>
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
 *  This file implements a configuration store. The class extends GObject.
 *
 */



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include <libxfce4util/libxfce4util.h>
#include <libxfce4ui/libxfce4ui.h>
#include <xfconf/xfconf.h>
//#include <exo/exo.h>
#include <libxfce4panel/xfce-panel-plugin.h>

#include "pulseaudio-plugin.h"
#include "pulseaudio-config.h"




#define DEFAULT_ENABLE_KEYBOARD_SHORTCUTS         TRUE




static void                 pulseaudio_config_finalize       (GObject          *object);
static void                 pulseaudio_config_get_property   (GObject          *object,
                                                              guint             prop_id,
                                                              GValue           *value,
                                                              GParamSpec       *pspec);
static void                 pulseaudio_config_set_property   (GObject          *object,
                                                              guint             prop_id,
                                                              const GValue     *value,
                                                              GParamSpec       *pspec);



struct _PulseaudioConfigClass
{
  GObjectClass      __parent__;
};

struct _PulseaudioConfig
{
  GObject          __parent__;

  gboolean         enable_keyboard_shortcuts;
};



enum
  {
    PROP_0,
    PROP_ENABLE_KEYBOARD_SHORTCUTS,
    N_PROPERTIES,
  };

enum
  {
    CONFIGURATION_CHANGED,
    LAST_SIGNAL
  };

static guint pulseaudio_config_signals [LAST_SIGNAL] = { 0, };


G_DEFINE_TYPE (PulseaudioConfig, pulseaudio_config, G_TYPE_OBJECT)



static void
pulseaudio_config_class_init (PulseaudioConfigClass *klass)
{
  GObjectClass                 *gobject_class;

  gobject_class               = G_OBJECT_CLASS (klass);
  gobject_class->finalize     = pulseaudio_config_finalize;
  gobject_class->get_property = pulseaudio_config_get_property;
  gobject_class->set_property = pulseaudio_config_set_property;

  g_object_class_install_property (gobject_class,
                                   PROP_ENABLE_KEYBOARD_SHORTCUTS,
                                   g_param_spec_boolean ("enable-keyboard-shortcuts", NULL, NULL,
                                                         DEFAULT_ENABLE_KEYBOARD_SHORTCUTS,
                                                         G_PARAM_READWRITE |
                                                         G_PARAM_STATIC_STRINGS));



  pulseaudio_config_signals[CONFIGURATION_CHANGED] =
    g_signal_new (g_intern_static_string ("configuration-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);
}



static void
pulseaudio_config_init (PulseaudioConfig *config)
{
  config->enable_keyboard_shortcuts = DEFAULT_ENABLE_KEYBOARD_SHORTCUTS;
}



static void
pulseaudio_config_finalize (GObject *object)
{
  //PulseaudioConfig *config = PULSEAUDIO_CONFIG (object);

  xfconf_shutdown();

  G_OBJECT_CLASS (pulseaudio_config_parent_class)->finalize (object);
}



static void
pulseaudio_config_get_property (GObject    *object,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  PulseaudioConfig     *config = PULSEAUDIO_CONFIG (object);

  switch (prop_id)
    {
    case PROP_ENABLE_KEYBOARD_SHORTCUTS:
      g_value_set_boolean (value, config->enable_keyboard_shortcuts);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}



static void
pulseaudio_config_set_property (GObject      *object,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  PulseaudioConfig     *config = PULSEAUDIO_CONFIG (object);
  gint                  val;

  switch (prop_id)
    {
    case PROP_ENABLE_KEYBOARD_SHORTCUTS:
      val = g_value_get_boolean (value);
      if (config->enable_keyboard_shortcuts != val)
        {
          config->enable_keyboard_shortcuts = val;
          printf ("********** NOTIFY %d\n", val);
          g_object_notify (G_OBJECT (config), "enable-keyboard-shortcuts");
          g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
        }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}




gboolean
pulseaudio_config_get_enable_keyboard_shortcuts (PulseaudioConfig *config)
{
  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), DEFAULT_ENABLE_KEYBOARD_SHORTCUTS);

  return config->enable_keyboard_shortcuts;
}




PulseaudioConfig *
pulseaudio_config_new (const gchar     *property_base)
{
  PulseaudioConfig    *config;
  XfconfChannel       *channel;
  gchar               *property;

  config = g_object_new (TYPE_PULSEAUDIO_CONFIG, NULL);

  if (xfconf_init (NULL))
    {
      channel = xfconf_channel_get ("xfce4-panel");

      property = g_strconcat (property_base, "/enable-keyboard-shortcuts", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_BOOLEAN, config, "enable-keyboard-shortcuts");
      g_free (property);

      g_object_notify (G_OBJECT (config), "enable-keyboard-shortcuts");
      g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
    }

  return config;
}
