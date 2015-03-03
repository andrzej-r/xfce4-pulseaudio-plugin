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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib.h>
#include <stdarg.h>

#include "pulseaudio-debug.h"

void
pulseaudio_debug_real (const gchar *log_domain,
                       const gchar *file,
                       const gchar *func,
                       gint line,
                       const gchar *format, ...)
{
  va_list  args;
  gchar   *prefixed_format;

  va_start (args, format);
  prefixed_format = g_strdup_printf ("[%s:%d %s]: %s", file, line, func, format);
  g_logv (log_domain, G_LOG_LEVEL_DEBUG, prefixed_format, args);
  va_end (args);

  g_free (prefixed_format);
}
