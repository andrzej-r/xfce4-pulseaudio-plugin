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

#ifndef __PULSEAUDIO_PLUGIN_H__
#define __PULSEAUDIO_PLUGIN_H__

#include <glib.h>
#include <gtk/gtk.h>
#include <libxfce4panel/libxfce4panel.h>

G_BEGIN_DECLS
typedef struct _PulseaudioPluginClass PulseaudioPluginClass;
typedef struct _PulseaudioPlugin      PulseaudioPlugin;

#define TYPE_PULSEAUDIO_PLUGIN            (pulseaudio_plugin_get_type ())
#define PULSEAUDIO_PLUGIN(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), TYPE_PULSEAUDIO_PLUGIN, PulseaudioPlugin))
#define PULSEAUDIO_PLUGIN_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  TYPE_PULSEAUDIO_PLUGIN, PulseaudioPluginClass))
#define IS_PULSEAUDIO_PLUGIN(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), TYPE_PULSEAUDIO_PLUGIN))
#define IS_PULSEAUDIO_PLUGIN_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  TYPE_PULSEAUDIO_PLUGIN))
#define PULSEAUDIO_PLUGIN_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  TYPE_PULSEAUDIO_PLUGIN, PulseaudioPluginClass))

GType pulseaudio_plugin_get_type      (void) G_GNUC_CONST;

void  pulseaudio_plugin_register_type (XfcePanelTypeModule *type_module);

G_END_DECLS

#endif /* !__PULSEAUDIO_PLUGIN_H__ */
