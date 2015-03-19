/* -*- c-basic-offset: 2 -*- vi:set ts=2 sts=2 sw=2:
 * * Copyright (C) 2014 Eric Koegel <eric@xfce.org>
 *
 * Licensed under the GNU General Public License Version 2
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */
/*
 * Based on the scale menu item implementation of the indicator applet:
 * Authors:
 *    Cody Russell <crussell@canonical.com>
 * http://bazaar.launchpad.net/~indicator-applet-developers/ido/trunk.14.10/view/head:/src/idoscalemenuitem.h
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "scalemenuitem.h"

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>

/* for DBG/TRACE */
#include <libxfce4util/libxfce4util.h>



static gboolean scale_menu_item_button_press_event      (GtkWidget          *menuitem,
                                                         GdkEventButton     *event);
static gboolean scale_menu_item_button_release_event    (GtkWidget          *menuitem,
                                                         GdkEventButton     *event);
static gboolean scale_menu_item_motion_notify_event     (GtkWidget          *menuitem,
                                                         GdkEventMotion     *event);
static void     scale_menu_item_parent_set              (GtkWidget          *item,
                                                         GtkWidget          *previous_parent);
static void     update_packing                          (ScaleMenuItem  *    self);




struct _ScaleMenuItemPrivate {
  GtkWidget            *scale;
  GtkWidget            *description_label;
  GtkWidget            *percentage_label;
  GtkWidget            *vbox;
  GtkWidget            *hbox;
  gboolean              grabbed;
  gboolean              ignore_value_changed;
};



enum {
  SLIDER_GRABBED,
  SLIDER_RELEASED,
  VALUE_CHANGED,
  LAST_SIGNAL
};




static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE (ScaleMenuItem, scale_menu_item, GTK_TYPE_IMAGE_MENU_ITEM)

#define GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TYPE_SCALE_MENU_ITEM, ScaleMenuItemPrivate))



static void
scale_menu_item_scale_value_changed (GtkRange *range,
                                     gpointer  user_data)
{
  ScaleMenuItem        *self = SCALE_MENU_ITEM (user_data);
  ScaleMenuItemPrivate *priv = GET_PRIVATE (self);

  /* The signal is not sent when it was set through
   * scale_menu_item_set_value().  */

  if (!priv->ignore_value_changed)
    g_signal_emit (self, signals[VALUE_CHANGED], 0, gtk_range_get_value (range));
}

static void
scale_menu_item_class_init (ScaleMenuItemClass *item_class)
{
  GObjectClass      *gobject_class =   G_OBJECT_CLASS      (item_class);
  GtkWidgetClass    *widget_class =    GTK_WIDGET_CLASS    (item_class);

  widget_class->button_press_event   = scale_menu_item_button_press_event;
  widget_class->button_release_event = scale_menu_item_button_release_event;
  widget_class->motion_notify_event  = scale_menu_item_motion_notify_event;
  widget_class->parent_set           = scale_menu_item_parent_set;


  /**
   * ScaleMenuItem::slider-grabbed:
   * @menuitem: The #ScaleMenuItem emitting the signal.
   *
   * The ::slider-grabbed signal is emitted when the pointer selects the slider. 
   */
  signals[SLIDER_GRABBED] = g_signal_new ("slider-grabbed",
                                          G_OBJECT_CLASS_TYPE (gobject_class),
                                          G_SIGNAL_RUN_FIRST,
                                          0,
                                          NULL, NULL,
                                          g_cclosure_marshal_VOID__VOID,
                                          G_TYPE_NONE, 0);

  /**
   * ScaleMenuItem::slider-released:
   * @menuitem: The #ScaleMenuItem emitting the signal.
   *
   * The ::slider-released signal is emitted when the pointer releases the slider.
   */
  signals[SLIDER_RELEASED] = g_signal_new ("slider-released",
                                           G_OBJECT_CLASS_TYPE (gobject_class),
                                           G_SIGNAL_RUN_FIRST,
                                           0,
                                           NULL, NULL,
                                           g_cclosure_marshal_VOID__VOID,
                                           G_TYPE_NONE, 0);


  /**
   * ScaleMenuItem::value-changed:
   * @menuitem: the #ScaleMenuItem for which the value changed
   * @value: the new value
   *
   * Emitted whenever the value of the contained scale changes because
   * of user input.
   */
  signals[VALUE_CHANGED] = g_signal_new ("value-changed",
                                         TYPE_SCALE_MENU_ITEM,
                                         G_SIGNAL_RUN_LAST,
                                         0, NULL, NULL,
                                         g_cclosure_marshal_VOID__DOUBLE,
                                         G_TYPE_NONE,
                                         1, G_TYPE_DOUBLE);


  g_type_class_add_private (item_class, sizeof (ScaleMenuItemPrivate));
}

