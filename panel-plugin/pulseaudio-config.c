/*
 *  Copyright (C) 2014 Andrzej <ndrwrdck@gmail.com>
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
#define DEFAULT_VOLUME_STEP                       6
#define DEFAULT_VOLUME_MAX                        153



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
  GObjectClass     __parent__;
};

struct _PulseaudioConfig
{
  GObject          __parent__;

  gboolean         enable_keyboard_shortcuts;
  guint            volume_step;
  guint            volume_max;
  gchar           *mixer_command;
};



enum
  {
    PROP_0,
    PROP_ENABLE_KEYBOARD_SHORTCUTS,
    PROP_VOLUME_STEP,
    PROP_VOLUME_MAX,
    PROP_MIXER_COMMAND,
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



  g_object_class_install_property (gobject_class,
                                   PROP_VOLUME_STEP,
                                   g_param_spec_uint ("volume-step", NULL, NULL,
                                                      1, 50, DEFAULT_VOLUME_STEP,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_STATIC_STRINGS));



  g_object_class_install_property (gobject_class,
                                   PROP_VOLUME_MAX,
                                   g_param_spec_uint ("volume-max", NULL, NULL,
                                                      1, 300, DEFAULT_VOLUME_MAX,
                                                      G_PARAM_READWRITE |
                                                      G_PARAM_STATIC_STRINGS));



  g_object_class_install_property (gobject_class,
                                   PROP_MIXER_COMMAND,
                                   g_param_spec_string ("mixer-command",
                                                        NULL, NULL,
                                                        DEFAULT_MIXER_COMMAND,
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
  config->volume_step               = DEFAULT_VOLUME_STEP;
  config->volume_max                = DEFAULT_VOLUME_MAX;
  config->mixer_command             = g_strdup (DEFAULT_MIXER_COMMAND);
}



static void
pulseaudio_config_finalize (GObject *object)
{
  PulseaudioConfig *config = PULSEAUDIO_CONFIG (object);

  xfconf_shutdown();
  g_free (config->mixer_command);

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

    case PROP_VOLUME_STEP:
      g_value_set_uint (value, config->volume_step);
      break;

    case PROP_VOLUME_MAX:
      g_value_set_uint (value, config->volume_max);
      break;

    case PROP_MIXER_COMMAND:
      g_value_set_string (value, config->mixer_command);
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
  guint                 val_uint;
  gboolean              val_bool;

  switch (prop_id)
    {
    case PROP_ENABLE_KEYBOARD_SHORTCUTS:
      val_bool = g_value_get_boolean (value);
      if (config->enable_keyboard_shortcuts != val_bool)
        {
          config->enable_keyboard_shortcuts = val_bool;
          g_object_notify (G_OBJECT (config), "enable-keyboard-shortcuts");
          g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
        }
      break;

    case PROP_VOLUME_STEP:
      val_uint = g_value_get_uint (value);
      if (config->volume_step != val_uint)
        {
          config->volume_step = val_uint;
          g_object_notify (G_OBJECT (config), "volume-step");
          g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
        }
      break;

    case PROP_VOLUME_MAX:
      val_uint = g_value_get_uint (value);
      if (config->volume_max != val_uint)
        {
          config->volume_max = val_uint;
          g_object_notify (G_OBJECT (config), "volume-max");
          g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
        }
      break;

    case PROP_MIXER_COMMAND:
      g_free (config->mixer_command);
      config->mixer_command = g_value_dup_string (value);
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




guint
pulseaudio_config_get_volume_step (PulseaudioConfig *config)
{
  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), DEFAULT_VOLUME_STEP);

  return config->volume_step;
}




guint
pulseaudio_config_get_volume_max (PulseaudioConfig *config)
{
  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), DEFAULT_VOLUME_MAX);

  return config->volume_max;
}




const gchar *
pulseaudio_config_get_mixer_command (PulseaudioConfig *config)
{
  g_return_val_if_fail (IS_PULSEAUDIO_CONFIG (config), DEFAULT_MIXER_COMMAND);

  return config->mixer_command;
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

      property = g_strconcat (property_base, "/volume-step", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_UINT, config, "volume-step");
      g_free (property);

      property = g_strconcat (property_base, "/volume-max", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_UINT, config, "volume-max");
      g_free (property);

      property = g_strconcat (property_base, "/mixer-command", NULL);
      xfconf_g_property_bind (channel, property, G_TYPE_STRING, config, "mixer-command");
      g_free (property);

      g_object_notify (G_OBJECT (config), "enable-keyboard-shortcuts");
      g_signal_emit (G_OBJECT (config), pulseaudio_config_signals [CONFIGURATION_CHANGED], 0);
    }

  return config;
}
