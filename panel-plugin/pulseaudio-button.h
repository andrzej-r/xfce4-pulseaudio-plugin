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

#ifndef __PULSEAUDIO_BUTTON_H__
#define __PULSEAUDIO_BUTTON_H__

#include <glib.h>
#include <gtk/gtk.h>

#include "pulseaudio-volume.h"
//#include "pulseaudio-config.h"

G_BEGIN_DECLS

GType pulseaudio_button_get_type (void);

#define TYPE_PULSEAUDIO_BUTTON             (pulseaudio_button_get_type())
#define PULSEAUDIO_BUTTON(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_PULSEAUDIO_BUTTON, PulseaudioButton))
#define PULSEAUDIO_BUTTON_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST((klass),  TYPE_PULSEAUDIO_BUTTON, PulseaudioButtonClass))
#define IS_PULSEAUDIO_BUTTON(obj)          (G_TYPE_CHECK_INSTANCE_TYPE((obj), TYPE_PULSEAUDIO_BUTTON))
#define IS_PULSEAUDIO_BUTTON_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE((klass),  TYPE_PULSEAUDIO_BUTTON))
#define PULSEAUDIO_BUTTON_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS((obj),  TYPE_PULSEAUDIO_BUTTON, PulseaudioButtonClass))

typedef struct          _PulseaudioButton              PulseaudioButton;
typedef struct          _PulseaudioButtonClass         PulseaudioButtonClass;

GtkWidget              *pulseaudio_button_new         (PulseaudioVolume *volume);

void                    pulseaudio_button_set_size    (PulseaudioButton *button,
                                                       gint              size);

G_END_DECLS

#endif /* !__PULSEAUDIO_BUTTON_H__ */