static void
remove_children (GtkContainer *container)
{
  GList * children;
  GList * l;

  g_return_if_fail (GTK_IS_CONTAINER (container));

  children = gtk_container_get_children (container);

  for (l=children; l!=NULL; l=l->next)
    gtk_container_remove (container, l->data);
  g_list_free (children);
}

static void
update_packing (ScaleMenuItem *self)
{
  ScaleMenuItemPrivate *priv;
  GtkBox               *hbox;
  GtkBox               *vbox;

  g_return_if_fail (IS_SCALE_MENU_ITEM (self));

  priv = GET_PRIVATE (self);
  hbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0));
  vbox = GTK_BOX (gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));

  TRACE("entering");

  if(priv->hbox)
    remove_children (GTK_CONTAINER (priv->hbox));
  if(priv->vbox)
  {
    remove_children (GTK_CONTAINER (priv->vbox));
    gtk_container_remove (GTK_CONTAINER (self), priv->vbox);
  }

  priv->hbox = GTK_WIDGET(hbox);
  priv->vbox = GTK_WIDGET(vbox);

  /* add the new layout */
  if (priv->description_label && priv->percentage_label)
  {
      /* [IC]  Description
       * [ON]  <----slider----> [percentage]%
       */
      gtk_box_pack_start (vbox, priv->description_label, FALSE, FALSE, 0);
      gtk_box_pack_start (vbox, priv->hbox, TRUE, TRUE, 0);
      gtk_box_pack_start (hbox, priv->scale, TRUE, TRUE, 0);
      gtk_box_pack_start (hbox, priv->percentage_label, FALSE, FALSE, 0);
  }
  else if (priv->description_label)
  {
      /* [IC]  Description
       * [ON]  <----slider---->
       */
      gtk_box_pack_start (vbox, priv->description_label, FALSE, FALSE, 0);
      gtk_box_pack_start (vbox, priv->hbox, TRUE, TRUE, 0);
      gtk_box_pack_start (hbox, priv->scale, TRUE, TRUE, 0);
  }
  else if (priv->percentage_label)
  {
      /* [ICON]  <----slider----> [percentage]%  */
      gtk_box_pack_start (vbox, priv->hbox, TRUE, TRUE, 0);
      gtk_box_pack_start (hbox, priv->scale, TRUE, TRUE, 0);
      gtk_box_pack_start (hbox, priv->percentage_label, FALSE, FALSE, 0);
  }
  else
  {
      /* [ICON]  <----slider---->  */
      gtk_box_pack_start (vbox, priv->hbox, TRUE, TRUE, 0);
      gtk_box_pack_start (hbox, priv->scale, TRUE, TRUE, 0);
  }


  gtk_widget_show_all (priv->vbox);
  gtk_widget_show_all (priv->hbox);

  gtk_container_add (GTK_CONTAINER (self), priv->vbox);
}

static void
scale_menu_item_init (ScaleMenuItem *self)
{
}

static gboolean
scale_menu_item_button_press_event (GtkWidget      *menuitem,
                                    GdkEventButton *event)
{
  ScaleMenuItemPrivate *priv;
  GtkAllocation         alloc;
  gint                  x, y;

  TRACE("entering");

  g_return_val_if_fail (IS_SCALE_MENU_ITEM (menuitem), FALSE);

  priv = GET_PRIVATE (menuitem);

  gtk_widget_get_allocation (priv->scale, &alloc);
  gtk_widget_translate_coordinates (menuitem, priv->scale, event->x, event->y, &x, &y);

  if (x > 0 && x < alloc.width && y > 0 && y < alloc.height)
    {
#if !(GTK_CHECK_VERSION (3, 14, 0))
      /* event coordinates are supposed to refer to GdkWindow but for some reason in Gtk+-3.12(?) they refer to the widget */
      event->x = (gdouble) x;
      event->y = (gdouble) y;
#endif
      gtk_widget_event (priv->scale, (GdkEvent*) event);
    }

  if (!priv->grabbed)
    {
      priv->grabbed = TRUE;
      g_signal_emit (menuitem, signals[SLIDER_GRABBED], 0);
    }

  return TRUE;
}

