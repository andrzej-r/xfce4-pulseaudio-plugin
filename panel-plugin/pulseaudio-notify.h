/*  Copyright (c) 2015 Andrzej <ndrwrdck@gmail.com>
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

#ifndef __PULSEAUDIO_NOTIFY_H__
#define __PULSEAUDIO_NOTIFY_H__

#ifdef HAVE_LIBNOTIFY

#include <glib-object.h>
#include "pulseaudio-config.h"
#include "pulseaudio-volume.h"

G_BEGIN_DECLS

#define TYPE_PULSEAUDIO_NOTIFY             (pulseaudio_notify_get_type ())
#define PULSEAUDIO_NOTIFY(obj)             (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PULSEAUDIO_NOTIFY, PulseaudioNotify))
#define PULSEAUDIO_NOTIFY_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_PULSEAUDIO_NOTIFY, PulseaudioNotifyClass))
#define IS_PULSEAUDIO_NOTIFY(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PULSEAUDIO_NOTIFY))
#define IS_PULSEAUDIO_NOTIFY_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_PULSEAUDIO_NOTIFY))
#define PULSEAUDIO_NOTIFY_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_PULSEAUDIO_NOTIFY, PulseaudioNotifyClass))

typedef struct          _PulseaudioNotify                 PulseaudioNotify;
typedef struct          _PulseaudioNotifyClass            PulseaudioNotifyClass;

GType                   pulseaudio_notify_get_type        (void) G_GNUC_CONST;

PulseaudioNotify       *pulseaudio_notify_new             (PulseaudioConfig *config,
                                                           PulseaudioVolume *volume);

void                    pulseaudio_notify_notify          (PulseaudioNotify *notify);

G_END_DECLS

#endif /* HAVE_LIBNOTIFY */
#endif /* !__PULSEAUDIO_NOTIFY_H__ */
