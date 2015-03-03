/*
 * Copyright (c) 2015 Guido Berhoerster <guido+xfce@berhoerster.name>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 * Boston, MA 02110-1301, USA.
 */

#ifndef __PULSEAUDIO_DEBUG_H__
#define __PULSEAUDIO_DEBUG_H__

#include <glib.h>
#include <stdarg.h>

G_BEGIN_DECLS

#define pulseaudio_debug(...)  pulseaudio_debug_real (G_LOG_DOMAIN, __FILE__, __func__, __LINE__, __VA_ARGS__)

void pulseaudio_debug_real      (const gchar    *log_domain,
                                 const gchar    *file,
                                 const gchar    *func,
                                 gint            line,
                                 const gchar    *format, ...) G_GNUC_PRINTF(5, 6);

G_END_DECLS

#endif /* !__PULSEAUDIO_DEBUG_H__ */