static gboolean
scale_menu_item_button_release_event (GtkWidget      *menuitem,
                                      GdkEventButton *event)
{
  ScaleMenuItemPrivate *priv;
  gint                  x, y;

  TRACE("entering");

  g_return_val_if_fail (IS_SCALE_MENU_ITEM (menuitem), FALSE);

  priv = GET_PRIVATE (menuitem);

#if !(GTK_CHECK_VERSION (3, 14, 0))
  gtk_widget_translate_coordinates (menuitem, priv->scale, event->x, event->y, &x, &y);
  event->x = (gdouble) x;
  event->y = (gdouble) y;
#endif
  gtk_widget_event (priv->scale, (GdkEvent*)event);

  if (priv->grabbed)
    {
      priv->grabbed = FALSE;
      g_signal_emit (menuitem, signals[SLIDER_RELEASED], 0);
    }

  return TRUE;
}

static gboolean
scale_menu_item_motion_notify_event (GtkWidget      *menuitem,
                                     GdkEventMotion *event)
{
  ScaleMenuItemPrivate *priv;
  GtkWidget            *scale;
  GtkAllocation         alloc;
  gint                  x, y;

  g_return_val_if_fail (IS_SCALE_MENU_ITEM (menuitem), FALSE);

  priv = GET_PRIVATE (menuitem);
  scale = priv->scale;

  gtk_widget_get_allocation (priv->scale, &alloc);
  gtk_widget_translate_coordinates (menuitem, priv->scale, event->x, event->y, &x, &y);

  if (x > 0 && x < alloc.width && y > 0 && y < alloc.height)
    {
#if !(GTK_CHECK_VERSION (3, 14, 0))
      event->x = (gdouble) x;
      event->y = (gdouble) y;
#endif
      gtk_widget_event (scale, (GdkEvent*) event);
    }

  return TRUE;
}

static void
menu_hidden (GtkWidget     *menu,
             ScaleMenuItem *scale)
{
  ScaleMenuItemPrivate *priv;

  g_return_if_fail (IS_SCALE_MENU_ITEM (scale));
  g_return_if_fail (GTK_IS_MENU (menu));

  priv = GET_PRIVATE (scale);

  if (priv->grabbed)
    {
      priv->grabbed = FALSE;
      g_signal_emit (scale, signals[SLIDER_RELEASED], 0);
    }
}

static void
scale_menu_item_parent_set (GtkWidget *item,
                            GtkWidget *previous_parent)

{
  GtkWidget *parent;

  g_return_if_fail (IS_SCALE_MENU_ITEM (item));

  if (previous_parent != NULL)
    {
      g_signal_handlers_disconnect_by_func (previous_parent, menu_hidden, item);
    }

  parent = gtk_widget_get_parent (item);

  if (parent != NULL)
    {
      g_signal_connect (parent, "hide", G_CALLBACK (menu_hidden), item);
    }
}



GtkWidget*
scale_menu_item_new_with_range (gdouble           min,
                                gdouble           max,
                                gdouble           step)
{
  ScaleMenuItem        *scale_item;
  ScaleMenuItemPrivate *priv;

  TRACE("entering");

  scale_item = SCALE_MENU_ITEM (g_object_new (TYPE_SCALE_MENU_ITEM, NULL));

  priv = GET_PRIVATE (scale_item);

  priv->scale = gtk_scale_new_with_range (GTK_ORIENTATION_HORIZONTAL, min, max, step);
  priv->vbox = NULL;
  priv->hbox = NULL;

  g_signal_connect (priv->scale, "value-changed", G_CALLBACK (scale_menu_item_scale_value_changed), scale_item);
  g_object_ref (priv->scale);
  gtk_widget_set_size_request (priv->scale, 100, -1);
  gtk_range_set_inverted (GTK_RANGE(priv->scale), FALSE);
  gtk_scale_set_draw_value (GTK_SCALE(priv->scale), FALSE);

  update_packing (scale_item);

  gtk_widget_add_events (GTK_WIDGET(scale_item), GDK_SCROLL_MASK|GDK_POINTER_MOTION_MASK|GDK_BUTTON_MOTION_MASK);

  return GTK_WIDGET(scale_item);
}

/**
 * scale_menu_item_get_scale:
 * @menuitem: The #ScaleMenuItem
 *
 * Retrieves the scale widget.
 *
 * Return Value: (transfer none)
 **/
