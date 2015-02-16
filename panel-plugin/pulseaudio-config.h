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

#ifndef __PULSEAUDIO_CONFIG_H__
#define __PULSEAUDIO_CONFIG_H__

#include <glib.h>
#include <dbus/dbus-glib.h>

G_BEGIN_DECLS

typedef struct _PulseaudioConfigClass PulseaudioConfigClass;
typedef struct _PulseaudioConfig      PulseaudioConfig;

#define TYPE_PULSEAUDIO_CONFIG             (pulseaudio_config_get_type ())
#define PULSEAUDIO_CONFIG(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PULSEAUDIO_CONFIG, PulseaudioConfig))
#define PULSEAUDIO_CONFIG_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_PULSEAUDIO_CONFIG, PulseaudioConfigClass))
#define IS_PULSEAUDIO_CONFIG(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PULSEAUDIO_CONFIG))
#define IS_PULSEAUDIO_CONFIG_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_PULSEAUDIO_CONFIG))
#define PULSEAUDIO_CONFIG_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_PULSEAUDIO_CONFIG, PulseaudioConfigClass))

GType              pulseaudio_config_get_type                       (void)                                       G_GNUC_CONST;

PulseaudioConfig  *pulseaudio_config_new                            (const gchar          *property_base);

gboolean           pulseaudio_config_get_enable_keyboard_shortcuts  (PulseaudioConfig     *config);
guint              pulseaudio_config_get_volume_step                (PulseaudioConfig     *config);

G_END_DECLS

#endif /* !__PULSEAUDIO_CONFIG_H__ */
