/*  Copyright (c) 2014 Andrzej <ndrwrdck@gmail.com>
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
 *  This file implements a pulseaudio volume class abstracting out
 *  operations on pulseaudio mixer.
 *
 */



#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <pulse/pulseaudio.h>
#include <pulse/glib-mainloop.h>

#include "pulseaudio-debug.h"
#include "pulseaudio-volume.h"


static void                 pulseaudio_volume_finalize        (GObject            *object);
static void                 pulseaudio_volume_connect         (PulseaudioVolume   *volume);
static gdouble              pulseaudio_volume_v2d             (pa_volume_t         vol);


struct _PulseaudioVolume
{
  GObject               __parent__;

  pa_glib_mainloop     *pa_mainloop;
  pa_context           *pa_context;
  gboolean              connected;

  gdouble               volume;
  gboolean              muted;

  gdouble               volume_mic;
  gboolean              muted_mic;


};

struct _PulseaudioVolumeClass
{
  GObjectClass          __parent__;
};




enum
{
  VOLUME_CHANGED,
  LAST_SIGNAL
};

static guint pulseaudio_volume_signals[LAST_SIGNAL] = { 0, };




G_DEFINE_TYPE (PulseaudioVolume, pulseaudio_volume, G_TYPE_OBJECT)

static void
pulseaudio_volume_class_init (PulseaudioVolumeClass *klass)
{
  GObjectClass      *gobject_class;

  gobject_class = G_OBJECT_CLASS (klass);
  gobject_class->finalize = pulseaudio_volume_finalize;

  pulseaudio_volume_signals[VOLUME_CHANGED] =
    g_signal_new (g_intern_static_string ("volume-changed"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  g_cclosure_marshal_VOID__VOID,
                  G_TYPE_NONE, 0);

}



static void
pulseaudio_volume_init (PulseaudioVolume *volume)
{
  volume->connected = FALSE;
  volume->volume = 0.0;
  volume->muted = FALSE;

  volume->pa_mainloop = pa_glib_mainloop_new (NULL);

  pulseaudio_volume_connect (volume);
}



static void
pulseaudio_volume_finalize (GObject *object)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (object);

  pa_glib_mainloop_free (volume->pa_mainloop);

  (*G_OBJECT_CLASS (pulseaudio_volume_parent_class)->finalize) (object);
}




/* sink event callbacks */
static void
pulseaudio_volume_sink_info_cb (pa_context         *context,
                                const pa_sink_info *i,
                                int                 eol,
                                void               *userdata)
{
  gboolean  muted;
  gdouble   vol;

  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);
  if (i == NULL) return;

  muted = (gboolean) i->mute;
  vol = pulseaudio_volume_v2d (i->volume.values[0]);

  if (volume->muted != muted)
    {
      pulseaudio_debug ("Updated Mute: %d -> %d", volume->muted, muted);
      volume->muted = muted;
      g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_CHANGED], 0);
    }

  if (ABS (volume->volume - vol) > 2e-3)
    {
      pulseaudio_debug ("Updated Volume: %04.3f -> %04.3f", volume->volume, vol);
      volume->volume = vol;
      g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_CHANGED], 0);
    }
}



static void
pulseaudio_volume_server_info_cb (pa_context           *context,
                                  const pa_server_info *i,
                                  void                 *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);
  if (i == NULL) return;

  pulseaudio_debug ("default sink name = %s\n", i->default_sink_name);
  pa_context_get_sink_info_by_name (context, i->default_sink_name, pulseaudio_volume_sink_info_cb, volume);
}




static void
pulseaudio_volume_sink_check (PulseaudioVolume *volume,
                              pa_context       *context)
{
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));

  pa_context_get_server_info (context, pulseaudio_volume_server_info_cb, volume);
}




static void
pulseaudio_volume_subscribe_cb (pa_context                   *context,
                                pa_subscription_event_type_t  t,
                                uint32_t                      idx,
                                void                         *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK)
    {
    case PA_SUBSCRIPTION_EVENT_SINK          :
      pulseaudio_volume_sink_check (volume, context);
      pulseaudio_debug ("PulseAudio sink event");
      break;

    case PA_SUBSCRIPTION_EVENT_SOURCE        :
      pulseaudio_debug ("PulseAudio source event");
      break;

    case PA_SUBSCRIPTION_EVENT_SOURCE_OUTPUT :
      pulseaudio_debug ("PulseAudio source output event");
      break;

    default                                  :
      pulseaudio_debug ("Unknown PulseAudio event");
      break;
    }
}




static void
pulseaudio_volume_context_state_cb (pa_context *context,
                                    void       *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  switch (pa_context_get_state (context))
    {
    case PA_CONTEXT_READY        :
      pa_context_subscribe (context, PA_SUBSCRIPTION_MASK_SINK | PA_SUBSCRIPTION_MASK_SOURCE | PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT, NULL, NULL);
      pa_context_set_subscribe_callback (context, pulseaudio_volume_subscribe_cb, volume);

      pulseaudio_debug ("PulseAudio connection established");
      volume->connected = TRUE;
      pulseaudio_volume_sink_check (volume, context);
      break;

    case PA_CONTEXT_FAILED       :
    case PA_CONTEXT_TERMINATED   :
      g_warning ("Disconected from PulseAudio server");
      break;

    case PA_CONTEXT_CONNECTING   :
      pulseaudio_debug ("Connecting to PulseAudio server");
      break;

    case PA_CONTEXT_SETTING_NAME :
      pulseaudio_debug ("Setting application name");
      break;

    case PA_CONTEXT_AUTHORIZING  :
      pulseaudio_debug ("Authorizing");
      break;

    case PA_CONTEXT_UNCONNECTED  :
      pulseaudio_debug ("Not connected to PulseAudio server");
      break;

    default                      :
      g_warning ("Unknown pulseaudio context state");
      break;
    }
}