GtkWidget*
scale_menu_item_get_scale (ScaleMenuItem *menuitem)
{
  ScaleMenuItemPrivate *priv;

  g_return_val_if_fail (IS_SCALE_MENU_ITEM (menuitem), NULL);

  priv = GET_PRIVATE (menuitem);

  return priv->scale;
}

/**
 * scale_menu_item_get_description_label:
 * @menuitem: The #ScaleMenuItem
 *
 * Retrieves a string of the text for the description label widget.
 *
 * Return Value: The label text.
 **/
const gchar*
scale_menu_item_get_description_label (ScaleMenuItem *menuitem)
{
  ScaleMenuItemPrivate *priv;

  g_return_val_if_fail (IS_SCALE_MENU_ITEM (menuitem), NULL);

  priv = GET_PRIVATE (menuitem);

  return gtk_label_get_text (GTK_LABEL (priv->description_label));
}

/**
 * scale_menu_item_get_percentage_label:
 * @menuitem: The #ScaleMenuItem
 *
 * Retrieves a string of the text for the percentage label widget.
 *
 * Return Value: The label text.
 **/
const gchar*
scale_menu_item_get_percentage_label (ScaleMenuItem *menuitem)
{
  ScaleMenuItemPrivate *priv;

  g_return_val_if_fail (IS_SCALE_MENU_ITEM (menuitem), NULL);

  priv = GET_PRIVATE (menuitem);

  return gtk_label_get_text (GTK_LABEL (priv->percentage_label));
}

/**
 * scale_menu_item_set_description_label:
 * @menuitem: The #ScaleMenuItem
 * @label: The label text
 *
 * Sets the text for the description label widget. If label is NULL
 * then the description label is removed from the #ScaleMenuItem.
 **/
void
scale_menu_item_set_description_label (ScaleMenuItem *menuitem,
                                       const gchar   *label)
{
  ScaleMenuItemPrivate *priv;

  g_return_if_fail (IS_SCALE_MENU_ITEM (menuitem));

  priv = GET_PRIVATE (menuitem);

  if (label == NULL && priv->description_label)
    {
      /* remove label */
      g_object_unref (priv->description_label);
      priv->description_label = NULL;
      return;
    }

  if (priv->description_label && label)
    {
      gtk_label_set_markup (GTK_LABEL (priv->description_label), label);
    }
  else if(label)
    {
      /* create label */
      priv->description_label = gtk_label_new (NULL);
      gtk_label_set_markup (GTK_LABEL (priv->description_label), label);

      /* align left */
      gtk_misc_set_alignment (GTK_MISC(priv->description_label), 0, 0);
    }

    update_packing (menuitem);
}


/**
 * scale_menu_item_set_percentage_label:
 * @menuitem: The #ScaleMenuItem
 * @label: The label text
 *
 * Sets the text for the percentage label widget. If label is NULL
 * then the percentage label is removed from the #ScaleMenuItem.
 **/
void
scale_menu_item_set_percentage_label (ScaleMenuItem *menuitem,
                                      const gchar   *label)
{
  ScaleMenuItemPrivate *priv;

  g_return_if_fail (IS_SCALE_MENU_ITEM (menuitem));

  priv = GET_PRIVATE (menuitem);

  if (label == NULL && priv->percentage_label)
    {
      /* remove label */
      g_object_unref (priv->percentage_label);
      priv->percentage_label = NULL;
      return;
    }

  if (priv->percentage_label && label)
    {
      gtk_label_set_text (GTK_LABEL (priv->percentage_label), label);
    }
  else if(label)
    {
      /* create label */
      priv->percentage_label = gtk_label_new (label);
      /* align left */
      gtk_misc_set_alignment (GTK_MISC(priv->percentage_label), 0, 0);
    }

    update_packing (menuitem);
}


/**
 *  scale_menu_item_set_value:
 *
 * Sets the value of the scale inside @item to @value, without emitting
 * "value-changed".
 */
void
scale_menu_item_set_value (ScaleMenuItem *item,
                           gdouble        value)
{
  ScaleMenuItemPrivate *priv;

  g_return_if_fail (IS_SCALE_MENU_ITEM (item));

  priv = GET_PRIVATE (item);

  /* set ignore_value_changed to signify to the scale menu item that it
   * should not emit its own value-changed signal, as that should only
   * be emitted when the value is changed by the user. */

  priv->ignore_value_changed = TRUE;
  gtk_range_set_value (GTK_RANGE (priv->scale), value);
  priv->ignore_value_changed = FALSE;
}