static void
pulseaudio_volume_connect (PulseaudioVolume *volume)
{
  pa_proplist  *proplist;
  gint          err;

  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));
  g_return_if_fail (!volume->connected);

  proplist = pa_proplist_new ();
#ifdef HAVE_CONFIG_H
  pa_proplist_sets (proplist, PA_PROP_APPLICATION_NAME, PACKAGE_NAME);
  pa_proplist_sets (proplist, PA_PROP_APPLICATION_VERSION, PACKAGE_VERSION);
  pa_proplist_sets (proplist, PA_PROP_APPLICATION_ID, "org.xfce.pulseaudio-plugin");
  pa_proplist_sets (proplist, PA_PROP_APPLICATION_ICON_NAME, "multimedia-volume-control");
#endif

  volume->pa_context = pa_context_new_with_proplist (pa_glib_mainloop_get_api (volume->pa_mainloop), NULL, proplist);
  pa_context_set_state_callback(volume->pa_context, pulseaudio_volume_context_state_cb, volume);

  err = pa_context_connect (volume->pa_context, NULL, PA_CONTEXT_NOFAIL, NULL);
  if (err < 0)
    g_warning ("pa_context_connect() failed: %s", pa_strerror (err));
  //g_warning ("pa_context_connect() failed: %s", pa_strerror (pa_context_errno (volume->pa_context)));
}




static gdouble
pulseaudio_volume_v2d (pa_volume_t vol)
{
  gdouble volume;

  volume = (gdouble) vol - PA_VOLUME_MUTED;
  volume /= (gdouble) (PA_VOLUME_NORM - PA_VOLUME_MUTED);
  /* for safety */
  volume = MIN (MAX (volume, 0.0), 1.0);
  return volume;
}



static pa_volume_t
pulseaudio_volume_d2v (gdouble vol)
{
  gdouble volume;

  volume = (PA_VOLUME_NORM - PA_VOLUME_MUTED) * vol;
  volume = (pa_volume_t) volume + PA_VOLUME_MUTED;
  /* for safety */
  volume = MIN (MAX (volume, PA_VOLUME_MUTED), PA_VOLUME_NORM);
  return volume;
}




gboolean
pulseaudio_volume_get_muted (PulseaudioVolume *volume)
{
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), FALSE);

  return volume->muted;
}



/* final callback for volume/mute changes */
/* pa_context_success_cb_t */
static void
pulseaudio_volume_sink_volume_changed (pa_context *context,
                                       int         success,
                                       void       *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);

  if (success)
    g_signal_emit (G_OBJECT (volume), pulseaudio_volume_signals [VOLUME_CHANGED], 0);
}

/* mute setting callbacks */
/* pa_sink_info_cb_t */
static void
pulseaudio_volume_set_muted_cb1 (pa_context         *context,
                                 const pa_sink_info *i,
                                 int                 eol,
                                 void               *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);
  if (i == NULL) return;

  pa_context_set_sink_mute_by_index (context, i->index, volume->muted, pulseaudio_volume_sink_volume_changed, volume);
}



void
pulseaudio_volume_set_muted (PulseaudioVolume *volume,
                             gboolean          muted)
{
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));
  g_return_if_fail (pa_context_get_state (volume->pa_context) == PA_CONTEXT_READY);

  if (volume->muted != muted)
    {
      volume->muted = muted;
      pa_context_get_sink_info_list (volume->pa_context, pulseaudio_volume_set_muted_cb1, volume);
    }
}



void
pulseaudio_volume_toggle_muted (PulseaudioVolume *volume)
{
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));

  pulseaudio_volume_set_muted (volume, !volume->muted);
}




gdouble
pulseaudio_volume_get_volume (PulseaudioVolume *volume)
{
  g_return_val_if_fail (IS_PULSEAUDIO_VOLUME (volume), 0.0);

  return volume->volume;
}



/* volume setting callbacks */
/* pa_sink_info_cb_t */
static void
pulseaudio_volume_set_volume_cb2 (pa_context         *context,
                                  const pa_sink_info *i,
                                  int                 eol,
                                  void               *userdata)
{
  //char st[PA_CVOLUME_SNPRINT_MAX];

  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);
  if (i == NULL) return;

  //pulseaudio_debug ("*** %s", pa_cvolume_snprint (st, sizeof (st), &i->volume));
  pa_cvolume_set (&i->volume, 1, pulseaudio_volume_d2v (volume->volume));
  pa_context_set_sink_volume_by_index (context, i->index, &i->volume, pulseaudio_volume_sink_volume_changed, volume);
}



/* pa_server_info_cb_t */
static void
pulseaudio_volume_set_volume_cb1 (pa_context           *context,
                                  const pa_server_info *i,
                                  void                 *userdata)
{
  PulseaudioVolume *volume = PULSEAUDIO_VOLUME (userdata);
  if (i == NULL) return;

  pa_context_get_sink_info_by_name (context, i->default_sink_name, pulseaudio_volume_set_volume_cb2, volume);
}


void
pulseaudio_volume_set_volume (PulseaudioVolume *volume,
                              gdouble           vol)
{
  g_return_if_fail (IS_PULSEAUDIO_VOLUME (volume));
  g_return_if_fail (pa_context_get_state (volume->pa_context) == PA_CONTEXT_READY);

  if (volume->volume != vol)
    {
      volume->volume = vol;
      pa_context_get_server_info (volume->pa_context, pulseaudio_volume_set_volume_cb1, volume);
    }
}



PulseaudioVolume *
pulseaudio_volume_new (void)
{
  PulseaudioVolume *volume = g_object_new (TYPE_PULSEAUDIO_VOLUME, NULL);

  return volume;
}


